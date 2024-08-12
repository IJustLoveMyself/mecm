/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master COE 模块
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:16
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-31 15:10:46
 * @==============================================================================: 
 */
#ifndef _MECM_COE_H
#define _MECM_COE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	

/* SDO 邮箱缓存的大小 */
#define SDO_BUF_MAX_BYTE            512
#define MAX_SUBINDEX_NUM            100

#pragma pack(1)

typedef struct 
{
    uint16_t length;        /* 数据长度 */
    uint16_t address;       /* 目的地址 */
    uint8_t  channel:6;     /* 保留，默认0 */
    uint8_t  priority:2;    /* 优先级 */
    uint8_t  type:4;        /* 类型 */
    uint8_t  cnt:3;         /* 计数1~7 */
    uint8_t  reserved:1;   
}_mbx_header_t;


typedef struct 
{
    _mbx_header_t mbxheader;
    union 
    {
        uint16_t data;
        struct 
        {
            uint16_t pdonumber:9;
            uint16_t reserved:3;
            uint16_t opcode:4;
        }cmd;        
    }coe;
    uint8_t  cs;
    uint16_t index;
    uint8_t  subindex;
    uint8_t data1[SDO_BUF_MAX_BYTE]; 
} _sdo_data_t;


typedef struct 
{
    uint16_t n;
    uint16_t index[MAX_SUBINDEX_NUM];
} _pdo_assign_t;

typedef struct 
{
    uint16_t n;
    uint32_t index[MAX_SUBINDEX_NUM];
} _pdo_map_t;

#pragma pack()


/** CoE CMD */
enum
{
   COE_EMERGENCY  = 0x01,     /* 紧急事件信息 */
   COE_SDOREQ,                /* SDO请求 */
   COE_SDORES,                /* SDO响应 */
   COE_TXPDO,                 /* TXPDO数据 */
   COE_RXPDO,                 /* RXPDO数据 */
   COE_TXPDO_RR,              /* 远端TXPDO数据请求 */
   COE_RXPDO_RR,              /* 远端RXPDO数据请求 */
   COE_SDOINFO                /* SDO信息 */
};

enum
{
   SDO_DOWN_NORMAL    = 0x21,  /* 常规传输下载请求 */
   SDO_DOWN_EXP       = 0x23,  /* 快速传输下载请求 */
   SDO_DOWN_NORMAL_CA = 0x31,  /* 常规传输下载请求，完全操作 */
   SDO_UP_REQ         = 0x40,  /* 常规传输上传请求 */
   SDO_UP_REQ_CA      = 0x50,  /* 常规传输上传请求，完全操作 */
   SDO_SEG_UP_REQ     = 0x60,  /* 分段传输上传请求 */
   SDO_ABORT          = 0x80   /* 异常 */
};

int sdo_write_exp(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,int timeout);
int sdo_write_normal(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout);
int sdo_write_segment(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout);
int sdo_write(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout);
int sdo_read(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t *datalen,uint8_t complete_access,int timeout);
uint32_t get_pdo_map(uint16_t slave, uint16_t index);
#ifdef __cplusplus
}
#endif

#endif
