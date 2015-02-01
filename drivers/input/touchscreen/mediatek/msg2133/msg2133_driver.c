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

#include "tpd_custom_msg2133.h"

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#include "cust_gpio_usage.h"

#include <linux/dma-mapping.h>

#ifdef TP_PROXIMITY_SENSOR
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif


#define DIJIN_FW_MAJOR  0x2
#define TRULY_FW_MAJOR  0x4
#define FN_FW_MAJOR  0x1


#ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

extern struct tpd_device *tpd;

static struct i2c_client *i2c_client = NULL;
static struct task_struct *thread = NULL;

#ifdef TP_PROXIMITY_SENSOR
static u8 tpd_proximity_flag = 1;
static u8 tpd_proximity_detect = 1;//0-->close ; 1--> far away
#endif

#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->> "fmt"\n",##arg)
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->> "fmt"\n",##arg)
#define GTP_DEBUG(fmt,arg...)          do{\
                                         printk("<<-GTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)

static DECLARE_WAIT_QUEUE_HEAD(waiter);


static void tpd_eint_interrupt_handler(void);

static struct tag_para_touch_ssb_data_single touch_ssb_data = {0};

extern  int  fix_tp_proc_info(void  *tp_data, u8 data_len);


#ifdef MT6575 
 extern void mt65xx_eint_unmask(unsigned int line);
 extern void mt65xx_eint_mask(unsigned int line);
 extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
 extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
 extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
									  kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
									  kal_bool auto_umask);
#endif
#ifdef MT6577
	extern void mt65xx_eint_unmask(unsigned int line);
	extern void mt65xx_eint_mask(unsigned int line);
	extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
	extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
	extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
#endif

static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);

static unsigned short  tp_ic_major, tp_ic_minor,ic_fw_version;

static int tpd_flag = 0;
static int point_num = 0;
static int p_point_num = 0;


#define TPD_OK 0
// debug macros
#if defined(TP_DEBUG)
#define SSL_PRINT(x...)		printk("MSG2133:"x)
#else
#define SSL_PRINT(format, args...)  do {} while (0)
#endif

#ifdef TP_PROXIMITY_SENSOR
char ps_data_state[1] = {0};
enum
{
    DISABLE_CTP_PS,
    ENABLE_CTP_PS,
    RESET_CTP_PS
};
char tp_proximity_state = DISABLE_CTP_PS;  
#endif

struct touch_info
{
    unsigned short y[3];
    unsigned short x[3];
    unsigned short p[3];
    unsigned short count;
};

typedef struct
{
    unsigned short pos_x;
    unsigned short pos_y;
    unsigned short pos_x2;
    unsigned short pos_y2;
    unsigned short temp2;
    unsigned short temp;
    short dst_x;
    short dst_y;
    unsigned char checksum;

} SHORT_TOUCH_STATE;

struct msg_ts_priv 
{
    unsigned int buttons;
    int      prev_x[4];
    int	prev_y[4];
};

struct msg_ts_priv msg2133_priv;


 static const struct i2c_device_id msg2133_tpd_id[] = {{"msg2133",0},{}};
//unsigned short force[] = {TPD_I2C_NUMBER, TOUCH_ADDR_MSG20XX, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const forces[] = { force, NULL };
//static struct i2c_client_address_data addr_data = { .forces = forces, };

 static struct i2c_board_info __initdata msg2133_i2c_tpd={ I2C_BOARD_INFO("msg2133", (0x4c>>1))};

#ifdef RGK_TP_AUTO_UPGRADE_SUPPORT
#define AUTO_UPGRADE
#endif

#ifdef RGK_TP_NODE_UPGRAER_SUPPORT
#define NODE_UPGRADE
#endif


#ifdef RGK_TP_NODE_UPGRAER_SELF_APK_SUPPORT
#define SELF_APK_UPGRADE
#endif


#define APK_UPDATE

static struct class *firmware_class;
static struct device *firmware_cmd_dev;
#if defined (SELF_APK_UPGRADE)
static struct class *firmware_class_self_apk;
static struct device *firmware_cmd_dev_self_apk;
#endif
#define TOUCH_ADDR_MSG20XX   	0x4c >> 1
static int update_switch = 0;

#if 0//
#define FW_ADDR_MSG21XX   (0xC4)
#define FW_ADDR_MSG21XX_TP   (0x4C)
#define FW_UPDATE_ADDR_MSG21XX   (0x92)
#else
#define FW_ADDR_MSG21XX   (0xC4>>1)
#define FW_ADDR_MSG21XX_TP   (0x4C>>1)
#define FW_UPDATE_ADDR_MSG21XX   (0x92>>1)
#endif

#if 1
#define TP_DEBUG(format, ...)	printk(KERN_INFO "MSG2133_MSG21XXA_update_INFO ***" format "\n", ## __VA_ARGS__)
#define TP_DEBUG_ERR(format, ...)	printk(KERN_ERR "MSG2133_MSG21XXA_updating ***" format "\n", ## __VA_ARGS__)
#else
#define TP_DEBUG(format, ...)
#define TP_DEBUG_ERR(format, ...)
#endif

// add by anxiang.xiao for get specify TP info 2013-05-24
#define TP_PROC_BUFFER_MAX_SIZE 	512
unsigned char tp_proc_buffer[TP_PROC_BUFFER_MAX_SIZE];

int  fix_tp_proc_info(void  *tp_data, u8 data_len)
{
	int ret = 0;

	if (data_len > TP_PROC_BUFFER_MAX_SIZE ||  data_len <=0 || tp_data ==NULL)
			ret = -1;
	
	memcpy(tp_proc_buffer, tp_data,data_len);
	tp_proc_buffer[data_len] = '\0';

	return 0;
}
//end

//zhoulidong add
static char mtk_tpd_ic_name[32] = {0};

static int rgk_read_tp_info(char *page, char **start, off_t off,int count,
			  int *eof, void *data){
	ssize_t len = 0;
	if(off > 0)
	{
		*eof = 1;
		return 0;
	}
	// len = (ssize_t)sprintf(page, "TP IC:%s\n", mtk_tpd_ic_name);
	len = (ssize_t)sprintf(page, "%s", tp_proc_buffer);
	
	return len;
}

int rgk_creat_proc_tp_info(void)
{

	struct proc_dir_entry *tp_info_entry = NULL;
	
	tp_info_entry = create_proc_entry("rgk_tpInfo", 0444, NULL);
	if (tp_info_entry)
	{
			tp_info_entry->read_proc = rgk_read_tp_info;
			tp_info_entry->write_proc = NULL;
	}
	
	
	return 0;

}

//zhoulidong add end

static int HalTscrCReadI2CSeq(u8 addr, u8* read_data, u16 size)
{
    int ret;
    i2c_client->addr = addr;
    ret = i2c_master_recv(i2c_client, read_data, size);
    i2c_client->addr = FW_ADDR_MSG21XX_TP;
    
    if(ret <= 0)
    {
		TP_DEBUG_ERR("HalTscrCReadI2CSeq error %d,addr = %d\n", ret,addr);
	}
    return ret;
}

static int HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    int ret;
    i2c_client->addr = addr;
    ret = i2c_master_send(i2c_client, data, size);
    i2c_client->addr = FW_ADDR_MSG21XX_TP;

    if(ret <= 0)
    {
		TP_DEBUG_ERR("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", ret,addr);
	}
    return ret;
}
#if 1//adair:以下无需修改
/*
static void Get_Chip_Version(void)
{
    printk("[%s]: Enter!\n", __func__);
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[2];

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCE;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    if (dbbus_rx_data[1] == 0)
    {
        // it is Catch2
        TP_DEBUG(printk("*** Catch2 ***\n");)
        //FwVersion  = 2;// 2 means Catch2
    }
    else
    {
        // it is catch1
        TP_DEBUG(printk("*** Catch1 ***\n");)
        //FwVersion  = 1;// 1 means Catch1
    }

}
*/

static void dbbusDWIICEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
}

static void dbbusDWIICStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}


static void i2c_write(u8 addr, u8 *pbt_buf, int dw_lenth)
{
    int ret;
    i2c_client->addr = addr;
    i2c_client->addr|=I2C_ENEXT_FLAG;
    ret = i2c_master_send(i2c_client, pbt_buf, dw_lenth);
    i2c_client->addr = TOUCH_ADDR_MSG20XX;
    i2c_client->addr|=I2C_ENEXT_FLAG;

    if(ret <= 0)
    {
        printk("i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
    }
}

static void i2c_read(u8 addr, u8 *pbt_buf, int dw_lenth)
{
    int ret;
    i2c_client->addr = addr;
    i2c_client->addr|=I2C_ENEXT_FLAG;
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);
    i2c_client->addr = TOUCH_ADDR_MSG20XX;
    i2c_client->addr|=I2C_ENEXT_FLAG;

    if(ret <= 0)
    {
        printk("i2c_read_interface error line = %d, ret = %d\n", __LINE__, ret);
    }
}


static void get_ic_info(unsigned short *major, unsigned short *minor)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
#if 1	
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
#endif
    SSL_PRINT("\n");
    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x74;
	
    i2c_write(0x26, &dbbus_tx_data[0], 3);
  msleep(50);
    i2c_read(0x26, &dbbus_rx_data[0], 4);

    *major = (dbbus_rx_data[1] << 8) + dbbus_rx_data[0];
    *minor = (dbbus_rx_data[3] << 8) + dbbus_rx_data[2];
    SSL_PRINT("***major = %d ***\n", *major);
    SSL_PRINT("***minor = %d ***\n", *minor);
}


#if defined (APK_UPDATE) ||defined (AUTO_UPGRADE) || defined (NODE_UPGRADE) || defined (SELF_APK_UPGRADE)

