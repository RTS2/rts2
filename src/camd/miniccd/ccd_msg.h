/***************************************************************************\
    
    Copyright (c) 2001, 2002 David Schmenk
    
    All rights reserved.
    
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, and/or sell copies of the Software, and to permit persons
    to whom the Software is furnished to do so, provided that the above
    copyright notice(s) and this permission notice appear in all copies of
    the Software and that both the above copyright notice(s) and this
    permission notice appear in supporting documentation.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
    OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
    INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
    FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
    NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
    WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
    
    Except as contained in this notice, the name of a copyright holder
    shall not be used in advertising or otherwise to promote the sale, use
    or other dealings in this Software without prior written authorization
    of the copyright holder.
    
\***************************************************************************/

/***************************************************************************\
*                                                                           *
*                              CCD driver commands                          *
*                                                                           *
\***************************************************************************/

#ifndef _CCD_MSG_H_
#define _CCD_MSG_H_

/*
 * Messge header.
 */
typedef unsigned short CCD_ELEM_TYPE;
#define CCD_ELEM_SIZE               sizeof(CCD_ELEM_TYPE)
#define CCD_MSG_HEADER              0xCCDD
#define CCD_MSG_HEADER_INDEX        0
#define CCD_MSG_LENGTH_LO_INDEX     1
#define CCD_MSG_LENGTH_HI_INDEX     2
#define CCD_MSG_INDEX               3
/*
 * CCD query parameters.
 */
#define CCD_MSG_QUERY               0x0101
#define CCD_MSG_QUERY_LEN           (CCD_MSG_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_QUERY_STRING            "QUERY"
/*
 * CCD last request return code.   Only applies to text mode interface.
 */
#define CCD_MSG_ERRORNO             0x0102
#define CCD_MSG_ERRORNO_LEN         (CCD_MSG_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_ERRORNO_STRING          "ERRORNO"
/*
 * CCD expose image request.
 */
#define CCD_MSG_EXP                 0x0201
#define CCD_EXP_WIDTH_INDEX         (CCD_MSG_INDEX+1)
#define CCD_EXP_HEIGHT_INDEX        (CCD_MSG_INDEX+2)
#define CCD_EXP_XOFFSET_INDEX       (CCD_MSG_INDEX+3)
#define CCD_EXP_YOFFSET_INDEX       (CCD_MSG_INDEX+4)
#define CCD_EXP_XBIN_INDEX          (CCD_MSG_INDEX+5)
#define CCD_EXP_YBIN_INDEX          (CCD_MSG_INDEX+6)
#define CCD_EXP_DAC_INDEX           (CCD_MSG_INDEX+7)
#define CCD_EXP_FLAGS_INDEX         (CCD_MSG_INDEX+8)
#define CCD_EXP_MSEC_LO_INDEX       (CCD_MSG_INDEX+9)
#define CCD_EXP_MSEC_HI_INDEX       (CCD_MSG_INDEX+10)
#define CCD_MSG_EXP_LEN             (CCD_EXP_MSEC_HI_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_EXP_STRING              "EXPOSE"
#define CCD_EXP_WIDTH_STRING        "WIDTH"
#define CCD_EXP_HEIGHT_STRING       "HEIGHT"
#define CCD_EXP_XOFFSET_STRING      "XOFFSET"
#define CCD_EXP_YOFFSET_STRING      "YOFFSET"
#define CCD_EXP_XBIN_STRING         "XBIN"
#define CCD_EXP_YBIN_STRING         "YBIN"
#define CCD_EXP_DAC_STRING          "DAC"
#define CCD_EXP_FLAGS_STRING        "FLAGS"
#define CCD_EXP_MSEC_STRING         "MSEC"
/*
 * CCD expose image flags.
 */
#define CCD_EXP_FLAGS_NOBIN_ACCUM   4
#define CCD_EXP_FLAGS_NOWIPE_FRAME  8
#define CCD_EXP_FLAGS_NOOPEN_SHUTTER 16
#define CCD_EXP_FLAGS_TDI           32
#define CCD_EXP_FLAGS_NOCLEAR_FRAME 64
/*
 * CCD abort operations.
 */
#define CCD_MSG_ABORT               0x0301
#define CCD_MSG_ABORT_LEN           (CCD_MSG_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_ABORT_STRING            "ABORT"
/*
 * CCD control request.
 */
#define CCD_MSG_CTRL                0x0401
#define CCD_CTRL_CMD_INDEX          (CCD_MSG_INDEX+1)
#define CCD_CTRL_PARM_LO_INDEX      (CCD_MSG_INDEX+2)
#define CCD_CTRL_PARM_HI_INDEX      (CCD_MSG_INDEX+3)
#define CCD_MSG_CTRL_LEN            (CCD_CTRL_PARM_HI_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_CTRL_STRING             "CONTROL"
#define CCD_CTRL_CMD_STRING         "COMMAND"
#define CCD_CTRL_PARM_STRING        "PARAMETER"
/*
 * Well-defined control commands.
 */
