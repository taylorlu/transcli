#include "bitconfig.h"

#ifdef HAVE_LIBEAC3
#include "CEac3Encode.h"
#include "MEvaluater.h"
#include "StreamOutput.h"

#if defined(WIN32)
//#pragma comment(lib,"eac3enc.lib")
#endif

/* Datarates to set */
static int DolbyBitrateArray[6]={96,192,256,384,448,640};

/* Print Parameters supported in DD and DD+ encodng mode with valid ranges */
static void printDDplus_DDparams( ddpi_enc_query_op query_op)
{
    int i,j;

    /* Print paramteres supported by for DD+ */
    fprintf(stderr, "DD+  -  %d Parameters supported\nParameter ID\t\tValid Values\n",query_op.p_paramtab[DDPI_ENC_MODE_DDP]->nparams);
    for (i=0;i<query_op.p_paramtab[0]->nparams;i++)
    {
        fprintf(stderr, "\t%d\t\t",query_op.p_paramtab[DDPI_ENC_MODE_DDP]->p_params[i].id);
        if (query_op.p_paramtab[0]->p_params[i].range_type == DDPI_ENC_PARAMRANGE_LIST)
        {
            for (j=0;j<query_op.p_paramtab[0]->p_params[i].num_range_vals;j++)
            {
                fprintf(stderr, "%d, ",query_op.p_paramtab[DDPI_ENC_MODE_DDP]->p_params[i].p_range_vals[j]);
            }
            fprintf(stderr, "\n");
        }
        else
        {
            fprintf(stderr, "%d  -  %d \n",query_op.p_paramtab[DDPI_ENC_MODE_DDP]->p_params[i].min_val,query_op.p_paramtab[DDPI_ENC_MODE_DDP]->p_params[i].max_val);
        }

    }
    /* Print paramteres supported by for DD */
    fprintf(stderr, "\nDD  -  %d Parameters supported\nParameter ID\t\tValid Values\n",query_op.p_paramtab[DDPI_ENC_MODE_DD]->nparams);
    for (i=0;i<query_op.p_paramtab[DDPI_ENC_MODE_DD]->nparams;i++)
    {
        fprintf(stderr, "\t%d\t\t",query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].id);
        if (query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].range_type == DDPI_ENC_PARAMRANGE_LIST)
        {
            for (j=0;j<query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].num_range_vals;j++)
            {
                fprintf(stderr, "%d, ",query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].p_range_vals[j]);
            }
            fprintf(stderr, "\n");
        }
        else
        {
            fprintf(stderr, "%d  -  %d \n",query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].min_val,query_op.p_paramtab[DDPI_ENC_MODE_DD]->p_params[i].max_val);
        }
    }
}

/* enc_printerror: prints error messages */
static void enc_printerror(const char* p_msg)
{
    /* this can be replaced with customized error handling functionality */
    fprintf(stderr, p_msg);
    fprintf(stderr, "\n");
}

/* enc_error: single error handling function for all subroutine errors */
static void enc_error(int iserror, const char* p_msg, const char *p_file, int line, void *p_cbdata)
{
    char	p_errmsg[DDPI_MAX_ERROR_CB_STRLEN];

    /* this can be replaced with customized error handling functionality */
    if (iserror)
    {
        /* build error string */
        sprintf(p_errmsg,"\n\nFATAL ERROR:  %s\n\nError occurred in:\n%s (line %d), stream %d\n\n", 
            p_msg, p_file, line, (int)p_cbdata);
    }
    else
    {
        /* build warning string */
        sprintf(p_errmsg,"\nWARNING: %s (stream %d)\n", p_msg, (int)p_cbdata);
    }

    enc_printerror(p_errmsg);
}

CEac3Encode::CEac3Encode(const char* outFileName) : CAudioEncoder(outFileName), m_pEncInstance(NULL)
{
	m_logModuleType = LOGM_TS_AE_EAC3;
	memset(&m_inparams, 0, sizeof(m_inparams));
	memset(&m_inputBuffer, 0, sizeof(m_inputBuffer));
	for(int i=0; i<MAX_IN_CHANNELS; ++i) {
		m_pInBufPtrs[i] = NULL;
	}
	memset(&m_outparams, 0, sizeof(m_outparams));
	memset(&m_encStatus, 0, sizeof(m_encStatus));
	for(int i=0; i<11; ++i) {
		m_peak[i] = 0;
	}
	memset(&m_outputBuffer, 0, sizeof(m_outputBuffer));
    m_pOutBufptrs[0] = NULL;
	memset(m_outBitsBuf, 0, 2*ENC_FIO_MAX_OUTFRMWRDS);
}