#if defined (AUTO_UPGRADE) || defined (NODE_UPGRADE) || defined (SELF_APK_UPGRADE)
static unsigned char CTPM_FW_D206_DJ_48_10040_0336A[] = {
	#include "D206_DJ_48_10040_0336A_00_V2.48.i"
};
static unsigned char CTPM_FW_D208_DJ_48_10040_0359A[] = {
	#include "D206_DJ_48_10040_0336A_00_V2.48.i"
};
static unsigned char CTPM_FW_D208_DJ_S2_0374A[] = {
	#include "D208_S2_DJ_0374A_00_V2.50.i"
};
static unsigned char CTPM_FW_D206_FN_CS040X_RT01[] = {
	#include "D206_FN_CS040X-RT01_M505_C1_33.i"
};

static unsigned char CTPM_FW_D208_FN_CS040X_RT04[] = {
	#include "D209_FN_CS040X-RT04_C1.46.i"
};

static unsigned char CTPM_FW_D208_TRULY_CP1F0471[] = {
	#include "D208_TRULY_CP1F0471_v4.32.i"
};

unsigned short tp_i_file_major, tp_i_file_minor, i_file_fw_version;
#endif

#define U8 unsigned char
#define S8 signed char
#define U16 unsigned short
#define S16 signed short
#define U32 unsigned int
#define S32 signed int

static  char fw_version[10];
#define DWIIC_MODE_ISP    0
#define DWIIC_MODE_DBBUS  1
static U8 temp[94][1024];
static int FwDataCnt;
//static int FwVersion;


#define ENABLE_DMA      1
#if ENABLE_DMA
static u8 *gpDMABuf_va = NULL;
static u32 gpDMABuf_pa = NULL;
#endif

static u8 curr_ic_type = 0;
#define	CTP_ID_MSG21XX		1
#define	CTP_ID_MSG21XXA		2
#define	CTP_ID_UNKNOW		3
static unsigned short curr_ic_major=0;
static unsigned short curr_ic_minor=0;
//#define ENABLE_AUTO_UPDATA
#ifdef ENABLE_AUTO_UPDATA
static unsigned short update_bin_major=0;
static unsigned short update_bin_minor=0;
#endif

#ifdef RGK_TP_NODE_UPGRAER_WRITE_PERMISSION_OPEN
#define CTP_AUTHORITY 0777//0664
#else
#define CTP_AUTHORITY 0664
#endif

//u8  Fmr_Loader[1024];
u32 crc_tab[256];
static u8 g_dwiic_info_data[1024];   // Buffer for info data

#define N_BYTE_PER_TIME (8)//
#define UPDATE_TIMES (1024/N_BYTE_PER_TIME)



static void HalDisableIrq(void)
{
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, NULL, 1);
}

/*enable irq*/
static void HalEnableIrq(void)
{
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
}

/*reset the chip*/
static void _HalTscrHWReset(void)
{
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	msleep(100);
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(500);
}

static void dbbusDWIICIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);

    // Delay some interval to guard the next transaction
    udelay ( 150);//200 );        // delay about 0.2ms
}

static void drvISP_EntryIspMode(void)
{
    u8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };
	TP_DEBUG("\n******%s come in*******\n",__FUNCTION__);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
    udelay ( 150 );//200 );        // delay about 0.1ms
}

static u8 drvISP_Read(u8 n, u8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    u8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay( 800 );//200);
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[0];
        TP_DEBUG("dbbus=%d,%d===drvISP_Read=====\n",dbbus_rx_data[0],dbbus_rx_data[1]);
  	}
    else
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
    }

    return 0;
}

static void drvISP_WriteEnable(void)
{
    u8 bWriteData[2] =
    {
        0x10, 0x06
    };
    u8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    udelay(150);//1.16
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}


static void drvISP_ExitIspMode(void)
{
    u8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
    udelay( 150 );//200);
}

static u8 drvISP_ReadStatus(void)
{
    u8 bReadData = 0;
    u8 bWriteData[2] =
    {
        0x10, 0x05
    };
    u8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay(150);//200);
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    return bReadData;
}


static void drvISP_BlockErase(u32 addr)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
	TP_DEBUG("\n******%s come in*******\n",__FUNCTION__);
	u32 timeOutCount=0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	//msctpc_LoopDelay ( 1 );        // delay about 100us*****
	udelay(150);//200);
    timeOutCount=0;
	while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
	{
		timeOutCount++;
		if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	}
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;//0xD8;        //Block Erase
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;
    //bWriteData[4] = (addr & 0xFF) ;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 5);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		//msctpc_LoopDelay ( 1 );        // delay about 100us*****
	udelay(150);//200);
	timeOutCount=0;
	while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
	{
		timeOutCount++;
		if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
	}
}

static void drvISP_Program(u16 k, u8* pDataToWrite)
{
    u16 i = 0;
    u16 j = 0;
    //u16 n = 0;
    u8 TX_data[133];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
		u32 timeOutCount=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        //msctpc_LoopDelay ( 1 );        // delay about 100us*****
        udelay(150);//200);
       
        timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
			timeOutCount++;
			if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
		}
  
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    }
}

static ssize_t firmware_update_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
    return sprintf ( buf, "%s\n", fw_version );
}


static void drvISP_Verify ( u16 k, u8* pDataToVerify )
{
    u16 i = 0, j = 0;
    u8 bWriteData[5] ={ 0x10, 0x03, 0, 0, 0 };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
    u8 index = 0;
    u32 timeOutCount;
    for ( j = 0; j < 8; j++ ) //128*8 cycle
    {
        bWriteData[2] = ( u8 ) ( ( addr + j * 128 ) >> 16 );
        bWriteData[3] = ( u8 ) ( ( addr + j * 128 ) >> 8 );
        bWriteData[4] = ( u8 ) ( addr + j * 128 );
        udelay ( 100 );        // delay about 100us*****

        timeOutCount = 0;
        while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
        {
            timeOutCount++;
            if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
        }

        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 5 ); //write read flash addr
        udelay ( 100 );        // delay about 100us*****
        drvISP_Read ( 128, RX_data );
        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 ); //cmd end
        for ( i = 0; i < 128; i++ ) //log out if verify error
        {
            if ( ( RX_data[i] != 0 ) && index < 10 )
            {
                //TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
                index++;
            }
            if ( RX_data[i] != pDataToVerify[128 * j + i] )
            {
                TP_DEBUG ( "k=%d,j=%d,i=%d===============Update Firmware Error================", k, j, i );
            }
        }
    }
}

static void drvISP_ChipErase()
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
    u32 timeOutCount = 0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 3 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
    }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;

    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
    }
}

/* update the firmware part, used by apk*/
/*show the fw version*/

static ssize_t firmware_update_c2 ( struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay ( 300 );

    // Program and Verify
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    //Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for ( i = 0; i < 94; i++ ) // total  94 KB : 1 byte per R/W
    {
        drvISP_Program ( i, temp[i] ); // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
    TP_DEBUG_ERR ( "update_C2 OK\n" );
    drvISP_ExitIspMode();
    _HalTscrHWReset();
    FwDataCnt = 0;
    HalEnableIrq();	
    return size;
}

static u32 Reflect ( u32 ref, char ch ) //unsigned int Reflect(unsigned int ref, char ch)
{
    u32 value = 0;
    u32 i = 0;

    for ( i = 1; i < ( ch + 1 ); i++ )
    {
        if ( ref & 1 )
        {
            value |= 1 << ( ch - i );
        }
        ref >>= 1;
    }
    return value;
}

u32 Get_CRC ( u32 text, u32 prevCRC, u32 *crc32_table )
{
    u32  ulCRC = prevCRC;
	ulCRC = ( ulCRC >> 8 ) ^ crc32_table[ ( ulCRC & 0xFF ) ^ text];
    return ulCRC ;
}
static void Init_CRC32_Table ( u32 *crc32_table )
{
    u32 magicnumber = 0x04c11db7;
    u32 i = 0, j;

    for ( i = 0; i <= 0xFF; i++ )
    {
        crc32_table[i] = Reflect ( i, 8 ) << 24;
        for ( j = 0; j < 8; j++ )
        {
            crc32_table[i] = ( crc32_table[i] << 1 ) ^ ( crc32_table[i] & ( 0x80000000L ) ? magicnumber : 0 );
        }
        crc32_table[i] = Reflect ( crc32_table[i], 32 );
    }
}

typedef enum
{
	EMEM_ALL = 0,
	EMEM_MAIN,
	EMEM_INFO,
} EMEM_TYPE_t;

static void drvDB_WriteReg8Bit ( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 4 );
}

static void drvDB_WriteReg ( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 5 );
}

static unsigned short drvDB_ReadReg ( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}

