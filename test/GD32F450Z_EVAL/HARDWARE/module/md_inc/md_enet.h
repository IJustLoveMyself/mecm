/*
 * @******************************************************************************: 
 * @Description: 
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-06-01 20:38:22
 * @LastEditors: gxf
 * @LastEditTime: 2024-04-02 18:05:52
 * @==============================================================================: 
 */
#ifndef _MD_ENET_H_
#define _MD_ENET_H_
#include "gd32f4xx.h" 


int enet_system_init(void);
int enet_buf_send(uint8_t *buf,uint32_t length);
int enet_buf_recv(uint8_t *buf,uint32_t length);

int enet_buf_recv_2(uint8_t *buf,uint32_t length,uint16_t *recvlen);
#endif
