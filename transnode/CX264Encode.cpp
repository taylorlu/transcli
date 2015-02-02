#include "bitconfig.h"

#if defined (HAVE_LIBX264)
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
//#include <windows.h>
//#pragma comment(lib,"pthreadVC2.lib")
//#pragma comment(lib,"libx264.lib")
#else
#define SetConsoleTitle(t)
#endif

#include "MEvaluater.h"
#include "StreamOutput.h"
#include "CX264Encode.h"
#include "vfhead.h"
#include "logger.h"
#include "yuvUtil.h"

/* Pulldown types
static const char * const pulldown_names[] = { "none", "22", "32", "64", "double", "triple", "euro", 0 };
struct cli_pulldown_t{
	int mod;
	uint8_t pattern[24];
	float fps_factor;
} ;

enum pulldown_type_e
{
	X264_PULLDOWN_22 = 1,
	X264_PULLDOWN_32,
	X264_PULLDOWN_64,
	X264_PULLDOWN_DOUBLE,
	X264_PULLDOWN_TRIPLE,
	X264_PULLDOWN_EURO
};
#define TB  PIC_STRUCT_TOP_BOTTOM
#define BT  PIC_STRUCT_BOTTOM_TOP
#define TBT PIC_STRUCT_TOP_BOTTOM_TOP
#define BTB PIC_STRUCT_BOTTOM_TOP_BOTTOM

static const cli_pulldown_t pulldown_values[] =
{
	{0,  {0}, 1.0f},
	{1,  {TB},                                   1.0f},
	{4,  {TBT, BT, BTB, TB},                     1.25f},
	{2,  {PIC_STRUCT_DOUBLE, PIC_STRUCT_TRIPLE}, 1.0f},
	{1,  {PIC_STRUCT_DOUBLE},                    2.0f},
	{1,  {PIC_STRUCT_TRIPLE},                    3.0f},
	{24, {TBT, BT, BT, BT, BT, BT, BT, BT, BT, BT, BT, BT,BTB, TB, TB, TB, TB, TB, TB, TB, TB, TB, TB, TB}, 25.f/24.f}
};

#undef TB
#undef BT
#undef TBT
#undef BTB*/


CX264Encode::CX264Encode(const char* outFileName):CVideoEncoder(outFileName)
{
	m_logModuleType = LOGM_TS_VE_H264;
	m_pX264Ctx = NULL;
	m_planeSize = 0;
	m_firstDts = m_prevDts = m_lastDts = 0;
	m_largestPts = m_secondLargestPts = -1;
	m_ticksPerFrame = 0;
	m_citicalError = 0;
}

CX264Encode::~CX264Encode()
{
#ifdef DEBUG_DUMP_H264
	if (dump_output) {
		destroyStreamOutput(dump_output);
		dump_output = NULL;
	}
#endif
	Stop();
}