static int drvTP_erase_emem_c32 ( void )
{
    /////////////////////////
    //Erase  all
    /////////////////////////
    
    //enter gpio mode
    drvDB_WriteReg ( 0x16, 0x1E, 0xBEAF );

    // before gpio mode, set the control pin as the orginal status
    drvDB_WriteReg ( 0x16, 0x08, 0x0000 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptrim = 1, h'04[2]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x04 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptm = 6, h'04[12:14] = b'110
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x60 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    // pmasi = 1, h'04[6]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x44 );
    // pce = 1, h'04[11]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x68 );
    // perase = 1, h'04[7]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xC4 );
    // pnvstr = 1, h'04[5]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xE4 );
    // pwe = 1, h'04[9]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x6A );
    // trigger gpio load
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    return ( 1 );
}

static ssize_t firmware_update_c32 ( struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size,  EMEM_TYPE_t emem_type )
{
    u8  dbbus_tx_data[4];
    u8  dbbus_rx_data[2] = {0};
      // Buffer for slave's firmware

    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

#if 1
    /////////////////////////
    // Erase  all
    /////////////////////////
    drvTP_erase_emem_c32();
    mdelay ( 1000 ); //MCR_CLBK_DEBUG_DELAY ( 1000, MCU_LOOP_DELAY_COUNT_MS );

    //ResetSlave();
    _HalTscrHWReset();
    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Reset Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x1C70 );


    drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks

    //polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );


    //calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

    for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
    {
        if ( i < 32 )   //emem_main
        {
            if ( i == 31 )
            {
                temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
                temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

                for ( j = 0; j < 1016; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
            else
            {
                for ( j = 0; j < 1024; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
        }
        else  // emem_info
        {
            for ( j = 0; j < 1024; j++ )
            {
                //crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
                crc_info = Get_CRC ( temp[i][j], crc_info, &crc_tab[0] );
            }
        }

        //drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );

        // polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

        drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
    }

    //write file done
    drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );

    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );
    // polling 0x3CE4 is 0x9432
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x9432 );

    crc_main = crc_main ^ 0xffffffff;
    crc_info = crc_info ^ 0xffffffff;

    // CRC Main from TP
    crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
    crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );
 
    //CRC Info from TP
    crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
    crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );

    TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();
    if ( ( crc_main_tp != crc_main ) || ( crc_info_tp != crc_info ) )
    {
        TP_DEBUG_ERR ( "update_C32 FAILED\n" );
		_HalTscrHWReset();
        FwDataCnt = 0;
    	HalEnableIrq();		
        return ( 0 );
    }

    TP_DEBUG_ERR ( "update_C32 OK\n" );
	_HalTscrHWReset();
    FwDataCnt = 0;
	HalEnableIrq();	

    return size;
#endif
}

static int drvTP_erase_emem_c33 ( EMEM_TYPE_t emem_type )
{
    // stop mcu
    drvDB_WriteReg ( 0x0F, 0xE6, 0x0001 );

    //disable watch dog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    // set PROGRAM password
    drvDB_WriteReg8Bit ( 0x16, 0x1A, 0xBA );
    drvDB_WriteReg8Bit ( 0x16, 0x1B, 0xAB );

    //proto.MstarWriteReg(F1.loopDevice, 0x1618, 0x80);
    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    if ( emem_type == EMEM_ALL )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x08, 0x10 ); //mark
    }

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x40 );
    mdelay ( 10 );

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    // erase trigger
    if ( emem_type == EMEM_MAIN )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x04 ); //erase main
    }
    else
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x08 ); //erase all block
    }

    return ( 1 );
}

static int drvTP_read_emem_dbbus_c33 ( EMEM_TYPE_t emem_type, u16 addr, size_t size, u8 *p, size_t set_pce_high )
{
    u32 i;

    // Set the starting address ( must before enabling burst mode and enter riu mode )
    drvDB_WriteReg ( 0x16, 0x00, addr );

    // Enable the burst mode ( must before enter riu mode )
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) | 0x0001 );

    // Set the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0xABBA );

    // Enable the information block if pifren is HIGH
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );

        // Set the PIFREN to be HIGH
        drvDB_WriteReg ( 0x16, 0x08, 0x0010 );
    }

    // Set the PCE to be HIGH
    drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
    mdelay ( 10 );

    // Wait pce becomes 1 ( read data ready )
    while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );

    for ( i = 0; i < size; i += 4 )
    {
        // Fire the FASTREAD command
        drvDB_WriteReg ( 0x16, 0x0E, drvDB_ReadReg ( 0x16, 0x0E ) | 0x0001 );

        // Wait the operation is done
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0001 ) != 0x0001 );

        p[i + 0] = drvDB_ReadReg ( 0x16, 0x04 ) & 0xFF;
        p[i + 1] = ( drvDB_ReadReg ( 0x16, 0x04 ) >> 8 ) & 0xFF;
        p[i + 2] = drvDB_ReadReg ( 0x16, 0x06 ) & 0xFF;
        p[i + 3] = ( drvDB_ReadReg ( 0x16, 0x06 ) >> 8 ) & 0xFF;
    }

    // Disable the burst mode
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) & ( ~0x0001 ) );

    // Clear the starting address
    drvDB_WriteReg ( 0x16, 0x00, 0x0000 );

    //Always return to main block
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE before change block
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );
        // Set the PIFREN to be LOW
        drvDB_WriteReg ( 0x16, 0x08, drvDB_ReadReg ( 0x16, 0x08 ) & ( ~0x0010 ) );

        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    // Clear the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0x0000 );

    if ( set_pce_high )
    {
        // Set the PCE to be HIGH before jumping back to e-flash codes
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    return ( 1 );
}


static int drvTP_read_info_dwiic_c33 ( void )
{
    u8  dwiic_tx_data[5];
    u8  dwiic_rx_data[4];
    u16 reg_data=0;
    mdelay ( 300 );

    // Stop Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    drvDB_WriteReg ( 0x3C, 0xE4, 0xA4AB );

	drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );

    // TP SW reset
    drvDB_WriteReg ( 0x1E, 0x04, 0x829F );
	mdelay ( 1 );
    dwiic_tx_data[0] = 0x10;
    dwiic_tx_data[1] = 0x0F;
    dwiic_tx_data[2] = 0xE6;
    dwiic_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dwiic_tx_data, 4 );	
    mdelay ( 100 );
    do{
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x5B58 );
    dwiic_tx_data[0] = 0x72;
    dwiic_tx_data[1] = 0x80;
    dwiic_tx_data[2] = 0x00;
    dwiic_tx_data[3] = 0x04;
    dwiic_tx_data[4] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , dwiic_tx_data, 5 );
    mdelay ( 50 );

    // recive info data
    //HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[0], 1024 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[0], 8 );
    return ( 1 );
}

static int drvTP_info_updata_C33 ( u16 start_index, u8 *data, u16 size )
{
    // size != 0, start_index+size !> 1024
    u16 i;
    for ( i = 0; i < size; i++ )
    {
        g_dwiic_info_data[start_index] = * ( data + i );
        start_index++;
    }
    return ( 1 );
}

static ssize_t firmware_update_c33 ( struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size, EMEM_TYPE_t emem_type )
{
    u8  dbbus_tx_data[4];
    u8  dbbus_rx_data[2] = {0};
    u8  life_counter[2];
    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
  
    int update_pass = 1;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;
    drvTP_read_info_dwiic_c33();
	
    if ( 0/*g_dwiic_info_data[0] == 'M' && g_dwiic_info_data[1] == 'S' && g_dwiic_info_data[2] == 'T' && g_dwiic_info_data[3] == 'A' && g_dwiic_info_data[4] == 'R' && g_dwiic_info_data[5] == 'T' && g_dwiic_info_data[6] == 'P' && g_dwiic_info_data[7] == 'C' */)
    {
        // updata FW Version
        //drvTP_info_updata_C33 ( 8, &temp[32][8], 5 );

		g_dwiic_info_data[8]=temp[32][8];
		g_dwiic_info_data[9]=temp[32][9];
		g_dwiic_info_data[10]=temp[32][10];
		g_dwiic_info_data[11]=temp[32][11];
        // updata life counter
        life_counter[1] = (( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) >> 8 ) & 0xFF;
        life_counter[0] = ( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) & 0xFF;
		g_dwiic_info_data[12]=life_counter[0];
		g_dwiic_info_data[13]=life_counter[1];
        //drvTP_info_updata_C33 ( 10, &life_counter[0], 3 );
        drvDB_WriteReg ( 0x3C, 0xE4, 0x78C5 );
		drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );
        // TP SW reset
        drvDB_WriteReg ( 0x1E, 0x04, 0x829F );

        mdelay ( 50 );
        //polling 0x3CE4 is 0x2F43
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );

        }
        while ( reg_data != 0x2F43 );
        // transmit lk info data
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , &g_dwiic_info_data[0], 1024 );
        //polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );
    }

    //erase main
    drvTP_erase_emem_c33 ( EMEM_MAIN );
    mdelay ( 1000 );

    //ResetSlave();
    _HalTscrHWReset();

    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0x1C70 );
    }

    switch ( emem_type )
    {
        case EMEM_ALL:
            drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks
            break;
        case EMEM_MAIN:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for main block
            break;
        case EMEM_INFO:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for info block

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );

            drvDB_WriteReg8Bit ( 0x3C, 0xE4, 0xC5 ); //
            drvDB_WriteReg8Bit ( 0x3C, 0xE5, 0x78 ); //

            drvDB_WriteReg8Bit ( 0x1E, 0x04, 0x9F );
            drvDB_WriteReg8Bit ( 0x1E, 0x05, 0x82 );

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x00 );
            mdelay ( 100 );
            break;
    }
    // polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );
    // calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

    for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
    {
        if ( emem_type == EMEM_INFO )
			i = 32;

        if ( i < 32 )   //emem_main
        {
            if ( i == 31 )
            {
                temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
                temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

                for ( j = 0; j < 1016; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
            else
            {
                for ( j = 0; j < 1024; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
        }
        else  //emem_info
        {
            for ( j = 0; j < 1024; j++ )
            {
                //crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
                crc_info = Get_CRC ( g_dwiic_info_data[j], crc_info, &crc_tab[0] );
            }
            if ( emem_type == EMEM_MAIN ) break;
        }
        //drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
        #if 1
        {
            u32 n = 0;
            for(n=0;n<UPDATE_TIMES;n++)
            {
                TP_DEBUG_ERR("i=%d,n=%d",i,n);
                HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i]+n*N_BYTE_PER_TIME, N_BYTE_PER_TIME );
            }
        }
        #else
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );
        #endif
        // polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );
        drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
    }
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // write file done and check crc
        drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );
    }
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // polling 0x3CE4 is 0x9432
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }while ( reg_data != 0x9432 );
    }

    crc_main = crc_main ^ 0xffffffff;
    crc_info = crc_info ^ 0xffffffff;

    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // CRC Main from TP
        crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
        crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );

        // CRC Info from TP
        crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
        crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );
    }
    TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();
    update_pass = 1;
	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        if ( crc_main_tp != crc_main )
            update_pass = 0;

        if ( crc_info_tp != crc_info )
            update_pass = 0;
    }

    if ( !update_pass )
    {
        TP_DEBUG_ERR ( "update_C33 ok111\n" );
		_HalTscrHWReset();
        FwDataCnt = 0;
    	HalEnableIrq();	
        return size;
    }

    TP_DEBUG_ERR ( "update_C33 OK\n" );
	_HalTscrHWReset();
    FwDataCnt = 0;
    HalEnableIrq();	
    return size;
}

