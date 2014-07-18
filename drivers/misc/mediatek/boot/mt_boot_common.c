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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot_common.h>

#define MOD "BOOT_COMMON"

/* this vairable will be set by mt_fixup.c */
BOOTMODE g_boot_mode __nosavedata = UNKNOWN_BOOT;
boot_reason_t g_boot_reason __nosavedata = BR_UNKNOWN;

/* return boot reason */
boot_reason_t get_boot_reason(void)
{
    return g_boot_reason;
}

/* set boot reason */
void set_boot_reason (boot_reason_t br)
{
    g_boot_reason = br;
}

/* return boot mode */
BOOTMODE get_boot_mode(void)
{
    return g_boot_mode;
}

/* set boot mode */
void set_boot_mode (BOOTMODE bm)
{
    g_boot_mode = bm;
}

/* for convenience, simply check is meta mode or not */
bool is_meta_mode(void)
{   
    if(g_boot_mode == META_BOOT)
    {   
        return true;
    }
    else
    {   
        return false;
    }
}

bool is_advanced_meta_mode(void)
{
    if (g_boot_mode == ADVMETA_BOOT)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static int boot_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "\n\rMTK BOOT MODE : " );
    switch(g_boot_mode)
    {
        case NORMAL_BOOT :
            seq_printf(m, "NORMAL BOOT\n");
            break;
        case META_BOOT :
            seq_printf(m, "META BOOT\n");
            break;
        case ADVMETA_BOOT :
            seq_printf(m, "Advanced META BOOT\n");
            break;   
        case ATE_FACTORY_BOOT :
            seq_printf(m, "ATE_FACTORY BOOT\n");
            break;
        case ALARM_BOOT :
            seq_printf(m, "ALARM BOOT\n");
            break;
        default :
            seq_printf(m, "UNKNOWN BOOT\n");
            break;
    }  

    return 0;
}

static int boot_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, boot_proc_show, NULL);
}

static const struct file_operations boot_proc_fops = {
    .open   = boot_proc_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= seq_release,
};

static int __init boot_common_init(void)
{
    /* create proc entry at /proc/boot_mode */
    if (NULL == proc_create("boot_mode", 0, NULL, &boot_proc_fops))
        printk("[%s] can't create proc", MOD);

    return 0;
}

static void __exit boot_common_exit(void)
{
    
}

module_init(boot_common_init);
module_exit(boot_common_exit);
MODULE_DESCRIPTION("MTK Boot Information Common Driver");
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(is_meta_mode);
EXPORT_SYMBOL(is_advanced_meta_mode);
EXPORT_SYMBOL(get_boot_mode);
