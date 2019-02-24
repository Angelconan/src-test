#pragma once
#ifndef _target_tracking_c_H_
#define _target_tracking_c_H_
#include "mbuffer.h"
#define distinguish_target
typedef struct candidate_list
{
	int candidate_picture_list[20][5];
	int *candidate_picture_in_ref_num[20];
	int *candidate_picture_active_zone[20][4];
	int candidate_picture_active_4x4_position[2];
	int pic_mv[4];
	short int candidate_4x4_position_array[640*480/16];
	short int candidate_mv_array[640 * 480 /16];
	int m, n, candidate_pic_x, candidate_pic_y, candidate_ref_list_num, reflash_num;
	int active_pic, ref_num, mvx, mvy, mb_num, macblock_num, picture_width, picture_height, vec1_x, vec1_y;
	int pic_4x4_num;
	int realloc_num;
	int statistics_x, statistics_y;
	int median_filter_mvx, median_filter_mvy;
}candi_list;
void candidate_one_frame_INIT(int picture_height, int picture_width);
void candidate_list_INIT(int ref_num, int *active_zone);
int candidate_judgment(int active_pic, int ref_num, int mvx, int mvy, int mb_num, int vec1_x, int vec1_y, int macblock_num, int picture_width, int picture_height);
void calculate_4x4_candidate(int picture_width);
void jump_over();
void get_pic_mv(int mvx, int mvy, int mb_num, int macblock_num);
int judgment_candidate_active_zone();
void save_4x4_active_zone();
int judgment_last_4x4(int macroblock_num, int mb_num, int pic_width, int pic_height);
void algorithm_candidate_active_zone();
void reflash_candidate_picture_list();
void output_candidate_active_zone();
int findout_candidate(int check_target);
void release_candidate_list();
void statistics_direction(int mvx, int mvy);
void fix_active_zone();
void median_filter();
void distinguish_part_location_uv(int i, int j, StorablePicture *p, int crop_top, int crop_bottom, int crop_left, int crop_right, FILE *p_out);
#endif