#define _FW_UPDATE_C3_
#ifdef _FW_UPDATE_C3_
static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
	HalDisableIrq();

    _HalTscrHWReset();

    // Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
	// c2:2133 c32:2133a(2) c33:2138
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG_ERR ( "111dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );
    if ( dbbus_rx_data[0] == 2 )
    {
        // check version
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3C;
        dbbus_tx_data[2] = 0xEA;
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
        HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
        TP_DEBUG_ERR ( "dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );

        if ( dbbus_rx_data[0] == 3 ){
            return firmware_update_c33 ( dev, attr, buf, size, EMEM_MAIN );
		}
        else{

            return firmware_update_c32 ( dev, attr, buf, size, EMEM_ALL );
        }
    }
    else
    {
        return firmware_update_c2 ( dev, attr, buf, size );
    } 
}
#else
static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};

    _HalTscrHWReset();

    // 1. Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay ( 300 );

    // 2.Program and Verify
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for ( i = 0; i < 94; i++ ) // total  94 KB : 1 byte per R/W
    {
        drvISP_Program ( i, temp[i] ); // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
    TP_DEBUG ( "update OK\n" );
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    
    return size;
}
#endif
static DEVICE_ATTR(update, CTP_AUTHORITY, firmware_update_show, firmware_update_store);

static u8 getchipType(void)
{
    u8 curr_ic_type = 0;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
	
	_HalTscrHWReset();
    HalDisableIrq();
    mdelay ( 100 );
    
	dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 100 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
	// c2:2133 c32:2133a(2) c33:2138
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    
    if ( dbbus_rx_data[0] == 2 )
    {
    	curr_ic_type = CTP_ID_MSG21XXA;
    }
    else if( dbbus_rx_data[0] == 1 )
    {
    	curr_ic_type = CTP_ID_MSG21XX;
    }
    else
    {
    	curr_ic_type = CTP_ID_UNKNOW;
    }
    TP_DEBUG_ERR("CURR_IC_TYPE = %d \n",curr_ic_type);
   // dbbusDWIICIICNotUseBus();
   // dbbusDWIICNotStopMCU();
   // dbbusDWIICExitSerialDebugMode();
    HalEnableIrq();
    
    return curr_ic_type;
    
}
static void getMSG21XXFWVersion(u8 curr_ic_type)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;
    int ret_w = 0;
    int ret_r = 0;

	_HalTscrHWReset();
    HalDisableIrq();
    mdelay ( 100 );
    
	dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 100 );
    
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
     if(curr_ic_type==CTP_ID_MSG21XXA)
    {
    dbbus_tx_data[2] = 0x2A;
    }
    else if(curr_ic_type==CTP_ID_MSG21XX)
    {
        dbbus_tx_data[2] = 0x74;
    }
    else
    {
        TP_DEBUG_ERR("***ic_type = %d ***\n", curr_ic_type);
        dbbus_tx_data[2] = 0x2A;
    }
    ret_w = HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    ret_r = HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);
    
    if(ret_r<0
        ||ret_w<0)
    {
        curr_ic_major = 0xffff;
        curr_ic_minor = 0xffff;
    }
    else
    {
        curr_ic_major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
        curr_ic_minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];
    }

    TP_DEBUG_ERR("***FW Version major = %d ***\n", curr_ic_major);
    TP_DEBUG_ERR("***FW Version minor = %d ***\n", curr_ic_minor);
    
    _HalTscrHWReset();
    HalEnableIrq();
    mdelay ( 100 );
}
#if 0
/*test=================*/
static ssize_t firmware_clear_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
	printk(" +++++++ [%s] Enter!++++++\n", __func__);
	u16 k=0,i = 0, j = 0;
	u8 bWriteData[5] =
	{
        0x10, 0x03, 0, 0, 0
	};
	u8 RX_data[256];
	u8 bWriteData1 = 0x12;
	u32 addr = 0;
	u32 timeOutCount=0;
	for (k = 0; k < 94; i++)   // total  94 KB : 1 byte per R/W
	{
		addr = k * 1024;
		for (j = 0; j < 8; j++)   //128*8 cycle
		{
			bWriteData[2] = (u8)((addr + j * 128) >> 16);
			bWriteData[3] = (u8)((addr + j * 128) >> 8);
			bWriteData[4] = (u8)(addr + j * 128);
			//msctpc_LoopDelay ( 1 );        // delay about 100us*****
			udelay(150);//200);

			timeOutCount=0;
			while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
			{
				timeOutCount++;
				if ( timeOutCount >= 100000 ) 
					break; /* around 1 sec timeout */
	  		}
        
			HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
			//msctpc_LoopDelay ( 1 );        // delay about 100us*****
			udelay(150);//200);
			drvISP_Read(128, RX_data);
			HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
			for (i = 0; i < 128; i++)   //log out if verify error
			{
				if (RX_data[i] != 0xFF)
				{
					//TP_DEBUG(printk("k=%d,j=%d,i=%d===============erase not clean================",k,j,i);)
					printk("k=%d,j=%d,i=%d  erase not clean !!",k,j,i);
				}
			}
		}
	}
	TP_DEBUG("read finish\n");
	return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_clear_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{

	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	printk(" +++++++ [%s] Enter!++++++\n", __func__);
	//msctpc_LoopDelay ( 100 ); 	   // delay about 100ms*****

	// Enable slave's ISP ECO mode

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x08;
	dbbus_tx_data[2] = 0x0c;
	dbbus_tx_data[3] = 0x08;
	
	// Disable the Watchdog
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Stop MCU
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Enable SPI Pad
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
	dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x25;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);

	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//WP overwrite
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x0E;
	dbbus_tx_data[3] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	//set pin high
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x10;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	//set FRO to 50M
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 1,0
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbusDWIICIICNotUseBus();
	dbbusDWIICNotStopMCU();
	dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
	TP_DEBUG(printk("chip erase+\n");)
    drvISP_BlockErase(0x00000);
	TP_DEBUG(printk("chip erase-\n");)
    drvISP_ExitIspMode();
    return size;
}
static DEVICE_ATTR(clear, CTP_AUTHORITY, firmware_clear_show, firmware_clear_store);
#endif //0
/*test=================*/
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/

static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;
   
