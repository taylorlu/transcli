#include "bitconfig.h"

#if defined (HAVE_LIBX265)
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sstream>

#ifdef _WIN32
//#pragma comment(lib,"libx265.lib")
//#pragma comment(lib,"assembly.lib")
#else
#define SetConsoleTitle(t)
#endif

#include "MEvaluater.h"
#include "StreamOutput.h"
#include "CX265Encode.h"
#include "vfhead.h"
#include "logger.h"
#include "yuvUtil.h"

CX265Encode::CX265Encode(const char* outFileName):CVideoEncoder(outFileName)
{
	m_logModuleType = LOGM_TS_VE_H265;
	m_pX265Ctx = NULL;
	//m_pNal = NULL;
	m_planeSize = 0;
#ifdef DUMP_YUV
	dump_output = 0;
#endif
}

CX265Encode::~CX265Encode()
{
	Stop();
}

bool CX265Encode::Initialize()
{
#ifdef DUMP_YUV
	dump_output = getStreamOutput(CStreamOutput::FILE_STREAM);
	dump_output->Open("e:\\test.yuv");
#endif
	InitWaterMark();
	//Init file stream
	if(m_encodePass == 1) {
		m_pOutStream = getStreamOutput(CStreamOutput::FILE_STREAM);
		if(!m_pOutStream->Open(m_strOutFile.c_str())) {
			logger_err(m_logModuleType, "Open Out file error.\n");
			return false;
		}
	}

	std::ostringstream x265Cmd;
	m_frameCount = 0;
	m_planeSize = m_vInfo.res_out.width*m_vInfo.res_out.height;
	int presetId = m_pXmlPrefs->GetInt("videoenc.x265.preset");
	int tuneId = m_pXmlPrefs->GetInt("videoenc.x265.tune");
	
	if(presetId > 0 && tuneId > 0) {
		x265_param_default_preset( &m_x265Param, x265_preset_names[presetId], x265_tune_names[tuneId]);
	} else {
		x265_param_default(&m_x265Param);
	}

	m_x265Param.sourceWidth = m_vInfo.res_out.width;
	m_x265Param.sourceHeight = m_vInfo.res_out.height;
	x265Cmd << " --input-res " << m_x265Param.sourceWidth << 'x' << m_x265Param.sourceHeight; 
	x265Cmd << " --input-depth 8";
	m_x265Param.internalCsp = X265_CSP_I420;
	m_x265Param.internalBitDepth = 8;

	if(m_vInfo.fps_in.den > 0) {
		m_x265Param.fpsNum = m_vInfo.fps_in.num;
		m_x265Param.fpsDenom = m_vInfo.fps_in.den;
	} else {
		m_x265Param.fpsNum = 25;
		m_x265Param.fpsDenom = 1;
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.levelIdc")) {
		int levelIdc = m_pXmlPrefs->GetInt("videoenc.x265.levelIdc");
		if(levelIdc == -1 || (levelIdc >= 10 && levelIdc <= 51)) {
			m_x265Param.levelIdc = levelIdc;
		}
	}

	/*
	if(m_vInfo.dest_par.den > 0) {
		m_x265Param.vui.bEnableAspectRatioIdc = 1;
		m_x265Param.vui.aspectRatioIdc = X265_EXTENDED_SAR;
		m_x265Param.vui.sarWidth = m_vInfo.dest_par.num;
		m_x265Param.vui.sarHeight = m_vInfo.dest_par.den;
	}*/

	x265Cmd << " --fps " << (float)m_vInfo.fps_in.num/m_vInfo.fps_in.den;

	// Rate control
	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) {
		m_vInfo.bitrate = 600;
	}
	
	float qp = (100 - m_pXmlPrefs->GetInt("overall.video.quality")) / 2.f;

	int rateMode = m_pXmlPrefs->GetInt("overall.video.mode");
	switch (rateMode) {
		case RC_MODE_ABR:
		case RC_MODE_2PASS:
			m_x265Param.rc.bitrate = m_vInfo.bitrate;
			m_x265Param.rc.rateControlMode = X265_RC_ABR; 
			x265Cmd << " --bitrate " << m_vInfo.bitrate;
    		break;
		case RC_MODE_VBR:
			if(qp > 0) {
				m_x265Param.rc.qp = (int)qp;
			}
			m_x265Param.rc.rateControlMode = X265_RC_CQP;
			x265Cmd << " -q " << m_x265Param.rc.qp;
			break;
	}
	
	if(m_pXmlPrefs->ExistKey("videoenc.x265.threads")) {
		int threadNum = m_pXmlPrefs->GetInt("videoenc.x265.threads");
		if(threadNum > 0) m_x265Param.poolNumThreads = threadNum;
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.frameThreads")) {
		int frameThreads = m_pXmlPrefs->GetInt("videoenc.x265.frameThreads");
		if(frameThreads > 1) {
			m_x265Param.frameNumThreads = frameThreads;
			x265Cmd << " -F " << frameThreads;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.wpp")) {
		bool bEnableWpp = m_pXmlPrefs->GetBoolean("videoenc.x265.wpp");
		if(bEnableWpp) {
			m_x265Param.bEnableWavefront = 1;
			x265Cmd << " --wpp";
		} else {
			m_x265Param.bEnableWavefront = 0;
			x265Cmd << " --no-wpp";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.ctu")) {
		int ctuNum = m_pXmlPrefs->GetInt("videoenc.x265.ctu");
		if(ctuNum > 0) {
			m_x265Param.maxCUSize = (uint32_t)ctuNum;
			x265Cmd << " --ctu " << ctuNum;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.ctuIntra")) {
		int ctuIntra = m_pXmlPrefs->GetInt("videoenc.x265.ctuIntra");
		if(ctuIntra > 0) {
			m_x265Param.tuQTMaxIntraDepth = (uint32_t)ctuIntra;
			x265Cmd << " --tu-intra-depth " << ctuIntra;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.ctuInter")) {
		int ctuInter = m_pXmlPrefs->GetInt("videoenc.x265.ctuInter");
		if(ctuInter > 0) {
			m_x265Param.tuQTMaxInterDepth = (uint32_t)ctuInter;
			x265Cmd << " --tu-inter-depth " << ctuInter;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.bIntra")) {
		bool bIntra = m_pXmlPrefs->GetBoolean("videoenc.x265.bIntra");
		if(bIntra) {
			m_x265Param.bIntraInBFrames = 1;
			x265Cmd << " --b-intra ";
		} else {
			m_x265Param.bIntraInBFrames = 0;
		}
	}

	//ME search method (DIA, HEX, UMH, STAR, FULL)
	if(m_pXmlPrefs->ExistKey("videoenc.x265.me")) {
		int me = m_pXmlPrefs->GetInt("videoenc.x265.me");
		if(me > 0) {
			m_x265Param.searchMethod = me;
			x265Cmd << " --me " << me;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.subme")) {
		int subme = m_pXmlPrefs->GetInt("videoenc.x265.subme");
		if(subme >= 0 && subme <= X265_MAX_SUBPEL_LEVEL) {
			m_x265Param.subpelRefine = subme;
			x265Cmd << " --subme " << subme;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.merange")) {
		int merange = m_pXmlPrefs->GetInt("videoenc.x265.merange");
		if(merange > 0) {
			m_x265Param.searchRange = merange;
			x265Cmd << " --merange " << merange;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.rect")) {
		bool bEnableRect = m_pXmlPrefs->GetBoolean("videoenc.x265.rect");
		if(bEnableRect) {
			m_x265Param.bEnableRectInter = 1;
			x265Cmd << " --rect";
		} else {
			m_x265Param.bEnableRectInter = 0;
			x265Cmd << " --no-rect";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.amp")) {
		bool bEnableAmp = m_pXmlPrefs->GetBoolean("videoenc.x265.amp");
		if(bEnableAmp) {
			m_x265Param.bEnableAMP = 1;
			x265Cmd << " --amp";
		} else {
			m_x265Param.bEnableAMP = 0;
			x265Cmd << " --no-amp";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.lossless")) {
		bool bEnableLossless = m_pXmlPrefs->GetBoolean("videoenc.x265.lossless");
		if(bEnableLossless) {
			m_x265Param.bLossless = 1;
			x265Cmd << " --lossless";
		} else {
			m_x265Param.bLossless = 0;
			x265Cmd << " --no-lossless";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.cuLossless")) {
		bool bCuLossless = m_pXmlPrefs->GetBoolean("videoenc.x265.cuLossless");
		if(bCuLossless) {
			m_x265Param.bCULossless = 1;
			x265Cmd << " --cu-lossless";
		} else {
			m_x265Param.bCULossless = 0;
			x265Cmd << " --no-cu-lossless";
		}
	}
	
	if(m_pXmlPrefs->ExistKey("videoenc.x265.maxMerge")) {
		int maxMerge = m_pXmlPrefs->GetInt("videoenc.x265.maxMerge");
		if(maxMerge > 0 && maxMerge <= 5) {
			m_x265Param.maxNumMergeCand = (uint32_t)maxMerge;
			x265Cmd << " --max-merge " << maxMerge;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.earlySkip")) {
		bool bEnableEarlySkip = m_pXmlPrefs->GetBoolean("videoenc.x265.earlySkip");
		if(bEnableEarlySkip) {
			m_x265Param.bEnableEarlySkip = 1;
			x265Cmd << " --early-skip";
		} else {
			m_x265Param.bEnableEarlySkip = 0;
			x265Cmd << " --no-early-skip";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.fastCbf")) {
		bool bEnableFastCbf = m_pXmlPrefs->GetBoolean("videoenc.x265.fastCbf");
		if(bEnableFastCbf) {
			m_x265Param.bEnableCbfFastMode = 1;
			x265Cmd << " --fast-cbf";
		} else {
			m_x265Param.bEnableCbfFastMode = 0;
			x265Cmd << " --no-fast-cbf";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.rdPenalty")) {
		int rdPenalty = m_pXmlPrefs->GetInt("videoenc.x265.rdPenalty");
		if(rdPenalty >= 0 && rdPenalty <= 2) {
			m_x265Param.rdPenalty = rdPenalty;
			x265Cmd << " rdpenalty " << rdPenalty;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.tskip")) {
		bool bEnableTransformSkip = m_pXmlPrefs->GetBoolean("videoenc.x265.tskip");
		if(bEnableTransformSkip) {
			m_x265Param.bEnableTransformSkip = 1;
			x265Cmd << " --tskip";
		} else {
			m_x265Param.bEnableTransformSkip = 0;
			x265Cmd << " --no-tskip";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.tskipFast")) {
		bool bEnableTransformSkipFast = m_pXmlPrefs->GetBoolean("videoenc.x265.tskipFast");
		if(bEnableTransformSkipFast) {
			m_x265Param.bEnableTSkipFast = 1;
			x265Cmd << " --tskip-fast";
		} else {
			m_x265Param.bEnableTSkipFast = 0;
			x265Cmd << " --no-tskip-fast";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.openGop")) {
		bool bEnableOpenGOP = m_pXmlPrefs->GetBoolean("videoenc.x265.openGop");
		if(bEnableOpenGOP) {
			m_x265Param.bOpenGOP = 1;
		} else {
			m_x265Param.bOpenGOP = 0;
		}
	}
	
	if(m_pXmlPrefs->ExistKey("videoenc.x265.keyint")) {
		int keyint = m_pXmlPrefs->GetInt("videoenc.x265.keyint");
		if(keyint > 0) {
			if(keyint < 10) {
				m_x265Param.keyframeMax = keyint*m_x265Param.fpsNum/m_x265Param.fpsDenom;
			} else {
				m_x265Param.keyframeMax = keyint;
			}
			x265Cmd << " --keyint " << keyint;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.lookahead")) {
		int lookahead = m_pXmlPrefs->GetInt("videoenc.x265.lookahead");
		if(lookahead > 0) {
			m_x265Param.lookaheadDepth = lookahead;
			x265Cmd << " --rc-lookahead " << lookahead;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.bframes")) {
		int bframes = m_pXmlPrefs->GetInt("videoenc.x265.bframes");
		if(bframes >= 0) {
			m_x265Param.bframes = bframes;
			x265Cmd << " --bframes " << bframes;
		}
	}

	//0 - none, 1 - fast, 2 - full (trellis)
	if(m_pXmlPrefs->ExistKey("videoenc.x265.badapt")) {
		int badapt = m_pXmlPrefs->GetInt("videoenc.x265.badapt");
		if(badapt >= 0 && badapt <= 2) {
			m_x265Param.bFrameAdaptive = badapt;
			x265Cmd << " --b-adapt " << badapt;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.reframes")) {
		int reframes = m_pXmlPrefs->GetInt("videoenc.x265.reframes");
		if(reframes > 0) {
			m_x265Param.maxNumReferences = reframes;
			x265Cmd << " --ref " << reframes;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.qcomp")) {
		int qcomp = m_pXmlPrefs->GetInt("videoenc.x265.qcomp");
		if(qcomp >= 0 && qcomp <= 100) {
			float fqcomp = qcomp / 100.f;
			m_x265Param.rc.qCompress = fqcomp;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.weightp")) {
		bool bEnableWeightp = m_pXmlPrefs->GetBoolean("videoenc.x265.weightp");
		if(bEnableWeightp) {
			m_x265Param.bEnableWeightedPred = 1;
			x265Cmd << " --weightp";
		} else {
			m_x265Param.bEnableWeightedPred = 0;
			x265Cmd << " --no-weightp";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.weightb")) {
		bool bEnableWeightb = m_pXmlPrefs->GetBoolean("videoenc.x265.weightb");
		if(bEnableWeightb) {
			m_x265Param.bEnableWeightedBiPred = 1;
		} else {
			m_x265Param.bEnableWeightedBiPred = 0;
		}
	}
	
	if(m_pXmlPrefs->ExistKey("videoenc.x265.bpyramid")) {
		bool bEnableBpyramid = m_pXmlPrefs->GetBoolean("videoenc.x265.bpyramid");
		if(bEnableBpyramid) {
			m_x265Param.bBPyramid = 1;
		} else {
			m_x265Param.bBPyramid = 0;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.cbqpoffs")) {
		int cbqpoffs = m_pXmlPrefs->GetInt("videoenc.x265.cbqpoffs");
		if(cbqpoffs > 0) m_x265Param.cbQpOffset = cbqpoffs;
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.crqpoffs")) {
		int crqpoffs = m_pXmlPrefs->GetInt("videoenc.x265.crqpoffs");
		if(crqpoffs > 0) m_x265Param.crQpOffset = crqpoffs;
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.rdLevel")) {
		int rdLevel = m_pXmlPrefs->GetInt("videoenc.x265.rdLevel");
		if(rdLevel >= 0 && rdLevel <= 6) {
			m_x265Param.rdLevel = rdLevel;
			x265Cmd << " --rd " << rdLevel;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.psyRd")) {
		float psyrd = m_pXmlPrefs->GetFloat("videoenc.x265.psyRd");
		if(psyrd > -0.00001 && psyrd <= 2.f) {
			m_x265Param.psyRd = psyrd;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.signHide")) {
		bool bEnableSignHide = m_pXmlPrefs->GetBoolean("videoenc.x265.signHide");
		if(bEnableSignHide) {
			m_x265Param.bEnableSignHiding = 1;
		} else {
			m_x265Param.bEnableSignHiding = 0;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.loopFilter")) {
		bool bEnableLoopFilter = m_pXmlPrefs->GetBoolean("videoenc.x265.loopFilter");
		if(bEnableLoopFilter) {
			m_x265Param.bEnableLoopFilter = 1;
		} else {
			m_x265Param.bEnableLoopFilter = 0;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.sao")) {
		bool bEnableSao = m_pXmlPrefs->GetBoolean("videoenc.x265.sao");
		if(bEnableSao) {
			m_x265Param.bEnableSAO = 1;
			x265Cmd << " --sao";
		} else {
			m_x265Param.bEnableSAO = 0;
			x265Cmd << " --no-sao";
		}
	}

	/*if(m_pXmlPrefs->ExistKey("videoenc.x265.saoLcuBounds")) {
		int saoLcuBounds = m_pXmlPrefs->GetInt("videoenc.x265.saoLcuBounds");
		if(saoLcuBounds > 0) {
			m_x265Param.saoLcuBoundary = saoLcuBounds;
			x265Cmd << " --sao-lcu-bounds " << saoLcuBounds;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.saoLcuOpt")) {
		int saoLcuOpt = m_pXmlPrefs->GetInt("videoenc.x265.saoLcuOpt");
		if(saoLcuOpt > 0) {
			m_x265Param.saoLcuBasedOptimization = saoLcuOpt;
			x265Cmd << " --sao-lcu-opt " << saoLcuOpt;
		}
	}*/

	if(m_pXmlPrefs->ExistKey("videoenc.x265.aqMode")) {
		int aqMode = m_pXmlPrefs->GetInt("videoenc.x265.aqMode");
		if(aqMode >= 0 && aqMode <= 2) {
			m_x265Param.rc.aqMode = aqMode;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.aqStrength")) {
		float aqStrength = m_pXmlPrefs->GetFloat("videoenc.x265.aqStrength");
		if(aqStrength > 0 && aqStrength < 3) {
			m_x265Param.rc.aqStrength = aqStrength;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.ssim")) {
		bool bEnableSsim = m_pXmlPrefs->GetBoolean("videoenc.x265.ssim");
		if(bEnableSsim) {
			m_x265Param.bEnableSsim = 1;
			x265Cmd << " --ssim";
		} else {
			m_x265Param.bEnableSsim = 0;
			x265Cmd << " --no-ssim";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.psnr")) {
		bool bEnablePsnr = m_pXmlPrefs->GetBoolean("videoenc.x265.psnr");
		if(bEnablePsnr) {
			m_x265Param.bEnablePsnr = 1;
			x265Cmd << " --psnr";
		} else {
			m_x265Param.bEnablePsnr = 0;
			x265Cmd << " --no-psnr";
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.hash")) {
		int hash = m_pXmlPrefs->GetInt("videoenc.x265.hash");
		if(hash > 0) {
			m_x265Param.decodedPictureHashSEI = hash;
			x265Cmd << " --hash " << hash;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.vbvMaxrate")) {
		int maxrate = m_pXmlPrefs->GetInt("videoenc.x265.vbvMaxrate");
		if(maxrate > 0) {
			m_x265Param.rc.vbvMaxBitrate = maxrate;
			x265Cmd << " --vbv-maxrate " << maxrate;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.vbvBufferSize")) {
		int vbvBufSize = m_pXmlPrefs->GetInt("videoenc.x265.vbvBufferSize");
		if(vbvBufSize > 0) {
			m_x265Param.rc.vbvBufferSize = vbvBufSize;
			x265Cmd << " --vbv-bufsize " << vbvBufSize;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.vbvBufferInit")) {
		int vbvBufInit = m_pXmlPrefs->GetInt("videoenc.x265.vbvBufferInit");
		if(vbvBufInit > 0) {
			m_x265Param.rc.vbvBufferInit = vbvBufInit;
			x265Cmd << " --vbv-init " << vbvBufInit;
		}
	}

	if(m_pXmlPrefs->ExistKey("videoenc.x265.cuTree")) {
		bool bEnableCUTree = m_pXmlPrefs->GetBoolean("videoenc.x265.cuTree");
		if(bEnableCUTree) {
			m_x265Param.rc.cuTree = 1;
			x265Cmd << " --cutree";
		} else {
			m_x265Param.rc.cuTree = 0;
			x265Cmd << " --no-cutree";
		}
	}

	// 2Pass control
	if(m_bMultiPass) {
		if(m_encodePass == 2) {
			m_x265Param.rc.bStatWrite = 1;
		} else {
			m_x265Param.rc.bStatRead = 1;
		}
		m_x265Param.rc.statFileName = m_pPassLogFile;
	}

	//x265_setup_primitives(&m_x265Param, -1);

	m_pX265Ctx = x265_encoder_open(&m_x265Param);
	if(!m_pX265Ctx) {
		logger_err( LOGM_TS_VE_H265, "HEVC encoder initialize failed.\n");
		return false;
	}

	uint32_t nal = 0;
	x265_nal* pNal = NULL;
	if(x265_encoder_headers(m_pX265Ctx, &pNal, &nal) < 0) {
		logger_err( LOGM_TS_VE_H265, "x265_encoder_headers failed.\n");
		return false;
	} else {
		write_nals(pNal, nal);
	}

	x265_picture_init(&m_x265Param, &m_x265Pic);

	logger_status( LOGM_TS_VE_H265, "HEVC encoder is initialized successfully.\n");
	return true;
}

void CX265Encode::write_nals(x265_nal* pNal, int nalCount)
{
	//m_nalWriteMutex.Lock();
	 for (int i = 0; i < nalCount; i++)
    {
        m_pOutStream->Write((char*)pNal->payload, pNal->sizeBytes);
		pNal++;
    }
	// m_nalWriteMutex.Unlock();
}

bool CX265Encode::Stop()
{
	if(m_pX265Ctx) {
		uint32_t i_nal = 0;
		x265_nal* pNal = NULL;
		int encodeNum = 0;
		while(true) {
			encodeNum = x265_encoder_encode(m_pX265Ctx, &pNal, &i_nal, NULL, NULL);
			if(i_nal) {
				write_nals(pNal, i_nal);
			}
			if(encodeNum <= 0) break;
			m_frameCount += encodeNum;
		}
		x265_encoder_close(m_pX265Ctx);
		m_pX265Ctx = NULL;
		x265_cleanup();
	}
#ifdef DUMP_YUV
	if(dump_output) destroyStreamOutput(dump_output);
	dump_output = 0;
#endif

	return CVideoEncoder::Stop();
}

int CX265Encode::EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast)
{
	if(!pFrameBuf) return VIDEO_ENC_END;				// End encoding
	int ret = 0;
	fill_x265_pic(pFrameBuf);
	int64_t lastDts = 0;
	int encodedSize = encode_frame(&m_x265Pic, lastDts);
	if( encodedSize >= 0) {
		++m_frameCount;
		ret = m_frameCount;
	} else {
		ret = VIDEO_ENC_ERROR;
	}
	return ret;
}


void CX265Encode::fill_x265_pic(uint8_t* frameBuf)
{
	if(frameBuf != NULL) {
		m_x265Pic.planes[0] = (char*)frameBuf;
		m_x265Pic.planes[1] = (char*)(frameBuf) + m_planeSize;
		m_x265Pic.planes[2] = (char*)(frameBuf) + m_planeSize + (m_planeSize >> 2);
		m_x265Pic.bitDepth = 8;
		m_x265Pic.stride[0] = m_vInfo.res_out.width;
		m_x265Pic.stride[1] = m_x265Pic.stride[2] = m_x265Pic.stride[0] >> 1;

		#ifdef DUMP_YUV
		if(dump_output) {
			dump_output->Write((char*)m_x265Pic.planes[0], m_planeSize);
			dump_output->Write((char*)m_x265Pic.planes[1], m_planeSize/4);
			dump_output->Write((char*)m_x265Pic.planes[2], m_planeSize/4);
		}
		#endif
	}
}

int CX265Encode::encode_frame(x265_picture* pic, int64_t& lastDts)
{
	uint32_t i_nal = 0;
	int i_frame_size = 0;
	x265_nal* pNal = NULL;

	pic->poc = m_frameCount;
	i_frame_size = x265_encoder_encode(m_pX265Ctx, &pNal, &i_nal, pic, NULL);
	if( i_frame_size < 0 ) {
		logger_err(m_logModuleType, "HEVC_encoder_encode failed.\n" );
		return -1;
	}

	//if( i_frame_size > 0 ) {
	if(m_pOutStream && i_nal > 0) {
		write_nals(pNal, i_nal);
	}
	//}

	return i_frame_size;
}

#endif
