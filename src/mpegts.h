
/*
 * Copyright (C) Roman Arutyunyan
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>


//#include <openssl/aes.h>
#ifndef u_char
#define u_char unsigned char
#endif

#ifdef WIN32

#ifndef uint64_t
#define uint64_t unsigned __int64
#endif

#ifndef int64_t
#define int64_t __int64
#endif

#ifndef off_t
#define off_t long
#endif

#ifndef ssize_t
#define ssize_t  __int64
#endif

#ifndef int8_t
#define int8_t  signed char
#endif

#ifndef uint8_t
#define uint8_t  unsigned char
#endif

#ifndef uint32_t
#define uint32_t  unsigned int
#endif

#ifndef uint16_t
#define uint16_t  unsigned short
#endif

#endif //win32

typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t;

typedef ngx_uint_t      ngx_msec_t;
typedef ngx_int_t       ngx_msec_int_t;

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6

#define ngx_memmove(dst, src, n)   (void) memmove(dst, src, n)
#define ngx_movemem(dst, src, n)   (((u_char *) memmove(dst, src, n)) + (n))
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)

/* msvc and icc7 compile memcmp() to the inline loop */
#define ngx_memcmp(s1, s2, n)  memcmp((const char *) s1, (const char *) s2, n)
#define ngx_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define ngx_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))


typedef struct {
    FILE*    fd;
    unsigned int    encrypt:1;
    unsigned int   size:4;
    u_char      buf[16];
    u_char      iv[16];
//    AES_KEY     key;
} ngx_rtmp_mpegts_file_t;


typedef struct {
    uint64_t    pts;
    uint64_t    dts;
	ngx_uint_t  pid;
	ngx_uint_t  sid;
	ngx_uint_t  cc;
    unsigned    key:1;
} ngx_rtmp_mpegts_frame_t;

typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s    ngx_buf_t;
typedef struct ngx_chain_s  ngx_chain_t;

struct ngx_buf_s {
	u_char          *pos;
	u_char          *last;
	off_t            file_pos;
	off_t            file_last;

	u_char          *start;         /* start of buffer */
	u_char          *end;           /* end of buffer */
	ngx_buf_tag_t    tag;
};

struct ngx_chain_s {
	ngx_buf_t    *buf;
	ngx_chain_t  *next;
};


intptr_t ngx_rtmp_mpegts_init_encryption(ngx_rtmp_mpegts_file_t *file,
    u_char *key, size_t key_len, uint64_t iv);
intptr_t ngx_rtmp_mpegts_open_file(ngx_rtmp_mpegts_file_t *file, u_char *path);
intptr_t ngx_rtmp_mpegts_close_file(ngx_rtmp_mpegts_file_t *file);
intptr_t ngx_rtmp_mpegts_write_frame(ngx_rtmp_mpegts_file_t *file,
    ngx_rtmp_mpegts_frame_t *f, ngx_buf_t *b);