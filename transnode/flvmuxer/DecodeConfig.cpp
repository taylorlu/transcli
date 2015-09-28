#include "DecodeConfig.h"
#include <string.h>
#include <iostream>
#include <math.h>
using namespace std;

ConfigDecoder::ConfigDecoder(void)
{
	config = NULL;
	init();
}

ConfigDecoder::~ConfigDecoder(void)
{
	if (config)
	{
		delete[] config;
		config = NULL;
	}
}

unsigned int ConfigDecoder::sample_rates[] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000
};

unsigned int ConfigDecoder::get_audio_samplerate( const unsigned char* str )
{
	unsigned short temp;
	temp = 0;
	temp |= str[0] << 8;
	temp |= str[1];
	temp = temp << 5;
	temp = temp >> 12;
	return sample_rates[temp];
}

void ConfigDecoder::read_config_sps( const unsigned char* str,const unsigned int len)
{
	init();
	config = new unsigned char[len];
	memcpy(config,str,len);
	configlength = len;
	attachdata();

	m_sSPS.sps_type = read_unsigned_n(8);
	m_sSPS.profile_idc = read_unsigned_n(8);
	m_sSPS.constraint_set0_flag = read_unsigned_n(1);
	m_sSPS.constraint_set1_flag = read_unsigned_n(1);
	m_sSPS.constraint_set2_flag = read_unsigned_n(1);
	m_sSPS.constraint_set3_flag = read_unsigned_n(1);
	m_sSPS.reserved_zero_4bits = read_unsigned_n(4);
	m_sSPS.level_idc = read_unsigned_n(8);
	m_sSPS.seq_parameter_set_id = read_unsigned_golomb();
	if( m_sSPS.profile_idc == 100 || m_sSPS.profile_idc == 110 ||
		m_sSPS.profile_idc == 122 || m_sSPS.profile_idc == 144)
	{
		m_sSPS.chroma_format_idc = read_unsigned_golomb();
		if (m_sSPS.chroma_format_idc == 3)
		{
			m_sSPS.residual_colour_transform_flag = read_unsigned_n(1);
		}
		m_sSPS.bit_depth_luma_minus8 = read_unsigned_golomb();
		m_sSPS.bit_depth_chroma_minus8 = read_unsigned_golomb();
		m_sSPS.qpprime_y_zero_transform_bypass_flag = read_unsigned_n(1);
		m_sSPS.seq_scaling_matrix_present_flag = read_unsigned_n(1);
		if (m_sSPS.seq_scaling_matrix_present_flag)
		{
			for (int i = 0; i < 8; i ++)
			{
				m_sSPS.seq_scaling_list_present_flag[i] = read_unsigned_n(1);
				if (m_sSPS.seq_scaling_list_present_flag[i])
				{
					if (i < 6)
					{
					}
					else
					{
					}
				}
			}
		}
	}
	m_sSPS.log2_max_frame_num_minus4 = read_unsigned_golomb();
	m_sSPS.pic_order_cnt_type = read_unsigned_golomb();
	if (m_sSPS.pic_order_cnt_type == 0)
	{
		m_sSPS.log2_max_pic_order_cnt_lsb_minus4 = read_unsigned_golomb();
	}
	else if (m_sSPS.pic_order_cnt_type == 1)
	{
		m_sSPS.delta_pic_order_always_zero_flag = read_unsigned_n(1);
		m_sSPS.offset_for_non_ref_pic = read_signed_golomb();
		m_sSPS.offset_for_top_to_bottom_field = read_signed_golomb();
		m_sSPS.num_ref_frames_in_pic_order_cnt_cycle = read_unsigned_golomb();
		m_sSPS.offset_for_ref_frame = new int[m_sSPS.num_ref_frames_in_pic_order_cnt_cycle];
		for (int i = 0; i < m_sSPS.num_ref_frames_in_pic_order_cnt_cycle; i ++)
		{
			m_sSPS.offset_for_ref_frame[i] = read_signed_golomb();
		}
	}
	m_sSPS.num_ref_frames = read_unsigned_golomb();
	m_sSPS.gaps_in_frame_num_value_allowed_flag = read_unsigned_n(1);
	m_sSPS.pic_width_in_mbs_minus1 = read_unsigned_golomb();
	m_sSPS.pic_height_in_map_units_minus1 = read_unsigned_golomb();
	m_sSPS.frame_mbs_only_flag = read_unsigned_n(1);
	if (!m_sSPS.frame_mbs_only_flag)
	{
		m_sSPS.mb_adaptive_frame_field_flag = read_unsigned_n(1);
	}
	m_sSPS.direct_8x8_inference_flag = read_unsigned_n(1);
	m_sSPS.frame_cropping_flag = read_unsigned_n(1);
	if (m_sSPS.frame_cropping_flag)
	{
		m_sSPS.frame_crop_left_offset = read_unsigned_golomb();
		m_sSPS.frame_crop_right_offset = read_unsigned_golomb();
		m_sSPS.frame_crop_top_offset = read_unsigned_golomb();
		m_sSPS.frame_crop_bottom_offset = read_unsigned_golomb();
	}
	m_sSPS.vui_parameters_present_flag = read_unsigned_n(1);
	if (m_sSPS.vui_parameters_present_flag)
	{
		vui_parameters();
	}
}

