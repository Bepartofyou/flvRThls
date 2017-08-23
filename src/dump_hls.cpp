/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2016 Marc Noirot <marc.noirot AT gmail.com>

    This file is part of FLVMeta.

    FLVMeta is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLVMeta is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLVMeta; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "dump.h"
#include "dump_hls.h"

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <string>
#include <math.h>

#ifdef WIN32
#define sprintf	sprintf_s
#endif

static std::string num2str(double i)
{
	std::stringstream ss;
	ss << i;
	return ss.str();
}

static int str2num(std::string s)
{
	int num;
	std::stringstream ss(s);
	ss >> num;
	return num;
}

/* hls FLV file full dump callbacks */

static int hls_on_header(flv_header * header, flv_parser * parser) {
    int * n;

    n = (int*) malloc(sizeof(uint32));
    if (n == NULL) {
        return ERROR_MEMORY;
    }

    parser->user_data = n;
    *n = 0;

    printf("Magic: %.3s\n", header->signature);
    printf("Version: %" PRI_BYTE "u\n", header->version);
    printf("Has audio: %s\n", flv_header_has_audio(*header) ? "yes" : "no");
    printf("Has video: %s\n", flv_header_has_video(*header) ? "yes" : "no");
    printf("Offset: %u\n", swap_uint32(header->offset));
    return OK;
}

static int hls_on_tag(flv_tag * tag, flv_parser * parser) {
    int * n;

    /* increment current tag number */
    n = ((int*)parser->user_data);
    ++(*n);

    printf("--- Tag #%u at 0x%" FILE_OFFSET_PRINTF_FORMAT "X", *n, FILE_OFFSET_PRINTF_TYPE(parser->stream->current_tag_offset));
    printf(" (%" FILE_OFFSET_PRINTF_FORMAT "u) ---\n", FILE_OFFSET_PRINTF_TYPE(parser->stream->current_tag_offset));
    printf("Tag type: %s\n", dump_string_get_tag_type(tag));
    printf("Body length: %u\n", flv_tag_get_body_length(*tag));
    printf("Timestamp: %u\n", flv_tag_get_timestamp(*tag));

    return OK;
}

static int hls_on_tag_ex(flv_tag * tag, flv_parser * parser) {

	printf("--- Tag #%u at 0x%" FILE_OFFSET_PRINTF_FORMAT "X", 1, FILE_OFFSET_PRINTF_TYPE(parser->stream->current_tag_offset));
	printf(" (%" FILE_OFFSET_PRINTF_FORMAT "u) ---\n", FILE_OFFSET_PRINTF_TYPE(parser->stream->current_tag_offset));
	printf("Tag type: %s\n", dump_string_get_tag_type(tag));
	printf("Body length: %u\n", flv_tag_get_body_length(*tag));
	printf("Timestamp: %u\n", flv_tag_get_timestamp(*tag));

	return OK;
}

static std::string get_flv_key(std::string name, std::string outpath) {

#ifdef WIN32
	return name.substr(0, name.rfind(".flv"));
#else
	std::string tmp1 = name.substr(0, name.rfind(".flv"));

	int index = tmp1.rfind("\/");
	std::string tmp2 = tmp1.substr(index + 1, tmp1.length() - index);

	return outpath + "/" + tmp2;
	//return tmp1.substr(index + 1, tmp1.length() - index);
#endif
}

