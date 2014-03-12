#include <linux/types.h>
#include <cust_acc.h>
#include <mach/mt_pm_ldo.h>
#define AGOLD_ACC_KXTIK1004_DIR_VAULE 5
/*---------------------------------------------------------------------------*/
#if defined(MTK_AUTO_DETECT_ACCELEROMETER)
static struct acc_hw kxtik1004_cust_acc_hw = {
    .i2c_num = 2,
#if defined(AGOLD_ACC_KXTIK1004_DIR_VAULE)
    .direction = AGOLD_ACC_KXTIK1004_DIR_VAULE,
#else
    .direction = 0,
#endif
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 6, //old value 16                  /*!< don't enable low pass fileter */
};
/*---------------------------------------------------------------------------*/
struct acc_hw* kxtik1004_get_cust_acc_hw(void)
{
    return &kxtik1004_cust_acc_hw;
}
#else
static struct acc_hw cust_acc_hw = {
    .i2c_num = 2,
#if defined(AGOLD_ACC_KXTIK1004_DIR_VAULE) // fix me
#warning
    .direction = AGOLD_ACC_KXTIK1004_DIR_VAULE,
#else
    .direction = 0,
#endif
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 6, //old value 16                  /*!< don't enable low pass fileter */
};
/*---------------------------------------------------------------------------*/
struct acc_hw* get_cust_acc_hw(void) 
{
    return &cust_acc_hw;
}
#endif
