#ifndef __AAC_PARSE_H
#define __AAC_PARSE_H

#include "AudioSourceData.h"
#include <vector>
using namespace std;

class AACParse
{
public:
	AACParse(void);
	~AACParse(void);

private:
	vector<AudioSourceData> mAudios;

	FILE *mFile;

	char* mConfig;
	unsigned int mConfigSize;

	unsigned int mSampleRate;

	char* mTempfile;
	int mAacEncType;

public:
	bool ParseAACFile(const  char* filepath);
	int Get_One_ADTS_Frame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size);
	bool ParseADTS(const  char* filepath, bool bmp4);
	char* GetConfig();
	unsigned int GetConfigLength();
	unsigned int Size();
	unsigned int GetSizeByIndex(unsigned int index);
	bool GetDataByIndex(unsigned int index,char* data,unsigned int &size);
	double GetPTSByIndex(unsigned int index);
	unsigned int GetSampleRate();
	unsigned long long GetFileLength();
	void SetAacEncodeType(int type) {mAacEncType = type;}

private:
	bool analyze_mp4head(FILE *infile,unsigned long long frontlength = 0);
	void Clear();
};


#endif