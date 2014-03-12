#include <linux/kernel.h>
#include <linux/module.h>

#include "core/met_drv.h"

static const char strTopology[] = "LITTLE:0,1";

extern struct metdevice met_pl310;

static int __init met_plf_init(void)
{
	met_register(&met_pl310);
	met_set_platform("mt8580", 1);
	met_set_topology(strTopology, 1);
	return 0;
}

static void __exit met_plf_exit(void)
{
	met_deregister(&met_pl310);
	met_set_platform(NULL, 0);
	met_set_topology(NULL, 0);
}

module_init(met_plf_init);
module_exit(met_plf_exit);
MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_MT8580");
MODULE_LICENSE("GPL");
