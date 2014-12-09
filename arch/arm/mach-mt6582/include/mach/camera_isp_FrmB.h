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

#ifndef _MT_ISP_FRMB_H
#define _MT_ISP_FRMB_H

#include "camera_isp.h"

//#define KERNEL_LOG  //enable debug log flag if defined
#define ISR_LOG_ON  //turn on log print at isr if defined
#define T_STAMP_2_0 //time stamp workaround method. (increase timestamp baseon fix fps, not read at each isr)

#define _rtbc_buf_que_2_0_

/*******************************************************************************
*
********************************************************************************/
typedef enum
{
    ISP_IRQ_CLEAR_NONE_FRMB,
    ISP_IRQ_CLEAR_WAIT_FRMB,
    ISP_IRQ_CLEAR_STATUS_FRMB,
    ISP_IRQ_CLEAR_ALL_FRMB
}ISP_IRQ_CLEAR_ENUM_FRMB;


typedef enum
{
    ISP_IRQ_TYPE_INT_FRMB,
    ISP_IRQ_TYPE_DMA_FRMB,
    ISP_IRQ_TYPE_INTB_FRMB,
    ISP_IRQ_TYPE_DMAB_FRMB,
    ISP_IRQ_TYPE_INTC_FRMB,
    ISP_IRQ_TYPE_DMAC_FRMB,
    ISP_IRQ_TYPE_INTX_FRMB,
    ISP_IRQ_TYPE_DMAX_FRMB,
    ISP_IRQ_TYPE_AMOUNT_FRMB
}ISP_IRQ_TYPE_ENUM_FRMB;


typedef enum    //special user for specific operation
{
    ISP_IRQ_WAITIRQ_SPEUSER_NONE = 0,
    ISP_IRQ_WAITIRQ_SPEUSER_EIS = 1,
    ISP_IRQ_WAITIRQ_SPEUSER_NUM
}ISP_IRQ_WAITIRQ_SPEUSER_ENUM;
typedef struct
{
    ISP_IRQ_TYPE_ENUM_FRMB   Type;
    unsigned int                            Status;
    int                                             UserKey;                     /* user key for doing interrupt operation */
}ISP_IRQ_USER_STRUCT_FRMB;

typedef struct
{
    unsigned int    tLastSig_sec;                       /* time stamp of the latest occuring signal*/
    unsigned int    tLastSig_usec;                  /* time stamp of the latest occuring signal*/
    unsigned int    tMark2WaitSig_sec;            /* time period from marking a signal to user try to wait and get the signal*/
    unsigned int    tMark2WaitSig_usec;            /* time period from marking a signal to user try to wait and get the signal*/
    unsigned int    tLastSig2GetSig_sec;         /* time period from latest occuring signal to user try to wait and get the signal*/
    unsigned int    tLastSig2GetSig_usec;         /* time period from latest occuring signal to user try to wait and get the signal*/
    int                        passedbySigcnt;          /* the count for the signal passed by  */
}ISP_IRQ_TIME_STRUCT_FRMB;

typedef struct
{
    unsigned int       tLastSOF2P1done_sec;                       /* time stamp of the last closest occuring sof signal for pass1 done*/
    unsigned int       tLastSOF2P1done_usec;                  /* time stamp of the last closest occuring sof signal for pass1 done*/
}ISP_EIS_META_STRUCT;

typedef struct
{
    ISP_IRQ_CLEAR_ENUM_FRMB  Clear;
    ISP_IRQ_USER_STRUCT_FRMB  UserInfo;
    ISP_IRQ_TIME_STRUCT_FRMB  TimeInfo;
    ISP_EIS_META_STRUCT EisMeta;
    ISP_IRQ_WAITIRQ_SPEUSER_ENUM SpecUser;
    unsigned int       Timeout;                     /* time out for waiting for a specific interrupt */
    unsigned int       bDumpReg;
}ISP_WAIT_IRQ_STRUCT_FRMB;

typedef struct
{
    int  userKey;
    char* userName;
}ISP_REGISTER_USERKEY_STRUCT_FRMB;


typedef struct
{
    ISP_IRQ_TYPE_ENUM   Type;
    unsigned int       Status;
}ISP_READ_IRQ_STRUCT_FRMB;