bool CX264Encode::Initialize()
{

	InitWaterMark();
	//Init file stream
	if(m_encodePass == 1) {
		m_pOutStream = getStreamOutput(CStreamOutput::FILE_STREAM);
		if(!m_pOutStream->Open(m_strOutFile.c_str())) {
			logger_err(m_logModuleType, "Open Out file error.\n");
			return false;
		}
	}

	m_frameCount = 0;
	m_planeSize = m_vInfo.res_out.width*m_vInfo.res_out.height;
	
	x264_param_default( &m_x264Param );
	
	int preset = m_pXmlPrefs->GetInt("videoenc.x264.preset");
	int tune =  m_pXmlPrefs->GetInt("videoenc.x264.tune");
	if (preset > 0 && tune > 0 ) {
		x264_param_default_preset(&m_x264Param, 
			x264_preset_names[preset-1], x264_tune_names[tune-1]);
	}
	
	int showSei = m_pXmlPrefs->GetInt("videoenc.x264.showInfo");
	if(showSei >= 0) {
		m_x264Param.i_verinfo_type = showSei;
	}
	// If use turbo mode in pass one
	bool bTurbo = true;
	if(m_pXmlPrefs->GetInt("videoenc.x264.turbo") <= 0 ||
		(preset > 0 && !_stricmp(x264_preset_names[preset-1], "placebo"))) {
		bTurbo = false;
	}
	
	// Rate control
	int rateMode = m_pXmlPrefs->GetInt("overall.video.mode");

	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) {
		m_vInfo.bitrate = 600;
	}
	int maxRate = m_pXmlPrefs->GetInt("videoenc.x264.vbv_maxrate");
	if(maxRate < 0) maxRate = 0;

	int vbvBufSize = m_pXmlPrefs->GetInt("videoenc.x264.vbv_bufsize");
	float vbvInitSize = m_pXmlPrefs->GetFloat("videoenc.x264.vbv_init");
	float ratetol = m_pXmlPrefs->GetFloat("videoenc.x264.ratetol");
	m_x264Param.rc.i_bitrate = m_vInfo.bitrate;

	float qp = (100 - m_pXmlPrefs->GetInt("overall.video.quality")) / 2.f;

	switch (rateMode) {
		case RC_MODE_ABR:
			rateMode = X264_RC_ABR;
			if(maxRate > 0 ) m_x264Param.rc.i_vbv_max_bitrate = maxRate;
			break;
		case RC_MODE_VBR:
			if(qp > 0) {
				m_x264Param.rc.f_rf_constant = qp;
			}
			rateMode = X264_RC_CRF;
			if(maxRate > 0 ) m_x264Param.rc.i_vbv_max_bitrate = maxRate;
			if(vbvBufSize > 0) m_x264Param.rc.i_vbv_buffer_size = vbvBufSize;
			break;
		case RC_MODE_CBR:
			rateMode = X264_RC_ABR;
			ratetol = 0.1f;
			m_x264Param.rc.i_vbv_max_bitrate = (int)(m_vInfo.bitrate*1.1);
			vbvBufSize = m_vInfo.bitrate / 4;
			break;
			//m_x264Param.rc.f_qcompress = 0.0f;
		case RC_MODE_2PASS:
		case RC_MODE_3PASS:
			rateMode = X264_RC_ABR;
			if(maxRate > 0 ) m_x264Param.rc.i_vbv_max_bitrate = maxRate;
			break;
	}
	
	// Rate control
	m_x264Param.rc.i_rc_method = rateMode;
	if(rateMode == X264_RC_ABR) {
		if(vbvBufSize > 0) {
			m_x264Param.rc.i_vbv_buffer_size = vbvBufSize;
			if(ratetol > 0.001f) {
				m_x264Param.rc.f_rate_tolerance = ratetol;
			} 
		}
		
		if(vbvInitSize > 0) m_x264Param.rc.f_vbv_buffer_init = vbvInitSize;
	}

	float ipratio = m_pXmlPrefs->GetFloat("videoenc.x264.ipratio");
	if(ipratio > 0) {
		m_x264Param.rc.f_ip_factor = ipratio;
	}
	float pbratio = m_pXmlPrefs->GetFloat("videoenc.x264.pbratio");
	if(pbratio > 0) {
		m_x264Param.rc.f_pb_factor = pbratio;
	}
	int qpmax = m_pXmlPrefs->GetInt("videoenc.x264.qpmax");
	if(qpmax >0 && qpmax < 69) {
		m_x264Param.rc.i_qp_max = qpmax;
	}
	int qpmin = m_pXmlPrefs->GetInt("videoenc.x264.qpmin");
	if(qpmin >0 && qpmin < 69) {
		m_x264Param.rc.i_qp_min = qpmin;
	}
	int qpstep = m_pXmlPrefs->GetInt("videoenc.x264.qpstep");
	if(qpstep >=0 && qpstep <= 50) {
		m_x264Param.rc.i_qp_step = qpstep;
	}
	int qcomp = m_pXmlPrefs->GetInt("videoenc.x264.qcomp");
	if(qcomp >= 0 && qcomp <= 100) {
		float fqcomp = qcomp / 100.f;
		m_x264Param.rc.f_qcompress = fqcomp;
	}
	int qpOffset = m_pXmlPrefs->GetInt("videoenc.x264.qp_offset");
	if(qpOffset !=0) {
		m_x264Param.analyse.i_chroma_qp_offset = qpOffset;
	}

	int rcLookAhead = m_pXmlPrefs->GetInt("videoenc.x264.rc_lookahead");
	if(rcLookAhead > 0) {
		m_x264Param.rc.i_lookahead = rcLookAhead;
	}

	bool bMbtree = m_pXmlPrefs->GetBoolean("videoenc.x264.mbtree");
	if(bMbtree) {
		m_x264Param.rc.b_mb_tree = 1;
	} else {
		m_x264Param.rc.b_mb_tree = 0;
	}
	
	int aqMode = m_pXmlPrefs->GetInt("videoenc.x264.aq_mode");
	if(aqMode >= 0 && aqMode <= 2) {
		m_x264Param.rc.i_aq_mode = aqMode;
	}
	float aqStrength = m_pXmlPrefs->GetFloat("videoenc.x264.aq_strength");
	if(aqStrength > 0) {
		m_x264Param.rc.f_aq_strength = aqStrength;
	}

	// Target Qp (Extension param) used in the 2nd pass
	float targetQp = m_pXmlPrefs->GetFloat("videoenc.x264.targetQp"); 
	if(m_bMultiPass && m_encodePass == 1 && targetQp > 0 && targetQp < 69) {
		m_x264Param.f_targetQp = targetQp;
	}

	// Zones (different bitrate)
	float zoneStart1 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneStart1");
	float zoneEnd1 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneEnd1");
	if(zoneStart1 > -0.00001f && zoneEnd1 > zoneStart1) {
		char* zoneStr = (char*)malloc(64);
		memset(zoneStr, 0, 64);
		float fpsVal = (float)m_vInfo.fps_out.num/(float)m_vInfo.fps_out.den;
		int startSec = (int)(zoneStart1*fpsVal);
		int endSec = (int)(zoneEnd1*fpsVal);
		float zoneRf1 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneRf1");
		int strNum = sprintf(zoneStr, "%d,%d,b=%1.2f", startSec, endSec, zoneRf1);

		float zoneStart2 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneStart2");
		float zoneEnd2 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneEnd2");
		if(zoneStart2 > -0.00001f && zoneEnd2 > zoneStart2 &&
			(zoneStart2 > zoneEnd1) || (zoneStart1 > zoneEnd2)) {	// not overlap
			float zoneRf2 = m_pXmlPrefs->GetFloat("videoenc.x264.zoneRf2");
			startSec = (int)(zoneStart2*fpsVal);
			endSec = (int)(zoneEnd2*fpsVal);
			sprintf(zoneStr+strNum, "/%d,%d,b=%1.2f", startSec, endSec, zoneRf2);
		}
		m_x264Param.rc.psz_zones = zoneStr;
		logger_info(m_logModuleType, "Enable x264 zone:%s\n", zoneStr);
	}
	
	// Basic parameter
	m_x264Param.i_width = m_vInfo.res_out.width;
	m_x264Param.i_height = m_vInfo.res_out.height;
	m_x264Param.i_fps_num = m_vInfo.fps_out.num;
	m_x264Param.i_fps_den = m_vInfo.fps_out.den;
	
	m_x264Param.vui.i_sar_width = m_vInfo.dest_par.num;
	m_x264Param.vui.i_sar_height = m_vInfo.dest_par.den;
	
	int x264Profile = m_pXmlPrefs->GetInt("videoenc.x264.profile");
	if(x264Profile <= 0) x264Profile = H264_HIGH;
	int x264Level   = m_pXmlPrefs->GetInt("videoenc.x264.level");
	if(x264Level > 0) m_x264Param.i_level_idc = x264Level;
	bool ifDeblock   = m_pXmlPrefs->GetBoolean("videoenc.x264.deblock");
	bool ifCabac     = m_pXmlPrefs->GetBoolean("videoenc.x264.cabac");
	int threadNum   = m_pXmlPrefs->GetInt("videoenc.x264.threads");
	if(threadNum > 0) m_x264Param.i_threads = threadNum;
	int bFrameNum   = m_pXmlPrefs->GetInt("videoenc.x264.bframes");
	if(bFrameNum > 0) m_x264Param.i_bframe = bFrameNum;
	int bframeAdapt = m_pXmlPrefs->GetInt("videoenc.x264.b_adapt");
	if(bframeAdapt >= 0 && bframeAdapt <= 2) {
		m_x264Param.i_bframe_adaptive = bframeAdapt;
	}
	int bframeBias = m_pXmlPrefs->GetInt("videoenc.x264.b_bias");
	if(bframeBias >= -100 && bframeBias <= 100) {
		m_x264Param.i_bframe_bias = bframeBias;
	}

	int outFps = m_vInfo.fps_out.num/m_vInfo.fps_out.den;
	int keyInt = m_pXmlPrefs->GetInt("videoenc.x264.keyint");
	if(keyInt <= 0) {
		keyInt = 10*outFps;
	} else if(keyInt < 10) {	// 0<keyint<10, it's seconds
		keyInt *= outFps;
	}
	if(keyInt > 0) m_x264Param.i_keyint_max = keyInt;
	
	int keyIntMin = m_pXmlPrefs->GetInt("videoenc.x264.keyint_min");
	if(keyIntMin > outFps) keyIntMin = outFps;
	if(keyIntMin > 0) m_x264Param.i_keyint_min = keyIntMin;
	
	m_x264Param.b_cabac = ifCabac ? 1 : 0;
	m_x264Param.b_deblocking_filter = ifDeblock ? 1 : 0;
	if(ifDeblock) {
		int deblockAlpha = m_pXmlPrefs->GetInt("videoenc.x264.deblockAlpha");
		int deblockBeta = m_pXmlPrefs->GetInt("videoenc.x264.deblockBeta");
		if(deblockBeta > -7 && deblockBeta < 7 && deblockAlpha > -7 && deblockAlpha < 7) {
			m_x264Param.i_deblocking_filter_alphac0 = deblockAlpha;
			m_x264Param.i_deblocking_filter_beta = deblockBeta;
		}
	}
	int refFrames = m_pXmlPrefs->GetInt("videoenc.x264.frameref");
	if(refFrames > 0) m_x264Param.i_frame_reference = refFrames;
	// analyse parameter
	int me = m_pXmlPrefs->GetInt("videoenc.x264.me");
	if(me >= 0 && me <= 4) {
		m_x264Param.analyse.i_me_method = me;
	}
	int meRange = m_pXmlPrefs->GetInt("videoenc.x264.me_range");
	if(meRange >=4 && meRange <= 64) {
		m_x264Param.analyse.i_me_range = meRange;
	}
	int subme = m_pXmlPrefs->GetInt("videoenc.x264.subme");
	if(subme >=0 && subme <= 10) {
		m_x264Param.analyse.i_subpel_refine = subme;
	}
	int directPred = m_pXmlPrefs->GetInt("videoenc.x264.direct_pred");
	if(directPred >= 0 && directPred <= 3) {
		//const char* strDirect[] = {"none", "spatial", "temporal", "auto"};
		m_x264Param.analyse.i_direct_mv_pred = directPred;
	}

	// Advanced parameter
	int scenecutNum = m_pXmlPrefs->GetInt("videoenc.x264.scenecut");
	if(scenecutNum >= 0) {
		m_x264Param.i_scenecut_threshold = scenecutNum;
	}

	int weight_p = m_pXmlPrefs->GetInt("videoenc.x264.weight_p");
	if(weight_p >= 0 && weight_p <= 2) {
		m_x264Param.analyse.i_weighted_pred = weight_p;
	}

	int bpyramid = m_pXmlPrefs->GetInt("videoenc.x264.b_pyramid");
	if(bpyramid >= 0 && bpyramid <= 2) {
		m_x264Param.i_bframe_pyramid = bpyramid;
	}

	int nalhrdVal = m_pXmlPrefs->GetInt("videoenc.x264.nalhrd");
	if(nalhrdVal == 1 || nalhrdVal == 2) {
		m_x264Param.i_nal_hrd = nalhrdVal;
	}

	int partitions = m_pXmlPrefs->GetInt("videoenc.x264.partitions");
	if(partitions >= 0 && partitions <= 3) {
		const char* strPartition[] = {"none", "p8x8,i4x4,b8x8", "p8x8,i8x8,i4x4,b8x8", "all"};
		const char* parStr = strPartition[partitions];
		m_x264Param.analyse.inter = 0;
		if( strstr( parStr, "none" ) )  m_x264Param.analyse.inter =  0;
		if( strstr( parStr, "all" ) )   m_x264Param.analyse.inter = ~0;
		if( strstr( parStr, "i4x4" ) )  m_x264Param.analyse.inter |= X264_ANALYSE_I4x4;
		if( strstr( parStr, "i8x8" ) )  m_x264Param.analyse.inter |= X264_ANALYSE_I8x8;
		if( strstr( parStr, "p8x8" ) )  m_x264Param.analyse.inter |= X264_ANALYSE_PSUB16x16;
		if( strstr( parStr, "p4x4" ) )  m_x264Param.analyse.inter |= X264_ANALYSE_PSUB8x8;
		if( strstr( parStr, "b8x8" ) )  m_x264Param.analyse.inter |= X264_ANALYSE_BSUB16x16;
	}

	bool p8x8dct = m_pXmlPrefs->GetBoolean("videoenc.x264.p8x8dct");
	if(!p8x8dct && x264Profile != H264_HIGH) {
		m_x264Param.analyse.b_transform_8x8 = 0;
	}

	bool noChromaMe = m_pXmlPrefs->GetBoolean("videoenc.x264.no_choma_me");
	if(noChromaMe) {
		m_x264Param.analyse.b_chroma_me = 0;
	}

	bool mixRef = m_pXmlPrefs->GetBoolean("videoenc.x264.mixed_refs");
	if(!mixRef) {
		m_x264Param.analyse.b_mixed_references = 0;
	}

	bool fastPSkip = m_pXmlPrefs->GetBoolean("videoenc.x264.fast_pskip");
	if(!fastPSkip) {
		m_x264Param.analyse.b_fast_pskip = 0;
	}

	bool dctDecimate = m_pXmlPrefs->GetBoolean("videoenc.x264.dct_decimate");
	if(!dctDecimate) {
		m_x264Param.analyse.b_dct_decimate = 0;
	}

	bool bWeightB = m_pXmlPrefs->GetBoolean("videoenc.x264.weight_b");
	if(bFrameNum > 1 && bWeightB) {
		m_x264Param.analyse.b_weighted_bipred = 1;
	} else {
		m_x264Param.analyse.b_weighted_bipred = 0;
	}

	bool bPsy = m_pXmlPrefs->GetBoolean("videoenc.x264.psy");
	if(bPsy) {
		m_x264Param.analyse.b_psy = 1;
		float psyRd = m_pXmlPrefs->GetFloat("videoenc.x264.psy_rd");
		float psyTrellis = m_pXmlPrefs->GetFloat("videoenc.x264.psy_trellis");
		m_x264Param.analyse.f_psy_rd = psyRd;
		m_x264Param.analyse.f_psy_trellis = psyTrellis;
	} else {
		m_x264Param.analyse.b_psy = 0;
	}

