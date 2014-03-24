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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_fs.c
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   MT6516 CCCI RPC
 *
 * Author:
 * -------
 *   
 *
 ****************************************************************************/

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <ccci.h>

/*********************************************************************************
 * RPC Daemon section
 *********************************************************************************/

#if defined(CONFIG_MTK_TC1_FEATURE)
DECLARE_WAIT_QUEUE_HEAD(rpc_daemon_waitq);
DECLARE_WAIT_QUEUE_HEAD(rpc2_daemon_waitq);
static struct kfifo     rpc_daemon_fifo[2];
typedef struct _rpc_stream_msg_t
{
	unsigned length;
	unsigned index;
}rpc_stream_msg_t;
static dev_t		rpc_dev_num[2];
static struct cdev	rpc_cdev[2];
static void rpc_daemon_notify(int md_id, unsigned int buff_index);

#define CCCI_RPC_IOC_MAGIC		'R'
#define CCCI_RPC_IOCTL_GET_INDEX	_IO(CCCI_RPC_IOC_MAGIC, 1)
#define CCCI_RPC_IOCTL_SEND		_IOR(CCCI_RPC_IOC_MAGIC, 2, unsigned int)
//--------------------------------------------------------------------------------
#endif

typedef struct test
{
	int op;
	char buf[0];
}test_t;

typedef struct _rpc_ctl_block
{
	spinlock_t			rpc_fifo_lock;
	RPC_BUF				*rpc_buf_vir;
	unsigned int		rpc_buf_phy;
	unsigned int		rpc_buf_len;
	struct kfifo		rpc_fifo;
	struct work_struct	rpc_work;
	int					m_md_id;
	int					rpc_smem_instance_size;
	int					rpc_max_buf_size;
	int					rpc_ch_num;
}rpc_ctl_block_t;

static rpc_ctl_block_t	*rpc_ctl_block[MAX_MD_NUM];

extern void ccci_rpc_work_helper(int md_id, int *p_pkt_num, RPC_PKT pkt[], RPC_BUF *p_rpc_buf, unsigned int tmp_data[]);

static int get_pkt_info(int md_id, unsigned int* pktnum, RPC_PKT* pkt_info, char* pdata)
{
	unsigned int	pkt_num = *((unsigned int*)pdata);
	unsigned int	idx = 0;
	unsigned int	i = 0;
	rpc_ctl_block_t	*ctl_b = rpc_ctl_block[md_id];

	CCCI_RPC_MSG(md_id, "package number = 0x%08X\n", pkt_num);

	if(pkt_num > IPC_RPC_MAX_ARG_NUM)
		return -1;

	idx = sizeof(unsigned int);
	for(i = 0; i < pkt_num; i++)
	{
		pkt_info[i].len = *((unsigned int*)(pdata + idx));
		idx += sizeof(unsigned int);
		pkt_info[i].buf = (pdata + idx);

		CCCI_RPC_MSG(md_id, "pak[%d]: vir = 0x%08X, len = 0x%08X\n", i, \
								(unsigned int)pkt_info[i].buf, pkt_info[i].len);

		// 4 byte alignment
		idx += ((pkt_info[i].len+3)>>2)<<2;
	}

	if(idx > ctl_b->rpc_max_buf_size)
	{
		CCCI_MSG_INF(md_id, "rpc", "over flow, pdata = %p, idx = 0x%08X, max = %p\n", 
								pdata, idx, pdata + ctl_b->rpc_max_buf_size);
		return -1;
	}
	*pktnum = pkt_num;

	return 0;
}


