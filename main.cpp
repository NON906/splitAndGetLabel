#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>

extern int startupJulius();
extern int execJulius(std::string inputFilePath);
extern void shutdownJulius();
extern void segmentAndRefresh();
extern void setBaseHtsVoice(std::string htsVoice);
extern void setApi(int api, std::string key);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [-g API_KEY] BASEHTSVOICE\n", argv[0]);
		return -1;
	}

	for (int argLoop = 1; argLoop < argc; argLoop++) {
		if (argv[argLoop][0] == '-') {
			switch (argv[argLoop][1]) {
				case 'g':
					setApi(1, std::string(argv[argLoop + 1]));
					argLoop++;
					break;
				default:
					fprintf(stderr, "Invalid option: %s\n", argv[argLoop]);
					break;
			}
		}
		else {
			setBaseHtsVoice(std::string(argv[argLoop]));
		}
	}


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