typedef struct
{
    ISP_IRQ_TYPE_ENUM   Type;
    unsigned int       Status;
}ISP_CLEAR_IRQ_STRUCT_FRMB;

typedef enum
{
    ISP_HOLD_TIME_VD_FRMB,
    ISP_HOLD_TIME_EXPDONE_FRMB
}ISP_HOLD_TIME_ENUM_FRMB;

typedef struct
{
    unsigned int Addr_FrmB;   // register's addr
    unsigned int Val_FrmB;    // register's value
}ISP_REG_STRUCT_FRMB;

typedef struct
{
    unsigned int Data_FrmB;   // pointer to ISP_REG_STRUCT
    unsigned int Count_FrmB;  // count
}ISP_REG_IO_STRUCT_FRMB;

//typedef void (*pIspCallback)(void);

typedef enum
{
    //Work queue. It is interruptible, so there can be "Sleep" in work queue function.
    ISP_CALLBACK_WORKQUEUE_VD_FRMB,
    ISP_CALLBACK_WORKQUEUE_EXPDONE_FRMB,
    //Tasklet. It is uninterrupted, so there can NOT be "Sleep" in tasklet function.
    ISP_CALLBACK_TASKLET_VD_FRMB,
    ISP_CALLBACK_TASKLET_EXPDONE_FRMB,
    ISP_CALLBACK_AMOUNT_FRMB
}ISP_CALLBACK_ENUM_FRMB;

typedef struct
{
    ISP_CALLBACK_ENUM   Type_FrmB;
    pIspCallback        Func_FrmB;
}ISP_CALLBACK_STRUCT_FRMB;

//
// length of the two memory areas
#define P1_DEQUE_CNT    1
////////////////////////////////////////
//DEFINED IN CAMREA_ISP.H
//#define RT_BUF_TBL_NPAGES 16
//#define ISP_RT_BUF_SIZE 16
//#define ISP_RT_CQ0C_BUF_SIZE (ISP_RT_BUF_SIZE)//(ISP_RT_BUF_SIZE>>1)
///////////////////////////////////////
//pass1 setting sync index
#define ISP_REG_P1_CFG_IDX 0x4090


//
typedef enum
{
    _cam_tg_  = 0,
    _cam_tg2_ ,     // 1
    _camsv_tg_,     // 2
    _camsv2_tg_,    // 3
    _cam_tg_max_
}_isp_tg_enum_;

#if 0//defined in camera_isp.h
//
typedef enum
{
    _imgi_  = 0,
    _vipi_ ,    // 1
    _vip2i_,    // 2
    _vip3i_,    // 3
    _imgo_,     // 4
    _ufdi_,     // 5
    _lcei_,     // 6
    _ufeo_,     // 7
    _rrzo_,     // 8
    _imgo_d_,   // 9
    _rrzo_d_,   // 10
    _img2o_,    // 11
    _img3o_,    // 12
    _img3bo_,   // 13
    _img3co_,   // 14
    _camsv_imgo_,  // 15
    _camsv2_imgo_,// 16
    _mfbo_,     // 17
    _feo_,      //18
    _wrot_,    // 19
    _wdma_,     // 20
    _jpeg_,     // 21
    _venc_stream_,     // 21
    _rt_dma_max_
}_isp_dma_enum_;
#endif

//
typedef struct {
    unsigned int   w;
    unsigned int   h;
    unsigned int   xsize;
    unsigned int   stride;
    unsigned int   fmt;
    unsigned int   pxl_id;
    unsigned int   wbn;
    unsigned int   ob;
    unsigned int   lsc;
    unsigned int   rpg;
    unsigned int   m_num_0;
    unsigned int   frm_cnt;
}ISP_RT_IMAGE_INFO_STRUCT;

typedef struct {
    unsigned int   srcX;    //crop window start point
    unsigned int   srcY;
    unsigned int   srcW;    //crop window size
    unsigned int   srcH;
    unsigned int   dstW;    //rrz out size
    unsigned int   dstH;
}ISP_RT_HRZ_INFO_STRUCT;

typedef struct{
    unsigned int x; //in pix
    unsigned int y; //in pix
    unsigned int w; //in byte
    unsigned int h; //in byte
}ISP_RT_DMAO_CROPPING_STRUCT;


