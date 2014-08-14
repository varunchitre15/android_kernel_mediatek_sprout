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

typedef struct
{
    /* MID + PNM in CID register */	
    u8   m_id;         // Manufacturer ID
    char pro_name[8];  // Product name

    u8   r_smpl;
    u8   d_smpl;
    u8   cmd_rxdly;
    u8   rd_rxdly;
    u8   wr_rxdly;
}mmcdev_info,*pmmcdev_info;

static const mmcdev_info g_mmcTable[] = {
    // hynix
    {0x90,	"HYNIX ",	0,	0,	0,	0,	0},

    // end
    {0x00,	"xxxxxx",	0,	0,	0,	0,	0}
};
