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

#ifndef SECMOD_H
#define SECMOD_H

#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>

struct sec_ops {
    int (*sec_get_rid)(unsigned int *rid);
};

struct sec_mod {
    dev_t                 id;
    int                   init;
    spinlock_t            lock;
    const struct sec_ops *ops;
};

#endif /* end of SECMOD_H */
