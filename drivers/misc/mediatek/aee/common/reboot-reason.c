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

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/aee.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>
#include <asm/memory.h>
#include <mach/wd_api.h>
#include <mach/smp.h>
#include "aee-common.h"

#define RR_PROC_NAME "reboot-reason"

/* Some chip do not have reg dump, define a weak to avoid build error */
int __weak mt_reg_dump(char *buf) { return 1; }
EXPORT_SYMBOL(mt_reg_dump);

static struct proc_dir_entry *aee_rr_file;
struct last_reboot_reason aee_rr_last_rec;

#define WDT_NORMAL_BOOT 0
#define WDT_HW_REBOOT 1
#define WDT_SW_REBOOT 2

typedef enum {
    BR_POWER_KEY = 0,
    BR_USB,
    BR_RTC,
    BR_WDT,
    BR_WDT_BY_PASS_PWK,
    BR_TOOL_BY_PASS_PWK,
    BR_2SEC_REBOOT,
    BR_UNKNOWN,
    BR_KE_REBOOT
} boot_reason_t;

char boot_reason[][16]={"keypad","usb_chg","rtc","wdt","reboot","tool reboot","smpl","others","kpanic"};
#define MAX_EXCEPTION_FRAME		16

void aee_rr_last(struct last_reboot_reason *lrr)
{
	memcpy(&aee_rr_last_rec, lrr, sizeof(struct last_reboot_reason));
}

static int aee_rr_reboot_reason_proc_show(struct seq_file *m, void *v)
{
	int i;
	static char reg_buf[1024]; 

	seq_printf(m, "WDT status: %d\n fiq step: %u\n",
		   aee_rr_last_rec.wdt_status, aee_rr_last_rec.fiq_step);
	for (i = 0; i < NR_CPUS; i++) {
	  seq_printf(m, "CPU %d\n"
		     "  irq: enter(%d, %llu) quit(%d, %llu)\n"
		     "  sched: %llu, \"%s\"\n"
		     "  hotplug: %d, %d\n", i,
		     aee_rr_last_rec.last_irq_enter[i], aee_rr_last_rec.jiffies_last_irq_enter[i],
		     aee_rr_last_rec.last_irq_exit[i], aee_rr_last_rec.jiffies_last_irq_exit[i], 
		     aee_rr_last_rec.jiffies_last_sched[i], aee_rr_last_rec.last_sched_comm[i],
		     aee_rr_last_rec.hotplug_data1[i], aee_rr_last_rec.hotplug_data2[i]);
	}
	if (mt_reg_dump(reg_buf) == 0)
	  seq_printf(m, "%s \n", reg_buf);

	return 0;
}

static int aee_rr_reboot_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aee_rr_reboot_reason_proc_show, NULL);
}

static const struct file_operations aee_rr_reboot_reason_proc_fops = {
	.open		= aee_rr_reboot_reason_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


void aee_rr_proc_init(struct proc_dir_entry *aed_proc_dir)
{
	aee_rr_file = proc_create(RR_PROC_NAME, 
				  0444, aed_proc_dir, 
				  &aee_rr_reboot_reason_proc_fops);
	if (aee_rr_file == NULL) {
		printk(KERN_ERR "%s: Can't create rr proc entry\n", __func__);
	}
}
EXPORT_SYMBOL(aee_rr_proc_init);

void aee_rr_proc_done(struct proc_dir_entry *aed_proc_dir)
{
	remove_proc_entry(RR_PROC_NAME, aed_proc_dir);
}
EXPORT_SYMBOL(aee_rr_proc_done);

/* define /sys/bootinfo/powerup_reason */
extern int g_is_panic;
static ssize_t powerup_reason_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
		int g_boot_reason=0;
		char *br_ptr;
		if ((br_ptr = strstr(saved_command_line, "boot_reason=")) != 0) 
		{
			/* get boot reason */
			g_boot_reason = br_ptr[12] - '0';
			printk(KERN_ERR "g_boot_reason=%d ,g_is_panic=%d\n", g_boot_reason,g_is_panic);
			if( (g_boot_reason==BR_WDT_BY_PASS_PWK) && (g_is_panic!=0))
				g_boot_reason=BR_KE_REBOOT;
			
			return sprintf(buf, "%s\n", boot_reason[g_boot_reason]);	
		}
		else
			return 0;
  
}

static struct kobj_attribute powerup_reason_attr = __ATTR_RO(powerup_reason);

struct kobject *bootinfo_kobj;
EXPORT_SYMBOL(bootinfo_kobj);

static struct attribute * bootinfo_attrs[] = {
  &powerup_reason_attr.attr,
  NULL
};

static struct attribute_group bootinfo_attr_group = {
  .attrs = bootinfo_attrs,
};

int ksysfs_bootinfo_init(void)
{
  int error;

  bootinfo_kobj = kobject_create_and_add("bootinfo", NULL);
  if (!bootinfo_kobj) {
    return -ENOMEM;
  }

  error = sysfs_create_group(bootinfo_kobj, &bootinfo_attr_group);
  if (error)
    kobject_put(bootinfo_kobj);

  return error;
}