typedef struct {
    unsigned int   memID;
    unsigned int   size;
    unsigned int   base_vAddr;
    unsigned int   base_pAddr;
    unsigned int   timeStampS;
    unsigned int   timeStampUs;
    unsigned int   bFilled;
    ISP_RT_IMAGE_INFO_STRUCT image;
    ISP_RT_HRZ_INFO_STRUCT HrzInfo;
    ISP_RT_DMAO_CROPPING_STRUCT dmaoCrop;   //imgo
    unsigned int          bDequeued;
    signed int         bufIdx;//used for replace buffer
}ISP_RT_BUF_INFO_STRUCT_FRMB;

//
typedef struct  {
    unsigned int           count;
    unsigned int           sof_cnt;         //cnt for current sof
    unsigned int           img_cnt;         //cnt for mapping to which sof
    //rome support only deque 1 image at a time
    //ISP_RT_BUF_INFO_STRUCT  data[ISP_RT_BUF_SIZE];
    ISP_RT_BUF_INFO_STRUCT_FRMB  data[P1_DEQUE_CNT];
}ISP_DEQUE_BUF_INFO_STRUCT_FRMB;

//
typedef struct  {
    unsigned int           start;          //current DMA accessing buffer
    unsigned int           total_count;    //total buffer number.Include Filled and empty
    unsigned int           empty_count;    //total empty buffer number include current DMA accessing buffer
    unsigned int           pre_empty_count;//previous total empty buffer number include current DMA accessing buffer
    unsigned int           active;
    unsigned int           read_idx;
    unsigned int           img_cnt;         //cnt for mapping to which sof
    ISP_RT_BUF_INFO_STRUCT_FRMB  data[ISP_RT_BUF_SIZE];
}ISP_RT_RING_BUF_INFO_STRUCT_FRMB;

//
typedef enum
{
    ISP_RT_BUF_CTRL_ENQUE_FRMB,      // 0
#ifdef _rtbc_buf_que_2_0_
            ISP_RT_BUF_CTRL_ENQUE_IMD_FRMB,
#else
            ISP_RT_BUF_CTRL_ENQUE_IMD_FRMB = ISP_RT_BUF_CTRL_ENQUE_FRMB,
#endif
    ISP_RT_BUF_CTRL_EXCHANGE_ENQUE_FRMB, // 1
    ISP_RT_BUF_CTRL_DEQUE_FRMB,          // 2
    ISP_RT_BUF_CTRL_IS_RDY_FRMB,         // 3
#ifdef _rtbc_buf_que_2_0_
    ISP_RT_BUF_CTRL_DMA_EN_FRMB,           // 4
#endif
    ISP_RT_BUF_CTRL_GET_SIZE_FRMB,       // 5
    ISP_RT_BUF_CTRL_CLEAR_FRMB,          // 6
    ISP_RT_BUF_CTRL_CUR_STATUS_FRMB,     //7
    ISP_RT_BUF_CTRL_MAX_FRMB
}ISP_RT_BUF_CTRL_ENUM_FRMB;

typedef struct  {
    ISP_RTBC_STATE_ENUM state;
    unsigned long dropCnt;
    ISP_RT_RING_BUF_INFO_STRUCT_FRMB  ring_buf[_rt_dma_max_];
}ISP_RT_BUF_STRUCT_FRMB;
//
typedef struct  {
    ISP_RT_BUF_CTRL_ENUM_FRMB    ctrl;
    _isp_dma_enum_          buf_id;
    unsigned int            data_ptr;
    unsigned int            ex_data_ptr; //exchanged buffer
}ISP_BUFFER_CTRL_STRUCT_FRMB;


#if 0//defined in camera_isp.h
typedef enum
{
    ISP_RTBC_STATE_INIT, // 0
    ISP_RTBC_STATE_SOF,
    ISP_RTBC_STATE_DONE,
    ISP_RTBC_STATE_MAX
}ISP_RTBC_STATE_ENUM;
//
typedef enum
{
    ISP_RTBC_BUF_EMPTY, // 0
    ISP_RTBC_BUF_FILLED,// 1
    ISP_RTBC_BUF_LOCKED,// 2
}ISP_RTBC_BUF_STATE_ENUM;
#endif

//
//reference count
#define _use_kernel_ref_cnt_

