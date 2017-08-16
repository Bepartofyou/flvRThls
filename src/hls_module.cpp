#include <stdio.h>
#include "hls_module.h"

#if 0

#define NGX_RTMP_HLS_BUFSIZE            (1024*1024)
#define NGX_RTMP_HLS_DIR_ACCESS         0744


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


#define NGX_RTMP_HLS_NAMING_SEQUENTIAL  1
#define NGX_RTMP_HLS_NAMING_TIMESTAMP   2
#define NGX_RTMP_HLS_NAMING_SYSTEM      3


#define NGX_RTMP_HLS_SLICING_PLAIN      1
#define NGX_RTMP_HLS_SLICING_ALIGNED    2


#define NGX_RTMP_HLS_TYPE_LIVE          1
#define NGX_RTMP_HLS_TYPE_EVENT         2


static ngx_rtmp_hls_frag_t *
ngx_rtmp_hls_get_frag(ngx_rtmp_session_t *s, ngx_int_t n)
{
	ngx_rtmp_hls_ctx_t         *ctx;
	ngx_rtmp_hls_app_conf_t    *hacf;

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);
	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	return &ctx->frags[(ctx->frag + n) % (hacf->winfrags * 2 + 1)];
}


static void
ngx_rtmp_hls_next_frag(ngx_rtmp_session_t *s)
{
	ngx_rtmp_hls_ctx_t         *ctx;
	ngx_rtmp_hls_app_conf_t    *hacf;

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);
	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	if (ctx->nfrags == hacf->winfrags) {
		ctx->frag++;
	}
	else {
		ctx->nfrags++;
	}
}

static ngx_int_t
ngx_rtmp_hls_copy(ngx_rtmp_session_t *s, void *dst, u_char **src, size_t n,
ngx_chain_t **in)
{
	u_char  *last;
	size_t   pn;

	if (*in == NULL) {
		return NGX_ERROR;
	}

	for (;;) {
		last = (*in)->buf->last;

		if ((size_t)(last - *src) >= n) {
			if (dst) {
				ngx_memcpy(dst, *src, n);
			}

			*src += n;

			while (*in && *src == (*in)->buf->last) {
				*in = (*in)->next;
				if (*in) {
					*src = (*in)->buf->pos;
				}
			}

			return NGX_OK;
		}

		pn = last - *src;

		if (dst) {
			ngx_memcpy(dst, *src, pn);
			dst = (u_char *)dst + pn;
		}

		n -= pn;
		*in = (*in)->next;

		if (*in == NULL) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
				"hls: failed to read %uz byte(s)", n);
			return NGX_ERROR;
		}

		*src = (*in)->buf->pos;
	}
}


static ngx_int_t
ngx_rtmp_hls_append_aud(ngx_rtmp_session_t *s, ngx_buf_t *out)
{
	static u_char   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

	if (out->last + sizeof(aud_nal) > out->end) {
		return NGX_ERROR;
	}

	out->last = ngx_cpymem(out->last, aud_nal, sizeof(aud_nal));

	return NGX_OK;
}


