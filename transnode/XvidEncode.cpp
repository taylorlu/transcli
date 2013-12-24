#include "XvidEncode.h"
#include "logger.h"
#include <stdio.h>
//#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#include <time.h>
#endif

#include "MEvaluater.h"
#include "vfhead.h"
#include "StreamOutput.h"

#define DEFAULT_QUANT 400

CXvidEncode::CXvidEncode(const char* outFileName):CVideoEncoder(outFileName),
m_pXvidHandle(NULL)
{
	m_logModuleType = LOGM_TS_VE_XVID;
	m_xvidFrame.bitstream = NULL;
}

bool CXvidEncode::Initialize() 
{
	InitWaterMark();
	//Init file stream
	m_pOutStream = getStreamOutput(CStreamOutput::FILE_STREAM);
	if(!m_pOutStream->Open(m_strOutFile.c_str())) {
		logger_err(m_logModuleType, "Open Output file error.\n");
		return false;
	}

	xvid_plugin_single_t single;
	//xvid_plugin_2pass1_t rc2pass1;
	//xvid_plugin_2pass2_t rc2pass2;
	//xvid_plugin_ssim_t ssim;
	//xvid_plugin_fixed_t rcfixed;
	xvid_enc_plugin_t plugins[7];
	xvid_gbl_init_t xvid_gbl_init;
	xvid_enc_create_t xvid_enc_create;
	//m_yuvFrameSize = m_vInfo.res_in.width*m_vInfo.res_in.height*3/2;
	
	//==========================Default params=============================
	int		debugLevel    = 0;
	bool	bUseAssembler = true;
	int		singleEnc	  = 1;
	int		profileIndex = m_pXmlPrefs->GetInt("videoenc.xvid.profile");
	int		maxBframes   = m_pXmlPrefs->GetInt("videoenc.xvid.bframes");
	int		encThreadNum  = m_pXmlPrefs->GetInt("videoenc.xvid.threads");
	int		colorSpace	  = XVID_CSP_I420;
	int		reactFactor	  = 16;
	int     averagePeriod = 100;
	int     bufSmooth     = 100;
	int     maxKeyInter   = m_pXmlPrefs->GetInt("videoenc.xvid.keyint");
	float   fps			  = (float)m_vInfo.fps_out.num/m_vInfo.fps_out.den;
	int		quants[6]     = {2, 31, 2, 31, 2, 31};
	int     bqRatio		  = m_pXmlPrefs->GetInt("videoenc.xvid.b_ratio");
	int     bqOffset	  = m_pXmlPrefs->GetInt("videoenc.xvid.b_offset");
	int     enableStats   = 0;
	int		vopQuality = m_pXmlPrefs->GetInt("videoenc.xvid.vop");
	int     meQuality = m_pXmlPrefs->GetInt("videoenc.xvid.meQuality");

	const int vop_presets[] = {
		0,											// quality 0
		0,											// quality 1
		XVID_VOP_HALFPEL,							// quality 2
		XVID_VOP_HALFPEL | XVID_VOP_INTER4V,		// quality 3
		XVID_VOP_HALFPEL | XVID_VOP_INTER4V,		// quality 4
		XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
		XVID_VOP_TRELLISQUANT,						// quality 5
		XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
		XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED,	// quality 6
	};

	const int motion_presets[] = {
		0,															// quality 0
		XVID_ME_ADVANCEDDIAMOND16,									// quality 1
		XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16,		// quality 2
		XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
		XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8,			// quality 3
		XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
		XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
		XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,					// quality 4
		XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
		XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
		XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,					// quality 5

		XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
		XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
		XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,					// quality 6
	};

	int xvidProfileSet[] = {0x00, 0x08, 0x01,0x02,0x03,0x91, 0x92,0x93,0x94,0xf0, 0xf1,0xf2, 0xf3, 0xf4};
	
	/*------------------------------------------------------------------------
	* XviD core initialization
	*----------------------------------------------------------------------*/

	/* Set version -- version checking will done by xvidcore */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
	xvid_gbl_init.version = XVID_VERSION;
	xvid_gbl_init.debug = debugLevel;

	/* Do we have to enable ASM optimizations ? */
	if (bUseAssembler) {
#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_ASM;
#else
		xvid_gbl_init.cpu_flags = 0;
#endif
	} else {
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;
	}

	/* Initialize XviD core -- Should be done once per __process__ */
	xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);
	if(encThreadNum <= 0) {
		getCpuAndEncInfo(encThreadNum);
	}

	/*------------------------------------------------------------------------
	* XviD encoder initialization
	*----------------------------------------------------------------------*/

	/* Version again */
	memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
	xvid_enc_create.version = XVID_VERSION;

	/* Width and Height of input frames */
	xvid_enc_create.width = m_vInfo.res_out.width;
	xvid_enc_create.height = m_vInfo.res_out.height;
	
	if(profileIndex > (int)(sizeof(xvidProfileSet)/sizeof(xvidProfileSet[0]))) {
		profileIndex = 0;
	}

	xvid_enc_create.profile = xvidProfileSet[profileIndex]; 

	/* init plugins  */
	//    xvid_enc_create.zones = ZONES;
	//    xvid_enc_create.num_zones = NUM_ZONES;

	xvid_enc_create.plugins = plugins;
	xvid_enc_create.num_plugins = 0;
	
	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) m_vInfo.bitrate = 900;		//default value
	if (singleEnc) {
		memset(&single, 0, sizeof(xvid_plugin_single_t));
		single.version = XVID_VERSION;
		single.bitrate = m_vInfo.bitrate*1000;
		single.reaction_delay_factor = reactFactor;
		single.averaging_period = averagePeriod;
		single.buffer = bufSmooth;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
		plugins[xvid_enc_create.num_plugins++].param = &single;
	}

