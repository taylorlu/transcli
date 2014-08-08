#include "FileMixer.h"
#include "GlobalDefine.h"
#include "mp4headInterface.h"
#include "AACParse.h"
#include "AC3Parse.h"

//#include "xzcompress.h"
#include "crc32.h"
#include <time.h>

FileMixer::FileMixer(int outputType )
{
	mOutputType = outputType;
	m_pAParser = NULL;
}

FileMixer::~FileMixer(void)
{
}

bool FileMixer::ParseAACFile(const char* file )
{
	m_pAParser = new AACParse();
	m_pAParser->SetContainerType(mOutputType);
	return m_pAParser->Parse(file);
}

bool FileMixer::ParseAC3File(const char* file)
{
	m_pAParser = new AC3Parse();
	return m_pAParser->Parse(file);
}

bool FileMixer::Parse264File(const char* file )
{
	bool ret = mVParser.Parse264File(file);
	mVParser.FinishParsing();
	return ret;
}

bool FileMixer::WriteOutPutFile(const char* file)
{
	//unsigned long long t = time(0);
	if (!file) {
		return false;
	}

	mFile.Open(file);
	if (!mFile.IsOpen()) {
		return false;
	}

	if (mVParser.Size() < 1 || m_pAParser->Size() < 1) {
		return false;
	}
	
	mVideoDuration = mVParser.GetPTSByIndex(mVParser.Size() - 1);
	mVideoDuration += mVParser.GetPTSByIndex(1);
	mAudioDuration = m_pAParser->GetPTSByIndex(m_pAParser->Size() - 1);
	mAudioDuration += m_pAParser->GetPTSByIndex(1);

	bool ret = true;

	//============
	//outputtype = OUTPUT_TYPE_MP4;
	//============

	switch(mOutputType)
	{
	case OUTPUT_TYPE_FLV:
		ret = WriteFileFlv();
		break;
	case OUTPUT_TYPE_MP4:
		ret = WriteFileMp4();
		break;
	default:
		ret = false;
	}

	mFile.Close();
	//unsigned int tt = time(0) - t;

	return ret;
}

bool FileMixer::WriteFileFlv()
{
	char* vconfig = mVParser.GetConfig();
	uint8_t* aconfig = m_pAParser->GetConfig();
	if(!vconfig || !aconfig) {
		return false;
	}

	//flv head
	WriteFlvHead();
	//meta
	mKeyFrames.push_back(mVParser.mKeyFrameLocation);
	mKeyFrames.push_back(mVParser.mKeyFrameTimes);
	WriteFlvMeta();

	//video config
	WriteFlvVideoTag(mVParser.GetConfigLength(),0,vconfig,true,true);
	delete[] vconfig;
	
	//audio config
	WriteFlvAudioTag(m_pAParser->GetConfigLength(),0,(char*)aconfig,true);
	delete[] aconfig;

	//audio & video data
	WriteFlvData();

	mFileLength = mFile.Tell();
	mBitRate = mFileLength / mDuration / 1000 * 8; 
	WriteFlvMeta();

	return !mFile.Error();
}

void FileMixer::WriteFlvHead()
{
	mFile.WriteBytes("FLV",3);
	mFile.Write8(1);
	mFile.Write8(5);
	mFile.Write32(9);
	mFile.Write32(0);
}

