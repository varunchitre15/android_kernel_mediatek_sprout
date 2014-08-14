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

#include <linux/types.h>
#include <linux/genhd.h> 
//#include <linux/proc_fs.h>
//#include <linux/seq_file.h>
//#include <asm/uaccess.h>

//#include <linux/mmc/sd_misc.h>

//#include "efi.h"


struct hd_struct *get_part(char *name)
{
    dev_t devt;
    int partno;
    struct disk_part_iter piter;
    struct gendisk *disk;
    struct hd_struct *part = NULL; 
    
    if (!name)
        return part;

    devt = blk_lookup_devt("mmcblk0", 0);
    disk = get_gendisk(devt, &partno);

    if (!disk || get_capacity(disk) == 0)
        return 0;

	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter))) {
        if (part->info && !strcmp(part->info->volname, name)) {
            get_device(part_to_dev(part));
            break;
        }
	}
	disk_part_iter_exit(&piter);
    
    return part;
}
EXPORT_SYMBOL(get_part);


void put_part(struct hd_struct *part)
{
    disk_put_part(part);
}
EXPORT_SYMBOL(put_part);