//
typedef enum
{
    ISP_REF_CNT_GET_FRMB,    // 0
    ISP_REF_CNT_INC_FRMB,    // 1
    ISP_REF_CNT_DEC_FRMB,    // 2
    ISP_REF_CNT_DEC_AND_RESET_P1_P2_IF_LAST_ONE_FRMB,    // 3
    ISP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE_FRMB,    // 4
    ISP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE_FRMB,    // 5
    ISP_REF_CNT_MAX_FRMB
}ISP_REF_CNT_CTRL_ENUM_FRMB;

//
typedef enum
{
    ISP_REF_CNT_ID_IMEM_FRMB,    // 0
    ISP_REF_CNT_ID_ISP_FUNC_FRMB,// 1
    ISP_REF_CNT_ID_GLOBAL_PIPE_FRMB, // 2
    ISP_REF_CNT_ID_P1_PIPE_FRMB,     // 3
    ISP_REF_CNT_ID_P2_PIPE_FRMB,     // 4
    ISP_REF_CNT_ID_MAX_FRMB,
}ISP_REF_CNT_ID_ENUM_FRMB;

typedef struct  {
    ISP_REF_CNT_CTRL_ENUM_FRMB   ctrl;
    ISP_REF_CNT_ID_ENUM_FRMB     id;
    unsigned long           data_ptr;
}ISP_REF_CNT_CTRL_STRUCT_FRMB;


//struct for enqueue/dequeue control in ihalpipe wrapper
typedef enum
{
    ISP_ED_BUFQUE_CTRL_ENQUE_FRAME=0,          // 0,signal that a specific buffer is enqueued
    ISP_ED_BUFQUE_CTRL_WAIT_DEQUE,             // 1,a dequeue thread is waiting to do dequeue
    ISP_ED_BUFQUE_CTRL_DEQUE_SUCCESS,          // 2,signal that a buffer is dequeued (success)
    ISP_ED_BUFQUE_CTRL_DEQUE_FAIL,             // 3,signal that a buffer is dequeued (fail)
    ISP_ED_BUFQUE_CTRL_WAIT_FRAME,             // 4,wait for a specific buffer
    ISP_ED_BUFQUE_CTRL_WAKE_WAITFRAME,         // 5,wake all sleeped users to check buffer is dequeued or not
    ISP_ED_BUFQUE_CTRL_CLAER_ALL,              // 6,free all recored dequeued buffer
    ISP_ED_BUFQUE_CTRL_MAX
}ISP_ED_BUFQUE_CTRL_ENUM;

typedef struct
{
    ISP_ED_BUFQUE_CTRL_ENUM ctrl;
    unsigned int            processID;
    unsigned int            callerID;
    int                     p2burstQIdx;
    int                     p2dupCQIdx;
    unsigned int            timeoutUs;
}ISP_ED_BUFQUE_STRUCT_FRMB;

typedef enum
{
    ISP_ED_BUF_STATE_NONE  =-1,
    ISP_ED_BUF_STATE_ENQUE=0,
    ISP_ED_BUF_STATE_RUNNING,
    ISP_ED_BUF_STATE_WAIT_DEQUE_FAIL,
    ISP_ED_BUF_STATE_DEQUE_SUCCESS,
    ISP_ED_BUF_STATE_DEQUE_FAIL
}ISP_ED_BUF_STATE_ENUM;


/********************************************************************************************
 pass1 real time buffer control use cq0c
********************************************************************************************/
//
//#define _rtbc_use_cq0c_ //defined in camera_isp.h

#define _MAGIC_NUM_ERR_HANDLING_

#if defined(_rtbc_use_cq0c_)
//
#if 0 //defined in camera_isp.h
typedef volatile union _CQ_RTBC_FBC_
{
    volatile struct
    {
        unsigned long FBC_CNT                   : 4;
        unsigned long rsv_4                     : 7;
        unsigned long RCNT_INC                  : 1;
        unsigned long rsv_12                    : 2;
        unsigned long FBC_EN                    : 1;
        unsigned long LOCK_EN                   : 1;
        unsigned long FB_NUM                    : 4;
        unsigned long RCNT                      : 4;
        unsigned long WCNT                      : 4;
        unsigned long DROP_CNT                  : 4;
    } Bits;
    unsigned long Reg_val;
}CQ_RTBC_FBC;

