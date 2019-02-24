/*
**This is a target_tracking program which active in the H.264(JM) decoder.
**designed by Wang Hao
**On 2017/12/25
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include"yuv2rgb.h"

#if defined WIN32
#include <conio.h>
#endif

#include <assert.h>
#include "target_tracking_c.h"
//using namespace std;
candi_list track;
void candidate_list_INIT(int ref_num, int *active_zone)
{
	track.reflash_num = 0;
	track.n = -1;
	for (size_t i = 0; i < 20; i++)
		for (size_t j = 0; j < 5; j++)
		{
			track.candidate_picture_list[i][j] = -1;
		}
	for (int i = 0; i < 20; i++)
	{
		track.candidate_picture_in_ref_num[i] = track.candidate_picture_list[i];
		track.candidate_picture_active_zone[i][0] = &track.candidate_picture_list[i][1];//active zone in picture x 
		track.candidate_picture_active_zone[i][1] = &track.candidate_picture_list[i][2];//active zone in picture y 
		track.candidate_picture_active_zone[i][2] = &track.candidate_picture_list[i][3];//active zone in picture width
		track.candidate_picture_active_zone[i][3] = &track.candidate_picture_list[i][4];//active zone in picture height
	}
	track.candidate_picture_list[track.reflash_num][0] = ref_num;
	for (int i = 1; i<5; i++)
		track.candidate_picture_list[track.reflash_num][i] = active_zone[i-1];
};
void candidate_one_frame_INIT(int picture_height, int picture_width)
{
	track.realloc_num = 1;
	track.pic_4x4_num = (picture_height / 4)*(picture_width / 4);
	track.statistics_x = 0;
	track.statistics_y = 0;
}
void release_candidate_list()
{
	track.n = -1;
}
int candidate_judgment(int active_pic, int ref_num, int mvx, int mvy, int vec1_x, int vec1_y, int mb_num, int macblock_num, int picture_height, int picture_width)
{
	//printf("* * *active_pic=%d,ref_num=%d,mvx=%d,mvy=%d,mb_num=%d,macblock_num=%d,picture_width=%d,picture_height=%d\n",active_pic,ref_num,mvx,mvy,mb_num,macblock_num,picture_width,picture_height);
	track.active_pic = active_pic;
	track.ref_num = ref_num;
	track.mvx = mvx;
	track.mvy = mvy;
	track.mb_num = mb_num;
	track.macblock_num = macblock_num;
	track.picture_width = picture_width;
	track.picture_height = picture_height;
	track.vec1_x = vec1_x;
	track.vec1_y = vec1_y;
	if (findout_candidate(track.ref_num) && track.mb_num != 999)
	{
		get_pic_mv(track.mvx, track.mvy, track.mb_num, track.macblock_num);
		calculate_4x4_candidate(track.picture_width);
		if (judgment_candidate_active_zone())
		{
			statistics_direction(track.mvx, track.mvy);
			save_4x4_active_zone();
		}
		if (judgment_last_4x4(track.macblock_num, track.mb_num, track.picture_width, track.picture_height))
		{
			release_candidate_list();
		}
	}
	else if (track.active_pic == track.ref_num && track.mb_num != 999)
	{
		int x = track.macblock_num * 16 % picture_width;
		int y = (track.macblock_num * 16 / picture_width) * 16;
		for (int m = 0; m < track.n; m++)
		{
			if (track.candidate_4x4_position_array[0 + m * 2] == (x - 4) || track.candidate_4x4_position_array[1 + m * 2] == (y - 4))
			{
				track.candidate_4x4_position_array[0 + track.n * 2] = x + 16;
				track.candidate_4x4_position_array[1 + track.n * 2] = y + 16;
			}
		}
		if (judgment_last_4x4(track.macblock_num, track.mb_num, track.picture_width, track.picture_height))
		{
			release_candidate_list();
		}
	}
	else if (track.mb_num == 999)
	{
		int x = track.macblock_num * 16 % picture_width;
		int y = (track.macblock_num * 16 / picture_width) * 16;
		for (int i = 0; i<4; i++)
		{
			for (int j = 0; j<4; j++)
			{
				if ((x + i * 4) >= track.candidate_picture_list[track.candidate_ref_list_num][1] && (x + i * 4) <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3]))
				{
					if ((y + j * 4) >= track.candidate_picture_list[track.candidate_ref_list_num][2] && (y + j * 4) <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4]))
					{
						track.candidate_4x4_position_array[0 + track.n * 2] = x + i * 4;
						track.candidate_4x4_position_array[1 + track.n * 2] = y + j * 4;
						//	track.n++;
						//printf("BBBBBBBBB\n");
					}
				}
			}
		}
		if ((track.macblock_num + 1) == (track.picture_width / 16)*(track.picture_height / 16))
		{
			algorithm_candidate_active_zone();
			median_filter();
			reflash_candidate_picture_list();
			fix_active_zone();
			release_candidate_list();
		}
	}
	else
	{
		return 0;
	}
}
int findout_candidate(int check_target)
{
	for (int var = 0; var < 20; var++)
	{
		if (check_target == track.candidate_picture_list[var][0])
		{
			track.candidate_ref_list_num = var;
			return 1;
		}
	}

	return 0;
}
void get_pic_mv(int mvx, int mvy, int mb_num, int macblock_num)
{
	track.pic_mv[0] = mvx;
	track.pic_mv[1] = mvy;
	track.pic_mv[2] = mb_num;
	track.pic_mv[3] = macblock_num;
	//printf("\n %d %d %d %d \n", track.pic_mv[0], track.pic_mv[1], track.pic_mv[2], track.pic_mv[3]);
}
void calculate_4x4_candidate(int picture_width)
{
	int dx, dy;
	dx = track.vec1_x & 3; dy = track.vec1_y & 3;
	track.candidate_pic_x = track.pic_mv[3] * 16 % picture_width + (((track.pic_mv[2] / 4) % 2) * 16) + ((track.pic_mv[2] % 4) % 2) * 4 + track.pic_mv[0];
	track.candidate_pic_y = (track.pic_mv[3] * 16 / picture_width) * 16 + (((track.pic_mv[2] / 4) / 2) * 16) + ((track.pic_mv[2] % 4) / 2) * 4 + track.pic_mv[1];
	//	cout << "***The position in the ref of candidate_list is" << track.candidate_pic_x << "," << track.candidate_pic_y << endl;
	//printf("* * *The position in the ref of candidate_list (track.candidate_pic_x,track.candidate_pic_y) is (%d,%d)! ! !\n", track.candidate_pic_x, track.candidate_pic_y);
	track.candidate_pic_x = (track.vec1_x - dx) / 4;
	track.candidate_pic_y = (track.vec1_y - dy) / 4;
	//printf("* * *H.264 The position in the ref of candidate_list (track.candidate_pic_x,track.candidate_pic_y) is (%d,%d)! ! !\n", track.candidate_pic_x, track.candidate_pic_y);
}
int judgment_candidate_active_zone()
{
	//printf("%d %d %d %d %d %d\n", track.candidate_picture_list[track.candidate_ref_list_num][1], track.candidate_picture_list[track.candidate_ref_list_num][2], track.candidate_picture_list[track.candidate_ref_list_num][3], track.candidate_picture_list[track.candidate_ref_list_num][4], track.candidate_pic_x, track.candidate_pic_y);
	if (track.candidate_pic_x >= track.candidate_picture_list[track.candidate_ref_list_num][1] && track.candidate_pic_x <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3]))
	{
		if (track.candidate_pic_y >= track.candidate_picture_list[track.candidate_ref_list_num][2] && track.candidate_pic_y <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4]))
		{
			track.n++;
			//printf("md \n");
			return 1;
		}
	}
	return 0;
}
void save_4x4_active_zone()
{
	track.candidate_4x4_position_array[0 + track.n * 2] = track.pic_mv[3] * 16 % track.picture_width + (((track.pic_mv[2] / 4) % 2) * 8) + ((track.pic_mv[2] % 4) % 2) * 4;//(int)track.candidate_pic_x;//(candidate_pic_y / 16)*(picture_width / 16) + candidate_pic_x / 16;
	track.candidate_4x4_position_array[1 + track.n * 2] = (track.pic_mv[3] * 16 / track.picture_width) * 16 + (((track.pic_mv[2] / 4) / 2) * 8) + ((track.pic_mv[2] % 4) / 2) * 4;//(int)track.candidate_pic_y;//(candidate_pic_y%16)/4+(candidate_pic_x%16)/4
	track.candidate_mv_array[0 + track.n * 2] = track.mvx;
	track.candidate_mv_array[1 + track.n * 2] = track.mvy;
}
int judgment_last_4x4(int macroblock_num, int mb_num, int pic_width, int pic_height)
{
	if ((macroblock_num + 1) == (pic_width / 16)*(pic_height / 16))
	{
		//	printf("I didn't come in ?\n");
		if (mb_num == 15)
		{
			algorithm_candidate_active_zone();
			median_filter();
			reflash_candidate_picture_list();
			//		printf("last mb_num!\n");
			fix_active_zone();
			//	printf("fuck!");
			return 1;
		}
	}
	return 0;
}

void algorithm_candidate_active_zone()
{
	int middle_number;
	for (int i = 0; i<track.n; i++)
		for (int m = 0; m + 1<track.n; m++)
		{
			if (track.candidate_4x4_position_array[0 + m * 2]>track.candidate_4x4_position_array[0 + (m + 1) * 2])
			{
				middle_number = track.candidate_4x4_position_array[0 + m * 2];
				track.candidate_4x4_position_array[0 + m * 2] = track.candidate_4x4_position_array[0 + (m + 1) * 2];
				track.candidate_4x4_position_array[0 + (m + 1) * 2] = middle_number;
			}
		}
	for (int i = 0; i<track.n; i++)
		for (int m = 0; m + 2<track.n; m++)
		{
			if (track.candidate_4x4_position_array[1 + m * 2]>track.candidate_4x4_position_array[1 + (m + 1) * 2])
			{
				middle_number = track.candidate_4x4_position_array[1 + m * 2];
				track.candidate_4x4_position_array[1 + m * 2] = track.candidate_4x4_position_array[1 + (m + 1) * 2];
				track.candidate_4x4_position_array[1 + (m + 1) * 2] = middle_number;
			}
		}
	int threshold = track.n*0.2;
	track.candidate_4x4_position_array[0] = track.candidate_4x4_position_array[2];
	track.candidate_4x4_position_array[1] = track.candidate_4x4_position_array[3];
	track.candidate_4x4_position_array[0 + track.n * 2] = track.candidate_4x4_position_array[0 + (track.n - 2) * 2];
	track.candidate_4x4_position_array[1 + track.n * 2] = track.candidate_4x4_position_array[1 + (track.n - 2) * 2];
	/*for (int n = 0; n<2 * (track.n - 1); n++)
	{
		printf("%d \n", track.candidate_4x4_position_array[n]);
	}*/
	//printf("the active zone is (%d,%d),(%d,%d)\n", track.candidate_4x4_position_array[0], track.candidate_4x4_position_array[1], track.candidate_4x4_position_array[0 + track.n * 2], track.candidate_4x4_position_array[1 + track.n * 2]);
}
void fix_active_zone()
{
	if (track.reflash_num == 0)
	{
		if (track.statistics_x>0)
		{
			track.candidate_picture_list[track.reflash_num][1] = track.candidate_4x4_position_array[0];
			track.candidate_picture_list[track.reflash_num][3] = track.candidate_picture_list[19][3];
			if (track.candidate_picture_list[track.reflash_num][1] < (track.candidate_picture_list[19][1] - track.median_filter_mvx))
			{
				track.candidate_picture_list[track.reflash_num][1] = track.candidate_picture_list[19][1] - track.median_filter_mvx;
			}
		}
		else if (track.statistics_x < 0)
		{
			track.candidate_picture_list[track.reflash_num][1] = track.candidate_4x4_position_array[0 + track.n * 2] - track.candidate_picture_list[19][3];
			track.candidate_picture_list[track.reflash_num][3] = track.candidate_picture_list[19][3];
			if (track.candidate_picture_list[track.reflash_num][1] >(track.candidate_picture_list[19][1] - track.median_filter_mvx))
			{
				track.candidate_picture_list[track.reflash_num][1] = track.candidate_picture_list[19][1] - track.median_filter_mvx;
			}
		}
		if (track.statistics_y > 0)
		{
			track.candidate_picture_list[track.reflash_num][2] = track.candidate_4x4_position_array[1];
			track.candidate_picture_list[track.reflash_num][4] = track.candidate_picture_list[19][4];
			if (track.candidate_picture_list[track.reflash_num][2] < (track.candidate_picture_list[19][2] - track.median_filter_mvy))
			{
				track.candidate_picture_list[track.reflash_num][2] = track.candidate_picture_list[19][2] - track.median_filter_mvy;
			}
		}
		else if (track.statistics_y < 0)
		{
			track.candidate_picture_list[track.reflash_num][2] = track.candidate_4x4_position_array[1 + track.n * 2] - track.candidate_picture_list[19][4];
			track.candidate_picture_list[track.reflash_num][4] = track.candidate_picture_list[19][4];
			if (track.candidate_picture_list[track.reflash_num][2] >(track.candidate_picture_list[19][2] - track.median_filter_mvy))
			{
				track.candidate_picture_list[track.reflash_num][2] = track.candidate_picture_list[19][2] - track.median_filter_mvy;
			}
		}
		printf("the fix active zone is (%d,%d),(%d,%d)\n", track.candidate_picture_list[track.reflash_num][1], track.candidate_picture_list[track.reflash_num][2], track.candidate_picture_list[track.reflash_num][3], track.candidate_picture_list[track.reflash_num][4]);
	}
	else
	{
		if (track.statistics_x>0)
		{
			track.candidate_picture_list[track.reflash_num][1] = track.candidate_4x4_position_array[0];
			track.candidate_picture_list[track.reflash_num][3] = track.candidate_picture_list[track.reflash_num - 1][3];
			if (track.candidate_picture_list[track.reflash_num][1] < (track.candidate_picture_list[track.reflash_num - 1][1] - track.median_filter_mvx))
			{
				track.candidate_picture_list[track.reflash_num][1] = track.candidate_picture_list[track.reflash_num - 1][1] - track.median_filter_mvx;
			}
		}
		else if (track.statistics_x < 0)
		{
			track.candidate_picture_list[track.reflash_num][1] = track.candidate_4x4_position_array[0 + track.n * 2] - track.candidate_picture_list[track.reflash_num - 1][3];
			track.candidate_picture_list[track.reflash_num][3] = track.candidate_picture_list[track.reflash_num - 1][3];
			if (track.candidate_picture_list[track.reflash_num][1] >(track.candidate_picture_list[track.reflash_num - 1][1] - track.median_filter_mvx))
			{
				track.candidate_picture_list[track.reflash_num][1] = track.candidate_picture_list[track.reflash_num - 1][1] - track.median_filter_mvx;
			}
		}
		if (track.statistics_y > 0)
		{
			track.candidate_picture_list[track.reflash_num][2] = track.candidate_4x4_position_array[1];
			track.candidate_picture_list[track.reflash_num][4] = track.candidate_picture_list[track.reflash_num - 1][4];
			if (track.candidate_picture_list[track.reflash_num][2] < (track.candidate_picture_list[track.reflash_num - 1][2] - track.median_filter_mvy))
			{
				track.candidate_picture_list[track.reflash_num][2] = track.candidate_picture_list[track.reflash_num - 1][2] - track.median_filter_mvy;
			}
		}
		else if (track.statistics_y < 0)
		{
			track.candidate_picture_list[track.reflash_num][2] = track.candidate_4x4_position_array[1 + track.n * 2] - track.candidate_picture_list[track.reflash_num - 1][4];
			track.candidate_picture_list[track.reflash_num][4] = track.candidate_picture_list[track.reflash_num - 1][4];
			if (track.candidate_picture_list[track.reflash_num][2] >(track.candidate_picture_list[track.reflash_num - 1][2] - track.median_filter_mvy))
			{
				track.candidate_picture_list[track.reflash_num][2] = track.candidate_picture_list[track.reflash_num - 1][2] - track.median_filter_mvy;
			}
		}
		printf("the fix active zone is (%d,%d),(%d,%d)\n", track.candidate_picture_list[track.reflash_num][1], track.candidate_picture_list[track.reflash_num][2], track.candidate_picture_list[track.reflash_num][3], track.candidate_picture_list[track.reflash_num][4]);
	}
}
void reflash_candidate_picture_list()
{
	track.reflash_num++;
	if (track.reflash_num > 19) track.reflash_num = 0;
	track.candidate_picture_list[track.reflash_num][0] = track.active_pic;
	track.candidate_picture_list[track.reflash_num][1] = track.candidate_4x4_position_array[0];
	track.candidate_picture_list[track.reflash_num][2] = track.candidate_4x4_position_array[1];
	track.candidate_picture_list[track.reflash_num][3] = track.candidate_4x4_position_array[0 + track.n * 2] - track.candidate_4x4_position_array[0];
	track.candidate_picture_list[track.reflash_num][4] = track.candidate_4x4_position_array[1 + track.n * 2] - track.candidate_4x4_position_array[1];
	//printf("track.reflash_num=%d\n",track.reflash_num);
	//printf("track.reflash_num=%d\n",track.reflash_num);
	//printf("the reflash active zone is (%d,%d),(%d,%d)\n", track.candidate_picture_list[track.reflash_num][1], track.candidate_picture_list[track.reflash_num][2], track.candidate_picture_list[track.reflash_num][3], track.candidate_picture_list[track.reflash_num][4]);
}
void statistics_direction(int mvx, int mvy)
{
	if (mvx>0)
	{
		track.statistics_x++;
	}
	else if (mvx < 0)
	{
		track.statistics_x--;
	}
	if (mvy>0)
	{
		track.statistics_y++;
	}
	else if (mvy < 0)
	{
		track.statistics_y--;
	}
	//	printf("the direction is (%d,%d)\n", track.statistics_x, track.statistics_y);
}
void median_filter()
{
	int middle_number;
	//printf("track.n=%d \n", track.n);
	for (int i = 0; i<track.n; i++)
		for (int m = 0; m + 1<track.n; m++)
		{
			if (track.candidate_mv_array[0 + m * 2]>track.candidate_mv_array[0 + (m + 1) * 2])
			{
				middle_number = track.candidate_mv_array[0 + m * 2];
				track.candidate_mv_array[0 + m * 2] = track.candidate_mv_array[0 + (m + 1) * 2];
				track.candidate_mv_array[0 + (m + 1) * 2] = middle_number;
			}
		}
	for (int i = 0; i<track.n; i++)
		for (int m = 0; m + 2< track.n; m++)
		{
			if (track.candidate_mv_array[1 + m * 2] > track.candidate_mv_array[1 + (m + 1) * 2])
			{
				middle_number = track.candidate_mv_array[1 + m * 2];
				track.candidate_mv_array[1 + m * 2] = track.candidate_mv_array[1 + (m + 1) * 2];
				track.candidate_mv_array[1 + (m + 1) * 2] = middle_number;
			}
		}
	/*for (int n = 0; n<2 * (track.n - 1); n++)
	{
		printf("%d \n", track.candidate_mv_array[n]);
	}*/
	if (track.n % 2 == 0)
	{
		track.median_filter_mvx = track.candidate_mv_array[0 + track.n];
		track.median_filter_mvy = track.candidate_mv_array[1 + track.n];
	}
	else
	{
		track.median_filter_mvx = track.candidate_mv_array[1 + track.n];
		track.median_filter_mvy = track.candidate_mv_array[2 + track.n];
	}
	/*	for (int i = 0; i < track.n; i++)
	{
	printf("(%d,%d)",track.candidate_mv_array[0+i*2], track.candidate_mv_array[1 + i * 2]);
	printf("\n");
	}*/
	//printf("the direction is (%d,%d)\n", track.statistics_x, track.statistics_y);
	//printf("the median_filter mv is (%d,%d)\n", track.median_filter_mvx, track.median_filter_mvy);
}
void distinguish_part_location_uv(int i, int j, StorablePicture *p, int crop_top, int crop_bottom, int crop_left, int crop_right, FILE *p_out)
{
	int find_candidate, find_active_zone;
	//printf("the output frame_num is %d\n", p->frame_num);
	if (findout_candidate(p->frame_num))
	{
		for (i = crop_top; i<p->size_y_cr - crop_bottom; i++)
			for (j = crop_left; j<p->size_x_cr - crop_right; j++)
			{
				if (j >= track.candidate_picture_list[track.candidate_ref_list_num][1] / 2 && j <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3]) / 2)
					if (i >= track.candidate_picture_list[track.candidate_ref_list_num][2] / 2 && i <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + 4) / 2)
						p->imgUV[0][i][j] = 0;
				if (j <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3]) / 2 && j >= track.candidate_picture_list[track.candidate_ref_list_num][1] / 2)
					if (i >= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4] - 4) / 2 && i <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4]) / 2)
						p->imgUV[0][i][j] = 0;
				if (i >= track.candidate_picture_list[track.candidate_ref_list_num][2] / 2 && i <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4]) / 2)
					if (j >= track.candidate_picture_list[track.candidate_ref_list_num][1] / 2 && j <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + 4) / 2)
						p->imgUV[0][i][j] = 0;
				if (i <= (track.candidate_picture_list[track.candidate_ref_list_num][2] + track.candidate_picture_list[track.candidate_ref_list_num][4]) / 2 && i >= track.candidate_picture_list[track.candidate_ref_list_num][2] / 2)
					if (j >= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3] - 4) / 2 && j <= (track.candidate_picture_list[track.candidate_ref_list_num][1] + track.candidate_picture_list[track.candidate_ref_list_num][3]) / 2)
						p->imgUV[0][i][j] = 0;
				//      else
				fputc(p->imgUV[0][i][j], p_out);
			}
		for (i = crop_top; i<p->size_y_cr - crop_bottom; i++)
			for (j = crop_left; j<p->size_x_cr - crop_right; j++)
			{
				fputc(p->imgUV[1][i][j], p_out);
			}
	}
	else
	{
		for (i = crop_top; i<p->size_y_cr - crop_bottom; i++)
			for (j = crop_left; j<p->size_x_cr - crop_right; j++)
			{
				fputc(p->imgUV[0][i][j], p_out);
			}
		for (i = crop_top; i<p->size_y_cr - crop_bottom; i++)
			for (j = crop_left; j<p->size_x_cr - crop_right; j++)
			{
				fputc(p->imgUV[1][i][j], p_out);
			}
	}
	int te = YUV2RGB(p->imgY, p->imgUV[0], p->imgUV[1], p->size_x, p->size_y, p->size_y_cr, p->size_x_cr, crop_top, crop_left, crop_bottom, crop_right);
}