static std::string get_ts_name(flv_parser * parser){

	char conunt[100] = { 0 };

	//sprintf(conunt, "%.4u", parser->stream->hlsconfig.hls_count + parser->key_ID_start / parser->stream->hlsconfig.hls_segment_num);
	//std::string strfile = get_flv_key(std::string(parser->stream->flvname),std::string(parser->stream->outpath)) + "-keyframeID-" +
	//	num2str(parser->stream->hlsconfig.key_frame_current + parser->key_ID_start > 0 ? parser->stream->hlsconfig.key_frame_current + parser->key_ID_start - 1 : 0) 
	//	+ "-" + std::string(conunt) + ".ts\n";
	if (parser->key_ID_start != 0)
	{
		sprintf(conunt, "%.4u", parser->stream->hlsconfig.ts_count-1 + parser->key_ID_start / parser->stream->hlsconfig.hls_segment_num);
	}
	else
	{
		sprintf(conunt, "%.4u", parser->stream->hlsconfig.ts_count + parser->key_ID_start / parser->stream->hlsconfig.hls_segment_num);
	}


	//sprintf(conunt, "%.4u", parser->stream->hlsconfig.ts_count + parser->key_ID_start / parser->stream->hlsconfig.hls_segment_num);
	std::string strfile = get_flv_key(std::string(parser->stream->flvname), std::string(parser->stream->outpath)) + "-keyframeID-" +
		num2str(parser->stream->hlsconfig.ts_fragment_id + parser->key_ID_start > 0 ? parser->stream->hlsconfig.ts_fragment_id + parser->key_ID_start - 1 : 0)
		+ "-" + std::string(conunt) + ".ts";

	//strcpy(name, strfile.c_str());
	return strfile;
}

static int hls_segment(flv_parser * parser) {

	if (parser->b_ts && (parser->key_ID_start == 0 || (parser->key_ID_start != 0 && parser->stream->hlsconfig.key_frame_count != 0 )))
	{
		//bepartofyou
		if (parser->key_ID_end != -1 && parser->stream->hlsconfig.key_frame_count + parser->key_ID_start < parser->key_ID_end)
		{
			parser->hlsmodule->ngx_rtmp_hls_close_fragment_ex2();
		}
		else
		{
			parser->hlsmodule->ngx_rtmp_hls_close_fragment_ex2();
			//parser->hlsmodule->ngx_rtmp_hls_close_fragment_ex();
		}
		
		if (((parser->key_ID_end == -1) && (parser->stream->hlsconfig.key_frame_count + parser->key_ID_start < parser->stream->keyframePos.size())) ||
			((int)(parser->stream->hlsconfig.key_frame_count + parser->key_ID_start) < (int)parser->key_ID_end))
		{
			
			parser->hlsmodule->ngx_rtmp_hls_open_fragment_ex(get_ts_name(parser).c_str(), 0, 0);
		}
	}

	if (parser->b_m3u8)
	{
		if (parser->stream->hlsconfig.hls_file == NULL){
			//int index = parser->stream->flvname.rfind(".flv");
			std::string hlsname = get_flv_key(std::string(parser->stream->flvname), std::string(parser->stream->outpath)) + ".m3u8";
			parser->stream->hlsconfig.hls_file = fopen(hlsname.c_str(), "wb");

			//fwrite("#EXTM3U\n", strlen("#EXTM3U\n"), 1, parser->stream->hlsconfig.hls_file);
			//fwrite("#EXT-X-VERSION:3\n", strlen("#EXT-X-VERSION:3\n"), 1, parser->stream->hlsconfig.hls_file);
			//fwrite("#EXT-X-TARGETDURATION:12\n", strlen("#EXT-X-TARGETDURATION:12\n"), 1, parser->stream->hlsconfig.hls_file);
			//fwrite("#EXT-X-MEDIA-SEQUENCE:0\n", strlen("#EXT-X-MEDIA-SEQUENCE:0\n"), 1, parser->stream->hlsconfig.hls_file);

			parser->hls_content.push_back("#EXTM3U\n");
			parser->hls_content.push_back("#EXT-X-VERSION:3\n");
			parser->hls_content.push_back("#EXT-X-TARGETDURATION:12\n");
			parser->hls_content.push_back("#EXT-X-MEDIA-SEQUENCE:0\n");
		}
		else
		{
			uint32_t interval = parser->stream->hlsconfig.hls_end_ts - parser->stream->hlsconfig.hls_start_ts;
			parser->stream->hlsconfig.hls_start_ts = parser->stream->hlsconfig.hls_end_ts;

			parser->stream->hlsconfig.hls_segment_duration = parser->stream->hlsconfig.hls_segment_duration > interval ?
				parser->stream->hlsconfig.hls_segment_duration : interval;

			char ts[100] = { 0 };
			sprintf(ts, "%.6f", (double)interval / (double)1000);
			//std::string tmp = "#EXTINF:" + num2str((double)interval/(double)1000) + ",\n";
			std::string strts = "#EXTINF:" + std::string(ts) + ",\n";

			//fwrite(strts.c_str(), strts.size(), 1, parser->stream->hlsconfig.hls_file);
			parser->hls_content.push_back(strts);

			char conunt[100] = { 0 };
			sprintf(conunt, "%.4u", parser->stream->hlsconfig.hls_count);
			std::string strfile = get_flv_key(std::string(parser->stream->flvname), std::string(parser->stream->outpath)) + "-keyframeID-" +
				num2str(parser->stream->hlsconfig.key_frame_current > 0 ? parser->stream->hlsconfig.key_frame_current - 1 : 0) + "-" + std::string(conunt) + ".ts\n";

			//fwrite(strfile.c_str(), strfile.size(), 1, parser->stream->hlsconfig.hls_file);
			//fflush(parser->stream->hlsconfig.hls_file);
			parser->hls_content.push_back(strfile);

			//end
			if (parser->stream->hlsconfig.key_frame_count == parser->stream->keyframePos.size()){
				//fwrite("#EXT-X-ENDLIST", strlen("#EXT-X-ENDLIST"), 1, parser->stream->hlsconfig.hls_file);
				//fclose(parser->stream->hlsconfig.hls_file);

				parser->hls_content.push_back("#EXT-X-ENDLIST");
				std::string strts = "#EXT-X-TARGETDURATION:" + num2str(ceil((double)parser->stream->hlsconfig.hls_segment_duration / (double)1000)) + "\n";
				parser->hls_content[2] = strts;

				for (size_t index = 0; index < parser->hls_content.size(); index++)
					fwrite(parser->hls_content[index].c_str(), parser->hls_content[index].size(), 1, parser->stream->hlsconfig.hls_file);

				fclose(parser->stream->hlsconfig.hls_file);

				std::vector<std::string>().swap(parser->hls_content);
			}
		}
	}

	return OK;
}