#endif

typedef struct _cq_info_rtbc_st_frmb_
{
    CQ_CMD_ST imgo_frmb;
    CQ_CMD_ST img2o_frmb;//rrzo
    CQ_CMD_ST next_cq0ci_frmb;
    CQ_CMD_ST end_frmb;
    unsigned long imgo_base_pAddr_frmb;
    unsigned long img2o_base_pAddr_frmb;//rrzo
    signed int imgo_buf_idx_frmb; //used for replace buffer
    signed int img2o_buf_idx_frmb; //used for replace buffer//rrzo
}CQ_INFO_RTBC_ST_FRMB;

typedef struct _cq_ring_cmd_st_frmb_
{
    CQ_INFO_RTBC_ST_FRMB cq_rtbc_frmb;
    unsigned long next_pa_frmb;
    struct _cq_ring_cmd_st_frmb_ *pNext_frmb;
}CQ_RING_CMD_ST_FRMB;

typedef struct _cq_rtbc_ring_st_frmb_
{
    CQ_RING_CMD_ST_FRMB rtbc_ring_frmb[ISP_RT_CQ0C_BUF_SIZE];
    unsigned long   imgo_ring_size_frmb;
    unsigned long   img2o_ring_size_frmb;//rrzo
}CQ_RTBC_RING_ST_FRMB;

#endif

//CQ0B for AE smoothing, set obc_gain0~3
typedef struct _cq0b_info_rtbc_st_frmb_
{
    CQ_CMD_ST ob_frmb;
    CQ_CMD_ST end_frmb;
}CQ0B_INFO_RTBC_ST_FRMB;

typedef struct _cq0b_ring_cmd_st_frmb_
{
    CQ0B_INFO_RTBC_ST_FRMB cq0b_rtbc_frmb;
    unsigned long next_pa_frmb;
    struct _cq0b_ring_cmd_st_frmb_ *pNext_frmb;
}CQ0B_RING_CMD_ST_FRMB;

typedef struct _cq0b_rtbc_ring_st_frmb_
{
    CQ0B_RING_CMD_ST_FRMB rtbc_ring_frmb;
}CQ0B_RTBC_RING_ST_FRMB;

//
/********************************************************************************************

********************************************************************************************/


/*******************************************************************************
*
********************************************************************************/

