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

#ifndef __MT_IO_H__
#define __MT_IO_H__

#define IO_SPACE_LIMIT 0xffffffff

#define __io(a)         ((void __iomem *)((a)))
#define __mem_pci(a)    (a)
#define __mem_isa(a)    (a)


#endif  /* !__MT_IO_H__ */