#ifdef FIXME
	xvid_enc_create.num_threads = 1; //encThreadNum;
#else
	xvid_enc_create.num_threads = encThreadNum;
#endif

	/* Frame rate  */
	xvid_enc_create.fincr = m_vInfo.fps_out.den;
	xvid_enc_create.fbase = m_vInfo.fps_out.num;

	/* Maximum key frame interval */
	if(maxKeyInter <= 0) {
		maxKeyInter = (int)(10*fps);
	} else if(maxKeyInter < 10) {	// 0<keyint<10, it's seconds
		maxKeyInter = (int)(maxKeyInter*fps);
	}
	xvid_enc_create.max_key_interval = maxKeyInter;
	xvid_enc_create.min_quant[0] = quants[0];
	xvid_enc_create.min_quant[1] = quants[2];
	xvid_enc_create.min_quant[2] = quants[4];
	xvid_enc_create.max_quant[0] = quants[1];
	xvid_enc_create.max_quant[1] = quants[3];
	xvid_enc_create.max_quant[2] = quants[5];

	/* Bframes settings */
	xvid_enc_create.max_bframes = maxBframes;
	xvid_enc_create.bquant_ratio = bqRatio;
	xvid_enc_create.bquant_offset = bqOffset;

	/* Frame drop ratio */
	xvid_enc_create.frame_drop_ratio = 0;

	/* Global encoder options */
	xvid_enc_create.global = 0;

	if (m_pXmlPrefs->GetBoolean("videoenc.xvid.packed"))
		xvid_enc_create.global |= XVID_GLOBAL_PACKED;

	if (m_pXmlPrefs->GetBoolean("videoenc.xvid.closedgop"))
		xvid_enc_create.global |= XVID_GLOBAL_CLOSED_GOP;

	if (enableStats)
		xvid_enc_create.global |= XVID_GLOBAL_EXTRASTATS_ENABLE;

	/* I use a small value here, since will not encode whole movies, but short clips */
	int xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);
	if(xerr == XVID_ERR_MEMORY) {
		logger_err(m_logModuleType, "xvid encoder create failed.\n");
		return false;
	}
	/* Retrieve the encoder instance from the structure */
	m_pXvidHandle = xvid_enc_create.handle;

	//-------------- Init xvid_enc_stats --------------------
	//memset(&m_xvidStats, 0, sizeof(m_xvidStats));
	//m_xvidStats.version = XVID_VERSION;

	/*---------------------------------------------------------------------
	* XviD frame struct initialization
	*----------------------------------------------------------------------*/
	memset(&m_xvidFrame, 0, sizeof(m_xvidFrame));
	m_xvidFrame.version = XVID_VERSION;
	m_xvidFrame.vol_flags = 0;
	m_xvidFrame.quant_intra_matrix = NULL;
	m_xvidFrame.quant_inter_matrix = NULL;
	int parMode = m_pXmlPrefs->GetInt("videoenc.xvid.parMode");
	if(parMode == 0 && m_vInfo.dest_par.num > 0 && m_vInfo.dest_par.den > 0) {
		m_xvidFrame.par = XVID_PAR_EXT;
		m_xvidFrame.par_width = m_vInfo.dest_par.num;
		m_xvidFrame.par_height = m_vInfo.dest_par.den;
	} else if(parMode >= XVID_PAR_11_VGA && parMode <= XVID_PAR_169_NTSC){
		m_xvidFrame.par = parMode;
	}
	
	/* Force the right quantizer -- It is internally managed by RC plugins */
	m_xvidFrame.quant = 0;
	m_xvidFrame.bframe_threshold = 0;
	m_xvidFrame.vop_flags = vop_presets[vopQuality];
	m_xvidFrame.motion = motion_presets[meQuality];
	m_xvidFrame.input.csp = colorSpace;
	m_xvidFrame.type = XVID_TYPE_AUTO;
	m_xvidFrame.input.stride[0] = m_vInfo.res_out.width;

	bool bTrellis = m_pXmlPrefs->GetBoolean("videoenc.xvid.trellis");
	if(bTrellis) m_xvidFrame.vop_flags |= XVID_VOP_TRELLISQUANT;
	bool bHqAc = m_pXmlPrefs->GetBoolean("videoenc.xvid.hqac");
	if(bHqAc) m_xvidFrame.vop_flags |= XVID_VOP_HQACPRED;
	

	m_xvidFrame.bitstream = malloc(m_vInfo.res_out.width*m_vInfo.res_out.height*3); // big enough
	if(m_xvidFrame.bitstream == NULL) {
		logger_err(m_logModuleType, "Allocate output buffer failed!\n");
	}
	m_xvidFrame.length = -1;
	logger_info(m_logModuleType, "XVID encoder init successfully!\n");
	return true;
}