void FileMixer::WriteFlvMeta()
{
	vector<MetaUnit> metaunits;
	vector<char*> keyframesubtitle;

	keyframesubtitle.push_back("filepositions");
	keyframesubtitle.push_back("times");

	metaunits.push_back(GetMetaString("MetaCreator","PPLive"));
	metaunits.push_back(GetMetaDouble("duration",mDuration));
	metaunits.push_back(GetMetaDouble("width",mVParser.mWidth));
	metaunits.push_back(GetMetaDouble("height",mVParser.mHeight));
	metaunits.push_back(GetMetaDouble("bitrate",mBitRate));
	metaunits.push_back(GetMetaDouble("filelength",mFileLength));
	metaunits.push_back(GetMetaDouble("lastkeyframetimestamp",mLastKeyFrameTimeStamp));
	metaunits.push_back(GetMetaDouble("lastkeyframelocation",mLastKeyFrameLocation));
	metaunits.push_back(GetArrayData("keyframes",keyframesubtitle,mKeyFrames));
	
	mFile.Seek(9 + 4,SEEK_SET);
	mFile.Write8(18);
	mFile.Write24(0);
	mFile.Write32(0);
	mFile.Write24(0);

	mFile.Write8(2);
	mFile.Write16(10);
	mFile.WriteBytes("onMetaData",10);
	mFile.Write8(8);
	mFile.Write32(metaunits.size());
	for (int i = 0; i < metaunits.size(); i ++)
	{
		metaunits[i].WriteMetaUnit(mFile.Pointer());
	}
	mFile.Write24(9);
	unsigned long long pos = mFile.Tell();
	mFile.Write32(pos - 13);
	mFile.Seek(14,SEEK_SET);
	mFile.Write24(pos - 24);
	mFile.Seek(0,SEEK_END);
}

MetaUnit FileMixer::GetMetaString( const char* strtitle,const char* stringdada )
{
	MetaUnit metaunit;
	metaunit.GetStringData(strtitle,stringdada);
	return metaunit;
}

MetaUnit FileMixer::GetMetaDouble( const char* strtitle,double doubledata )
{
	MetaUnit metaunit;
	metaunit.GetDoubleData(strtitle,doubledata);
	return metaunit;
}

MetaUnit FileMixer::GetArrayData( const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata )
{
	MetaUnit metaunit;
	metaunit.GetArrayData(strtitle,subtitles,arraydata);
	return metaunit;
}

void FileMixer::WriteFlvVideoTag( unsigned int size,unsigned int timestamp,char* data,bool iskeyframe /*= false*/,bool isconfig /*= false*/,short dts /*= 0*/ )
{
	mFile.Write8(9);
	mFile.Write24(size + 5);
	mFile.Write24((timestamp & 0xffffff));
	mFile.Write8((timestamp >> 24) & 0xff);
	mFile.Write24(0);
	
	if (iskeyframe)
	{
		mFile.Write8(0x17);
	}
	else
	{
		mFile.Write8(0x27);
	}
	
	if (isconfig)
	{
		mFile.Write8(0);
	}
	else
	{
		mFile.Write8(1);
	}
	mFile.Write24(dts);
	mFile.WriteBytes(data,size);
	mFile.Write32(size + 16);
}

void FileMixer::WriteFlvAudioTag(unsigned int size,unsigned int timestamp,char* data,bool isconfig /*= false*/ )
{
	mFile.Write8(8);
	mFile.Write24(size + 2);
	mFile.Write24((timestamp & 0xffffff));
	mFile.Write8((timestamp >> 24) & 0xff);
	mFile.Write24(0);
	mFile.Write8(0xaf);
	if (isconfig)
	{
		mFile.Write8(0);
	}
	else
	{
		mFile.Write8(1);
	}
	mFile.WriteBytes(data,size);
	mFile.Write32(size + 13);
}