// 	int nr = m_pXmlPrefs->GetInt("videoenc.x264.nr");
// 	if(nr >= 100 && nr <= 100000) {
// 		sstr << " --nr " << nr;
// 	}

	if(m_x264Param.b_cabac) {
		int trellis = m_pXmlPrefs->GetInt("videoenc.x264.trellis");
		if(trellis >= 0 && trellis <= 2) {
			m_x264Param.analyse.i_trellis = trellis;
		}
	}

	int spsId = m_pXmlPrefs->GetInt("videoenc.x264.spsid");
	if(spsId > 0) {
		m_x264Param.i_sps_id = spsId;
	}

	// Interlace encoding
	if(m_pXmlPrefs->GetBoolean("overall.task.interlace")) {
		m_x264Param.b_interlaced = 1;
		if(m_vInfo.interlaced == 1) {
			m_x264Param.b_tff = 1;
		} else {
			m_x264Param.b_tff = 0;
		}
	}

	bool bPsnr = m_pXmlPrefs->GetBoolean("videoenc.x264.psnr");
	if(bPsnr && m_encodePass == 1) {
		//if(targetQp > 0 || m_encodePass == 1) {
			m_x264Param.analyse.b_psnr = 1;
		//}
	}
	bool bSsim = m_pXmlPrefs->GetBoolean("videoenc.x264.ssim");
	if(bSsim && m_encodePass == 1) {
		//if(targetQp > 0 || m_encodePass == 1) {
			m_x264Param.analyse.b_ssim = 1;
		//}
	}
	bool bSavePsnr = m_pXmlPrefs->GetBoolean("videoenc.x264.savePsnr");
	if(bSavePsnr) {		// Save psnr and ssim in both pass
		std::string psnrFile = m_destFileName;
		if(psnrFile.empty()) {
			psnrFile = m_strOutFile;
		}
		psnrFile += ".psnr.txt";
		strncpy(m_x264Param.psz_psnr_file, psnrFile.c_str(), 256);
	}

	// Extra customed parameters
	if(m_encodePass == 1) {
		const char* userDataStr = m_pXmlPrefs->GetString("videoenc.x264.userData");
		if(userDataStr && *userDataStr) {
			strncpy(m_x264Param.psz_custom_data, userDataStr, 64);
		}
	
		// Analyse qp from psnr.txt file
		if(m_pXmlPrefs->GetBoolean("videoenc.x264.adjustMinQP") && bSavePsnr &&
			FileExist(m_x264Param.psz_psnr_file) ) {	
			char* qpContent = ReadTextFile(m_x264Param.psz_psnr_file);
			if(qpContent) {
				int iQp = 0, pQp = 0, bQp = 0, avgQp = 0;
				const char* qptype[4] = {"IQP:", "PQP:", "BQP:", "Average QP:"};
				double pass1Qp[4] = {0.0};
					// Read I/P/B/Avg Qp from file content
				for(int idx = 0; idx < 4; ++idx) {
					char* tmp = strstr(qpContent, qptype[idx]);
					if(tmp) {
						tmp += strlen(qptype[idx]);
						pass1Qp[idx] = atof(tmp);
					}
				}
				iQp = (int)(pass1Qp[0] + 0.5); pQp = (int)(pass1Qp[1] + 0.5);
				bQp = (int)(pass1Qp[2] + 0.5); avgQp = (int)(pass1Qp[3] + 0.5);

                int maxQp = MAX(MAX(iQp, pQp), bQp);
                int minQp = MIN(MIN(iQp, pQp), bQp);

				int minQp2pass = maxQp;
				switch(m_pXmlPrefs->GetInt("videoenc.x264.minQPType")) {
				case 1: minQp2pass = minQp; break;
				case 2: 
					minQp2pass = avgQp;
					if(m_vInfo.res_out.width > 960 && m_vInfo.res_out.width < 1440) minQp2pass += 1;
                    minQp2pass = MIN(minQp2pass, maxQp);
					break;
				case 3: minQp2pass = iQp; break;
				case 4: minQp2pass = pQp; break;
				case 5: minQp2pass = bQp; break;
				}

				// If average Qp > threshold, increase bitrate
				int incBrThreshold = m_pXmlPrefs->GetInt("videoenc.x264.incBrThreshold");
				if(incBrThreshold > 0 && avgQp > incBrThreshold) {
					int incBrPercent = m_pXmlPrefs->GetInt("videoenc.x264.incBrPercent");
					if(incBrPercent > 0) {
						m_x264Param.rc.i_bitrate += (int)(m_vInfo.bitrate*incBrPercent/100.f);
					}
				}

				int pass2MaxMinQp = m_pXmlPrefs->GetInt("videoenc.x264.pass2MaxMinQp");
				int pass2MinMinQp = m_pXmlPrefs->GetInt("videoenc.x264.pass2MinMinQp");

				if(pass2MaxMinQp > 0 && minQp2pass > pass2MaxMinQp) minQp2pass = pass2MaxMinQp;
				if(pass2MinMinQp > 0 && minQp2pass < pass2MinMinQp) minQp2pass = pass2MinMinQp;
					
				int incQpThreshold = m_pXmlPrefs->GetInt("videoenc.x264.incMinQpThreshold");
				if(minQp2pass <= incQpThreshold) {
					minQp2pass += 1;
					//if(minQp2pass > maxQp) minQp2pass = maxQp;
				}
					
				m_x264Param.rc.i_qp_min = minQp2pass;
			}
		}
	}

	
	// Adjust reference frames
	if(refFrames > 1) {
		int mbs = (((m_x264Param.i_width)+15)>>4) * (((m_x264Param.i_height)+15)>>4);
		int i;
		for( i = 0; x264_levels[i].level_idc != 0; i++ )
			if( m_x264Param.i_level_idc == x264_levels[i].level_idc ) {
				while( mbs * 384 * m_x264Param.i_frame_reference > x264_levels[i].dpb
					&& m_x264Param.i_frame_reference > 1 ) {
						m_x264Param.i_frame_reference--;
				}
				break;
			}
	}

	// 2Pass control
	if(m_bMultiPass) {
		if(m_encodePass == 2) {
			m_x264Param.rc.b_stat_write = 1;
			m_x264Param.rc.psz_stat_out = m_pPassLogFile;
			if(!applyFastFirstPass(bTurbo)) return false;
		} else {
			m_x264Param.rc.b_stat_read = 1;
			m_x264Param.rc.psz_stat_in = m_pPassLogFile;
		}
	}

	/* Apply profile restrictions. */
	if(!applyProfileRestrict(x264Profile)) {
		return false;
	}
	
	if(m_x264Param.i_fps_den > 0) {
		m_x264Param.b_vfr_input = 0;
	}
	
	if(!m_x264Param.b_vfr_input) {
		m_x264Param.i_timebase_num = m_x264Param.i_fps_den;
		m_x264Param.i_timebase_den = m_x264Param.i_fps_num;
	}

	if( ( m_pX264Ctx = x264_encoder_open( &m_x264Param ) ) == NULL ) {
		logger_err(m_logModuleType, "H264_encoder_open failed.\n" );
		return false;
	}
	x264_encoder_parameters(m_pX264Ctx, &m_x264Param );

	
	// Process params and init x264 encoder
	m_ticksPerFrame = (int64_t)m_x264Param.i_timebase_den * m_x264Param.i_fps_den / m_x264Param.i_timebase_num / m_x264Param.i_fps_num;
	if( m_ticksPerFrame < 1 && !m_x264Param.b_vfr_input) {
		logger_err(LOGM_TS_VE_H264, "Wrong ticks per frame.\n");
		return false;
	}
	if(m_ticksPerFrame) m_ticksPerFrame = 1;

	/* Create a new pic */
	if(x264_picture_alloc(&m_x264Pic, X264_CSP_I420, m_x264Param.i_width, m_x264Param.i_height) != 0) {
		logger_err(m_logModuleType, "H264_picture_alloc failed.\n" );
		return false;
	}

	/* Do not force any parameters */
	m_x264Pic.i_type = X264_TYPE_AUTO;
	m_x264Pic.i_qpplus1 = 0;

	// dts/pts processing

	logger_status( LOGM_TS_VE_H264, "H264 encoder is initialized successfully\n");
	return true;
}

