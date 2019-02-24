/*
**This is a target_tracking program which active in the H.264(JM) decoder.
**designed by Wang Hao
**On 2017/12/25
*/

#include<stdlib.h>
#include<stdio.h>
#include<algorithm>
#include<bitset>
#include<math.h>
#include<string>
#include<iostream>

#include "target_tracking.h"
	using namespace std;
	candidate_list::candidate_list(int ref_num, int active_zone[])
	{
		for (size_t i = 0; i < 20; i++)
			for (size_t j = 0; j < 4; j++)
			{
				candidate_picture_list[i][j] = -1;
			}
		for (int i = 0; i < 20; i++)
		{
			candidate_picture_in_ref_num[i] = candidate_picture_list[i];
			candidate_picture_active_zone[i][0] = &candidate_picture_list[i][1];//active zone in picture x 
			candidate_picture_active_zone[i][1] = &candidate_picture_list[i][2];//active zone in picture y 
			candidate_picture_active_zone[i][2] = &candidate_picture_list[i][3];//active zone in picture width
			candidate_picture_active_zone[i][3] = &candidate_picture_list[i][4];//active zone in picture height
		}
		candidate_picture_active_4x4_position = new int *[m];
		for (int i = 0; i < m; i++)
			candidate_picture_active_4x4_position[i] = new int[4];
		candidate_4x4_position_array = new int *[m];
		for (int i = 0; i < m; i++)
			candidate_4x4_position_array[i] = new int[2];
		*candidate_picture_in_ref_num[0] = ref_num;
		*candidate_picture_active_zone[0] = active_zone;
		cout << "***candidate_list INIT complete!" << endl;
	};
	candidate_list::~candidate_list()
	{
		for (int i = 0; i < m; i++)
		{
			delete[] candidate_picture_active_4x4_position[i];
			delete[] candidate_4x4_position_array[i];
		}
		delete candidate_picture_active_4x4_position;
		delete candidate_4x4_position_array;
		cout << "***candidate_list dymanic array release!" << endl;
	}
	bool candidate_list::candidate_judgment(int active_pic, int ref_num, int mvx, int mvy, int mb_num, int macblock_num, int picture_width, int picture_height)
	{
		if (findout_candidate(ref_num))
		{
			cout << "***I find the ref" << ref_num << "which I need" << candidate_ref_list_num << " in the candidate_list" << endl;
			get_pic_mv(mvx, mvy, mb_num, macblock_num);
			calculate_4x4_candidate(picture_width);
			if (judgment_candidate_active_zone())
			{
				save_4x4_active_zone();
				if (judgment_last_4x4(macblock_num, mb_num, picture_width, picture_height))
				{
					cout << "***I'm last P4x4 block£¡" << endl;
				}
			}
		}
		else
		{
			jump_over();
			return 0;
		}

	}
	bool candidate_list::findout_candidate(int check_target)
	{
		for (int var = 0; var < 20; var++)
		{
			if (check_target == *candidate_picture_in_ref_num[var])
			{
				candidate_ref_list_num = var;
				return 1;
			}
		}
		return 0;
	}
	void candidate_list::jump_over()
	{
		cout << "***I didn't use the ref_pic in the candidate_list!" << endl;
	}
	void candidate_list::get_pic_mv(int mvx, int mvy, int mb_num, int macblock_num)
	{
		pic_mv[0] = mvx;
		pic_mv[1] = mvy;
		pic_mv[2] = mb_num;
		pic_mv[3] = macblock_num;
	}
	void candidate_list::calculate_4x4_candidate(int picture_width)
	{
		candidate_pic_x = pic_mv[3] * 16 % picture_width + (((pic_mv[2] / 4) % 2) * 16) + ((pic_mv[2] % 4) % 2) * 4 + pic_mv[0];
		candidate_pic_y = pic_mv[3] * 16 / picture_width + (((pic_mv[2] / 4) / 2) * 16) + ((pic_mv[2] % 4) / 2) * 4 + pic_mv[1];
		cout << "***The position in the ref of candidate_list is" << candidate_pic_x << "," << candidate_pic_y << endl;
	}
	bool candidate_list::judgment_candidate_active_zone()
	{
		if (candidate_pic_x >= *candidate_picture_active_zone[candidate_ref_list_num][0] && candidate_pic_y <= (*candidate_picture_active_zone[candidate_ref_list_num][0] + *candidate_picture_active_zone[candidate_ref_list_num][2] - 4))
		{
			if (candidate_pic_y >= *candidate_picture_active_zone[candidate_ref_list_num][1] && candidate_pic_y <= (*candidate_picture_active_zone[candidate_ref_list_num][1] + *candidate_picture_active_zone[candidate_ref_list_num][3] - 4))
			{
				n++;
				return 1;
			}
		}
		return 0;
	}
	void candidate_list::save_4x4_active_zone()
	{
		candidate_4x4_position_array[n][0] = candidate_pic_x;//(candidate_pic_y / 16)*(picture_width / 16) + candidate_pic_x / 16;
		candidate_4x4_position_array[n][1] = candidate_pic_y;//(candidate_pic_y%16)/4+(candidate_pic_x%16)/4
	}
	bool candidate_list::judgment_last_4x4(int macroblock_num, int mb_num, int pic_width, int pic_height)
	{
		if ((macroblock_num + 1) == (pic_width / 16)*(pic_height / 16))
		{
			if (mb_num == 15)
			{
				algorithm_candidate_active_zone();
			}
		}
	}
	void candidate_list::algorithm_candidate_active_zone()
	{

	}
	void candidate_list::reflash_candidate_picture_list()
	{

	}
	void candidate_list::output_candidate_active_zone()
	{

	}
