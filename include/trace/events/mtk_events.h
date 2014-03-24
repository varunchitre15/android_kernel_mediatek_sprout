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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_events 

#if !defined(_TRACE_MTK_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MTK_EVENTS_H

#include <linux/tracepoint.h>

//TRACE_EVENT(new_trace_event,
//
//	TP_PROTO(unsigned int irq,
//		 int pid),
//
//	TP_ARGS(irq, pid),
//
//	TP_STRUCT__entry(
//		__field(	unsigned int,	member1			)
//		__field(	int,	member2			)
//	),
//
//	TP_fast_assign(
//		__entry->member1	= irq;
//		__entry->member2	= pid;
//	),
//
//	TP_printk("output string for new trace event %d,%d",
//		__entry->member1,
//		__entry->member2
//	)
//);

TRACE_EVENT(tracing_on,

        TP_PROTO(int on),

        TP_ARGS(on),

        TP_STRUCT__entry(
            __field(int, on)
        ),

        TP_fast_assign(
            __entry->on = on;
        ),

        TP_printk("ftrace is %s",
            __entry->on?"enabled":"disabled")
);

TRACE_EVENT(cpu_hotplug,

        TP_PROTO(unsigned int cpu_id, unsigned int state, unsigned long long ts),

        TP_ARGS(cpu_id, state, ts),

        TP_STRUCT__entry(
            __field(    u32,    cpu_id)
            __field(    u32,    state)
            __field(    u64,    ts)
        ),

        TP_fast_assign(
            __entry->cpu_id = cpu_id;
            __entry->state = state;
            __entry->ts = ts;
        ),

        TP_printk("cpu=%lu state=%s last_%s_ts=%Lu",
            (unsigned long)__entry->cpu_id,
            __entry->state?"online":"offline",
            __entry->state?"offline":"online",
            __entry->ts)
);

TRACE_EVENT(ipi_handler_entry,

        TP_PROTO(int ipinr, const char *ipiname),

        TP_ARGS(ipinr, ipiname),

        TP_STRUCT__entry(
            __field(	int,	ipi		)
            __string(	name,	ipiname	)
        ),

        TP_fast_assign(
		    __entry->ipi= ipinr;
		    __assign_str(name, ipiname);
        ),

	    TP_printk("ipi=%d name=%s", __entry->ipi, __get_str(name))
);

TRACE_EVENT(ipi_handler_exit,

        TP_PROTO(int ipinr),

        TP_ARGS(ipinr),

        TP_STRUCT__entry(
            __field(	int,	ipi		)
        ),

        TP_fast_assign(
		    __entry->ipi= ipinr;
        ),

	    TP_printk("ipi=%d", __entry->ipi)
);

TRACE_EVENT(unnamed_irq,

        TP_PROTO(int irq),

        TP_ARGS(irq),

        TP_STRUCT__entry(
            __field(	int,	irq		)
        ),

        TP_fast_assign(
		    __entry->irq= irq;
        ),

	    TP_printk("irq=%d", __entry->irq)
);

#endif /* _TRACE_MTK_EVENTS_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