static int hls_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    printf("* Video codec: %s\n", dump_string_get_video_codec(vt));
    printf("* Video frame type: %s\n", dump_string_get_video_frame_type(vt));

    /* if AVC, detect frame type and composition time */
    if (flv_video_tag_codec_id(vt) == FLV_VIDEO_TAG_CODEC_AVC) {
        flv_avc_packet_type type;

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_avc_packet_type)) < sizeof(flv_avc_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        printf("* AVC packet type: %s\n", dump_string_get_avc_packet_type(type));

		/* composition time */
		if (type == FLV_AVC_PACKET_TYPE_SEQUENCE_HEADER) {

			read_avc_sps_pps(parser);

			parser->stream->flag_videoconfig = true;
		}

        ///* composition time */
        //if (type == FLV_AVC_PACKET_TYPE_NALU) {
        //    uint24_be composition_time;

        //    if (flv_read_tag_body(parser->stream, &composition_time, sizeof(uint24_be)) < sizeof(uint24_be)) {
        //        return ERROR_INVALID_TAG;
        //    }

        //    printf("* Composition time offset: %i\n", uint24_be_to_uint32(composition_time));
        //}
    }

    return OK;
}

static int hls_on_video_tag_ex(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {

	//return OK;

	//printf("* Video codec: %s\n", dump_string_get_video_codec(vt));
	//printf("* Video frame type: %s\n", dump_string_get_video_frame_type(vt));

	/* if AVC, detect frame type and composition time */
	if (flv_video_tag_codec_id(vt) == FLV_VIDEO_TAG_CODEC_AVC) {
		flv_avc_packet_type type;

		/* packet type */
		if (flv_read_tag_body(parser->stream, &type, sizeof(flv_avc_packet_type)) < sizeof(flv_avc_packet_type)) {
			return ERROR_INVALID_TAG;
		}

		//printf("* AVC packet type: %s\n", dump_string_get_avc_packet_type(type));

		/* sequence header */
		if (type == FLV_AVC_PACKET_TYPE_SEQUENCE_HEADER) {

			parser->stream->hlsconfig.key_frame_count++;
			printf("1111111111111111111\n");
			read_avc_sps_pps(parser);

			parser->stream->h264_header_count++;

			//if (parser->stream->h264 == NULL){
			//	std::string h264_name = num2str(parser->stream->h264_header_count) + ".h264";
			//	parser->stream->h264 = fopen(h264_name.c_str(), "wb");
			//}
			//else{
			//	fclose(parser->stream->h264);

			//	std::string h264_name = num2str(parser->stream->h264_header_count) + ".h264";
			//	parser->stream->h264 = fopen(h264_name.c_str(), "wb");
			//}
		}
		else{

			/*get first pkt ts*/
			if (!parser->stream->hlsconfig.flag_first_ts){
				parser->stream->hlsconfig.first_ts = flv_tag_get_timestamp(*tag);
				parser->stream->hlsconfig.hls_start_ts = parser->stream->hlsconfig.first_ts;
				printf("first pkt ts:%u, video\n", parser->stream->hlsconfig.first_ts);

				parser->stream->hlsconfig.hls_segment_num = 6;
				hls_segment(parser);

				parser->stream->hlsconfig.flag_first_ts = true;
			}

			//if (parser->stream->h264)
			{

				if (parser->stream->h264_buffer == NULL){

					parser->stream->h264_buf_max = uint24_be_to_uint32(tag->body_length);
					parser->stream->h264_buffer = (uint8_t*)malloc(sizeof(uint8_t)*parser->stream->h264_buf_max);
				}
				else
				{
					if (parser->stream->h264_buf_max < uint24_be_to_uint32(tag->body_length))
					{
						parser->stream->h264_buf_max = uint24_be_to_uint32(tag->body_length);
						parser->stream->h264_buffer = (uint8_t*)realloc(parser->stream->h264_buffer, parser->stream->h264_buf_max);
					}
				}

				/* read the composition time */
				uint24 composition_time;
				if (flv_read_tag_body(parser->stream, &composition_time, sizeof(uint24)) < sizeof(uint24)) {
					return FLV_ERROR_EOF;
				}

				static uint8_t video_buffer[1024 * 1024 * 10];
				uint32_t video_len = 0;

				/* read raw h264 */
				uint8_t statcode[4] = { 0, 0, 0, 1 };
				while (parser->stream->current_tag_body_length > 4)
				{
					//uint8_t   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };
					//memcpy(video_buffer + video_len, aud_nal, 6);
					//video_len += 6;

					uint32_nal nalusize = { 0 };
					if (flv_read_tag_body(parser->stream, &nalusize, sizeof(uint32_t)) < sizeof(uint32_t)) {
						return FLV_ERROR_EOF;
					}

					int x = uint32_be_to_uint32(nalusize);

					if (parser->stream->h264)
						fwrite(statcode, 4, 1, parser->stream->h264);

					memcpy(video_buffer + video_len, statcode, 4);
					video_len += 4;

					if (flv_read_tag_body(parser->stream, parser->stream->h264_buffer, uint32_be_to_uint32(nalusize)) < uint32_be_to_uint32(nalusize)) {
						return ERROR_INVALID_TAG;
					}

					if (parser->stream->h264)
						fwrite(parser->stream->h264_buffer, uint32_be_to_uint32(nalusize), 1, parser->stream->h264);

					memcpy(video_buffer + video_len, parser->stream->h264_buffer, uint32_be_to_uint32(nalusize));
					video_len += uint32_be_to_uint32(nalusize);
				}

				if (parser->stream->current_tag_body_length != 0)
					printf("2222222222: %d\n", parser->stream->current_tag_body_length);

				//segment TS  last io timestamp
				parser->stream->hlsconfig.hls_end_ts = flv_tag_get_timestamp(*tag);

				if (flv_video_tag_frame_type(vt) == FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME){
					parser->stream->hlsconfig.key_frame_count++;

					if (parser->stream->hlsconfig.key_frame_count &&
						(parser->stream->hlsconfig.key_frame_count-1) % parser->stream->hlsconfig.hls_segment_num == 0)
					{
						parser->stream->hlsconfig.ts_fragment_id = parser->stream->hlsconfig.key_frame_count;
						parser->stream->hlsconfig.ts_count++;
						hls_segment(parser);
						parser->stream->hlsconfig.key_frame_current = parser->stream->hlsconfig.key_frame_count;
						parser->stream->hlsconfig.hls_count++;
					}
				}

				parser->hlsmodule->ngx_rtmp_hls_video_ex(video_buffer, video_len, flv_tag_get_timestamp(*tag), flv_video_tag_frame_type(vt) == FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME);



			}
		}
	}

	return OK;
}

static void adtsHeaderAnalysis(uint8_t *pBuffer, uint32_t length)
{
	STRU_ADTS_HEADER  m_sAACInfo;

	if (((pBuffer[0] == 0xFF) && ((pBuffer[1] & 0xF1) == 0xF0)) | // 代表有crc校验，所以adts长度为9
		((pBuffer[0] == 0xFF) && ((pBuffer[1] & 0xF1) == 0xF1)))  // 代表没有crc校验，所以adts长度为7
	{
		m_sAACInfo.syncword = ((uint16_t)pBuffer[0] & 0xFFF0) >> 4;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:syncword  %d\n", m_sAACInfo.syncword);

		m_sAACInfo.id = ((uint16_t)pBuffer[1] & 0x8) >> 3;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:id  %d\n", m_sAACInfo.id);

		m_sAACInfo.layer = ((uint16_t)pBuffer[1] & 0x6) >> 1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:layer  %d\n", m_sAACInfo.layer);

		m_sAACInfo.protection_absent = (uint16_t)pBuffer[1] & 0x1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:protection_absent  %d\n", m_sAACInfo.protection_absent);

		m_sAACInfo.profile = (((uint16_t)pBuffer[2] & 0xc0) >> 6) + 1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:profile  %d\n", m_sAACInfo.profile);

		m_sAACInfo.sampling_frequency_index = ((uint16_t)pBuffer[2] & 0x3c) >> 2;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:sampling_frequency_index  %d\n", m_sAACInfo.sampling_frequency_index);

		m_sAACInfo.private_bit = ((uint16_t)pBuffer[2] & 0x2) >> 1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:pritvate_bit  %d\n", m_sAACInfo.private_bit);

		m_sAACInfo.channel_configuration = ((((uint16_t)pBuffer[2] & 0x1) << 2) | (((uint16_t)pBuffer[3] & 0xc0) >> 6));
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:channel_configuration  %d\n", m_sAACInfo.channel_configuration);

		m_sAACInfo.original_copy = ((uint16_t)pBuffer[3] & 0x30) >> 5;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:original_copy  %d\n", m_sAACInfo.original_copy);

		m_sAACInfo.home = ((uint16_t)pBuffer[3] & 0x10) >> 4;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:home  %d\n", m_sAACInfo.home);

		//		m_sAACInfo.emphasis = ((unsigned short)pBuffer[3] & 0xc) >> 2;                      
		//fprintf(stderr, "m_sAACInfo:emphasis  %d\n", m_sAACInfo.emphasis);
		m_sAACInfo.copyright_identification_bit = ((uint16_t)pBuffer[3] & 0x2) >> 1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() pAdm_sAACInfots_header:copyright_identification_bit  %d\n", m_sAACInfo.copyright_identification_bit);

		m_sAACInfo.copyright_identification_start = (uint16_t)pBuffer[3] & 0x1;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:copyright_identification_start  %d\n", m_sAACInfo.copyright_identification_start);

		m_sAACInfo.aac_frame_length = ((((uint16_t)pBuffer[4]) << 5) | (((uint16_t)pBuffer[5] & 0xf8) >> 3));
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:aac_frame_length  %d\n", m_sAACInfo.aac_frame_length);

		m_sAACInfo.adts_buffer_fullness = (((uint16_t)pBuffer[5] & 0x7) | ((uint16_t)pBuffer[6]));
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:adts_buffer_fullness  %d\n", m_sAACInfo.adts_buffer_fullness);

		m_sAACInfo.no_raw_data_blocks_in_frame = ((uint16_t)pBuffer[7] & 0xc0) >> 6;
		printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:no_raw_data_blocks_in_frame  %d\n", m_sAACInfo.no_raw_data_blocks_in_frame);

		// The protection_absent bit indicates whether or not the header contains the two extra bytes
		if (m_sAACInfo.protection_absent == 0)
		{
			m_sAACInfo.crc_check = ((((uint16_t)pBuffer[7] & 0x3c) << 10) | (((uint16_t)pBuffer[8]) << 2) | (((uint16_t)pBuffer[9] & 0xc0) >> 6));
			printf("AudioCodecInfo::AdtsHeaderAnalysis() m_sAACInfo:crc_check  %d\n", m_sAACInfo.crc_check);
		}
	}
}

static void adts_header_generate(uint8_t *buf, int size, int pce_size, int channel, int profile, int samplerate_index)
{
	int touchsize = size + 7;

	//copy from nginx-rtmp-module 'ngx_rtmp_hls_module.c'  line:1787
	buf[0] = 0xff;
	buf[1] = 0xf1;
	buf[2] = (uint8_t)(((profile - 1) << 6)
		| (samplerate_index << 2)
		| ((channel & 0x04) >> 2));

	buf[3] = (uint8_t)(((channel & 0x03) << 6) | ((touchsize >> 11) & 0x03));
	buf[4] = (uint8_t)(touchsize >> 3);
	buf[5] = (uint8_t)((touchsize << 5) | 0x1f);
	buf[6] = 0xfc;
}

static int hls_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {

	if (parser->stream->flag_audioconfig)
		return OK;

    printf("* Sound type: %s\n", dump_string_get_sound_type(at));
    printf("* Sound size: %s\n", dump_string_get_sound_size(at));
    printf("* Sound rate: %s\n", dump_string_get_sound_rate(at));
    printf("* Sound format: %s\n", dump_string_get_sound_format(at));

    /* if AAC, detect packet type */
    if (flv_audio_tag_sound_format(at) == FLV_AUDIO_TAG_SOUND_FORMAT_AAC) {
        flv_aac_packet_type type;

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_aac_packet_type)) < sizeof(flv_aac_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        printf("* AAC packet type: %s\n", dump_string_get_aac_packet_type(type));

		if (type == FLV_AAC_PACKET_TYPE_SEQUENCE_HEADER){

			flv_aac_packet_type conf[2];
			if (flv_read_tag_body(parser->stream, &conf, sizeof(conf)) < sizeof(conf)) {
				return ERROR_INVALID_TAG;
			}

			uint8_t samplerate_index = 0;
			samplerate_index |= ((conf[1] >> 7) & 0x01);
			samplerate_index |= ((conf[0] << 1) & 0x0E);

			parser->stream->audioconfig.samplerate_index = samplerate_index;
			parser->stream->audioconfig.channels = uint8_t((conf[1] >> 3) & 0x0f);
			parser->stream->audioconfig.profile = uint8_t((conf[0] >> 3) & 0x1f);

			parser->stream->flag_audioconfig = true;

			//uint8_t p[7] = { 0 };
			//int size = 155;
			//adts_header_generate(p, size, 0, parser->stream->audioconfig.channels, parser->stream->audioconfig.profile,
			//	parser->stream->audioconfig.samplerate_index);

			//for (size_t i = 0; i < 7; i++)
			//	printf("0x%X\n", p[i]);

			//adtsHeaderAnalysis(p, 7);
		}
    }

    return OK;
}