void FileMixer::WriteFlvData()
{
	unsigned int videoindex = 0;
	unsigned int audioindex = 0;

	unsigned int videopts = 0;
	unsigned int audiopts = 0;

	unsigned int keyframeindex = 0;

	char *data = new char[1024 * 1024];

	while(videoindex < mVParser.Size() || audioindex < m_pAParser->Size())
	{
		if (videoindex >= mVParser.Size())//video over write audio
		{
			unsigned int size = 1024 * 1024;
			bool ret = m_pAParser->GetDataByIndex(audioindex,data,size);
			if (ret)
			{
				mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 13);
				mFrameType.push_back(1);
				WriteFlvAudioTag(size,audiopts,data);
			}
			audioindex ++;
			audiopts = m_pAParser->GetPTSByIndex(audioindex);
		}
		else
		{
			if (audioindex >= m_pAParser->Size())//audio over write video
			{
				unsigned int size = 1024 * 1024;
				bool ret = mVParser.GetDataByIndex(videoindex,data,size);
				bool iskeyframe = mVParser.IsKeyFrame(videoindex);
				if (iskeyframe)
				{
					mKeyFrames[0][keyframeindex] = mFile.Tell();
					mKeyFrames[1][keyframeindex ++] = videopts / 1000.;
					mKeyFrameIndex.push_back(videoindex + 1);
				}
				if (ret)
				{
					mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 16);
					mFrameType.push_back(0);
					mVideoDelta.push_back(0);
					WriteFlvVideoTag(size,videopts,data,iskeyframe);
				}
				videoindex ++;
				videopts = mVParser.GetPTSByIndex(videoindex);
			}
			else
			{
				if (audiopts > videopts)//write video
				{
					unsigned int size = 1024 * 1024;
					bool ret = mVParser.GetDataByIndex(videoindex,data,size);
					bool iskeyframe = mVParser.IsKeyFrame(videoindex);
					if (iskeyframe)
					{
						mKeyFrames[0][keyframeindex] = mFile.Tell();
						mKeyFrames[1][keyframeindex ++] = videopts / 1000.;
						mKeyFrameIndex.push_back(videoindex + 1);
					}
					if (ret)
					{
						mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 16);
						mFrameType.push_back(0);
						mVideoDelta.push_back(0);
						WriteFlvVideoTag(size,videopts,data,iskeyframe);
					}
					videoindex ++;
					videopts = mVParser.GetPTSByIndex(videoindex);
				}
				else//write audio
				{
					unsigned int size = 1024 * 1024;
					bool ret = m_pAParser->GetDataByIndex(audioindex,data,size);
					if (ret)
					{
						mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 13);
						mFrameType.push_back(1);
						WriteFlvAudioTag(size,audiopts,data);
					}
					audioindex ++;
					audiopts = m_pAParser->GetPTSByIndex(audioindex);
				}
			}
		}
	}

	delete[] data;

	mVideoDuration = videopts;
	mAudioDuration = audiopts;
	mDuration = mVideoDuration > mAudioDuration ? mVideoDuration : mAudioDuration;
	mDuration /= 1000;
	if (mKeyFrames[0].size())
	{
		mLastKeyFrameLocation = mKeyFrames[0][mKeyFrames[0].size() - 1];
	}
	else
	{
		mLastKeyFrameLocation = 0;
	}
	if (mKeyFrames[1].size())
	{
		mLastKeyFrameTimeStamp = mKeyFrames[1][mKeyFrames[1].size() - 1];
	}
	else
	{
		mLastKeyFrameTimeStamp = 0;
	}
}

bool FileMixer::InitTempFile(const char* tempfile )
{
	return mVParser.Init(tempfile);
}

double FileMixer::GetVideoPtsByFilePos( unsigned long long pos )
{
	return mVParser.GetPtsByFilePos(pos);
}

bool FileMixer::WriteFileMp4()
{
	if (mVParser.Size() < 40 || m_pAParser->Size() < 40)
	{
		return false;
	}
	GenerateChunkList();
	//ftyp
	WriteMp4Head();

	//moov
	WriteMp4BoxMoov();

	//mdat
	WriteMp4BoxMdat();

	//free
	WriteMp4BoxFree();

	//rewrite stco
	mFile.Seek(mVideoStcoPos,SEEK_SET);
	WriteMp4BoxStco(true);
	mFile.Seek(mAudioStcoPos,SEEK_SET);
	WriteMp4BoxStco(false);

	if (mFile.Error())
	{
		return false;
	}

	return true;
}

