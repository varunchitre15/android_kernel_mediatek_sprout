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

#ifndef __MT_SMP_H
#define __MT_SMP_H

#include <linux/cpumask.h>
#include <mach/irqs.h>
extern void irq_raise_softirq(const struct cpumask *mask, unsigned int irq);

/* use Soft IRQ1 as the IPI */
static inline void smp_cross_call(const struct cpumask *mask)
{
    irq_raise_softirq(mask, CPU_BRINGUP_SGI);
}

static inline int get_HW_cpuid(void)
{
    int id;
    asm ("mrc     p15, 0, %0, c0, c0, 5 @ Get CPUID\n"
            : "=r" (id));
    return (id&0x3)+((id&0xF00)>>6);
}

#endif  /* !__MT_SMP_H */
