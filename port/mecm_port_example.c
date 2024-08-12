/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master 接口文件
 * @Autor: gxf
 * @Date: 2023-06-02 15:58:47
 * @LastEditors: gxf
 * @LastEditTime: 2023-08-18 18:06:45
 * @==============================================================================: 
 */


#include "mecm_port.h"
#include "mecm_base.h"

/**
 * @******************************************************************************: 
 * @func: [mecm_eth0_init]
 * @description: 网卡初始化
 * @note: 
 * @author: gxf
 * @param  [char] *eth0 待初始化的网卡
 * @return [int] 0：初始化失败 1：初始化成功 
 * @==============================================================================: 
 */
int mecm_eth0_init(char *eth0)
{
    mecm_base_init();
    /*网卡初始化*/
    /*
     do something 
    */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_data_send]
 * @description: 网络数据包发送
 * @note: 
 * @author: gxf
 * @param [uint8_t] *buf 待发送的数据包
 * @param [uint32_t] len 待发送数据包的长度
 * @return [int] 0：发送失败，1：发送成功
 * @==============================================================================: 
 */
int mecm_data_send(uint8_t *buf,uint32_t len)
{
    /*  网络数据发送 do something */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_data_recv]
 * @description: 网络数据包接收
 * @note: 
 * @author: gxf
 * @param [uint8_t] *buf  接收数据存储地址
 * @param [uint32_t] len  接收数据存储地址的长度
 * @param [uint32_t] timeout 接收超时时间,单位us
 * @return [int] 0：接收失败，1：接收成功
 * @==============================================================================: 
 */
int mecm_data_recv(uint8_t *buf,uint32_t len,uint32_t timeout)
{
    /* 网络数据接收 do something */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_delay_us]
 * @description: 延时
 * @note: 
 * @author: gxf
 * @param [uint32_t] us 微妙延时
 * @return [*]
 * @==============================================================================: 
 */
void mecm_delay_us(uint32_t us)
{
    /* us延时 do something */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_delay_ms]
 * @description: 延时
 * @note: 
 * @author: gxf
 * @param [uint32_t] ms 毫秒延时
 * @return [*]
 * @==============================================================================: 
 */
void mecm_delay_ms(uint32_t ms)
{
    /* ms延时 do something */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_current_time_us]
 * @description: 获取当前系统时间
 * @note: 
 * @author: gxf
 * @return [uint64_t] 微妙
 * @==============================================================================: 
 */
uint64_t mecm_current_time_us(void)
{
    /* 获取当前系统时间 do something */
}

/**
 * @******************************************************************************: 
 * @func: [mecm_current_time_ns]
 * @description: 获取当前系统时间
 * @note: 
 * @author: gxf
 * @return [uint64_t] 纳秒
 * @==============================================================================: 
 */
uint64_t mecm_current_time_ns(void)
{
    /* 获取当前系统时间 do something */
}

