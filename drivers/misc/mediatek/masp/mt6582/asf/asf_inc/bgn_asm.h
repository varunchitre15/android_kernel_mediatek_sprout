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

#ifndef _BN_ASM_H
#define _BN_ASM_H

#define ALU_INIT                         \
{                                        \
    unsigned long s_0, s_1, b_0, b_1;    \
    unsigned long r_0, r_1, r_x, r_y;    \
    b_0 = ( b << biH ) >> biH;           \
    b_1 = ( b >> biH );

#define ALU_CORE                         \
    s_0 = ( *s << biH ) >> biH;          \
    s_1 = ( *s >> biH ); s++;            \
    r_x = s_0 * b_1; r_0 = s_0 * b_0;    \
    r_y = s_1 * b_0; r_1 = s_1 * b_1;    \
    r_1 += ( r_x >> biH );               \
    r_1 += ( r_y >> biH );               \
    r_x <<= biH; r_y <<= biH;            \
    r_0 += r_x; r_1 += (r_0 < r_x);      \
    r_0 += r_y; r_1 += (r_0 < r_y);      \
    r_0 +=  c; r_1 += (r_0 <  c);        \
    r_0 += *d; r_1 += (r_0 < *d);        \
    c = r_1; *(d++) = r_0;

#define ALU_STOP                         \
}

#endif
