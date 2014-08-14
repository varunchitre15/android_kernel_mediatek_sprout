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

#ifndef AUDDRV_BTCVSD_H
#define AUDDRV_BTCVSD_H

#include <mach/mt_typedefs.h>
#include "AudioBTCVSDDef.h"

#undef DEBUG_AUDDRV
#ifdef DEBUG_AUDDRV
#define PRINTK_AUDDRV(format, args...) printk(format, ##args )
#else
#define PRINTK_AUDDRV(format, args...)
#endif

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/


/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

typedef enum
{
    BT_SCO_TXSTATE_IDLE = 0x0,
    BT_SCO_TXSTATE_INIT,
    BT_SCO_TXSTATE_READY,
    BT_SCO_TXSTATE_RUNNING,
    BT_SCO_TXSTATE_ENDING,
    BT_SCO_RXSTATE_IDLE = 0x10,
    BT_SCO_RXSTATE_INIT,
    BT_SCO_RXSTATE_READY,
    BT_SCO_RXSTATE_RUNNING,
    BT_SCO_RXSTATE_ENDING,
    BT_SCO_TXSTATE_DIRECT_LOOPBACK
} CVSD_STATE;

typedef enum
{
    BT_SCO_DIRECT_BT2ARM,
    BT_SCO_DIRECT_ARM2BT
} BT_SCO_DIRECT;


typedef enum
{
    BT_SCO_CVSD_30 = 0,
    BT_SCO_CVSD_60 = 1,
    BT_SCO_CVSD_90 = 2,
    BT_SCO_CVSD_120 = 3,
    BT_SCO_CVSD_10 = 4,
    BT_SCO_CVSD_20 = 5,
    BT_SCO_CVSD_MAX = 6
} BT_SCO_PACKET_LEN;


typedef struct
{
    kal_uint32 pucTXPhysBufAddr;
    kal_uint32 pucRXPhysBufAddr;
    kal_uint8 *pucTXVirtBufAddr;
    kal_uint8 *pucRXVirtBufAddr;
    kal_int32 u4TXBufferSize;
    kal_int32 u4RXBufferSize;
} CVSD_MEMBLOCK_T;


typedef struct
{
    kal_uint8     PacketBuf[SCO_RX_PACKER_BUF_NUM][SCO_RX_PLC_SIZE + BTSCO_CVSD_PACKET_VALID_SIZE];
    kal_bool        PacketValid[SCO_RX_PACKER_BUF_NUM];
    kal_int32    iPacket_w;
    kal_int32    iPacket_r;
    kal_uint8     TempPacketBuf[BT_SCO_PACKET_180];
    kal_bool      fOverflow;
    kal_uint32      u4BufferSize;   //RX packetbuf size
} BT_SCO_RX_T;


typedef struct
{
    kal_uint8     PacketBuf[SCO_TX_PACKER_BUF_NUM][SCO_TX_ENCODE_SIZE];
    kal_int32    iPacket_w;
    kal_int32    iPacket_r;
    kal_uint8     TempPacketBuf[BT_SCO_PACKET_180];
    kal_bool      fUnderflow;
    kal_uint32      u4BufferSize; //TX packetbuf size
} BT_SCO_TX_T;

CVSD_MEMBLOCK_T BT_CVSD_Mem;

// here is temp address for ioremap BT hardware register
void *BTSYS_PKV_BASE_ADDRESS = 0;
void *BTSYS_SRAM_BANK2_BASE_ADDRESS = 0;

#endif


