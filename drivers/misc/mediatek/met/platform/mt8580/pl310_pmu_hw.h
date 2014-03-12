#ifndef _PL310_PMU_HW_H_
#define _PL310_PMU_HW_H_

#define REG2_EV_COUNTER_CTRL	0x200
#define REG2_EV_COUNTER1_CFG	0x204
#define REG2_EV_COUNTER0_CFG	0x208
#define REG2_EV_COUNTER1	0x20c
#define REG2_EV_COUNTER0	0x210

#define REG2_INT_MASK		0x214
#define REG2_INT_MASK_STATUS	0x218
#define REG2_INT_RAW_STATUS	0x21c
#define REG2_INT_CLEAR		0x220

void pl310_pmu_hw_start(struct met_pmu *pmu, int count, int toggle);
void pl310_pmu_hw_stop(int count);
unsigned int pl310_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value);

#endif // _PL310_PMU_HW_H_