static ngx_int_t
ngx_rtmp_hls_append_sps_pps(ngx_rtmp_session_t *s, ngx_buf_t *out)
{
	ngx_rtmp_codec_ctx_t           *codec_ctx;
	u_char                         *p;
	ngx_chain_t                    *in;
	ngx_rtmp_hls_ctx_t             *ctx;
	int8_t                          nnals;
	uint16_t                        len, rlen;
	ngx_int_t                       n;

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	codec_ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_codec_module);

	if (ctx == NULL || codec_ctx == NULL) {
		return NGX_ERROR;
	}

	in = codec_ctx->avc_header;
	if (in == NULL) {
		return NGX_ERROR;
	}

	p = in->buf->pos;

	/*
	* Skip bytes:
	* - flv fmt
	* - H264 CONF/PICT (0x00)
	* - 0
	* - 0
	* - 0
	* - version
	* - profile
	* - compatibility
	* - level
	* - nal bytes
	*/

	if (ngx_rtmp_hls_copy(s, NULL, &p, 10, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* number of SPS NALs */
	if (ngx_rtmp_hls_copy(s, &nnals, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	nnals &= 0x1f; /* 5lsb */

	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: SPS number: %uz", nnals);

	/* SPS */
	for (n = 0;; ++n) {
		for (; nnals; --nnals) {

			/* NAL length */
			if (ngx_rtmp_hls_copy(s, &rlen, &p, 2, &in) != NGX_OK) {
				return NGX_ERROR;
			}

			ngx_rtmp_rmemcpy(&len, &rlen, 2);

			ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
				"hls: header NAL length: %uz", (size_t)len);

			/* AnnexB prefix */
			if (out->end - out->last < 4) {
				ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
					"hls: too small buffer for header NAL size");
				return NGX_ERROR;
			}

			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 1;

			/* NAL body */
			if (out->end - out->last < len) {
				ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
					"hls: too small buffer for header NAL");
				return NGX_ERROR;
			}

			if (ngx_rtmp_hls_copy(s, out->last, &p, len, &in) != NGX_OK) {
				return NGX_ERROR;
			}

			out->last += len;
		}

		if (n == 1) {
			break;
		}

		/* number of PPS NALs */
		if (ngx_rtmp_hls_copy(s, &nnals, &p, 1, &in) != NGX_OK) {
			return NGX_ERROR;
		}

		ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
			"hls: PPS number: %uz", nnals);
	}

	return NGX_OK;
}


static uint64_t
ngx_rtmp_hls_get_fragment_id(ngx_rtmp_session_t *s, uint64_t ts)
{
	ngx_rtmp_hls_ctx_t         *ctx;
	ngx_rtmp_hls_app_conf_t    *hacf;

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);

	switch (hacf->naming) {

	case NGX_RTMP_HLS_NAMING_TIMESTAMP:
		return ts;

	case NGX_RTMP_HLS_NAMING_SYSTEM:
		return (uint64_t)ngx_cached_time->sec * 1000 + ngx_cached_time->msec;

	default: /* NGX_RTMP_HLS_NAMING_SEQUENTIAL */
		return ctx->frag + ctx->nfrags;
	}
}


static ngx_int_t
ngx_rtmp_hls_close_fragment(ngx_rtmp_session_t *s)
{
	ngx_rtmp_hls_ctx_t         *ctx;

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);
	if (ctx == NULL || !ctx->opened) {
		return NGX_OK;
	}

	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: close fragment n=%uL", ctx->frag);

	ngx_rtmp_mpegts_close_file(&ctx->file);

	ctx->opened = 0;

	ngx_rtmp_hls_next_frag(s);

	ngx_rtmp_hls_write_playlist(s);

	return NGX_OK;
}


static ngx_int_t
ngx_rtmp_hls_open_fragment(ngx_rtmp_session_t *s, uint64_t ts,
ngx_int_t discont)
{
	uint64_t                  id;
	ngx_fd_t                  fd;
	ngx_uint_t                g;
	ngx_rtmp_hls_ctx_t       *ctx;
	ngx_rtmp_hls_frag_t      *f;
	ngx_rtmp_hls_app_conf_t  *hacf;

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	if (ctx->opened) {
		return NGX_OK;
	}

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);

	id = ngx_rtmp_hls_get_fragment_id(s, ts);

	if (hacf->granularity) {
		g = (ngx_uint_t)hacf->granularity;
		id = (uint64_t)(id / g) * g;
	}

	ngx_sprintf(ctx->stream.data + ctx->stream.len, "%uL.ts%Z", id);

	ngx_log_debug6(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: open fragment file='%s', keyfile='%s', "
		"frag=%uL, n=%ui, time=%uL, discont=%i",
		ctx->stream.data,
		ctx->keyfile.data ? ctx->keyfile.data : (u_char *) "",
		ctx->frag, ctx->nfrags, ts, discont);

	if (ngx_rtmp_mpegts_open_file(&ctx->file, ctx->stream.data,
		s->connection->log)
		!= NGX_OK)
	{
		return NGX_ERROR;
	}

	ctx->opened = 1;

	f = ngx_rtmp_hls_get_frag(s, ctx->nfrags);

	ngx_memzero(f, sizeof(*f));

	f->active = 1;
	f->discont = discont;
	f->id = id;
	f->key_id = ctx->key_id;

	ctx->frag_ts = ts;

	/* start fragment with audio to make iPhone happy */

	ngx_rtmp_hls_flush_audio(s);

	return NGX_OK;
}