#define CCD_CTRL_CMD_RESET          0xFFFF
#define CCD_CTRL_CMD_DEC_MOD        0xFFFE
#define CCD_CTRL_CMD_GET_CFW        0xFFFD
#define CCD_CTRL_CMD_SET_CFW        0xFFFC
/*
 * CCD parameters.
 */
#define CCD_MSG_CCD                 0x1001
#define CCD_CCD_NAME_INDEX          (CCD_MSG_INDEX+1)
#define CCD_CCD_NAME_LEN            32
#define CCD_CCD_WIDTH_INDEX         (CCD_MSG_INDEX+2+CCD_CCD_NAME_LEN)
#define CCD_CCD_HEIGHT_INDEX        (CCD_MSG_INDEX+3+CCD_CCD_NAME_LEN)
#define CCD_CCD_PIX_WIDTH_INDEX     (CCD_MSG_INDEX+4+CCD_CCD_NAME_LEN)
#define CCD_CCD_PIX_HEIGHT_INDEX    (CCD_MSG_INDEX+5+CCD_CCD_NAME_LEN)
#define CCD_CCD_FIELDS_INDEX        (CCD_MSG_INDEX+6+CCD_CCD_NAME_LEN)
#define CCD_CCD_DEPTH_INDEX         (CCD_MSG_INDEX+7+CCD_CCD_NAME_LEN)
#define CCD_CCD_DAC_INDEX           (CCD_MSG_INDEX+8+CCD_CCD_NAME_LEN)
#define CCD_CCD_COLOR_INDEX         (CCD_MSG_INDEX+9+CCD_CCD_NAME_LEN)
#define CCD_CCD_CAPS_INDEX          (CCD_MSG_INDEX+10+CCD_CCD_NAME_LEN)
#define CCD_MSG_CCD_LEN             (CCD_CCD_CAPS_INDEX*CCD_ELEM_SIZE+CCD_ELEM_SIZE)
#define CCD_CCD_STRING              "CCD"
#define CCD_CCD_NAME_STRING         "NAME"
#define CCD_CCD_WIDTH_STRING        "WIDTH"
#define CCD_CCD_HEIGHT_STRING       "HEIGHT"
#define CCD_CCD_PIX_WIDTH_STRING    "PIX_WIDTH"
#define CCD_CCD_PIX_HEIGHT_STRING   "PIX_HEIGHT"
#define CCD_CCD_FIELDS_STRING       "FIELDS"
#define CCD_CCD_DEPTH_STRING        "DEPTH"
#define CCD_CCD_DAC_STRING          "DAC"
#define CCD_CCD_COLOR_STRING        "COLOR"
#define CCD_CCD_CAPS_STRING         "CAPS"
/*
 * CCD color representation.
 *  Packed colors allow individual sizes up to 16 bits.
 *  2X2 matrix bits are represented as:
 *      0 1
 *      2 3
 */
#define CCD_COLOR_PACKED_RGB        0x8000
#define CCD_COLOR_PACKED_BGR        0x4000
#define CCD_COLOR_PACKED_RED_SIZE   0x0F00
#define CCD_COLOR_PACKED_GREEN_SIZE 0x00F0
#define CCD_COLOR_PACKED_BLUE_SIZE  0x000F
#define CCD_COLOR_MATRIX_ALT_EVEN   0x2000
#define CCD_COLOR_MATRIX_ALT_ODD    0x1000
#define CCD_COLOR_MATRIX_2X2        0x0000
#define CCD_COLOR_MATRIX_RED_MASK   0x0F00
#define CCD_COLOR_MATRIX_GREEN_MASK 0x00F0
#define CCD_COLOR_MATRIX_BLUE_MASK  0x000F
#define CCD_COLOR_MONOCHROME        0x0FFF
/*
 * CCD image.
 */
#define CCD_MSG_IMAGE               0x2001
#define CCD_IMAGE_PIXELS_INDEX      (CCD_MSG_INDEX+1)
#define CCD_MSG_IMAGE_LEN           (CCD_IMAGE_PIXELS_INDEX*CCD_ELEM_SIZE)	/* Note: not +CCD_ELEM_SIZE as pixels is based on image width */
#define CCD_IMAGE_STRING            "IMAGE"
#define CCD_IMAGE_WIDTH_STRING      "WIDTH"
#define CCD_IMAGE_HEIGHT_STRING     "HEIGHT"
/*
 * Message sizes.
 */
#define MAX_CCD_MSG_LEN             CCD_MSG_EXP_LEN

#endif /* _CCD_MSG_H_ */
