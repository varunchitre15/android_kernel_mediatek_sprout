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

#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include "tpd_custom_ft6x06.h"

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#if defined(MT6577)
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#elif defined(MT6575)
#include <mach/mt6575_pm_ldo.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_boot.h>
#elif defined(CONFIG_ARCH_MT6573)
#include <mach/mt6573_boot.h>
#endif

#if 0
#define    FTS_PACKET_LENGTH               128
#define    FTS_PACKET_NO_DMA_LENGTH        128
#else
#define    FTS_PACKET_LENGTH               2  // 2 data length
#define    FTS_PACKET_NO_DMA_LENGTH        2  // 2
#endif

static unsigned char *tpDMABuf_va = NULL;
static u32 tpDMABuf_pa = 0;

static tinno_ts_data *g_pts = NULL;

int fts_write_reg(u8 addr, u8 para)
{
    int rc;
    char buf[3];

    buf[0] = addr;
    buf[1] = para;
    mutex_lock(&g_pts->mutex);
    rc = i2c_master_send(g_pts->client, buf, 2);
    mutex_unlock(&g_pts->mutex);
    return rc;
}

int fts_read_reg(u8 addr, unsigned char *pdata)
{
    int rc;
    unsigned char buf[2];

    buf[0] = addr;               //register address

    mutex_lock(&g_pts->mutex);
    i2c_master_send(g_pts->client, &buf[0], 1);
    rc = i2c_master_recv(g_pts->client, &buf[0], 1);
    mutex_unlock(&g_pts->mutex);

    if (rc < 0)
        pr_err("msg %s i2c read error: %d\n", __func__, rc);

    *pdata = buf[0];
    return rc;
}

u8 fts_cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
    int rc;
    u8 write_cmd[4] = {0};
    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    mutex_lock(&g_pts->mutex);
    g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;
    rc = i2c_master_send(g_pts->client, write_cmd, num);
    mutex_unlock(&g_pts->mutex);
    return rc;
}

