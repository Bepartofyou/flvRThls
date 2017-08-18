#include "ngx_rtmp_hls_module.h"

CHlsModule* CHlsModule::m_instance = NULL;


CHlsModule::CHlsModule()
{
	/* ngx_rtmp_hls_ctx_t */
	ctx.opened = 0;

	ctx.file = { 0 };

	ctx.playlist = {0};
	//{len = 47, data = 0x16394f0 "/home/wupengqiang/work/rtmp/video-js/hls/1.m3u8"}
	ctx.playlist_bak = { 0 };
	//{len = 51, data = 0x1639520 "/home/wupengqiang/work/rtmp/video-js/hls/1.m3u8.bak"}
	ctx.var_playlist = { 0 };
	ctx.var_playlist_bak = { 0 };
	ctx.stream = { 0 };
	//{len = 43, data = 0x161f1c0 "/home/wupengqiang/work/rtmp/video-js/hls/1-5.ts"}
	ctx.keyfile = { 0 };
	ctx.name = { 0 };
	//{len = 1, data = 0x16394e8 "1"}
	ctx.key[16] = { 0 };

	ctx.frag = 0;
	ctx.frag_ts = 0;
	//87030
	ctx.key_id = 0;
	ctx.nfrags = 0;
	//5
	ctx.frags = 0;
	//{id = 0, key_id = 0, duration = 5, active = 1, discont = 1}

	ctx.audio_cc = 0;
	ctx.video_cc = 0;
	ctx.key_frags = 0;

	ctx.aframe_base = 0;
	ctx.aframe_num = 0;

	ctx.aframe = 0;
	ctx.aframe_pts = 0;

	ctx.var = 0;
	//(ngx_rtmp_hls_variant_t *) 0x0

	/* ngx_rtmp_codec_ctx_t */
	codec_ctx.width = 640;
	codec_ctx.height = 480;
	codec_ctx.duration = 0;
	codec_ctx.frame_rate = 15;
	codec_ctx.video_data_rate = 0;
	codec_ctx.video_codec_id = NGX_RTMP_VIDEO_H264;
	codec_ctx.audio_data_rate = 0;
	codec_ctx.audio_codec_id = NGX_RTMP_AUDIO_AAC;
	codec_ctx.aac_profile = 2;
	codec_ctx.aac_chan_conf = 2;
	codec_ctx.aac_sbr = 0;
	codec_ctx.aac_ps = 0;
	codec_ctx.avc_profile = 66;
	codec_ctx.avc_compat = 192;
	codec_ctx.avc_level = 31;
	codec_ctx.avc_nal_bytes = 4;
	codec_ctx.avc_ref_frames = 3;
	codec_ctx.sample_rate = 22050;
	codec_ctx.sample_size = 2;
	codec_ctx.audio_channels = 2;
	codec_ctx.profile[32] = { 0 };
	codec_ctx.level[32] = { 0 };

	codec_ctx.avc_header = 0;
	codec_ctx.aac_header = 0;

	codec_ctx.meta = 0;
	codec_ctx.meta_version = 1;

	/* ngx_rtmp_hls_app_conf_t */
	hacf.hls = 1;
	hacf.fraglen = 0;
	//5000
	hacf.max_fraglen = 500000;
	//50000
	hacf.muxdelay = 700;
	hacf.sync = 2;
	hacf.playlen = 30000;
	hacf.winfrags = 6;
	hacf.continuous = 1;
	hacf.nested = 0;
	hacf.path = { 0 };
	// {len = 40, data = 0x1624aa0 "/home/wupengqiang/work/rtmp/video-js/hls"}
	hacf.naming = 1;
	hacf.slicing = 1;
	hacf.type = 1;
	hacf.slot = 0;
	//{name = {len = 40, data = 0x1624aa0 "/home/wupengqiang/work/rtmp/video-js/hls"}, len = 0, level = {0, 0, 0}, manager = 0x4b37d4 <ngx_rtmp_hls_cleanup>, 
	//purger = 0, loader = 0, data = 0x1624b98, conf_file = 0x161af67 "./prefix/conf/nginx.conf", line = 32
    //}
	hacf.max_audio_delay = 300;
	hacf.audio_buffer_size = 1024*1024;
	hacf.cleanup = 1;
	hacf.variant = 0;
	//(ngx_array_t *) 0x0
	hacf.base_url = { 0 };
	//{len = 0, data = 0x4c9fdb ""}
	hacf.granularity = 0;
	hacf.keys = 0;
	hacf.key_path = { 0 };
	//{len = 40, data = 0x1624aa0 "/home/wupengqiang/work/rtmp/video-js/hls"}
	hacf.key_url = { 0 };
	//{len = 0, data = 0x4c9fdb ""}
	hacf.frags_per_key = 0;
}

CHlsModule::~CHlsModule()
{
	//this->destoryInstance();
}

CHlsModule* CHlsModule::getInstance() {

	if (NULL == m_instance) {
		m_instance = new CHlsModule();

	}
	return m_instance;
}

void CHlsModule::destoryInstance() {

	if (m_instance) {
		delete m_instance;
		m_instance = NULL;
	}
}




ngx_rtmp_hls_frag_t * CHlsModule::ngx_rtmp_hls_get_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_int_t n)
{
	return &ctx->frags[(ctx->frag + n) % (hacf->winfrags * 2 + 1)];
}

void CHlsModule::ngx_rtmp_hls_next_frag(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	if (ctx->nfrags == hacf->winfrags) {
		ctx->frag++;
	} else {
		ctx->nfrags++;
	}
}

ngx_int_t CHlsModule::ngx_rtmp_hls_rename_file(u_char *src, u_char *dst)
{
	/* rename file with overwrite */

#if (WIN32)
	return ngx_rename_file(src, dst);
#else
	return ngx_rename_file(src, dst);
#endif
}

ngx_int_t CHlsModule::ngx_rtmp_hls_write_variant_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	static u_char             buffer[1024];

	u_char                   *p, *last;
	ssize_t                   rc;
	FILE*                  fd;
	ngx_str_t                *arg;
	ngx_uint_t                n, k;
	ngx_rtmp_hls_variant_t   *var;

	fd = fopen((const char*)ctx->var_playlist_bak.data, "wb");

	if (fd == NULL) {
		printf("hls: CreateFile() failed: '%V'  \n",
			&ctx->var_playlist_bak);

		return NGX_ERROR;
	}

