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

#ifdef BUILD_LK
#else
#include <linux/string.h>
#endif

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (480)
#define FRAME_HEIGHT                                         (854)

#define REGFLAG_DELAY                                         0XFE
#define REGFLAG_END_OF_TABLE                                  0x00   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0
#define LCM_ID_ILINT35512						                          0x80

static unsigned int lcm_driver_id;
static unsigned int lcm_module_id;

static struct LCM_setting_table *para_init_table;
static unsigned int para_init_size;
static LCM_PARAMS *para_params ;

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                             (lcm_util.udelay(n))
#define MDELAY(n)                                             (lcm_util.mdelay(n))

//static kal_bool IsFirstBoot = KAL_TRUE;

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        	lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)

static  LCM_setting_table_V3 lcm_initialization_setting[] = {
    {0x39,0xF0,5,{0x55,0xAA,0x52,0x08,0x02}}, //Page 2 enable
    {0x39,0xB2,32,{0x00,0x01,0x04,0x05,0x02,0x03,0x06,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0F,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x09,0x0C,0x0D,0x0A,0x0B,0x0E,0x0F}},                
    {0x39,0xF0,5,{0x55,0xAA,0x52,0x08,0x00}}, //CMD2_Page0
    {0x15,0xB5,1,{0x6B}}, //## 480x854
    {0x15,0xB6,1,{0x02}},
    {0x39,0xB7,2,{0x00,0x00}},
    {0x39,0xB8,4,{0x01,0x07,0x07,0x07}},
    {0x15,0xBA,1,{0x01}},
    {0x39,0xBB,2,{0x22,0x02}},
    {0x15,0xBC,1,{0x02}},
    {0x39,0xBD,5,{0x01,0x84,0x1A,0x1C,0x00}},//## AUO GOA mapping
    {0x39,0xC8,18,{0x81,0x0A,0x46,0x64,0x46,0x64,0x46,0x64,0x46,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64}},                 
    {0x39,0xF0,5,{0x55,0xAA,0x52,0x08,0x01}}, //CMD2_Page1
    {0x15,0xB0,1,{0x08}},
    {0x15,0xB1,1,{0x08}},
    {0x15,0xB2,1,{0x00}},
    {0x15,0xB3,1,{0x08}},
    {0x15,0xB5,1,{0x0A}},
    {0x15,0xB6,1,{0x44}},
    {0x15,0xB7,1,{0x35}},
    {0x15,0xB8,1,{0x24}},
    {0x15,0xB9,1,{0x35}},
    {0x15,0xBA,1,{0x24}},
    {0x39,0xBC,3,{0x00,0x98,0x00}},
    {0x39,0xBD,3,{0x00,0x98,0x00}},
    {0x15,0xBF,1,{0x01}},
    {0x39,0xBE,2,{0x00,0x48}}, //VCOM
    {0x15,0xE0,1,{0x01}},//## Positive Gamma
    {0x39,0xD1,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},
    {0x39,0xD2,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},
    {0x39,0xD3,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},//## negative Gamma 
    {0x39,0xD4,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},
    {0x39,0xD5,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},
    {0x39,0xD6,52,{0x00,0x00,0x00,0x0F,0x00,0x2A,0x00,0x42,0x00,0x5B,0x00,0x7E,0x00,0x9F,0x00,0xD0,0x00,0xF8,0x01,0x39,0x01,0x6B,0x01,0xBB,0x01,0xFD,0x01,0xFF,0x02,0x3A,0x02,0x7E,0x02,0xA6,0x02,0xDA,0x02,0xFA,0x03,0x29,0x03,0x46,0x03,0x6A,0x03,0x80,0x03,0x9C,0x03,0xBA,0x03,0xFF}},
    {0x15,0x11,1,{0x00}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 120, {}},
    {0x15,0x29,1,{0x00}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 10, {}},
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 0, {0x00}},

    // Sleep Mode On
    {0x10, 0, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};





static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                //MDELAY(10);//soso add or it will fail to send register
           }
    }

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_set_params(struct LCM_setting_table *init_table, unsigned int init_size, LCM_PARAMS *params)
{
    para_init_table = init_table;
    para_init_size = init_size;
    para_params = params;
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    if (para_params != NULL)
    {
        memcpy(params, para_params, sizeof(LCM_PARAMS));
    }
    else
    {
        params->type   = LCM_TYPE_DSI;
        params->width  = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;
        params->dsi.mode   = SYNC_PULSE_VDO_MODE;
        // DSI
        /* Command mode setting */
        params->dsi.LANE_NUM                = LCM_TWO_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
        params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
        params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        // Highly depends on LCD driver capability.
        // Not support in MT6573
        params->dsi.packet_size=256;
        // Video mode setting
        params->dsi.intermediat_buffer_num = 2;
        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
        params->dsi.vertical_sync_active				= 6;// 3    2
        params->dsi.vertical_backporch					= 14;// 20   1
        params->dsi.vertical_frontporch					= 20; // 1  12
        params->dsi.vertical_active_line				= FRAME_HEIGHT;
        params->dsi.horizontal_sync_active				= 10;// 50  2
        params->dsi.horizontal_backporch				= 60;
        params->dsi.horizontal_frontporch				= 60;
        params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
        // Bit rate calculation
        params->dsi.PLL_CLOCK=208;
        params->dsi.pll_div1=30;//32        // fref=26MHz, fvco=fref*(div1+1)    (div1=0~63, fvco=500MHZ~1GHz)
        params->dsi.pll_div2=1;         // div2=0~15: fout=fvo/(2*div2)
#if (LCM_DSI_CMD_MODE)
        params->dsi.fbk_div =7;
#else
        params->dsi.fbk_div =7;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
#endif  // div2=0~15: fout=fvo/(2*div2)
    }
}

static void lcm_get_id(unsigned int* driver_id, unsigned int* module_id)
{
    *driver_id = lcm_driver_id;
    *module_id = lcm_module_id;
}
static void lcm_init(void)
{

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms

    if (para_init_table != NULL)
    {
        push_table(para_init_table, para_init_size, 1);
    }
    else
    {
        dsi_set_cmdq_V3(lcm_initialization_setting,ARRAY_SIZE(lcm_initialization_setting),1);
    }
    //push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms

    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
    //lcm_compare_id();

    lcm_init();

    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
    unsigned int id = 0;
    unsigned char buffer[3];
    unsigned int array[16];

    SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0x04, buffer, 3);
    id = buffer[1]; //we only need ID
    lcm_driver_id = id;
    lcm_module_id = 0x0;
#if defined(BUILD_UBOOT)
    /*The Default Value should be 0x00,0x80,0x00*/
    printf("\n\n\n\n[soso]%s, id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x\n", __func__, buffer[0],buffer[1],buffer[2]);
#endif
    return (id == 0x80)?1:0;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
    char  buffer[2];
    int   array[1];
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0x0a, buffer, 2);
    return buffer[0]!=0x9c;
#endif
}

static unsigned int lcm_esd_recover(void)
{
    lcm_init();
#ifndef BUILD_LK
    printk("sym lcm_esd_recover  nt35512s_dsi_vdo_djn_auo_fwvga_ips \n");
#endif
    return TRUE;
}
LCM_DRIVER nt35512_wvga_dsi_vdo_boe_drv =
{
    .name           = "nt35512_dsi_vdo_lcm_drv",
    .set_util_funcs = lcm_set_util_funcs,
    .set_params     = lcm_set_params,
    .get_id         = lcm_get_id,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .esd_check      = lcm_esd_check,
    .esd_recover    = lcm_esd_recover,
};

