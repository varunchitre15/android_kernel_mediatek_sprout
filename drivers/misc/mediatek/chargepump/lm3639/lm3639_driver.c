/*  Date: 2011/8/8 11:00:00
 *  Revision: 1.6
 */

/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */


/* file lm3639.c
   brief This file contains all function implementations for the lm3639 in linux

*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/kernel.h>
#include <linux/delay.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/mt_pm_ldo.h>

#include <linux/platform_device.h>
#include <cust_acc.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>

#include <linux/leds.h>

#define LCD_LED_MAX 0x7F
#define LCD_LED_MIN 0

//#define DEFAULT_BRIGHTNESS 0x6B //for 26mA
#define DEFAULT_BRIGHTNESS 0x73 //for 20mA

#define LM3530_MIN_BRIGHTNESS	0x57//0x4F
#define LM3530_DEFAULT_BRIGHTNESS	0x73//for 20mA

#define LM3530_MIN_VALUE_SETTINGS 30 /* value for LM3530_MIN_BRIGHTNESS in leds_brightness_set*/
#define LM3530_DEFAULT_VALUE_SETTINGS 102 /* value for LM3530_DEFAULT_BRIGHTNESS in leds_brightness_set*/


#define BACKLIHGT_NAME 			"charge-pump"

#define Lm3639_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define LM3639_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define LM3639_DEV_NAME        "charge-pump"

#define CPD_TAG                  "[ChargePump] "
#define CPD_FUN(f)               printk(CPD_TAG"%s\n", __FUNCTION__)
#define CPD_ERR(fmt, args...)    printk(CPD_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define CPD_LOG(fmt, args...)    printk(CPD_TAG fmt, ##args)

// I2C variable
static struct i2c_client *new_client = NULL;
static const struct i2c_device_id lm3639_i2c_id[] = {{LM3639_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_lm3639={ I2C_BOARD_INFO("charge-pump", 0x39)};

// Flash control
unsigned char strobe_ctrl;
unsigned char flash_ctrl;

// Gamma 2.2 Table
#if 0
unsigned char bright_arr[] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13,
                               14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 24, 25, 26,
                               26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 38, 39, 40, 41, 43, 44, 45, 47, 48,
                               49, 51, 52, 54, 55, 57, 58, 60, 62, 63, 65, 67, 69, 70, 72, 74, 76, 78, 80, 82,
                               84, 86, 88, 90, 92, 94, 96, 98,101,103,105,107,110,112,115,117,119,122,124,127};

#else
//LGE_CHANGE_S : 2012-11-08 : hyunjin2.lim 
#if 0
unsigned char bright_arr[] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13,
                               13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 20, 20, 21, 22, 23, 23, 24, 25,
                               26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 40, 41, 42, 43, 45, 46, 48,
                               49, 50, 52, 53, 55, 56, 58, 60, 61, 63, 65, 66, 68, 70, 72, 73, 75, 77, 79, 81,
                               83, 85, 87, 89, 91, 93, 96, 98,100,102,105,107,109,112,114,116,119,121,124,127};

// Gamma 2.3 Table
unsigned char bright_arr[] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12,
                               12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 23,
                               24, 25, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 38, 39, 40, 42, 43, 44, 
                               46, 47, 48, 50, 51, 53, 54, 56, 58, 59, 61, 63, 64, 66, 68, 70, 72, 74, 76, 78,
                               80, 82, 84, 86, 88, 90, 92, 94, 97, 99,101,104,106,109,111,113,116,119,121,124,
                               127};
#else
// Gamma 2.25 Table
unsigned char bright_arr[] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12,
                               13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 23, 24,
                               24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 38, 39, 40, 41, 43, 44, 45,
                               47, 48, 49, 51, 52, 54, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 73, 74, 76, 78,
                               80, 82, 84, 86, 89, 91, 93, 95, 97, 100, 102, 104, 106, 109, 111, 114, 116, 119, 121, 124,
                               127};
#endif
//LGE_CHANGE_E : 2012-11-08 : hyunjin2.lim
#endif
static unsigned char current_brightness = 0;
static unsigned char is_suspend = 0;

