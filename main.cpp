#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>

extern int startupJulius();
extern int execJulius(std::string inputFilePath);
extern void shutdownJulius();
extern void segmentAndRefresh();
extern void setApi(int api, std::string key);
extern void addOptions(std::string opt, std::string val);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [-G API_KEY] [OpenJTalkOptions]\n", argv[0]);
		return -1;
	}

	for (int argLoop = 1; argLoop < argc; argLoop++) {
		if (argv[argLoop][0] == '-') {
			switch (argv[argLoop][1]) {
				case 'G':
					setApi(1, std::string(argv[argLoop + 1]));
					argLoop++;
					break;
				default:
					addOptions(std::string(argv[argLoop]), std::string(argv[argLoop + 1]));
					argLoop++;
					break;
			}
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
