/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master COE 模块 用于实现COE协议通信
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:26
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 15:11:03
 * @==============================================================================: 
 */

#include <string.h>
#include "mecm_coe.h"
#include "mecm_type.h"
#include "mecm_config.h"
#include "mecm_mailbox.h"

static _pdo_assign_t pdo_assign;
static _pdo_map_t pdo_map;

/**
 * @******************************************************************************: 
 * @func: [sdo_write_exp]
 * @description: SDO写 快速传输
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave  从站编号
 * @param [uint16_t] index  索引
 * @param [uint8_t] subindex 子索引
 * @param [uint8_t] *data  待写入的数据
 * @param [uint32_t] datalen 数据长度
 * @param [int] timeout 超时时间
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int sdo_write_exp(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,int timeout)
{
    _sdo_data_t senddata;
    _sdo_data_t recvdata;
    uint16_t address = 0;
    int wkc = 0;
    memset(&senddata,0,sizeof(_sdo_data_t));
    memset(&recvdata,0,sizeof(_sdo_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据长度 快速传输10字节，其他传输 10+x 字节 */
    senddata.mbxheader.length   = htoes(0x000A);
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_COE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;
    /* PDO编号在PDO通信的时候才会用 */
    senddata.coe.cmd.pdonumber  = 0x0000;
    senddata.coe.cmd.reserved   = 0x0000;
    senddata.coe.cmd.opcode     = COE_SDOREQ;
    /* 大小端转换 */
    senddata.coe.data           = htoes(senddata.coe.data);
    senddata.cs                 = SDO_DOWN_EXP|(((4-datalen)<<2)&0x0C);
    senddata.index              = htoes(index);
    senddata.subindex           = subindex;
    memcpy(senddata.data1,data,datalen);
    /* 发送邮箱数据 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    /* 发送成功，再去读取返回信息 */
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_sdo_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            /* 判断回复信息 */
            if((recvdata.mbxheader.type == TYPE_MBX_COE)
            &&(recvdata.coe.cmd.opcode == COE_SDORES)
            &&(recvdata.cs != SDO_ABORT)
            &&(etohs(recvdata.index)==index)
            &&(recvdata.subindex == subindex))
            {
                /* */
            }else
            {
                /* 回复信息不正确 */
                if(recvdata.cs == SDO_ABORT )
                {
                    /* 可以打印异常信息 */                   
                }
                wkc = 0;
            }            
        }
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [sdo_write_exp]
 * @description: SDO写 常规传输
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave  从站编号
 * @param [uint16_t] index  索引
 * @param [uint8_t] subindex 子索引
 * @param [uint8_t] *data  待写入的数据
 * @param [uint32_t] datalen 数据长度
 * @param [uint8_t] complete_access  1:使用完全传输 0：不使用
 * @param [int] timeout 超时时间
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int sdo_write_normal(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout)
{
    _sdo_data_t senddata;
    _sdo_data_t recvdata;
    uint16_t address = 0;
    int wkc = 0;
    memset(&senddata,0,sizeof(_sdo_data_t));
    memset(&recvdata,0,sizeof(_sdo_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据长度 快速传输10字节，其他传输 10+x 字节 */
    senddata.mbxheader.length = htoes(0x000A+datalen);
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_COE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;
    /* PDO编号在PDO通信的时候才会用 */
    senddata.coe.cmd.pdonumber  = 0x0000;
    senddata.coe.cmd.reserved   = 0x0000;
    senddata.coe.cmd.opcode     = COE_SDOREQ;
    /* 大小端转换 */
    senddata.coe.data           = htoes(senddata.coe.data);    
    senddata.index              = htoes(index);
    senddata.subindex           = subindex;
    /* 是否时完全传输 */
    if(complete_access)
    {
        senddata.cs = SDO_DOWN_NORMAL_CA;
        /* 如果是完全传输则子索引为0或1 */
        if(senddata.subindex>1) 
        {
            senddata.subindex = 1;
        }
            
    }      
    else
    {
        senddata.cs = SDO_DOWN_NORMAL;        
    }        
    /* 前四个数据字节填充长度 */
    senddata.data1[0] = datalen&0xFF;
    senddata.data1[1] = (datalen>>8)&0xFF;
    senddata.data1[2] = (datalen>>16)&0xFF;
    senddata.data1[3] = (datalen>>24)&0xFF;
    memcpy(&senddata.data1[4],data,datalen);
    /* 发送邮箱数据 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
   /* 发送成功，再去读取返回信息 */
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_sdo_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            /* 判断回复信息 */
            if((recvdata.mbxheader.type == TYPE_MBX_COE)
            &&(recvdata.coe.cmd.opcode == COE_SDORES)
            &&(recvdata.cs != SDO_ABORT)
            &&(etohs(recvdata.index)==index)
            &&(recvdata.subindex == subindex))
            {
                /* */
            }else
            {
                /* 回复信息不正确 */
                if(recvdata.cs == SDO_ABORT )
                {
                    /* 可以打印异常信息 */                   
                }
                wkc = 0;
            }            
        }
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [sdo_write_exp]
 * @description: SDO写 分段传输
 * @note: 分段传输的第一包数据还是先采用常规传输
 * @author: gxf
 * @param [uint16_t] slave  从站编号
 * @param [uint16_t] index  索引
 * @param [uint8_t] subindex 子索引
 * @param [uint8_t] *data  待写入的数据
 * @param [uint32_t] datalen 数据长度
 * @param [uint8_t] complete_access  1:使用完全传输 0：不使用
 * @param [int] timeout 超时时间
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int sdo_write_segment(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout)
{
    _sdo_data_t senddata;
    _sdo_data_t recvdata;
    uint16_t address = 0;
    int wkc = 0;
    uint8_t  toggle = 0;
    uint32_t sendsize = 0;
    uint32_t sendoffset = 0;
    /* 一次能够传输的最大数据长度 = 邮箱的长度-6byte邮箱头-2byte COE头-8byte SDO 数据 */
    uint16_t maxlen = mecm_slave[slave].standard_wl-0x10;
    sendsize = maxlen;
    memset(&senddata,0,sizeof(_sdo_data_t));
    memset(&recvdata,0,sizeof(_sdo_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据长度 第一包使用常规传输，后续包使用分段传输 */
    senddata.mbxheader.length = htoes(0x000A+sendsize);
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_COE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;
    /* PDO编号在PDO通信的时候才会用 */
    senddata.coe.cmd.pdonumber  = 0x0000;
    senddata.coe.cmd.reserved   = 0x0000;
    senddata.coe.cmd.opcode     = COE_SDOREQ;
    /* 大小端转换 */
    senddata.coe.data           = htoes(senddata.coe.data);
    senddata.index              = htoes(index);
    senddata.subindex           = subindex;
    /* 是否时完全传输 */
    if(complete_access)
    {
        senddata.cs = SDO_DOWN_NORMAL_CA;
        /* 如果是完全传输则子索引为0或1 */
        if(senddata.subindex>1)
        {
            senddata.subindex = 1;
        } 
            
    }      
    else
    {
        senddata.cs = SDO_DOWN_NORMAL;        
    }        
    /* 前四个数据字节填充长度 */
    senddata.data1[0] = datalen&0xFF;
    senddata.data1[1] = (datalen>>8)&0xFF;
    senddata.data1[2] = (datalen>>16)&0xFF;
    senddata.data1[3] = (datalen>>24)&0xFF;
    memcpy(&senddata.data1[4],data,sendsize);
    /* 发送邮箱数据 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    /* 计算剩余数据的地址和长度 */
    sendoffset = sendoffset+sendsize;
    datalen = datalen - sendsize;
   /* 发送成功，再去读取返回信息 */
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_sdo_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            /* 判断回复信息 */
            if((recvdata.mbxheader.type == TYPE_MBX_COE)
            &&(recvdata.coe.cmd.opcode == COE_SDORES)
            &&(recvdata.cs != SDO_ABORT)
            &&(etohs(recvdata.index)==index)
            &&(recvdata.subindex == subindex))
            {
                /* 分段传输不需要再发送索引、子索引、数据长度,全部填充数据，所以多了7个字节 */
                maxlen = maxlen+7;
                /* 响应正确，开始进行后续的分段传输 */
                while(1)
                {
                    memset(&senddata,0,sizeof(_sdo_data_t));
                    /* 计算传输长度 */
                    sendsize = datalen;
                    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0，这里填写了从站别名，后续可以用来对数据进行判断 */
                    senddata.mbxheader.address  = htoes(address);
                    senddata.mbxheader.channel  = 0x00;
                    senddata.mbxheader.priority = 0x00;
                    senddata.mbxheader.type     = TYPE_MBX_COE;
                    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
                    senddata.mbxheader.reserved = 0x00;
                    /* PDO编号在PDO通信的时候才会用 */
                    senddata.coe.cmd.pdonumber  = 0x0000;
                    senddata.coe.cmd.reserved   = 0x0000;
                    senddata.coe.cmd.opcode     = COE_SDOREQ;
                        /* 大小端转换 */
                    senddata.coe.data           = htoes(senddata.coe.data);
                    if(sendsize>7)
                    {
                        /* 不是最后一包 */
                        if(sendsize>maxlen)
                        {
                            sendsize = maxlen;
                            senddata.cs = 0x00;
                        }else
                        {
                            senddata.cs = 0x01;
                        }
                        /* sendsize+2byte COE +1byte CS */
                        senddata.mbxheader.length = htoes(sendsize+3);
                    }else
                    {
                        senddata.cs = 0x01|((7-sendsize)<<1);
                    }
                    senddata.cs = senddata.cs|toggle;
                    memcpy((uint8_t *)&senddata.index,data+sendoffset,sendsize);
                    /* 发送邮箱数据 */
                    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout); 
                    sendoffset = sendoffset+sendsize;
                    datalen = datalen - sendsize; 
                    if(wkc>0)
                    {
                        memset(&recvdata,0,sizeof(_sdo_data_t));
                        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
                        if(wkc>0) 
                        {
                            if((recvdata.mbxheader.type == TYPE_MBX_COE)
                            &&(recvdata.coe.cmd.opcode == COE_SDORES)
                            &&((recvdata.cs&0xe0)== 0x20))
                            {
                                /* 数据已经发送完成了，退出循环*/
                                if(datalen == 0) 
                                    break;
                            }else
                            {
                                /* 通讯失败则直接退出传输 */
                                wkc = 0;
                                break;
                            }
                                                            
                        }else
                        {
                            /* 通讯失败则直接退出传输 */
                            break;
                        }
                                               
                    }else
                    {
                        /* 通讯失败则直接退出传输 */
                        break;
                    }
                    /* 翻转位 */      
                    toggle = toggle^0x10;            
                }
            }else
            {
                /* 回复信息不正确 */
                if(recvdata.cs == SDO_ABORT )
                {
                    /* 可以打印异常信息 */                   
                }
                wkc = 0;
            }            
        }
    }
    return wkc;
}
/**
 * @******************************************************************************: 
 * @func: [sdo_write]
 * @description: SDO 写
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint16_t] index 索引
 * @param [uint8_t] subindex 子索引
 * @param [uint8_t] *data 待写入数据
 * @param [uint32_t] datalen 待写入数据长度
 * @param [uint8_t] complete_access 是否完全传输
 * @param [int] timeout 超时时间
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int sdo_write(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t datalen,uint8_t complete_access,int timeout)
{
    uint16_t maxlen = mecm_slave[slave].standard_wl-0x10;
    int wkc = 0;
    /* 快速传输 */
    if(datalen<=4)
    {
        wkc = sdo_write_exp(slave,index,subindex,data,datalen,timeout);
    }else if(datalen>maxlen)
    {
        /* 分段传输 */ 
        wkc = sdo_write_normal(slave,index,subindex,data,datalen,complete_access,timeout);
    }else
    {
        wkc = sdo_write_segment(slave,index,subindex,data,datalen,complete_access,timeout);
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [sdo_read]
 * @description: SDO读
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint16_t] index 索引
 * @param [uint8_t] subindex 子索引
 * @param [uint8_t] *data 读取数据存放的位置
 * @param [uint32_t] *datalen 读取的数据的长度
 * @param [uint8_t] complete_access 是否完全传输
 * @param [int] timeout 超时时间
 * @return [*] <=0:读取失败
 * @==============================================================================: 
 */
int sdo_read(uint16_t slave,uint16_t index,uint8_t subindex,uint8_t *data,uint32_t *datalen,uint8_t complete_access,int timeout)
{
    _sdo_data_t senddata;
    _sdo_data_t recvdata;
    uint16_t address = 0;
    int wkc = 0;
    uint32_t recvsize = 0;
    uint32_t totalsize = 0;
    uint32_t recvoffset = 0;
    uint8_t  toggle = 0;
    memset(&senddata,0,sizeof(_sdo_data_t));
    memset(&recvdata,0,sizeof(_sdo_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据长度 快速传输10字节，其他传输 10+x 字节 */
    senddata.mbxheader.length = htoes(0x000A);
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_COE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;
    /* PDO编号在PDO通信的时候才会用 */
    senddata.coe.cmd.pdonumber  = 0x0000;
    senddata.coe.cmd.reserved   = 0x0000;
    senddata.coe.cmd.opcode     = COE_SDOREQ;
    /* 大小端转换 */
    senddata.coe.data           = htoes(senddata.coe.data);
    senddata.index              = htoes(index);
    senddata.subindex           = subindex;
    /* 是否完全传输 */
    if(complete_access == 0)
    {
        senddata.cs = SDO_UP_REQ;
    }else
    {
        senddata.cs = SDO_UP_REQ_CA;
        /* 如果是完全传输则子索引为0或1 */
        if(senddata.subindex>1)
        {
            senddata.subindex = 1;
        } 
            
    }
    
    /* 发送邮箱数据 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    /* 发送成功，再去读取返回信息 */
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_sdo_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            /* 判断回复信息 */
            if((recvdata.mbxheader.type == TYPE_MBX_COE)
            &&(recvdata.coe.cmd.opcode == COE_SDORES)
            &&(etohs(recvdata.index)==index)
            &&(recvdata.subindex == subindex))
            {
                /* 快速传输 */
                if((recvdata.cs&0x02)>0)
                {
                    /* 获取数据和长度 */
                    recvsize = 4-((recvdata.cs&0x0C)>>2);
                    memcpy(data,recvdata.data1,recvsize);
                    *datalen = recvsize;
                }else
                {
                    totalsize = (recvdata.data1[0])|(recvdata.data1[1]<<8)|(recvdata.data1[2]<<16)|(recvdata.data1[3]<<24);
                    recvsize  = etohs(recvdata.mbxheader.length)-10;
                    /* 需要进行分段传输 */
                    if(totalsize>recvsize)
                    {
                        /* 先保存当前读到的数据，然后开始分段传输 */
                        memcpy(data,(uint8_t *)&recvdata.data1[4],recvsize);
                        recvoffset = recvoffset+recvsize;
                        *datalen = recvoffset;
                        toggle = 0x00;
                        while(1)
                        {
                            memset(&senddata,0,sizeof(_sdo_data_t));
                            senddata.mbxheader.length = htoes(0x000A);
                            /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0，这里填写了从站别名，后续可以用来对数据进行判断 */
                            senddata.mbxheader.address  = htoes(address);
                            senddata.mbxheader.channel  = 0x00;
                            senddata.mbxheader.priority = 0x00;
                            senddata.mbxheader.type     = TYPE_MBX_COE;
                            senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
                            senddata.mbxheader.reserved = 0x00;
                            /* PDO编号在PDO通信的时候才会用 */
                            senddata.coe.cmd.pdonumber  = 0x0000;
                            senddata.coe.cmd.reserved   = 0x0000;
                            senddata.coe.cmd.opcode     = COE_SDOREQ;
                            /* 大小端转换 */
                            senddata.coe.data           = htoes(senddata.coe.data);
                            senddata.cs                 = SDO_SEG_UP_REQ+toggle;
                            senddata.index              = htoes(index);
                            senddata.subindex           = subindex;
                            /* 发送数据 */
                            wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
                            if(wkc>0)
                            {
                                /* 接收数据 */
                                memset(&recvdata,0,sizeof(_sdo_data_t));
                                wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
                                if(wkc>0)
                                {
                                    if((recvdata.mbxheader.type == TYPE_MBX_COE)
                                    &&(recvdata.coe.cmd.opcode == COE_SDORES)
                                    &&(etohs(recvdata.index)==index)
                                    &&(recvdata.subindex == subindex))
                                    {   
                                        /* 分段传输的时候，接收的数据没有索引和子索引，所以时减3 2byte COE +1byte CS*/
                                        recvsize  = etohs(recvdata.mbxheader.length)-3;     
                                        /* 最后一包数据 */
                                        if((recvdata.cs&0x01)== 0x01)
                                        {
                                            /* 最后一包中的有效数据不大于7个字节 */
                                            if(recvsize == 7)
                                            {
                                                recvsize = (recvdata.cs&0x0E)>>1;
                                            }
                                            memcpy(data+recvoffset,(uint8_t *)&recvdata.index,recvsize);
                                            recvoffset = recvoffset+recvsize;
                                            *datalen = recvoffset;
                                        }else
                                        {
                                            memcpy(data+recvoffset,(uint8_t *)&recvdata.index,recvsize);
                                            recvoffset = recvoffset+recvsize;
                                            *datalen = recvoffset;
                                        }
                                    }else
                                    {
                                        /* 接收数据验证不通过，退出循环 */
                                        wkc = 0;
                                        break;
                                    }
                                    
                                }else
                                {
                                    /* 退出循环 */
                                    break;
                                }
                                
                            }else
                            {
                                /* 退出循环 */
                                break;
                            }
                            toggle = toggle^0x10;    
                        }
                        
                    }else
                    {
                        /* 不需要分段传输 */
                        memcpy(data,&recvdata.data1[4],recvsize);
                        *datalen = recvsize;
                    }
                    
                }
            }else
            {
                /* 回复信息不正确 */
                if(recvdata.cs == SDO_ABORT )
                {
                    /* 可以打印异常信息 */                   
                }
                wkc = 0;
            }            
        }
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [get_pdo_map]
 * @description: 获取PDO的映射长度
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint16_t] index 一般默认0x1C12中的是RPDO，0x1C13是TPDO
 * @return [*]
 * @==============================================================================: 
 */
uint32_t get_pdo_map(uint16_t slave, uint16_t index)
{
    int wkc = 0;
    uint32_t size = 0;
    uint32_t len = 0;
    uint16_t n_index = 0;
    uint16_t i = 0,j=0;
    uint16_t mapindex = 0;
    uint32_t n_mapindex = 0;
    memset((uint8_t *)&pdo_assign,0,sizeof(_pdo_assign_t));
    memset((uint8_t *)&pdo_map,0,sizeof(_pdo_map_t));
    /* 通过完全传输来读取0x1C12或者0x1C13对象字典中的所有数据 */
    wkc = sdo_read(slave,index,0,(uint8_t *)&pdo_assign,&len,1,50000);
    if(wkc<=0)
    {
        return size;
    }
    /* 获取0x1C12后者0x1C13中的对象字典个数 */
    n_index = etohs(pdo_assign.n);
    if(n_index == 0)
    {
        return size;
    }
    for(i = 0;i<n_index;i++)
    {
        mapindex = etohs(pdo_assign.index[i]);
        wkc = sdo_read(slave,mapindex,0,(uint8_t *)&pdo_map,&len,1,50000);
        if(wkc<=0)
        {
            return size;
        }           
        /* 获取TPDO或RPDO的映射个数 */
        n_mapindex = etohs(pdo_map.n);
        if (n_mapindex == 0)
        {
            return size;
        }       
        for(j=0;j<n_mapindex;j++)
        {
            /* 根据映射内容计算映射长度 单位bit */
            size = size+(etohl(pdo_map.index[j])&0xFF);
        }
    }

    return size;
    
}
