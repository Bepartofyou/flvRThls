
/*
 * Copyright (C) Roman Arutyunyan
 */

#include <stdio.h>
#include "mpegts.h"

#define NGX_RTMP_HLS_BUFSIZE            (1024*1024)
#define NGX_RTMP_HLS_DIR_ACCESS         0744


typedef struct {
    uint64_t                            id;
    uint64_t                            key_id;
    double                              duration;
    unsigned                            active:1;
    unsigned                            discont:1; /* before */
} ngx_rtmp_hls_frag_t;

typedef struct {
    unsigned                            opened:1;

    ngx_rtmp_mpegts_file_t              file;

	uint64_t                            frag;
	uint64_t                            frag_ts;

    ngx_uint_t                          audio_cc;
    ngx_uint_t                          video_cc;
    ngx_uint_t                          key_frags;

    uint64_t                            aframe_base;
    uint64_t                            aframe_num;

    ngx_buf_t                          *aframe;
    uint64_t                            aframe_pts;

} ngx_rtmp_hls_ctx_t;


typedef struct {
    ngx_flag_t                          hls;

	ngx_uint_t                          muxdelay;
	ngx_uint_t                          sync;
    ngx_uint_t                          slicing;
    ngx_uint_t                          type;

	ngx_uint_t                          max_audio_delay;
    size_t                              audio_buffer_size;

} ngx_rtmp_hls_app_conf_t;


typedef struct {
	ngx_uint_t                  width;
	ngx_uint_t                  height;
	ngx_uint_t                  duration;
	ngx_uint_t                  frame_rate;
	ngx_uint_t                  video_data_rate;
	ngx_uint_t                  video_codec_id;
	ngx_uint_t                  audio_data_rate;
	ngx_uint_t                  audio_codec_id;
	ngx_uint_t                  aac_profile;
	ngx_uint_t                  aac_chan_conf;
	ngx_uint_t                  aac_sbr;
	ngx_uint_t                  aac_ps;
	ngx_uint_t                  avc_profile;
	ngx_uint_t                  avc_compat;
	ngx_uint_t                  avc_level;
	ngx_uint_t                  avc_nal_bytes;
	ngx_uint_t                  avc_ref_frames;
	ngx_uint_t                  sample_rate;    /* 5512, 11025, 22050, 44100 */
	ngx_uint_t                  sample_size;    /* 1=8bit, 2=16bit */
	ngx_uint_t                  audio_channels; /* 1, 2 */
	u_char                      profile[32];
	u_char                      level[32];

	ngx_chain_t                *avc_header;
	ngx_chain_t                *aac_header;

	ngx_chain_t                *meta;
	ngx_uint_t                  meta_version;
} ngx_rtmp_codec_ctx_t;

typedef struct {
	uint32_t                csid;       /* chunk stream id */
	uint32_t                timestamp;  /* timestamp (delta) */
	uint32_t                mlen;       /* message length */
	uint8_t                 type;       /* message type id */
	uint32_t                msid;       /* message stream id */
} ngx_rtmp_header_t;

#define NGX_RTMP_HLS_NAMING_SEQUENTIAL  1
#define NGX_RTMP_HLS_NAMING_TIMESTAMP   2
#define NGX_RTMP_HLS_NAMING_SYSTEM      3


#define NGX_RTMP_HLS_SLICING_PLAIN      1
#define NGX_RTMP_HLS_SLICING_ALIGNED    2


#define NGX_RTMP_HLS_TYPE_LIVE          1
#define NGX_RTMP_HLS_TYPE_EVENT         2

/* Audio codecs */
enum {
	/* Uncompressed codec id is actually 0,
	* but we use another value for consistency */
	NGX_RTMP_AUDIO_UNCOMPRESSED = 16,
	NGX_RTMP_AUDIO_ADPCM = 1,
	NGX_RTMP_AUDIO_MP3 = 2,
	NGX_RTMP_AUDIO_LINEAR_LE = 3,
	NGX_RTMP_AUDIO_NELLY16 = 4,
	NGX_RTMP_AUDIO_NELLY8 = 5,
	NGX_RTMP_AUDIO_NELLY = 6,
	NGX_RTMP_AUDIO_G711A = 7,
	NGX_RTMP_AUDIO_G711U = 8,
	NGX_RTMP_AUDIO_AAC = 10,
	NGX_RTMP_AUDIO_SPEEX = 11,
	NGX_RTMP_AUDIO_MP3_8 = 14,
	NGX_RTMP_AUDIO_DEVSPEC = 15,
};


/* Video codecs */
enum {
	NGX_RTMP_VIDEO_JPEG = 1,
	NGX_RTMP_VIDEO_SORENSON_H263 = 2,
	NGX_RTMP_VIDEO_SCREEN = 3,
	NGX_RTMP_VIDEO_ON2_VP6 = 4,
	NGX_RTMP_VIDEO_ON2_VP6_ALPHA = 5,
	NGX_RTMP_VIDEO_SCREEN2 = 6,
	NGX_RTMP_VIDEO_H264 = 7
};


static ngx_int_t
ngx_rtmp_hls_append_aud(ngx_buf_t *out);


static uint64_t
ngx_rtmp_hls_get_fragment_id(int flag, uint64_t ts);


static ngx_int_t
ngx_rtmp_hls_open_fragment(uint64_t ts,
ngx_int_t discont);

static ngx_int_t
ngx_rtmp_hls_flush_audio(ngx_rtmp_hls_ctx_t *ctx);


static ngx_rtmp_hls_frag_t *
ngx_rtmp_hls_get_frag(ngx_int_t n);

static void
ngx_rtmp_hls_next_frag();


static ngx_int_t
ngx_rtmp_hls_copy(void *dst, u_char **src, size_t n,
ngx_chain_t **in);

static ngx_int_t
ngx_rtmp_hls_append_sps_pps(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_ctx_t *ctx, ngx_buf_t *out);

static ngx_int_t
ngx_rtmp_hls_parse_aac_header(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_uint_t *objtype,
ngx_uint_t *srindex, ngx_uint_t *chconf);


static ngx_int_t
ngx_rtmp_hls_close_fragment(ngx_rtmp_hls_ctx_t *ctx);


static ngx_int_t
ngx_rtmp_hls_stream_eof(ngx_rtmp_hls_ctx_t *ctx);


static ngx_int_t
ngx_rtmp_hls_audio(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_header_t *h,
ngx_chain_t *in);


static ngx_int_t
ngx_rtmp_hls_video(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_header_t *h,
ngx_chain_t *in);

static void
ngx_rtmp_hls_update_fragment(uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate);

static void
ngx_rtmp_hls_merge_app_conf(void *parent, void *child);
