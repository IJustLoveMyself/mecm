/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master Base 模块 
 * @Autor: gxf
 * @Date: 2023-06-02 15:52:57
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-01 21:00:08
 * @==============================================================================: 
 */

#include "mecm_base.h"
#include "mecm_type.h"
#include "mecm_port.h"
#include <string.h>

/* 收发接口 */
mecm_port_t mecm_port;
/* 目的地址 */
static uint8_t dstMAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
/* 源地址 */
static uint8_t srcMAC[6] = {0x01,0x01,0x01,0x01,0x01,0x01};


/**
 * @******************************************************************************: 
 * @func: [mecm_base_init]
 * @description: mecm base模块一些变量的初始化
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void mecm_base_init(void)
{
    int i = 0;
    /* 清空收发结构体 */
    memset(&mecm_port,0,sizeof(mecm_port_t));
    /* 将所有发送帧的包头填充好，后续组包的时候就不需要进行填充了 */
    for(i = 0;i<MAXBUF;i++)
    {
        memcpy( mecm_port.mecm_sendbuf[i].en_header.dst,dstMAC,6);
        memcpy( mecm_port.mecm_sendbuf[i].en_header.src,srcMAC,6);
        mecm_port.mecm_sendbuf[i].en_header.etype = htons(0x88A4);
    }
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_src_dst_mac]
 * @description: 配置数据帧中的MAC地址，如果不配置，则使用默认的
 * @note: 
 * @author: gxf
 * @param [uint8_t] *src 源地址
 * @param [uint8_t] *dst 目的地址
 * @return [*]
 * @==============================================================================: 
 */
void mecm_set_src_dst_mac(uint8_t *src,uint8_t *dst)
{
    uint8_t i = 0;
    for(i=0;i<6;i++)
    {
        srcMAC[i] = src[i];
    }
    for(i=0;i<6;i++)
    {
        dstMAC[i] = dst[i];
    }
}


/**
 * @******************************************************************************: 
 * @func: [mecm_setup_datagram_np]
 * @description: 数据组包，自增寻址，配置寻址，广播调用 
 * @note: 
 * @author: gxf
 * @param [mecm_type_e] cmd  寻址命令
 * @param [uint8_t] idx 索引
 * @param [uint16_t] offset 填充数据起始位置的偏移量
 * @param [uint16_t] adp 从站地址
 * @param [uint16_t] ado 内存地址
 * @param [uint8_t] *data 如果是写命令，则代表待写入数据地址，如果时读命令，则无意义
 * @param [uint16_t] length 待操作数据的长度
 * @return [*]  返回当前数据包的总长度
 * @==============================================================================: 
 */
uint16_t mecm_setup_datagram_np(mecm_type_e cmd,uint8_t idx,uint16_t offset,uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint8_t more)
{
    uint16_t framlength = 0;
    mecm_datagram_header_t dg_header;
    mecm_port.mecm_sendbuf[idx].ec_header.clehgth = (offset+10+length+2-16);//总长度-以太网帧头的长度
    mecm_port.mecm_sendbuf[idx].ec_header.reserve = 0;
    mecm_port.mecm_sendbuf[idx].ec_header.ctype   = 0x01;
    /* 大小端转换，如果本机是小端，其实这个转换无意义 */
    mecm_port.mecm_sendbuf[idx].ec_header = htoes(mecm_port.mecm_sendbuf[idx].ec_header);
    dg_header.cmd     = cmd;
    dg_header.idx     = idx;
    dg_header.addr.np.adp = htoes(adp);
    dg_header.addr.np.ado = htoes(ado);
    /* 默认 r c 一直等于0 */
    dg_header.info.data.r = 0;
    dg_header.info.data.c = 0;
    dg_header.info.data.m = more;
    dg_header.info.data.dlength = length;
    dg_header.info.length = htoes(dg_header.info.length);
    dg_header.irq     = 0x0000;
    /* 填充 datagram header */
    memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset,(uint8_t *)&dg_header,10);
    /* 填充数据 */
    if(length > 0)
    {
      switch (cmd)
      {
         case CMD_NOP:
         case CMD_APRD:
         case CMD_FPRD:
         case CMD_BRD:
         case CMD_LRD:
            /* 读数据，填充 0  */
            memset((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10,0,length);
            break;
         default:
            memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10,data,length);
            break;
      }
    }       
    /* 填充 WCK 的值 0*/
    uint16_t wck = 0;
    memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10+length,(uint8_t *)&wck,2);
    /* 当前帧的总长度 */
    framlength = offset+10+length+2;
    /* 记录当前数据包在整个帧中的偏移量，方便后续取数据*/
    mecm_port.datagram_info[idx].datagram_offset[mecm_port.datagrams[idx]] = offset;
    /* 记录当前数据包中数据的长度，不包含帧头和WCK的长度*/
    mecm_port.datagram_info[idx].datagram_length[mecm_port.datagrams[idx]] = length;
    /* 帧个数++ */
    mecm_port.datagrams[idx]++;
    return framlength;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_setup_datagram_logi]
 * @description: 数据组包，逻辑寻址时调用
 * @note: 
 * @author: gxf
 * @param [mecm_type_e] cmd 寻址命令
 * @param [uint8_t] idx 索引
 * @param [uint16_t] offset 填充数据起始位置的偏移量
 * @param [uint32_t] logiaddr 逻辑地址
 * @param [uint8_t] *data 待操作数据的地址
 * @param [uint16_t] length 待操作数据的长度
 * @return [*]返回当前数据包的总长度
 * @==============================================================================: 
 */