typedef enum
{
    //ISP_CMD_RESET,          //Reset
    //ISP_CMD_RESET_BUF,
    //ISP_CMD_READ_REG,       //Read register from driver
    //ISP_CMD_WRITE_REG,      //Write register to driver
    //ISP_CMD_HOLD_TIME,
    //ISP_CMD_HOLD_REG,       //Hold reg write to hw, on/off
    //ISP_CMD_WAIT_IRQ,       //Wait IRQ
    ///ISP_CMD_READ_IRQ,       //Read IRQ
    //ISP_CMD_CLEAR_IRQ,      //Clear IRQ
    //ISP_CMD_DUMP_REG,       //Dump ISP registers , for debug usage
    //ISP_CMD_SET_USER_PID,   //for signal
    //ISP_CMD_RT_BUF_CTRL,   //for pass buffer control
    //ISP_CMD_REF_CNT,        //get imem reference count
    //ISP_CMD_DEBUG_FLAG,      //Dump message level
    //ISP_CMD_SENSOR_FREQ_CTRL      // sensor frequence control

    ISP_CMD_REGISTER_IRQ_FRMB = ISP_CMD_SENSOR_FREQ_CTRL+1 ,    //register for a specific irq
    ISP_CMD_DEBUG_FLAG_FRMB,
    ISP_CMD_UNREGISTER_IRQ_FRMB,  //unregister for a specific irq
    ISP_CMD_WAIT_IRQ_FRMB,       //Wait IRQ
    ISP_CMD_ED_QUEBUF_CTRL_FRMB,
    ISP_CMD_UPDATE_REGSCEN_FRMB,
    ISP_CMD_QUERY_REGSCEN_FRMB,
    ISP_CMD_UPDATE_BURSTQNUM_FRMB,
    ISP_CMD_QUERY_BURSTQNUM_FRMB,
    ISP_CMD_DUMP_ISR_LOG_FRMB,   //dump isr log
    ISP_CMD_GET_CUR_SOF_FRMB,
    ISP_CMD_GET_DMA_ERR_FRMB,
    ISP_CMD_GET_INT_ERR_FRMB,
#ifdef T_STAMP_2_0
    ISP_CMD_SET_FPS_FRMB,
#endif
    ISP_CMD_REGISTER_IRQ_USER_KEY,              /* register for a user key to do irq operation */
    ISP_CMD_MARK_IRQ_REQUEST,                    /* mark for a specific register befor wait for the interrupt if needed */
    ISP_CMD_GET_MARK2QUERY_TIME,             /* query time information between read and mark */
    ISP_CMD_FLUSH_IRQ_REQUEST,                    /* flush signal */
    ISP_CMD_SET_CAM_VERSION,                    /* set camera version */
    ISP_CMD_GET_DROP_FRAME_FRMB,            //dump current frame informaiton, 1 for drop frmae, 2 for last working frame
}ISP_CMD_ENUM_FRMB;
//
//#define ISP_RESET           _IO  (ISP_MAGIC, ISP_CMD_RESET)
//#define ISP_RESET_BUF       _IO  (ISP_MAGIC, ISP_CMD_RESET_BUF)
//#define ISP_READ_REGISTER   _IOWR(ISP_MAGIC, ISP_CMD_READ_REG,      ISP_REG_IO_STRUCT)
//#define ISP_WRITE_REGISTER  _IOWR(ISP_MAGIC, ISP_CMD_WRITE_REG,     ISP_REG_IO_STRUCT)
//#define ISP_HOLD_REG_TIME   _IOW (ISP_MAGIC, ISP_CMD_HOLD_TIME,     ISP_HOLD_TIME_ENUM)
//#define ISP_HOLD_REG        _IOW (ISP_MAGIC, ISP_CMD_HOLD_REG,      bool)
//#define ISP_WAIT_IRQ        _IOW (ISP_MAGIC, ISP_CMD_WAIT_IRQ,      ISP_WAIT_IRQ_STRUCT)
//#define ISP_READ_IRQ        _IOR (ISP_MAGIC, ISP_CMD_READ_IRQ,      ISP_READ_IRQ_STRUCT)
//#define ISP_CLEAR_IRQ       _IOW (ISP_MAGIC, ISP_CMD_CLEAR_IRQ,     ISP_CLEAR_IRQ_STRUCT)
//#define ISP_DUMP_REG        _IO  (ISP_MAGIC, ISP_CMD_DUMP_REG)
//#define ISP_SET_USER_PID    _IOW (ISP_MAGIC, ISP_CMD_SET_USER_PID,    unsigned long)
//#define ISP_BUFFER_CTRL     _IOWR(ISP_MAGIC, ISP_CMD_RT_BUF_CTRL,    ISP_BUFFER_CTRL_STRUCT)
//#define ISP_REF_CNT_CTRL    _IOWR(ISP_MAGIC, ISP_CMD_REF_CNT,       ISP_REF_CNT_CTRL_STRUCT)
#define ISP_DEBUG_FLAG_FRMB      _IOW (ISP_MAGIC, ISP_CMD_DEBUG_FLAG_FRMB,    unsigned long)
//#define ISP_SENSOR_FREQ_CTRL  _IOW (ISP_MAGIC, ISP_CMD_SENSOR_FREQ_CTRL,    unsigned long)
#define ISP_REGISTER_IRQ_FRMB    _IOW (ISP_MAGIC, ISP_CMD_REGISTER_IRQ_FRMB,  ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_UNREGISTER_IRQ_FRMB    _IOW (ISP_MAGIC, ISP_CMD_UNREGISTER_IRQ_FRMB,  ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_WAIT_IRQ_FRMB               _IOW (ISP_MAGIC, ISP_CMD_WAIT_IRQ_FRMB,  ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_ED_QUEBUF_CTRL_FRMB     _IOWR (ISP_MAGIC, ISP_CMD_ED_QUEBUF_CTRL_FRMB, ISP_ED_BUFQUE_STRUCT_FRMB)
#define ISP_UPDATE_REGSCEN_FRMB     _IOWR (ISP_MAGIC, ISP_CMD_UPDATE_REGSCEN_FRMB, unsigned int)
#define ISP_QUERY_REGSCEN_FRMB     _IOR (ISP_MAGIC, ISP_CMD_QUERY_REGSCEN_FRMB, unsigned int)
#define ISP_UPDATE_BURSTQNUM_FRMB _IOW(ISP_MAGIC,ISP_CMD_UPDATE_BURSTQNUM_FRMB, int)
#define ISP_QUERY_BURSTQNUM_FRMB _IOR (ISP_MAGIC,ISP_CMD_QUERY_BURSTQNUM_FRMB, int)
#define ISP_DUMP_ISR_LOG_FRMB    _IO  (ISP_MAGIC, ISP_CMD_DUMP_ISR_LOG_FRMB)
#define ISP_GET_CUR_SOF_FRMB     _IOR (ISP_MAGIC, ISP_CMD_GET_CUR_SOF_FRMB,      unsigned long)
#define ISP_GET_DMA_ERR_FRMB     _IOWR (ISP_MAGIC, ISP_CMD_GET_DMA_ERR_FRMB,      unsigned int)
#define ISP_GET_INT_ERR_FRMB     _IOR    (ISP_MAGIC, ISP_CMD_GET_INT_ERR_FRMB,      unsigned long)
#ifdef T_STAMP_2_0
    #define ISP_SET_FPS_FRMB     _IOW (ISP_MAGIC, ISP_CMD_SET_FPS_FRMB,    unsigned int)