static int hls_on_audio_tag_ex(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {

	/* if AAC, detect packet type */
	if (flv_audio_tag_sound_format(at) == FLV_AUDIO_TAG_SOUND_FORMAT_AAC) {
		flv_aac_packet_type type;

		/* packet type */
		if (flv_read_tag_body(parser->stream, &type, sizeof(flv_aac_packet_type)) < sizeof(flv_aac_packet_type)) {
			return ERROR_INVALID_TAG;
		}

		if (type == FLV_AAC_PACKET_TYPE_SEQUENCE_HEADER){

			parser->stream->aac_header_count++;

			//if (parser->stream->aac == NULL){
			//	std::string aac_name = num2str(parser->stream->aac_header_count) + ".aac";
			//	parser->stream->aac = fopen(aac_name.c_str(), "wb");
			//}
			//else{
			//	fclose(parser->stream->aac);

			//	std::string aac_name = num2str(parser->stream->aac_header_count) + ".aac";
			//	parser->stream->aac = fopen(aac_name.c_str(), "wb");
			//}
		}
		else{
			/*get first pkt ts*/
			if (!parser->stream->hlsconfig.flag_first_ts){
				parser->stream->hlsconfig.first_ts = flv_tag_get_timestamp(*tag);
				parser->stream->hlsconfig.hls_start_ts = parser->stream->hlsconfig.first_ts;
				printf("first pkt ts:%u, audio\n", parser->stream->hlsconfig.first_ts);

				parser->stream->hlsconfig.hls_segment_num = 6;
				hls_segment(parser);

				parser->stream->hlsconfig.flag_first_ts = true;
			}

			//if (parser->stream->aac)
			{

				if (parser->stream->aac_buffer == NULL){

					parser->stream->aac_buf_max = uint24_be_to_uint32(tag->body_length);
					parser->stream->aac_buffer = (uint8_t*)malloc(sizeof(uint8_t)*parser->stream->aac_buf_max);
				}
				else
				{
					if (parser->stream->aac_buf_max < uint24_be_to_uint32(tag->body_length))
					{
						parser->stream->aac_buf_max = uint24_be_to_uint32(tag->body_length);
						parser->stream->aac_buffer = (uint8_t*)realloc(parser->stream->aac_buffer, parser->stream->aac_buf_max);
					}
				}

				static uint8_t audio_buffer[1024 * 1024];
				uint32_t audio_len = 0;

				uint8_t adts_header[7] = { 0 };

				adts_header_generate(adts_header, uint24_be_to_uint32(tag->body_length) - 2, 0,
					parser->stream->audioconfig.channels, parser->stream->audioconfig.profile, parser->stream->audioconfig.samplerate_index);

				if (parser->stream->aac)
					fwrite(adts_header, 7, 1, parser->stream->aac);

				memcpy(audio_buffer + audio_len, adts_header, 7);
				audio_len += 7;

				if (flv_read_tag_body(parser->stream, parser->stream->aac_buffer, uint24_be_to_uint32(tag->body_length) - 2) < uint24_be_to_uint32(tag->body_length) - 2) {
					return ERROR_INVALID_TAG;
				}

				if (parser->stream->aac)
					fwrite(parser->stream->aac_buffer, uint24_be_to_uint32(tag->body_length) - 2, 1, parser->stream->aac);

				memcpy(audio_buffer + audio_len, parser->stream->aac_buffer, uint24_be_to_uint32(tag->body_length) - 2);
				audio_len += (uint24_be_to_uint32(tag->body_length) - 2);

				//segment TS  last io timestamp
				parser->stream->hlsconfig.hls_end_ts = flv_tag_get_timestamp(*tag);

				parser->hlsmodule->ngx_rtmp_hls_audio_ex(audio_buffer, audio_len, flv_tag_get_timestamp(*tag), false);
			}
		}
	}

	return OK;
}

static int hls_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    printf("* Metadata event name: %s\n", amf_string_get_bytes(name));
    printf("* Metadata contents: ");
    //amf_data_dump(stdout, data, 0);
	dump_hls_amf_data(data, parser);
    printf("\n");
    return OK;
}

