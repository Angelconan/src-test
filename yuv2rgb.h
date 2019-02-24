#include<stdbool.h>
#pragma once
#define UINT unsigned int
#define LPBYTE unsigned char**
#ifdef YUV2RGB
bool YUV2RGB(LPBYTE yBuf, LPBYTE uBuf, LPBYTE vBuf, int size_x, int size_y, int size_x_cr, int size_y_cr, int crop_top, int crop_left, int crop_bottom, int crop_right)
#endif // !YUV2RGB