static int rpc_write(int md_id, int buf_idx, RPC_PKT* pkt_src, unsigned int pkt_num)
{
	int				ret = 0;
	ccci_msg_t		msg;
	RPC_BUF			*rpc_buf_tmp = NULL;
	unsigned char	*pdata = NULL;
	unsigned int 	data_len = 0;
	unsigned int 	i = 0;
	unsigned int	AlignLength = 0;
	rpc_ctl_block_t	*ctl_b = rpc_ctl_block[md_id];

	//rpc_buf_tmp = ctl_b->rpc_buf_vir + buf_idx;
	rpc_buf_tmp = (RPC_BUF*)((unsigned int)(ctl_b->rpc_buf_vir) + ctl_b->rpc_smem_instance_size*buf_idx);
	rpc_buf_tmp->op_id = IPC_RPC_API_RESP_ID | rpc_buf_tmp->op_id;
	pdata = rpc_buf_tmp->buf;
	*((unsigned int*)pdata) = pkt_num;

	pdata += sizeof(unsigned int);
	data_len += sizeof(unsigned int);

	for(i = 0; i < pkt_num; i++)
	{
		if((data_len + 2*sizeof(unsigned int) + pkt_src[i].len) > ctl_b->rpc_max_buf_size)
		{
			CCCI_MSG_INF(md_id, "rpc", "Stream buffer full!!\n");
			ret = -CCCI_ERR_LARGE_THAN_BUF_SIZE;
			goto _Exit;
		}

		*((unsigned int*)pdata) = pkt_src[i].len;
		pdata += sizeof(unsigned int);
		data_len += sizeof(unsigned int);

		// 4  byte aligned
		AlignLength = ((pkt_src[i].len + 3) >> 2) << 2;
		data_len += AlignLength;

		if(pdata != pkt_src[i].buf)
			memcpy(pdata, pkt_src[i].buf, pkt_src[i].len);
		else
			CCCI_RPC_MSG(md_id, "same addr, no copy\n");

		pdata += AlignLength;
	}

	//msg.data0  = ctl_b->rpc_buf_phy + (sizeof(RPC_BUF) * buf_idx);
	msg.data0 = (unsigned int)(ctl_b->rpc_buf_phy)-md_2_ap_phy_addr_offset_fixed + ctl_b->rpc_smem_instance_size*buf_idx;
	msg.data1  = data_len + 4;
	msg.reserved = buf_idx;
	msg.channel = CCCI_RPC_TX;

	CCCI_RPC_MSG(md_id, "Write, %08X, %08X, %08X, %08X\n", 
			msg.data0, msg.data1, msg.channel, msg.reserved);

	mb();
	ret = ccci_message_send(md_id, &msg, 1);
	if(ret != sizeof(ccci_msg_t))
	{
		CCCI_MSG_INF(md_id, "rpc", "fail send msg <%d>!!!\n", ret);
		return ret;
	} else 
		ret = 0;

_Exit:
	return ret;
}

static void ccci_rpc_work(struct work_struct *work)
{
	int				pkt_num = 0;
	int				ret_val = 0;
	unsigned int	buf_idx = 0;
	RPC_PKT			pkt[IPC_RPC_MAX_ARG_NUM] = { {0}, };
	RPC_BUF			*rpc_buf_tmp = NULL;
	unsigned int	tmp_data[4];
	rpc_ctl_block_t	*ctl_b = container_of(work, rpc_ctl_block_t, rpc_work);
	int				md_id= ctl_b->m_md_id;

	CCCI_RPC_MSG(md_id, "ccci_rpc_work++\n");

	if(ctl_b->rpc_buf_vir == NULL)
	{
		CCCI_MSG_INF(md_id, "rpc", "invalid rpc_buf_vir!!\n");
		return;
	}

	while(kfifo_out(&ctl_b->rpc_fifo, &buf_idx, sizeof(unsigned int)))
	{
		if(buf_idx < 0 || buf_idx > ctl_b->rpc_ch_num)
		{
			CCCI_MSG_INF(md_id, "rpc", "invalid idx %d\n", buf_idx);
			ret_val = FS_PARAM_ERROR;  // !!!!! Make more meaningful
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}

		pkt_num = 0;
		memset(pkt, 0x00, sizeof(RPC_PKT)*IPC_RPC_MAX_ARG_NUM);

		//rpc_buf_tmp = ctl_b->rpc_buf_vir + buf_idx;
		rpc_buf_tmp = (RPC_BUF*)((unsigned int)(ctl_b->rpc_buf_vir) + ctl_b->rpc_smem_instance_size*buf_idx);

		if(get_pkt_info(md_id, &pkt_num, pkt, rpc_buf_tmp->buf) < 0)
		{
			CCCI_MSG_INF(md_id, "rpc", "Fail to get packet info\n");
			ret_val = FS_PARAM_ERROR;  // !!!!! Make more meaningful
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}

#if defined(CONFIG_MTK_TC1_FEATURE)
		#define RPC_CCCI_TC1_CMD  	0x00003000

		if((rpc_buf_tmp->op_id & 0x0000F000) == RPC_CCCI_TC1_CMD)
		{
			rpc_daemon_notify(md_id, buf_idx);
			return;
		}
		else
			ccci_rpc_work_helper(md_id, &pkt_num, pkt, rpc_buf_tmp, tmp_data);  // !!!!! Make more meaningful
#else
		ccci_rpc_work_helper(md_id, &pkt_num, pkt, rpc_buf_tmp, tmp_data);  // !!!!! Make more meaningful
#endif
		    
			
_Next:
		if(rpc_write(md_id, buf_idx, pkt, pkt_num) != 0)
		{
			CCCI_MSG_INF(md_id, "rpc", "fail to write packet!!\r\n");
			return ;
		}
	} 

	CCCI_RPC_MSG(md_id, "ccci_rpc_work--\n");    
}