static int hls_segment_finish(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {

	(void)tag;
	(void)name;
	(void)data;
	//(void)parser;

	hls_segment(parser);

	return OK;
}

static int hls_on_prev_tag_size(uint32 size, flv_parser * parser) {
    printf("Previous tag size: %u\n", size);
    return OK;
}

static int hls_on_stream_end(flv_parser * parser) {
    free(parser->user_data);
    return OK;
}

/* hls FLV file metadata dump callback */
static int hls_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    flvmeta_opts * options = (flvmeta_opts*) parser->user_data;

    if (options->metadata_event == NULL) {
        if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
			dump_hls_amf_data(data, parser);
            return FLVMETA_DUMP_STOP_OK;
        }
    }
    else {
        if (!strcmp((char*)amf_string_get_bytes(name), options->metadata_event)) {
			dump_hls_amf_data(data, parser);
        }
    }
    return OK;
}

/* setup dumping */

void dump_hls_setup_metadata_dump(flv_parser * parser) {
    if (parser != NULL) {
		parser->on_metadata_tag = hls_on_metadata_tag_only;
    }
}

int dump_hls_file(flv_parser * parser, const flvmeta_opts * options) {
	parser->on_header = hls_on_header;
	parser->on_tag = hls_on_tag;
	parser->on_audio_tag = hls_on_audio_tag;
	parser->on_video_tag = hls_on_video_tag;
	parser->on_metadata_tag = hls_on_metadata_tag;
	parser->on_prev_tag_size = hls_on_prev_tag_size;
	parser->on_stream_end = hls_on_stream_end;

	return flv_parse(options->input_file, parser, 0);
}