void ksysfs_bootinfo_exit(void)
{
  kobject_put(bootinfo_kobj);
}

/* end sysfs bootinfo */

static inline unsigned int get_linear_memory_size(void)
{
   return (unsigned long)high_memory - PAGE_OFFSET; 
}

static char nested_panic_buf[1024];
static int aee_nested_printf(const char *fmt, ...)
{
    va_list args;
    static int total_len = 0;
    va_start(args, fmt);
    total_len += vsnprintf(nested_panic_buf, sizeof(nested_panic_buf), fmt, args);
    va_end(args);

    aee_sram_fiq_log(nested_panic_buf);
    
    return total_len;
}

static void print_error_msg(int len)
{
   static char error_msg[][50] = {"Bottom unaligned", "Bottom out of kernel addr", 
								  "Top out of kernel addr", "Buf len not enough"};
   int tmp = (-len) - 1;
   
   aee_sram_fiq_log(error_msg[tmp]);
}

/*save stack as binary into buf, 
 *return value
 
    -1: bottom unaligned
    -2: bottom out of kernel addr space
    -3 top out of kernel addr addr
    -4: buff len not enough
    >0: used length of the buf
 */
int aee_dump_stack_top_binary(char *buf, int buf_len, 
						unsigned long bottom, unsigned long top)
{
	/*should check stack address in kernel range*/
	if (bottom & 3) {
		return -1;
	}
	if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) && 
		(bottom <= (PAGE_OFFSET + get_linear_memory_size())))){
		return -2;
	}

	if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) && 
		(top <= (PAGE_OFFSET + get_linear_memory_size())))){
		return -3;
	}

	if(buf_len < top - bottom) {
		return -4;
	}

	memcpy((void *)buf, (void *)bottom, top - bottom);

	return top - bottom;
}

//extern void mt_fiq_printf(const char *fmt, ...);