#define NGX_RTMP_HLS_VAR_HEADER "#EXTM3U\n#EXT-X-VERSION:3\n"

	rc = fwrite(NGX_RTMP_HLS_VAR_HEADER, sizeof(NGX_RTMP_HLS_VAR_HEADER) - 1, 1, fd);
	if (rc < 0) {
		printf("hls: WriteFile failed: '%V'  \n",
			&ctx->var_playlist_bak);
		fclose(fd);
		return NGX_ERROR;
	}

	var = (ngx_rtmp_hls_variant_t *)hacf->variant->elts;
	for (n = 0; n < hacf->variant->nelts; n++, var++)
	{
		p = buffer;
		last = buffer + sizeof(buffer);

		p = ngx_slprintf(p, last, "#EXT-X-STREAM-INF:PROGRAM-ID=1");

		arg = (ngx_str_t*)var->args.elts;
		for (k = 0; k < var->args.nelts; k++, arg++) {
			p = ngx_slprintf(p, last, ",%V", arg);
		}

		if (p < last) {
			*p++ = '\n';
		}

		p = ngx_slprintf(p, last, "%V%*s%V",
			&hacf->base_url,
			ctx->name.len - ctx->var->suffix.len, ctx->name.data,
			&var->suffix);
		if (hacf->nested) {
			p = ngx_slprintf(p, last, "%s", "/index");
		}

		p = ngx_slprintf(p, last, "%s", ".m3u8\n");

		rc = fwrite(buffer, p - buffer, 1, fd);
		if (rc < 0) {
			printf("hls: write failed '%V'  \n",
				&ctx->var_playlist_bak);
			fclose(fd);
			return NGX_ERROR;
		}
	}

	fclose(fd);

	if (ngx_rtmp_hls_rename_file(ctx->var_playlist_bak.data,
		ctx->var_playlist.data)
		== NGX_FILE_ERROR)
	{
		printf("hls: rename failed: '%V'->'%V'  \n",
			&ctx->var_playlist_bak, &ctx->var_playlist);
		return NGX_ERROR;
	}

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_write_playlist(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	static u_char                   buffer[1024];
	FILE*                        fd;
	u_char                         *p, *end;
	ssize_t                         n;
	ngx_rtmp_hls_frag_t            *f;
	ngx_uint_t                      i, max_frag;
	ngx_str_t                       name_part, key_name_part;
	uint64_t                        prev_key_id;
	const char                     *sep, *key_sep;


	fd = fopen((const char*)ctx->playlist_bak.data, "wb");

	if (fd == NULL) {
		printf("hls: open failed: '%V'  \n",
			&ctx->playlist_bak);
		return NGX_ERROR;
	}

	max_frag = hacf->fraglen / 1000;

	for (i = 0; i < ctx->nfrags; i++) {
		f = ngx_rtmp_hls_get_frag(ctx, hacf, i);
		if (f->duration > max_frag) {
			max_frag = (ngx_uint_t) (f->duration + .5);
		}
	}

	p = buffer;
	end = p + sizeof(buffer);

	p = ngx_slprintf(p, end,
		"#EXTM3U\n"
		"#EXT-X-VERSION:3\n"
		"#EXT-X-MEDIA-SEQUENCE:%uL\n"
		"#EXT-X-TARGETDURATION:%ui\n",
		ctx->frag, max_frag);

	if (hacf->type == NGX_RTMP_HLS_TYPE_EVENT) {
		p = ngx_slprintf(p, end, "#EXT-X-PLAYLIST-TYPE: EVENT\n");
	}

	n = fwrite(buffer, p - buffer, 1, fd);
	if (n < 0) {
		printf("hls: write failed: '%V'  \n",
			&ctx->playlist_bak);
		fclose(fd);
		return NGX_ERROR;
	}

	sep = hacf->nested ? (hacf->base_url.len ? "/" : "") : "-";
	key_sep = hacf->nested ? (hacf->key_url.len ? "/" : "") : "-";

	name_part.len = 0;
	if (!hacf->nested || hacf->base_url.len) {
		name_part = ctx->name;
	}

	key_name_part.len = 0;
	if (!hacf->nested || hacf->key_url.len) {
		key_name_part = ctx->name;
	}

	prev_key_id = 0;

	for (i = 0; i < ctx->nfrags; i++) {
		f = ngx_rtmp_hls_get_frag(ctx,hacf, i);

		p = buffer;
		end = p + sizeof(buffer);

		if (f->discont) {
			p = ngx_slprintf(p, end, "#EXT-X-DISCONTINUITY\n");
		}

		if (hacf->keys && (i == 0 || f->key_id != prev_key_id)) {
			p = ngx_slprintf(p, end, "#EXT-X-KEY:METHOD=AES-128,"
				"URI=\"%V%V%s%uL.key\",IV=0x%032XL\n",
				&hacf->key_url, &key_name_part,
				key_sep, f->key_id, f->key_id);
		}

		prev_key_id = f->key_id;

		p = ngx_slprintf(p, end,
			"#EXTINF:%.3f,\n"
			"%V%V%s%uL.ts\n",
			f->duration, &hacf->base_url, &name_part, sep, f->id);

		printf("hls: fragment frag=%uL, n=%ui/%ui, duration=%.3f, "
			"discont=%i  \n",
			ctx->frag, i + 1, ctx->nfrags, f->duration, f->discont);

		n = fwrite(buffer, p - buffer, 1, fd);
		if (n < 0) {
			printf("hls: write failed '%V'  \n",
				&ctx->playlist_bak);
			fclose(fd);
			return NGX_ERROR;
		}
	}

	fclose(fd);

	if (ngx_rtmp_hls_rename_file(ctx->playlist_bak.data, ctx->playlist.data)
		== NGX_FILE_ERROR)
	{
		printf("hls: rename failed: '%V'->'%V'  \n",
			&ctx->playlist_bak, &ctx->playlist);
		return NGX_ERROR;
	}

	if (ctx->var) {
		return ngx_rtmp_hls_write_variant_playlist(ctx, hacf);
	}

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_copy(void *dst, u_char **src, size_t n, ngx_chain_t **in)
{
	u_char  *last;
	size_t   pn;

	if (*in == NULL) {
		return NGX_ERROR;
	}

	for ( ;; ) {
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
			printf("hls: failed to read %uz byte(s)  \n", n);
			return NGX_ERROR;
		}

		*src = (*in)->buf->pos;
	}
}