void FileMixer::GenerateChunkList()
{
	unsigned int videocount = mVParser.Size();
	unsigned int audiocount = m_pAParser->Size();

	Mp4Chunk videofirstchunk;
	videofirstchunk.mIndex = 1;
	videofirstchunk.mSamples = 9;
	mVideoChunk.push_back(videofirstchunk);

	videocount -= 9;
	int chunkindex = 2;
	while(videocount)
	{
		unsigned int samples = 7;
		if (videocount < 7)
		{
			samples = videocount;
		}
		Mp4Chunk videochunk;
		videochunk.mIndex = chunkindex ++;
		videochunk.mSamples = samples;
		mVideoChunk.push_back(videochunk);

		videocount -= samples;
	}

	Mp4Chunk audiofirstchunk;
	audiofirstchunk.mIndex = 1;
	audiofirstchunk.mSamples = 9;
	mAudioChunk.push_back(audiofirstchunk);

	audiocount -= 9;
	chunkindex = 2;
	while(audiocount)
	{
		unsigned int samples = 7;
		if (audiocount < 7)
		{
			samples = audiocount;
		}
		Mp4Chunk audiochunk;
		audiochunk.mIndex = chunkindex ++;
		audiochunk.mSamples = samples;
		mAudioChunk.push_back(audiochunk);

		audiocount -= samples;
	}
}

void FileMixer::WriteMp4Head()
{
	unsigned long long boxhead = mFile.Tell();

	mFile.Write32(0);
	WriteBoxType("ftyp");
	WriteBoxType("isom");

	mFile.Write32(1);
	WriteBoxType("isom");
	WriteBoxType("avc1");

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxMoov()
{
	unsigned long long boxhead = mFile.Tell();

	mFile.Write32(0);
	WriteBoxType("moov");
	WriteMp4BoxMvhd();

	WriteMp4BoxTrak(true);
	WriteMp4BoxTrak(false);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxMvhd()
{
	int maxTrackID = 1;
	unsigned long long maxTrackLenTemp,maxTrackLen = 0;

	maxTrackLen = mAudioDuration > mVideoDuration ? mAudioDuration : mVideoDuration;
	maxTrackLen = maxTrackLen * 600 / 1000;
	mHighVersion = maxTrackLen < 0xffffff ? false : true;
	if (mHighVersion)
	{
		mFile.Write32(120);
	}
	else
	{
		mFile.Write32(108);
	}
	WriteBoxType("mvhd");
	if (mHighVersion)
	{
		mFile.Write8(1);
	}
	else
	{
		mFile.Write8(0);
	}
	mFile.Write24(0);

	mFile.Write32( time(0)); /* creation time */
	mFile.Write32( time(0)); /* modification time */

	mFile.Write32( 600);
	if (mHighVersion)/* duration of longest track */
	{
		mFile.Write64( maxTrackLen);
	}
	else
	{
		mFile.Write32( maxTrackLen); 
	}

	mFile.Write32( 0x00010000); /* reserved (preferred rate) 1.0 = normal */
	mFile.Write16( 0x0100); /* reserved (preferred volume) 1.0 = normal */
	mFile.Write16( 0); /* reserved */
	mFile.Write32( 0); /* reserved */
	mFile.Write32( 0); /* reserved */

	/* Matrix structure */
	mFile.Write32( 0x00010000); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x00010000); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x40000000); /* reserved */

	mFile.Write32( 0); /* reserved (preview time) */
	mFile.Write32( 0); /* reserved (preview duration) */
	mFile.Write32( 0); /* reserved (poster time) */
	mFile.Write32( 0); /* reserved (selection time) */
	mFile.Write32( 0); /* reserved (selection duration) */
	mFile.Write32( 0); /* reserved (current time) */
	maxTrackID = 2;
	mFile.Write32( maxTrackID+1); /* Next track id */
}