static ngx_int_t
ngx_rtmp_hls_parse_aac_header(ngx_rtmp_session_t *s, ngx_uint_t *objtype,
ngx_uint_t *srindex, ngx_uint_t *chconf)
{
	ngx_rtmp_codec_ctx_t   *codec_ctx;
	ngx_chain_t            *cl;
	u_char                 *p, b0, b1;

	codec_ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_codec_module);

	cl = codec_ctx->aac_header;

	p = cl->buf->pos;

	if (ngx_rtmp_hls_copy(s, NULL, &p, 2, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	if (ngx_rtmp_hls_copy(s, &b0, &p, 1, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	if (ngx_rtmp_hls_copy(s, &b1, &p, 1, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	*objtype = b0 >> 3;
	if (*objtype == 0 || *objtype == 0x1f) {
		ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
			"hls: unsupported adts object type:%ui", *objtype);
		return NGX_ERROR;
	}

	if (*objtype > 4) {

		/*
		* Mark all extended profiles as LC
		* to make Android as happy as possible.
		*/

		*objtype = 2;
	}

	*srindex = ((b0 << 1) & 0x0f) | ((b1 & 0x80) >> 7);
	if (*srindex == 0x0f) {
		ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
			"hls: unsupported adts sample rate:%ui", *srindex);
		return NGX_ERROR;
	}

	*chconf = (b1 >> 3) & 0x0f;

	ngx_log_debug3(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: aac object_type:%ui, sample_rate_index:%ui, "
		"channel_config:%ui", *objtype, *srindex, *chconf);

	return NGX_OK;
}


static void
ngx_rtmp_hls_update_fragment(ngx_rtmp_session_t *s, uint64_t ts,
ngx_int_t boundary, ngx_uint_t flush_rate)
{
	ngx_rtmp_hls_ctx_t         *ctx;
	ngx_rtmp_hls_app_conf_t    *hacf;
	ngx_rtmp_hls_frag_t        *f;
	ngx_msec_t                  ts_frag_len;
	ngx_int_t                   same_frag, force, discont;
	ngx_buf_t                  *b;
	int64_t                     d;

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);
	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);
	f = NULL;
	force = 0;
	discont = 1;

	if (ctx->opened) {
		f = ngx_rtmp_hls_get_frag(s, ctx->nfrags);
		d = (int64_t)(ts - ctx->frag_ts);

		if (d > (int64_t)hacf->max_fraglen * 90 || d < -90000) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
				"hls: force fragment split: %.3f sec, ", d / 90000.);
			force = 1;

		}
		else {
			f->duration = (ts - ctx->frag_ts) / 90000.;
			discont = 0;
		}
	}

	switch (hacf->slicing) {
	case NGX_RTMP_HLS_SLICING_PLAIN:
		if (f && f->duration < hacf->fraglen / 1000.) {
			boundary = 0;
		}
		break;

	case NGX_RTMP_HLS_SLICING_ALIGNED:

		ts_frag_len = hacf->fraglen * 90;
		same_frag = ctx->frag_ts / ts_frag_len == ts / ts_frag_len;

		if (f && same_frag) {
			boundary = 0;
		}

		if (f == NULL && (ctx->frag_ts == 0 || same_frag)) {
			ctx->frag_ts = ts;
			boundary = 0;
		}

		break;
	}

	if (boundary || force) {
		ngx_rtmp_hls_close_fragment(s);
		ngx_rtmp_hls_open_fragment(s, ts, discont);
	}

	b = ctx->aframe;
	if (ctx->opened && b && b->last > b->pos &&
		ctx->aframe_pts + (uint64_t)hacf->max_audio_delay * 90 / flush_rate
		< ts)
	{
		ngx_rtmp_hls_flush_audio(s);
	}
}