CEac3Encode::~CEac3Encode(void)
{
}

bool CEac3Encode::Initialize()
{
	if(!m_strOutFile.empty()) {
		m_pStreamOut = getStreamOutput(CStreamOutput::FILE_STREAM);
		if(!m_pStreamOut->Open(m_strOutFile.c_str())) {
			logger_err(m_logModuleType, "Create output file failed.\n");
			return false;
		}
	}

	/* Declarations for variables used for ddpi_enc_query*/
    ddpi_enc_query_op query_op;

    /* Declarations for variables used for ddpi_enc_querymem*/
    ddpi_enc_querymem_op  querymem_op;
    ddpi_enc_memory_pool mem_pool;
    ddpi_enc_static_params staticparams;
    ddpi_enc_pgm_cfg pgm_cfg;

	int err = ddpi_enc_query(&query_op);
	/* Print all parameters their valid ranges for DD+ and DD */
    printDDplus_DDparams(query_op);

	/*Set up static parameter structure */
    for (int i=0; i<DDPI_ENC_MODE_COUNT; i++)
    {
        staticparams.p_modes[i]=query_op.p_modes[i]; /* Enabling all modes supported by the library*/
    }
    staticparams.max_pgms=1;
    staticparams.p_modes[DDPI_ENC_MODE_DDP]=1; /* Make sure DDP mode is enabled*/
    staticparams.p_modes[DDPI_ENC_MODE_DD]=1; /* Make sure DD mode is enabled*/
    staticparams.p_pgm_cfgs = &pgm_cfg;
	// Encoding mode (0:DD+, 1:DD)
	int encMode = m_pXmlPrefs->GetInt("audioenc.dolby.type");
    staticparams.p_pgm_cfgs->init_mode = encMode;
    staticparams.p_pgm_cfgs->max_chans = DDPI_ENC_MAXCHANS; /* Set to default maximum */
    staticparams.p_pgm_cfgs->max_wordsperframeset=ENC_FIO_MAX_OUTFRMWRDS;

    querymem_op.p_pool_reqs = &mem_pool;

    /****  API CALL: Query for memory */
    err = ddpi_enc_querymem(&staticparams, &querymem_op);

    mem_pool.p_mem = (void *)malloc(querymem_op.p_pool_reqs->size);
    if (!mem_pool.p_mem) {
		logger_err(m_logModuleType, "Error while allocating memory\n");
    }

	/**** API CALL: Open Encoder */
    err = ddpi_enc_open(&m_pEncInstance, &staticparams, &mem_pool, &enc_error, 0);

    m_inputBuffer.data_type = DLB_BUFFER_SHORT_16;
    m_inputBuffer.nchannel = m_aInfo.out_channels; /* Number of channels in the buffer */
    m_inputBuffer.nstride = m_aInfo.out_channels; /* If input is not interleaved, this will be 1*/
	m_inputBuffer.ppdata = m_pInBufPtrs;
	if(m_aInfo.out_channels == 2) {		// Default is 5.1 channel, if input is 2, then change to mode 2
		m_pXmlPrefs->SetInt("audioenc.dolby.acmode", 2);
	} else if (m_aInfo.out_channels == 1) {
		m_pXmlPrefs->SetInt("audioenc.dolby.acmode", 1);
	}

	/* Set input parameters and buffers */
	m_inparams.npgms = 1;
    m_inparams.nsamples = NUM_SAMPLES;
    m_inparams.p_pgm_pcm_buffers = &m_inputBuffer;

	 /* Set output parameters and buffers */
	m_pOutBufptrs[0] = m_outBitsBuf;
	m_outparams.p_status = &m_encStatus;
	m_outparams.bitstream.ppdata = m_pOutBufptrs;
    m_outparams.bitstream.nchannel=1;
    m_outparams.bitstream.data_type = DLB_BUFFER_OCTET_UNPACKED;
    m_outparams.bitstream.nstride = 1;
    m_outparams.noctets = 0;
    m_outparams.p_status->p_peak = m_peak;

	// Support change 4 params
	const int paramsCount = 7;
	ddpi_enc_param enc_params[paramsCount];

	// Bitrate
	enc_params[0].id = DDPI_ENC_PARAMID_DATARATE;
	int bitrateCode = m_pXmlPrefs->GetInt("audioenc.dolby.bitrate");
	if(bitrateCode > 4) bitrateCode = 4;
	if(bitrateCode < 0) bitrateCode = 0;
	enc_params[0].val = DolbyBitrateArray[bitrateCode];

	// Channel mode
	enc_params[1].id = DDPI_ENC_PARAMID_ACMOD;
	int acMode = m_pXmlPrefs->GetInt("audioenc.dolby.acmode");
	enc_params[1].val = acMode;

	// Enable LFE
	enc_params[2].id = DDPI_ENC_PARAMID_LFE;
	int enableLFE = 1;
	if(acMode < 3) enableLFE = 0;
	enc_params[2].val = enableLFE;

	// Bitstream mode
	enc_params[3].id = DDPI_ENC_PARAMID_BSMOD;
	int bsMode = m_pXmlPrefs->GetInt("audioenc.dolby.bsMode");
	enc_params[3].val = bsMode;

	// Room type
	if(acMode == 0) {	// 1+1 channel mode
		enc_params[4].id = DDPI_ENC_PARAMID_ROOMTYP2;
	} else {
		enc_params[4].id = DDPI_ENC_PARAMID_ROOMTYP;
	}
	int roomType = m_pXmlPrefs->GetInt("audioenc.dolby.roomType");
	enc_params[4].val = roomType;

	// A/D convert type
	enc_params[5].id = DDPI_ENC_PARAMID_ADCONVTYP;
	int adType = m_pXmlPrefs->GetInt("audioenc.dolby.adType");
	enc_params[5].val = adType;

	// Peak Mixing level
	if(acMode == 0) {	// 1+1 channel mode
		enc_params[6].id = DDPI_ENC_PARAMID_MIXLEVEL2;
	} else {
		enc_params[6].id = DDPI_ENC_PARAMID_MIXLEVEL;
	}
	int mixLevel = m_pXmlPrefs->GetInt("audioenc.dolby.mixLevel");
	enc_params[6].val = mixLevel+80;

	err = ddpi_enc_setparams(m_pEncInstance, 0, paramsCount, enc_params);
    if (err) {
		logger_err(m_logModuleType, "Error while setting metadata = %d\n", err);
		return false;
    }

	m_frameLen = NUM_SAMPLES*m_aInfo.out_channels*2;
	m_bClosed = 0;
	m_bitrate /= 1000;
	return true;
}

