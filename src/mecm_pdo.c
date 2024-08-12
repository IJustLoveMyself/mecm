/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master PDO模块
 * @Autor: gxf
 * @Date: 2023-11-01 17:46:10
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-02 20:14:10
 * @==============================================================================: 
 */
#include "mecm_pdo.h"
#include "mecm_config.h"
#include "mecm_base.h"

static int64_t difftimer;                 /* 下一个sync信号于系统时钟的时间差 */
static uint64_t dctime = 0;
static uint64_t synctime = 0;
int64_t get_difftimer(void)
{
    return difftimer;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_processdata_all]
 * @description: 与所有从站进行PDO通信
 * @note: 
 * @author: gxf
 * @param [uint8_t] mapping_mode PDO映射方式 1：映射方式1；其他：映射方式2
 * @param [uint8_t] get_alstate 是否同时读取alstate
 * @param [uint32_t] timeout 超时时间
 * @return [*] PDO通信的wkc
 * @==============================================================================: 
 */
int mecm_processdata_all(uint8_t mapping_mode,uint8_t get_alstate,uint32_t timeout)
{
    int wck = 0;
    uint8_t idx;
    uint32_t startaddr = 0;
    uint8_t *bufaddr = 0;
    uint16_t slave = 1;
    uint16_t lrwlength = 0;
    uint16_t dctimeroffset = 0;
    uint16_t sync0offset = 0;
    uint16_t aloffset = 0;
    uint16_t framlength = 0;
    uint16_t configaddr = 0;

    uint8_t buf[6];
    /* 映射方式1 rtrtrtrt */
    if(mapping_mode == 1)
    {
        /* 获取逻辑通信的起始地址 从第一个不为0的地址开始 */
        for (slave = 1; slave <= mecm_slave_count; slave++)
        {
            if(mecm_slave[slave].logrpdoaddr != 0)
            {
            startaddr = mecm_slave[slave].logrpdoaddr;
            bufaddr = mecm_slave[slave].rpdoaddr;
            break;
            }else if(mecm_slave[slave].logtpdoaddr != 0)
            {
            startaddr = mecm_slave[slave].logtpdoaddr;
            bufaddr = mecm_slave[slave].tpdoaddr;
            break;
            }        
        }        
    }else
    {
        /* 映射方式2 rrrrrrttttttt */
        /* 获取逻辑通信的起始地址 从第一个不为0的地址开始 先查RPDO地址*/
        for (slave = 1; slave <= mecm_slave_count; slave++)
        {
            if(mecm_slave[slave].logrpdoaddr != 0)
            {
                startaddr = mecm_slave[slave].logrpdoaddr;
                bufaddr = mecm_slave[slave].rpdoaddr;
                break;
            }      
        }
        /* 没有RPDO映射，在查TPDO地址 */
        if(startaddr == 0)
        {
            for (slave = 1; slave <= mecm_slave_count; slave++)
            {
            if(mecm_slave[slave].logtpdoaddr != 0)
                {
                    startaddr = mecm_slave[slave].logtpdoaddr;
                    bufaddr = mecm_slave[slave].tpdoaddr;
                    break;
                }        
            }        
        }

    }

    /* 计算逻辑读写的长度 */
    for (slave = 1; slave <= mecm_slave_count; slave++)
    {
        if(mecm_slave[slave].rpdobyte != 0)
        {
            lrwlength = lrwlength+mecm_slave[slave].rpdobyte;
        }
        if(mecm_slave[slave].tpdobyte != 0)
        {
            lrwlength = lrwlength+mecm_slave[slave].tpdobyte;
        }        
    }

    if((startaddr == 0)||(lrwlength == 0))
    {
        return 0;
    }
    /* 数据组包 */
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* LRW 数据*/
    dctimeroffset = mecm_setup_datagram_logi(CMD_LRW,idx,16,startaddr,bufaddr,lrwlength,1);
    /* 读取系统时钟数据包 数据包 DC同步模式会用到 先查找离主站最近的DC可用的从站 */
    for (slave = 1; slave <= mecm_slave_count; slave++)
    {
        if(mecm_slave[slave].dcflag == 1)
        {
            configaddr = mecm_slave[slave].configaddr;
            break;
        }
        
    }
    sync0offset = mecm_setup_datagram_np(CMD_FRMW,idx,dctimeroffset,configaddr,ESC_REG_DCSYSTIME,(uint8_t *)&dctime,sizeof(uint64_t),1);   
    /* 如果需要同时获取状态信息,添加读取数据包 */
    if(get_alstate)
    {
        /* 读取从站下一个sync信号系统时钟的数据包*/
        framlength = mecm_setup_datagram_np(CMD_FPRD,idx,sync0offset,configaddr,ESC_REG_DCSTART0,(uint8_t *)&synctime,sizeof(uint64_t),1);
        aloffset = framlength;        
        for (slave = 1; slave <= mecm_slave_count; slave++)
        {
            configaddr = mecm_slave[slave].configaddr;
            if(slave != mecm_slave_count)
            {
                framlength = mecm_setup_datagram_np(CMD_FPRD,idx,framlength,configaddr,ESC_REG_ALSTAT,buf,6,1);
            }              
            else
            {
                framlength = mecm_setup_datagram_np(CMD_FPRD,idx,framlength,configaddr,ESC_REG_ALSTAT,buf,6,0);
            }
            
        }
    }else
    {
        framlength = mecm_setup_datagram_np(CMD_FPRD,idx,sync0offset,configaddr,ESC_REG_DCSTART0,(uint8_t *)&synctime,sizeof(uint64_t),0);
    }
    
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 PDO数据*/
        wck = (mecm_port.mecm_recvbuf[idx].datagram[dctimeroffset-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[dctimeroffset-16-2];
        memcpy(bufaddr,&(mecm_port.mecm_recvbuf[idx].datagram[10]),dctimeroffset-16-10-2);
        /* 获取系统时钟数据 */
        memcpy(&dctime,&(mecm_port.mecm_recvbuf[idx].datagram[dctimeroffset+10]),8);
        /* 获取下一个sync0的时间数据*/
        memcpy(&synctime,&(mecm_port.mecm_recvbuf[idx].datagram[sync0offset+10]),8);
        /* 计算时间差 */
        difftimer = synctime-dctime;
        /* 获取状态信息 */
        if(get_alstate)
        {
            for(slave = 1; slave <= mecm_slave_count; slave++)
            {
                /* 18表示 datagram包头10byte+数据6byte+wkc 2byte */
                memcpy(buf,&(mecm_port.mecm_recvbuf[idx].datagram[aloffset+10+(slave-1)*18]),6);
                mecm_slave[slave].state = (buf[1]<<8)|buf[0];
                mecm_slave[slave].alstatuscode = (buf[5]<<8)|buf[4];
            }              
        }
      
    }else{
        wck = 0;
    }
    mecm_clear_port(idx);
    return wck;      
}