static ngx_int_t
ngx_rtmp_hls_flush_audio(ngx_rtmp_session_t *s)
{
	ngx_rtmp_hls_ctx_t             *ctx;
	ngx_rtmp_mpegts_frame_t         frame;
	ngx_int_t                       rc;
	ngx_buf_t                      *b;

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	if (ctx == NULL || !ctx->opened) {
		return NGX_OK;
	}

	b = ctx->aframe;

	if (b == NULL || b->pos == b->last) {
		return NGX_OK;
	}

	ngx_memzero(&frame, sizeof(frame));

	frame.dts = ctx->aframe_pts;
	frame.pts = frame.dts;
	frame.cc = ctx->audio_cc;
	frame.pid = 0x101;
	frame.sid = 0xc0;

	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: flush audio pts=%uL, cc=%u", frame.pts, frame.cc);

	FILE* fp_aac = fopen("111.aac", "wb+");
	fwrite(b->pos, b->last - b->pos, 1, fp_aac);
	fclose(fp_aac);

	rc = ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, b);

	if (rc != NGX_OK) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
			"hls: audio flush failed");
	}

	ctx->audio_cc = frame.cc;
	b->pos = b->last = b->start;

	return rc;
}


static ngx_int_t
ngx_rtmp_hls_audio(ngx_rtmp_session_t *s, ngx_rtmp_header_t *h,
ngx_chain_t *in)
{
	ngx_rtmp_hls_app_conf_t        *hacf;
	ngx_rtmp_hls_ctx_t             *ctx;
	ngx_rtmp_codec_ctx_t           *codec_ctx;
	uint64_t                        pts, est_pts;
	int64_t                         dpts;
	size_t                          bsize;
	ngx_buf_t                      *b;
	u_char                         *p;
	ngx_uint_t                      objtype, srindex, chconf, size;

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	codec_ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_codec_module);

	if (hacf == NULL || !hacf->hls || ctx == NULL ||
		codec_ctx == NULL || h->mlen < 2)
	{
		return NGX_OK;
	}

	if (codec_ctx->audio_codec_id != NGX_RTMP_AUDIO_AAC ||
		codec_ctx->aac_header == NULL || ngx_rtmp_is_codec_header(in))
	{
		return NGX_OK;
	}

	b = ctx->aframe;

	if (b == NULL) {

		b = ngx_pcalloc(s->connection->pool, sizeof(ngx_buf_t));
		if (b == NULL) {
			return NGX_ERROR;
		}

		ctx->aframe = b;

		b->start = ngx_palloc(s->connection->pool, hacf->audio_buffer_size);
		if (b->start == NULL) {
			return NGX_ERROR;
		}

		b->end = b->start + hacf->audio_buffer_size;
		b->pos = b->last = b->start;
	}

	size = h->mlen - 2 + 7;
	pts = (uint64_t)h->timestamp * 90;

	if (b->start + size > b->end) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
			"hls: too big audio frame");
		return NGX_OK;
	}

	/*
	* start new fragment here if
	* there's no video at all, otherwise
	* do it in video handler
	*/

	ngx_rtmp_hls_update_fragment(s, pts, codec_ctx->avc_header == NULL, 2);

	if (b->last + size > b->end) {
		ngx_rtmp_hls_flush_audio(s);
	}

	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: audio pts=%uL", pts);

	if (b->last + 7 > b->end) {
		ngx_log_debug0(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
			"hls: not enough buffer for audio header");
		return NGX_OK;
	}

	p = b->last;
	b->last += 5;

	/* copy payload */

	for (; in && b->last < b->end; in = in->next) {

		bsize = in->buf->last - in->buf->pos;
		if (b->last + bsize > b->end) {
			bsize = b->end - b->last;
		}

		b->last = ngx_cpymem(b->last, in->buf->pos, bsize);
	}

	/* make up ADTS header */

	if (ngx_rtmp_hls_parse_aac_header(s, &objtype, &srindex, &chconf)
		!= NGX_OK)
	{
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
			"hls: aac header error");
		return NGX_OK;
	}

	/* we have 5 free bytes + 2 bytes of RTMP frame header */

	p[0] = 0xff;
	p[1] = 0xf1;
	p[2] = (u_char)(((objtype - 1) << 6) | (srindex << 2) |
		((chconf & 0x04) >> 2));
	p[3] = (u_char)(((chconf & 0x03) << 6) | ((size >> 11) & 0x03));
	p[4] = (u_char)(size >> 3);
	p[5] = (u_char)((size << 5) | 0x1f);
	p[6] = 0xfc;

	if (p != b->start) {
		ctx->aframe_num++;
		return NGX_OK;
	}

	ctx->aframe_pts = pts;

	if (!hacf->sync || codec_ctx->sample_rate == 0) {
		return NGX_OK;
	}

	/* align audio frames */

	/* TODO: We assume here AAC frame size is 1024
	*       Need to handle AAC frames with frame size of 960 */

	est_pts = ctx->aframe_base + ctx->aframe_num * 90000 * 1024 /
		codec_ctx->sample_rate;
	dpts = (int64_t)(est_pts - pts);

	ngx_log_debug2(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: audio sync dpts=%L (%.5fs)",
		dpts, dpts / 90000.);

	if (dpts <= (int64_t)hacf->sync * 90 &&
		dpts >= (int64_t)hacf->sync * -90)
	{
		ctx->aframe_num++;
		ctx->aframe_pts = est_pts;
		return NGX_OK;
	}

	ctx->aframe_base = pts;
	ctx->aframe_num = 1;

	ngx_log_debug2(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: audio sync gap dpts=%L (%.5fs)",
		dpts, dpts / 90000.);

	return NGX_OK;
}