bool CX264Encode::applyProfileRestrict(int profileNum)
{
	switch(profileNum) {
	case  H264_BASE_LINE:
		m_x264Param.analyse.b_transform_8x8 = 0;
		m_x264Param.b_cabac = 0;
		m_x264Param.i_cqm_preset = X264_CQM_FLAT;
		m_x264Param.psz_cqm_file = NULL;
		m_x264Param.i_bframe = 0;
		m_x264Param.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
		if( m_x264Param.b_interlaced ) {
			logger_err(LOGM_TS_VE_H264, "Baseline profile doesn't support interlacing.\n" );
			return false;
		}
		break;
	case H264_MAIN:
		m_x264Param.analyse.b_transform_8x8 = 0;
		m_x264Param.i_cqm_preset = X264_CQM_FLAT;
		m_x264Param.psz_cqm_file = NULL;
		break;
	case H264_HIGH:
		// Default param setting is high
		break;
	default:
		logger_err(LOGM_TS_VE_H264, "Invalid profile!\n");
		return false;
	}
	return true;
}

bool CX264Encode::applyFastFirstPass(bool bTurbo)
{
	if(bTurbo) {
		m_x264Param.i_frame_reference = 2;
		m_x264Param.analyse.b_transform_8x8 = 0;
		m_x264Param.analyse.inter = 0;
		m_x264Param.analyse.i_me_method = X264_ME_DIA;
        m_x264Param.analyse.i_subpel_refine = MIN( 4, m_x264Param.analyse.i_subpel_refine );
		m_x264Param.analyse.i_trellis = 0;
		m_x264Param.analyse.i_me_range = 16;
	}
	return true;
}

