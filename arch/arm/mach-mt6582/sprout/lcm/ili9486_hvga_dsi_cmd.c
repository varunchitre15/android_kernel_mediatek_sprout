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

#include "lcm_drv.h"


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (320)
#define FRAME_HEIGHT                                         (480)
#define LCM_ID       (0x86)
#define REGFLAG_DELAY                                         0XFE
#define REGFLAG_END_OF_TABLE                                  0xFF   // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                             (lcm_util.udelay(n))
#define MDELAY(n)                                             (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


static struct LCM_setting_table lcm_initialization_setting[] = {


//{REGFLAG_DELAY, 120, {}},
{0xC0, 2, {0x10,0x0d}},
//{REGFLAG_DELAY, 10, {}},
//{0xC1, 2, {0x41,0x00}},
{0xC1, 1, {0x42}},
//{REGFLAG_DELAY, 10, {}},
//{0XC5, 3, {0x00,0x5a,0x80}},54
{0XC5, 3, {0x00,0x54,0x80}},
//{REGFLAG_DELAY, 10, {}},
{0xB1, 2, {0xB0,0x11}},
//{REGFLAG_DELAY, 10, {}},
{0xB4, 1, {0x02}},
//{REGFLAG_DELAY, 10, {}},
{0xE0, 15, {0x0f,0x21,0x20,0x0b,0x0f,0x08,0x4f,0xf1,0x3f,0x08,0x0d,0x00,0x00,0x00,0x00}},
 //{REGFLAG_DELAY, 10, {}},
{0xE1, 15, {0x0f,0x3f,0x3f,0x0f,0x12,0x07,0x40,0x0e,0x30,0x07,0x10,0x04,0x20,0x1e,0x00}},
//{REGFLAG_DELAY, 10, {}},
{0XF1, 6, {0x36,0x04,0x00,0x3C,0x0F,0x8F}},
//{REGFLAG_DELAY, 10, {}},
{0XF2, 9, {0x18,0xA3,0x12,0x02,0xb2,0x12,0xFF,0x10,0x00}},
//{0xF7, 5, {0xA9,0x91,0x2D,0x8A,0x4F}},
{0xF7, 4, {0xA9,0x91,0x2D,0x8A}},
//{REGFLAG_DELAY, 10, {}},
{0XF8, 2, {0x21,0x04}},
//{REGFLAG_DELAY, 10, {}},
{0XF9, 2, {0x00,0x08}},
//{REGFLAG_DELAY, 10, {}},
{0x36, 1, {0x48}},
{0x3A, 1, {0x66}},
{0x11, 1, {0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29, 1, {0x00}},
//{REGFLAG_DELAY, 120, {}},

{REGFLAG_END_OF_TABLE, 0x00, {}}


};


static struct LCM_setting_table lcm_set_window[] = {
    {0x2A,    4,    {0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
    {0x2B,    4,    {0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 250, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_compare_id_setting[] = {
    // Display off sequence
//    {0xB9,    3,    {0xFF, 0x83, 0x63}},
//    {REGFLAG_DELAY, 10, {}},

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


static void lcm_get_params(LCM_PARAMS *params)
{
        memset(params, 0, sizeof(LCM_PARAMS));

        params->type   = LCM_TYPE_DSI;

        params->width  = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;

        // enable tearing-free
        params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
        params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

        params->dsi.mode   = CMD_MODE;

        // DSI
        /* Command mode setting */
        params->dsi.LANE_NUM                = LCM_ONE_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
        params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
        params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB666;

        // Highly depends on LCD driver capability.
        params->dsi.packet_size=256;
        // Video mode setting
        params->dsi.PS=LCM_PACKED_PS_18BIT_RGB666;

        params->dsi.word_count=320*3;
        params->dsi.vertical_sync_active=2;
        params->dsi.vertical_backporch=2;
        params->dsi.vertical_frontporch=2;
        params->dsi.vertical_active_line=480;

        params->dsi.line_byte=2180;        // 2256 = 752*3
        params->dsi.horizontal_sync_active_byte=26;
        params->dsi.horizontal_backporch_byte=206;
        params->dsi.horizontal_frontporch_byte=206;
        params->dsi.rgb_byte=(320*3+6);

        params->dsi.horizontal_sync_active_word_count=20;
        params->dsi.horizontal_backporch_word_count=200;
        params->dsi.horizontal_frontporch_word_count=200;

        // Bit rate calculation
        params->dsi.pll_div1=18;        // fref=26MHz, fvco=fref*(div1+1)    (div1=0~63, fvco=500MHZ~1GHz)18
        params->dsi.pll_div2=1;            // div2=0~15: fout=fvo/(2*div2)

}

static unsigned int lcm_compare_id()
{
    unsigned int count=0,id = 0;
    unsigned char buffer[4]={0};
    unsigned int array[16];
        SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
        MDELAY(10);
        SET_RESET_PIN(0);
        MDELAY(10);
        SET_RESET_PIN(1);
        MDELAY(100);

//    push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);

    array[0] = 0x00043700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
//    id = read_reg(0xF4);d3

    count = read_reg_v2(0x04, buffer, 4);
#if defined(BUILD_UBOOT)
    printf("ddddd buffer[0]=0x%x,buffer[1]=0x%x,buffer[2]=0x%x,buffer[3]=0x%x,count=%d\n",buffer[0],buffer[1],buffer[2],buffer[3],count);
#endif

    id = buffer[0]; //we only need ID

    return (LCM_ID == id)?1:0;
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(200);
lcm_compare_id();
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{

lcm_init();
/*
lcm_initialization_setting[2].para_list[1]+=1;
#if defined(BUILD_UBOOT)
    printf("xxxlcm_initialization_setting[2].para_list[1]=0x%x\n",lcm_initialization_setting[2].para_list[1]);
#else
    printk("xxxlcm_initialization_setting[2].para_list[1]=0x%x\n",lcm_initialization_setting[2].para_list[1]);
#endif
*/
    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
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
        data_array[3]= 0x00053902;
        data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
        data_array[5]= (y1_LSB);
        data_array[6]= 0x002c3909;

        dsi_set_cmdq(&data_array, 7, 0);

    }



static void lcm_setbacklight(unsigned int level)
{
    unsigned int default_level = 0;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255)
            level = 255;

    if(level >0)
            mapped_level = default_level+(level)*(255-default_level)/(255);
    else
            mapped_level=0;

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_setpwm(unsigned int divider)
{
    // TBD
}


static unsigned int lcm_getpwm(unsigned int divider)
{
    // ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
    // pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
    unsigned int pwm_clk = 23706 / (1<<divider);
    return pwm_clk;
}


// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9486_hvga_dsi_cmd_lcm_drv =
{
    .name            = "ili9486_hvga_dsi_cmd",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .update         = lcm_update,
    //.set_backlight    = lcm_setbacklight,
    //.set_pwm        = lcm_setpwm,
    //.get_pwm        = lcm_getpwm
    .compare_id    = lcm_compare_id,
};
