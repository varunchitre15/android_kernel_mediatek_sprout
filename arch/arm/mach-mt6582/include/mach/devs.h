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

#ifndef __DEVS_H__
#define __DEVS_H__

#include <board-custom.h>
#include <mach/board.h>

#define CFG_DEV_UART1
#define CFG_DEV_UART2
#define CFG_DEV_UART3
#define CFG_DEV_UART4

/*
 * Define constants.
 */

#define MTK_UART_SIZE 0x100

/*
 * Define function prototype.
 */

extern int mt_board_init(void);

//extern unsigned int *get_modem_size_list(void);
//extern unsigned int get_nr_modem(void);

#endif  /* !__MT6575_DEVS_H__ */