/*
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

*/
   

    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    if(curr_ic_type==CTP_ID_MSG21XXA)
    {
    dbbus_tx_data[2] = 0x2A;
    }
    else if(curr_ic_type==CTP_ID_MSG21XX)
    {
        dbbus_tx_data[2] = 0x74;
    }
    else
    {
        TP_DEBUG_ERR("***ic_type = %d ***\n", dbbus_tx_data[2]);
        dbbus_tx_data[2] = 0x2A;
    }
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];
    curr_ic_major = major;
    curr_ic_minor = minor;

    TP_DEBUG_ERR("***major = %d ***\n", major);
    TP_DEBUG_ERR("***minor = %d ***\n", minor);
    sprintf(fw_version,"%03x%03x", major, minor);
    //TP_DEBUG(printk("***fw_version = %s ***\n", fw_version);)
   
    return size;
}
static DEVICE_ATTR(version, CTP_AUTHORITY, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
{
    int i;
	TP_DEBUG_ERR("***FwDataCnt = %d ***\n", FwDataCnt);
   // for (i = 0; i < 1024; i++)
    {
        memcpy(temp[FwDataCnt], buf, 1024);
    }
    FwDataCnt++;
    return size;
}
static DEVICE_ATTR(data, CTP_AUTHORITY, firmware_data_show, firmware_data_store);

#endif//adair:以上无需修改

//end for update firmware

 static u8 Calculate_8BitsChecksum( u8 *msg, s32 s32Length )
 {
	 s32 s32Checksum = 0;
	 s32 i;
 
	 for( i = 0 ; i < s32Length; i++ )
	 {
		 s32Checksum += msg[i];
	 }
 
	 return ( u8 )( ( -s32Checksum ) & 0xFF );
 }
#endif


#if	defined(AUTO_UPGRADE) ||  defined (SELF_APK_UPGRADE)
static void get_i_file_info(const char *fw_buf, size_t size, unsigned short *major, unsigned short *minor)
{
	if (1 == tp_ic_major)
	{
	   *major=(fw_buf[0x8009]<<8)|fw_buf[0x8008];
	   *minor=(fw_buf[0x800b]<<8)|fw_buf[0x800a];
	}
	else
	{
	   *major=(fw_buf[0x7f4f]<<8)|fw_buf[0x7f4e];
	   *minor=(fw_buf[0x7f51]<<8)|fw_buf[0x7f50];
	}
}

int msg_ctpm_auto_upgrade(struct i2c_client *client)
{
	U8 *p = temp;
	int i = 0;
	int size;
	unsigned char  tp_info[512];
	int len;

	if (tp_ic_major == DIJIN_FW_MAJOR)
	{
		get_i_file_info(CTPM_FW_D208_DJ_48_10040_0359A, sizeof(CTPM_FW_D208_DJ_48_10040_0359A), &tp_i_file_major, &tp_i_file_minor);
		i_file_fw_version = tp_i_file_minor;
	}
	else if (tp_ic_major == TRULY_FW_MAJOR)
	{
		get_i_file_info(CTPM_FW_D208_TRULY_CP1F0471, sizeof(CTPM_FW_D208_TRULY_CP1F0471), &tp_i_file_major, &tp_i_file_minor);
		i_file_fw_version = tp_i_file_minor;

	}
	else  if (tp_ic_major == FN_FW_MAJOR)
	{
		if (0x30 <= tp_ic_minor && tp_ic_minor < 0x40 )
		{
			get_i_file_info(CTPM_FW_D206_FN_CS040X_RT01, sizeof(CTPM_FW_D206_FN_CS040X_RT01), &tp_i_file_major, &tp_i_file_minor);
			i_file_fw_version = tp_i_file_minor;

		}
		else if (0x40 <= tp_ic_minor && tp_ic_minor < 0x50)
		{
			get_i_file_info(CTPM_FW_D208_FN_CS040X_RT04, sizeof(CTPM_FW_D208_FN_CS040X_RT04), &tp_i_file_major, &tp_i_file_minor);
			i_file_fw_version = tp_i_file_minor;
		
		}
	}
	
	// judge i file and ic version 
	printk("\n@@@tp_ic_major = 0x%x, tp_ic_minor = 0x%x, tp_i_file_major = 0x%x, tp_i_file_minor = 0x%x@@@",tp_ic_major,tp_ic_minor,tp_i_file_major,tp_i_file_minor);
	// begin

	if ((tp_ic_major == DIJIN_FW_MAJOR  ||  tp_ic_major == TRULY_FW_MAJOR ||  tp_ic_major == FN_FW_MAJOR) &&
																	( ic_fw_version < i_file_fw_version))
	//end 
	{
		if (tp_ic_major == DIJIN_FW_MAJOR)
		{
			size = sizeof(CTPM_FW_D208_DJ_48_10040_0359A);

			while (i < 94 * 1024 &&  i < size)
			*p++ =  CTPM_FW_D208_DJ_48_10040_0359A[i++];
		}
		else if (tp_ic_major == TRULY_FW_MAJOR)
		{
			size = sizeof(CTPM_FW_D208_TRULY_CP1F0471);

			while (i < 94 * 1024 &&  i < size)
			*p++ =  CTPM_FW_D208_TRULY_CP1F0471[i++];
		}
		else  if (tp_ic_major == FN_FW_MAJOR)
		{
			if (0x30 <= tp_ic_minor && tp_ic_minor < 0x40 )
			{
				size = sizeof(CTPM_FW_D206_FN_CS040X_RT01);

				while (i < 94 * 1024 &&  i < size)
				*p++ =  CTPM_FW_D206_FN_CS040X_RT01[i++];
			}
			else if (0x40 <= tp_ic_minor && tp_ic_minor < 0x50)
			{
				size = sizeof(CTPM_FW_D208_FN_CS040X_RT04);

				while (i < 94 * 1024 &&  i < size)
				*p++ =  CTPM_FW_D208_FN_CS040X_RT04[i++];
			}
		}
		
		firmware_update_store(NULL, NULL, NULL, 0);	
		msleep(300);
		printk("\n@@@ line = %d ,upgrade finished tp_ic_major = 0x%x, tp_ic_minor = 0x%x @@@", __LINE__, tp_ic_major,tp_ic_minor);
	}
	
	msleep(50);
	get_ic_info(&tp_ic_major, &tp_ic_minor);
	ic_fw_version = tp_ic_minor;
	if (tp_ic_major == FN_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","FuNa",client->addr,tp_ic_major,tp_ic_minor);
	else if (tp_ic_major == DIJIN_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","DiJin",client->addr,tp_ic_major,tp_ic_minor);
	else if (tp_ic_major == TRULY_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","Truly",client->addr,tp_ic_major,tp_ic_minor);
	else
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","UN kown",client->addr,tp_ic_major,tp_ic_minor);

	fix_tp_proc_info(tp_info, len);
}


static int auto_upgrade_thread(void * date)
{
	  msg_ctpm_auto_upgrade(i2c_client);      
}
#endif

#if	defined(NODE_UPGRADE) 

static ssize_t firmware_node_upgrade_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
	char *temp;
	u8 current_fw_version;

	get_ic_info(&tp_ic_major, &tp_ic_minor);
	ic_fw_version = tp_ic_minor;
	printk("\n@@line = %d, fw_ver = 0x%x, fw_major = 0x%x, fm_minor= 0x%x", __LINE__, ic_fw_version, tp_ic_major, tp_ic_minor);

	return sprintf(buf, "###### TP newest supported firmware: \n \n\
DiJin TP [D206]   [48-10040-0336A][firmware:0x48,major:0x2,minor:0x48]----1\n\n\
DiJin TP [D208]   [48-10040-0359A][firmware:0x48,major:0x2,minor:0x48]----2\n\n\
DiJin TP [D208-S2][0374A]         [firmware:0x50,major:0x2,minor:0x50]----3\n\n\
FuNa  TP [D206]   [CS040X_RT01]   [firmware:0x33,major:0x1,minor:0x33]----4\n\n\
FuNa  TP [D208|D209]   [CS040X_RT04]   [firmware:0x46,major:0x1,minor:0x46]----5\n\n\ 
Truly TP [D208]   [TRULY_CP1F0471][firmware:0x32,major:0x4,minor:0x32]----6\n\n\ 
########################################\n\
###### current  TP IC firmware :\n\n\
------------------------------ [firmware:0x%x,major:0x%x,minor:0x%x] \n\n\
try [echo n >> upgrade_node] to upgrade TP firmware !\n\n\
compare the corresponding TP newest supported firmware and current TP IC firmware to decide to upgrade or not !\n\n\n "
											, ic_fw_version, tp_ic_major, tp_ic_minor);	
}

static ssize_t firmware_node_upgrade_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
	int i_ret;
	int i = 0;
	U8 *p = temp;
	int fw_size;

	
	unsigned long val = simple_strtoul(buf, NULL, 10);

	if(buf == NULL)
	{
		printk("'select the project through clicking  'cat ftp_fw_update'");
		return -1;
	}
	printk("ft5x06_store_fw_update\n");

	if(val == 1){
		 fw_size = sizeof(CTPM_FW_D206_DJ_48_10040_0336A);
		while (i < fw_size)
			*p++ =  CTPM_FW_D206_DJ_48_10040_0336A[i++];

	}else if(val == 2){
		 fw_size = sizeof(CTPM_FW_D208_DJ_48_10040_0359A);
		while (i < fw_size)
			*p++ =  CTPM_FW_D208_DJ_48_10040_0359A[i++];
		
	}else if(val == 3){
		 fw_size = sizeof(CTPM_FW_D208_DJ_S2_0374A);
		while (i < fw_size)
			*p++ =  CTPM_FW_D208_DJ_S2_0374A[i++];

	}else if(val == 4){
		 fw_size = sizeof(CTPM_FW_D206_FN_CS040X_RT01);
		while (i < fw_size)
			*p++ =  CTPM_FW_D206_FN_CS040X_RT01[i++];

	}else if(val == 5){
		 fw_size = sizeof(CTPM_FW_D208_FN_CS040X_RT04);
		while (i < fw_size)
			*p++ =  CTPM_FW_D208_FN_CS040X_RT04[i++];

	}
	else if(val == 6){
		 fw_size = sizeof(CTPM_FW_D208_TRULY_CP1F0471);
		while (i < fw_size)
			*p++ =  CTPM_FW_D208_TRULY_CP1F0471[i++];
	}

	firmware_update_store(NULL, NULL, NULL, 0);	

	msleep(300);
	
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
	msleep(10);
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(300);

	get_ic_info(&tp_ic_major, &tp_ic_minor);
	ic_fw_version = tp_ic_minor;

	printk("\n@@line = %d, fw_ver = 0x%x, fw_major = 0x%x, fw_minor = 0x%x", __LINE__, ic_fw_version, tp_ic_major, tp_ic_minor);
	
	return size;
}


static DEVICE_ATTR(upgrade_node, CTP_AUTHORITY, firmware_node_upgrade_show, firmware_node_upgrade_store);
#endif

#ifdef SELF_APK_UPGRADE
static ssize_t firmware_node_upgrade_self_apk_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
	char *temp;
	u8 current_fw_version;

	get_ic_info(&tp_ic_major, &tp_ic_minor);
	ic_fw_version = tp_ic_minor;
	printk("\n@@line = %d, fw_ver = 0x%x, fw_major = 0x%x, fm_minor= 0x%x", __LINE__, ic_fw_version, tp_ic_major, tp_ic_minor);

	if (tp_ic_major == DIJIN_FW_MAJOR)
	{
		get_i_file_info(CTPM_FW_D208_DJ_48_10040_0359A, sizeof(CTPM_FW_D208_DJ_48_10040_0359A), &tp_i_file_major, &tp_i_file_minor);
		i_file_fw_version = tp_i_file_minor;
		return sprintf(buf, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,current firmware :0x%x,newest firmware:0x%x,","msg2133A","DiJin",i2c_client->addr,tp_ic_major,tp_ic_minor,tp_ic_minor,i_file_fw_version);	
	}
	else if (tp_ic_major == TRULY_FW_MAJOR)
	{
		get_i_file_info(CTPM_FW_D208_TRULY_CP1F0471, sizeof(CTPM_FW_D208_TRULY_CP1F0471), &tp_i_file_major, &tp_i_file_minor);
		i_file_fw_version = tp_i_file_minor;
		return sprintf(buf, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,current firmware :0x%x,newest firmware:0x%x,","msg2133A","Truly",i2c_client->addr,tp_ic_major,tp_ic_minor,tp_ic_minor,i_file_fw_version);	
	}
	else  if (tp_ic_major == FN_FW_MAJOR)
	{
		if (0x30 <= tp_ic_minor && tp_ic_minor < 0x40 )
		{
			get_i_file_info(CTPM_FW_D206_FN_CS040X_RT01, sizeof(CTPM_FW_D206_FN_CS040X_RT01), &tp_i_file_major, &tp_i_file_minor);
			i_file_fw_version = tp_i_file_minor;

			return sprintf(buf, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,current firmware :0x%x,newest firmware:0x%x,","msg2133A","FuNa",i2c_client->addr,tp_ic_major,tp_ic_minor,tp_ic_minor,i_file_fw_version);	
		}
		else if (0x40 <= tp_ic_minor && tp_ic_minor < 0x50)
		{
			get_i_file_info(CTPM_FW_D208_FN_CS040X_RT04, sizeof(CTPM_FW_D208_FN_CS040X_RT04), &tp_i_file_major, &tp_i_file_minor);
			i_file_fw_version = tp_i_file_minor;
			
			return sprintf(buf, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,current firmware :0x%x,newest firmware:0x%x,","msg2133A","FuNa",i2c_client->addr,tp_ic_major,tp_ic_minor,tp_ic_minor,i_file_fw_version);	
		
		}
	}
	else
		return sprintf(buf, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","nukown",i2c_client->addr,tp_ic_major,tp_ic_minor);	


}

static ssize_t firmware_node_upgrade_self_apk_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
	msg_ctpm_auto_upgrade(i2c_client);      

	return size;
}

static DEVICE_ATTR(upgrade_node_self_apk , CTP_AUTHORITY, firmware_node_upgrade_self_apk_show, firmware_node_upgrade_self_apk_store);

#endif


#ifdef TP_PROXIMITY_SENSOR
static ssize_t show_proximity_sensor(struct device *dev, struct device_attribute *attr, char *buf)
{
   printk("\n######### show_proximity_sensor\n");

    return    sprintf(buf, "ps value : %d, enable value: %d \n", tpd_proximity_detect, tpd_proximity_flag);//mijianfeng //*buf = ps_data_state[0];
}

static ssize_t store_proximity_sensor(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    U8 ps_store_data[4];
	int s = 0;

	unsigned long val = simple_strtoul(buf, NULL, 10);

    printk("\n######### store_proximity_sensor buf=%d,size=%d\n", *buf, size);


    if(buf != NULL && size != 0)
    {
        if(0 == val)
        {
            printk("\n######### DISABLE_CTP_PS buf=%d,size=%d\n", val, size);
            ps_store_data[0] = 0x52;
            ps_store_data[1] = 0x00;
	    		  ps_store_data[2] = 0x4a;			//0x78;  //0x62 modif by xuzhoubin
            ps_store_data[3] = 0xa1;
            i2c_write(TOUCH_ADDR_MSG20XX, &ps_store_data[0], 4);
            msleep(2000);
            printk("RESET_CTP_PS buf=%d\n", val);
            mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
            msleep(100);
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
            //changed in 2012-07-07 by [Hally]
		#ifdef TPD_RSTPIN_1V8
    		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_IN);
    		mt_set_gpio_pull_enable(GPIO_CTP_RST_PIN, GPIO_PULL_DISABLE);
		#else
			mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		#endif
            mdelay(500);
            mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        }
        else if(1 == val)
        {
            printk("\n######### ENABLE_CTP_PS buf=%d,size=%d\n", val, size);
            ps_store_data[0] = 0x52;
            ps_store_data[1] = 0x00;
	    		  ps_store_data[2] = 0x4a;		//0x78; //0x62 modif by xuzhoubin
            ps_store_data[3] = 0xa0;
            i2c_write(TOUCH_ADDR_MSG20XX, &ps_store_data[0], 4);
        }
        tpd_proximity_flag = val;
    }

    return size;
}
static DEVICE_ATTR(proximity_sensor, CTP_AUTHORITY, show_proximity_sensor, store_proximity_sensor);
#endif

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .name = "msg2133",
        .owner = THIS_MODULE,
    },
    .probe = tpd_probe,
    .remove = __devexit_p(tpd_remove),
    .id_table = msg2133_tpd_id,
    .detect = tpd_detect,
//    .address_data = &addr_data,
};

#if 0
static  void tpd_down(int x, int y)
{
    input_report_abs(tpd->dev, ABS_PRESSURE, 128);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 128);
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 128);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    SSL_PRINT("MSG2133 D[%4d %4d %4d] ", x, y,1);

   //add by lisong for cit test
   if (FACTORY_BOOT == get_boot_mode())
   {   
   	tpd_button(x, y, 1);  
   }
}

