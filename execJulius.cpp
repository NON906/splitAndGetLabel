#include <stdio.h>
#include <string>
#include <fstream>
#include <julius/juliuslib.h>
#include <sent/adin.h>

extern void recordAndSplitSetUp(Recog *recog);

static Recog *recog;

int startupJulius()
{
	Jconf *jconf;

	jlog_set_output(NULL);

	char *argv[13];
	argv[0] = new char[64];
	strcpy(argv[0], "julius");
	argv[1] = new char[64];
	strcpy(argv[1], "-C");
	argv[2] = new char[64];
	strcpy(argv[2], "../../dictation-kit-v4.4/main.jconf");
	argv[3] = new char[64];
	strcpy(argv[3], "-C");
	argv[4] = new char[64];
	strcpy(argv[4], "../../dictation-kit-v4.4/am-dnn.jconf");
	argv[5] = new char[64];
	strcpy(argv[5], "-dnnconf");
	argv[6] = new char[64];
	strcpy(argv[6], "../../dictation-kit-v4.4/julius.dnnconf");
	argv[7] = new char[64];
	strcpy(argv[7], "-v");
	argv[8] = new char[64];
	std::ifstream ifs("../../bccwj.60k.pdp_new.htkdic");
	if (ifs.is_open()) {	// ファイルの有無
		strcpy(argv[8], "../../bccwj.60k.pdp_new.htkdic");
	} else {
		strcpy(argv[8], "../../dictation-kit-v4.4/model/lang_m/bccwj.60k.pdp.htkdic");
	}
	argv[9] = new char[64];
	strcpy(argv[9], "-input");
	argv[10] = new char[64];
	strcpy(argv[10], "rawfile");
	argv[11] = new char[64];
	strcpy(argv[11], "-cutsilence");
	argv[12] = new char[64];
	strcpy(argv[12], "-spsegment");

	jconf = j_config_load_args_new(13, argv);
	if (jconf == NULL) {
		fprintf(stderr, "Error in load config\n");
		return -1;
	}

	for (int loop = 0; loop < 13; loop++) {
		delete[] argv[loop];
	}

	recog = j_create_instance_from_jconf(jconf);
	if (recog == NULL) {
		fprintf(stderr, "Error in startup\n");
		return -1;
	}

	recordAndSplitSetUp(recog);

	if (j_adin_init(recog) == FALSE) {
		fprintf(stderr, "Error in j_adin_init\n");
		return -1;
	}

	return 0;
}

void shutdownJulius()
{
	j_recog_free(recog);
}

int execJulius(std::string inputFilePath)
{
	int ret;

	char *copyedInputFilePath = new char[inputFilePath.size() + 1];
 	strcpy(copyedInputFilePath, inputFilePath.c_str());

	switch (j_open_stream(recog, copyedInputFilePath)) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "error in input stream\n");
		return -1;
	case -2:
		fprintf(stderr, "failed to begin input stream\n");
		return -1;
	}

	delete[] copyedInputFilePath;

	ret = j_recognize_stream(recog);
	if (ret == -1) {
		fprintf(stderr, "Error in j_recognize_stream\n");
		return -1;
	}
	
	j_close_stream(recog);

	return 0;
}
