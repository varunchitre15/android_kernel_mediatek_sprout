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

#include <linux/mutex.h>
#include <linux/semaphore.h>

/*mutex*/
extern struct mutex * mt_mutex_init(void);
extern void mt_mutex_lock_nested(struct mutex *lock, unsigned int subclass);
extern void mt_mutex_lock_nest_lock(struct mutex *lock, struct rw_semaphore *nest_lock);
extern int  mt_mutex_lock_interruptible_nested(struct mutex *lock,unsigned int subclass);

extern int mt_mutex_lock_killable_nested(struct mutex *lock,unsigned int subclass);
extern void mt_mutex_lock(struct mutex *lock);
extern int mt_mutex_lock_interruptible(struct mutex *lock);
extern int mt_mutex_lock_killable(struct mutex *lock);

extern int mt_mutex_trylock(struct mutex *lock);
extern void mt_mutex_unlock(struct mutex *lock);
extern int mt_atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock);


/* semaphore */
extern struct semaphore* mt_sema_init(int val);
extern void mt_down(struct semaphore *sem);
extern int mt_down_interruptible(struct semaphore *sem);
extern int mt_down_killable(struct semaphore *sem);
extern int mt_down_trylock(struct semaphore *sem);
extern int mt_down_timeout(struct semaphore *sem, long jiffies);
extern void mt_up(struct semaphore *sem);