static void hls_get_av_config(flv_parser * parser, const flvmeta_opts * options) {
	//parser->on_header = hls_on_header;
	//parser->on_tag = hls_on_tag;
	parser->on_audio_tag = hls_on_audio_tag;
	parser->on_video_tag = hls_on_video_tag;
	//parser->on_metadata_tag = hls_on_metadata_tag;
	//parser->on_prev_tag_size = hls_on_prev_tag_size;
	//parser->on_stream_end = hls_on_stream_end;
}


int dump_hls_file_ex(flv_parser * parser, const flvmeta_opts * options) {

	//hls_get_av_config(parser, options);

	parser->on_audio_tag = hls_on_audio_tag;
	parser->on_video_tag = hls_on_video_tag;
	parser->on_metadata_tag = hls_on_metadata_tag;

	return flv_parse_av_config(options->input_file, options->output_file, parser, 0);
}

int fragement_hls_file_ex(flv_parser * parser, const flvmeta_opts * options) {

	//parser->on_tag = hls_on_tag_ex;
	parser->on_audio_tag = hls_on_audio_tag_ex;
	parser->on_video_tag = hls_on_video_tag_ex;
	parser->on_metadata_tag = hls_segment_finish;

	//return flv_get_raw_av(parser, 0);
	file_offset_t offset_start = options->keyframe_start_index > 0 ? parser->stream->keyframePos[options->keyframe_start_index] : 0;
	file_offset_t offset_end = options->keyframe_end_index > 0 ? parser->stream->keyframePos[options->keyframe_end_index] : INT_MAX;
	return flv_get_raw_av(parser, offset_start, offset_end);
}