static void ccci_rpc_callback(void *private)
{
	logic_channel_info_t	*ch_info = (logic_channel_info_t*)private;
	ccci_msg_t				msg;
	rpc_ctl_block_t			*ctl_b = (rpc_ctl_block_t *)ch_info->m_owner;
	int						md_id = ctl_b->m_md_id;

	while(get_logic_ch_data(ch_info, &msg)){
		CCCI_RPC_MSG(md_id, "callback, %08X, %08X, %08X, %08X\n", 
							msg.data0, msg.data1, msg.channel, msg.reserved);
		spin_lock_bh(&ctl_b->rpc_fifo_lock);
		kfifo_in(&ctl_b->rpc_fifo, &msg.reserved, sizeof(unsigned int));
		spin_unlock_bh(&ctl_b->rpc_fifo_lock);
	}

	schedule_work(&ctl_b->rpc_work);
	CCCI_RPC_MSG(md_id, "callback --\n");    
}

#if defined(CONFIG_MTK_TC1_FEATURE)
void rpc_daemon_notify(int md_id, unsigned int buff_index)
{        
	if(md_id == 0){
        	kfifo_in(&rpc_daemon_fifo[md_id], &buff_index, sizeof(unsigned int));
		wake_up_interruptible(&rpc_daemon_waitq);
	}
	else if(md_id == 1){
        	kfifo_in(&rpc_daemon_fifo[md_id], &buff_index, sizeof(unsigned int));
		wake_up_interruptible(&rpc2_daemon_waitq);
	}
}
static int rpc_get_share_mem_index(struct file *file)
{
	int ret;
	unsigned long flag;
	int md_id;
	rpc_ctl_block_t	*ctl_b = (rpc_ctl_block_t *)file->private_data;

	CCCI_RPC_MSG(0, "get index start\n");

        md_id = ctl_b->m_md_id;

	if(md_id == 0){
		if (wait_event_interruptible(rpc_daemon_waitq, kfifo_len(&rpc_daemon_fifo[md_id]) != 0) != 0)
		{
			return -ERESTARTSYS;
		}
	}
	else if(md_id == 1){
		if (wait_event_interruptible(rpc2_daemon_waitq, kfifo_len(&rpc_daemon_fifo[md_id]) != 0) != 0)
		{
			return -ERESTARTSYS;
		}
	}
        else
		return -EFAULT;

	if (kfifo_out(&rpc_daemon_fifo[md_id], (unsigned int *) &ret, sizeof(int)) != sizeof(int)){
		CCCI_RPC_MSG(0, "Unable to get new request from fifo\n");
		return -EFAULT;
	}
    
	CCCI_RPC_MSG(0, "get index end\n");

	return ret;
}