static ngx_int_t
ngx_rtmp_hls_video(ngx_rtmp_session_t *s, ngx_rtmp_header_t *h,
ngx_chain_t *in)
{
	ngx_rtmp_hls_app_conf_t        *hacf;
	ngx_rtmp_hls_ctx_t             *ctx;
	ngx_rtmp_codec_ctx_t           *codec_ctx;
	u_char                         *p;
	uint8_t                         fmt, ftype, htype, nal_type, src_nal_type;
	uint32_t                        len, rlen;
	ngx_buf_t                       out, *b;
	uint32_t                        cts;
	ngx_rtmp_mpegts_frame_t         frame;
	ngx_uint_t                      nal_bytes;
	ngx_int_t                       aud_sent, sps_pps_sent, boundary;
	static u_char                   buffer[NGX_RTMP_HLS_BUFSIZE];

	hacf = ngx_rtmp_get_module_app_conf(s, ngx_rtmp_hls_module);

	ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_hls_module);

	codec_ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_codec_module);

	if (hacf == NULL || !hacf->hls || ctx == NULL || codec_ctx == NULL ||
		codec_ctx->avc_header == NULL || h->mlen < 1)
	{
		return NGX_OK;
	}

	/* Only H264 is supported */
	if (codec_ctx->video_codec_id != NGX_RTMP_VIDEO_H264) {
		return NGX_OK;
	}

	p = in->buf->pos;
	if (ngx_rtmp_hls_copy(s, &fmt, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* 1: keyframe (IDR)
	* 2: inter frame
	* 3: disposable inter frame */

	ftype = (fmt & 0xf0) >> 4;

	/* H264 HDR/PICT */

	if (ngx_rtmp_hls_copy(s, &htype, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* proceed only with PICT */

	if (htype != 1) {
		return NGX_OK;
	}

	/* 3 bytes: decoder delay */

	if (ngx_rtmp_hls_copy(s, &cts, &p, 3, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	cts = ((cts & 0x00FF0000) >> 16) | ((cts & 0x000000FF) << 16) |
		(cts & 0x0000FF00);

	ngx_memzero(&out, sizeof(out));

	out.start = buffer;
	out.end = buffer + sizeof(buffer);
	out.pos = out.start;
	out.last = out.pos;

	nal_bytes = codec_ctx->avc_nal_bytes;
	aud_sent = 0;
	sps_pps_sent = 0;

	while (in) {
		if (ngx_rtmp_hls_copy(s, &rlen, &p, nal_bytes, &in) != NGX_OK) {
			return NGX_OK;
		}

		len = 0;
		ngx_rtmp_rmemcpy(&len, &rlen, nal_bytes);

		if (len == 0) {
			continue;
		}

		if (ngx_rtmp_hls_copy(s, &src_nal_type, &p, 1, &in) != NGX_OK) {
			return NGX_OK;
		}

		nal_type = src_nal_type & 0x1f;

		ngx_log_debug2(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
			"hls: h264 NAL type=%ui, len=%uD",
			(ngx_uint_t)nal_type, len);

		if (nal_type >= 7 && nal_type <= 9) {
			if (ngx_rtmp_hls_copy(s, NULL, &p, len - 1, &in) != NGX_OK) {
				return NGX_ERROR;
			}
			continue;
		}

		if (!aud_sent) {
			switch (nal_type) {
			case 1:
			case 5:
			case 6:
				if (ngx_rtmp_hls_append_aud(s, &out) != NGX_OK) {
					ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
						"hls: error appending AUD NAL");
				}
			case 9:
				aud_sent = 1;
				break;
			}
		}

		switch (nal_type) {
		case 1:
			sps_pps_sent = 0;
			break;
		case 5:
			if (sps_pps_sent) {
				break;
			}
			if (ngx_rtmp_hls_append_sps_pps(s, &out) != NGX_OK) {
				ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
					"hls: error appenging SPS/PPS NALs");
			}
			sps_pps_sent = 1;
			break;
		}

		/* AnnexB prefix */

		if (out.end - out.last < 5) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
				"hls: not enough buffer for AnnexB prefix");
			return NGX_OK;
		}

		/* first AnnexB prefix is long (4 bytes) */

		if (out.last == out.pos) {
			*out.last++ = 0;
		}

		*out.last++ = 0;
		*out.last++ = 0;
		*out.last++ = 1;
		*out.last++ = src_nal_type;

		/* NAL body */

		if (out.end - out.last < (ngx_int_t)len) {
			ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
				"hls: not enough buffer for NAL");
			return NGX_OK;
		}

		if (ngx_rtmp_hls_copy(s, out.last, &p, len - 1, &in) != NGX_OK) {
			return NGX_ERROR;
		}

		out.last += (len - 1);
	}

	ngx_memzero(&frame, sizeof(frame));

	frame.cc = ctx->video_cc;
	frame.dts = (uint64_t)h->timestamp * 90;
	frame.pts = frame.dts + cts * 90;
	frame.pid = 0x100;
	frame.sid = 0xe0;
	frame.key = (ftype == 1);

	/*
	* start new fragment if
	* - we have video key frame AND
	* - we have audio buffered or have no audio at all or stream is closed
	*/

	b = ctx->aframe;
	boundary = frame.key && (codec_ctx->aac_header == NULL || !ctx->opened ||
		(b && b->last > b->pos));

	ngx_rtmp_hls_update_fragment(s, frame.dts, boundary, 1);

	if (!ctx->opened) {
		return NGX_OK;
	}

	ngx_log_debug2(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
		"hls: video pts=%uL, dts=%uL, cc=%u", frame.pts, frame.dts, frame.cc);

	if (ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, &out) != NGX_OK) {
		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
			"hls: video frame failed");
	}

	ctx->video_cc = frame.cc;

	return NGX_OK;
}

