#ifndef __PPS_H
#define __PPS_H

class PPS
{
public:
	PPS(void);
	~PPS(void);

public:
	unsigned char	pps_type;
	unsigned int	pic_parameter_set_id;
	unsigned int	seq_parameter_set_id;
	unsigned char	entropy_coding_present_flag;
	unsigned char	pic_order_present_flag;
	unsigned char	num_slice_groups_minus1;

	unsigned int	slice_group_map_type;
	unsigned int*	run_length_minus1;
	unsigned int*	top_left;
	unsigned int*	bottom_right;

	unsigned char	slice_group_change_direction_flag;
	unsigned int	slice_group_change_rate_minus1;
	unsigned int	pic_size_in_map_units_minus1;
	unsigned int*	slice_group_id;

	unsigned int	num_ref_idx_10_active_minus1;
	unsigned int	num_ref_idx_11_active_minus1;
	unsigned char	weighted_pred_flag;
	unsigned char	weighted_bipred_idc;
	int				pic_init_qp_minus26;
	int				pic_init_qs_minus26;
	int				chroma_qp_index_offset;
	unsigned char	deblocking_filter_control_present_flag;
	unsigned char	constrained_intra_pred_flag;
	unsigned char	redundant_pic_cnt_present_flag;

	unsigned char	transform_8x8_mode_flag;
	unsigned char	pic_scaling_matrix_present_flag;
	unsigned char*	pic_scaling_list_present_flag;
	int				second_chroma_qp_index_offset;
};

#endif