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

#include "si_common.h"
#include "si_cra.h"
#include "si_cra_cfg.h"
pageConfig_t    g_addrDescriptor[SII_CRA_MAX_DEVICE_INSTANCES][SII_CRA_DEVICE_PAGE_COUNT] =
{
    {
    { DEV_I2C_0,  DEV_PAGE_TPI_0    },  
    { DEV_I2C_0,  DEV_PAGE_TX_L0_0  },  
    { DEV_I2C_0,  DEV_PAGE_TX_L1_0  },  
    { DEV_I2C_0,  DEV_PAGE_TX_2_0   },  
    { DEV_I2C_0,  DEV_PAGE_TX_3_0   },  
    { DEV_I2C_0,  DEV_PAGE_CBUS_0   },  
    { DEV_DDC_0,  DEV_PAGE_DDC_EDID },  
    { DEV_DDC_0,  DEV_PAGE_DDC_SEGM }   
    },
    {
    { DEV_I2C_0,  DEV_PAGE_TPI_1    },  
    { DEV_I2C_0,  DEV_PAGE_TX_L0_1  },  
    { DEV_I2C_0,  DEV_PAGE_TX_L1_1  },  
    { DEV_I2C_0,  DEV_PAGE_TX_2_1   },  
    { DEV_I2C_0,  DEV_PAGE_TX_3_1   },  
    { DEV_I2C_0,  DEV_PAGE_CBUS_1   },  
    { DEV_DDC_0,  DEV_PAGE_DDC_EDID },  
    { DEV_DDC_0,  DEV_PAGE_DDC_SEGM }   
    }
};
SiiReg_t g_siiRegPageBaseRegs [SII_CRA_DEVICE_PAGE_COUNT] =
{
    TX_PAGE_L0 | 0xFF,     
    TX_PAGE_L0 | 0xFF,     
    TX_PAGE_L0 | 0xFC,     
    TX_PAGE_L0 | 0xFD,     
    TX_PAGE_L0 | 0xFE,     
    TX_PAGE_L0 | 0xFF,     
    TX_PAGE_L0 | 0xFF,     
    TX_PAGE_L0 | 0xFF,     
};
SiiReg_t g_siiRegPageBaseReassign [] =
{
        0xFFFF      
};