bool CX264Encode::Stop()
{
	if(m_pX264Ctx) {
		while(x264_encoder_delayed_frames( m_pX264Ctx ) ) {
			m_prevDts = m_lastDts;
			int i_frame_size = encode_frame(NULL, m_lastDts );
			if(i_frame_size < 0) {
				logger_err(LOGM_TS_VE_H264, "H264_encoder_delayed_frames failed!\n");
				break;
			} else if(i_frame_size) {
				m_frameCount++;
				if( m_frameCount == 1 )
					m_firstDts = m_prevDts = m_lastDts;
			}
		}

		x264_picture_clean( &m_x264Pic );
		x264_encoder_close( m_pX264Ctx );
		m_pX264Ctx = NULL;
	}

	if(m_bMultiPass) {
		std::string psnrFile = m_destFileName;
		psnrFile += ".psnr.txt";
		if(FileExist(psnrFile.c_str())) {
			if(m_encodePass == 2) {		// Backup pass1 log
				if(m_pXmlPrefs->GetBoolean("videoenc.x264.backup1passLog")) {
					std::string backupFile = m_destFileName + ".psnr_pass1.txt";
                    TsCopyFile(psnrFile.c_str(), backupFile.c_str());
				}
			} else {	// Remove Qp log file after done
				if(m_pXmlPrefs->GetBoolean("videoenc.x264.removeQpLog")) {
					RemoveFile(psnrFile.c_str());
				}
			}
		}
	}
	return CVideoEncoder::Stop();
}