int CXvidEncode::EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast)
{
	int encodeRet = VIDEO_ENC_ERROR;
	if(!pFrameBuf) return VIDEO_ENC_END;				// End encoding

	m_xvidFrame.input.plane[0] = pFrameBuf;
	int ret = xvid_encore(m_pXvidHandle, XVID_ENC_ENCODE, &m_xvidFrame, NULL);
	if(ret > 0) {
		if(m_pOutStream) {
			int writeLen = m_pOutStream->Write((char*)m_xvidFrame.bitstream, ret);
			if(writeLen != 1) {
				logger_err(m_logModuleType, "write bitstream failed!\n");
				encodeRet = VIDEO_ENC_ERROR;
			} else {
				encodeRet = ++m_frameCount;
			}
		}
	} else {	// Error process
		switch(ret) {
		case 0:
			logger_err(m_logModuleType, "no output.\n");
			break;
		case XVID_ERR_VERSION:
			logger_err(m_logModuleType, "wrong version passed to core.\n");
			break;
		case XVID_ERR_END:
			logger_err(m_logModuleType, "End of stream reached before end of coding.\n");
			break;
		case XVID_ERR_FORMAT:
			logger_err(m_logModuleType, "the image subsystem reported the image had a wrong format.\n");
			break;
		}
		encodeRet = VIDEO_ENC_ERROR;
	}
	
	return encodeRet;
}

bool CXvidEncode::Stop()
{
	// Destroy encoder
	if(m_pXvidHandle) {
		xvid_encore(m_pXvidHandle, XVID_ENC_DESTROY, NULL, NULL);
		m_pXvidHandle = NULL;
	}

	if(m_xvidFrame.bitstream) {
		free(m_xvidFrame.bitstream);
		m_xvidFrame.bitstream = NULL;
	}

	return CVideoEncoder::Stop();
}

int CXvidEncode::getCpuAndEncInfo(int& threadNum)
{
	xvid_gbl_info_t xvid_gbl_info;
	int ret;

	memset(&xvid_gbl_info, 0, sizeof(xvid_gbl_info));
	xvid_gbl_info.version = XVID_VERSION;
	ret = xvid_global(NULL, XVID_GBL_INFO, &xvid_gbl_info, NULL);
	if (xvid_gbl_info.build != NULL) {
		logger_info(m_logModuleType, "xvidcore build version: %s\n", xvid_gbl_info.build);
	}
	//logger_info(m_logModuleType, "Bitstream version: %d.%d.%d\n", XVID_VERSION_MAJOR(xvid_gbl_info.actual_version), XVID_VERSION_MINOR(xvid_gbl_info.actual_version), XVID_VERSION_PATCH(xvid_gbl_info.actual_version));
	logger_info(m_logModuleType, "Detected CPU flags: \n");
	std::string cpuFlagStr;
	if (xvid_gbl_info.cpu_flags & XVID_CPU_ASM)
		cpuFlagStr += "ASM ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_MMX)
		cpuFlagStr += "MMX ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_MMXEXT)
		cpuFlagStr += "MMXEXT ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE)
		cpuFlagStr += "SSE ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE2)
		cpuFlagStr += "SSE2 ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE3)
		cpuFlagStr += "SSE3 ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE41)
		cpuFlagStr += "SSE41 ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_3DNOW)
		cpuFlagStr += "3DNOW ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_3DNOWEXT)
		cpuFlagStr += "3DNOWEXT ";
	if (xvid_gbl_info.cpu_flags & XVID_CPU_TSC)
		cpuFlagStr += "TSC ";
	cpuFlagStr += "\n";
	logger_info(m_logModuleType, "CPU Flags: %s", cpuFlagStr.c_str());
	if (threadNum <= 0)
		threadNum = xvid_gbl_info.num_threads;
	logger_info(m_logModuleType, "Detected %d cpus, using %d threads.\n", xvid_gbl_info.num_threads, threadNum);
	return ret;
}

CXvidEncode::~CXvidEncode(void)
{
	Stop();
}
