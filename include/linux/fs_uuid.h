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

#include <linux/device.h>

struct hd_struct;
struct block_device;

struct fs_info {
	char uuid[16];
	dev_t dev_t;
	char *last_mount;
	int last_mount_size;
};

int part_matches_fs_info(struct hd_struct *part, struct fs_info *seek);
dev_t blk_lookup_fs_info(struct fs_info *seek);
struct fs_info *fs_info_from_block_dev(struct block_device *bdev);
void free_fs_info(struct fs_info *fs_info);
int bdev_matches_key(struct block_device *bdev, const char *key);
struct block_device *next_bdev_of_type(struct block_device *last,
	const char *key);
