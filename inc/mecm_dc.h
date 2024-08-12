/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master DC同步模块
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:53
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-17 20:31:58
 * @==============================================================================: 
 */
#ifndef _MECM_DC_H
#define _MECM_DC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	

typedef struct 
{
    uint8_t preslave;         /* 端口连接的上一个从站 */
    uint8_t preport;          /* 端口连接的上一个从站的端口 */
    uint8_t active;           /* 端口是否激活 */
    uint8_t use;              /* 端口在计算拓扑的时候是否已经使用了 */
    uint32_t porttime;        /* 端口锁存的时间 */   
}slaveport_t;



typedef struct 
{
    uint8_t portcnt;           /* 总共激活的端口个数 */
    uint8_t usecnt;            /* 已经用于拓扑计算的端口个数 */
    slaveport_t port[4];
    uint32_t delaytime;        /* 延时时间 */
} slave_dc_t;


void mecm_dc_init(void);
void mecm_sync0_init(uint16_t slave,uint32_t cycltime, uint32_t starttime);
#ifdef __cplusplus
}
#endif

#endif