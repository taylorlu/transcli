#ifndef _FILE_MIXER_H_
#define _FILE_MIXER_H_

#include "../flvmuxer/AudioParse.h"
#include "../flvmuxer/H264Parse.h"
#include "../flvmuxer/MetaUnit.h"
#include "../flvmuxer/ComDef.h"
#include "../flvmuxer/Mp4Chunk.h"
#include "../flvmuxer/FileWriter.h"


class FileMixer
{
public:
	FileMixer(int outputType = OUTPUT_TYPE_FLV);
	~FileMixer(void);

private:
	H264Parse mVParser;
	AudioParse* m_pAParser;

	FileWriter mFile;

	//flv
	double mDuration;
	double mBitRate;
	double mFileLength;
	double mMp4Boxpos;
	double mMp4Headerpos;
	double mSlimmp4HeaderSize;
	double mMp4HeadLen;

	vector<unsigned long long> mFrameOffset;
	vector<unsigned char> mFrameType;
	vector<unsigned int> mKeyFrameIndex;
	vector<unsigned short> mVideoDelta;
	double mVideoDuration;
	double mAudioDuration;

	double mLastKeyFrameTimeStamp;
	double mLastKeyFrameLocation;
    vector<vector<double> > mKeyFrames;

	//mp4
	bool mHighVersion;
	vector<Mp4Chunk> mVideoChunk;
	vector<Mp4Chunk> mAudioChunk;

	unsigned long long mVideoStcoPos;
	unsigned long long mAudioStcoPos;

	int mOutputType;

public:
	bool ParseAACFile(const char* file);
	bool ParseAC3File(const char* file);
	//bool ParseAACFiles(vector<char*> files);//,vector<double> videopts);
	bool Parse264File(const char* file);
	bool WriteOutPutFile(const char* file, bool vflag, bool aflag);
	bool InitTempFile(const char* tempfile);

	double GetVideoPtsByFilePos(unsigned long long pos);

private:
	//flv
	bool WriteFileFlv(bool vflag, bool aflag);
	void WriteFlvHead();
	void WriteFlvMeta();
	void WriteFlvData(bool vflag, bool aflag);
	void WriteFlvEnd();

	void WriteFlvVideoTag(unsigned int size,unsigned int timestamp,char* data,bool iskeyframe = false,bool isconfig = false,short dts = 0);
	void WriteFlvAudioTag(unsigned int size,unsigned int timestamp,char* data,bool isconfig = false);

	MetaUnit GetMetaString(const char* strtitle,const char* stringdada);
	MetaUnit GetMetaDouble(const char* strtitle,double doubledata);
	MetaUnit GetArrayData(const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata);

	//mp4
	bool WriteFileMp4();
	void GenerateChunkList();
	void WriteMp4Head();
	void WriteMp4BoxMoov();
	void WriteMp4BoxMvhd();
	void WriteMp4BoxTrak(bool video);
	void WriteMp4BoxTkhd(bool video);
	void WriteMp4BoxMdia(bool video);
	void WriteMp4BoxMdhd(bool video);
	void WriteMp4BoxHdlr(bool video);
	void WriteMp4BoxMinf(bool video);
	void WriteMp4BoxVmhd();
	void WriteMp4BoxSmhd();
	void WriteMp4BoxDinf();
	void WriteMp4BoxDref();
	void WriteMp4BoxStbl(bool video);
	void WriteMp4BoxStsd(bool video);
	void WriteMp4BoxStsdVideo();
	void WriteMp4BoxAvcc();
	void WriteMp4BoxStsdAudio();
	void WriteMp4BoxEsds();
	void WriteAC3ConfigBox(const char* boxType);
	void WriteMp4BoxStts(bool video);
	void WriteMp4BoxStss();
	void WriteMp4BoxCtts();
	void WriteMp4BoxStsc(bool video);
	void WriteMp4BoxStsz(bool video);
	void WriteMp4BoxStco(bool video);
	void WriteMp4BoxMdat();
	void WriteMp4BoxFree();
	void WriteBoxType(const char* type);
	void WriteBoxSize(unsigned long long boxhead);

	unsigned int descrLength(unsigned int len);
	void putDescr(int tag,unsigned int size);
};

#endif
