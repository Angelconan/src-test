#pragma once
#ifndef _target_tracking_H_
#define _target_tracking_H_
	typedef class candidate_list
	{
	public:
		int candidate_picture_list[20][5];
		int *candidate_picture_in_ref_num[20];
		int *candidate_picture_active_zone[20][4];
		int **candidate_picture_active_4x4_position;
		int pic_mv[4];
		int **candidate_4x4_position_array;
		candidate_list(int ref_num, int active_zone[]);
		~candidate_list();
		bool candidate_judgment(int active_pic, int ref_num, int mvx, int mvy, int mb_num, int macblock_num, int picture_width, int picture_height);
		void calculate_4x4_candidate(int picture_width);
		void jump_over();
		void get_pic_mv(int mvx, int mvy, int mb_num, int macblock_num);
		bool judgment_candidate_active_zone();
		void save_4x4_active_zone();
		bool judgment_last_4x4(int macroblock_num, int mb_num, int pic_width, int pic_height);
		void algorithm_candidate_active_zone();
		void reflash_candidate_picture_list();
		void output_candidate_active_zone();
		bool findout_candidate(int check_target);
	private:
		int m, n = -1, candidate_pic_x, candidate_pic_y, candidate_ref_list_num;
	}candi_list;
#endif