static int rpc_daemon_send_helper(struct file *file, unsigned long arg)
{
	void __user		*argp;
        ccci_msg_t              msg;
	rpc_stream_msg_t	message;
	int			ret = 0;
        rpc_ctl_block_t		*ctl_b = (rpc_ctl_block_t *)file->private_data; //rpc_ctl_block[0];

	argp = (void __user *) arg;
	if (copy_from_user((void *) &message, argp, sizeof(rpc_stream_msg_t)))
	{
		return -EFAULT;
	}
       
    msg.data0 = (unsigned int)(ctl_b->rpc_buf_phy)-md_2_ap_phy_addr_offset_fixed + sizeof(RPC_BUF)*message.index;
	msg.data1  = message.length + 4;
	msg.reserved = message.index;
	msg.channel = CCCI_RPC_TX;

	CCCI_RPC_MSG(0, "Write, %08X, %08X, %08X, %08X\n", 
			msg.data0, msg.data1, msg.channel, msg.reserved);

	mb();
	ret = ccci_message_send(0, &msg, 1);
	if(ret != sizeof(ccci_msg_t))
	{
		CCCI_MSG_INF(0, "rpc", "fail send msg <%d>!!!\n", ret);
		return ret;
	}

	return 0;
}

static int rpc_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long off, start, len;
	rpc_ctl_block_t	*ctl_b = (rpc_ctl_block_t *)file->private_data; //rpc_ctl_block[0];

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)){
		return -EINVAL;
	}


	off   = vma->vm_pgoff << PAGE_SHIFT;
	start = (unsigned long) ctl_b->rpc_buf_phy;
	len   = PAGE_ALIGN((start & ~PAGE_MASK) + ctl_b->rpc_smem_instance_size);

	if ((vma->vm_end - vma->vm_start + off) > len)
	{
		return -EINVAL;
	}

	off += start & PAGE_MASK;
	vma->vm_pgoff  = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static int rpc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	
	switch(cmd)
	{
	case CCCI_RPC_IOCTL_GET_INDEX:
		ret = rpc_get_share_mem_index(file);
		break;

	case CCCI_RPC_IOCTL_SEND:
		ret = rpc_daemon_send_helper(file, arg);
		break;

	default:
		ret = -ENOIOCTLCMD;
		break;
	}
 
	return ret;
}

static int rpc_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);
	int major = imajor(inode);
	int md_id;
        
	md_id = get_md_id_by_dev_major(major);
	file->private_data = (void*)rpc_ctl_block[md_id];

	return 0;
}

static int rpc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations rpc_fops = 
{
	.owner   	= THIS_MODULE,
	.unlocked_ioctl	= rpc_ioctl,
	.open		= rpc_open,
	.mmap		= rpc_mmap,
	.release	= rpc_release,
};

int rpc_device_init(int md_id)
{
	int  ret;
        int  major, minor;
        char name[16];

	printk("ccci_rpc (%d): rpc_device_init++\n", md_id);

	ret=kfifo_alloc(&rpc_daemon_fifo[md_id], sizeof(unsigned int)*8, GFP_KERNEL);
	if (ret)
	{
		printk("ccci_rpc: Unable to create daemon fifo\n");
		return ret;
	}

        ret = get_dev_id_by_md_id(md_id, "rpc", &major, &minor);
	if (ret < 0) {
		CCCI_MSG_INF(md_id, "rpc", "get rpc dev id fail: %d\n", ret);
		return ret;
	}
        
        if(md_id)
	    snprintf(name, 16, "ccci%d_rpc", md_id+1);
  	else 
	    strcpy(name, "ccci_rpc");

	rpc_dev_num[md_id] = MKDEV(major, minor);	// Using FS major, sub id is 1, not 0
	ret		   = register_chrdev_region(rpc_dev_num[md_id], 1, name);
    
	if (ret){
		printk("ccci_rpc: Register character device failed\n");
		return ret;
	}
	
	cdev_init(&rpc_cdev[md_id], &rpc_fops);
	rpc_cdev[md_id].owner = THIS_MODULE;
	rpc_cdev[md_id].ops   = &rpc_fops;

	ret = cdev_add(&rpc_cdev[md_id], rpc_dev_num[md_id], 1);
	if (ret){
		printk("ccci_rpc: Char device add failed\n");
		unregister_chrdev_region(rpc_dev_num[md_id], 1);
		return ret;
	}

	printk("ccci_rpc (%d): rpc_device_init--\n", md_id);

	return 0;
}

 void rpc_device_deinit(int md_id)
{
	kfifo_free(&rpc_daemon_fifo[md_id]);
	cdev_del(&rpc_cdev[md_id]);
	unregister_chrdev_region(rpc_dev_num[md_id], 1);
}
//-------------------------------------------------------------------------------------
#endif

