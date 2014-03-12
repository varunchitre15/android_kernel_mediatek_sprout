#ifndef _STAT_H_
#define _STAT_H_

#include <linux/device.h>

int stat_reg(struct kobject *parent);
void stat_unreg(void);

void stat_start(void);
void stat_stop(void);
void stat_polling(unsigned long long stamp, int cpu);

#endif // _STAT_H_