static atomic_t nested_panic_time = ATOMIC_INIT(0);
void aee_stop_nested_panic(struct pt_regs *regs)
{
	struct thread_info *thread = current_thread_info();
	int i = 0, len = 0;
	int timeout = 1000000;
	int res = 0, cpu = 0;
	struct wd_api *wd_api = NULL;
	unsigned long high, bottom, fp;
	struct stackframe cur_frame;

	local_irq_disable();
	preempt_disable();

	cpu = get_HW_cpuid();
	aee_nested_printf("\n CPU %d, nested_panic_time=%d\n", cpu, nested_panic_time);

	/*Data abort handler data abort again on many cpu.
	  Since ram console api is lockless, this should be prevented*/
	if (atomic_xchg(&nested_panic_time, 1) != 0) {
		aee_nested_printf("multicore enters nested panic\n");
		aee_nested_printf("[r0-r15,cpsr]%08lx %08lx %08lx %08lx %08lx %08lx %08lx "
				  "%08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx",
				  regs->ARM_r0, regs->ARM_r1, regs->ARM_r2, 
				  regs->ARM_r3, regs->ARM_r4, regs->ARM_r5, 
				  regs->ARM_r6, regs->ARM_r7, regs->ARM_r8, 
				  regs->ARM_r9, regs->ARM_r10,
				  regs->ARM_fp, regs->ARM_ip, regs->ARM_sp,
				  regs->ARM_lr, regs->ARM_pc, regs->ARM_cpsr);
		/* save backtrace addr */
		bottom = regs->ARM_sp;
		high = ALIGN(bottom, THREAD_SIZE);
		cur_frame.lr = regs->ARM_lr;
		cur_frame.fp = regs->ARM_fp;
		aee_nested_printf("\nCurrent backtrace:\n");
		for (i=0; i < MAX_EXCEPTION_FRAME; i++) 
		{
			aee_nested_printf("%08lx, ", cur_frame.lr);
			fp = cur_frame.fp;
			if ((fp < (bottom + 12)) || ((fp + 4) >= high)) {
				goto out;
			}
			cur_frame.fp = *(unsigned long *)(fp - 12);
			cur_frame.lr = *(unsigned long *)(fp - 4);
			if (!((cur_frame.lr >= (PAGE_OFFSET + THREAD_SIZE)) && 
				virt_addr_valid(cur_frame.lr)) ){
				goto out;
			}
		}
		goto out; 
	}

	/*must guarantee Only one cpu can run here*/
	aee_nested_printf("Nested panic\n");
	aee_nested_printf("Previous reg:\n");
	aee_nested_printf("pc: %08lx lr: %08lx psr: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_pc, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_lr, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_cpsr);
	aee_nested_printf("sp: %08lx ip: %08lx fp: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_sp, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_ip, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_fp);
	aee_nested_printf("r10: %08lx r9: %08lx r8: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r10, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r9, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r8);
	aee_nested_printf("r7: %08lx r6: %08lx r5: %08lx r4: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r7, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r6, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r5, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r4);
	aee_nested_printf("r3: %08lx r2: %08lx r1: %08lx r0: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r3, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r2, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r1, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r0);

	aee_nested_printf("Current reg:\n");
	aee_nested_printf("pc: %08lx lr: %08lx psr: %08lx\n",
			regs->ARM_pc, 
			regs->ARM_lr, 
			regs->ARM_cpsr);
	aee_nested_printf("sp: %08lx ip: %08lx fp: %08lx\n",
			regs->ARM_sp, 
			regs->ARM_ip, 
			regs->ARM_fp);
	aee_nested_printf("r10: %08lx r9: %08lx r8: %08lx\n",
			regs->ARM_r10, 
			regs->ARM_r9, 
			regs->ARM_r8);
	aee_nested_printf("r7: %08lx r6: %08lx r5: %08lx r4: %08lx\n",
			regs->ARM_r7, 
			regs->ARM_r6, 
			regs->ARM_r5, 
			regs->ARM_r4);
	len = aee_nested_printf("r3: %08lx r2: %08lx r1: %08lx r0: %08lx\n",
			regs->ARM_r3, 
			regs->ARM_r2, 
			regs->ARM_r1, 
			regs->ARM_r0);

	/*should not print stack info. this may overwhelms ram console used by fiq*/
	if (0!= in_fiq_handler()) {
			goto out;
	}

	aee_nested_printf("Previous stack [%08lx %08lx]\n", 
					((struct pt_regs *)thread->regs_on_excp)->ARM_sp,
					((struct pt_regs *)thread->regs_on_excp)->ARM_sp + 256);

	/*Dump first panic stack*/
	len = aee_dump_stack_top_binary(nested_panic_buf, sizeof(nested_panic_buf), 
                             ((struct pt_regs *)thread->regs_on_excp)->ARM_sp,
                             ((struct pt_regs *)thread->regs_on_excp)->ARM_sp + 256);
	if (len > 0) {
		aee_sram_fiq_save_bin(nested_panic_buf, len);

		/* save backtrace addr */
		bottom = ((struct pt_regs *)thread->regs_on_excp)->ARM_sp;
		high = ALIGN(bottom, THREAD_SIZE);
		cur_frame.lr = ((struct pt_regs *)thread->regs_on_excp)->ARM_lr;
		cur_frame.fp = ((struct pt_regs *)thread->regs_on_excp)->ARM_fp;
		aee_nested_printf("\nPrevious backtrace:\n");
		for (i=0; i < MAX_EXCEPTION_FRAME; i++) 
		{
			aee_nested_printf("%08lx, ", cur_frame.lr);
			fp = cur_frame.fp;
			/* Expand region on purpose for kernel stack overflow case debug */
			if ((fp < (bottom + 12)) || ((fp + 4) >= (high + THREAD_SIZE))) {
				goto cur_stk;
			}
			cur_frame.fp = *(unsigned long *)(fp - 12);
			cur_frame.lr = *(unsigned long *)(fp - 4);
			if (!((cur_frame.lr >= (PAGE_OFFSET + THREAD_SIZE)) && 
				virt_addr_valid(cur_frame.lr)) ){
				goto cur_stk;
			}
		}
    } else {
		print_error_msg(len);
    }

cur_stk:
	aee_nested_printf("\nCurrent stack [%08lx %08lx]\n", regs->ARM_sp, regs->ARM_sp + 256);

	/*Dump second panic stack*/
	len  = aee_dump_stack_top_binary(nested_panic_buf, sizeof(nested_panic_buf), 
								regs->ARM_sp, regs->ARM_sp + 256);

	if (len > 0) {
		aee_sram_fiq_save_bin(nested_panic_buf, len);

		/* save backtrace addr */
		bottom = regs->ARM_sp;
		high = ALIGN(bottom, THREAD_SIZE);
		cur_frame.lr = regs->ARM_lr;
		cur_frame.fp = regs->ARM_fp;
		aee_nested_printf("\nCurrent backtrace:\n");
		for (i=0; i < MAX_EXCEPTION_FRAME; i++) 
		{
			aee_nested_printf("%08lx, ", cur_frame.lr);
			fp = cur_frame.fp;
			if ((fp < (bottom + 12)) || ((fp + 4) >= high)) {
				goto out;
			}
			cur_frame.fp = *(unsigned long *)(fp - 12);
			cur_frame.lr = *(unsigned long *)(fp - 4);
			if (!((cur_frame.lr >= (PAGE_OFFSET + THREAD_SIZE)) && 
				virt_addr_valid(cur_frame.lr)) ){
				goto out;
			}
		}
	} else {
		print_error_msg(len);
	}

	res = get_wd_api(&wd_api);
	if (res) {
		aee_nested_printf("aee_stop_nested_panic, get wd api error\n");
	} else {
		wd_api->wd_restart(WD_TYPE_NOLOCK);
		wd_api->wd_aee_confirm_hwreboot();
	}
out:
	/* waiting for the WDT timeout */
	while (1)
	{
		// output to UART directly to avoid printk nested panic
		//mt_fiq_printf("%s hang here%d\t", __func__, i++);
		while(timeout--)
		{
		    udelay(1);
		}
		timeout = 1000000;
	}

}






