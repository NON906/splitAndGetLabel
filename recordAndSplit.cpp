#include <list>
#include <string>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include <julius/juliuslib.h>

#define API_GOOGLE 1

typedef struct {
	char *speech;
	int speechNum;
} StructSpeech;

static std::list<StructSpeech> speechs;
static int fileCount = 1;
static std::string fileCountAlphabet = "a";

static int apiMode = 0;
static std::string apiKey;

static std::string options = "";

void segmentAndRefresh()
{
	char originalPath[MAXLINELEN];
	if (getcwd(originalPath, MAXLINELEN) == NULL) {
		return;
	}
	char command[MAXLINELEN];
	snprintf(command, MAXLINELEN, "cd ../../segment_adapt_windows-v1.0/ && perl segment_adapt.pl && cd \"%s\"", originalPath);
	system(command);

	for (int no = 1; no < fileCount; no++) {
		char fromStr[MAXLINELEN];
		char toStr[MAXLINELEN];

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/data/raw/a%02d.raw", no);
		snprintf(toStr, MAXLINELEN, "../../HTS-demo_NIT-ATR503-M001/data/raw/nitech_jp_atr503_m001_%s%02d.raw", fileCountAlphabet.c_str(), no);
		rename(fromStr, toStr);

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/data/mono/a%02d.lab", no);
		snprintf(toStr, MAXLINELEN, "../../HTS-demo_NIT-ATR503-M001/data/labels/mono/nitech_jp_atr503_m001_%s%02d.lab", fileCountAlphabet.c_str(), no);
		rename(fromStr, toStr);

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/data/full/a%02d.lab", no);
		snprintf(toStr, MAXLINELEN, "../../HTS-demo_NIT-ATR503-M001/data/labels/full/nitech_jp_atr503_m001_%s%02d.lab", fileCountAlphabet.c_str(), no);
		rename(fromStr, toStr);

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/a%02d.raw", no);
		remove(fromStr);

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/labels/mono/a%02d.lab", no);
		remove(fromStr);

		snprintf(fromStr, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/labels/full/a%02d.lab", no);
		remove(fromStr);
	}

	int alphabetSize = fileCountAlphabet.size();
		
	fileCountAlphabet[alphabetSize - 1]++;

	bool allZ = true;
	for (int charNo = alphabetSize - 1; charNo >= 0; charNo--) {
		if (fileCountAlphabet[charNo] > 'z') {
			fileCountAlphabet[charNo] = 'a';
			if (charNo > 0) {
				fileCountAlphabet[charNo - 1]++;
			}
		}
		else {
			allZ = false;
			break;
		}
	}

	if (allZ) {
		fileCountAlphabet.clear();
		for (int charNo = 0; charNo < alphabetSize + 1; charNo++) {
			fileCountAlphabet += "a";
		}
	}

	fileCount = 1;
}

static void recordSampleAdd(Recog *recog, SP16 *speech, int sampleNum, void *dummy)
{
	StructSpeech structSpeech;

	char *newSpeech = new char[sampleNum * 2];
	for (int loop = 0; loop < sampleNum; loop++) {
		char *speechBytes = (char *)(&(speech[loop]));
		newSpeech[loop * 2] = speechBytes[1];
		newSpeech[loop * 2 + 1] = speechBytes[0];
	}
	structSpeech.speech = newSpeech;

	structSpeech.speechNum = sampleNum * 2;

	speechs.push_back(structSpeech);
}

static void recordSampleSegment(Recog *recog, void *dummy)
{
	StructSpeech structSpeech;
	structSpeech.speech = NULL;
	speechs.push_back(structSpeech);
}

static void recordSampleResult(Recog *recog, void *dummy)
{
	int allSpeechNum = 0;
	for (std::list<StructSpeech>::iterator it = speechs.begin(); it != speechs.end() && it->speech != NULL; it++) {
		allSpeechNum += it->speechNum;
	}
	if (allSpeechNum <= 0) {
		while (!speechs.empty()) {
			speechs.pop_front();
		}
		return;
	}
	char *allSpeech = new char[allSpeechNum];

	char *allSpeechPos = allSpeech;
	while (!speechs.empty()) {
		StructSpeech front = speechs.front();
		if (front.speech == NULL) {
			speechs.pop_front();
			break;
		}
		memcpy(allSpeechPos, front.speech, sizeof(char) * front.speechNum);
		allSpeechPos += front.speechNum;
		delete[] front.speech;
		speechs.pop_front();
	}

	char rawFilePath[MAXLINELEN];
	snprintf(rawFilePath, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/a%02d.raw", fileCount);

	FILE *rawFp = fopen(rawFilePath, "wb");
	if (rawFp == NULL) {
		return;
	}

	if (fwrite(allSpeech, sizeof(char), allSpeechNum, rawFp) < allSpeechNum) {
		fclose(rawFp);
		remove(rawFilePath);
		return;
	}

	fclose(rawFp);

	std::string sentStr = "";

	switch (apiMode) {
		case API_GOOGLE:
			{
				remove("tmp.json.result");
				remove("tmp.result");
				remove("tmp.json");
				remove("tmp.base64");
				remove("tmp.flac");
				remove("tmp.wav");

				char apiCommand[MAXLINELEN];
				snprintf(apiCommand, MAXLINELEN, "sox -r 16k -e signed-integer -b 16 -B \"%s\" --bits 16 --channels 1 --rate 16000 tmp.wav", rawFilePath);
				system(apiCommand);
				system("flac -V tmp.wav 2> /dev/null");
				system("base64 tmp.flac > tmp.base64");

				struct stat stbuf;
				int fd = open("tmp.base64", O_RDONLY);
				if (fstat(fd, &stbuf) == -1) {
					remove("tmp.base64");
					remove("tmp.flac");
					remove("tmp.wav");
					break;
				}
				int fileSize = stbuf.st_size;
				close(fd);
				char *base64Data = new char[fileSize];
				FILE *base64Fp = fopen("tmp.base64", "r");
				if (base64Fp == NULL) {
					remove("tmp.base64");
					remove("tmp.flac");
					remove("tmp.wav");
					break;
				}
				char *base64Pos = base64Data;
				while (fgets(base64Pos, fileSize, base64Fp) != NULL) {
					base64Pos += strlen(base64Pos) - 1;
					base64Pos[0] = '\0';
				}
				fclose(base64Fp);

				FILE *apiFp = fopen("tmp.json", "w");
				if (apiFp == NULL) {
					remove("tmp.base64");
					remove("tmp.flac");
					remove("tmp.wav");
					break;
				}
				fputs("{\n", apiFp);
				fputs("    \"config\": {\n", apiFp);
				fputs("        \"encoding\": \"FLAC\",\n", apiFp);
				fputs("        \"sampleRateHertz\": 16000,\n", apiFp);
				fputs("        \"languageCode\": \"ja-JP\"\n", apiFp);
				fputs("    },\n", apiFp);
				fputs("    \"audio\": {\n", apiFp);
				fputs("        \"content\": \"", apiFp);
				fputs(base64Data, apiFp);
				fputs("\"\n", apiFp);
				fputs("    }\n", apiFp);
				fputs("}\n", apiFp);
				fclose(apiFp);

				delete[] base64Data;

				snprintf(apiCommand, MAXLINELEN, "curl -s -X POST -H \"Content-Type: application/json\" --data-binary @tmp.json \"https://speech.googleapis.com/v1/speech:recognize?key=%s\" > tmp.json.result", apiKey.c_str());
				system(apiCommand);
				system("cat tmp.json.result | jq -r \".results[0].alternatives[0].transcript\" > tmp.result");
 
				fd = open("tmp.result", O_RDONLY);
				if (fstat(fd, &stbuf) == -1) {
					remove("tmp.json.result");
					remove("tmp.result");
					remove("tmp.json");
					remove("tmp.base64");
					remove("tmp.flac");
					remove("tmp.wav");
					break;
				}
				fileSize = stbuf.st_size;
				close(fd);
				char *resultData = new char[fileSize];
				FILE *resultFp = fopen("tmp.result", "r");
				if (resultFp == NULL) {
					remove("tmp.json.result");
					remove("tmp.result");
					remove("tmp.json");
					remove("tmp.base64");
					remove("tmp.flac");
					remove("tmp.wav");
					break;
				}
				fgets(resultData, fileSize, resultFp);
				fclose(resultFp);

				sentStr = std::string(resultData);
				if (sentStr == "null") {
					sentStr = "";
				}

				delete[] resultData;

				remove("tmp.json.result");
				remove("tmp.result");
				remove("tmp.json");
				remove("tmp.base64");
				remove("tmp.flac");
				remove("tmp.wav");
			}
			break;
		default:
			for (RecogProcess *r = recog->process_list; r; r = r->next) {
	
				if (!r->live) {
					continue;
				}

				if (r->result.status < 0) {
					continue;
				}

				WORD_INFO *winfo = r->lm->winfo;
				for (int n = 0; n < r->result.sentnum; n++) {
					Sentence *s = &(r->result.sent[n]);
					WORD_ID *seq = s->word;
					int seqnum = s->word_num;
		
					for (int i = 0; i < seqnum; i++) {
						sentStr += winfo->woutput[seq[i]];
					}
				}
			}
			break;
	}

	if (sentStr == "") {
		remove(rawFilePath);
		return;
	}

	system(("echo \"" + sentStr + "\" | ../../open_jtalk/bin/open_jtalk -x ../../open_jtalk/dic/ " + options + " -ot tmp.txt 2> /dev/null").c_str());

	FILE *logFp = fopen("tmp.txt", "r");
	if (logFp == NULL) {
		remove(rawFilePath);
		return;
	}

	char log[MAXLINELEN];

	do {
		if (fgets(log, MAXLINELEN, logFp) == NULL) {
			fclose(logFp);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}
	} while (strcmp(log, "[Output label]\n") != 0);

	char monoFilePath[MAXLINELEN];
	snprintf(monoFilePath, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/labels/mono/a%02d.lab", fileCount);

	FILE *monoFp = fopen(monoFilePath, "w");
	if (monoFp == NULL) {
		fclose(logFp);
		remove("tmp.txt");
		remove(rawFilePath);
		return;
	}

	char fullFilePath[MAXLINELEN];
	snprintf(fullFilePath, MAXLINELEN, "../../segment_adapt_windows-v1.0/akihiro/labels/full/a%02d.lab", fileCount);

	FILE *fullFp = fopen(fullFilePath, "w");
	if (fullFp == NULL) {
		fclose(monoFp);
		fclose(logFp);
		remove(monoFilePath);
		remove("tmp.txt");
		remove(rawFilePath);
		return;
	}

	for (;;) {
		if (fgets(log, MAXLINELEN, logFp) == NULL) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}

		if (log[0] == '\n') {
			break;
		}

		if (fputs(log, fullFp) == EOF) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}

		char *startTimeStr = strtok(log, " ");
		if (startTimeStr == NULL) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}

		char *endTimeStr = strtok(NULL, " ");
		if (endTimeStr == NULL) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}

		char *phoneAllStr = strtok(NULL, " ");
		if (phoneAllStr == NULL) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}
		char *phoneStr = phoneAllStr;
		while (phoneStr[0] != '-') {
			phoneStr++;
		}
		phoneStr++;
		char *phoneEnd = phoneStr;
		while (phoneEnd[0] != '+') {
			phoneEnd++;
		}
		phoneEnd[0] = '\0';

		if (fprintf(monoFp, "%s %s %s\n", startTimeStr, endTimeStr, phoneStr) < 0) {
			fclose(fullFp);
			fclose(monoFp);
			fclose(logFp);
			remove(fullFilePath);
			remove(monoFilePath);
			remove("tmp.txt");
			remove(rawFilePath);
			return;
		}
	}

	fclose(fullFp);
	fclose(monoFp);
	fclose(logFp);
	remove("tmp.txt");

	fileCount++;
	if (fileCount >= 100) {
		segmentAndRefresh();
	}

	delete[] allSpeech;
}

void recordAndSplitSetUp(Recog *recog)
{
	callback_add_adin(recog, CALLBACK_ADIN_TRIGGERED, recordSampleAdd, NULL);
	callback_add(recog, CALLBACK_EVENT_SEGMENT_BEGIN, recordSampleSegment, NULL);
	callback_add(recog, CALLBACK_RESULT, recordSampleResult, NULL);
}

void setApi(int api, std::string key)
{
	apiMode = api;
	apiKey = key;
}

void addOptions(std::string opt, std::string val)
{
	options += " " + opt + " " + val + " ";
}
