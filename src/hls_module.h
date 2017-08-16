
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


#define NGX_RTMP_HLS_NAMING_SEQUENTIAL  1
#define NGX_RTMP_HLS_NAMING_TIMESTAMP   2
#define NGX_RTMP_HLS_NAMING_SYSTEM      3


#define NGX_RTMP_HLS_SLICING_PLAIN      1
#define NGX_RTMP_HLS_SLICING_ALIGNED    2


#define NGX_RTMP_HLS_TYPE_LIVE          1
#define NGX_RTMP_HLS_TYPE_EVENT         2


#if 0

static ngx_rtmp_hls_frag_t *
ngx_rtmp_hls_get_frag(ngx_rtmp_session_t *s, ngx_int_t n);

static void
ngx_rtmp_hls_next_frag(ngx_rtmp_session_t *s);

static ngx_int_t
ngx_rtmp_hls_copy(ngx_rtmp_session_t *s, void *dst, u_char **src, size_t n,
ngx_chain_t **in);

static ngx_int_t
ngx_rtmp_hls_append_aud(ngx_rtmp_session_t *s, ngx_buf_t *out);

static ngx_int_t
ngx_rtmp_hls_append_sps_pps(ngx_rtmp_session_t *s, ngx_buf_t *out);

static uint64_t
ngx_rtmp_hls_get_fragment_id(ngx_rtmp_session_t *s, uint64_t ts);

static ngx_int_t
ngx_rtmp_hls_open_fragment(ngx_rtmp_session_t *s, uint64_t ts,
ngx_int_t discont);

static ngx_int_t
ngx_rtmp_hls_parse_aac_header(ngx_rtmp_session_t *s, ngx_uint_t *objtype,
ngx_uint_t *srindex, ngx_uint_t *chconf);

static void
ngx_rtmp_hls_update_fragment(ngx_rtmp_session_t *s, uint64_t ts,
ngx_int_t boundary, ngx_uint_t flush_rate);

static ngx_int_t
ngx_rtmp_hls_flush_audio(ngx_rtmp_session_t *s);

static ngx_int_t
ngx_rtmp_hls_audio(ngx_rtmp_session_t *s, ngx_rtmp_header_t *h,
ngx_chain_t *in);

static ngx_int_t
ngx_rtmp_hls_video(ngx_rtmp_session_t *s, ngx_rtmp_header_t *h,
ngx_chain_t *in);

static ngx_int_t
ngx_rtmp_hls_stream_eof(ngx_rtmp_session_t *s, ngx_rtmp_stream_eof_t *v);

static char *
ngx_rtmp_hls_merge_app_conf(ngx_conf_t *cf, void *parent, void *child);

#endif