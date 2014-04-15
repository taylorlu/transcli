#include "PPS.h"
#include <iostream>

PPS::PPS(void)
{
	pps_type = -1;
	pic_parameter_set_id = -1;
	seq_parameter_set_id = -1;
	entropy_coding_present_flag = -1;
	pic_order_present_flag = -1;
	num_slice_groups_minus1 = -1;

	slice_group_map_type = -1;
	run_length_minus1 = NULL;
	top_left = NULL;
	bottom_right = NULL;

	slice_group_change_direction_flag = -1;
	slice_group_change_rate_minus1 = -1;
	pic_size_in_map_units_minus1 = -1;
	slice_group_id = NULL;

	num_ref_idx_10_active_minus1 = -1;
	num_ref_idx_11_active_minus1 = -1;
	weighted_pred_flag = -1;
	weighted_bipred_idc = -1;
	pic_init_qp_minus26 = -1;
	pic_init_qs_minus26 = -1;
	chroma_qp_index_offset = -1;
	deblocking_filter_control_present_flag = -1;
	constrained_intra_pred_flag = -1;
	redundant_pic_cnt_present_flag = -1;

	transform_8x8_mode_flag = -1;
	pic_scaling_matrix_present_flag = -1;
	pic_scaling_list_present_flag = NULL;
	second_chroma_qp_index_offset = -1;
}

PPS::~PPS(void)
{
	if (run_length_minus1)
	{
		delete[] run_length_minus1;
		run_length_minus1 = NULL;
	}
	if (top_left)
	{
		delete[] top_left;
		top_left = NULL;
	}
	if (bottom_right)
	{
		delete[] bottom_right;
		bottom_right = NULL;
	}
	if (slice_group_id)
	{
		delete[] slice_group_id;
		slice_group_id = NULL;
	}
	if (pic_scaling_list_present_flag)
	{
		delete[] pic_scaling_list_present_flag;
		pic_scaling_list_present_flag = NULL;
	}
}
