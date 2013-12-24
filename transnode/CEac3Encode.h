#ifndef _EAC3_ENCODE_H_
#define _EAC3_ENCODE_H_

#include "AudioEncode.h"
#include "eac3\enc_api.h"

#define MAX_IN_CHANNELS 6
#define NUM_SAMPLES 1536 /* Samples per frame */
#define ENC_FIO_MAX_OUTFRMWRDS  24624 /* Maximum words in output frame */

class CEac3Encode : public CAudioEncoder
{
public:
	CEac3Encode(const char* outFileName);
	~CEac3Encode(void);

	bool    Initialize();
	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen) ;	
	bool    Stop();

private:
	CEac3Encode();
	
	enc_instance*		m_pEncInstance;

	ddpi_enc_process_ip m_inparams;
	dlb_buffer			m_inputBuffer;
	void*				m_pInBufPtrs[MAX_IN_CHANNELS];

	ddpi_enc_process_op m_outparams;
    ddpi_enc_pgm_status m_encStatus;
	short               m_peak[11];
	dlb_buffer			m_outputBuffer;
    void*				m_pOutBufptrs[1];
	int32_t				m_outBitsBuf[2*ENC_FIO_MAX_OUTFRMWRDS];

	/* Global array use to store all parameters Parameter with ID N is stored at index N */
	//ddpi_enc_param		m_encParamsArr[DDPI_ENC_PARAMID_CONFIGQUEUE+1];
};

#endif