void FileMixer::WriteMp4BoxTrak( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("trak");
	WriteMp4BoxTkhd(video);
	WriteMp4BoxMdia(video);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxTkhd(bool video)
{
	if (mHighVersion)
	{
		mFile.Write32(104);
	}
	else
	{
		mFile.Write32(92);
	}
	WriteBoxType("tkhd");
	if (mHighVersion)
	{
		mFile.Write8(1);
	}
	else
	{
		mFile.Write8(0);
	}
	mFile.Write24(1);
	if (mHighVersion)
	{
		mFile.Write64(time(0));
		mFile.Write64(time(0));
	}
	else
	{
		mFile.Write32(time(0));
		mFile.Write32(time(0));
	}
	if (video)
	{
		mFile.Write32(1);
	}
	else
	{
		mFile.Write32(2);
	}
	mFile.Write32(0);
	unsigned long long duration = 0;
	if (video)
	{
		duration = mVideoDuration / 1000 * 600;
	}
	else
	{
		duration = mAudioDuration / 1000 * 600;
	}
	if (mHighVersion)
	{
		mFile.Write64(duration);
	}
	else
	{
		mFile.Write32(duration);
	}
	mFile.Write32(0);
	mFile.Write32(0);
	mFile.Write32(0);
	if (video)
	{
		mFile.Write16(0);
	}
	else
	{
		mFile.Write16(0x0100);
	}
	mFile.Write16(0);

	/* Matrix structure */
	mFile.Write32( 0x00010000); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x00010000); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x0); /* reserved */
	mFile.Write32( 0x40000000); /* reserved */

	if (video)
	{
		mFile.Write32(mVParser.mWidth * 0x10000);
		mFile.Write32(mVParser.mHeight * 0x10000);
	}
	else
	{
		mFile.Write32(0);
		mFile.Write32(0);
	}
}

void FileMixer::WriteMp4BoxMdia( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("mdia");
	WriteMp4BoxMdhd(video);
	WriteMp4BoxHdlr(video);
	WriteMp4BoxMinf(video);

	WriteBoxSize(boxhead);
}


void FileMixer::WriteMp4BoxMdhd( bool video )
{
	if (mHighVersion)
	{
		mFile.Write64(44);
	}
	else
	{
		mFile.Write32(32);
	}
	WriteBoxType("mdhd");
	mFile.Write8(mHighVersion ? 1 : 0);
	mFile.Write24(0);

	if (mHighVersion) {
		mFile.Write64(time(0));
		mFile.Write64(time(0));
	} else {
		mFile.Write32(time(0));
		mFile.Write32(time(0));
	}
	if (video) {
		mFile.Write32(mVParser.GetFrameRate() * 1000);
	} else {
		mFile.Write32(m_pAParser->GetSampleRate());
	}
	if (mHighVersion) {
		if(video) {
			mFile.Write64(mVParser.Size() * 1000);
		} else {
			mFile.Write64(m_pAParser->Size() * m_pAParser->GetFrameLength());	// 1024
		}
	} else {
		if (video) {
			mFile.Write32(mVParser.Size() * 1000);
		} else {
			mFile.Write32(m_pAParser->Size() * m_pAParser->GetFrameLength());
		}
	}
	mFile.Write16(21956);
	mFile.Write16(0);
}

