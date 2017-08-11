#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>

extern int startupJulius();
extern int execJulius(std::string inputFilePath);
extern void shutdownJulius();
extern void segmentAndRefresh();
extern void setBaseHtsVoice(std::string htsVoice);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s BASEHTSVOICE\n", argv[0]);
		return -1;
	}

	setBaseHtsVoice(std::string(argv[1]));

	if (startupJulius() < 0) {
		// startupJulius()でエラー出力済み
		return -1;
	}
	
	DIR* dp = opendir("tmp");
	if (dp == NULL) {
		fprintf(stderr, "Error in open directory\n");
		return -1;
	}
	
	struct dirent* dent;
	do {
		dent = readdir(dp);
		if (dent != NULL && dent->d_name[0] != '.') {
			if (execJulius("tmp/" + std::string(dent->d_name)) < 0) {
				closedir(dp);
				shutdownJulius();
				// execJulius()でエラー出力済み
				return -1;
			}
		}
	} while (dent != NULL);

	closedir(dp);

	shutdownJulius();

	segmentAndRefresh();

	return 0;
}