uint16_t mecm_setup_datagram_logi(mecm_type_e cmd,uint8_t idx,uint16_t offset,uint32_t logiaddr,uint8_t *data,uint16_t length,uint8_t more)
{
    uint16_t framlength = 0;
    mecm_datagram_header_t dg_header;
    mecm_port.mecm_sendbuf[idx].ec_header.clehgth = (offset+10+length+2-16);
    mecm_port.mecm_sendbuf[idx].ec_header.reserve = 0;
    mecm_port.mecm_sendbuf[idx].ec_header.ctype   = 0x01;
    /* 大小端转换，如果本机是小端，其实这个转换无意义 */
    mecm_port.mecm_sendbuf[idx].ec_header = htoes(mecm_port.mecm_sendbuf[idx].ec_header);
    dg_header.cmd     = cmd;
    dg_header.idx     = idx;
    dg_header.addr.logiaddr = htoel(logiaddr);
    /* 默认 r c 一直等于0 */
    dg_header.info.data.r = 0;
    dg_header.info.data.c = 0;
    dg_header.info.data.m = more;
    dg_header.info.data.dlength = length;
    dg_header.info.length = htoes(dg_header.info.length);
    dg_header.irq     = 0x0000;
    /* 填充 datagram header */
    memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset,(uint8_t *)&dg_header,10);
    /* 填充数据 */
    if(length > 0)
    {
      switch (cmd)
      {
         case CMD_NOP:
         case CMD_APRD:
         case CMD_FPRD:
         case CMD_BRD:
         case CMD_LRD:
            /* 读数据，填充 0  */
            memset((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10,0,length);
            break;
         default:
            memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10,data,length);
            break;
      }
    } 
    /* 填充 WCK 的值 0*/
    uint16_t wck = 0;
    memcpy((uint8_t *)&mecm_port.mecm_sendbuf[idx]+offset+10+length,(uint8_t *)&wck,2);
    /* 当前帧的总长度 */
    framlength = offset+10+length+2;
    /* 记录当前数据包在整个帧中的偏移量，方便后续取数据*/
    mecm_port.datagram_info[idx].datagram_offset[mecm_port.datagrams[idx]] = offset;
    /* 记录当前数据包中数据的长度，不包含帧头和WCK的长度*/
    mecm_port.datagram_info[idx].datagram_length[mecm_port.datagrams[idx]] = length;
    /* 帧个数++ */
    mecm_port.datagrams[idx]++;
    return framlength;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_communicate]
 * @description: 数据帧收发
 * @note: 
 * @author: gxf
 * @param [uint8_t] idx 索引编号
 * @param [uint8_t] *buf 待发送数据的地址
 * @param [uint16_t] length 待发送数据的总长度
 * @param [uint32_t] timeout 超时时间,单位us
 * @return [*] 1:数据帧收发成功 0：收发失败
 * @==============================================================================: 
 */
int mecm_communicate(uint8_t idx,uint8_t *buf,uint16_t length,uint32_t timeout)
{
    int wck = 0;
    uint64_t starttime = mecm_current_time_us();
    uint64_t stoptime  = starttime + timeout;
    do
    {
        wck = mecm_data_send((uint8_t *)&(mecm_port.mecm_sendbuf[idx]),length);
        if(wck == 1)
        {
            wck = mecm_data_recv((uint8_t *)&(mecm_port.mecm_recvbuf[idx]),MECM_MAXDATAGRAM,1000);
            /* 读取成功 */
            if(wck == 1)
            {
                /* 判断idx 和 包头是否是需要的ethercat数据包 */
                if((mecm_port.mecm_recvbuf[idx].en_header.etype == htons(0x88A4))&&(mecm_port.mecm_recvbuf[idx].datagram[1] == idx))
                {
                    wck = 1;
                }else{
                    wck = 0;
                }
                
            }
        }
    } while ((wck == 0)&&(mecm_current_time_us()<stoptime));
    return wck;
}
/**
 * @******************************************************************************: 
 * @func: [get_idx]
 * @description: 获取当前的索引值，预留功能，目前程序简单还没有用到，暂时固定返回0；
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
uint8_t get_idx(void)
{
    static uint16_t idx = 0;
    idx++;
    if(idx>=MAXBUF)
        idx = 0;
    return idx;
}
/**
 * @******************************************************************************: 
 * @func: [mecm_clear_port]
 * @description: 清空使用的port接口的内容
 * @note: 
 * @author: gxf
 * @param [uint8_t] idx
 * @return [*]
 * @==============================================================================: 
 */