/* generic */
#define LM3639_MAX_RETRY_I2C_XFER (100)
#define LM3639_I2C_WRITE_DELAY_TIME 1
// [LGE_UPDATE_S] [dongyu.gwak@lge.com] [2012-12-27] current limit 
typedef struct
{
    kal_bool       bat_exist;
    kal_bool       bat_full;
    kal_bool       bat_low;
    UINT32      bat_charging_state;
    UINT32      bat_vol;
    kal_bool     charger_exist;
    UINT32      pre_charging_current;
    UINT32      charging_current;
    INT32      charger_vol;
    UINT32       charger_protect_status;
    UINT32      ISENSE;
    UINT32      ICharging;
    INT32       temperature;
    UINT32      total_charging_time;
    UINT32      PRE_charging_time;
    UINT32      CC_charging_time;
    UINT32      TOPOFF_charging_time;
    UINT32      POSTFULL_charging_time;
    UINT32       charger_type;
    UINT32       PWR_SRC;
    UINT32       SOC;
    UINT32       ADC_BAT_SENSE;
    UINT32       ADC_I_SENSE;
} PMU_ChargerStruct;

extern PMU_ChargerStruct BMT_status;
// [LGE_UPDATE_E] [dongyu.gwak@lge.com] [2012-12-27]

/*	i2c read routine for API*/
static char lm3639_i2c_read(struct i2c_client *client, u8 reg_addr,
		u8 *data, u8 len)
{
#if !defined BMA_USE_BASIC_I2C_FUNC
	s32 dummy;
	if (NULL == client)
		return -1;

	while (0 != len--) {
#ifdef BMA_SMBUS
		dummy = i2c_smbus_read_byte_data(client, reg_addr);
		if (dummy < 0) {
			CPD_ERR("i2c bus read error");
			return -1;
		}
		*data = (u8)(dummy & 0xff);
#else
		dummy = i2c_master_send(client, (char *)&reg_addr, 1);
		if (dummy < 0)
		{
            printk("send dummy is %d", dummy);
			return -1;
		}

		dummy = i2c_master_recv(client, (char *)data, 1);
		if (dummy < 0)
		{
            printk("recv dummy is %d", dummy);
			return -1;
		}
#endif
		reg_addr++;
		data++;
	}
	return 0;
#else
	int retry;

	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &reg_addr,
		},

		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = len,
		 .buf = data,
		 },
	};

	for (retry = 0; retry < LM3639_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) > 0)
			break;
		else
			mdelay(LM3639_I2C_WRITE_DELAY_TIME);
	}

	if (LM3639_MAX_RETRY_I2C_XFER <= retry) {
		CPD_ERR("I2C xfer error");
		return -EIO;
	}

	return 0;
#endif
}

/*	i2c write routine for */
static char lm3639_i2c_write(struct i2c_client *client, u8 reg_addr,
		u8 *data, u8 len)
{
#if !defined BMA_USE_BASIC_I2C_FUNC
	s32 dummy;

#ifndef BMA_SMBUS
	u8 buffer[2];
#endif

	if (NULL == client)
		return -1;

	while (0 != len--) {
#if 1//def BMM_SMBUS
		dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
#else
		buffer[0] = reg_addr;
		buffer[1] = *data;
		dummy = i2c_master_send(client, (char *)buffer, 2);
#endif
		reg_addr++;
		data++;
		if (dummy < 0) {
			return -1;
		}

	}
	return 0;
#else
	u8 buffer[2];
	int retry;
	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = 2,
		 .buf = buffer,
		 },
	};

	while (0 != len--) {
		buffer[0] = reg_addr;
		buffer[1] = *data;
		for (retry = 0; retry < LM3639_MAX_RETRY_I2C_XFER; retry++) {
			if (i2c_transfer(client->adapter, msg,
						ARRAY_SIZE(msg)) > 0) {
				break;
			} else {
				mdelay(LM3639_I2C_WRITE_DELAY_TIME);
			}
		}
		if (LM3639_MAX_RETRY_I2C_XFER <= retry) {
			return -EIO;
		}
		reg_addr++;
		data++;
	}

	return 0;
#endif
}



static int lm3639_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	return lm3639_i2c_read(client,reg_addr,data,1);
}

static int lm3639_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	int ret_val = 0;
	int i = 0;

	ret_val = lm3639_i2c_write(client,reg_addr,data,1);

	for ( i = 0; i < 5; i++)
	{
		if (ret_val != 0)
			lm3639_i2c_write(client,reg_addr,data,1);
		else
			return ret_val;
	}
	return ret_val;
}

static int lm3639_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	return lm3639_i2c_read(client,reg_addr,data,len);
}

