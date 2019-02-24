/**********************************************************************
* Software Copyright Licensing Disclaimer
*
* This software module was originally developed by contributors to the
* course of the development of ISO/IEC 14496-10 for reference purposes
* and its performance may not have been optimized.  This software
* module is an implementation of one or more tools as specified by
* ISO/IEC 14496-10.  ISO/IEC gives users free license to this software
* module or modifications thereof. Those intending to use this software
* module in products are advised that its use may infringe existing
* patents.  ISO/IEC have no liability for use of this software module
* or modifications thereof.  The original contributors retain full
* rights to modify and use the code for their own purposes, and to
* assign or donate the code to third-parties.
*
* This copyright notice must be included in all copies or derivative
* works.  Copyright (c) ISO/IEC 2004.
**********************************************************************/

/*!
*************************************************************************************
* \file annexb.c
*
* \brief
*    Annex B Byte Stream format
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
*      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
*************************************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "annexb.h"
#include "memalloc.h"


FILE *bits = NULL;                //!< the bit stream file
static int FindStartCode(unsigned char *Buf, int zeros_in_startcode);

int IsFirstByteStreamNALU = 1;
int LastAccessUnitExists = 0;
int NALUCount = 0;


/*!
************************************************************************
* \brief
*    Returns the size of the NALU (bits between start codes in case of
*    Annex B.  nalu->buf and nalu->len are filled.  Other field in
*    nalu-> remain uninitialized (will be taken care of by NALUtoRBSP.
*
* \return
*     0 if there is nothing any more to read (EOF)
*    -1 in case of any error
*
*  \note Side-effect: Returns length of start-code in bytes.
*
* \note
*   GetAnnexbNALU expects start codes at byte aligned positions in the file
*
************************************************************************
*/