static void tpd_up(int x, int y)
{
    input_report_abs(tpd->dev, ABS_PRESSURE, 0);
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 0);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    SSL_PRINT("MSG2133 U[%4d %4d %4d] ", x, y, 0);

   //add by lisong for cit test
   if (FACTORY_BOOT == get_boot_mode())
   {   
   	tpd_button(x, y, 0);  
   }
}
#else
static  void tpd_down(int x, int y)
{
   // input_report_abs(tpd->dev, ABS_PRESSURE, 128);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);	//128);
    //input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 128);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    SSL_PRINT("MSG2133 D[%4d %4d %4d] ", x, y,1);

   //add by lisong for cit test
   if (FACTORY_BOOT == get_boot_mode())
   {   
   	tpd_button(x, y, 1);  
   }
}

static void tpd_up(int x, int y)
{
    //input_report_abs(tpd->dev, ABS_PRESSURE, 0);
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    //input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
    //input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 0);
    //input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    //input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    SSL_PRINT("MSG2133 U[%4d %4d %4d] ", x, y, 0);

   //add by lisong for cit test
   if (FACTORY_BOOT == get_boot_mode())
   {   
   	tpd_button(x, y, 0);  
   }
}
#endif

unsigned char tpd_check_sum(unsigned char *pval)
{
    int i, sum = 0;

    for(i = 0; i < 7; i++)
    {
        sum += pval[i];
    }

    return (unsigned char)((-sum) & 0xFF);
}

static bool msg2033_i2c_read(char *pbt_buf, int dw_lenth)
{
    int ret;
    //SSL_PRINT("The msg_i2c_client->addr=0x%x i2c_client->timing=%d\n",i2c_client->addr,i2c_client->timing);

    i2c_client->addr|=I2C_ENEXT_FLAG;
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);
    //SSL_PRINT("msg_i2c_read_interface ret=%d pbt_buf[0]=0x%x pbt_buf[1]=0x%x pbt_buf[2]=0x%x pbt_buf[3]=0x%x pbt_buf[4]=0x%x pbt_buf[5]=0x%x pbt_buf[6]=0x%x pbt_buf[7]=0x%x \n",ret,pbt_buf[0],pbt_buf[1],pbt_buf[2],pbt_buf[3],pbt_buf[4],pbt_buf[5],pbt_buf[6],pbt_buf[7]);
    if(ret <= 0)
    {
        SSL_PRINT("msg_i2c_read_interface error\n");
        return false;
    }

    return true;
}

#ifdef TP_PROXIMITY_SENSOR
static s32 tpd_get_ps_value(void)
{
    return tpd_proximity_detect;
	
}

static s32 tpd_enable_ps(s32 enable)
{
    s32 ret = -1;
    U8 ps_store_data[4];

    if (enable)
    {
        tpd_proximity_flag = 1;
        GTP_INFO("TPD proximity function to be on.");

		ps_store_data[0] = 0x52;
		ps_store_data[1] = 0x00;
		ps_store_data[2] = 0x4a;		//0x78; //0x62 modif by xuzhoubin
		ps_store_data[3] = 0xa0;
		i2c_write(TOUCH_ADDR_MSG20XX, &ps_store_data[0], 4);
    }
    else
    {
        tpd_proximity_flag = 0;
        GTP_INFO("TPD proximity function to be off.");


		ps_store_data[0] = 0x52;
		ps_store_data[1] = 0x00;
		ps_store_data[2] = 0x4a;			//0x78;  //0x62 modif by xuzhoubin
		ps_store_data[3] = 0xa1;
		i2c_write(TOUCH_ADDR_MSG20XX, &ps_store_data[0], 4);
		msleep(2000);
            mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
            msleep(100);
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
            //changed in 2012-07-07 by [Hally]
		#ifdef TPD_RSTPIN_1V8
    		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_IN);
    		mt_set_gpio_pull_enable(GPIO_CTP_RST_PIN, GPIO_PULL_DISABLE);
		#else
			mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		#endif
            mdelay(500);
            mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    }

    return 0;
}

s32 tpd_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                   void *buff_out, s32 size_out, s32 *actualout)
{
    s32 err = 0;
    s32 value;
    hwm_sensor_data *sensor_data;
	

    switch (command)
    {
        case SENSOR_DELAY:
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                GTP_ERROR("Set delay parameter error!");
                err = -EINVAL;
            }

            // Do nothing
            break;

        case SENSOR_ENABLE:
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                GTP_ERROR("Enable sensor parameter error!");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                err = tpd_enable_ps(value);
            }

            break;

        case SENSOR_GET_DATA:
            if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data)))
            {
                GTP_ERROR("Get sensor data parameter error!");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = tpd_get_ps_value();
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }

            break;

        default:
            GTP_ERROR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}
#endif