void mecm_clear_port(uint8_t idx)
{
    /* 发送数据帧内容清空 */
    memset(mecm_port.mecm_sendbuf[idx].datagram,0,MECM_MAXDATAGRAM);
    /* 接收数据帧内容清空 */
    memset(mecm_port.mecm_recvbuf[idx].datagram,0,MECM_MAXDATAGRAM);
    /* 数据帧中datagram的个数清0 */
    mecm_port.datagrams[idx] = 0;
    /* 数据帧中datagram的信息清0 */
    memset(mecm_port.datagram_info[idx].datagram_length,0,MAXDATAGRAMS);
    memset(mecm_port.datagram_info[idx].datagram_offset,0,MAXDATAGRAMS);
}
/**
 * @******************************************************************************: 
 * @func: [mecm_BRD]
 * @description: 广播读取,所有从站数据逻辑或放入数据报文
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的地址 一般填0
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_BRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_BRD,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_BWR]
 * @description: 广播写入
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的地址 一般填0
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_BWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_BWR,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_APRD]
 * @description: 自动递增读取
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的地址 ,每经过一个从站会+1
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_APRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_APRD,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;    
}

/**
 * @******************************************************************************: 
 * @func: [mecm_ARMW]
 * @description:自动递增多次读写 
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的地址 ,每经过一个从站会+1
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_ARMW(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_ARMW,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;    
}

/**
 * @******************************************************************************: 
 * @func: [mecm_APWR]
 * @description:自动递增写入
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的地址 ,每经过一个从站会+1
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_APWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_APWR,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;      
}

/**
 * @******************************************************************************: 
 * @func: [mecm_FRMW]
 * @description:配置地址多次读写
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的配置地址或从站别名
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_FRMW(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_FRMW,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;      
}

/**
 * @******************************************************************************: 
 * @func: [mecm_FPRD]
 * @description:配置地址读取
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的配置地址或从站别名
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_FPRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_FPRD,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;      
}

/**
 * @******************************************************************************: 
 * @func: [mecm_FPWR]
 * @description:配置地址写入
 * @note: 
 * @author: gxf
 * @param [uint16_t] adp 从站的配置地址或从站别名
 * @param [uint16_t] ado 从站的内存
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_FPWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_np(CMD_FPWR,idx,16,adp,ado,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;  
}

/**
 * @******************************************************************************: 
 * @func: [mecm_LRW]
 * @description: 逻辑地址读写
 * @note: 
 * @author: gxf
 * @param [uint32_t] addr 逻辑地址
 * @param [uint8_t] *data 读写数据存放的地址
 * @param [uint16_t] length 读写数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_LRW(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_logi(CMD_LRW,idx,16,addr,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;  
}
/**
 * @******************************************************************************: 
 * @func: [mecm_FPWR]
 * @description:配置地址写入
 * @note: 
 * @author: gxf
 * @param [uint32_t] addr 逻辑地址
 * @param [uint8_t] *data 写入数据存放的地址
 * @param [uint16_t] length 写入数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_LWR(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_logi(CMD_LWR,idx,16,addr,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;  
}
/**
 * @******************************************************************************: 
 * @func: [mecm_FPWR]
 * @description:配置地址写入
 * @note: 
 * @author: gxf
 * @param [uint32_t] addr 逻辑地址
 * @param [uint8_t] *data 读取数据存放的地址
 * @param [uint16_t] length 读取数据的长度
 * @param [uint32_t] timeout 超时时间，单位us
 * @return [int]     wck<0 没有收到数据帧 
 * @==============================================================================: 
 */
int mecm_LRD(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout)
{
    int wck;
    uint8_t idx;
    uint16_t framlength = 0;
    idx = get_idx();
    mecm_port.sendindex = idx;
    /* 数据组包 */
    framlength = mecm_setup_datagram_logi(CMD_LRD,idx,16,addr,data,length,0);
    /* 数据发送与接收 */
    wck = mecm_communicate(idx,(uint8_t *)&(mecm_port.mecm_sendbuf[idx]),framlength,timeout);
    if(wck == 1)
    {
        /* 获取数据 */
        wck = (mecm_port.mecm_recvbuf[idx].datagram[framlength-16-1]<<8)|mecm_port.mecm_recvbuf[idx].datagram[framlength-16-2];
        memcpy(data,&(mecm_port.mecm_recvbuf[idx].datagram[10]),framlength-16-10-2);
    }else{
        wck = -1;
    }
    mecm_clear_port(idx);
    return wck;      
}




