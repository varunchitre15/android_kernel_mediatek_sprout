#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#if 0
/* declare function prototype */
#endif


void evdo_host_wakeup(void)
{
#if 0
	/* call correct wake up func here */
#endif
}

static char myCmdBuf[10];
static ssize_t EVDO_HOST_RESET_Write_Proc(struct file *file, const char *buf, unsigned long len, void *data)
{
	int ret = copy_from_user(myCmdBuf, buf, len);
	if (ret < 0)
		return -1;

	myCmdBuf[len] = '\0';
	printk(KERN_WARNING "EVDO_HOST_RESET_Write_Proc(), myCmdBuf : %s\n", myCmdBuf);

#if 0		
	/* call correct IP reset  func here */
#endif

	return -2;  
}


static int EVDO_HOST_RESET_Read_Proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = buf;
	int len = 0;

	if (off != 0)
		return 0;

	printk(KERN_WARNING "EVDO_HOST_RESET_Read_Proc\n");

	p += sprintf(p, "%d\n",  1);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
}


static int __init mtk_evdo_host_interface_init(void)
{

	struct proc_dir_entry *prEntry;
	printk(KERN_WARNING "mtk_evdo_host_interface_init\n");

	prEntry = create_proc_entry("EVDO_HOST_RESET", 0660, 0);
	if (prEntry)
	{
		prEntry->read_proc = EVDO_HOST_RESET_Read_Proc;
		prEntry->write_proc = EVDO_HOST_RESET_Write_Proc;
		printk(KERN_WARNING "add /proc/EVDO_HOST_RESET ok\n");
	}
	else
	{
		printk(KERN_WARNING "add /proc/EVDO_HOST_RESET fail\n");
	}

	return 0;
}

static void __exit mtk_evdo_host_interface_exit(void)
{
	printk(KERN_WARNING "mtk_evdo_host_interface_exit\n");

}

module_init(mtk_evdo_host_interface_init);
module_exit(mtk_evdo_host_interface_exit);