static int tpd_touchinfo(struct touch_info *cinfo)
{
    SHORT_TOUCH_STATE ShortTouchState;
    BYTE reg_val[8] = {0};
    unsigned int  temp = 0;
    u32 bitmask;
    int i;
    int ret;
	
#ifdef TP_PROXIMITY_SENSOR
    s32 err = 0;
    hwm_sensor_data sensor_data;
#endif

#ifdef TP_FIRMWARE_UPDATE
    if(update_switch)
    {
        return false;
    }
#endif
	msg2033_i2c_read(reg_val, 8);
	SSL_PRINT("received raw data from touch panel as following in fun %s\n", __func__);
	SSL_PRINT("MSG_ID=0x%x,\n", reg_val[0]);

	temp = tpd_check_sum(reg_val);
	SSL_PRINT("check_sum=%d,reg_val_7=%d\n", temp, reg_val[7]);
    if(temp == reg_val[7])
    {
        SSL_PRINT("TP_PS \nreg_val[1]=0x%x\nreg_val[2]=0x%x\nreg_val[3]=0x%x\nreg_val[4]=0x%x\nreg_val[5]=0x%x\nreg_val[6]=0x%x\nreg_val[7]=0x%x by cheehwa\n", reg_val[1], reg_val[2], reg_val[3], reg_val[4], reg_val[5], reg_val[6], reg_val[7]);
		
        if(reg_val[0] == 0x52) //CTP  ID
        {
        	ShortTouchState.pos_x = ((reg_val[1] & 0xF0) << 4) | reg_val[2];
		    ShortTouchState.pos_y = ((reg_val[1] & 0x0F) << 8) | reg_val[3];
		    ShortTouchState.dst_x = ((reg_val[4] & 0xF0) << 4) | reg_val[5];
		    ShortTouchState.dst_y = ((reg_val[4] & 0x0F) << 8) | reg_val[6];
		
		    if((ShortTouchState.dst_x) & 0x0800)
		    {
		        ShortTouchState.dst_x |= 0xF000;
		    }
		
		    if((ShortTouchState.dst_y) & 0x0800)
		    {
		        ShortTouchState.dst_y |= 0xF000;
		    }
		
		    ShortTouchState.pos_x2 = ShortTouchState.pos_x + ShortTouchState.dst_x;
		    ShortTouchState.pos_y2 = ShortTouchState.pos_y + ShortTouchState.dst_y;
		    
	#if (defined(TPD_HAVE_BUTTON) || defined(TP_PROXIMITY_SENSOR))
	      	if(reg_val[1] == 0xFF && reg_val[2] == 0xFF&& reg_val[3] == 0xFF && reg_val[4] == 0xFF)  //modif by xuzhoubin  add && reg_val[4] == 0xFF
	     	{
			 	if(reg_val[5] == 0x0||reg_val[5] == 0xFF)
				{
					i = TPD_KEY_COUNT;
				}
		#ifdef TP_PROXIMITY_SENSOR
		else if(reg_val[5] == 0x80 || reg_val[5] == 0x40) // close to
		{

			        if (tpd_proximity_flag == 1)
			        {

					if(reg_val[5] == 0x80)
						tpd_proximity_detect = 0;

					else if(reg_val[5] == 0x40) // leave
						tpd_proximity_detect = 1;

			            //get raw data
			            //map and store data to hwm_sensor_data
			            sensor_data.values[0] = tpd_get_ps_value();
			            sensor_data.value_divide = 1;
			            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
			            //report to the up-layer
			            ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);

			            if (ret)
			            {
			                GTP_ERROR("Call hwmsen_get_interrupt_data fail = %d\n", err);
			            }
			        }
			}
		#endif
				else
				{
					for(i=0;i<TPD_KEY_COUNT;i++)
					{
						bitmask=1<<i;
						if(reg_val[5] & bitmask) 
						{ // button[i] on
#ifndef TPD_HOME_KEY_LONG_PRESS						
							if(msg2133_priv.buttons & bitmask)
							{	//button[i] still on
								//repeat button[i]
								SSL_PRINT("Button%d repeat.\n",i);
							}
							else
							{
#endif							
								//button[i] down
								SSL_PRINT("Button%d down.\n",i);
								cinfo->x[0]  = tpd_keys_dim_local[i][0];
								cinfo->y[0]=  tpd_keys_dim_local[i][1];
								point_num = 1;
								break;
#ifndef TPD_HOME_KEY_LONG_PRESS								
							}
#endif							
						}
						else
						{
							if(msg2133_priv.buttons & bitmask)
							{
								//button[i] up
								SSL_PRINT("Button%d up.\n",i);
							}
						}
					}
				}
				if(i == TPD_KEY_COUNT)
				{
					point_num = 0;
				}
				msg2133_priv.buttons = reg_val[5];
	     	}
	#else
	     	if(reg_val[1] == 0xFF && reg_val[2] == 0xFF&& reg_val[3] == 0xFF&& reg_val[4] == 0xFF)
			{
		  		point_num = 0;
	     	}
	#endif
            else if((ShortTouchState.dst_x == 0) && (ShortTouchState.dst_y == 0))
            {
		  #if defined(TPD_XY_INVERT)

				  #if defined(TPD_X_INVERT)
		                cinfo->x[0] = (2048-ShortTouchState.pos_y) * TPD_RES_X / 2048;
				  #else
				  		cinfo->x[0] = (ShortTouchState.pos_y) * TPD_RES_X / 2048;
				  #endif
		
				  #if defined(TPD_Y_INVERT)
		                cinfo->y[0] = (2048-ShortTouchState.pos_x) * TPD_RES_Y / 2048;	
				  #else
		                cinfo->y[0] = ShortTouchState.pos_x * TPD_RES_Y / 2048;	
				  #endif
		  
		  #else

				  #if defined(TPD_X_INVERT)
		                cinfo->x[0] = (2048-ShortTouchState.pos_x) * TPD_RES_X / 2048;
				  #else
		                cinfo->x[0] = ShortTouchState.pos_x * TPD_RES_X / 2048;
				  #endif
		
				  #if defined(TPD_Y_INVERT)
		                cinfo->y[0] = (2048-ShortTouchState.pos_y) * TPD_RES_Y / 2048;
				  #else
		                cinfo->y[0] = ShortTouchState.pos_y * TPD_RES_Y / 2048;
				  #endif
		  
		  #endif
                point_num = 1;
            }
            else
            {
                #if defined(TPD_XY_INVERT)

				  #if defined(TPD_X_INVERT)
				   		cinfo->x[0] = (2048-ShortTouchState.pos_y) * TPD_RES_X / 2048;
				  #else
		                cinfo->x[0] = ShortTouchState.pos_y * TPD_RES_X / 2048;
				  #endif

				  #if defined(TPD_Y_INVERT)
		                cinfo->y[0] = (2048-ShortTouchState.pos_x) * TPD_RES_Y / 2048;
				  #else
		                cinfo->y[0] = ShortTouchState.pos_x * TPD_RES_Y / 2048;
				  #endif

				  #if defined(TPD_X_INVERT)
		                cinfo->x[1] = (2048-ShortTouchState.pos_y2) * TPD_RES_X / 2048;
				  #else
		                cinfo->x[1] = ShortTouchState.pos_y2 * TPD_RES_X / 2048;
				  #endif

				  #if defined(TPD_Y_INVERT)
		                cinfo->y[1] = (2048-ShortTouchState.pos_x2) * TPD_RES_Y / 2048;
				  #else
		                cinfo->y[1] = ShortTouchState.pos_x2 * TPD_RES_Y / 2048;
				  #endif
		  
		  #else
		  
				  #if defined(TPD_X_INVERT)
				  		cinfo->x[0] = (2048-ShortTouchState.pos_x) * TPD_RES_X / 2048;
				  #else
		                cinfo->x[0] = ShortTouchState.pos_x * TPD_RES_X / 2048;
				  #endif
		
				  #if defined(TPD_Y_INVERT)
		                cinfo->y[0] = (2048-ShortTouchState.pos_y) * TPD_RES_Y / 2048;
				  #else
		                cinfo->y[0] = ShortTouchState.pos_y * TPD_RES_Y / 2048;
				  #endif
		
				  #if defined(TPD_X_INVERT)
		                cinfo->x[1] = (2048-ShortTouchState.pos_x2) * TPD_RES_X / 2048;
				  #else
		                cinfo->x[1] = ShortTouchState.pos_x2 * TPD_RES_X / 2048;
				  #endif
		
				  #if defined(TPD_Y_INVERT)
		                cinfo->y[1] = (2048-ShortTouchState.pos_y2) * TPD_RES_Y / 2048;
				  #else
		                cinfo->y[1] = ShortTouchState.pos_y2 * TPD_RES_Y / 2048;
				  #endif

		  #endif
                point_num = 2;
            }
        }
        return true;
    }
    else
    {
        SSL_PRINT("tpd_check_sum_ XXXX\n");
        return  false;
    }
}

static int touch_event_handler(void *unused)
{
    struct touch_info cinfo;
    int touch_state = 3;
    unsigned long time_eclapse;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag != 0);
        tpd_flag = 0;
        set_current_state(TASK_RUNNING);
        if(tpd_touchinfo(&cinfo))
        {

            if(point_num == 1)
            {
                 printk("\n##############MSG_X1 = %d,MSG_Y1 = %d\n", cinfo.x[0], cinfo.y[0]);
                tpd_down(cinfo.x[0], cinfo.y[0]);
                SSL_PRINT("msg_press 1 point--->\n");
                input_sync(tpd->dev);
            }
            else if(point_num == 2)
            {
                 printk("\n############## MSG_X1 = %d,MSG_Y1 = %d\n", cinfo.x[0], cinfo.y[0]);

                printk("\n############## MSG_X2 = %d,MSG_Y2 = %d\n", cinfo.x[1], cinfo.y[1]);
                tpd_down(cinfo.x[0], cinfo.y[0]);
                tpd_down(cinfo.x[1], cinfo.y[1]);
                SSL_PRINT("msg_press 2 points--->\n");
                input_sync(tpd->dev);
            }
            else if(point_num == 0)
            {
                // printk("\n##############release --->\n");
#ifdef TPD_HOME_KEY_LONG_PRESS
		  tpd_up(cinfo.x[0], cinfo.y[0]);
#else
                input_mt_sync(tpd->dev);
#endif
                input_sync(tpd->dev);
            }
        }
    }
    while(!kthread_should_stop());

    return 0;
}

