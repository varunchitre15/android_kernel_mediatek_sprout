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

#ifndef _TRACE_H_
#define _TRACE_H_

#define FMT1	",%x\n"
#define FMT2	",%x,%x\n"
#define FMT3	",%x,%x,%x\n"
#define FMT4	",%x,%x,%x,%x\n"
#define FMT5	",%x,%x,%x,%x,%x\n"
#define FMT6	",%x,%x,%x,%x,%x,%x\n"
#define FMT7	",%x,%x,%x,%x,%x,%x,%x\n"
#define FMT8	",%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT9	",%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT10	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"

#define FMT14	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT17	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT18	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT19	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT30	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT34	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT37	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT44	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT50	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT66	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define FMT82	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"

#define VAL1	,value[0]
#define VAL2	,value[0],value[1]
#define VAL3	,value[0],value[1],value[2]
#define VAL4	,value[0],value[1],value[2],value[3]
#define VAL5	,value[0],value[1],value[2],value[3],value[4]
#define VAL6	,value[0],value[1],value[2],value[3],value[4],value[5]
#define VAL7	,value[0],value[1],value[2],value[3],value[4],value[5],value[6]
#define VAL8	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7]
#define VAL9	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8]
#define VAL10	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]

#define VAL14	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13]
#define VAL17	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16]
#define VAL18	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17]
#define VAL19	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18]

#define VAL30	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]

#define VAL34	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33]

#define VAL37	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33],value[34],value[35],value[36]

#define VAL44	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33],value[34],value[35],value[36],value[37],value[38],value[39]\
		,value[40],value[41],value[42],value[43]
#define VAL50	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33],value[34],value[35],value[36],value[37],value[38],value[39]\
		,value[40],value[41],value[42],value[43],value[44],value[45],value[46],value[47],value[48],value[49]
#define VAL66	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33],value[34],value[35],value[36],value[37],value[38],value[39]\
		,value[40],value[41],value[42],value[43],value[44],value[45],value[46],value[47],value[48],value[49]\
		,value[50],value[51],value[52],value[53],value[54],value[55],value[56],value[57],value[58],value[59]\
		,value[60],value[61],value[62],value[63],value[64],value[65]
#define VAL82	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14],value[15],value[16],value[17],value[18],value[19]\
                ,value[20],value[21],value[22],value[23],value[24],value[25],value[26],value[27],value[28],value[29]\
		,value[30],value[31],value[32],value[33],value[34],value[35],value[36],value[37],value[38],value[39]\
		,value[40],value[41],value[42],value[43],value[44],value[45],value[46],value[47],value[48],value[49]\
		,value[50],value[51],value[52],value[53],value[54],value[55],value[56],value[57],value[58],value[59]\
		,value[60],value[61],value[62],value[63],value[64],value[65],value[66],value[67],value[68],value[69]\
		,value[70],value[71],value[72],value[73],value[74],value[75],value[76],value[77],value[78],value[79]\
		,value[80],value[81]


void mp_cp(unsigned long long timestamp,
	   struct task_struct *task,
	   unsigned long program_counter,
	   unsigned long dcookie,
	   unsigned long offset,
	   unsigned char cnt, unsigned int *value);

void mp_cp2(unsigned long long timestamp,
	   struct task_struct *task,
	   unsigned long program_counter,
	   unsigned long dcookie,
	   unsigned long offset,
	   unsigned char cnt, unsigned int *value);

void cpu_frequency(unsigned int frequency, unsigned int cpu_id);
extern void (*mp_cp_ptr)(unsigned long long timestamp,
	       struct task_struct *task,
	       unsigned long program_counter,
	       unsigned long dcookie,
	       unsigned long offset,
	       unsigned char cnt, unsigned int *value);

#endif // _TRACE_H_