int __init ccci_rpc_init(int md_id)
{ 
	int 				ret;
	rpc_ctl_block_t		*ctl_b;
	rpc_cfg_inf_t		rpc_cfg;
	int					rpc_buf_vir, rpc_buf_phy, rpc_buf_len;

	// Allocate fs ctrl struct memory
	ctl_b = (rpc_ctl_block_t *)kmalloc(sizeof(rpc_ctl_block_t), GFP_KERNEL);
	if(ctl_b == NULL)
		return -CCCI_ERR_GET_MEM_FAIL;
	memset(ctl_b, 0, sizeof(rpc_ctl_block_t));
	rpc_ctl_block[md_id] = ctl_b;

	// Get rpc config information
	ASSERT(ccci_get_sub_module_cfg(md_id, "rpc", (char*)&rpc_cfg, sizeof(rpc_cfg_inf_t)) \
				== sizeof(rpc_cfg_inf_t) );

	ASSERT(ccci_rpc_base_req(md_id, &rpc_buf_vir, &rpc_buf_phy, &rpc_buf_len) == 0);
	ctl_b->rpc_buf_vir = (RPC_BUF*)rpc_buf_vir;
	ctl_b->rpc_buf_phy = (unsigned int)rpc_buf_phy;
	ctl_b->rpc_buf_len = rpc_buf_len;
	ctl_b->rpc_max_buf_size = rpc_cfg.rpc_max_buf_size;
	ctl_b->rpc_ch_num = rpc_cfg.rpc_ch_num;
	ctl_b->rpc_smem_instance_size = sizeof(RPC_BUF) + ctl_b->rpc_max_buf_size;
	//Note!!!!! we should check cofigure mistake

	// Init ctl_b
	ctl_b->m_md_id = md_id;
	spin_lock_init(&ctl_b->rpc_fifo_lock);


	ret=kfifo_alloc(&ctl_b->rpc_fifo,sizeof(unsigned) * ctl_b->rpc_ch_num, GFP_KERNEL);

	if (ret<0)
	{
		CCCI_MSG_INF(md_id, "rpc", "Unable to create fifo\n");
		goto _KFIFO_ALLOC_FAIL;
	}

	INIT_WORK(&ctl_b->rpc_work, ccci_rpc_work);

	// modem related channel registration.
	CCCI_RPC_MSG(md_id, "rpc_buf_vir=0x%p, rpc_buf_phy=0x%08X, rpc_buf_len=0x%08X\n", \
								ctl_b->rpc_buf_vir, ctl_b->rpc_buf_phy, ctl_b->rpc_buf_len);
	ASSERT(ctl_b->rpc_buf_vir != NULL);
	ASSERT(ctl_b->rpc_buf_len != 0);
	ASSERT(register_to_logic_ch(md_id, CCCI_RPC_RX, ccci_rpc_callback, ctl_b) == 0);

#if defined(CONFIG_MTK_TC1_FEATURE)
	ASSERT(register_to_logic_ch(md_id, CCCI_RPC_TX, ccci_rpc_callback, ctl_b) == 0);

	ret = rpc_device_init(md_id);
	if(0 != ret)
        goto _KFIFO_ALLOC_FAIL;
#endif
	
	return 0;

_KFIFO_ALLOC_FAIL:
	kfree(ctl_b);
	rpc_ctl_block[md_id] = NULL;

	return ret;
}

void __exit ccci_rpc_exit(int md_id)
{
	rpc_ctl_block_t		*ctl_b;

	CCCI_RPC_MSG(md_id, "ccci_rpc_exit\n");
	ctl_b = rpc_ctl_block[md_id];
	if (ctl_b == NULL)
		return;
	
#if defined(CONFIG_MTK_TC1_FEATURE)
	rpc_device_deinit(md_id);
#endif
	kfifo_free(&ctl_b->rpc_fifo);
	un_register_to_logic_ch(md_id, CCCI_RPC_RX);
#if defined(CONFIG_MTK_TC1_FEATURE)
	un_register_to_logic_ch(md_id, CCCI_RPC_TX);
#endif
	kfree(ctl_b);
	rpc_ctl_block[md_id] = NULL;
}


