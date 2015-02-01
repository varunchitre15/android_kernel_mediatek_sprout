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
#include <stdio.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef BUILD_LK
#define LCM_PRINT printf
#else
#define LCM_PRINT pr_debug
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;
static struct LCM_setting_table *para_init_table = NULL;
static unsigned int para_init_size = 0;
static LCM_PARAMS *para_params = NULL;

static unsigned int lcm_driver_id = 0x0;
static unsigned int lcm_module_id = 0x0;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define LCM_ID_NT35590        (0x90)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)            lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


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
           }
    }

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_get_id(unsigned int* driver_id, unsigned int* module_id)
{
    *driver_id = lcm_driver_id;
    *module_id = lcm_module_id;
}


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
        params->type = LCM_TYPE_DSI;
        params->width = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;

        params->dsi.mode = CMD_MODE;
        params->dsi.LANE_NUM = LCM_THREE_LANE;
        params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
        params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

        params->dsi.PLL_CLOCK = 221;
    }
}


static void init_lcm_registers(void)
{
    unsigned int data_array[16];

    data_array[0]=0x00023902;
    data_array[1]=0x000008C2;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0]=0x00023902;
    data_array[1]=0x000002BA;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0]=0x00023902;
    data_array[1]=0x0000773A;
    dsi_set_cmdq(data_array, 2, 1);

    // ********************  EABLE DSI TE packet **************//
    data_array[0]=0x00351500;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0]=0x773a1500;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0]= 0x00053902;
    data_array[1]= 0x0100002a;
    data_array[2]= 0x000000df;
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x00053902;
    data_array[1]= 0x0300002b;
    data_array[2]= 0x00000055;
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0]= 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
}


static void lcm_init(void)
{
    int i, j;
    int size;

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(20);

    if (para_init_table != NULL)
    {
        push_table(para_init_table, para_init_size, 1);
    }
    else
    {
        init_lcm_registers();
    }
}


static void lcm_suspend(void)
{
    unsigned int data_array[16];

    data_array[0] = 0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    data_array[0] = 0x014F1500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(40);
}


static void lcm_resume(void)
{
    lcm_init();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);
}


static void lcm_setbacklight(unsigned int level)
{
    unsigned int data_array[16];

    LCM_PRINT("lcm_setbacklight = %d\n", level);

    if (level > 255)
        level = 255;

    data_array[0]= 0x00023902;
    data_array[1] =(0x51|(level<<8));
    dsi_set_cmdq(data_array, 2, 1);
}


static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[2];
    unsigned int array[16];

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);

    SET_RESET_PIN(1);
    MDELAY(20);

    array[0] = 0x00023700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xF4, buffer, 2);
    id = buffer[0]; //we only need ID

    lcm_driver_id = id;
    // TBD
    lcm_module_id = 0x0;

    #ifdef BUILD_LK
        printf("%s, LK nt35590 debug: nt35590 id = 0x%08x\n", __func__, id);
    #else
        pr_debug("%s, kernel nt35590 horse debug: nt35590 id = 0x%08x\n", __func__, id);
    #endif

    if(id == LCM_ID_NT35590)
        return 1;
    else
        return 0;
}


// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35590_dsi_cmd_6571_fwvga_lcm_drv =
{
    .name           = "nt35590_dsi_cmd_6571_fwvga",
    .set_util_funcs = lcm_set_util_funcs,
    .set_params     = lcm_set_params,
    .get_id     = lcm_get_id,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .set_backlight  = lcm_setbacklight,
    .compare_id     = lcm_compare_id,
    .update         = lcm_update
};