int chargepump_set_backlight_level(unsigned int level)
{
	unsigned char data = 0;
	unsigned char data1 = 0;
	unsigned int bright_per = 0;

	if (level == 0) {
		//printk( "------	 backlight_level suspend-----\n");
		data1 = 0x00; //backlight2 brightness 0
		lm3639_smbus_write_byte(new_client, 0x05, &data1);
		
		data1 = 0x00; //backlight disable
		lm3639_smbus_write_byte(new_client, 0x0A, &data1);
		
		mt_set_gpio_out(GPIO_LCD_BL_EN,GPIO_OUT_ZERO);
		is_suspend = 1;
	}
	else 
	{
		//Gamma 2.2 Table adapted
		bright_per = (level - (unsigned int)20) *(unsigned int)100 / (unsigned int)235;
		data = bright_arr[bright_per];
		//printk("ksw %s bright_per = %d, data = %d\n", __func__, bright_per, data);
		if (is_suspend == 1) {
			//printk( "------	 backlight_level resume-----\n");
			is_suspend = 0;
			mt_set_gpio_out(GPIO_LCD_BL_EN,GPIO_OUT_ONE);
			mdelay(10);
			lm3639_smbus_write_byte(new_client, 0x05, &data);
			
			data1 = 0x19;//backlight enable
			lm3639_smbus_write_byte(new_client, 0x0A, &data1);
		}
		
		lm3639_smbus_write_byte(new_client, 0x05, &data);
	}
	return 0;
}

static int lm3639_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{   
	int ret;
	unsigned char data = 0x01;
	new_client = client;

    CPD_FUN();
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CPD_LOG("i2c_check_functionality error\n");
		return -1;
	}

	if (client == NULL) 
		printk("%s client is NULL\n", __func__);
	else
	{
		printk("%s %x %x %x\n", __func__, client->adapter, client->addr, client->flags);
	}
	return 0;
}


static int lm3639_remove(struct i2c_client *client)
{
    new_client = NULL;
	return 0;
}


static int lm3639_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{ 
	return 0;
}

static struct i2c_driver lm3639_driver = {
	.driver = {
//		.owner	= THIS_MODULE,
		.name	= LM3639_DEV_NAME,
	},
	.probe		= lm3639_probe,
	.remove		= lm3639_remove,
//	.detect		= lm3639_detect,
	.id_table	= lm3639_i2c_id,
//	.address_data = &lm3639250_i2c_addr_data,
};

static int lm3639_pd_probe(struct platform_device *pdev) 
{
	mt_set_gpio_mode(GPIO_LCD_BL_EN, GPIO_LCD_BL_EN_M_GPIO);
	mt_set_gpio_pull_enable(GPIO_LCD_BL_EN, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCD_BL_EN, GPIO_DIR_OUT);

	i2c_add_driver(&lm3639_driver);
	return 0;
}

