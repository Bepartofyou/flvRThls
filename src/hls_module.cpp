#include <stdio.h>
#include <stdlib.h>
#include "hls_module.h"

#if 0

static inline ngx_int_t
ngx_rtmp_is_codec_header(ngx_chain_t *in)
{
	return in->buf->pos + 1 < in->buf->last && in->buf->pos[1] == 0;
}


static void *
ngx_rtmp_rmemcpy(void *dst, const void* src, size_t n)
{
	u_char     *d, *s;

	d = (u_char*)dst;
	s = (u_char*)src + n - 1;

	while (s >= (u_char*)src) {
		*d++ = *s--;
	}

	return dst;
}

static ngx_int_t
ngx_rtmp_hls_append_aud(ngx_buf_t *out)
{
	static u_char   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

	if (out->last + sizeof(aud_nal) > out->end) {
		return NGX_ERROR;
	}

	out->last = ngx_cpymem(out->last, aud_nal, sizeof(aud_nal));

	return NGX_OK;
}

static uint64_t
ngx_rtmp_hls_get_fragment_id(int flag, uint64_t ts)
{
	switch (flag) {

	case NGX_RTMP_HLS_NAMING_TIMESTAMP:
		return ts;

	//case NGX_RTMP_HLS_NAMING_SYSTEM:
	//	return (uint64_t)ngx_cached_time->sec * 1000 + ngx_cached_time->msec;

	default: /* NGX_RTMP_HLS_NAMING_SEQUENTIAL */
		return 1;
	}
}

static ngx_int_t
ngx_rtmp_hls_open_fragment(uint64_t ts,
ngx_int_t discont)
{
	u_char* filename = (u_char*)"111111.ts";
	uint64_t                  id;
	ngx_rtmp_hls_ctx_t       *ctx;
	ngx_rtmp_hls_frag_t      *f;

	id = ngx_rtmp_hls_get_fragment_id(0, ts);

	printf("hls: open fragment file='%s', time=%uL, discont=%i \n", filename, ts, discont);

	if (ngx_rtmp_mpegts_open_file(&ctx->file, filename)!= NGX_OK)
	{
		return NGX_ERROR;
	}

	ctx->opened = 1;

	//f = ngx_rtmp_hls_get_frag(s, ctx->nfrags);

	ngx_memzero(f, sizeof(*f));

	f->active = 1;
	f->discont = discont;
	f->id = id;

	ctx->frag_ts = ts;

	/* start fragment with audio to make iPhone happy */

	ngx_rtmp_hls_flush_audio(ctx);

	return NGX_OK;
}



static ngx_int_t
ngx_rtmp_hls_flush_audio(ngx_rtmp_hls_ctx_t  *ctx)
{
	//ngx_rtmp_hls_ctx_t             *ctx;
	ngx_rtmp_mpegts_frame_t         frame;
	ngx_int_t                       rc;
	ngx_buf_t                      *b;

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

	printf("hls: flush audio pts=%uL, cc=%u \n", frame.pts, frame.cc);

	FILE* fp_aac = fopen("111.aac", "wb+");
	fwrite(b->pos, b->last - b->pos, 1, fp_aac);
	fclose(fp_aac);

	rc = ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, b);

	if (rc != NGX_OK) {
		printf("hls: audio flush failed \n");
	}

	ctx->audio_cc = frame.cc;
	b->pos = b->last = b->start;

	return rc;
}


static ngx_rtmp_hls_frag_t *
ngx_rtmp_hls_get_frag(ngx_int_t n)
{
	return NULL;
}

static void
ngx_rtmp_hls_next_frag()
{

	//if (ctx->nfrags == hacf->winfrags) {
	//	ctx->frag++;
	//}
	//else {
	//	ctx->nfrags++;
	//}
}

static ngx_int_t
ngx_rtmp_hls_copy(void *dst, u_char **src, size_t n,
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
			printf("hls: failed to read %uz byte(s) \n", n);
			return NGX_ERROR;
		}

		*src = (*in)->buf->pos;
	}
}