void ConfigDecoder::read_config_pps( const unsigned char* str,const unsigned int len )
{
	init();
	config = new unsigned char[len];
	memcpy(config,str,len);
	configlength = len;
	attachdata();

	m_sPPS.pps_type = read_unsigned_n(8);
	m_sPPS.pic_parameter_set_id = read_unsigned_golomb();
	m_sPPS.seq_parameter_set_id = read_unsigned_golomb();
	m_sPPS.entropy_coding_present_flag = read_unsigned_n(1);
	m_sPPS.pic_order_present_flag = read_unsigned_n(1);
	m_sPPS.num_slice_groups_minus1 = read_unsigned_golomb();
	if (m_sPPS.num_slice_groups_minus1 > 0)
	{
		m_sPPS.slice_group_map_type = read_unsigned_golomb();
		if (m_sPPS.slice_group_map_type == 0)
		{
			m_sPPS.run_length_minus1 = new unsigned int[m_sPPS.num_slice_groups_minus1 + 1];
			for(int i = 0; i < m_sPPS.num_slice_groups_minus1 + 1; i ++)
			{
				m_sPPS.run_length_minus1[i] = read_unsigned_golomb();
			}
		}
		else if (m_sPPS.slice_group_map_type == 2)
		{
			m_sPPS.top_left = new unsigned int[m_sPPS.num_slice_groups_minus1 + 1];
			m_sPPS.bottom_right = new unsigned int[m_sPPS.num_slice_groups_minus1 + 1];
			for (int i = 0; i < m_sPPS.num_slice_groups_minus1 + 1; i ++)
			{
				m_sPPS.top_left[i] = read_unsigned_golomb();
				m_sPPS.bottom_right[i] = read_unsigned_golomb();
			}
		}
		else if (m_sPPS.slice_group_map_type == 3 || m_sPPS.slice_group_map_type == 4 || m_sPPS.slice_group_map_type == 5)
		{
			m_sPPS.slice_group_change_direction_flag = read_unsigned_n(1);
			m_sPPS.slice_group_change_rate_minus1 = read_unsigned_golomb();
		}
		else if (m_sPPS.slice_group_map_type == 6)
		{
			m_sPPS.pic_size_in_map_units_minus1 = read_unsigned_golomb();
			m_sPPS.slice_group_id = new unsigned int[m_sPPS.pic_size_in_map_units_minus1 + 1];
			for (int i = 0; i < m_sPPS.pic_size_in_map_units_minus1 + 1; i ++)
			{
				m_sPPS.slice_group_id[i] = read_unsigned_n(ceil(log10((double)m_sPPS.num_slice_groups_minus1 + 1) / log10(2.)));
			}
		}
	}
	m_sPPS.num_ref_idx_10_active_minus1 = read_unsigned_golomb();
	m_sPPS.num_ref_idx_11_active_minus1 = read_unsigned_golomb();
	m_sPPS.weighted_pred_flag = read_unsigned_n(1);
	m_sPPS.weighted_bipred_idc = read_unsigned_n(2);
	m_sPPS.pic_init_qp_minus26 = read_signed_golomb();
	m_sPPS.pic_init_qs_minus26 = read_signed_golomb();
	m_sPPS.chroma_qp_index_offset = read_signed_golomb();
	m_sPPS.deblocking_filter_control_present_flag = read_unsigned_n(1);
	m_sPPS.constrained_intra_pred_flag = read_unsigned_n(1);
	m_sPPS.redundant_pic_cnt_present_flag = read_unsigned_n(1);
	if (more_rbsp_data())
	{
		m_sPPS.transform_8x8_mode_flag = read_unsigned_n(1);
		m_sPPS.pic_scaling_matrix_present_flag = read_unsigned_n(1);
		if (m_sPPS.pic_scaling_matrix_present_flag)
		{
			m_sPPS.pic_scaling_list_present_flag = new unsigned char[6 + 2 * m_sPPS.transform_8x8_mode_flag];
			for (int i = 0; i < 6 + 2 * m_sPPS.transform_8x8_mode_flag; i ++)
			{
				m_sPPS.pic_scaling_list_present_flag[i] = read_unsigned_n(1);
				if (m_sPPS.pic_scaling_list_present_flag[i])
				{
					if (i < 6)
					{
					}
					else
					{
					}
				}
			}
		}
		m_sPPS.second_chroma_qp_index_offset = read_signed_golomb();
	}
}

