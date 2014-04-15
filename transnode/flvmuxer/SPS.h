#ifndef __SPS_H
#define __SPS_H

#define Extended_Sar 255

class SPS
{
public:
	SPS(void);
	~SPS(void);

public:
	unsigned char	sps_type;
	unsigned char	profile_idc;
	unsigned char	constraint_set0_flag;
	unsigned char	constraint_set1_flag;
	unsigned char	constraint_set2_flag;
	unsigned char	constraint_set3_flag;
	unsigned char	reserved_zero_4bits;
	unsigned char	level_idc;

	unsigned int	seq_parameter_set_id;

	unsigned int	chroma_format_idc;
	unsigned char	residual_colour_transform_flag;
	unsigned int	bit_depth_luma_minus8;
	unsigned int	bit_depth_chroma_minus8;
	unsigned char	qpprime_y_zero_transform_bypass_flag;
	unsigned char	seq_scaling_matrix_present_flag;
	unsigned char	seq_scaling_list_present_flag[8];

	unsigned int	log2_max_frame_num_minus4;
	unsigned int	pic_order_cnt_type;

	unsigned int	log2_max_pic_order_cnt_lsb_minus4;

	unsigned char	delta_pic_order_always_zero_flag;
	int				offset_for_non_ref_pic;
	int				offset_for_top_to_bottom_field;
	unsigned int	num_ref_frames_in_pic_order_cnt_cycle;
	int*			offset_for_ref_frame;

	unsigned int	num_ref_frames;
	unsigned char	gaps_in_frame_num_value_allowed_flag;
	unsigned int	pic_width_in_mbs_minus1;
	unsigned int	pic_height_in_map_units_minus1;
	unsigned char	frame_mbs_only_flag;

	unsigned char	mb_adaptive_frame_field_flag;

	unsigned char	direct_8x8_inference_flag;
	unsigned char	frame_cropping_flag;

	unsigned int	frame_crop_left_offset;
	unsigned int	frame_crop_right_offset;
	unsigned int	frame_crop_top_offset;
	unsigned int	frame_crop_bottom_offset;

	//vui
	unsigned char	vui_parameters_present_flag;

	unsigned char	aspect_ratio_info_present_flag;
	unsigned char	aspect_ratio_idc;
	unsigned short	sar_width;
	unsigned short	sar_height;

	unsigned char	overscan_info_present_flag;
	unsigned char	overscan_appropriate_flag;
	unsigned char	video_signal_type_present_flag;
	unsigned char	video_format;
	unsigned char	video_full_range_flag;
	unsigned char	colour_description_present_flag;
	unsigned char	colour_primaries;
	unsigned char	transfer_characteristics;
	unsigned char	matrix_coefficients;

	unsigned char	chroma_loc_info_present_flag;
	unsigned int	chroma_sample_loc_type_top_field;
	unsigned int	chroma_sample_loc_type_bottom_field;
	unsigned char	timing_info_present_flag;
	unsigned int	num_units_in_tick;
	unsigned int	time_scale;
	unsigned char	fixed_frame_rate_flag;

	unsigned char	nal_hrd_parameters_present_flag;
	unsigned char	vcl_hrd_parameters_present_flag;
	unsigned char	low_delay_hrd_flag;
	unsigned char	pic_struct_present_flag;

	unsigned char	bitstream_restriction_flag;
	unsigned char	motion_vectors_over_pic_boundaries_flag;
	unsigned int	max_bytes_per_pic_denom;
	unsigned int	max_bits_per_mb_denom;
	unsigned int	log2_max_mv_length_horizontal;
	unsigned int	log2_max_mv_length_vertical;
	unsigned int	num_reorder_frames;
	unsigned int	max_dec_frame_buffering;

	//hrd
	unsigned int	cpb_cnt_minus1;
	unsigned char	bit_rate_scale;
	unsigned char	cpb_size_scale;
	unsigned int*	bit_rate_value_minus1;
	unsigned int*	cpb_size_value_minus1;
	unsigned char*	cbr_flag;
	unsigned char	initial_cpb_removal_delay_length_minus1;
	unsigned char	cpb_removal_delay_length_minus1;
	unsigned char	dpb_output_delay_length_minus1;
	unsigned char	time_offset_length;
};

#endif