#endif
#define ISP_GET_DROP_FRAME_FRMB  _IOWR    (ISP_MAGIC, ISP_CMD_GET_DROP_FRAME_FRMB,      unsigned int)
#define ISP_REGISTER_IRQ_USER_KEY  _IOR(ISP_MAGIC,ISP_CMD_REGISTER_IRQ_USER_KEY, ISP_REGISTER_USERKEY_STRUCT_FRMB)
#define ISP_MARK_IRQ_REQUEST         _IOWR(ISP_MAGIC,ISP_CMD_MARK_IRQ_REQUEST,ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_GET_MARK2QUERY_TIME  _IOWR(ISP_MAGIC,ISP_CMD_GET_MARK2QUERY_TIME,ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_FLUSH_IRQ_REQUEST         _IOW(ISP_MAGIC,ISP_CMD_FLUSH_IRQ_REQUEST,ISP_WAIT_IRQ_STRUCT_FRMB)
#define ISP_SET_CAM_VERSION         _IOW(ISP_MAGIC,ISP_CMD_SET_CAM_VERSION, bool)

//


//
//int32_t ISP_MDPClockOnCallback(uint64_t engineFlag);
//int32_t ISP_MDPDumpCallback(uint64_t engineFlag, int level);
//int32_t ISP_MDPResetCallback(uint64_t engineFlag);
//int32_t ISP_MDPClockOffCallback(uint64_t engineFlag);
//int32_t ISP_BeginGCECallback(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress);
//int32_t ISP_EndGCECallback(uint32_t taskID, uint32_t regCount, uint32_t *regValues);
//

//basically , should separate into p1/p1_d/p2/camsv/camsv_d,
//currently, only use camsv/camsv_d/others
typedef enum _eISPIrq
{
    _IRQ            = 0,
    _IRQ_D          = 1,
    _CAMSV_IRQ      = 2,
    _CAMSV_D_IRQ    = 3,
    _IRQ_MAX        = 4,
}eISPIrq;
//

//static int ISP_ED_BufQue_CTRL_FUNC(ISP_ED_BUFQUE_STRUCT param);
//static int ISP_RTBC_ENQUE_FRMB(int dma,ISP_RT_BUF_INFO_STRUCT* prt_buf_info);
//static int ISP_RTBC_DEQUE_FRMB(int dma,ISP_DEQUE_BUF_INFO_STRUCT*    pdeque_buf);
//static long ISP_Buf_CTRL_FUNC_FRMB(unsigned int Param);
//static int ISP_SOF_Buf_Get_FRMB(eISPIrq irqT,unsigned long long sec,unsigned long usec,bool bDrop);
//static int ISP_DONE_Buf_Time_FrmB(eISPIrq irqT,unsigned long long sec,unsigned long usec);
//static int ISP_WaitIrq_FrmB(ISP_WAIT_IRQ_STRUCT_FRMB* WaitIrq);
//

#endif