void FileMixer::WriteMp4BoxHdlr( bool video )
{
	unsigned long long boxhead = mFile.Tell();

	mFile.Write32(0);
	WriteBoxType("hdlr");
	mFile.Write32(0);
	mFile.Write32(0);
	if (video)
	{
		WriteBoxType("vide");
	}
	else
	{
		WriteBoxType("soun");
	}
	mFile.Write32(0);
	mFile.Write32(0);
	mFile.Write32(0);
	if (video)
	{
		mFile.WriteBytes("VideoHandler",12);
	}
	else
	{
		mFile.WriteBytes("AudioHandler",12);
	}
	mFile.Write8(0);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxMinf( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("minf");

	if (video)
	{
		WriteMp4BoxVmhd();
	}
	else
	{
		WriteMp4BoxSmhd();
	}

	WriteMp4BoxDinf();
	WriteMp4BoxStbl(video);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxVmhd()
{
	mFile.Write32(20);
	WriteBoxType("vmhd");
	mFile.Write32(1);
	mFile.Write64(0);
}

void FileMixer::WriteMp4BoxSmhd()
{
	mFile.Write32(16);
	WriteBoxType("smhd");
	mFile.Write32(0);
	mFile.Write16(0);
	mFile.Write16(0);
}

void FileMixer::WriteMp4BoxDinf()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("dinf");
	WriteMp4BoxDref();

	WriteBoxSize(boxhead);
}


void FileMixer::WriteMp4BoxDref()
{
	mFile.Write32(28);
	WriteBoxType("dref");
	mFile.Write32(0);
	mFile.Write32(1);

	mFile.Write32(0xc);
	WriteBoxType("url ");
	mFile.Write32(1);
}

void FileMixer::WriteMp4BoxStbl( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stbl");
	WriteMp4BoxStsd(video);
	WriteMp4BoxStts(video);
	if (video)
	{
		WriteMp4BoxStss();
		WriteMp4BoxCtts();
	}
	WriteMp4BoxStsc(video);
	WriteMp4BoxStsz(video);
	WriteMp4BoxStco(video);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStsd( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stsd");
	mFile.Write32(0);
	mFile.Write32(1);
	if (video)
	{
		WriteMp4BoxStsdVideo();
	}
	else
	{
		WriteMp4BoxStsdAudio();
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStsdVideo()
{
	unsigned long long boxhead = mFile.Tell();
	char compressor_name[32];
	mFile.Write32(0);
	WriteBoxType("avc1");
	mFile.Write32(0);
	mFile.Write16(0);
	mFile.Write16(1);

	mFile.Write16( 0); /* Codec stream version */
	mFile.Write16( 0); /* Codec stream revision (=0) */

	mFile.Write32( 0); /* Reserved */
	mFile.Write32( 0); /* Reserved */
	mFile.Write32( 0); /* Reserved */

	mFile.Write16( mVParser.mWidth); /* Video width */
	mFile.Write16( mVParser.mHeight); /* Video height */
	mFile.Write32( 0x00480000); /* Horizontal resolution 72dpi */
	mFile.Write32( 0x00480000); /* Vertical resolution 72dpi */
	mFile.Write32( 0); /* Data size (= 0) */
	mFile.Write16( 1); /* Frame count (= 1) */

	memset(compressor_name,0,32);

	mFile.Write8( strlen(compressor_name));
	mFile.WriteBytes(compressor_name,31);

	mFile.Write16( 0x18); /* Reserved */
	mFile.Write16( 0xffff); /* Reserved */

	WriteMp4BoxAvcc();

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxAvcc()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("avcC");
	char* config = mVParser.GetConfig();
	mFile.WriteBytes(config,mVParser.GetConfigLength());
	delete[] config;

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStsdAudio()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType(m_pAParser->GetAudioType());

	mFile.Write32( 0); /* Reserved */
	mFile.Write16( 0); /* Reserved */
	mFile.Write16( 1); /* Data-reference index, XXX  == 1 */

	mFile.Write16( 0); /* Version */
	mFile.Write16( 0); /* Revision level */
	mFile.Write32( 0); /* Reserved */

	/* SoundDescription */
	mFile.Write16(m_pAParser->GetChannelCount());
	mFile.Write16( 16);
	mFile.Write16( 0);


	mFile.Write16( 0); /* packet size (= 0) */
	mFile.Write16( m_pAParser->GetSampleRate()); /* Time scale */
	mFile.Write16( 0); /* Reserved */

	uint8_t audioOTI = m_pAParser->GetObjectTypeIndication();
	if(audioOTI == AUDIO_OTI_MP4A) {
		WriteMp4BoxEsds();
	} else {	// AC3 or EC3
		const char* dtype = "dac3";
		if(audioOTI == AUDIO_OTI_EC3) {
			dtype = "dec3";
		}
		WriteAC3ConfigBox(dtype);
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteAC3ConfigBox(const char* boxType)
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType(boxType);
	
	// AC3 config
	uint8_t *config = m_pAParser->GetConfig();
	mFile.WriteBytes((const char*)config, m_pAParser->GetConfigLength());

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxEsds()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("esds");
	mFile.Write32(0);

	uint32_t configLen = m_pAParser->GetConfigLength();
	// ES descriptor
	putDescr( 0x03, 3 + descrLength(13 + configLen) + descrLength(1));
	//putDescr( 0x03, 28/*3+5+13+configLen+5+1*/);
	mFile.Write16(2);
	mFile.Write8(0);

	// DecoderConfig descriptor
	putDescr(0x04, 13 + configLen);

	// Object type indication
	mFile.Write8(m_pAParser->GetObjectTypeIndication());

	mFile.Write8(0x15);

	// Buffer size DB
	//mFile.Write24(375);
	mFile.Write8(mVParser.mKeyFrameLocation.size() >> (3 + 16));
	mFile.Write16((mVParser.mKeyFrameLocation.size() >> 3) & 0xffff);

	// Max bitrate & average bitrate
	mFile.Write32(0);
	uint32_t averageBr = m_pAParser->GetFileLength()*8*1000/mAudioDuration;	// kbps
	mFile.Write32(averageBr);

	//config
	putDescr(0x05, configLen);
	uint8_t *config = m_pAParser->GetConfig();
	mFile.WriteBytes((const char*)config, configLen);
	
	//SL descriptor
	putDescr(0x06, 1);
	mFile.Write8(0x02);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStts( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stts");
	mFile.Write32(0);
	mFile.Write32(1);
	if (video)
	{
		mFile.Write32(mVParser.Size());
		mFile.Write32(1000);
	}
	else
	{
		mFile.Write32(m_pAParser->Size());
		mFile.Write32(m_pAParser->GetFrameLength());
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStss()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stss");
	mFile.Write32(0);

	unsigned int keyframesize = mVParser.mKeyFrameIndex.size();

	mFile.Write32(keyframesize);
	for (int i = 0; i < keyframesize; i ++)
	{
		mFile.Write32(mVParser.mKeyFrameIndex[i]);
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxCtts()
{

}

void FileMixer::WriteMp4BoxStsc( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stsc");
	mFile.Write32(0);

	vector<Mp4Chunk> stsc;
	unsigned int samples = 0;
	if (video)
	{
		for (int i = 0; i < mVideoChunk.size(); i ++)
		{
			if (samples != mVideoChunk[i].mSamples)
			{
				stsc.push_back(mVideoChunk[i]);
				samples = mVideoChunk[i].mSamples;
			}
		}
	}
	else
	{
		for (int i = 0; i < mAudioChunk.size(); i ++)
		{
			if (samples != mAudioChunk[i].mSamples)
			{
				stsc.push_back(mAudioChunk[i]);
				samples = mAudioChunk[i].mSamples;
			}
		}
	}

	mFile.Write32(stsc.size());
	for (int i = 0; i < stsc.size(); i ++)
	{
		mFile.Write32(stsc[i].mIndex);
		mFile.Write32(stsc[i].mSamples);
		mFile.Write32(1);
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStsz( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("stsz");
	mFile.Write32(0);

	mFile.Write32(0);//sample size
	unsigned int samplecount = 0;
	if (video)
	{
		samplecount = mVParser.Size();
		mFile.Write32(samplecount);
		for (int i = 0; i < samplecount; i ++)
		{
			mFile.Write32(mVParser.GetSizeByIndex(i));
		}
	}
	else
	{
		samplecount = m_pAParser->Size();
		mFile.Write32(samplecount);
		for (int i = 0; i < samplecount; i ++)
		{
			mFile.Write32(m_pAParser->GetSizeByIndex(i));
		}
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxStco( bool video )
{
	unsigned long long boxhead = mFile.Tell();
	if (video)
	{
		mVideoStcoPos = boxhead;
	}
	else
	{
		mAudioStcoPos = boxhead;
	}
	unsigned long long length = mVParser.GetFileLength() + m_pAParser->GetFileLength() + 10 * 1024 * 1024;
	bool mode64 = false;
	if (length > (unsigned long long)(0xffffffff))
	{
		mode64 = true;
	}
	mFile.Write32(0);
	if (mode64)
	{
		WriteBoxType("co64");
	}
	else
	{
		WriteBoxType("stco");
	}
	mFile.Write32(0);
	if (video)
	{
		mFile.Write32(mVideoChunk.size());
		for (int i = 0; i < mVideoChunk.size(); i ++)
		{
			if (mode64)
			{
				mFile.Write64(mVideoChunk[i].mOffset);
			}
			else
			{
				mFile.Write32(mVideoChunk[i].mOffset);
			}
		}
	}
	else
	{
		mFile.Write32(mAudioChunk.size());
		for (int i = 0; i < mAudioChunk.size(); i ++)
		{
			if (mode64)
			{
				mFile.Write64(mAudioChunk[i].mOffset);
			}
			else
			{
				mFile.Write32(mAudioChunk[i].mOffset);
			}
		}
	}

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxMdat()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("mdat");

	char *data = new char[1024 * 1024];

	unsigned int videoindex = 0;
	for (int i = 0; i < mVideoChunk.size(); i ++)
	{
		mVideoChunk[i].mOffset = mFile.Tell();
		for (int j = 0; j < mVideoChunk[i].mSamples; j ++)
		{
			unsigned int size = 1024 * 1024;
			bool ret = mVParser.GetDataByIndex(videoindex ++,data,size);
			if (ret)
			{
				mFile.WriteBytes(data,size);
			}
		}
	}

	unsigned int audioindex = 0;
	for (int i = 0; i < mAudioChunk.size(); i ++)
	{
		mAudioChunk[i].mOffset = mFile.Tell();
		for (int j = 0; j < mAudioChunk[i].mSamples; j ++)
		{
			unsigned int size = 1024 * 1024;
			bool ret = m_pAParser->GetDataByIndex(audioindex ++,data,size);
			if (ret)
			{
				mFile.WriteBytes(data,size);
			}
		}
	}

	delete[] data;

	WriteBoxSize(boxhead);
}

void FileMixer::WriteMp4BoxFree()
{
	unsigned long long boxhead = mFile.Tell();
	mFile.Write32(0);
	WriteBoxType("free");

	mFile.WriteBytes("File Producer:PPTV",18);

	WriteBoxSize(boxhead);
}

void FileMixer::WriteBoxType(const char* type )
{
	mFile.WriteBytes(type,4);
}

void FileMixer::WriteBoxSize( unsigned long long boxhead )
{
	unsigned long long boxtail = mFile.Tell();
	mFile.Seek(boxhead,SEEK_SET);
	mFile.Write32(boxtail - boxhead);
	mFile.Seek(boxtail,SEEK_SET);
}

unsigned int FileMixer::descrLength( unsigned int len )
{
	int i;
	for(i=1; len>>(7*i); i++);
	return len + 1 + i;
}

void FileMixer::putDescr( int tag,unsigned int size )
{
	uint32_t writeLen = descrLength(size);
	int i = (int)(writeLen - size - 2);
	/*int i=1;
	for(; size >> (7*i); ++i);
	i -= 1;*/
	mFile.Write8(tag);
	for(; i>0; i--)
		mFile.Write8((size>>(7*i)) | 0x80);
	mFile.Write8(size & 0x7F);
}
