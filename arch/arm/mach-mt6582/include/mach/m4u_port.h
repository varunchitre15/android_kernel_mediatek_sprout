/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __M4U_PORT_H__
#define __M4U_PORT_H__

//====================================
// about portid
//====================================
#define M4U_LARB0_PORTn(n)      ((n)+0)
#define M4U_LARB1_PORTn(n)      ((n)+9)
#define M4U_LARB2_PORTn(n)      ((n)+16)
#define M4U_LARB3_PORTn(n)      ((n)+33)
#define M4U_LARB4_PORTn(n)      ((n)+33)
#define M4U_LARB5_PORTn(n)      ((n)+33)

enum
{
    DISP_OVL_0               =  M4U_LARB0_PORTn(0)    ,
    DISP_RDMA                =  M4U_LARB0_PORTn(1)    ,
    DISP_WDMA                =  M4U_LARB0_PORTn(2)    ,
    MM_CMDQ                  =  M4U_LARB0_PORTn(3)    ,
    MDP_RDMA                 =  M4U_LARB0_PORTn(4)    ,
    MDP_WDMA                 =  M4U_LARB0_PORTn(5)    ,
    MDP_ROTO                 =  M4U_LARB0_PORTn(6)    ,
    MDP_ROTCO                =  M4U_LARB0_PORTn(7)    ,
    MDP_ROTVO                =  M4U_LARB0_PORTn(8)    ,
  
    VDEC_MC_EXT              =  M4U_LARB1_PORTn(0)    ,
    VDEC_PP_EXT              =  M4U_LARB1_PORTn(1)    ,
    VDEC_AVC_MV_EXT          =  M4U_LARB1_PORTn(2)    ,
    VDEC_PRED_RD_EXT         =  M4U_LARB1_PORTn(3)    ,
    VDEC_PRED_WR_EXT         =  M4U_LARB1_PORTn(4)    ,
    VDEC_VLD_EXT             =  M4U_LARB1_PORTn(5)    ,
    VDEC_PP_INT              =  M4U_LARB1_PORTn(6)    ,
  
    CAM_IMGO                 =  M4U_LARB2_PORTn(0)    ,
    CAM_IMG2O                =  M4U_LARB2_PORTn(1)    ,
    CAM_LSCI                 =  M4U_LARB2_PORTn(2)    ,
    CAM_IMGI                 =  M4U_LARB2_PORTn(3)    ,
    CAM_ESFKO                =  M4U_LARB2_PORTn(4)    ,
    CAM_AAO                  =  M4U_LARB2_PORTn(5)    ,
    JPGENC_RDMA              =  M4U_LARB2_PORTn(6)    ,
    JPGENC_BSDMA             =  M4U_LARB2_PORTn(7)    ,
    VENC_RD_COMV             =  M4U_LARB2_PORTn(8)    ,
    VENC_SV_COMV             =  M4U_LARB2_PORTn(9)    ,
    VENC_RCPU                =  M4U_LARB2_PORTn(10)    ,
    VENC_REC_FRM             =  M4U_LARB2_PORTn(11)    ,
    VENC_REF_LUMA            =  M4U_LARB2_PORTn(12)    ,
    VENC_REF_CHROMA          =  M4U_LARB2_PORTn(13)    ,
    VENC_BSDMA               =  M4U_LARB2_PORTn(14)    ,
    VENC_CUR_LUMA            =  M4U_LARB2_PORTn(15)    ,
    VENC_CUR_CHROMA          =  M4U_LARB2_PORTn(16)    ,

    M4U_CLNTMOD_LCDC_UI      = M4U_LARB3_PORTn(8)       ,

    M4U_PORT_UNKNOWN,

};
#define M4U_CLNTMOD_MAX M4U_PORT_UNKNOWN


#endif

