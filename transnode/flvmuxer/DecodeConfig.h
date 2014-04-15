#ifndef __DECODE_CONFIG_H
#define __DECODE_CONFIG_H

#include "SPS.h"
#include "PPS.h"

class ConfigDecoder
{
public:
	ConfigDecoder(void);
	~ConfigDecoder(void);

public:
	SPS m_sSPS;
	PPS m_sPPS;
	//sliceheader m_slh;
	static unsigned int sample_rates[];

private:

	unsigned int tempdata;
	unsigned int bufferpos;
	unsigned int leftbits;

	unsigned char* config;
	unsigned int configlength;
	enum Slice_Type 
	{
		SLICE_TYPE_P = 5,
		SLICE_TYPE_B,
		SLICE_TYPE_I,
		SLICE_TYPE_SP,
		SLICE_TYPE_SI,
	};

public:
	unsigned int get_audio_samplerate(const unsigned char* str);
	void read_config_sps(const unsigned char* str,const unsigned int len);
	void read_config_pps(const unsigned char* str,const unsigned int len);
	void read_slicehear(const unsigned char* str,const unsigned int len,unsigned char naltype,unsigned char nal_ref_idc);

private:
	void init();
	void attachdata();
	unsigned int read_unsigned_n(unsigned int num);
	unsigned int read_unsigned_golomb();
	int read_signed_golomb();

	bool more_rbsp_data();

	void vui_parameters();
	void hrd_parameters();

	void ref_pic_list_reordering();
	void pred_weight_table();
	void dec_ref_pic_marking(unsigned char nal_unit_type);
};
#endif