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

typedef struct
{
    uint8_t reqStatus;       
    uint8_t retryCount;
    uint8_t command;         
    uint8_t offsetData;      
    uint8_t length;          
    union
    {
    uint8_t msgData[ 16 ];   
	unsigned char	*pdatabytes;			
    }payload_u;
} cbus_req_t;
bool_t 	SiiMhlTxChipInitialize( void );
void 	SiiMhlTxDeviceIsr( void );
bool_t	SiiMhlTxDrvSendCbusCommand ( cbus_req_t *pReq  );
bool_t SiiMhlTxDrvCBusBusy(void);
void	SiiMhlTxDrvTmdsControl( bool_t enable );
void	SiiMhlTxDrvPowBitChange( bool_t powerOn );
void	SiiMhlTxDrvNotifyEdidChange ( void );
bool_t SiiMhlTxReadDevcap( uint8_t offset );
void SiiMhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length);
void SiMhlTxDrvSetClkMode(uint8_t clkMode);
extern	void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus );
extern	void	SiiMhlTxNotifyConnection( bool_t mhlConnected );
extern void SiiMhlTxNotifyRgndMhl( void );
extern	void	SiiMhlTxMscCommandDone( uint8_t data1 );
extern	void	SiiMhlTxMscWriteBurstDone( uint8_t data1 );
extern	void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 );
extern	void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 );
extern	void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData );
extern	void	SiiMhlTxGotMhlWriteBurst( uint8_t *spadArray );
extern void SiiMhlTxDrvProcessRgndMhl( void );
