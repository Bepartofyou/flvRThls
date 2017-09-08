#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <vector>
#include <math.h>

#include "flvmeta.h"
#include "dump.h"
#include "cmdline.h"



static void judge_segment_num(flv_parser& parser, int segment){
	int vecSize = parser.stream->keyframeTs.size();

	//for (size_t i = 0; i < vecSize-1; i++)
	//{
	//	printf("%f\n", parser.stream->keyframeTs[i]);
	//}
	double timespan = parser.stream->keyframeTs[vecSize - 1] - parser.stream->keyframeTs[0] > 1 ? parser.stream->keyframeTs[vecSize - 1] - parser.stream->keyframeTs[0] : 1;
	int segment_nub = ceil((double)segment * (double)vecSize / (double)timespan);

	parser.segment_num = segment_nub > 6 ? 6 : segment_nub;
}

int main(int argc, char ** argv) {

	struct gengetopt_args_info info;
	if (cmdline_parser(argc, argv, &info) != 0) {
		exit(1);
	}

	switch (info.m3u8_flag){
	default:
		break;
	}


	int errcode;

	/* flvmeta default options */
	static flvmeta_opts options;

	
	options.metadata = NULL;
	options.check_level = FLVMETA_CHECK_LEVEL_WARNING;
	options.quiet = 0;
	options.check_report_format = FLVMETA_FORMAT_RAW;
	options.error_handling = FLVMETA_EXIT_ON_ERROR;
	options.verbose = 0;
	options.metadata_event = NULL;
	options.dump_metadata = 0;
	options.insert_onlastsecond = 1;
	options.reset_timestamps = 0;
	options.all_keyframes = 0;
	options.preserve_metadata = 0;


	options.command = FLVMETA_FULL_DUMP_COMMAND;
	//options.command = FLVMETA_DUMP_COMMAND;
	options.input_file = info.flvfile_arg;
	//options.input_file = argv[1];
	options.output_file = info.outpath_arg;
	options.domain = info.domain_arg;
	options.dump_format = FLVMETA_FORMAT_HLS;
	options.keyframe_start_index = info.key_ID_start_arg;
	options.keyframe_end_index = info.key_ID_end_arg;


	flv_parser parser;
	memset(&parser, 0, sizeof(flv_parser));
	parser.hlsmodule = CHlsModule::getInstance();
	parser.hlsmodule->ctx.audio_cc = info.audio_cc_arg;
	parser.hlsmodule->ctx.video_cc = info.video_cc_arg;
	parser.hlsmodule->ctx.aframe_base = info.aframe_base_arg;
	parser.hlsmodule->ctx.aframe_pts = info.aframe_pts_arg;

	parser.seek_flag = 0;
	parser.ts_start_flag = 0;
	parser.b_m3u8 = info.m3u8_flag;
	parser.b_ts = info.ts_flag;
	parser.key_ID_start = info.key_ID_start_arg;
	parser.key_ID_end = info.key_ID_end_arg;
	parser.flag_over = 0;


	switch (options.command) 
	{
	case FLVMETA_DUMP_COMMAND:
		errcode = dump_metadata(&options);
		break;
	case FLVMETA_FULL_DUMP_COMMAND:{
		errcode = dump_flv_file(&options, &parser);
		judge_segment_num(parser, info.segmengttime_arg);

		//over write key_ID_start and key_ID_end
		if (info.ts_start_arg != -1 && info.ts_end_arg != -1)
		{
			parser.key_ID_start = info.ts_start_arg * parser.segment_num;
			parser.key_ID_end =
				(info.ts_end_arg + 1) * parser.segment_num < parser.stream->keyframePos.size() ? (info.ts_end_arg + 1) * parser.segment_num : -1;

			options.keyframe_start_index = parser.key_ID_start;
			options.keyframe_end_index = parser.key_ID_end;
		}

		errcode = fragment_flv_file(&options, &parser);
		int xxx = 0;
		}
		break;
	default:
		errcode = FLV_OK;
		break;
	}

	return errcode;
}