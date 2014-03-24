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

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include "sec_boot_lib.h"

/**************************************************************************
 *  MODULE NAME
 **************************************************************************/
#define MOD                         "ASF.FS"

/**************************************************************************
 *  EXTERNAL VARIABLES
 **************************************************************************/
extern bool                         bSecroExist;
extern bool                         bSecroIntergiy;

/**************************************************************************
 *  READ SECRO
 **************************************************************************/
uint32 sec_fs_read_secroimg (char* path, char* buf)
{
    uint32 ret  = SEC_OK;    
    const uint32 size = sizeof(AND_SECROIMG_T);
    uint32 temp = 0;    
    ASF_FILE fd;

    /* ------------------------ */    
    /* check parameter          */
    /* ------------------------ */
    SMSG(TRUE,"[%s] open '%s'\n",MOD,path);
    if(0 == size)
    {
        ret = ERR_FS_SECRO_READ_SIZE_CANNOT_BE_ZERO;
        goto _end;
    }

    /* ------------------------ */    
    /* open secro               */
    /* ------------------------ */    
    fd = ASF_OPEN(path);
    
    if (ASF_IS_ERR(fd)) 
    {
        ret = ERR_FS_SECRO_OPEN_FAIL;
        goto _open_fail;
    }

    /* ------------------------ */
    /* read secro               */
    /* ------------------------ */    
    /* configure file system type */
    osal_set_kernel_fs();
    
    /* adjust read off */
    ASF_SEEK_SET(fd,0);     
    
    /* read secro content */   
    if(0 >= (temp = ASF_READ(fd,buf,size)))
    {
        ret = ERR_FS_SECRO_READ_FAIL;
        goto _end;
    }

    if(size != temp)
    {
        SMSG(TRUE,"[%s] size '0x%x', read '0x%x'\n",MOD,size,temp);
        ret = ERR_FS_SECRO_READ_WRONG_SIZE;
        goto _end;
    }

    /* ------------------------ */       
    /* check integrity          */
    /* ------------------------ */
    if(SEC_OK != (ret = sec_secro_check()))
    {        
        goto _end;
    }

    /* ------------------------ */       
    /* SECROIMG is valid        */
    /* ------------------------ */
    bSecroExist = TRUE;    

_end:    
    ASF_CLOSE(fd);
    osal_restore_fs();
    
_open_fail:
    return ret;
}


