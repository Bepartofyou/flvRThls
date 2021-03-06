#ifndef _HLS_MODULE_H_INCLUDED_
#define _HLS_MODULE_H_INCLUDED_

#include "ngx_rtmp_mpegts.h"

#define NGX_RTMP_HLS_BUFSIZE            (1024*1024)
#define NGX_RTMP_HLS_DIR_ACCESS         0744

typedef struct {
	size_t      len;
	u_char     *data;
} ngx_str_t;

#define ngx_string(str)     { sizeof(str) - 1, (u_char *) str }
#define ngx_null_string     { 0, NULL }
#define ngx_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define ngx_str_null(str)   (str)->len = 0; (str)->data = NULL

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

#define NGX_RTMP_MAX_NAME           256
#define NGX_RTMP_MAX_URL            256
#define NGX_RTMP_MAX_ARGS           NGX_RTMP_MAX_NAME
typedef struct {
	u_char                          name[NGX_RTMP_MAX_NAME];
	u_char                          args[NGX_RTMP_MAX_ARGS];
	u_char                          type[16];
	int                             silent;
} ngx_rtmp_publish_t;

typedef struct {
	double                          stream;
} ngx_rtmp_close_stream_t;

typedef struct {
	uint32_t                csid;       /* chunk stream id */
	uint32_t                timestamp;  /* timestamp (delta) */
	uint32_t                mlen;       /* message length */
	uint8_t                 type;       /* message type id */
	uint32_t                msid;       /* message stream id */
} ngx_rtmp_header_t;

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
#define ngx_create_dir(name, access) CreateDirectory((const char *) name, NULL)
typedef BY_HANDLE_FILE_INFORMATION  ngx_file_info_t;
ngx_int_t ngx_file_info(u_char *filename, ngx_file_info_t *fi);
#define ngx_errno                  GetLastError()

#define NGX_INT_T_LEN   NGX_INT32_LEN
#define NGX_MAX_INT_T_VALUE  2147483647
#define NGX_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
typedef int32_t                     ngx_atomic_int_t;
typedef uint32_t                    ngx_atomic_uint_t;
#else
#define ngx_rename_file(o, n)    rename((const char *) o, (const char *) n)
#define ngx_create_dir(name, access) mkdir((const char *) name, access)
typedef struct stat              ngx_file_info_t;
#define ngx_file_info(file, sb)  stat((const char *) file, sb)
#define ngx_errno                  errno

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

#define NGX_MAX_PATH             4096

typedef struct ngx_file_s            ngx_file_t;
struct ngx_file_s {
	FILE*                   fd;
	ngx_str_t                  name;
	ngx_file_info_t            info;

	off_t                      offset;
	off_t                      sys_offset;
	unsigned                   valid_info : 1;
	unsigned                   directio : 1;
};


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

class CHlsModule
{
private:
	CHlsModule();
public:
	virtual ~CHlsModule();

public:
	static CHlsModule* getInstance();
	static void destoryInstance();

private:
	static CHlsModule* m_instance;

public:
	ngx_uint_t							m_last_ac;
	ngx_uint_t                          m_last_vc;
	uint64_t                            m_last_base;
	uint64_t                            m_last_pts;
public:
	ngx_rtmp_hls_ctx_t       ctx;
	ngx_rtmp_codec_ctx_t     codec_ctx;
	ngx_rtmp_hls_app_conf_t  hacf;

private:
	u_char *ngx_strlchr(u_char *p, u_char *last, u_char c);
	u_char *ngx_sprintf(u_char *buf, const char *fmt, ...);
	u_char *ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
	u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
	u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
	u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width);

	void * ngx_rtmp_rmemcpy(void *dst, const void* src, size_t n);
	ngx_int_t ngx_rtmp_is_codec_header(ngx_chain_t *in);

private:
	ngx_rtmp_hls_frag_t * ngx_rtmp_hls_get_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_int_t n);
	void ngx_rtmp_hls_next_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_rename_file(u_char *src, u_char *dst);
	ngx_int_t ngx_rtmp_hls_write_variant_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_write_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_copy(void *dst, u_char **src, size_t n, ngx_chain_t **in);
	ngx_int_t ngx_rtmp_hls_append_aud(ngx_buf_t *out);
	ngx_int_t ngx_rtmp_hls_append_sps_pps(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_ctx_t *ctx, ngx_buf_t *out);
	uint64_t ngx_rtmp_hls_get_fragment_id(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts);
	
	ngx_int_t ngx_rtmp_hls_ensure_directory(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_str_t *path);
	
	void ngx_rtmp_hls_restore_stream(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_publish(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_publish_t *v);
	ngx_int_t ngx_rtmp_hls_close_stream(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_close_stream_t *v);
	ngx_int_t ngx_rtmp_hls_parse_aac_header(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_uint_t *objtype, ngx_uint_t *srindex, ngx_uint_t *chconf);
	void ngx_rtmp_hls_update_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate);
	
	ngx_int_t ngx_rtmp_hls_stream_begin();
	ngx_int_t ngx_rtmp_hls_stream_eof(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_cleanup_dir(ngx_str_t *ppath, ngx_msec_t playlen);
	time_t ngx_rtmp_hls_cleanup(void *data);
public:
	ngx_int_t ngx_rtmp_hls_open_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts, ngx_int_t discont);
	ngx_int_t ngx_rtmp_hls_close_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf);
	ngx_int_t ngx_rtmp_hls_flush_audio(ngx_rtmp_hls_ctx_t *ctx);
	ngx_int_t ngx_rtmp_hls_audio(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_header_t *h,
		ngx_chain_t *in);
	ngx_int_t ngx_rtmp_hls_video(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_header_t *h,
		ngx_chain_t *in);

	///////
	ngx_int_t ngx_rtmp_hls_open_fragment_ex(const char* ts_file, uint64_t ts, ngx_int_t discont, ngx_int_t flag_m3u8);
	ngx_int_t ngx_rtmp_hls_close_fragment_ex();
	ngx_int_t ngx_rtmp_hls_close_fragment_ex2();
	ngx_int_t ngx_rtmp_hls_flush_audio_ex();
	//ngx_int_t ngx_rtmp_hls_audio_ex(ngx_rtmp_header_t *h, ngx_chain_t *in);

	ngx_int_t ngx_rtmp_hls_audio_ex(uint8_t* data, uint32_t size, uint32_t ts, bool keyframe);
	//ngx_int_t ngx_rtmp_hls_video_ex(ngx_rtmp_header_t *h,ngx_chain_t *in);
	ngx_int_t ngx_rtmp_hls_video_ex(uint8_t* data, uint32_t size, uint32_t ts, bool keyframe);

	void ngx_rtmp_hls_update_fragment_ex(uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate);
};

#endif /* _HLS_MODULE_H_INCLUDED_ */
