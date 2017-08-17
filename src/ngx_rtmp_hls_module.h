#ifndef _HLS_MODULE_H_INCLUDED_
#define _HLS_MODULE_H_INCLUDED_

#include "ngx_rtmp_mpegts.h"

#if 0

class CHlsModule
{
public:
	CHlsModule();
	~CHlsModule();

private:

};

#else

#define NGX_RTMP_HLS_BUFSIZE            (1024*1024)
#define NGX_RTMP_HLS_DIR_ACCESS         0744

typedef struct {
	size_t      len;
	u_char     *data;
} ngx_str_t;

typedef struct {
	void        *elts;
	ngx_uint_t   nelts;
	size_t       size;
	ngx_uint_t   nalloc;
} ngx_array_t;

typedef struct {
	ngx_str_t                  name;
	size_t                     len;
	void                      *data;

	u_char                    *conf_file;
	ngx_uint_t                 line;
} ngx_path_t;

typedef struct {
	uint64_t                            id;
	uint64_t                            key_id;
	double                              duration;
	unsigned                            active : 1;
	unsigned                            discont : 1; /* before */
} ngx_rtmp_hls_frag_t;


typedef struct {
	ngx_str_t                           suffix;
	ngx_array_t                         args;
} ngx_rtmp_hls_variant_t;


typedef struct {
	unsigned                            opened : 1;

	ngx_rtmp_mpegts_file_t              file;

	ngx_str_t                           playlist;
	ngx_str_t                           playlist_bak;
	ngx_str_t                           var_playlist;
	ngx_str_t                           var_playlist_bak;
	ngx_str_t                           stream;
	ngx_str_t                           keyfile;
	ngx_str_t                           name;
	u_char                              key[16];

	uint64_t                            frag;
	uint64_t                            frag_ts;
	uint64_t                            key_id;
	ngx_uint_t                          nfrags;
	ngx_rtmp_hls_frag_t                *frags; /* circular 2 * winfrags + 1 */

	ngx_uint_t                          audio_cc;
	ngx_uint_t                          video_cc;
	ngx_uint_t                          key_frags;

	uint64_t                            aframe_base;
	uint64_t                            aframe_num;

	ngx_buf_t                          *aframe;
	uint64_t                            aframe_pts;

	ngx_rtmp_hls_variant_t             *var;
} ngx_rtmp_hls_ctx_t;


typedef struct {
	ngx_str_t                           path;
	ngx_msec_t                          playlen;
	ngx_uint_t                          frags_per_key;
} ngx_rtmp_hls_cleanup_t;


typedef struct {
	ngx_flag_t                          hls;
	ngx_msec_t                          fraglen;
	ngx_msec_t                          max_fraglen;
	ngx_msec_t                          muxdelay;
	ngx_msec_t                          sync;
	ngx_msec_t                          playlen;
	ngx_uint_t                          winfrags;
	ngx_flag_t                          continuous;
	ngx_flag_t                          nested;
	ngx_str_t                           path;
	ngx_uint_t                          naming;
	ngx_uint_t                          slicing;
	ngx_uint_t                          type;
	ngx_path_t                         *slot;
	ngx_msec_t                          max_audio_delay;
	size_t                              audio_buffer_size;
	ngx_flag_t                          cleanup;
	ngx_array_t                        *variant;
	ngx_str_t                           base_url;
	ngx_int_t                           granularity;
	ngx_flag_t                          keys;
	ngx_str_t                           key_path;
	ngx_str_t                           key_url;
	ngx_uint_t                          frags_per_key;
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


#define NGX_RTMP_HLS_NAMING_SEQUENTIAL  1
#define NGX_RTMP_HLS_NAMING_TIMESTAMP   2
#define NGX_RTMP_HLS_NAMING_SYSTEM      3


#define NGX_RTMP_HLS_SLICING_PLAIN      1
#define NGX_RTMP_HLS_SLICING_ALIGNED    2


#define NGX_RTMP_HLS_TYPE_LIVE          1
#define NGX_RTMP_HLS_TYPE_EVENT         2

#if (WIN32)
typedef DWORD               ngx_pid_t;
#define ngx_rename_file(o, n)    MoveFile((const char *) o, (const char *) n)
#define NGX_INT_T_LEN   NGX_INT32_LEN
#define NGX_MAX_INT_T_VALUE  2147483647
#define NGX_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
typedef int32_t                     ngx_atomic_int_t;
typedef uint32_t                    ngx_atomic_uint_t;
#else
#define ngx_rename_file(o, n)    rename((const char *) o, (const char *) n)
typedef pid_t       ngx_pid_t;
#define NGX_INT_T_LEN   NGX_INT64_LEN
#define NGX_MAX_INT_T_VALUE  9223372036854775807
#define NGX_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

typedef int64_t                     ngx_atomic_int_t;
typedef uint64_t                    ngx_atomic_uint_t;
#endif

#define ngx_abs(value)       (((value) >= 0) ? (value) : - (value))
#define ngx_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define ngx_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define NGX_INT32_LEN   (sizeof("-2147483648") - 1)
#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1)
#define NGX_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define NGX_MAX_INT32_VALUE   (uint32_t) 0x7fffffff
#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"
#define NGX_INVALID_FILE         -1
#define NGX_FILE_ERROR           -1

typedef struct {
	unsigned    len : 28;

	unsigned    valid : 1;
	unsigned    no_cacheable : 1;
	unsigned    not_found : 1;
	unsigned    escape : 1;

	u_char     *data;
} ngx_variable_value_t;

//static ngx_rtmp_publish_pt              next_publish;
//static ngx_rtmp_close_stream_pt         next_close_stream;
//static ngx_rtmp_stream_begin_pt         next_stream_begin;
//static ngx_rtmp_stream_eof_pt           next_stream_eof;


//static char * ngx_rtmp_hls_variant(ngx_conf_t *cf, ngx_command_t *cmd,
//	void *conf);
//static ngx_int_t ngx_rtmp_hls_postconfiguration(ngx_conf_t *cf);
//static void * ngx_rtmp_hls_create_app_conf(ngx_conf_t *cf);
//static char * ngx_rtmp_hls_merge_app_conf(ngx_conf_t *cf,
//	void *parent, void *child);
//static ngx_int_t ngx_rtmp_hls_flush_audio(ngx_rtmp_session_t *s);
//static ngx_int_t ngx_rtmp_hls_ensure_directory(ngx_rtmp_session_t *s,
//	ngx_str_t *path);


static ngx_rtmp_hls_frag_t *
ngx_rtmp_hls_get_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_int_t n);
static void
ngx_rtmp_hls_next_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
static ngx_int_t
ngx_rtmp_hls_rename_file(u_char *src, u_char *dst);
static ngx_int_t
ngx_rtmp_hls_write_variant_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
static ngx_int_t
ngx_rtmp_hls_write_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
static ngx_int_t
ngx_rtmp_hls_copy(void *dst, u_char **src, size_t n, ngx_chain_t **in);
static ngx_int_t
ngx_rtmp_hls_append_aud(ngx_buf_t *out);

static u_char *
ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
static u_char *
ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
static u_char *
ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
static u_char *
ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width);


#endif

#endif /* _HLS_MODULE_H_INCLUDED_ */
