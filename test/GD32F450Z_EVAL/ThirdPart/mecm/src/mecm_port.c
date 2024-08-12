/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master 接口文件
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-06-02 15:58:47
 * @LastEditors: gxf
 * @LastEditTime: 2023-08-18 18:06:45
 * @==============================================================================: 
 */


#include "mecm_port.h"
#include "bsp_systick.h"
#include "md_enet.h"
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
    return enet_system_init();
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
    return enet_buf_send(buf,len);   
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
    int ret = 0;
    uint64_t starttime = mecm_current_time_us();
    uint64_t stoptime  = starttime + timeout;
    do{
        ret = enet_buf_recv(buf,len);
    }while((ret == 0)&&(mecm_current_time_us()<stoptime));  
    return ret;
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
    delay_us(us);
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
    delay_ms(ms);
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
    return osal_current_time_us();
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
    return osal_current_time_us()*1000;
}