static int lm3639_pd_remove(struct platform_device *pdev)
{
    CPD_FUN();
    i2c_del_driver(&lm3639_driver);
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void lm3639_early_suspend(struct early_suspend *h)
{
	int err = 0;
	unsigned char data;

	data = 0x00; //backlight2 brightness 0
	err = lm3639_smbus_write_byte(new_client, 0x05, &data);

	data = 0x00; //backlight disable
	err = lm3639_smbus_write_byte(new_client, 0x0A, &data);

	mt_set_gpio_out(GPIO_LCD_BL_EN,GPIO_OUT_ZERO);
}

void lm3639_flash_strobe_prepare(char OnOff,char ActiveHigh)
{
	int err = 0;

	strobe_ctrl=0;
	flash_ctrl=0;

    err = lm3639_smbus_read_byte(new_client, 0x09, &strobe_ctrl);        

    err = lm3639_smbus_read_byte(new_client, 0x0A, &flash_ctrl);

	if(ActiveHigh)
	{
		strobe_ctrl |= 0x20;
	}
	else
	{
		strobe_ctrl &= 0xDF;
	}

// [LGE_UPDATE_S] [dongyu.gwak@lge.com] [2012-09-27] torch mode
	if(OnOff == 1)
	{
		CPD_LOG("Strobe mode On\n");
		strobe_ctrl |= 0x10;		
        flash_ctrl |= 0x66;
    }
    else if(OnOff == 2)
    {
		CPD_LOG("Torch mode On\n");
		strobe_ctrl |= 0x10;
        flash_ctrl |= 0x42; // [LGE_UPDATE] [dongyu.gwak@lge.com] [2012-11-09] flash adjustment
    }
// [LGE_UPDATE_E] [dongyu.gwak@lge.com] [2012-09-27]
	else
	{
		CPD_LOG("Flash Off\n");
		strobe_ctrl &= 0xEF;
		flash_ctrl &= 0x99;  // [LGE_UPDATE] [dongyu.gwak@lge.com] [2012-11-09] flash adjustment
	}

	err = lm3639_smbus_write_byte(new_client, 0x09, &strobe_ctrl);

}

//strobe enable
void lm3639_flash_strobe_en()
{
	int err = 0;

	err = lm3639_smbus_write_byte(new_client, 0x0A, &flash_ctrl); 

}


//strobe level
void lm3639_flash_strobe_level(char level)
{
	int err = 0;
	unsigned char data1=0;
    unsigned char data2=0;
// [LGE_UPDATE_S] [dongyu.gwak@lge.com] [2012-11-27] torch AF assist
    unsigned char torch_level;
    unsigned char strobe_timeout = 0x1F; // [LGE_UPDATE] [dongyu.gwak@lge.com] [2012-12-13] preflash timeout to 1024ms
    
    if( level == 1) 
    {
        torch_level = 0x20;
    }
    else
    {
        torch_level = 0x50;
    }

        
    err = lm3639_smbus_read_byte(new_client, 0x06, &data1);
// [LGE_UPDATE_S] [dongyu.gwak@lge.com] [2012-11-17] Preflash
	if(31 < level)
    {    
		data1= torch_level | 0x06; // [LGE_UPDATE] [dongyu.gwak@lge.com] [2013-01-12] dark image in long distance shot
        strobe_timeout = 0x0F;// [LGE_UPDATE] [dongyu.gwak@lge.com] [2012-12-13] preflash timeout to 1024ms
    }
    else if(level < 0)
    {
		data1= torch_level ;
    }
    else
    {
		data1= torch_level | level; 
    }
// [LGE_UPDATE_S] [dongyu.gwak@lge.com] [2012-12-27]
    if(BMT_status.temperature <= (-15)) // [LGE_UPDATE] [dongyu.gwak@lge.com] [2013-01-07] current limit tuning
    {
	    CPD_LOG("Batt temp=%d\n", BMT_status.temperature );
        
        torch_level = 0xF0 & data1;
        level = 0x0F & data1;
        torch_level = 0xF0 & (torch_level >> 2);
        level = 0x0F & (level >> 2);
        
        data1 = torch_level | level;
    }
// [LGE_UPDATE_E] [dongyu.gwak@lge.com] [2012-12-27]    
// [LGE_UPDATE_E] [dongyu.gwak@lge.com] [2012-11-17] 
// [LGE_UPDATE_E] [dongyu.gwak@lge.com] [2012-11-27]
	CPD_LOG("Flash Level =0x%x\n", data1);
    err = lm3639_smbus_write_byte(new_client, 0x06, &data1);
    
    data2 = 0x40 | strobe_timeout;
    CPD_LOG("Storbe Timeout =0x%x\n", data2);
    err |= lm3639_smbus_write_byte(new_client, 0x07, &data2);
}

static void lm3639_late_resume(struct early_suspend *h)
{
	int err = 0;
	unsigned char data1;
	
	mt_set_gpio_out(GPIO_LCD_BL_EN, GPIO_OUT_ONE);
	mdelay(50);
	
	err = lm3639_smbus_write_byte(new_client, 0x05, &current_brightness);
	
	data1 =0x19;//backlight enable
	err = lm3639_smbus_write_byte(new_client, 0x0A, &data1);
}

static struct early_suspend lm3639_early_suspend_desc = {
	.level		= EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
	.suspend	= lm3639_early_suspend,
	.resume		= lm3639_late_resume,
};
#endif


static struct platform_driver lm3639_backlight_driver = {
	.probe      = lm3639_pd_probe,
	.remove     = lm3639_pd_remove,
	.driver     = {
		.name  = "charge-pump",
		.owner = THIS_MODULE,
	}
};      

static int __init lm3639_init(void)
{
    CPD_FUN();
   
	i2c_register_board_info(1, &i2c_lm3639, 1); 
#ifndef	CONFIG_MTK_LEDS
	register_early_suspend(&lm3639_early_suspend_desc);
#endif	
	if(platform_driver_register(&lm3639_backlight_driver))
	{
	 CPD_ERR("failed to register driver");
	 return -1;
	}

	return 0;
}

static void __exit lm3639_exit(void)
{
	platform_driver_unregister(&lm3639_backlight_driver);
}

EXPORT_SYMBOL(lm3639_flash_strobe_en);
EXPORT_SYMBOL(lm3639_flash_strobe_prepare);
EXPORT_SYMBOL(lm3639_flash_strobe_level);

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("lm3639 driver");
MODULE_LICENSE("GPL");

module_init(lm3639_init);
module_exit(lm3639_exit);

