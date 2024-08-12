/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master FOE模块
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:37
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 18:36:24
 * @==============================================================================: 
 */
#ifndef _MECM_FOE_H
#define _MECM_FOE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	
#include <stdio.h>
#include "mecm_coe.h"
#define FOE_BUF_MAX_BYTE 512

#define    FOE_ERRCODE_NOTDEFINED          0x8000 
#define    FOE_ERRCODE_NOTFOUND            0x8001 
#define    FOE_ERRCODE_ACCESS              0x8002 
#define    FOE_ERRCODE_DISKFULL            0x8003 
#define    FOE_ERRCODE_ILLEGAL             0x8004 
#define    FOE_ERRCODE_PACKENO             0x8005 
#define    FOE_ERRCODE_EXISTS              0x8006 
#define    FOE_ERRCODE_NOUSER              0x8007 
#define    FOE_ERRCODE_BOOTSTRAPONLY       0x8008 
#define    FOE_ERRCODE_NOTINBOOTSTRAP      0x8009 
#define    FOE_ERRCODE_NORIGHTS            0x800A 
#define    FOE_ERRCODE_PROGERROR           0x800B 
#define    FOE_ERRCODE_INVALID_CHECKSUM    0x800C 
#define    FOE_ERRCODE_INVALID_FIRMWARE    0x800D 
#define    FOE_ERRCODE_NO_FILE             0x800F 
#define    FOE_ERRCODE_NO_FILE_HEADER      0x8010 
#define    FOE_ERRCODE_FLASH_ERROR         0x8011 

#pragma pack(1)

typedef struct 
{
    _mbx_header_t mbxheader;
    uint8_t opcode;
    uint8_t reserved;
    union 
    {
        uint32_t password;
        uint32_t packetno;
        uint32_t errcode;
        struct 
        {
            uint16_t done;
            uint16_t entire;
        }busy;        
    }foe;
    uint8_t data[FOE_BUF_MAX_BYTE]; 
} _foe_data_t;

#pragma pack()

enum{
    FOE_READ  = 0x01,
    FOE_WRITE = 0x02,
    FOE_DATA  = 0x03,
    FOE_ACK   = 0x04,
    FOE_ERR   = 0x05,
    FOE_BUSY  = 0x06
};

enum{
    FOE_TASK_STATE_STOP = 0,
    FOE_TASK_STATE_EXCEPT,
    FOE_TASK_STATE_READ,
    FOE_TASK_STATE_WRITE,
    FOE_TASK_STATE_DATA,
    FOE_TASK_STATE_ACK,
    FOE_TASK_STATE_ERR,
    FOE_TASK_STATE_RETRY
};

int mecm_foe_start(uint16_t slave);
void mecm_foe_task(uint16_t slave,int timeout);
void mecm_foe_set_info(char *filename,uint32_t pwd,uint32_t filesize,uint8_t state);


__attribute__((weak)) void mecm_foe_read_data_handle(uint8_t *buf,uint16_t size);
__attribute__((weak)) void mecm_foe_write_data_handle(uint8_t *buf,uint16_t size);
__attribute__((weak)) void mecm_foe_err_handle(uint32_t errcode);
__attribute__((weak)) void mecm_foe_busy_handle(uint16_t done,uint16_t entire);
__attribute__((weak)) void mecm_foe_stop(void);
__attribute__((weak)) void mecm_foe_except(void);
#ifdef __cplusplus
}
#endif

#endif