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

#ifndef _ETM_STATE_H_
#define _ETM_STATE_H_

enum {
	TRACE_STATE_STOP = 0,		/**< trace stopped */
	TRACE_STATE_TRACING ,		/**< tracing */
	TRACE_STATE_UNFORMATTING,	/**< unformatting frame */
	TRACE_STATE_UNFORMATTED,	/**< frame unformatted */
	TRACE_STATE_SYNCING,		/**< syncing to trace head */ //TODO
	TRACE_STATE_PARSING,		/**< decoding packet */
};

/** check whether we are in some state */
#define IS_IN_STATE(ctx, _state) ((ctx)->state == _state)

#endif

