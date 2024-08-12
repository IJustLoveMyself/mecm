/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master 接口文件 用于移植
 * @Autor: gxf
 * @Date: 2023-06-02 15:58:35
 * @LastEditors: gxf
 * @LastEditTime: 2023-09-26 19:39:47
 * @==============================================================================: 
 */

#ifndef _MECM_PORT_H
#define _MECM_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	


/* 网卡初始化 */
int mecm_eth0_init(char *eth0);
/* 以太网数据包发送函数 */
int mecm_data_send(uint8_t *buf,uint32_t len);
/* 以太网数据包接收函数 */
int mecm_data_recv(uint8_t *buf,uint32_t len,uint32_t timeout);
/* 延时us */
void mecm_delay_us(uint32_t us);
/* 延时ms */
void mecm_delay_ms(uint32_t ms);
/* 获取当前的时间 us */
uint64_t mecm_current_time_us(void);
/* 获取当前时间 ns */
uint64_t mecm_current_time_ns(void);
#ifdef __cplusplus
}
#endif

#endif
