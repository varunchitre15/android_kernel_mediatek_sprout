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

#include <linux/init.h>
#include <linux/delay.h>
#include "mach/devs.h"
#include "mach/pm_common.h"



/*
 * board_init: entry point for board initialization.
 * Always return 1.
 */
static __init int board_init(void)
{
    /*
     * NoteXXX: There are two board init related functions.
     *          One is mt65xx_board_init() and another is custom_board_init().
     *
     *          mt65xx_board_init() is used for chip-dependent code.
     *          It is suggested to put driver code in this function to do:
     *          1). Capability structure of platform devices.
     *          2). Define platform devices with their resources.
     *          3). Register MT65XX platform devices.
     *
     *          custom_board_init() is used for customizable code.
     *          It is suggested to put driver code in this function to do:
     *          1). Initialize board (PINMUX, GPIOs).
     *          2). Perform board specific initialization:
     *              1-1). Register MT65xx platform devices.
     *              1-2). Register non-MT65xx platform devices.
     *                    (e.g. external peripheral GPS, BT, ¡K etc.)
     */
    // mt6582_power_management_init();

    mt_board_init();

    /* init low power PMIC setting after PMIC is ready */
    // mt6582_pmic_low_power_init();

    /* BUG BUG: temporarily marked since customization is not ready yet */
# if 0
    custom_board_init();
#endif

    return 0;
}

late_initcall(board_init);
