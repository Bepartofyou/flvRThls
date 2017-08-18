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
	options.command = FLVMETA_FULL_DUMP_COMMAND;
	//options.command = FLVMETA_DUMP_COMMAND;
	options.input_file = argv[1];
	options.output_file = NULL;
	options.metadata = NULL;
	options.check_level = FLVMETA_CHECK_LEVEL_WARNING;
	options.quiet = 0;
	options.check_report_format = FLVMETA_FORMAT_RAW;
	options.dump_metadata = 0;
	options.insert_onlastsecond = 1;
	options.reset_timestamps = 0;
	options.all_keyframes = 0;
	options.preserve_metadata = 0;
	options.error_handling = FLVMETA_EXIT_ON_ERROR;
	options.dump_format = FLVMETA_FORMAT_HLS;
	options.verbose = 0;
	options.metadata_event = NULL;

	flv_parser parser;
	memset(&parser, 0, sizeof(flv_parser));
	parser.hlsmodule = CHlsModule::getInstance();

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