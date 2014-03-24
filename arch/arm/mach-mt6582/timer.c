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

#include <asm/mach/time.h>
#include <mach/mt_timer.h>

extern struct mt_clock mt6582_gpt;
extern int generic_timer_register(void);


struct mt_clock *mt6582_clocks[] =
{
    &mt6582_gpt,
};

static void __init mt6582_timer_init(void)
{
    int i;
    struct mt_clock *clock;
    int err;

    for (i = 0; i < ARRAY_SIZE(mt6582_clocks); i++) {
        clock = mt6582_clocks[i];

        clock->init_func();

        if (clock->clocksource.name) {
            err = clocksource_register(&(clock->clocksource));
            if (err) {
                pr_err("mt6582_timer_init: clocksource_register failed for %s\n", clock->clocksource.name);
            }
        }

        err = setup_irq(clock->irq.irq, &(clock->irq));
        if (err) {
            pr_err("mt6582_timer_init: setup_irq failed for %s\n", clock->irq.name);
        }

        if (clock->clockevent.name)
            clockevents_register_device(&(clock->clockevent));
    }

#ifndef CONFIG_MT6582_FPGA
    err = generic_timer_register(); 
    if (err) {
        pr_err("generic_timer_register failed, err=%d\n", err);
    }
  // printk("fwq no generic timer");
#endif
}


struct sys_timer mt6582_timer = {
    .init = mt6582_timer_init,
};
