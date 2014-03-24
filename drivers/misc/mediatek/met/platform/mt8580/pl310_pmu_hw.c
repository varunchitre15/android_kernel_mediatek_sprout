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

#include <asm/system.h>
#include <linux/io.h>

#include "pl310_pmu.h"
#include "pl310_pmu_hw.h"

#define COUNTER_RESET_0		(1 << 1)
#define COUNTER_RESET_1		(1 << 2)
#define COUNTER_ENABLE		(1 << 0)

static int toggle_mode = 0;
static void __iomem *pl310_iobase = (void __iomem *) 0xfe002000; // mt8580

static inline unsigned int pl310_reg_read(unsigned int reg_offset)
{
	unsigned int value = readl(pl310_iobase + reg_offset);
	mb();
	return value;
}

static inline void pl310_reg_write(unsigned int reg_offset, unsigned int value)
{
	writel(value, pl310_iobase + reg_offset);
	// make sure writel() be completed before outer_sync()
	mb();
	outer_sync();
	// make sure outer_sync() be completed before following readl();
	mb();
}

static inline void pl310_pmu_type_select(unsigned int idx, unsigned int type)
{
	if (idx == 0) {
		pl310_reg_write(REG2_EV_COUNTER0_CFG, type << 2);
	} else { // idx == 1
		pl310_reg_write(REG2_EV_COUNTER1_CFG, type << 2);
	}
}

static inline unsigned int pl310_pmu_read_count(unsigned int idx)
{
	if (idx == 0) {
		return pl310_reg_read(REG2_EV_COUNTER0);
	} else {
		return pl310_reg_read(REG2_EV_COUNTER1);
	}
}

static void pl310_pmu_hw_reset_all(void)
{
	pl310_reg_write(REG2_EV_COUNTER_CTRL, COUNTER_RESET_1 | COUNTER_RESET_0);
	// disable counter
	pl310_pmu_type_select(0, 0);
	pl310_pmu_type_select(1, 0);
	// do not generate interrupts
	pl310_reg_write(REG2_EV_COUNTER0_CFG, 0);
	pl310_reg_write(REG2_EV_COUNTER1_CFG, 0);
	// clear values
	pl310_reg_write(REG2_EV_COUNTER0, 0);
	pl310_reg_write(REG2_EV_COUNTER1, 0);
	// clear overflow status
	pl310_reg_write(REG2_INT_CLEAR, 0x1);
}

void pl310_pmu_hw_start(struct met_pmu *pmu, int count, int toggle)
{
	int i;

	toggle_mode = toggle;
	if (toggle_mode == 1) {
		/*
		 * 0x2: DRHIT
		 * 0x3: DRREQ
		 * 0x4: DWHIT
		 * 0x5: DWREQ
		 */
		pmu[0].mode = MODE_POLLING;
		pmu[0].event = 0x2; // DRHIT
		pmu[1].mode = MODE_POLLING;
		pmu[1].event = 0x3; // DRREQ
		count = 2;
	}

	pl310_pmu_hw_reset_all();
	for (i=0; i<count; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pl310_pmu_type_select(i, pmu[i].event);
		}
	}
	pl310_reg_write(REG2_EV_COUNTER_CTRL, COUNTER_ENABLE);
}

void pl310_pmu_hw_stop(int count)
{
	pl310_reg_write(REG2_EV_COUNTER_CTRL, COUNTER_RESET_1 | COUNTER_RESET_0);
}

unsigned int pl310_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value)
{
	int i, cnt=0;

	for (i=0; i<count; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pmu_value[i] = pl310_pmu_read_count(i);
			cnt++;
		}
	}
	pl310_reg_write(REG2_EV_COUNTER_CTRL, COUNTER_RESET_1 | COUNTER_RESET_0);

	if (toggle_mode == 1) {
		pl310_reg_write(REG2_EV_COUNTER0_CFG, (6 - ((pl310_reg_read(REG2_EV_COUNTER0_CFG) >> 2) & 0xf)) << 2);
		/*
		 * 0x3: DRREQ
		 * 0x5: DWREQ
		 */
		cnt = (pl310_reg_read(REG2_EV_COUNTER1_CFG) >> 2) & 0xf;
		pl310_reg_write(REG2_EV_COUNTER1_CFG, (8 - cnt) << 2);
	}

	pl310_reg_write(REG2_EV_COUNTER_CTRL, COUNTER_ENABLE);

	return cnt;
}