static ngx_int_t
ngx_rtmp_hls_stream_eof(ngx_rtmp_session_t *s, ngx_rtmp_stream_eof_t *v)
{
	ngx_rtmp_hls_flush_audio(s);

	ngx_rtmp_hls_close_fragment(s);

	return next_stream_eof(s, v);
}

static char *
ngx_rtmp_hls_merge_app_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_rtmp_hls_app_conf_t    *prev = parent;
	ngx_rtmp_hls_app_conf_t    *conf = child;
	ngx_rtmp_hls_cleanup_t     *cleanup;

	ngx_conf_merge_value(conf->hls, prev->hls, 0);
	ngx_conf_merge_msec_value(conf->fraglen, prev->fraglen, 5000);
	ngx_conf_merge_msec_value(conf->max_fraglen, prev->max_fraglen,
		conf->fraglen * 10);
	ngx_conf_merge_msec_value(conf->muxdelay, prev->muxdelay, 700);
	ngx_conf_merge_msec_value(conf->sync, prev->sync, 2);
	ngx_conf_merge_msec_value(conf->playlen, prev->playlen, 30000);
	ngx_conf_merge_value(conf->continuous, prev->continuous, 1);
	ngx_conf_merge_value(conf->nested, prev->nested, 0);
	ngx_conf_merge_uint_value(conf->naming, prev->naming,
		NGX_RTMP_HLS_NAMING_SEQUENTIAL);
	ngx_conf_merge_uint_value(conf->slicing, prev->slicing,
		NGX_RTMP_HLS_SLICING_PLAIN);
	ngx_conf_merge_uint_value(conf->type, prev->type,
		NGX_RTMP_HLS_TYPE_LIVE);
	ngx_conf_merge_msec_value(conf->max_audio_delay, prev->max_audio_delay,
		300);
	ngx_conf_merge_size_value(conf->audio_buffer_size, prev->audio_buffer_size,
		NGX_RTMP_HLS_BUFSIZE);
	ngx_conf_merge_value(conf->cleanup, prev->cleanup, 1);
	ngx_conf_merge_str_value(conf->base_url, prev->base_url, "");
	ngx_conf_merge_value(conf->granularity, prev->granularity, 0);
	ngx_conf_merge_value(conf->keys, prev->keys, 0);
	ngx_conf_merge_str_value(conf->key_path, prev->key_path, "");
	ngx_conf_merge_str_value(conf->key_url, prev->key_url, "");
	ngx_conf_merge_uint_value(conf->frags_per_key, prev->frags_per_key, 0);

	if (conf->fraglen) {
		conf->winfrags = conf->playlen / conf->fraglen;
	}

	/* schedule cleanup */

	if (conf->hls && conf->path.len && conf->cleanup &&
		conf->type != NGX_RTMP_HLS_TYPE_EVENT)
	{
		if (conf->path.data[conf->path.len - 1] == '/') {
			conf->path.len--;
		}

		cleanup = ngx_pcalloc(cf->pool, sizeof(*cleanup));
		if (cleanup == NULL) {
			return NGX_CONF_ERROR;
		}

		cleanup->path = conf->path;
		cleanup->playlen = conf->playlen;

		conf->slot = ngx_pcalloc(cf->pool, sizeof(*conf->slot));
		if (conf->slot == NULL) {
			return NGX_CONF_ERROR;
		}

		conf->slot->manager = ngx_rtmp_hls_cleanup;
		conf->slot->name = conf->path;
		conf->slot->data = cleanup;
		conf->slot->conf_file = cf->conf_file->file.name.data;
		conf->slot->line = cf->conf_file->line;

		if (ngx_add_path(cf, &conf->slot) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}

	ngx_conf_merge_str_value(conf->path, prev->path, "");

	if (conf->keys && conf->cleanup && conf->key_path.len &&
		ngx_strcmp(conf->key_path.data, conf->path.data) != 0 &&
		conf->type != NGX_RTMP_HLS_TYPE_EVENT)
	{
		if (conf->key_path.data[conf->key_path.len - 1] == '/') {
			conf->key_path.len--;
		}

		cleanup = ngx_pcalloc(cf->pool, sizeof(*cleanup));
		if (cleanup == NULL) {
			return NGX_CONF_ERROR;
		}

		cleanup->path = conf->key_path;
		cleanup->playlen = conf->playlen;

		conf->slot = ngx_pcalloc(cf->pool, sizeof(*conf->slot));
		if (conf->slot == NULL) {
			return NGX_CONF_ERROR;
		}

		conf->slot->manager = ngx_rtmp_hls_cleanup;
		conf->slot->name = conf->key_path;
		conf->slot->data = cleanup;
		conf->slot->conf_file = cf->conf_file->file.name.data;
		conf->slot->line = cf->conf_file->line;

		if (ngx_add_path(cf, &conf->slot) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}

	ngx_conf_merge_str_value(conf->key_path, prev->key_path, "");

	if (conf->key_path.len == 0) {
		conf->key_path = conf->path;
	}

	return NGX_CONF_OK;
}

#endif