static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, "msg2133");
    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
//    printk("MSG2133 TPD interrupt has been triggered\n");
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}

static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int retval = TPD_OK;
      char data;
	unsigned char  tp_info[512];
	int len;
#ifdef TP_PROXIMITY_SENSOR
    struct hwmsen_object obj_ps;
#endif
    s32 err = 0;


    //    client->timing = 400;
   //  client->addr |= I2C_ENEXT_FLAG;
    i2c_client = client;

	if (tpd_load_status)
		return -1;
	printk("In tpd_probe_ ,the i2c addr=0x%x", client->addr);

    if(touch_ssb_data.power_id!= MT65XX_POWER_NONE)
    {
        hwPowerDown(touch_ssb_data.power_id,"TP");
        hwPowerOn(touch_ssb_data.power_id,VOL_2800,"TP");
        msleep(100);
    }

    hwPowerOn(MT65XX_POWER_LDO_VIO28, VOL_1800, "TP");
    hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_2800, "TP");
    hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");

    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);		//GPIO_CTP_EN_PIN
    //changed in 2012-07-07 by [Hally]
#ifdef TPD_RSTPIN_1V8
   	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_RST_PIN, GPIO_PULL_DISABLE);
#else
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#endif

    mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

#if 0
    mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
    mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#else
   mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, tpd_eint_interrupt_handler, 1);

#endif
    msleep(100);
	
	{
	    msg2033_i2c_read(&data,1);
	    printk("The CTP_ID=0x%x in %s(),L:%d\n",data,__func__,__LINE__);
	}
	if((i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &data)) < 0)
	{
	    printk("I2C transfer error, func: %s\n", __func__);
	    return -1;
	}
	
    tpd_load_status = 1;

#if ENABLE_DMA
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);

    if(!gpDMABuf_va)
    {
        printk("[MATV][Error] Allocate DMA I2C Buffer failed!\n");
    }

#endif
 //   firmware_class = class_create(THIS_MODULE, "mstar-touchscreen-msg2133A");
    firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
    if(IS_ERR(firmware_class))
    {
        pr_err("Failed to create class(firmware)!\n");
    }
	 firmware_cmd_dev = device_create(firmware_class,
                                     NULL, 0, NULL, "device");

    if(IS_ERR(firmware_cmd_dev))
    {
        pr_err("Failed to create device(firmware_cmd_dev)!\n");
    }

#ifdef APK_UPDATE
    // version
    if(device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
    }

    // update
    if(device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
    }

    // data
    if(device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);
    }
	//clear
	/*
    if(device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);
    }*/

#endif
	msleep(80);
	get_ic_info(&tp_ic_major, &tp_ic_minor);
	ic_fw_version = tp_ic_minor;

#ifdef AUTO_UPGRADE

	thread = kthread_run(auto_upgrade_thread, NULL, "msg2133x_fw_auto_upgrade");
	 if (IS_ERR(thread))
	{ 
		  retval = PTR_ERR(thread);
		  TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
	}

	msleep(10);
	
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);		//GPIO_CTP_EN_PIN
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);		//GPIO_CTP_EN_PIN
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);

	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);		//GPIO_CTP_EN_PIN
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

	msleep(30);
	
#endif

#ifndef AUTO_UPGRADE
	if (tp_ic_major == FN_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","FuNa",client->addr,tp_ic_major,tp_ic_minor);
	else if (tp_ic_major == DIJIN_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","DiJin",client->addr,tp_ic_major,tp_ic_minor);
	else if (tp_ic_major == TRULY_FW_MAJOR)
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","Truly",client->addr,tp_ic_major,tp_ic_minor);
	else
		len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x,TP major :0x%x,TP minor :0x%x,","msg2133A","UN kown",client->addr,tp_ic_major,tp_ic_minor);

	fix_tp_proc_info(tp_info, len);
#endif

#if	defined(NODE_UPGRADE) 
  	
    // node upgrade
    if(device_create_file(firmware_cmd_dev, &dev_attr_upgrade_node) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_upgrade_node.attr.name);
    }
#endif

#if	defined(SELF_APK_UPGRADE) 
    // node upgrade self apk
    // for cit tp firmware upgrade compatable need by anxiang.xiao 20131119
    firmware_class_self_apk = class_create(THIS_MODULE, "tp_firmware");
    if(IS_ERR(firmware_class_self_apk))
    {
        pr_err("Failed to create class(firmware)!\n");
    }
	 firmware_cmd_dev_self_apk = device_create(firmware_class_self_apk,
                                     NULL, 0, NULL, "device");

    if(IS_ERR(firmware_cmd_dev_self_apk))
    {
        pr_err("Failed to create device(firmware_cmd_dev)!\n");
    }
	// end 
    if(device_create_file(firmware_cmd_dev_self_apk, &dev_attr_upgrade_node_self_apk) < 0)
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_upgrade_node.attr.name);
    }
#endif

#ifdef TP_PROXIMITY_SENSOR
    //obj_ps.self = cm3623_obj;
    obj_ps.polling = 0;         //0--interrupt mode;1--polling mode;
    obj_ps.sensor_operate = tpd_ps_operate;

    if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        GTP_ERROR("hwmsen attach fail, return:%d.", err);
    }
    if(device_create_file(firmware_cmd_dev, &dev_attr_proximity_sensor) < 0) // /sys/class/mtk-tpd/device/proximity_sensor
    {
        pr_err("Failed to create device file(%s)!\n", dev_attr_proximity_sensor.attr.name);
    }

#endif
  //dev_set_drvdata(firmware_cmd_dev, NULL);

   thread = kthread_run(touch_event_handler, 0, "msg2133");
    if(IS_ERR(thread))
    {
        retval = PTR_ERR(thread);
        printk("msg2133" " failed to create kernel thread: %d\n", retval);
    }

    //SSL_PRINT("create device file:%s\n", dev_attr_proximity_sensor.attr.name);
    printk("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
    return 0;
}

static int __devexit tpd_remove(struct i2c_client *client)

{
    printk("TPD removed\n");

#if ENABLE_DMA
    if(gpDMABuf_va)
    {
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }

#endif
    return 0;
}


static int tpd_local_init(void)
{
    printk(" MSG2033 I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);

    if(i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        printk("MSG2033 unable to add i2c driver.\n");
        return -1;
    }

    if(touch_ssb_data.use_tpd_button == 1)
        tpd_button_setting(TPD_KEY_COUNT, touch_ssb_data.tpd_key_local, touch_ssb_data.tpd_key_dim_local);// initialize tpd button data

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8 * 4);
#endif
    SSL_PRINT("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    msg2133_priv.buttons=0;
    return 0;
}

static int tpd_resume(struct i2c_client *client)
{
    int retval = TPD_OK;

#ifdef TP_PROXIMITY_SENSOR
    if (tpd_proximity_flag == 1)
    {
        return retval;
    }
#endif
    printk("TPD wake up\n");
#ifdef TPD_CLOSE_POWER_IN_SLEEP
    hwPowerOn(touch_ssb_data.power_id, VOL_3300, "TP");
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	//changed in 2012-07-07 by [Hally]
#ifdef TPD_RSTPIN_1V8
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_RST_PIN, GPIO_PULL_DISABLE);
#else
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#endif      
#endif
    msleep(300);													//changed in 2012-07-07 by [Hally]
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    return retval;
}

static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
    int retval = TPD_OK;
    static char data = 0x3;
    printk("TPD enter sleep\n");

#ifdef TP_PROXIMITY_SENSOR
        if (tpd_proximity_flag == 1){
            // resume ......
            return retval;
        }
#endif

    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);

#ifdef TPD_CLOSE_POWER_IN_SLEEP
    hwPowerDown(touch_ssb_data.power_id, "TP");
#else
    //i2c_smbus_write_i2c_block_data(i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
    //mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
    //mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
#endif
    return retval;
}


static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "MSG2133",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
};
/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{

    int err = 0;
    char name[20] = "msg2133";
    printk("MediaTek MSG2033 touch panel driver init\n");
    err = tpd_ssb_data_match(name, &touch_ssb_data);
    if(err != 0){
        printk("touch tpd_ssb_data_match error\n");
        return -1;
    }
    printk("msg2133 touch_ssb_data:: name:(%s), endflag:0x%x, i2c_number:0x%x, i2c_addr:0x%x,power_id:%d, use_tpd_button:%d\n",
    touch_ssb_data.identifier,
    touch_ssb_data.endflag,
    touch_ssb_data.i2c_number,
    touch_ssb_data.i2c_addr,
    touch_ssb_data.power_id,
    touch_ssb_data.use_tpd_button
    );


    msg2133_i2c_tpd.addr = touch_ssb_data.i2c_addr;

    i2c_register_board_info(touch_ssb_data.i2c_number, &msg2133_i2c_tpd, 1);

    //add for ssb support
    tpd_device_driver.tpd_have_button = touch_ssb_data.use_tpd_button;

    if(tpd_driver_add(&tpd_device_driver) < 0)
    {
        printk("add MSG2033 driver failed\n");
    }

    return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
    printk("MediaTek MSG2033 touch panel driver exit\n");
    //input_unregister_device(tpd->dev);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);