int GetAnnexbNALU(NALU_t *nalu)
{
	int info2, info3, pos = 0;
	int StartCodeFound, rewind;
	//char *Buf;
	int LeadingZero8BitsCount = 0, TrailingZero8Bits = 0;

	//if ((Buf = (char*)calloc(nalu->max_size, sizeof(char))) == NULL) no_mem_exit("GetAnnexbNALU: Buf");
	char Buf[80000];
	//printf("???");
	while (!feof(bits) && (Buf[pos++] = fgetc(bits)) == 0); //灏咮its鏂囦欢澶?0000000璇诲嚭鏀惧叆Buf

															/*鍒ゆ柇鏄惁涓篘ALU鍗曞厓璧峰鏍囪瘑*/

	if (feof(bits)) //濡傛灉鏂囦欢鍏ㄩ儴璇诲畬
	{
		if (pos == 0)
			return 0;
		else
		{
			printf("GetAnnexbNALU can't read start code\n");
			//free(Buf);
			return -1;
		}
	}

	if (Buf[pos - 1] != 1)
	{
		printf("GetAnnexbNALU: no Start Code at the begin of the NALU, return -1\n");
		//free(Buf);
		return -1;
	}

	if (pos<3)
	{
		printf("GetAnnexbNALU: no Start Code at the begin of the NALU, return -1\n");
		//free(Buf);
		return -1;
	}
	else if (pos == 3)
	{
		nalu->startcodeprefix_len = 3; //4涓哄弬鏁伴泦鍜屼竴甯х殑绗竴涓猄lice锛?涓哄叾瀹?
		LeadingZero8BitsCount = 0;
	}
	else
	{
		LeadingZero8BitsCount = pos - 4; //闄ゅ幓NALU澶?x00000001浠ュ墠鐨?bits瀛楄妭0鐨勪釜鏁?
		nalu->startcodeprefix_len = 4;
	}

	//the 1st byte stream NAL unit can has leading_zero_8bits, but subsequent ones are not
	//allowed to contain it since these zeros(if any) are considered trailing_zero_8bits
	//of the previous byte stream NAL unit.
	if (!IsFirstByteStreamNALU && LeadingZero8BitsCount>0)
	{
		printf("GetAnnexbNALU: The leading_zero_8bits syntax can only be present in the first byte stream NAL unit, return -1\n");
		//free(Buf);
		return -1;
	}
	IsFirstByteStreamNALU = 0;

	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	while (!StartCodeFound)
	{
		if (feof(bits)) //濡傛灉鏂囦欢宸茬粡缁撴潫
		{
			//Count the trailing_zero_8bits
			while (Buf[pos - 2 - TrailingZero8Bits] == 0)
				TrailingZero8Bits++;
			nalu->len = (pos - 1) - nalu->startcodeprefix_len - LeadingZero8BitsCount - TrailingZero8Bits; //璁＄畻涓€涓狽ALU鍗曞厓鐨勯暱搴?
			memcpy(nalu->buf, &Buf[LeadingZero8BitsCount + nalu->startcodeprefix_len], nalu->len);  //澶嶅埗涓€涓狽ALU鍗曞厓鍒皀al_buf   
			nalu->forbidden_bit = (nalu->buf[0] >> 7) & 1; //寰楀埌forbidden_zero_bit鍙ユ硶鍏冪礌鍊?
			nalu->nal_reference_idc = (nalu->buf[0] >> 5) & 3; //寰楀埌nal_ref_idc鍙ユ硶鍏冪礌鍊?
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f; //寰楀埌nal_unit_type鍙ユ硶鍏冪礌鍊?

														 // printf ("GetAnnexbNALU, eof case: pos %d nalu->len %d, nalu->reference_idc %d, nal_unit_type %d \n", pos, nalu->len, nalu->nal_reference_idc, nalu->nal_unit_type);

#if TRACE //杈撳嚭trace_enc娴嬭瘯鏂囦欢 if _DEBUG
			fprintf(p_trace, "\n\nLast NALU in File\n\n");
			fprintf(p_trace, "Annex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
				nalu->startcodeprefix_len == 4 ? "long" : "short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
			fflush(p_trace);
#endif
			//free(Buf); //鍥犱负鏂囦欢宸茶瀹屾墍浠ラ噴鏀綛UF
			return pos - 1;
		}
		Buf[pos++] = fgetc(bits); //鏂囦欢姝ｅ父璇诲彇锛岃鍙朜ALU鍗曞厓鍐呭锛屼互瀛楄妭涓哄崟浣?
		info3 = FindStartCode(&Buf[pos - 4], 3);	//妫€娴嬫槸鍚︽壘鍒癗ALU鍗曞厓澶?x00000001锛?琛ㄧず鎵惧埌锛?琛ㄧず娌℃湁鎵惧埌++ info3=1表示找到的下一个NALU的开始码前有8比特填零码（00000001）
		if (info3 != 1)
			info2 = FindStartCode(&Buf[pos - 3], 2); //妫€娴嬫槸鍚︽壘鍒癗ALU鍗曞厓澶?x000001锛?琛ㄧず鎵惧埌锛?琛ㄧず娌℃湁鎵惧埌	//++ info2=1表示找到的下一个NALU的开始码前没有8比特填零码（000001）
		StartCodeFound = (info2 == 1 || info3 == 1); //1琛ㄧず鎵惧埌浜嗕袱绉峃ALU鍗曞厓澶寸殑浠绘剰涓€绉嶏紝0琛ㄧず娌℃湁鎵惧埌
	}

	//Count the trailing_zero_8bits
	if (info3 == 1)	//if the detected start code is 00 00 01, trailing_zero_8bits is sure not to be present
	{				//++ 即info2=1时，当前NALU必然不存在拖尾8比特填零码，因此不必计数；反之，即info3=1时，要分析当前NALU是否有拖尾8比特填零码，有则计数
		while (Buf[pos - 5 - TrailingZero8Bits] == 0) //濡傛灉鎵惧埌浜?x00000001锛屽垯璁＄畻鎷栧熬0涓暟
			TrailingZero8Bits++;
	}
	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = 0;
	if (info3 == 1)
		rewind = -4;
	else if (info2 == 1)
		rewind = -3;
	else
		printf(" Panic: Error in next start code search \n");

	if (0 != fseek(bits, rewind, SEEK_CUR))	//灏嗘枃浠舵寚閽堝墠绉籸ewind涓崟浣嶏紝濡傛灉鎴愬姛杩斿洖0锛屽鏋滀笉鎴愬姛杩斿洖闈? ++ SEEK_CUR=1，代表文件指针当前位置
	{
		snprintf(errortext, ET_SIZE, "GetAnnexbNALU: Cannot fseek %d in the bit stream file", rewind);
		//free(Buf);
		error(errortext, 600);
	}

	// Here the leading zeros(if any), Start code, the complete NALU, trailing zeros(if any)
	// and the next start code is in the Buf.
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits
	// is the size of the NALU.

	nalu->len = (pos + rewind) - nalu->startcodeprefix_len - LeadingZero8BitsCount - TrailingZero8Bits; //鍚屼笂鎵惧嚭NALU鍗曞厓鍙ユ硶鍏冪礌鍊?
	memcpy(nalu->buf, &Buf[LeadingZero8BitsCount + nalu->startcodeprefix_len], nalu->len);
	nalu->forbidden_bit = (nalu->buf[0] >> 7) & 1;		//+++++++++++++++++++++++++++
	nalu->nal_reference_idc = (nalu->buf[0] >> 5) & 3;	//++ 该三值定义参见标准7.4.1
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;		//+++++++++++++++++++++++++++


														//printf ("GetAnnexbNALU, regular case: pos %d nalu->len %d, nalu->reference_idc %d, nal_unit_type %d \n", pos, nalu->len, nalu->nal_reference_idc, nalu->nal_unit_type);
#if TRACE
	fprintf(p_trace, "\n\nAnnex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
		nalu->startcodeprefix_len == 4 ? "long" : "short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
	fflush(p_trace);
#endif

	//free(Buf);

	return (pos + rewind); //濡傛灉姝ｅ父搴旇杩斿洖1锛燂紵
}




/*!
************************************************************************
* \brief
*    Opens the bit stream file named fn
* \return
*    none
************************************************************************
*/
void OpenBitstreamFile(char *fn)
{
	if (NULL == (bits = fopen(fn, "rb")))
	{
		snprintf(errortext, ET_SIZE, "Cannot open Annex B ByteStream file '%s'", input->infile);
		error(errortext, 500);
	}
}


/*!
************************************************************************
* \brief
*    Closes the bit stream file
************************************************************************
*/
void CloseBitstreamFile()
{
	fclose(bits);
}


/*!
************************************************************************
* \brief
*    returns if new start code is found at byte aligned position buf.
*    new-startcode is of form N 0x00 bytes, followed by a 0x01 byte.
*
*  \return
*     1 if start-code is found or                      \n
*     0, indicating that there is no start code
*
*  \param Buf
*     pointer to byte-stream
*  \param zeros_in_startcode
*     indicates number of 0x00 bytes in start-code.
************************************************************************
*/
static int FindStartCode(unsigned char *Buf, int zeros_in_startcode)
{
	int info;
	int i;

	info = 1;
	for (i = 0; i < zeros_in_startcode; i++)
		if (Buf[i] != 0)
			info = 0;

	if (Buf[i] != 1)
		info = 0;
	return info;
}

void CheckZeroByteNonVCL(NALU_t *nalu, int * ret)
{
	int CheckZeroByte = 0;

	//This function deals only with non-VCL NAL units
	if (nalu->nal_unit_type >= 1 && nalu->nal_unit_type <= 5) //濡傛灉NALU鍗曞厓灞炰簬VCL鍗曞厓鍒欎笉鐢ㄦ墽琛孋heckZeroByteNonVCL鍑芥暟
		return;

	//for SPS and PPS, zero_byte shall exist
	if (nalu->nal_unit_type == NALU_TYPE_SPS || nalu->nal_unit_type == NALU_TYPE_PPS)	//++ NALU_TYPE_SPS=7,NALU_TYPE_PPS=8
		CheckZeroByte = 1;
	//check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
	if (nalu->nal_unit_type == NALU_TYPE_AUD || nalu->nal_unit_type == NALU_TYPE_SPS ||
		nalu->nal_unit_type == NALU_TYPE_PPS || nalu->nal_unit_type == NALU_TYPE_SEI ||
		(nalu->nal_unit_type >= 13 && nalu->nal_unit_type <= 18))
	{
		if (LastAccessUnitExists)
		{
			LastAccessUnitExists = 0;		//deliver the last access unit to decoder
			NALUCount = 0;                  //璁℃暟NALU鍗曞厓
		}
	}
	NALUCount++;	//++ 对已解码的非VCL NAL单元计数
					//for the first NAL unit in an access unit, zero_byte shall exists
	if (NALUCount == 1)
		CheckZeroByte = 1;
	if (CheckZeroByte && nalu->startcodeprefix_len == 3)
	{
		printf("warning: zero_byte shall exist\n");
		//because it is not a very serious problem, we may not indicate an error by setting ret to -1
		//*ret=-1;
	}
}

void CheckZeroByteVCL(NALU_t *nalu, int * ret)
{
	int CheckZeroByte = 0;

	//This function deals only with VCL NAL units
	if (!(nalu->nal_unit_type >= 1 && nalu->nal_unit_type <= 5))
		return;

	if (LastAccessUnitExists)
	{
		NALUCount = 0;
	}
	NALUCount++;	//++ 对解码NAL单元个数计数
					//the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates 
					//the start of a new access unit and hence the first NAL unit of the new access unit.						(sounds like a tongue twister :-)
	if (NALUCount == 1)
		CheckZeroByte = 1;
	LastAccessUnitExists = 1;
	if (CheckZeroByte && nalu->startcodeprefix_len == 3)
	{
		printf("warning: zero_byte shall exist\n");
		//because it is not a very serious problem, we may not indicate an error by setting ret to -1
		//*ret=-1;
	}
}