static ssize_t fts_dma_write_m_byte(unsigned char*returnData_va, u32 returnData_pa, int  len)
{
    int     rc=0;
    if (len > 0){
        mutex_lock(&g_pts->mutex);
        g_pts->client->addr = (g_pts->client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
        rc = i2c_master_send(g_pts->client, returnData_pa, len);
        g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;
        mutex_unlock(&g_pts->mutex);
        if (rc < 0) {
            printk(KERN_ERR"xxxxfocal write data error!! xxxx\n");
            return 0;
        }
    }
    return 1;
}

static ssize_t fts_dma_read_m_byte(unsigned char cmd, unsigned char*returnData_va, u32 returnData_pa,unsigned char len)
{
    int rc=0;
    unsigned char start_addr = cmd;

    mutex_lock(&g_pts->mutex);
    rc = i2c_master_send(g_pts->client, &start_addr, 1);
    if (rc < 0) {
        printk(KERN_ERR"xxxx focal sends command error!! xxxx\n");
        goto err;
    }

    if (len > 0){
        g_pts->client->addr = (g_pts->client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
        rc = i2c_master_recv(g_pts->client, returnData_pa, len);
        g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;
        if (rc < 0) {
            printk(KERN_ERR"xxxx focal reads data error!! xxxx\n");
            goto err;
        }
    }


    mutex_unlock(&g_pts->mutex);
    return 1;
err:
    mutex_unlock(&g_pts->mutex);
    return 0;
}

#if 1
static ssize_t fts_bin_write_block_no_dma(uint8_t* pbt_buf, int dw_lenth)
{
    unsigned char reg_val[3] = {0};
    int i = 0, j;
    char buf[256] = {0};
    int  packet_number;
    int  temp;
    int  lenght;
    unsigned char  packet_buf[FTS_PACKET_LENGTH + 6];
    unsigned char bt_ecc;
    int ret;
    bt_ecc = 0;
    CTP_DBG("start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    CTP_DBG("####Packet length = 0x %x\n", dw_lenth);
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    buf[0] = 0xbf;
    buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_NO_DMA_LENGTH;
        buf[2] = (u8)(temp>>8);
        buf[3] = (u8)temp;
        lenght = FTS_PACKET_NO_DMA_LENGTH;
        buf[4] = (u8)(lenght>>8);
        buf[5] = (u8)lenght;

        for (i=0;i<FTS_PACKET_NO_DMA_LENGTH;i++)
        {
            buf[6+i] = pbt_buf[j*FTS_PACKET_NO_DMA_LENGTH + i];
            bt_ecc ^= buf[6+i];
        }

        ret = i2c_master_send(g_pts->client, buf, FTS_PACKET_NO_DMA_LENGTH + 6);
            if (ret < 0) {
                CTP_DBG("[Error]TP write data error!! packet_number=%d\n", j);
                return 0;
            }
        mdelay(1); //LINE <tp> <DATE20130607> <tp upgrade delay> zhangxiaofei
        if ((j * FTS_PACKET_NO_DMA_LENGTH % 2048) == 0)
        {
            CTP_DBG("upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_NO_DMA_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_NO_DMA_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_NO_DMA_LENGTH;
        buf[2] = (u8)(temp>>8);
        buf[3] = (u8)temp;

        temp = (dw_lenth) % FTS_PACKET_NO_DMA_LENGTH;
        buf[4] = (u8)(temp>>8);
        buf[5] = (u8)temp;

        for (i=0;i<temp;i++)
        {
            buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_NO_DMA_LENGTH + i];
            bt_ecc ^= buf[6+i];
        }

        ret = i2c_master_send(g_pts->client, buf, temp+6);
           if (ret < 0) {
                CTP_DBG("[Error]TP write data error!! temp=%d\n", temp);
                return 0;
            }

        g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;

        mdelay(30);
    }

    //send the last six byte
    for (i = 0; i<6; i++)
    {
        packet_buf[0] = 0xbf;
        packet_buf[1] = 0x00;
        temp = 0x6ffa + i;
        packet_buf[2] = (u8)(temp>>8);
        packet_buf[3] = (u8)temp;
        temp =1;
        packet_buf[4] = (u8)(temp>>8);
        packet_buf[5] = (u8)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i];
        bt_ecc ^= packet_buf[6];

        i2c_master_send(g_pts->client, packet_buf, 7);

        mdelay(40);
    }

    /*********read out checksum***********************/
    /*send the operation head*/
    fts_cmd_write(0xcc,0x00,0x00,0x00,1);
    g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;
    i2c_master_recv(g_pts->client, &reg_val, 1);
    CTP_DBG("ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
        CTP_DBG("check sum error!!\n");
        return 5;
    }

    return 0;
}

static ssize_t fts_bin_write_block(uint8_t* pbt_buf, int dw_lenth)
{
    unsigned char reg_val[3] = {0};
    int i = 0, j;

    int  packet_number;
    int  temp;
    int  lenght;
    unsigned char  packet_buf[FTS_PACKET_LENGTH + 6];
    unsigned char bt_ecc;

    bt_ecc = 0;
    CTP_DBG("[FT520X]: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    CTP_DBG("####Packet length = 0x %x\n", dw_lenth);
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    tpDMABuf_va[0] = 0xbf;
    tpDMABuf_va[1] = 0x00;
    for (j=0;j<packet_number;j++){
        temp = j * FTS_PACKET_LENGTH;
        tpDMABuf_va[2] = (u8)(temp>>8);
        tpDMABuf_va[3] = (u8)temp;
        lenght = FTS_PACKET_LENGTH;
        tpDMABuf_va[4] = (u8)(lenght>>8);
        tpDMABuf_va[5] = (u8)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++){
            tpDMABuf_va[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i];
            bt_ecc ^= tpDMABuf_va[6+i];
        }

        fts_dma_write_m_byte(tpDMABuf_va, tpDMABuf_pa, FTS_PACKET_LENGTH + 6);
        mdelay(50);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0){
            CTP_DBG("[FT520X] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0){
        temp = packet_number * FTS_PACKET_LENGTH;
        tpDMABuf_va[2] = (u8)(temp>>8);
        tpDMABuf_va[3] = (u8)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        tpDMABuf_va[4] = (u8)(temp>>8);
        tpDMABuf_va[5] = (u8)temp;

        for (i=0;i<temp;i++){
            tpDMABuf_va[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i];
            bt_ecc ^= tpDMABuf_va[6+i];
        }

        fts_dma_write_m_byte(tpDMABuf_va, tpDMABuf_pa,temp+6);
        g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;

        mdelay(30);
    }

    //send the last six byte
    for (i = 0; i<6; i++){
        packet_buf[0] = 0xbf;
        packet_buf[1] = 0x00;
        temp = 0x6ffa + i;
        packet_buf[2] = (u8)(temp>>8);
        packet_buf[3] = (u8)temp;
        temp =1;
        packet_buf[4] = (u8)(temp>>8);
        packet_buf[5] = (u8)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i];
        bt_ecc ^= packet_buf[6];

        i2c_master_send(g_pts->client, packet_buf, 7);

        mdelay(40);
    }

    /*********read out checksum***********************/
    /*send the operation head*/
    fts_cmd_write(0xcc,0x00,0x00,0x00,1);
    g_pts->client->addr = g_pts->client->addr & I2C_MASK_FLAG;
    i2c_master_recv(g_pts->client, &reg_val, 1);
    CTP_DBG("ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc){
        CTP_DBG("check sum error!!\n");
        return 5;
    }

    return 0;
}

int fts_i2c_write_block( u8 *txbuf, int len )
{
    //if ( 5 == fts_bin_write_block(txbuf, len) ){
    if ( 5 == fts_bin_write_block_no_dma(txbuf, len) ){
           CTP_DBG("[Error]!! \n");
        return 5;
    }
    return len;
}
#else
int fts_i2c_write_block( u8 *txbuf, int len )
{
    char    write_data[8] = {0};
    int    i,ret=0;

    if(len == 0) {
        CTP_DBG("[Error]TP Write Len should not be zero!! \n");
        return 0;
    }

    //Driver does not allow (single write length > 8)
    while(len > 8)
    {
        for (i = 0; i<7; i++){
            write_data[i] = *(txbuf+i);
        }
        ret = i2c_master_send(g_pts->client, write_data, 8);
        if (ret < 0) {
            CTP_DBG("[Error]TP reads data error!! \n");
            return 0;
        }
        txbuf+=8;
        len -= 8;
    }
    if (len > 0){
        for (i = 0; i<len; i++){
            write_data[i] = *(txbuf+i);
        }
        ret = i2c_master_send(g_pts->client, write_data, len);
        if (ret < 0) {
            CTP_DBG("[Error]TP reads data error!! \n");
            return 0;
        }
    }

    return len;
}
#endif

#ifdef MT6577
#if 0

static int fts_read_block( tinno_ts_data *ts, u8 start_addr, u8 *rxbuf, u8 len )
{
    u8 retry;
    u16 left = len;
    u16 offset = 0;
    u8 addr = start_addr;

    if ( rxbuf == NULL )
        return -1;

    CTP_DBG("device %02X address %04X len %d\n", ts->client->addr, addr, len );

    mutex_lock(&g_pts->mutex);
    while ( left > 0 ){
        if ( left > MAX_TRANSACTION_LENGTH ){
            addr = addr + offset;

            ts->client->addr = ts->client->addr & I2C_MASK_FLAG;

            retry = 0;
            while ( i2c_master_send(ts->client, &addr, 1 ) < 0 ){
                retry++;
                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    mutex_unlock(&g_pts->mutex);
                    CTP_DBG("I2C read 0x%X length=%d failed\n", addr + offset, 1);
                    return -1;
                }
            }

            retry = 0;
            while ( i2c_master_recv(ts->client, &rxbuf[offset], MAX_TRANSACTION_LENGTH ) < 0 ){
                retry++;
                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    mutex_unlock(&g_pts->mutex);
                    CTP_DBG("I2C read 0x%X length=%d failed\n", addr + offset, MAX_TRANSACTION_LENGTH);
                    return -1;
                }
            }

            left -= MAX_TRANSACTION_LENGTH;
            offset += MAX_TRANSACTION_LENGTH;
        }
        else{
            addr = addr + offset;

            ts->client->addr = ts->client->addr & I2C_MASK_FLAG;

            retry = 0;
            while ( i2c_master_send(ts->client, &addr, 1 ) < 0 ){
                retry++;
                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    mutex_unlock(&g_pts->mutex);
                    CTP_DBG("I2C write 0x%X length=%d failed\n", addr + offset, 1);
                    return -1;
                }
            }

            retry = 0;
            while ( i2c_master_recv(ts->client, &rxbuf[offset], left ) < 0 ){
                retry++;
                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    CTP_DBG("I2C read 0x%X length=%d failed\n", addr + offset, left);
                    mutex_unlock(&g_pts->mutex);
                    return -1;
                }
            }

            offset += left;
            left = 0;
        }
    }
    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;

    mutex_unlock(&g_pts->mutex);
    return 0;
}
#else

