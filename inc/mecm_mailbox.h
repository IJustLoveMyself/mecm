/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master Mailbox 模块 用于实现邮箱通信协议
 * @Autor: gxf
 * @Date: 2023-10-30 11:26:21
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-30 11:45:41
 * @==============================================================================: 
 */
#ifndef _MECM_MAILBOX_H
#define _MECM_MAILBOX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	

int mecm_mailbox_send(uint16_t slave,uint8_t *mbx,int timeout);
int mecm_mailbox_recv(uint16_t slave,uint8_t *mbx,int timeout);
uint8_t get_mailbox_cnt(uint8_t slave);
#ifdef __cplusplus
}
#endif

#endif
