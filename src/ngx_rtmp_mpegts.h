#ifndef _MPEGTS_H_INCLUDED_
#define _MPEGTS_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef WIN32

#include "ngx_win32_config.h"
#ifndef off_t
#define off_t long
#endif
#ifndef ssize_t
#define ssize_t  __int64
#endif

#else
#include "ngx_linux_config.h"
#ifndef u_char
#define u_char unsigned char
#endif

#endif //win32

typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t;

typedef ngx_uint_t      ngx_msec_t;
typedef ngx_int_t       ngx_msec_int_t;
typedef int             ngx_fd_t;

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

typedef void *              ngx_buf_tag_t;
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
	ngx_buf_t       *shadow;

	/* the buf's content could be changed */
	unsigned         temporary : 1;

	/*
	* the buf's content is in a memory cache or in a read only memory
	* and must not be changed
	*/
	unsigned         memory : 1;

	/* the buf's content is mmap()ed and must not be changed */
	unsigned         mmap : 1;

	unsigned         recycled : 1;
	unsigned         in_file : 1;
	unsigned         flush : 1;
	unsigned         sync : 1;
	unsigned         last_buf : 1;
	unsigned         last_in_chain : 1;

	unsigned         last_shadow : 1;
	unsigned         temp_file : 1;

	/* STUB */ int   num;
};

struct ngx_chain_s {
	ngx_buf_t    *buf;
	ngx_chain_t  *next;
};

#if 0

class CMpegTS
{
public:
	CMpegTS();
	~CMpegTS();

private:

};

#else

typedef struct {
    FILE*       fd;
    unsigned    encrypt:1;
    unsigned    size:4;
    u_char      buf[16];
    u_char      iv[16];
    //AES_KEY     key;
} ngx_rtmp_mpegts_file_t;


typedef struct {
    uint64_t    pts;
    uint64_t    dts;
    ngx_uint_t  pid;
    ngx_uint_t  sid;
    ngx_uint_t  cc;
    unsigned    key:1;
} ngx_rtmp_mpegts_frame_t;


ngx_int_t ngx_rtmp_mpegts_init_encryption(ngx_rtmp_mpegts_file_t *file,
    u_char *key, size_t key_len, uint64_t iv);
ngx_int_t ngx_rtmp_mpegts_open_file(ngx_rtmp_mpegts_file_t *file, u_char *path);
ngx_int_t ngx_rtmp_mpegts_close_file(ngx_rtmp_mpegts_file_t *file);
ngx_int_t ngx_rtmp_mpegts_write_frame(ngx_rtmp_mpegts_file_t *file,
    ngx_rtmp_mpegts_frame_t *f, ngx_buf_t *b);

#endif 

#endif /* _MPEGTS_H_INCLUDED_ */
