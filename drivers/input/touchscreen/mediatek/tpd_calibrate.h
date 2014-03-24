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

#ifndef TPD_CALIBRATE_H
#define TPD_CALIBRATE_H
#ifdef TPD_HAVE_CALIBRATION

#ifndef TPD_CUSTOM_CALIBRATION

extern int tpd_calmat[8];
extern int tpd_def_calmat[8];

#endif

void tpd_calibrate(int *x, int *y);
#else

#define tpd_calibrate(x,y)

#endif

#endif