int CX264Encode::GetDelayedFrames()
{
	if(m_pX264Ctx) {
		return 10/*x264_encoder_delayed_frames(m_pX264Ctx)*/;
	}
	return 0;
}

int CX264Encode::EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast)
{
	if(!pFrameBuf) return VIDEO_ENC_END;				// End encoding
	int ret = 0;
	fill_x264_pic(pFrameBuf);
	if( !m_x264Param.b_vfr_input )
		m_x264Pic.i_pts = m_frameCount;

	if( m_x264Pic.i_pts <= m_largestPts ) {
		m_x264Pic.i_pts = m_largestPts + m_ticksPerFrame;
	}

	m_secondLargestPts = m_largestPts;
	m_largestPts = m_x264Pic.i_pts;
	m_x264Pic.i_type = X264_TYPE_AUTO;
	m_x264Pic.i_qpplus1 = 0;
	m_x264Pic.i_pic_struct = PIC_STRUCT_AUTO;

	m_prevDts = m_lastDts;
	int encodedSize = encode_frame(&m_x264Pic, m_lastDts);
	if( encodedSize >= 0) {
		++m_frameCount;
		if(m_frameCount == 1) {
			m_firstDts = m_prevDts = m_lastDts;
		}
		ret = m_frameCount;
	} else {
		ret = VIDEO_ENC_ERROR;
	}
	return ret;
}


