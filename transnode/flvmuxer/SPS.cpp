#include "SPS.h"
#include <iostream>

SPS::SPS(void)
{
	sps_type = -1;
	profile_idc = -1;
	constraint_set0_flag = -1;
	constraint_set1_flag = -1;
	constraint_set2_flag = -1;
	constraint_set3_flag = -1;
	reserved_zero_4bits = -1;
	level_idc = -1;

	seq_parameter_set_id = -1;

	chroma_format_idc = -1;
	residual_colour_transform_flag = -1;
	bit_depth_luma_minus8 = -1;
	bit_depth_chroma_minus8 = -1;
	qpprime_y_zero_transform_bypass_flag = -1;
	seq_scaling_matrix_present_flag = -1;

	log2_max_frame_num_minus4 = -1;
	pic_order_cnt_type = -1;

	log2_max_pic_order_cnt_lsb_minus4 = -1;

	delta_pic_order_always_zero_flag = -1;
	offset_for_non_ref_pic = -1;
	offset_for_top_to_bottom_field = -1;
	num_ref_frames_in_pic_order_cnt_cycle = -1;
	offset_for_ref_frame = NULL;

	num_ref_frames = -1;
	gaps_in_frame_num_value_allowed_flag = -1;
	pic_width_in_mbs_minus1 = -1;
	pic_height_in_map_units_minus1 = -1;
	frame_mbs_only_flag = -1;

	mb_adaptive_frame_field_flag = -1;

	direct_8x8_inference_flag = -1;
	frame_cropping_flag = -1;

	frame_crop_left_offset = -1;
	frame_crop_right_offset = -1;
	frame_crop_top_offset = -1;
	frame_crop_bottom_offset = -1;

	vui_parameters_present_flag = -1;

	aspect_ratio_info_present_flag = -1;
	aspect_ratio_idc = -1;
	sar_width = -1;
	sar_height = -1;

	overscan_info_present_flag = -1;
	overscan_appropriate_flag = -1;
	video_signal_type_present_flag = -1;
	video_format = -1;
	video_full_range_flag = -1;
	colour_description_present_flag = -1;
	colour_primaries = -1;
	transfer_characteristics = -1;
	matrix_coefficients = -1;

	chroma_loc_info_present_flag = -1;
	chroma_sample_loc_type_top_field = -1;
	chroma_sample_loc_type_bottom_field = -1;
	timing_info_present_flag = -1;
	num_units_in_tick = -1;
	time_scale = -1;
	fixed_frame_rate_flag = -1;

	nal_hrd_parameters_present_flag = -1;
	vcl_hrd_parameters_present_flag = -1;
	low_delay_hrd_flag = -1;
	pic_struct_present_flag = -1;

	bitstream_restriction_flag = -1;
	motion_vectors_over_pic_boundaries_flag = -1;
	max_bytes_per_pic_denom = -1;
	max_bits_per_mb_denom = -1;
	log2_max_mv_length_horizontal = -1;
	log2_max_mv_length_vertical = -1;
	num_reorder_frames = -1;
	max_dec_frame_buffering = -1;

	cpb_cnt_minus1 = -1;
	bit_rate_scale = -1;
	cpb_size_scale = -1;
	bit_rate_value_minus1 = NULL;
	cpb_size_value_minus1 = NULL;
	cbr_flag = NULL;
	initial_cpb_removal_delay_length_minus1 = -1;
	cpb_removal_delay_length_minus1 = -1;
	dpb_output_delay_length_minus1 = -1;
	time_offset_length = -1;
}

SPS::~SPS(void)
{
	if (offset_for_ref_frame)
	{
		delete[] offset_for_ref_frame;
		offset_for_ref_frame = NULL;
	}
	if (bit_rate_value_minus1)
	{
		delete[] bit_rate_value_minus1;
		bit_rate_value_minus1 = NULL;
	}
	if (cpb_size_value_minus1)
	{
		delete[] cpb_size_value_minus1;
		cpb_size_value_minus1 = NULL;
	}
	if (cbr_flag)
	{
		delete[] cbr_flag;
		cbr_flag = NULL;
	}
}
