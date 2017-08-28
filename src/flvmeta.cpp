#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <vector>

#include "flvmeta.h"
#include "dump.h"
#include "cmdline.h"

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

	parser.b_m3u8 = info.m3u8_flag;
	parser.b_ts = info.ts_flag;
	parser.key_ID_start = info.key_ID_start_arg;
	parser.key_ID_end = info.key_ID_end_arg;


	switch (options.command) 
	{
	case FLVMETA_DUMP_COMMAND:
		errcode = dump_metadata(&options);
		break;
	case FLVMETA_FULL_DUMP_COMMAND:{
		errcode = dump_flv_file(&options, &parser);
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