static int fts_read_block( tinno_ts_data *ts, u8 addr, u8 *rxbuf, u8 len )
{
    u8 retry;
    u16 left = len;
    u16 offset = 0;

    if ( rxbuf == NULL )
        return -1;

    CTP_DBG("device 0x%02X address %04X len %d\n", ts->client->addr, addr, len );

    mutex_lock(&g_pts->mutex);
    while ( left > 0 ){
        if ( left > MAX_I2C_TRANSFER_SIZE ){
            rxbuf[offset] = ( addr+offset ) & 0xFF;
            ts->client->addr = ts->client->addr & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;
            retry = 0;
            while ( i2c_master_send(ts->client, &rxbuf[offset], (MAX_I2C_TRANSFER_SIZE << 8 | 1)) < 0 ){
                retry++;

                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    mutex_unlock(&g_pts->mutex);
                    CTP_DBG("I2C read 0x%X length=%d failed\n", addr + offset, MAX_I2C_TRANSFER_SIZE);
                    return -1;
                }
            }
            left -= MAX_I2C_TRANSFER_SIZE;
            offset += MAX_I2C_TRANSFER_SIZE;
        }
        else{
            rxbuf[offset] = ( addr+offset ) & 0xFF;
            ts->client->addr = ts->client->addr & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;

            retry = 0;
            while ( i2c_master_send(ts->client, &rxbuf[offset], (left << 8 | 1)) < 0 ){
                retry++;

                if ( retry == 5 ){
                    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;
                    mutex_unlock(&g_pts->mutex);
                    CTP_DBG("I2C read 0x%X length=%d failed\n", addr + offset, left);
                    return -1;
                }
            }
            offset += left;
            left = 0;
        }
    }
    ts->client->addr = ts->client->addr & I2C_MASK_FLAG;

    mutex_unlock(&g_pts->mutex);
    return 0;
}
#endif
#else
static int fts_read_block( tinno_ts_data *ts, u8 start_addr, u8 *rxbuf, u8 len )
{
    u8 reg_addr;
    u8 retry;
    u16 left = len;
    u16 offset = 0;

    struct i2c_msg msg[2] =
    {
        {
            .addr = ts->client->addr,
            .flags = 0,
            .buf = &reg_addr,
            .len = 1,
            .timing = I2C_MASTER_CLOCK
        },
        {
            .addr = ts->client->addr,
            .flags = I2C_M_RD,
            .timing = I2C_MASTER_CLOCK
        },
    };

    if ( rxbuf == NULL )
        return -1;

    mutex_lock(&g_pts->mutex);
    while ( left > 0 ){
        reg_addr = ( start_addr+offset ) & 0xFF;

        msg[1].buf = &rxbuf[offset];

        if ( left > MAX_TRANSACTION_LENGTH ){
            msg[1].len = MAX_TRANSACTION_LENGTH;
            left -= MAX_TRANSACTION_LENGTH;
            offset += MAX_TRANSACTION_LENGTH;
        }
        else{
            msg[1].len = left;
            left = 0;
        }

        retry = 0;

        while ( i2c_transfer( ts->client->adapter, &msg[0], 2 ) != 2 ){
            retry++;
            if ( retry == 20 ){
                mutex_unlock(&g_pts->mutex);
                CTP_DBG("I2C read 0x%X length=%d failed\n", start_addr + offset, len);
                return -1;
            }
        }
    }

    mutex_unlock(&g_pts->mutex);
    return 0;
}
#endif

 int tpd_read_touchinfo(tinno_ts_data *ts)
 {
    int ret = 0;
    memset((void *)ts->buffer, FTS_INVALID_DATA, FTS_PROTOCOL_LEN);
#ifndef MT6577
    ret = fts_dma_read_m_byte(ts->start_reg, tpDMABuf_va, tpDMABuf_pa, FTS_PROTOCOL_LEN);
    if ( !ret ) {
        CTP_DBG("i2c_transfer failed, (%d)", ret);
        return -EAGAIN;
    }
    memcpy((void *)ts->buffer, tpDMABuf_va, FTS_PROTOCOL_LEN);
#else
    ret = fts_read_block(ts,ts->start_reg, ts->buffer, FTS_PROTOCOL_LEN);
    if ( ret ) {
        CTP_DBG("i2c_transfer failed, (%d)", ret);
        return -EAGAIN;
    }
#endif
    return 0;
}

int fts_iic_init( tinno_ts_data *ts )
{
    tpDMABuf_va = (unsigned char *)dma_alloc_coherent(NULL, 4096, &tpDMABuf_pa, GFP_KERNEL);
    if(!tpDMABuf_va){
        printk(KERN_ERR"xxxx Allocate DMA I2C Buffer failed!xxxx\n");
        return -1;
    }
    g_pts = ts;
    return 0;
}

