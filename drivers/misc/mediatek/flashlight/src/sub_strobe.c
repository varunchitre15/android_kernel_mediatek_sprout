#define pr_fmt(fmt) " kd_flashlight.c "fmt
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>
#include <linux/version.h>
/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PK_DBG_FUNC(fmt, arg...)    pr_debug("func=%s, "fmt, __func__, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err("func=%s, line=%d, "fmt, __func__, __LINE__, ##arg)





#define PK_DBG PK_DBG_FUNC
#define PK_ERR PK_ERROR
/* ======================================================== */

static int sub_strobe_ioctl(MUINT32 cmd, MUINT32 arg)
{
	PK_DBG("sub dummy ioctl");
	return 0;
}

static int sub_strobe_open(void *pArg)
{
	PK_DBG("sub dummy open");
	return 0;

}

static int sub_strobe_release(void *pArg)
{
	PK_DBG("sub dummy release");
	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL) {
		*pfFunc = &subStrobeFunc;
	}
	return 0;
}