ngx_int_t CHlsModule::ngx_rtmp_hls_append_aud(ngx_buf_t *out)
{
	static u_char   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

	if (out->last + sizeof(aud_nal) > out->end) {
		return NGX_ERROR;
	}

	out->last = ngx_cpymem(out->last, aud_nal, sizeof(aud_nal));

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_append_sps_pps(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_hls_ctx_t *ctx, ngx_buf_t *out)
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

	printf("hls: SPS number: %uz  \n", nnals);

	/* SPS */
	for (n = 0; ; ++n) {
		for (; nnals; --nnals) {

			/* NAL length */
			if (ngx_rtmp_hls_copy(&rlen, &p, 2, &in) != NGX_OK) {
				return NGX_ERROR;
			}

			ngx_rtmp_rmemcpy(&len, &rlen, 2);

			printf("hls: header NAL length: %uz  \n", (size_t) len);

			/* AnnexB prefix */
			if (out->end - out->last < 4) {
				printf("hls: too small buffer for header NAL size  \n");
				return NGX_ERROR;
			}

			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 0;
			*out->last++ = 1;

			/* NAL body */
			if (out->end - out->last < len) {
				printf("hls: too small buffer for header NAL  \n");
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

		printf("hls: PPS number: %uz  \n", nnals);
	}

	return NGX_OK;
}

uint64_t CHlsModule::ngx_rtmp_hls_get_fragment_id(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts)
{
	switch (hacf->naming) {

	case NGX_RTMP_HLS_NAMING_TIMESTAMP:
		return ts;

	//case NGX_RTMP_HLS_NAMING_SYSTEM:
	//	return (uint64_t) ngx_cached_time->sec * 1000 + ngx_cached_time->msec;

	default: /* NGX_RTMP_HLS_NAMING_SEQUENTIAL */
		return ctx->frag + ctx->nfrags;
	}
}

ngx_int_t CHlsModule::ngx_rtmp_hls_close_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	if (ctx == NULL || !ctx->opened) {
		return NGX_OK;
	}

	printf("hls: close fragment n=%uL  \n", ctx->frag);

	ngx_rtmp_mpegts_close_file(&ctx->file);

	ctx->opened = 0;

	ngx_rtmp_hls_next_frag(ctx, hacf);

	ngx_rtmp_hls_write_playlist(ctx, hacf);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_ensure_directory(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_str_t *path)
{
	size_t                    len;
	ngx_file_info_t           fi;
	(void)fi;

	static u_char  zpath[NGX_MAX_PATH + 1];

	if (path->len + 1 > sizeof(zpath)) {
		printf("hls: too long path  \n");
		return NGX_ERROR;
	}

	ngx_snprintf(zpath, sizeof(zpath), "%V%Z", path);

	//if (ngx_file_info(zpath, &fi) == NGX_FILE_ERROR) {

	//	if (ngx_errno != NGX_ENOENT) {
	//		printf("hls: stat() failed on '%V'  \n", path);
	//		return NGX_ERROR;
	//	}

	//	/* ENOENT */

	//	if (ngx_create_dir(zpath, NGX_RTMP_HLS_DIR_ACCESS) == NGX_FILE_ERROR) {
	//		printf("hls: mkdir() failed on '%V'  \n", path);
	//		return NGX_ERROR;
	//	}

	//	printf("hls: directory '%V' created  \n", path);

	//}
	//else {

	//	if (!ngx_is_dir(&fi)) {
	//		ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
	//			"hls: '%V' exists and is not a directory", path);
	//		return  NGX_ERROR;
	//	}

	//	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
	//		"hls: directory '%V' exists", path);
	//}

	if (!hacf->nested) {
		return NGX_OK;
	}

	len = path->len;
	if (path->data[len - 1] == '/') {
		len--;
	}

	if (len + 1 + ctx->name.len + 1 > sizeof(zpath)) {
		printf("hls: too long path  \n");
		return NGX_ERROR;
	}

	ngx_snprintf(zpath, sizeof(zpath) - 1, "%*s/%V%Z", len, path->data,
		&ctx->name);

	//if (ngx_file_info(zpath, &fi) != NGX_FILE_ERROR) {

	//	if (ngx_is_dir(&fi)) {
	//		ngx_log_debug1(NGX_LOG_DEBUG_RTMP, s->connection->log, 0,
	//			"hls: directory '%s' exists", zpath);
	//		return NGX_OK;
	//	}

	//	ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
	//		"hls: '%s' exists and is not a directory", zpath);

	//	return  NGX_ERROR;
	//}

	//if (ngx_errno != NGX_ENOENT) {
	//	ngx_log_error(NGX_LOG_ERR, s->connection->log, ngx_errno,
	//		"hls: " ngx_file_info_n " failed on '%s'", zpath);
	//	return NGX_ERROR;
	//}

	/* NGX_ENOENT */

	if (ngx_create_dir(zpath, NGX_RTMP_HLS_DIR_ACCESS) == NGX_FILE_ERROR) {
		printf("hls: mkdir() failed on '%s'  \n", zpath);
		return NGX_ERROR;
	}

	printf("hls: directory '%s' created  \n", zpath);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_open_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts, ngx_int_t discont)
{
	uint64_t                  id;
	FILE*                  fd;
	ngx_uint_t                g;
	ngx_rtmp_hls_frag_t      *f;

	if (ctx->opened) {
		return NGX_OK;
	}

	if (ngx_rtmp_hls_ensure_directory(ctx,hacf, &hacf->path) != NGX_OK) {
		return NGX_ERROR;
	}

	if (hacf->keys &&
		ngx_rtmp_hls_ensure_directory(ctx, hacf, &hacf->key_path) != NGX_OK)
	{
		return NGX_ERROR;
	}

	id = ngx_rtmp_hls_get_fragment_id(ctx, hacf, ts);

	if (hacf->granularity) {
		g = (ngx_uint_t) hacf->granularity;
		id = (uint64_t) (id / g) * g;
	}

	ngx_sprintf(ctx->stream.data + ctx->stream.len, "%uL.ts%Z", id);

	if (hacf->keys) {
		if (ctx->key_frags == 0) {

			ctx->key_frags = hacf->frags_per_key - 1;
			ctx->key_id = id;

			//if (RAND_bytes(ctx->key, 16) < 0) {
			//	printf("hls: failed to create key  \n");
			//	return NGX_ERROR;
			//}

			ngx_sprintf(ctx->keyfile.data + ctx->keyfile.len, "%uL.key%Z", id);

			fd = fopen((const char*)ctx->keyfile.data, "wb");

			if (fd == NULL) {
				printf("hls: failed to open key file '%s'  \n",
					ctx->keyfile.data);
				return NGX_ERROR;
			}
			
			if (fwrite(ctx->key, 16, 1, fd) != 16) {
				printf("hls: failed to write key file '%s'  \n",
					ctx->keyfile.data);
				fclose(fd);
				return NGX_ERROR;
			}

			fclose(fd);

		} else {
			if (hacf->frags_per_key) {
				ctx->key_frags--;
			}

			//if (ngx_set_file_time(ctx->keyfile.data, 0, ngx_cached_time->sec)
			//	!= NGX_OK)
			//{
			//	printf("utimes() '%s' failed  \n",
			//		ctx->keyfile.data);
			//}
		}
	}

	printf("hls: open fragment file='%s', keyfile='%s', "
		"frag=%uL, n=%ui, time=%uL, discont=%i  \n",
		ctx->stream.data,
		ctx->keyfile.data ? ctx->keyfile.data : (u_char *) "",
		ctx->frag, ctx->nfrags, ts, discont);

	if (hacf->keys &&
		ngx_rtmp_mpegts_init_encryption(&ctx->file, ctx->key, 16, ctx->key_id)
		!= NGX_OK)
	{
		printf("hls: failed to initialize hls encryption  \n");
		return NGX_ERROR;
	}

	if (ngx_rtmp_mpegts_open_file(&ctx->file, ctx->stream.data)
		!= NGX_OK)
	{
		return NGX_ERROR;
	}

	ctx->opened = 1;

	f = ngx_rtmp_hls_get_frag(ctx,hacf, ctx->nfrags);

	ngx_memzero(f, sizeof(*f));

	f->active = 1;
	f->discont = discont;
	f->id = id;
	f->key_id = ctx->key_id;

	ctx->frag_ts = ts;

	/* start fragment with audio to make iPhone happy */

	ngx_rtmp_hls_flush_audio(ctx);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_open_fragment_ex(const char* ts_file, uint64_t ts, ngx_int_t discont)
{
	if (this->ctx.opened) {
		return NGX_OK;
	}

	this->ctx.stream = ngx_string(ts_file);

	printf("hls: open fragment file='%s', keyfile='%s', "
		"frag=%uL, n=%ui, time=%uL, discont=%i  \n",
		this->ctx.stream.data,
		this->ctx.keyfile.data ? this->ctx.keyfile.data : (u_char *) "",
		this->ctx.frag, this->ctx.nfrags, ts, discont);

	if (ngx_rtmp_mpegts_open_file(&ctx.file, ctx.stream.data) != NGX_OK)
		return NGX_ERROR;

	this->ctx.opened = 1;

	/* start fragment with audio to make iPhone happy */

	ngx_rtmp_hls_flush_audio_ex();

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_close_fragment_ex()
{
	if (!this->ctx.opened) {
		return NGX_OK;
	}

	printf("hls: close fragment n=%uL  \n", this->ctx.frag);

	ngx_rtmp_mpegts_close_file(&this->ctx.file);

	this->ctx.opened = 0;

	//ngx_rtmp_hls_next_frag(&this->ctx, &this->hacf);

	//ngx_rtmp_hls_write_playlist(&this->ctx, &this->hacf);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_flush_audio_ex()
{
	ngx_rtmp_mpegts_frame_t         frame;
	ngx_int_t                       rc;
	ngx_buf_t                      *b;

	if (!this->ctx.opened) {
		return NGX_OK;
	}

	b = this->ctx.aframe;

	if (b == NULL || b->pos == b->last) {
		return NGX_OK;
	}

	ngx_memzero(&frame, sizeof(frame));

	frame.dts = this->ctx.aframe_pts;
	frame.pts = frame.dts;
	frame.cc = this->ctx.audio_cc;
	frame.pid = 0x101;
	frame.sid = 0xc0;

	//printf("hls: flush audio pts=%uL, cc=%uL  \n", frame.pts, frame.cc);

	FILE* fp_aac = fopen("111.aac", "ab+");
	fwrite(b->pos, b->last - b->pos, 1, fp_aac);
	fclose(fp_aac);

	rc = ngx_rtmp_mpegts_write_frame(&this->ctx.file, &frame, b);

	if (rc != NGX_OK) {
		printf("hls: audio flush failed  \n");
	}

	this->ctx.audio_cc = frame.cc;
	b->pos = b->last = b->start;

	return rc;
}

void CHlsModule::ngx_rtmp_hls_update_fragment_ex(uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate)
{
	ngx_buf_t                  *b;

	//if (boundary) {
	//	ngx_rtmp_hls_close_fragment_ex();
	//	ngx_rtmp_hls_open_fragment_ex(ts, 1);
	//}

	b = this->ctx.aframe;
	if (this->ctx.opened && b && b->last > b->pos &&
		this->ctx.aframe_pts + (uint64_t)this->hacf.max_audio_delay * 90 / flush_rate
		< ts)
	{
		ngx_rtmp_hls_flush_audio_ex();
	}
}

ngx_int_t CHlsModule::ngx_rtmp_hls_audio_ex(uint8_t* data, uint32_t size, uint32_t ts, bool keyframe)
{
	uint64_t                        pts, est_pts;
	int64_t                         dpts;
	ngx_buf_t                      *b;
	u_char                         *p;

	b = this->ctx.aframe;

	if (b == NULL) {

		b = (ngx_buf_t*)malloc(sizeof(ngx_buf_t));
		if (b == NULL) {
			return NGX_ERROR;
		}

		this->ctx.aframe = b;

		b->start = (u_char*)malloc(this->hacf.audio_buffer_size);
		if (b->start == NULL) {
			return NGX_ERROR;
		}

		b->end = b->start + this->hacf.audio_buffer_size;
		b->pos = b->last = b->start;
	}

	pts = (uint64_t)ts * 90;

	if (b->start + size > b->end) {
		printf("hls: too big audio frame  \n");
		return NGX_OK;
	}

	/*
	* start new fragment here if
	* there's no video at all, otherwise
	* do it in video handler
	*/

	//ngx_rtmp_hls_update_fragment(&this->ctx, &this->hacf, pts, this->codec_ctx.avc_header == NULL, 2);
	ngx_rtmp_hls_update_fragment_ex(pts, 0, 2);

	if (b->last + size > b->end) {
		ngx_rtmp_hls_flush_audio_ex();
	}

	//printf("hls: audio pts=%uL  \n", pts);

	if (b->last + 7 > b->end) {
		printf("hls: not enough buffer for audio header  \n");
		return NGX_OK;
	}

	p = b->last;
	/* copy payload */
	b->last = ngx_cpymem(b->last, data, size);

	/* we have 5 free bytes + 2 bytes of RTMP frame header */

	//p[0] = 0xff;
	//p[1] = 0xf1;
	//p[2] = (u_char)(((objtype - 1) << 6) | (srindex << 2) |
	//	((chconf & 0x04) >> 2));
	//p[3] = (u_char)(((chconf & 0x03) << 6) | ((size >> 11) & 0x03));
	//p[4] = (u_char)(size >> 3);
	//p[5] = (u_char)((size << 5) | 0x1f);
	//p[6] = 0xfc;

	if (p != b->start) {
		this->ctx.aframe_num++;
		return NGX_OK;
	}

	this->ctx.aframe_pts = pts;

	if (!this->hacf.sync || this->codec_ctx.sample_rate == 0) {
		return NGX_OK;
	}

	/* align audio frames */

	/* TODO: We assume here AAC frame size is 1024
	*       Need to handle AAC frames with frame size of 960 */

	est_pts = this->ctx.aframe_base + this->ctx.aframe_num * 90000 * 1024 /
		this->codec_ctx.sample_rate;
	dpts = (int64_t)(est_pts - pts);

	printf("hls: audio sync dpts=%L (%.5fs)  \n",
		dpts, dpts / 90000.);

	if (dpts <= (int64_t)this->hacf.sync * 90 &&
		dpts >= (int64_t)this->hacf.sync * -90)
	{
		this->ctx.aframe_num++;
		this->ctx.aframe_pts = est_pts;
		return NGX_OK;
	}

	this->ctx.aframe_base = pts;
	this->ctx.aframe_num = 1;

	printf("hls: audio sync gap dpts=%L (%.5fs)  \n",
		dpts, dpts / 90000.);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_video_ex(uint8_t* data, uint32_t size, uint32_t ts, bool keyframe)
{
	ngx_buf_t                       out, *b;
	uint32_t                        cts;
	ngx_rtmp_mpegts_frame_t         frame;
	static u_char                   buffer[NGX_RTMP_HLS_BUFSIZE];

	/* Only H264 is supported */
	if (this->codec_ctx.video_codec_id != NGX_RTMP_VIDEO_H264) {
		return NGX_OK;
	}

	ngx_memzero(&out, sizeof(out));

	out.start = buffer;
	out.end = buffer + sizeof(buffer);
	out.pos = out.start;
	out.last = out.pos;

	//if (ngx_rtmp_hls_append_aud(&out) != NGX_OK) {
	//	printf("hls: error appending AUD NAL  \n");
	//}

	if (out.end - out.last < (ngx_int_t)size) {
		printf("hls: not enough buffer for NAL  \n");
		return NGX_OK;
	}
	out.last = ngx_cpymem(out.last, data, size);


	ngx_memzero(&frame, sizeof(frame));

	cts = 0;

	frame.cc = this->ctx.video_cc;
	frame.dts = (uint64_t)ts * 90;
	frame.pts = frame.dts + cts * 90;
	frame.pid = 0x100;
	frame.sid = 0xe0;
	frame.key = keyframe;

	/*
	* start new fragment if
	* - we have video key frame AND
	* - we have audio buffered or have no audio at all or stream is closed
	*/

	b = this->ctx.aframe;
	if (frame.key && (b && b->last > b->pos))
		ngx_rtmp_hls_update_fragment_ex(frame.dts, 0, 1);

	if (!this->ctx.opened) {
		return NGX_OK;
	}

	//printf("hls: video pts=%uL, dts=%uL, cc=%uL  \n", frame.pts, frame.dts, frame.cc);

	FILE* fp_264 = fopen("111.264", "ab+");
	fwrite(out.pos, out.last - out.pos, 1, fp_264);
	fclose(fp_264);

	if (ngx_rtmp_mpegts_write_frame(&this->ctx.file, &frame, &out) != NGX_OK) {
		printf("hls: video frame failed  \n");
	}

	this->ctx.video_cc = frame.cc;

	return NGX_OK;
}

void CHlsModule::ngx_rtmp_hls_restore_stream(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	ngx_file_t                      file;
	ssize_t                         ret;
	off_t                           offset;
	u_char                         *p, *last, *end, *next, *pa, *pp, c;
	ngx_rtmp_hls_frag_t            *f;
	double                          duration;
	ngx_int_t                       discont;
	uint64_t                        mag, key_id, base;
	static u_char                   buffer[4096];

	ngx_memzero(&file, sizeof(file));

	ngx_str_set(&file.name, "m3u8");

	file.fd = fopen((const char*)ctx->playlist.data, "wb");
	if (file.fd == NULL) {
		return;
	}

	offset = 0;
	ctx->nfrags = 0;
	f = NULL;
	duration = 0;
	discont = 0;
	key_id = 0;

	for ( ;; ) {

		//ret = ngx_read_file(&file, buffer, sizeof(buffer), offset);
		fseek(file.fd, offset, SEEK_SET);
		ret = fread(buffer, sizeof(buffer), 1, file.fd);
		if (ret <= 0) {
			goto done;
		}

		p = buffer;
		end = buffer + ret;

		for ( ;; ) {
			last = ngx_strlchr(p, end, '\n');

			if (last == NULL) {
				if (p == buffer) {
					goto done;
				}
				break;
			}

			next = last + 1;
			offset += (next - p);

			if (p != last && last[-1] == '\r') {
				last--;
			}


#define NGX_RTMP_MSEQ           "#EXT-X-MEDIA-SEQUENCE:"
#define NGX_RTMP_MSEQ_LEN       (sizeof(NGX_RTMP_MSEQ) - 1)


			if (ngx_memcmp(p, NGX_RTMP_MSEQ, NGX_RTMP_MSEQ_LEN) == 0) {

				ctx->frag = (uint64_t) strtod((const char *)
					&p[NGX_RTMP_MSEQ_LEN], NULL);

				printf("hls: restore sequence frag=%uL  \n", ctx->frag);
			}


#define NGX_RTMP_XKEY           "#EXT-X-KEY:"
#define NGX_RTMP_XKEY_LEN       (sizeof(NGX_RTMP_XKEY) - 1)

			if (ngx_memcmp(p, NGX_RTMP_XKEY, NGX_RTMP_XKEY_LEN) == 0) {

				/* recover key id from initialization vector */

				key_id = 0;
				base = 1;
				pp = last - 1;

				for ( ;; ) {
					if (pp < p) {
						printf("hls: failed to read key id  \n");
						break;
					}

					c = *pp;
					if (c == 'x') {
						break;
					}

					if (c >= '0' && c <= '9') {
						c -= '0';
						goto next;
					}

					c |= 0x20;

					if (c >= 'a' && c <= 'f') {
						c -= 'a' - 10;
						goto next;
					}

					printf("hls: bad character in key id  \n");
					break;

				next:

					key_id += base * c;
					base *= 0x10;
					pp--;
				}
			}


#define NGX_RTMP_EXTINF         "#EXTINF:"
#define NGX_RTMP_EXTINF_LEN     (sizeof(NGX_RTMP_EXTINF) - 1)


			if (ngx_memcmp(p, NGX_RTMP_EXTINF, NGX_RTMP_EXTINF_LEN) == 0) {

				duration = strtod((const char *) &p[NGX_RTMP_EXTINF_LEN], NULL);

				printf("hls: restore durarion=%.3f  \n", duration);
			}


#define NGX_RTMP_DISCONT        "#EXT-X-DISCONTINUITY"
#define NGX_RTMP_DISCONT_LEN    (sizeof(NGX_RTMP_DISCONT) - 1)


			if (ngx_memcmp(p, NGX_RTMP_DISCONT, NGX_RTMP_DISCONT_LEN) == 0) {

				discont = 1;

				printf("hls: discontinuity  \n");
			}

			/* find '.ts\r' */

			if (p + 4 <= last &&
				last[-3] == '.' && last[-2] == 't' && last[-1] == 's')
			{
				f = ngx_rtmp_hls_get_frag(ctx, hacf, ctx->nfrags);

				ngx_memzero(f, sizeof(*f));

				f->duration = duration;
				f->discont = discont;
				f->active = 1;
				f->id = 0;

				discont = 0;

				mag = 1;
				for (pa = last - 4; pa >= p; pa--) {
					if (*pa < '0' || *pa > '9') {
						break;
					}
					f->id += (*pa - '0') * mag;
					mag *= 10;
				}

				f->key_id = key_id;

				ngx_rtmp_hls_next_frag(ctx, hacf);

				printf("hls: restore fragment '%*s' id=%uL, "
					"duration=%.3f, frag=%uL, nfrags=%ui  \n",
					(size_t) (last - p), p, f->id, f->duration,
					ctx->frag, ctx->nfrags);
			}

			p = next;
		}
	}

done:
	fclose(file.fd);
}

ngx_int_t CHlsModule::ngx_rtmp_hls_publish(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_publish_t *v)
{
	u_char                         *p, *pp;
	ngx_rtmp_hls_frag_t            *f;
	ngx_buf_t                      *b;
	size_t                          len;
	ngx_rtmp_hls_variant_t         *var;
	ngx_uint_t                      n;

	if (hacf == NULL || !hacf->hls || hacf->path.len == 0) {
		goto next;
	}

	printf("hls: publish: name='%s' type='%s'  \n",
		v->name, v->type);

	if (ctx == NULL) {

		ctx = (ngx_rtmp_hls_ctx_t*)malloc(sizeof(ngx_rtmp_hls_ctx_t));

	} else {

		f = ctx->frags;
		b = ctx->aframe;

		ngx_memzero(ctx, sizeof(ngx_rtmp_hls_ctx_t));

		ctx->frags = f;
		ctx->aframe = b;

		if (b) {
			b->pos = b->last = b->start;
		}
	}

	if (ctx->frags == NULL) {
		ctx->frags = (ngx_rtmp_hls_frag_t*)malloc(sizeof(ngx_rtmp_hls_frag_t) *(hacf->winfrags * 2 + 1));
		if (ctx->frags == NULL) {
			return NGX_ERROR;
		}
	}

	if (ngx_strstr(v->name, "..")) {
		printf("hls: bad stream name: '%s'  \n", v->name);
		return NGX_ERROR;
	}

	ctx->name.len = ngx_strlen(v->name);
	ctx->name.data = (u_char*)malloc(ctx->name.len + 1);

	if (ctx->name.data == NULL) {
		return NGX_ERROR;
	}

	*ngx_cpymem(ctx->name.data, v->name, ctx->name.len) = 0;

	len = hacf->path.len + 1 + ctx->name.len + sizeof(".m3u8");
	if (hacf->nested) {
		len += sizeof("/index") - 1;
	}

	ctx->playlist.data = (u_char*)malloc(len);
	p = ngx_cpymem(ctx->playlist.data, hacf->path.data, hacf->path.len);

	if (p[-1] != '/') {
		*p++ = '/';
	}

	p = ngx_cpymem(p, ctx->name.data, ctx->name.len);

	/*
	* ctx->stream holds initial part of stream file path
	* however the space for the whole stream path
	* is allocated
	*/

	ctx->stream.len = p - ctx->playlist.data + 1;
	ctx->stream.data = (u_char*)malloc(
		ctx->stream.len + NGX_INT64_LEN +
		sizeof(".ts"));

	ngx_memcpy(ctx->stream.data, ctx->playlist.data, ctx->stream.len - 1);
	ctx->stream.data[ctx->stream.len - 1] = (hacf->nested ? '/' : '-');

	/* varint playlist path */

	if (hacf->variant) {
		var = (ngx_rtmp_hls_variant_t*)hacf->variant->elts;
		for (n = 0; n < hacf->variant->nelts; n++, var++) {
			if (ctx->name.len > var->suffix.len &&
				ngx_memcmp(var->suffix.data,
				ctx->name.data + ctx->name.len - var->suffix.len,
				var->suffix.len)
				== 0)
			{
				ctx->var = var;

				len = (size_t) (p - ctx->playlist.data);

				ctx->var_playlist.len = len - var->suffix.len + sizeof(".m3u8")
					- 1;
				ctx->var_playlist.data = (u_char*)malloc(
					ctx->var_playlist.len + 1);

				pp = ngx_cpymem(ctx->var_playlist.data, ctx->playlist.data,
					len - var->suffix.len);
				pp = ngx_cpymem(pp, ".m3u8", sizeof(".m3u8") - 1);
				*pp = 0;

				ctx->var_playlist_bak.len = ctx->var_playlist.len +
					sizeof(".bak") - 1;
				ctx->var_playlist_bak.data = (u_char*)malloc(
					ctx->var_playlist_bak.len + 1);

				pp = ngx_cpymem(ctx->var_playlist_bak.data,
					ctx->var_playlist.data,
					ctx->var_playlist.len);
				pp = ngx_cpymem(pp, ".bak", sizeof(".bak") - 1);
				*pp = 0;

				break;
			}
		}
	}


	/* playlist path */

	if (hacf->nested) {
		p = ngx_cpymem(p, "/index.m3u8", sizeof("/index.m3u8") - 1);
	} else {
		p = ngx_cpymem(p, ".m3u8", sizeof(".m3u8") - 1);
	}

	ctx->playlist.len = p - ctx->playlist.data;

	*p = 0;

	/* playlist bak (new playlist) path */

	ctx->playlist_bak.data = (u_char*)malloc(
		ctx->playlist.len + sizeof(".bak"));
	p = ngx_cpymem(ctx->playlist_bak.data, ctx->playlist.data,
		ctx->playlist.len);
	p = ngx_cpymem(p, ".bak", sizeof(".bak") - 1);

	ctx->playlist_bak.len = p - ctx->playlist_bak.data;

	*p = 0;

	/* key path */

	if (hacf->keys) {
		len = hacf->key_path.len + 1 + ctx->name.len + 1 + NGX_INT64_LEN
			+ sizeof(".key");

		ctx->keyfile.data = (u_char*)malloc(len);
		if (ctx->keyfile.data == NULL) {
			return NGX_ERROR;
		}

		p = ngx_cpymem(ctx->keyfile.data, hacf->key_path.data,
			hacf->key_path.len);

		if (p[-1] != '/') {
			*p++ = '/';
		}

		p = ngx_cpymem(p, ctx->name.data, ctx->name.len);
		*p++ = (hacf->nested ? '/' : '-');

		ctx->keyfile.len = p - ctx->keyfile.data;
	}

	printf("hls: playlist='%V' playlist_bak='%V' "
		"stream_pattern='%V' keyfile_pattern='%V'  \n",
		&ctx->playlist, &ctx->playlist_bak,
		&ctx->stream, &ctx->keyfile);

	if (hacf->continuous) {
		ngx_rtmp_hls_restore_stream(ctx,hacf);
	}

next:
	//return next_publish(s, v);
	return NGX_ERROR;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_close_stream(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_close_stream_t *v)
{
	if (hacf == NULL || !hacf->hls || ctx == NULL) {
		goto next;
	}

	printf("hls: close stream");

	ngx_rtmp_hls_close_fragment(ctx,hacf);

next:
	//return next_close_stream(s, v);
	return NGX_ERROR;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_parse_aac_header(ngx_rtmp_codec_ctx_t *codec_ctx, ngx_uint_t *objtype, ngx_uint_t *srindex, ngx_uint_t *chconf)
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
		printf("hls: unsupported adts sample rate:%ui  \n", *srindex);
		return NGX_ERROR;
	}

	*chconf = (b1 >> 3) & 0x0f;

	printf("hls: aac object_type:%ui, sample_rate_index:%ui, "
		"channel_config:%ui  \n", *objtype, *srindex, *chconf);

	return NGX_OK;
}

void CHlsModule::ngx_rtmp_hls_update_fragment(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, uint64_t ts, ngx_int_t boundary, ngx_uint_t flush_rate)
{
	ngx_rtmp_hls_frag_t        *f;
	ngx_msec_t                  ts_frag_len;
	ngx_int_t                   same_frag, force,discont;
	ngx_buf_t                  *b;
	int64_t                     d;
	f = NULL;
	force = 0;
	discont = 1;

	if (ctx->opened) {
		f = ngx_rtmp_hls_get_frag(ctx,hacf, ctx->nfrags);
		d = (int64_t) (ts - ctx->frag_ts);

		if (d > (int64_t) hacf->max_fraglen * 90 || d < -90000) {
			printf("hls: force fragment split: %.3f sec,   \n", d / 90000.);
			force = 1;

		} else {
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
		ngx_rtmp_hls_close_fragment(ctx,hacf);
		ngx_rtmp_hls_open_fragment(ctx, hacf, ts, discont);
	}

	b = ctx->aframe;
	if (ctx->opened && b && b->last > b->pos &&
		ctx->aframe_pts + (uint64_t) hacf->max_audio_delay * 90 / flush_rate
		< ts)
	{
		ngx_rtmp_hls_flush_audio(ctx);
	}
}

ngx_int_t CHlsModule::ngx_rtmp_hls_flush_audio(ngx_rtmp_hls_ctx_t *ctx)
{
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

	//printf("hls: flush audio pts=%uL, cc=%uL  \n", frame.pts,frame.cc);

	FILE* fp_aac = fopen("111.aac", "ab+");
	fwrite(b->pos,b->last-b->pos,1,fp_aac);
	fclose(fp_aac);

	rc = ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, b);

	if (rc != NGX_OK) {
		printf("hls: audio flush failed  \n");
	}

	ctx->audio_cc = frame.cc;
	b->pos = b->last = b->start;

	return rc;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_audio(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_header_t *h,
ngx_chain_t *in)
{
	uint64_t                        pts, est_pts;
	int64_t                         dpts;
	size_t                          bsize;
	ngx_buf_t                      *b;
	u_char                         *p;
	ngx_uint_t                      objtype, srindex, chconf, size;

	if (hacf == NULL || !hacf->hls || ctx == NULL ||
		codec_ctx == NULL  || h->mlen < 2)
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
	pts = (uint64_t) h->timestamp * 90;

	if (b->start + size > b->end) {
		printf("hls: too big audio frame  \n");
		return NGX_OK;
	}

	/*
	* start new fragment here if
	* there's no video at all, otherwise
	* do it in video handler
	*/

	ngx_rtmp_hls_update_fragment(ctx, hacf, pts, codec_ctx->avc_header == NULL, 2);

	if (b->last + size > b->end) {
		ngx_rtmp_hls_flush_audio(ctx);
	}

	//printf("hls: audio pts=%uL  \n", pts);

	if (b->last + 7 > b->end) {
		printf("hls: not enough buffer for audio header  \n");
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
		printf("hls: aac header error  \n");
		return NGX_OK;
	}

	/* we have 5 free bytes + 2 bytes of RTMP frame header */

	p[0] = 0xff;
	p[1] = 0xf1;
	p[2] = (u_char) (((objtype - 1) << 6) | (srindex << 2) |
		((chconf & 0x04) >> 2));
	p[3] = (u_char) (((chconf & 0x03) << 6) | ((size >> 11) & 0x03));
	p[4] = (u_char) (size >> 3);
	p[5] = (u_char) ((size << 5) | 0x1f);
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
	dpts = (int64_t) (est_pts - pts);

	//printf("hls: audio sync dpts=%L (%.5fs)  \n",
	//	dpts, dpts / 90000.);

	if (dpts <= (int64_t) hacf->sync * 90 &&
		dpts >= (int64_t) hacf->sync * -90)
	{
		ctx->aframe_num++;
		ctx->aframe_pts = est_pts;
		return NGX_OK;
	}

	ctx->aframe_base = pts;
	ctx->aframe_num  = 1;

	//printf("hls: audio sync gap dpts=%L (%.5fs)  \n",
	//	dpts, dpts / 90000.);

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_video(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf, ngx_rtmp_codec_ctx_t *codec_ctx, ngx_rtmp_header_t *h,
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

		printf("hls: h264 NAL type=%ui, len=%uD  \n",
			(ngx_uint_t) nal_type, len);

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
					printf("hls: error appending AUD NAL  \n");
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
				printf("hls: error appenging SPS/PPS NALs  \n");
			}
			sps_pps_sent = 1;
			break;
		}

		/* AnnexB prefix */

		if (out.end - out.last < 5) {
			printf("hls: not enough buffer for AnnexB prefix  \n");
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

		if (out.end - out.last < (ngx_int_t) len) {
			printf("hls: not enough buffer for NAL  \n");
			return NGX_OK;
		}

		if (ngx_rtmp_hls_copy(out.last, &p, len - 1, &in) != NGX_OK) {
			return NGX_ERROR;
		}

		out.last += (len - 1);
	}

	ngx_memzero(&frame, sizeof(frame));

	frame.cc = ctx->video_cc;
	frame.dts = (uint64_t) h->timestamp * 90;
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

	ngx_rtmp_hls_update_fragment(ctx, hacf, frame.dts, boundary, 1);

	if (!ctx->opened) {
		return NGX_OK;
	}

	//printf("hls: video pts=%uL, dts=%uL, cc=%uL  \n", frame.pts, frame.dts, frame.cc);

	FILE* fp_264 = fopen("111.264", "ab+");
	fwrite(out.pos,out.last-out.pos,1,fp_264);
	fclose(fp_264);

	if (ngx_rtmp_mpegts_write_frame(&ctx->file, &frame, &out) != NGX_OK) {
		printf("hls: video frame failed  \n");
	}

	ctx->video_cc = frame.cc;

	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_stream_begin()
{
	//return next_stream_begin(s, v);
	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_stream_eof(ngx_rtmp_hls_ctx_t *ctx, ngx_rtmp_hls_app_conf_t *hacf)
{
	ngx_rtmp_hls_flush_audio(ctx);

	ngx_rtmp_hls_close_fragment(ctx,hacf);

	//return next_stream_eof(s, v);
	return NGX_OK;
}

ngx_int_t CHlsModule::ngx_rtmp_hls_cleanup_dir(ngx_str_t *ppath, ngx_msec_t playlen)
{
	//ngx_dir_t               dir;
	//time_t                  mtime, max_age;
	//ngx_err_t               err;
	//ngx_str_t               name, spath;
	//u_char                 *p;
	//ngx_int_t               nentries, nerased;
	//u_char                  path[NGX_MAX_PATH + 1];

	//ngx_log_debug2(NGX_LOG_DEBUG_RTMP, ngx_cycle->log, 0,
	//	"hls: cleanup path='%V' playlen=%M",
	//	ppath, playlen);

	//if (ngx_open_dir(ppath, &dir) != NGX_OK) {
	//	ngx_log_debug1(NGX_LOG_DEBUG_RTMP, ngx_cycle->log, ngx_errno,
	//		"hls: cleanup open dir failed '%V'", ppath);
	//	return NGX_ERROR;
	//}

	//nentries = 0;
	//nerased = 0;

	//for ( ;; ) {
	//	ngx_set_errno(0);

	//	if (ngx_read_dir(&dir) == NGX_ERROR) {
	//		err = ngx_errno;

	//		if (ngx_close_dir(&dir) == NGX_ERROR) {
	//			ngx_log_error(NGX_LOG_CRIT, ngx_cycle->log, ngx_errno,
	//				"hls: cleanup " ngx_close_dir_n " \"%V\" failed",
	//				ppath);
	//		}

	//		if (err == NGX_ENOMOREFILES) {
	//			return nentries - nerased;
	//		}

	//		ngx_log_error(NGX_LOG_CRIT, ngx_cycle->log, err,
	//			"hls: cleanup " ngx_read_dir_n
	//			" '%V' failed", ppath);
	//		return NGX_ERROR;
	//	}

	//	name.data = ngx_de_name(&dir);
	//	if (name.data[0] == '.') {
	//		continue;
	//	}

	//	name.len = ngx_de_namelen(&dir);

	//	p = ngx_snprintf(path, sizeof(path) - 1, "%V/%V", ppath, &name);
	//	*p = 0;

	//	spath.data = path;
	//	spath.len = p - path;

	//	nentries++;

	//	if (!dir.valid_info && ngx_de_info(path, &dir) == NGX_FILE_ERROR) {
	//		ngx_log_error(NGX_LOG_CRIT, ngx_cycle->log, ngx_errno,
	//			"hls: cleanup " ngx_de_info_n " \"%V\" failed",
	//			&spath);

	//		continue;
	//	}

	//	if (ngx_de_is_dir(&dir)) {

	//		if (ngx_rtmp_hls_cleanup_dir(&spath, playlen) == 0) {
	//			ngx_log_debug1(NGX_LOG_DEBUG_RTMP, ngx_cycle->log, 0,
	//				"hls: cleanup dir '%V'", &name);

	//			/*
	//			* null-termination gets spoiled in win32
	//			* version of ngx_open_dir
	//			*/

	//			*p = 0;

	//			if (ngx_delete_dir(path) == NGX_FILE_ERROR) {
	//				ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, ngx_errno,
	//					"hls: cleanup " ngx_delete_dir_n
	//					" failed on '%V'", &spath);
	//			} else {
	//				nerased++;
	//			}
	//		}

	//		continue;
	//	}

	//	if (!ngx_de_is_file(&dir)) {
	//		continue;
	//	}

	//	if (name.len >= 3 && name.data[name.len - 3] == '.' &&
	//		name.data[name.len - 2] == 't' &&
	//		name.data[name.len - 1] == 's')
	//	{
	//		max_age = playlen / 500;

	//	} else if (name.len >= 5 && name.data[name.len - 5] == '.' &&
	//		name.data[name.len - 4] == 'm' &&
	//		name.data[name.len - 3] == '3' &&
	//		name.data[name.len - 2] == 'u' &&
	//		name.data[name.len - 1] == '8')
	//	{
	//		max_age = playlen / 1000;

	//	} else if (name.len >= 4 && name.data[name.len - 4] == '.' &&
	//		name.data[name.len - 3] == 'k' &&
	//		name.data[name.len - 2] == 'e' &&
	//		name.data[name.len - 1] == 'y')
	//	{
	//		max_age = playlen / 500;

	//	} else {
	//		ngx_log_debug1(NGX_LOG_DEBUG_RTMP, ngx_cycle->log, 0,
	//			"hls: cleanup skip unknown file type '%V'", &name);
	//		continue;
	//	}

	//	mtime = ngx_de_mtime(&dir);
	//	if (mtime + max_age > ngx_cached_time->sec) {
	//		continue;
	//	}

	//	ngx_log_debug3(NGX_LOG_DEBUG_RTMP, ngx_cycle->log, 0,
	//		"hls: cleanup '%V' mtime=%T age=%T",
	//		&name, mtime, ngx_cached_time->sec - mtime);

	//	if (ngx_delete_file(path) == NGX_FILE_ERROR) {
	//		ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, ngx_errno,
	//			"hls: cleanup " ngx_delete_file_n " failed on '%V'",
	//			&spath);
	//		continue;
	//	}

	//	nerased++;
	//}

return NGX_OK;
}

time_t CHlsModule::ngx_rtmp_hls_cleanup(void *data)
{
	ngx_rtmp_hls_cleanup_t *cleanup = (ngx_rtmp_hls_cleanup_t*)data;

	ngx_rtmp_hls_cleanup_dir(&cleanup->path, cleanup->playlen);

	return cleanup->playlen / 500;
}

#if 0
 
static char *
ngx_rtmp_hls_variant(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_rtmp_hls_app_conf_t  *hacf = conf;

	ngx_str_t                *value, *arg;
	ngx_uint_t                n;
	ngx_rtmp_hls_variant_t   *var;

	value = cf->args->elts;

	if (hacf->variant == NULL) {
		hacf->variant = ngx_array_create(cf->pool, 1,
			sizeof(ngx_rtmp_hls_variant_t));
		if (hacf->variant == NULL) {
			return NGX_CONF_ERROR;
		}
	}

	var = ngx_array_push(hacf->variant);
	if (var == NULL) {
		return NGX_CONF_ERROR;
	}

	ngx_memzero(var, sizeof(ngx_rtmp_hls_variant_t));

	var->suffix = value[1];

	if (cf->args->nelts == 2) {
		return NGX_CONF_OK;
	}

	if (ngx_array_init(&var->args, cf->pool, cf->args->nelts - 2,
		sizeof(ngx_str_t))
		!= NGX_OK)
	{
		return NGX_CONF_ERROR;
	}

	arg = ngx_array_push_n(&var->args, cf->args->nelts - 2);
	if (arg == NULL) {
		return NGX_CONF_ERROR;
	}

	for (n = 2; n < cf->args->nelts; n++) {
		*arg++ = value[n];
	}

	return NGX_CONF_OK;
}

static void *
ngx_rtmp_hls_create_app_conf(ngx_conf_t *cf)
{
	ngx_rtmp_hls_app_conf_t *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_rtmp_hls_app_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->hls = NGX_CONF_UNSET;
	conf->fraglen = NGX_CONF_UNSET_MSEC;
	conf->max_fraglen = NGX_CONF_UNSET_MSEC;
	conf->muxdelay = NGX_CONF_UNSET_MSEC;
	conf->sync = NGX_CONF_UNSET_MSEC;
	conf->playlen = NGX_CONF_UNSET_MSEC;
	conf->continuous = NGX_CONF_UNSET;
	conf->nested = NGX_CONF_UNSET;
	conf->naming = NGX_CONF_UNSET_UINT;
	conf->slicing = NGX_CONF_UNSET_UINT;
	conf->type = NGX_CONF_UNSET_UINT;
	conf->max_audio_delay = NGX_CONF_UNSET_MSEC;
	conf->audio_buffer_size = NGX_CONF_UNSET_SIZE;
	conf->cleanup = NGX_CONF_UNSET;
	conf->granularity = NGX_CONF_UNSET;
	conf->keys = NGX_CONF_UNSET;
	conf->frags_per_key = NGX_CONF_UNSET_UINT;

	return conf;
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

static ngx_int_t
ngx_rtmp_hls_postconfiguration(ngx_conf_t *cf)
{
	ngx_rtmp_core_main_conf_t   *cmcf;
	ngx_rtmp_handler_pt         *h;

	cmcf = ngx_rtmp_conf_get_module_main_conf(cf, ngx_rtmp_core_module);

	h = ngx_array_push(&cmcf->events[NGX_RTMP_MSG_VIDEO]);
	*h = ngx_rtmp_hls_video;

	h = ngx_array_push(&cmcf->events[NGX_RTMP_MSG_AUDIO]);
	*h = ngx_rtmp_hls_audio;

	next_publish = ngx_rtmp_publish;
	ngx_rtmp_publish = ngx_rtmp_hls_publish;

	next_close_stream = ngx_rtmp_close_stream;
	ngx_rtmp_close_stream = ngx_rtmp_hls_close_stream;

	next_stream_begin = ngx_rtmp_stream_begin;
	ngx_rtmp_stream_begin = ngx_rtmp_hls_stream_begin;

	next_stream_eof = ngx_rtmp_stream_eof;
	ngx_rtmp_stream_eof = ngx_rtmp_hls_stream_eof;

	return NGX_OK;
}

#endif


u_char * CHlsModule::ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...)
{
	u_char   *p;
	va_list   args;

	va_start(args, fmt);
	p = ngx_vslprintf(buf, buf + max, fmt, args);
	va_end(args);

	return p;
}


u_char * CHlsModule::ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...)
{
	u_char   *p;
	va_list   args;

	va_start(args, fmt);
	p = ngx_vslprintf(buf, last, fmt, args);
	va_end(args);

	return p;
}


u_char * CHlsModule::ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args)
{
	u_char                *p, zero;
	int                    d;
	double                 f;
	size_t                 len, slen;
	int64_t                i64;
	uint64_t               ui64, frac;
	ngx_msec_t             ms;
	ngx_uint_t             width, sign, hex, max_width, frac_width, scale, n;
	ngx_str_t             *v;
	ngx_variable_value_t  *vv;

	while (*fmt && buf < last) {

		/*
		* "buf < last" means that we could copy at least one character:
		* the plain character, "%%", "%c", and minus without the checking
		*/

		if (*fmt == '%') {

			i64 = 0;
			ui64 = 0;

			zero = (u_char)((*++fmt == '0') ? '0' : ' ');
			width = 0;
			sign = 1;
			hex = 0;
			max_width = 0;
			frac_width = 0;
			slen = (size_t)-1;

			while (*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt++ - '0');
			}


			for (;;) {
				switch (*fmt) {

				case 'u':
					sign = 0;
					fmt++;
					continue;

				case 'm':
					max_width = 1;
					fmt++;
					continue;

				case 'X':
					hex = 2;
					sign = 0;
					fmt++;
					continue;

				case 'x':
					hex = 1;
					sign = 0;
					fmt++;
					continue;

				case '.':
					fmt++;

					while (*fmt >= '0' && *fmt <= '9') {
						frac_width = frac_width * 10 + (*fmt++ - '0');
					}

					break;

				case '*':
					slen = va_arg(args, size_t);
					fmt++;
					continue;

				default:
					break;
				}

				break;
			}


			switch (*fmt) {

			case 'V':
				v = va_arg(args, ngx_str_t *);

				len = ngx_min(((size_t)(last - buf)), v->len);
				buf = ngx_cpymem(buf, v->data, len);
				fmt++;

				continue;

			case 'v':
				vv = va_arg(args, ngx_variable_value_t *);

				len = ngx_min(((size_t)(last - buf)), vv->len);
				buf = ngx_cpymem(buf, vv->data, len);
				fmt++;

				continue;

			case 's':
				p = va_arg(args, u_char *);

				if (slen == (size_t)-1) {
					while (*p && buf < last) {
						*buf++ = *p++;
					}

				}
				else {
					len = ngx_min(((size_t)(last - buf)), slen);
					buf = ngx_cpymem(buf, p, len);
				}

				fmt++;

				continue;

			case 'O':
				i64 = (int64_t)va_arg(args, off_t);
				sign = 1;
				break;

			case 'P':
				i64 = (int64_t)va_arg(args, ngx_pid_t);
				sign = 1;
				break;

			case 'T':
				i64 = (int64_t)va_arg(args, time_t);
				sign = 1;
				break;

			case 'M':
				ms = (ngx_msec_t)va_arg(args, ngx_msec_t);
				if ((ngx_msec_int_t)ms == -1) {
					sign = 1;
					i64 = -1;
				}
				else {
					sign = 0;
					ui64 = (uint64_t)ms;
				}
				break;

			case 'z':
				if (sign) {
					i64 = (int64_t)va_arg(args, ssize_t);
				}
				else {
					ui64 = (uint64_t)va_arg(args, size_t);
				}
				break;

			case 'i':
				if (sign) {
					i64 = (int64_t)va_arg(args, ngx_int_t);
				}
				else {
					ui64 = (uint64_t)va_arg(args, ngx_uint_t);
				}

				if (max_width) {
					width = NGX_INT_T_LEN;
				}

				break;

			case 'd':
				if (sign) {
					i64 = (int64_t)va_arg(args, int);
				}
				else {
					ui64 = (uint64_t)va_arg(args, u_int);
				}
				break;

			case 'l':
				if (sign) {
					i64 = (int64_t)va_arg(args, long);
				}
				else {
					ui64 = (uint64_t)va_arg(args, u_long);
				}
				break;

			case 'D':
				if (sign) {
					i64 = (int64_t)va_arg(args, int32_t);
				}
				else {
					ui64 = (uint64_t)va_arg(args, uint32_t);
				}
				break;

			case 'L':
				if (sign) {
					i64 = va_arg(args, int64_t);
				}
				else {
					ui64 = va_arg(args, uint64_t);
				}
				break;

			case 'A':
				if (sign) {
					i64 = (int64_t)va_arg(args, ngx_atomic_int_t);
				}
				else {
					ui64 = (uint64_t)va_arg(args, ngx_atomic_uint_t);
				}

				if (max_width) {
					width = NGX_ATOMIC_T_LEN;
				}

				break;

			case 'f':
				f = va_arg(args, double);

				if (f < 0) {
					*buf++ = '-';
					f = -f;
				}

				ui64 = (int64_t)f;
				frac = 0;

				if (frac_width) {

					scale = 1;
					for (n = frac_width; n; n--) {
						scale *= 10;
					}

					frac = (uint64_t)((f - (double)ui64) * scale + 0.5);

					if (frac == scale) {
						ui64++;
						frac = 0;
					}
				}

				buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

				if (frac_width) {
					if (buf < last) {
						*buf++ = '.';
					}

					buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width);
				}

				fmt++;

				continue;

#if !(WIN32)
			case 'r':
				i64 = (int64_t)va_arg(args, rlim_t);
				sign = 1;
				break;
#endif

			case 'p':
				ui64 = (uintptr_t)va_arg(args, void *);
				hex = 2;
				sign = 0;
				zero = '0';
				width = 2 * sizeof(void *);
				break;

			case 'c':
				d = va_arg(args, int);
				*buf++ = (u_char)(d & 0xff);
				fmt++;

				continue;

			case 'Z':
				*buf++ = '\0';
				fmt++;

				continue;

			case 'N':
#if (WIN32)
				*buf++ = CR;
				if (buf < last) {
					*buf++ = LF;
				}
#else
				*buf++ = LF;
#endif
				fmt++;

				continue;

			case '%':
				*buf++ = '%';
				fmt++;

				continue;

			default:
				*buf++ = *fmt++;

				continue;
			}

			if (sign) {
				if (i64 < 0) {
					*buf++ = '-';
					ui64 = (uint64_t)-i64;

				}
				else {
					ui64 = (uint64_t)i64;
				}
			}

			buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);

			fmt++;

		}
		else {
			*buf++ = *fmt++;
		}
	}

	return buf;
}


u_char * CHlsModule::ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width)
{
	u_char         *p, temp[NGX_INT64_LEN + 1];
	/*
	* we need temp[NGX_INT64_LEN] only,
	* but icc issues the warning
	*/
	size_t          len;
	uint32_t        ui32;
	static u_char   hex[] = "0123456789abcdef";
	static u_char   HEX[] = "0123456789ABCDEF";

	p = temp + NGX_INT64_LEN;

	if (hexadecimal == 0) {

		if (ui64 <= (uint64_t)NGX_MAX_UINT32_VALUE) {

			/*
			* To divide 64-bit numbers and to find remainders
			* on the x86 platform gcc and icc call the libc functions
			* [u]divdi3() and [u]moddi3(), they call another function
			* in its turn.  On FreeBSD it is the qdivrem() function,
			* its source code is about 170 lines of the code.
			* The glibc counterpart is about 150 lines of the code.
			*
			* For 32-bit numbers and some divisors gcc and icc use
			* a inlined multiplication and shifts.  For example,
			* unsigned "i32 / 10" is compiled to
			*
			*     (i32 * 0xCCCCCCCD) >> 35
			*/

			ui32 = (uint32_t)ui64;

			do {
				*--p = (u_char)(ui32 % 10 + '0');
			} while (ui32 /= 10);

		}
		else {
			do {
				*--p = (u_char)(ui64 % 10 + '0');
			} while (ui64 /= 10);
		}

	}
	else if (hexadecimal == 1) {

		do {

			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = hex[(uint32_t)(ui64 & 0xf)];

		} while (ui64 >>= 4);

	}
	else { /* hexadecimal == 2 */

		do {

			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = HEX[(uint32_t)(ui64 & 0xf)];

		} while (ui64 >>= 4);
	}

	/* zero or space padding */

	len = (temp + NGX_INT64_LEN) - p;

	while (len++ < width && buf < last) {
		*buf++ = zero;
	}

	/* number safe copy */

	len = (temp + NGX_INT64_LEN) - p;

	if (buf + len > last) {
		len = last - buf;
	}

	return ngx_cpymem(buf, p, len);
}


ngx_int_t CHlsModule::ngx_rtmp_is_codec_header(ngx_chain_t *in)
{
	return in->buf->pos + 1 < in->buf->last && in->buf->pos[1] == 0;
}


void * CHlsModule::ngx_rtmp_rmemcpy(void *dst, const void* src, size_t n)
{
	u_char     *d, *s;

	d = (u_char*)dst;
	s = (u_char*)src + n - 1;

	while (s >= (u_char*)src) {
		*d++ = *s--;
	}

	return dst;
}

u_char * CHlsModule::ngx_sprintf(u_char *buf, const char *fmt, ...)
{
	u_char   *p;
	va_list   args;

	va_start(args, fmt);
	p = ngx_vslprintf(buf, (u_char*)(void *)-1, fmt, args);
	va_end(args);

	return p;
}

u_char * CHlsModule::ngx_strlchr(u_char *p, u_char *last, u_char c)
{
	while (p < last) {

		if (*p == c) {
			return p;
		}

		p++;
	}

	return NULL;
}