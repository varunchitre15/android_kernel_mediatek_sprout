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

#if !defined(_MT65XX_MON_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MT65XX_MON_TRACE_H

#include <linux/tracepoint.h>

TRACE_EVENT(mt65xx_mon_sched_switch,

    TP_PROTO(struct task_struct *prev,
         struct task_struct *next),

    TP_ARGS(prev, next),

    TP_STRUCT__entry(
        __field(unsigned int, log)
    ),

    TP_fast_assign(
        __entry->log = 0;
    ),

    TP_printk("log = %d",
        __entry->log
    )
);

TRACE_EVENT(mt65xx_mon_periodic,

    TP_PROTO(struct task_struct *prev,
         struct task_struct *next),

    TP_ARGS(prev, next),

    TP_STRUCT__entry(
        __field(unsigned int, log)
    ),

    TP_fast_assign(
        __entry->log = 0;
    ),

    TP_printk("log = %d",
        __entry->log
    )
);

TRACE_EVENT(mt65xx_mon_manual,

    TP_PROTO(unsigned int is_manual_start),

    TP_ARGS(is_manual_start),
    
    TP_STRUCT__entry(
        __field(unsigned int, log)
    ),

    TP_fast_assign(
        __entry->log = 0;
    ),

    TP_printk("log = %d",
        __entry->log
    )
);

#endif /* _MT65XX_MON_TRACE_H */


/* This part must be outside protection */
//#include <trace/define_trace.h>