void ConfigDecoder::read_slicehear( const unsigned char* str,const unsigned int len,unsigned char naltype,unsigned char nal_ref_idc )
{
#if 0
	init();
	config = new unsigned char[len];
	memcpy(config,str,len);
	configlength = len;
	attachdata();

	m_slh.clear();
	m_slh.first_mb_in_slice = read_unsigned_golomb();
	m_slh.slice_type = read_unsigned_golomb();
	m_slh.pic_parameter_set_id = read_unsigned_golomb();
	m_slh.frame_num = read_unsigned_n(m_sps.log2_max_frame_num_minus4 + 4);
	if (!m_sps.frame_mbs_only_flag)
	{
		m_slh.field_pic_flag = read_unsigned_n(1);
		if (m_slh.field_pic_flag)
		{
			m_slh.bottom_field_flag = read_unsigned_n(1);
		}
	}
	if (naltype == 5)
	{
		m_slh.idr_pic_id = read_unsigned_golomb();
	}
	if (m_sps.pic_order_cnt_type == 0)
	{
		m_slh.pic_order_cnt_lsb = read_unsigned_n(m_sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
		if (m_pps.pic_order_present_flag && !m_slh.field_pic_flag)
		{
			m_slh.delta_pic_order_cnt_bottom = read_signed_golomb();
		}
	}
	if (m_sps.pic_order_cnt_type == 1 && !m_sps.delta_pic_order_always_zero_flag)
	{
		m_slh.delta_pic_order_cnt0 = read_signed_golomb();
		if (m_pps.pic_order_present_flag && !m_slh.field_pic_flag)
		{
			m_slh.delta_pic_order_cnt1 = read_signed_golomb();
		}
	}
	if (m_pps.redundant_pic_cnt_present_flag)
	{
		m_slh.redundant_pic_cnt = read_unsigned_golomb();
	}
	if (m_slh.slice_type == SLICE_TYPE_B)
	{
		m_slh.direct_spatial_mv_pred_flag = read_unsigned_n(1);
	}
	if (m_slh.slice_type == SLICE_TYPE_P || m_slh.slice_type == SLICE_TYPE_SP || m_slh.slice_type == SLICE_TYPE_B)
	{
		m_slh.num_ref_idx_active_override_flag = read_unsigned_n(1);
		if (m_slh.num_ref_idx_active_override_flag)
		{
			m_slh.num_ref_idx_10_active_minus1 = read_unsigned_golomb();
			if (m_slh.slice_type == SLICE_TYPE_B)
			{
				m_slh.num_ref_idx_11_active_minus1 = read_unsigned_golomb();
			}
		}
	}
	ref_pic_list_reordering();
	if ((m_pps.weighted_pred_flag && (m_slh.slice_type == SLICE_TYPE_P || m_slh.slice_type == SLICE_TYPE_SP)) || (m_pps.weighted_bipred_idc == 1 && m_slh.slice_type == SLICE_TYPE_B))
	{
		pred_weight_table();
	}
	if (nal_ref_idc)
	{
		dec_ref_pic_marking(naltype);
	}
#endif
}

void ConfigDecoder::init()
{
	if (config)
	{
		delete[] config;
		config = NULL;
	}
	tempdata = 0;
	bufferpos = 0;
	leftbits = 0;
	configlength = 0;
}

void ConfigDecoder::attachdata()
{
	unsigned int attachnum = 0;
	bool trans = false;
	attachnum = (32 - leftbits) / 8;
	if (attachnum + bufferpos >= configlength)
	{
		attachnum = configlength - bufferpos;
	}
	for (int i = bufferpos; i < bufferpos + attachnum; i ++)
	{
		tempdata |= config[i] << (24 - leftbits);
		if ((tempdata & 0x00ffffff) == 3 && !trans)
		{
			tempdata = tempdata & 0xffffff00;
			bufferpos ++;
			trans = true;
		}
		else
		{
			leftbits += 8;
			trans = false;
		}
	}
	bufferpos += attachnum;
}

unsigned int ConfigDecoder::read_unsigned_n( unsigned int num)
{
	unsigned int temp = 0;
	for (int i = 0; i < num; i ++)
	{
		attachdata();
		temp = temp << 1;
		temp |= tempdata >> 31;
		tempdata = tempdata << 1;
		leftbits--;
	}
	return temp;
}

unsigned int ConfigDecoder::read_unsigned_golomb()
{
	unsigned int count = 0;
	unsigned int temp = 0;
	while(!(tempdata >> 31))
	{
		if(leftbits <= 0)
		{
			break;
		}
		attachdata();
		count ++;
		tempdata = tempdata << 1;
		leftbits --;
	}
	count ++;
	temp = read_unsigned_n(count) - 1;
	return temp;
}

int ConfigDecoder::read_signed_golomb()
{
	unsigned int uint = read_unsigned_golomb();
	int temp = (uint + 1) / 2;
	if (uint % 2 == 0)
	{
		temp *= -1;
	}
	return temp;
}

bool ConfigDecoder::more_rbsp_data()
{
	if (tempdata == 0x80000000)
	{
		return false;
	}
	return true;
}

void ConfigDecoder::vui_parameters()
{
	m_sSPS.aspect_ratio_info_present_flag = read_unsigned_n(1);
	if (m_sSPS.aspect_ratio_info_present_flag)
	{
		m_sSPS.aspect_ratio_idc = read_unsigned_n(8);
		if (m_sSPS.aspect_ratio_idc == Extended_Sar)
		{
			m_sSPS.sar_width = read_unsigned_n(16);
			m_sSPS.sar_height = read_unsigned_n(16);
		}
	}
	m_sSPS.overscan_info_present_flag = read_unsigned_n(1);
	if (m_sSPS.overscan_info_present_flag)
	{
		m_sSPS.overscan_appropriate_flag = read_unsigned_n(1);
	}
	m_sSPS.video_signal_type_present_flag = read_unsigned_n(1);
	if (m_sSPS.video_signal_type_present_flag)
	{
		m_sSPS.video_format = read_unsigned_n(3);
		m_sSPS.video_full_range_flag = read_unsigned_n(1);
		m_sSPS.colour_description_present_flag = read_unsigned_n(1);
		if (m_sSPS.colour_description_present_flag)
		{
			m_sSPS.colour_primaries = read_unsigned_n(8);
			m_sSPS.transfer_characteristics = read_unsigned_n(8);
			m_sSPS.matrix_coefficients = read_unsigned_n(8);
		}
	}
	m_sSPS.chroma_loc_info_present_flag = read_unsigned_n(1);
	if (m_sSPS.chroma_loc_info_present_flag)
	{
		m_sSPS.chroma_sample_loc_type_top_field = read_unsigned_golomb();
		m_sSPS.chroma_sample_loc_type_bottom_field = read_unsigned_golomb();
	}
	m_sSPS.timing_info_present_flag = read_unsigned_n(1);
	if (m_sSPS.timing_info_present_flag)
	{
		m_sSPS.num_units_in_tick = read_unsigned_n(32);
		m_sSPS.time_scale = read_unsigned_n(32);
		m_sSPS.fixed_frame_rate_flag = read_unsigned_n(1);
	}
	m_sSPS.nal_hrd_parameters_present_flag = read_unsigned_n(1);
	if (m_sSPS.nal_hrd_parameters_present_flag)
	{
		hrd_parameters();
	}
	m_sSPS.vcl_hrd_parameters_present_flag = read_unsigned_n(1);
	if (m_sSPS.vcl_hrd_parameters_present_flag)
	{
		hrd_parameters();
	}
	if (m_sSPS.nal_hrd_parameters_present_flag || m_sSPS.vcl_hrd_parameters_present_flag)
	{
		m_sSPS.low_delay_hrd_flag = read_unsigned_n(1);
	}
	m_sSPS.pic_struct_present_flag = read_unsigned_n(1);
	m_sSPS.bitstream_restriction_flag = read_unsigned_n(1);
	if (m_sSPS.bitstream_restriction_flag)
	{
		m_sSPS.motion_vectors_over_pic_boundaries_flag = read_unsigned_n(1);
		m_sSPS.max_bytes_per_pic_denom = read_unsigned_golomb();
		m_sSPS.max_bits_per_mb_denom = read_unsigned_golomb();
		m_sSPS.log2_max_mv_length_horizontal = read_unsigned_golomb();
		m_sSPS.log2_max_mv_length_vertical = read_unsigned_golomb();
		m_sSPS.num_reorder_frames = read_unsigned_golomb();
		m_sSPS.max_dec_frame_buffering = read_unsigned_golomb();
	}
}

void ConfigDecoder::hrd_parameters()
{
	m_sSPS.cpb_cnt_minus1 = read_unsigned_golomb();
	m_sSPS.bit_rate_scale = read_unsigned_n(4);
	m_sSPS.cpb_size_scale = read_unsigned_n(4);
	m_sSPS.bit_rate_value_minus1 = new unsigned int[m_sSPS.cpb_cnt_minus1 + 1];
	m_sSPS.cpb_size_value_minus1 = new unsigned int[m_sSPS.cpb_cnt_minus1 + 1];
	m_sSPS.cbr_flag = new unsigned char[m_sSPS.cpb_cnt_minus1 + 1];
	for (int i = 0; i <= m_sSPS.cpb_cnt_minus1; i ++)
	{
		m_sSPS.bit_rate_value_minus1[i] = read_unsigned_golomb();
		m_sSPS.cpb_size_value_minus1[i] = read_unsigned_golomb();
		m_sSPS.cbr_flag[i] = read_unsigned_n(1);
	}
	m_sSPS.initial_cpb_removal_delay_length_minus1 = read_unsigned_n(5);
	m_sSPS.cpb_removal_delay_length_minus1 = read_unsigned_n(5);
	m_sSPS.dpb_output_delay_length_minus1 = read_unsigned_n(5);
	m_sSPS.time_offset_length = read_unsigned_n(5);
}