int dump_hls_amf_data(const amf_data * data, flv_parser * parser) {

	//FILE* fp = fopen("test.m3u8", "wb");

	//fwrite("#EXTM3U\n", strlen("#EXTM3U\n"), 1, fp);
	//fwrite("#EXT-X-VERSION:3\n", strlen("#EXT-X-VERSION:3\n"), 1, fp);
	//fwrite("#EXT-X-TARGETDURATION:12\n", strlen("#EXT-X-TARGETDURATION:12\n"), 1, fp);
	//fwrite("#EXT-X-MEDIA-SEQUENCE:0\n", strlen("#EXT-X-MEDIA-SEQUENCE:0\n"), 1, fp);
	//fwrite("#EXTINF:6.000000,\n", strlen("#EXTINF:6.000000,\n"), 1, fp);
	//fwrite("test-0000.ts\n", strlen("test-0000.ts\n"), 1, fp);
	//fwrite("#EXT-X-ENDLIST", strlen("#EXT-X-ENDLIST"), 1, fp);

	//fclose(fp);

	printf("vector size:%d,%d\n", parser->stream->keyframePos.size(), parser->stream->keyframeTs.size());
	amf_data_dump_hls(parser->stream->keyframePos, parser->stream->keyframeTs, data, 0);
	printf("vector size:%d,%d\n", parser->stream->keyframePos.size(), parser->stream->keyframeTs.size());

	parser->stream->flag_metadata = true;
	//for (int i = 0; i < keyframePos.size(); i++)
	//	printf("index:%d, pos:%d\n", i, (uint32_t)keyframePos[i]);
	//for (int i = 0; i < keyframeTs.size(); i++)
	//	printf("index:%d, time:%lf\n", i, keyframeTs[i]);

	printf("\n");
	return OK;
}
