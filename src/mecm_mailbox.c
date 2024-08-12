/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master Mailbox 模块 用于实现邮箱读写
 * @Autor: gxf
 * @Date: 2023-10-30 11:26:39
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 15:35:39
 * @==============================================================================: 
 */
#include "mecm_mailbox.h"
#include "mecm_config.h"
#include "mecm_port.h"
#include "mecm_base.h"
/**
 * @******************************************************************************: 
 * @func: [mecm_mailbox_send]
 * @description: 邮箱发送
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave  从站编号
 * @param [uint8_t] *mbx 待发送数据的地址
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
int mecm_mailbox_send(uint16_t slave,uint8_t *mbx,int timeout)
{
    uint8_t i = 0;
    int sendSM = -1;
    int wkc = 0;
    int cnt = 0;
    uint16_t smState = 0;
    uint16_t sendmbx_addr  = 0;
    uint16_t sendmbx_length = 0;
    uint64_t start_time = 0;
    uint64_t stop_time = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    /* 首先查找发送邮箱 */
    for(i=0;i<mecm_slave[slave].nSM;i++)
    {
        if((mecm_slave[slave].rSMtype[i] == 0x01)
        &&(mecm_slave[slave].rSM[i].sm_start_address>=0x1000)
        &&(mecm_slave[slave].rSM[i].sm_length>0))
        {
            sendSM = i;
            sendmbx_addr = mecm_slave[slave].rSM[i].sm_start_address;
            sendmbx_length = mecm_slave[slave].rSM[i].sm_length;
            break;
        }
    }
    /* 没找到发送邮箱相关的配置 */
    if(sendSM<0)
        return -1;
    /* 等待发送邮箱为空*/
    start_time = mecm_current_time_us();
    stop_time = start_time+timeout;
    do
    {
        /* 第一次发送没有延时，下次循环发送之前先延时一下 */
        if(cnt++) mecm_delay_us(300);
        smState = 0;
        wkc = mecm_FPRD(configaddr,ESC_SM0_REG_STATUS+sendSM*8,(uint8_t *)&smState,2,timeout);
        smState = etohs(smState);
    } while (((wkc<=0)||((smState&0x08)!=0))&&(mecm_current_time_us()<stop_time));
    /* 通信错误或者邮箱一直没有空闲，直接返回*/
    if((wkc<=0)||((smState&0x08)!=0))
        return -1;
    /* 开始发送数据 */
    wkc = mecm_FPWR(configaddr,sendmbx_addr,mbx,sendmbx_length,timeout);
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_mailbox_recv]
 * @description: 邮箱接收
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint8_t] *mbx 接收数据存储地址
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
int mecm_mailbox_recv(uint16_t slave,uint8_t *mbx,int timeout)
{
    uint8_t i = 0;
    int recvSM = -1;
    int wkc = 0;
    int wkc2 = 0;
    int cnt = 0;
    uint16_t smState = 0;
    uint8_t  smPDI   = 0;
    uint16_t recvmbx_addr  = 0;
    uint16_t recvmbx_length = 0;
    uint64_t start_time = 0;
    uint64_t stop_time = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    /* 首先查找接收邮箱 */
    for(i=0;i<mecm_slave[slave].nSM;i++)
    {
        if((mecm_slave[slave].rSMtype[i] == 0x02)
        &&(mecm_slave[slave].rSM[i].sm_start_address>=0x1000)
        &&(mecm_slave[slave].rSM[i].sm_length>0))
        {
            recvSM = i;
            recvmbx_addr = mecm_slave[slave].rSM[i].sm_start_address;
            recvmbx_length = mecm_slave[slave].rSM[i].sm_length;
            break;
        }
    }
    /* 没找到接收邮箱相关的配置 */
    if(recvSM<0)
        return 0;
    /* 等待接收邮箱满 */
    start_time = mecm_current_time_us();
    stop_time = start_time+timeout;
    do
    {
        do
        {
            /* 第一次发送没有延时，下次循环发送之前先延时一下 */
            if(cnt++) mecm_delay_us(300);
            smState = 0;
            wkc = mecm_FPRD(configaddr,ESC_SM0_REG_STATUS+recvSM*8,(uint8_t *)&smState,2,timeout);
            smState = etohs(smState);
        } while (((wkc<=0)||((smState&0x08)==0))&&(mecm_current_time_us()<stop_time));
        /* 邮箱中没有数据，直接返回 */
        if((wkc<=0)||((smState&0x08)==0))
            return 0;
        /* 读邮箱 */
        wkc2 = mecm_FPRD(configaddr,recvmbx_addr,mbx,recvmbx_length,timeout);
        /* 读邮箱失败 */
        if(wkc2 <=0 )
        {
            /* 翻转请求位 */
            smState ^= smState;
            smState = htoes(smState);
            wkc = mecm_FPWR(configaddr,ESC_SM0_REG_STATUS+recvSM*8,(uint8_t *)&smState,2,timeout);       
            /* 等待从站应答 */
            cnt = 0;
            do
            {
                /* 第一次发送没有延时，下次循环发送之前先延时一下 */
                if(cnt++) mecm_delay_us(300);
                smPDI = 0;
                wkc = mecm_FPRD(configaddr,ESC_SM0_REG_PDI+recvSM*8,(uint8_t *)&smPDI,1,timeout);
            } while (((wkc<=0)||((smPDI&0x02)!=0))&&(mecm_current_time_us()<stop_time));
            /* 重传失败 */
            if((wkc<=0)||((smPDI&0x02) ==0))
                return 0;            
        }
    } while ((wkc2<=0)&&(mecm_current_time_us()<stop_time));
    return wkc2;
}


/**
 * @******************************************************************************: 
 * @func: [get_mailbox_cnt]
 * @description: 获取邮箱头中的cnt
 * @note: 
 * @author: gxf
 * @param [uint8_t] slave 从站编号
 * @return [*]
 * @==============================================================================: 
 */
uint8_t get_mailbox_cnt(uint8_t slave)
{
    if(mecm_slave[slave].mbx_cnt == 0)
    {
        mecm_slave[slave].mbx_cnt = 1;
    }else
    {
        mecm_slave[slave].mbx_cnt++;
        if(mecm_slave[slave].mbx_cnt>7)
            mecm_slave[slave].mbx_cnt = 1;
    }
    return mecm_slave[slave].mbx_cnt;
}
