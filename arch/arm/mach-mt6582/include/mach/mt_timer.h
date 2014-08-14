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

#ifndef __MT_TIMER_H__
#define __MT_TIMER_H__

#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

/*
 * Define data structures.
 */

typedef void (*clock_init_func)(void);

struct mt_clock
{
    struct clock_event_device clockevent;
    struct clocksource clocksource;
    struct irqaction irq;
    clock_init_func init_func;
};

#endif  /* !__MT_TIMER_H__ */