int64_t CEac3Encode::EncodeBuffer(uint8_t* pAudioBuf, int bufLen)
{
	int encodeRet = 0;
	for (int i=0;i<m_aInfo.out_channels;i++)
    {
        m_inputBuffer.ppdata[i] = &pAudioBuf[i*2]; /* if input is not interleaved, this will input_buffer.ppdata[i] = &pcmaudio[i*NUM_SAMPLES]; */
    }
	int err = ddpi_enc_process(m_pEncInstance, &m_inparams, &m_outparams);
    if (err) {
		logger_err(m_logModuleType, "Dolby encoder encoding failed.\n");
		return -1;
    }

	if(m_pStreamOut && m_pStreamOut->Write((char*)m_outparams.bitstream.ppdata[0], m_outparams.noctets) <= 0) {
		logger_err(m_logModuleType, "Write file failed.\n");
		return -1;
	}

	m_encodedBytes += m_outparams.noctets;
	encodeRet = m_encodedBytes;
	
	return encodeRet;
}

bool CEac3Encode::Stop()
{
	if(m_pEncInstance) {
		 ddpi_enc_close(m_pEncInstance);
		 m_pEncInstance = NULL;
	}

	if(m_pStreamOut) {
		destroyStreamOutput(m_pStreamOut);
		m_pStreamOut = NULL;
	}

	// Write handle is closed in trans worker
	if(m_fdRead > 0) {
		_close(m_fdRead);
		m_fdRead = -1;
	}

	return CAudioEncoder::Stop();
}

#endif