void CX264Encode::fill_x264_pic(const uint8_t* frameBuf)
{
	if(frameBuf != NULL) {
		memcpy(m_x264Pic.img.plane[0], frameBuf, m_planeSize);
		memcpy(m_x264Pic.img.plane[1], frameBuf+m_planeSize, m_planeSize/4);
		memcpy(m_x264Pic.img.plane[2], frameBuf+5*m_planeSize/4, m_planeSize/4);
	}
}

int CX264Encode::encode_frame(x264_picture_t* pic, int64_t& lastDts)
{
	if(m_citicalError) return -1;

	x264_picture_t pic_out;
	x264_nal_t *nal = NULL;
	int i_nal = 0;
	int i_frame_size = 0;

	i_frame_size = x264_encoder_encode( m_pX264Ctx, &nal, &i_nal, pic, &pic_out );

	if( i_frame_size < 0 ) {
		logger_err(m_logModuleType, "H264_encoder_encode failed.\n" );
		m_citicalError = 1;
		return -1;
	}

	if( i_frame_size > 0 ) {
		if(m_encodePass == 1 && m_pOutStream) {
			i_frame_size = m_pOutStream->Write((char*)(nal[0].p_payload), i_frame_size);
		}
		lastDts = pic_out.i_dts;
	}

	return i_frame_size;
}

#endif
