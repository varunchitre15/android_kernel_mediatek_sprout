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

#ifndef __REFCODE_PDTSCAN_H__
#define __REFCODE_PDTSCAN_H__

#define PDT_ADDR		   0xEF
#define PDT_SIZE			6
#define CFG_F54_TXCOUNT 30
#define CFG_F54_RXCOUNT 46

extern unsigned short F01_Data_Base;
extern unsigned short F01_Cmd_Base;
extern unsigned short F01_Ctrl_Base;
extern unsigned short F01_Query_Base;
extern unsigned short F11_Data_Base;
extern unsigned short F11_Query_Base;
extern unsigned short F34_Data_Base;
extern unsigned short F34_Ctrl_Base;
extern unsigned short F34_Query_Base;
extern unsigned short F54_Data_Base;
extern unsigned short F54_Command_Base;
extern unsigned short F1A_Data_Base;
extern unsigned short F1A_Query_Base;

extern unsigned short F54_Data_LowIndex;
extern unsigned short F54_Data_HighIndex;
extern unsigned short F54_Data_Buffer;
extern unsigned short F54_CBCSettings;

extern unsigned char numberOfTx;
extern unsigned char numberOfRx;
extern unsigned char MaxButton;
extern unsigned char TxChannelUsed[CFG_F54_TXCOUNT];
extern unsigned char RxChannelUsed[CFG_F54_TXCOUNT];
extern unsigned char CheckButton[CFG_F54_TXCOUNT][CFG_F54_RXCOUNT];
extern unsigned char ButtonTXUsed[CFG_F54_TXCOUNT];
extern unsigned char ButtonRXUsed[CFG_F54_TXCOUNT];

#ifdef F54_Porting
void SYNA_PDTScan(void);
void SYNA_PDTScan_BootloaderMode(void);
void SYNA_ConstructRMI_F54(void);
void SYNA_ConstructRMI_F1A(void);
void SYNA_PrintRMI(void);
#endif

#endif