static ngx_int_t
ngx_rtmp_hls_append_sps_pps(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_ctx_t *ctx,  ngx_buf_t *out)
{
	u_char                         *p;
	ngx_chain_t                    *in;
	int8_t                          nnals;
	uint16_t                        len, rlen;
	ngx_int_t                       n;

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

	if (ngx_rtmp_hls_copy(NULL, &p, 10, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* number of SPS NALs */
	if (ngx_rtmp_hls_copy(&nnals, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	nnals &= 0x1f; /* 5lsb */

	printf("hls: SPS number: %uz \n", nnals);

	/* SPS */
	for (n = 0;; ++n) {
		for (; nnals; --nnals) {

			/* NAL length */
			if (ngx_rtmp_hls_copy(&rlen, &p, 2, &in) != NGX_OK) {
				return NGX_ERROR;
			}

			ngx_rtmp_rmemcpy(&len, &rlen, 2);

			printf("hls: header NAL length: %uz \n", (size_t)len);

			/* AnnexB prefix */
			if (out->end - out->last < 4) {
				printf("hls: too small buffer for header NAL size \n");
				return NGX_ERROR;
			}

			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 1;

			/* NAL body */
			if (out->end - out->last < len) {
				printf("hls: too small buffer for header NAL \n");
				return NGX_ERROR;
			}

			if (ngx_rtmp_hls_copy(out->last, &p, len, &in) != NGX_OK) {
				return NGX_ERROR;
			}

			out->last += len;
		}

		if (n == 1) {
			break;
		}

		/* number of PPS NALs */
		if (ngx_rtmp_hls_copy(&nnals, &p, 1, &in) != NGX_OK) {
			return NGX_ERROR;
		}

		printf("hls: PPS number: %uz \n", nnals);
	}

	return NGX_OK;
}

static ngx_int_t
ngx_rtmp_hls_parse_aac_header(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_uint_t *objtype,
ngx_uint_t *srindex, ngx_uint_t *chconf)
{
	ngx_chain_t            *cl;
	u_char                 *p, b0, b1;

	cl = codec_ctx->aac_header;

	p = cl->buf->pos;

	if (ngx_rtmp_hls_copy(NULL, &p, 2, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	if (ngx_rtmp_hls_copy(&b0, &p, 1, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	if (ngx_rtmp_hls_copy(&b1, &p, 1, &cl) != NGX_OK) {
		return NGX_ERROR;
	}

	*objtype = b0 >> 3;
	if (*objtype == 0 || *objtype == 0x1f) {
		printf("hls: unsupported adts object type:%ui  \n", *objtype);
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
		printf("hls: unsupported adts sample rate:%ui \n", *srindex);
		return NGX_ERROR;
	}

	*chconf = (b1 >> 3) & 0x0f;

	printf("hls: aac object_type:%ui, sample_rate_index:%ui, "
		"channel_config:%ui \n", *objtype, *srindex, *chconf);

	return NGX_OK;
}

static void
ngx_rtmp_hls_update_fragment(uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate)
{
	ngx_rtmp_hls_ctx_t         *ctx;
	ngx_rtmp_hls_app_conf_t    *hacf;
	ngx_rtmp_hls_frag_t        *f;
	ngx_int_t                  force, discont;
	ngx_buf_t                  *b;
	int64_t                     d;

	f = NULL;
	force = 0;
	discont = 1;

	if (ctx->opened) {
		f = ngx_rtmp_hls_get_frag(111);
		d = (int64_t)(ts - ctx->frag_ts);

		f->duration = (ts - ctx->frag_ts) / 90000.;
		discont = 0;
	}

	if (boundary || force) {
		ngx_rtmp_hls_close_fragment(ctx);
		ngx_rtmp_hls_open_fragment(ts, discont);
	}

	b = ctx->aframe;
	if (ctx->opened && b && b->last > b->pos &&
		ctx->aframe_pts + (uint64_t)hacf->max_audio_delay * 90 / flush_rate
		< ts)
	{
		ngx_rtmp_hls_flush_audio(ctx);
	}
}


static ngx_int_t
ngx_rtmp_hls_stream_eof(ngx_rtmp_hls_ctx_t *ctx)
{
	ngx_rtmp_hls_flush_audio(ctx);

	ngx_rtmp_hls_close_fragment(ctx);

	return NGX_OK;
}


static ngx_int_t
ngx_rtmp_hls_close_fragment(ngx_rtmp_hls_ctx_t *ctx)
{
	if (ctx == NULL || !ctx->opened) {
		return NGX_OK;
	}

	printf("hls: close fragment n=%uL \n", ctx->frag);

	ngx_rtmp_mpegts_close_file(&ctx->file);

	ctx->opened = 0;

	ngx_rtmp_hls_next_frag();

	return NGX_OK;
}

static ngx_int_t
ngx_rtmp_hls_audio(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_header_t *h,
ngx_chain_t *in)
{
	uint64_t                        pts, est_pts;
	int64_t                         dpts;
	size_t                          bsize;
	ngx_buf_t                      *b;
	u_char                         *p;
	ngx_uint_t                      objtype, srindex, chconf, size;


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

		b = (ngx_buf_t*)malloc(sizeof(ngx_buf_t));
		if (b == NULL) {
			return NGX_ERROR;
		}

		ctx->aframe = b;

		b->start = (u_char*)malloc(hacf->audio_buffer_size);
		if (b->start == NULL) {
			return NGX_ERROR;
		}

		b->end = b->start + hacf->audio_buffer_size;
		b->pos = b->last = b->start;
	}

	size = h->mlen - 2 + 7;
	pts = (uint64_t)h->timestamp * 90;

	if (b->start + size > b->end) {
		printf("hls: too big audio frame \n");
		return NGX_OK;
	}

	/*
	* start new fragment here if
	* there's no video at all, otherwise
	* do it in video handler
	*/

	ngx_rtmp_hls_update_fragment(pts, codec_ctx->avc_header == NULL, 2);

	if (b->last + size > b->end) {
		ngx_rtmp_hls_flush_audio(ctx);
	}

	printf("hls: audio pts=%uL \n", pts);

	if (b->last + 7 > b->end) {
		printf("hls: not enough buffer for audio header \n");
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

	if (ngx_rtmp_hls_parse_aac_header(codec_ctx, &objtype, &srindex, &chconf)
		!= NGX_OK)
	{
		printf("hls: aac header error \n");
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

	printf("hls: audio sync dpts=%L (%.5fs) \n",
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

	printf("hls: audio sync gap dpts=%L (%.5fs) \n",
		dpts, dpts / 90000.);

	return NGX_OK;
}

static ngx_int_t
ngx_rtmp_hls_video(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_header_t *h,
ngx_chain_t *in)
{
	u_char                         *p;
	uint8_t                         fmt, ftype, htype, nal_type, src_nal_type;
	uint32_t                        len, rlen;
	ngx_buf_t                       out, *b;
	uint32_t                        cts;
	ngx_rtmp_mpegts_frame_t         frame;
	ngx_uint_t                      nal_bytes;
	ngx_int_t                       aud_sent, sps_pps_sent, boundary;
	static u_char                   buffer[NGX_RTMP_HLS_BUFSIZE];

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
	if (ngx_rtmp_hls_copy(&fmt, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* 1: keyframe (IDR)
	* 2: inter frame
	* 3: disposable inter frame */

	ftype = (fmt & 0xf0) >> 4;

	/* H264 HDR/PICT */

	if (ngx_rtmp_hls_copy(&htype, &p, 1, &in) != NGX_OK) {
		return NGX_ERROR;
	}

	/* proceed only with PICT */

	if (htype != 1) {
		return NGX_OK;
	}

	/* 3 bytes: decoder delay */

	if (ngx_rtmp_hls_copy(&cts, &p, 3, &in) != NGX_OK) {
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
		if (ngx_rtmp_hls_copy(&rlen, &p, nal_bytes, &in) != NGX_OK) {
			return NGX_OK;
		}

		len = 0;
		ngx_rtmp_rmemcpy(&len, &rlen, nal_bytes);

		if (len == 0) {
			continue;
		}

		if (ngx_rtmp_hls_copy(&src_nal_type, &p, 1, &in) != NGX_OK) {
			return NGX_OK;
		}

		nal_type = src_nal_type & 0x1f;

		printf("hls: h264 NAL type=%ui, len=%uD \n",
			(ngx_uint_t)nal_type, len);

		if (nal_type >= 7 && nal_type <= 9) {
			if (ngx_rtmp_hls_copy(NULL, &p, len - 1, &in) != NGX_OK) {
				return NGX_ERROR;
			}
			continue;
		}

		if (!aud_sent) {
			switch (nal_type) {
			case 1:
			case 5:
			case 6:
				if (ngx_rtmp_hls_append_aud(&out) != NGX_OK) {
					printf("hls: error appending AUD NAL \n");
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
			if (ngx_rtmp_hls_append_sps_pps(codec_ctx,ctx, &out) != NGX_OK) {
				printf("hls: error appenging SPS/PPS NALs \n");
			}
			sps_pps_sent = 1;
			break;
		}

		/* AnnexB prefix */

		if (out.end - out.last < 5) {
			printf("hls: not enough buffer for AnnexB prefix \n");
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
			printf("hls: not enough buffer for NAL \n");
			return NGX_OK;
		}

		if (ngx_rtmp_hls_copy(out.last, &p, len - 1, &in) != NGX_OK) {
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

	ngx_rtmp_hls_update_fragment(frame.dts, boundary, 1);

	if (!ctx->opened) {
		return NGX_OK;
	}

	printf("hls: video pts=%uL, dts=%uL, cc=%u \n", frame.pts, frame.dts, frame.cc);

	if (ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, &out) != NGX_OK) {
		printf("hls: video frame failed \n");
	}

	ctx->video_cc = frame.cc;

	return NGX_OK;
}

static void
ngx_rtmp_hls_merge_app_conf(ngx_rtmp_hls_app_conf_t *parent)
{

	parent->hls = 0;
	parent->muxdelay = 700;
	parent->sync = 2;
	parent->slicing = NGX_RTMP_HLS_SLICING_PLAIN;
	parent->type = NGX_RTMP_HLS_TYPE_LIVE;
	parent->max_audio_delay = 300;
	parent->audio_buffer_size = NGX_RTMP_HLS_BUFSIZE;
}

#endif