/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master FOE模块
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:45
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 18:34:31
 * @==============================================================================: 
 */

#include "mecm_foe.h"
#include "mecm_config.h"
#include "mecm_base.h"
#include "mecm_type.h"
#include "mecm_mailbox.h"


static char *mecm_foe_filename = NULL;    /* 请求的文件名字 */
static uint32_t mecm_foe_password; /* 请求密钥 */
static uint32_t mecm_foe_filesize; /* 文件大小，如果正在读请求，则表示已读文件大小，如果是写请求，则表示剩余文件大小 */
static uint8_t  mecm_foe_buf[FOE_BUF_MAX_BYTE]; /* 读写缓存 */
static int32_t  mecm_foe_packetno = 0;
static int32_t  mecm_foe_errcode = 0;    /* 错误码 */
static int32_t  mecm_foe_retry_len = 0;
static uint8_t  mecm_foe_task_state = FOE_TASK_STATE_STOP;
static uint8_t  mecm_foe_ack_last_flag = 0;


static int mecm_foe_read(uint16_t slave,int timeout);
static int mecm_foe_write(uint16_t slave,int timeout);
static int mecm_foe_data(uint16_t slave,int timeout);
static int mecm_foe_ack(uint16_t slave,int timeout);
static int mecm_foe_err(uint16_t slave,int timeout);
static int mecm_foe_retry(uint16_t slave,int timeout);
/**
 * @******************************************************************************: 
 * @func: [mecm_foe_start]
 * @description: 启动FOE配置
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_foe_start(uint16_t slave)
{
    int wkc = 0;
    int i = 0;
    
    mecm_sm_reg_t smconfig;
    /* 从机状态切换->Init->Bootstrap*/
    wkc = mecm_state_change(slave,ESC_STATE_INIT);
    do
    {
        wkc =  mecm_state_read(slave);
    } while ((mecm_slave[slave].state != ESC_STATE_INIT)&&(i<3));
    /* 状态切换失败 */
    if(mecm_slave[slave].state != ESC_STATE_INIT)
    {
        return 0;
    }
    /* 读取EEPROM信息 */
    mecm_get_eeprom_info(slave);
    /* 根据eeprom的信息配置FOE邮箱 */
    for(i=0;i<mecm_slave[slave].nSM;i++)
    {
        /* 配置写邮箱*/
        if(mecm_slave[slave].rSMtype[i] == 1)
        {            
            memset((uint8_t *)&smconfig,0,sizeof(mecm_sm_reg_t));
            smconfig.sm_start_address = mecm_slave[slave].bootstrap_wo;
            smconfig.sm_length        = mecm_slave[slave].bootstrap_wl;
            smconfig.sm_ctr.reg_byte  = mecm_slave[slave].rSM[i].sm_ctr.reg_byte; 
            smconfig.sm_activate.reg_bit.enable = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
            wkc = mecm_sm_config(slave,i);
            if(wkc <=0 )
            {
                return wkc;
            }                
        } 
        /* 配置读邮箱*/
        if(mecm_slave[slave].rSMtype[i] == 2)
        {
            memset((uint8_t *)&smconfig,0,sizeof(mecm_sm_reg_t));
            smconfig.sm_start_address = mecm_slave[slave].bootstrap_ro;
            smconfig.sm_length        = mecm_slave[slave].bootstrap_rl;
            smconfig.sm_ctr.reg_byte  = mecm_slave[slave].rSM[i].sm_ctr.reg_byte; 
            smconfig.sm_activate.reg_bit.enable = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
            wkc = mecm_sm_config(slave,i);
            if(wkc <=0 )
            {
                return wkc;
            }                
        }           
    } 
    /* 进入bootstrap*/
    mecm_state_change(slave,ESC_STATE_BOOT);
    do
    {
        wkc =  mecm_state_read(slave);
    } while ((mecm_slave[slave].state != ESC_STATE_BOOT)&&(i<3));
    /* 状态切换失败 */
    if(mecm_slave[slave].state != ESC_STATE_BOOT)
    {
        return 0;
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_task]
 * @description: FOE 任务
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
void mecm_foe_task(uint16_t slave,int timeout)
{
    switch (mecm_foe_task_state)
    {
    case FOE_TASK_STATE_STOP:
        mecm_foe_stop();
        break;
    case FOE_TASK_STATE_EXCEPT:
        mecm_foe_except();
        break;
    case FOE_TASK_STATE_READ:
        mecm_foe_task_state = mecm_foe_read(slave,timeout);
        break;
    case FOE_TASK_STATE_WRITE:
        mecm_foe_task_state = mecm_foe_write(slave,timeout);
        break;
    case FOE_TASK_STATE_DATA:
        mecm_foe_task_state = mecm_foe_data(slave,timeout);
        break;
    case FOE_TASK_STATE_ACK:
        mecm_foe_task_state = mecm_foe_ack(slave,timeout);
        break;    
    case FOE_TASK_STATE_ERR:
        mecm_foe_task_state = mecm_foe_read(slave,timeout);
        break;
    case FOE_TASK_STATE_RETRY:
        mecm_foe_task_state = mecm_foe_retry(slave,timeout);
        break;
    default:
        break;
    }
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_set_info]
 * @description: 设置FOE任务的初始状态
 * @note: 
 * @author: gxf
 * @param [char] *filename 文件名称
 * @param [uint32_t] pwd 密钥
 * @param [uint32_t] filesize 文件大小
 * @param [uint8_t] state 状态机状态
 * @return [*]
 * @==============================================================================: 
 */
void mecm_foe_set_info(char *filename,uint32_t pwd,uint32_t filesize,uint8_t state)
{
    mecm_foe_filename   = filename;
    mecm_foe_password   = pwd;
    mecm_foe_filesize   = filesize;
    mecm_foe_task_state = state;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_read]
 * @description: FOE 读请求
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_read(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t datalen = 0;
    uint16_t maxsize = 0;
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    mecm_foe_packetno = 0;
    mecm_foe_errcode = 0;
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据 */
    if(mecm_foe_filename == NULL)
    {
        return 0;
    }
    size = (uint16_t)strlen(mecm_foe_filename);
    maxsize = mecm_slave[slave].standard_wl-12;
    /* 数据的最大长度为 邮箱长度-邮箱头6byte-foe头 2byte-password 4byte*/
    if(size>=maxsize)
    {
        size = maxsize;
    }
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006+size); 
    /* foe 内容填充 */
    senddata.opcode = FOE_READ;
    senddata.reserved = 0;
    senddata.foe.password = mecm_foe_password;
    memcpy(senddata.data,mecm_foe_filename,size);
    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_foe_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            if(recvdata.mbxheader.type == TYPE_MBX_FOE)
            {
                switch (recvdata.opcode)
                {
                case FOE_DATA:
                    /* 判断数据包的编号 */
                    if(recvdata.foe.packetno == ++mecm_foe_packetno)
                    {
                        datalen = recvdata.mbxheader.length-6;
                        mecm_foe_filesize = mecm_foe_filesize+datalen;
                        memcpy(mecm_foe_buf,recvdata.data,datalen);
                        mecm_foe_read_data_handle(mecm_foe_buf,mecm_foe_filesize);
                        /* 最后一包 */
                        if(datalen<maxsize)
                        {
                            mecm_foe_ack_last_flag = 1;
                        }
                        return FOE_TASK_STATE_ACK;
                    }else
                    {
                        mecm_foe_errcode = FOE_ERRCODE_PACKENO;
                        return FOE_TASK_STATE_ERR;
                    }
                                       
                    break;
                case FOE_ERR:
                    mecm_foe_err_handle(recvdata.foe.errcode);
                    return FOE_TASK_STATE_EXCEPT;
                    break;
                default:
                    break;
                }
            }else
            {
                return FOE_TASK_STATE_EXCEPT;
            }
            
        }else
        {
            return FOE_TASK_STATE_EXCEPT;
        }
        
    }
    return FOE_TASK_STATE_EXCEPT;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_write]
 * @description: FOE写请求
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_write(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t maxsize = 0;
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    mecm_foe_packetno = 0;
    mecm_foe_errcode = 0;
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充数据 */
    if(mecm_foe_filename == NULL)
    {
        return 0;
    }
    size = (uint16_t)strlen(mecm_foe_filename);
    maxsize = mecm_slave[slave].standard_wl-12;
    /* 数据的最大长度为 邮箱长度-邮箱头6byte-foe头 2byte-password 4byte*/
    if(size>=maxsize)
    {
        size = maxsize;
    }
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006+size); 
    /* foe 内容填充 */
    senddata.opcode = FOE_WRITE;
    senddata.reserved = 0;
    senddata.foe.password = mecm_foe_password;
    memcpy(senddata.data,mecm_foe_filename,size);
    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_foe_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            if(recvdata.mbxheader.type == TYPE_MBX_FOE)
            {
                switch (recvdata.opcode)
                {
                case FOE_ACK:
                    /* 判断数据包的编号 */
                    if(recvdata.foe.packetno == mecm_foe_packetno)
                    {
                        mecm_foe_write_data_handle(mecm_foe_buf,mecm_foe_filesize);
                        return FOE_TASK_STATE_DATA;
                    }else
                    {
                        mecm_foe_errcode = FOE_ERRCODE_PACKENO;
                        return FOE_TASK_STATE_ERR;
                    }                  
                    break;
                case FOE_ERR:
                    mecm_foe_err_handle(recvdata.foe.errcode);
                    return FOE_TASK_STATE_EXCEPT;
                    break;
                default:
                    break;
                }
            }else
            {
                return FOE_TASK_STATE_EXCEPT;
            }
            
        }else
        {
            return FOE_TASK_STATE_EXCEPT;
        }
        
    }
    return FOE_TASK_STATE_EXCEPT; 
}


/**
 * @******************************************************************************: 
 * @func: [mecm_foe_data]
 * @description: FOE数据帧
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_data(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t datalen = 0;
    uint16_t maxsize = 0;
    uint8_t  last_packet_flag = 0;    /* 最后一包标志 */
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    size = mecm_foe_filesize;
    maxsize = mecm_slave[slave].standard_wl-12;
    /* 数据的最大长度为 邮箱长度-邮箱头6byte-foe头 2byte-password 4byte*/
    if(size>=maxsize)
    {
        size = maxsize;
    }else
    {
        /* 最后一包数据 */
        last_packet_flag = 1;
    }
    mecm_foe_retry_len = size;
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006+size); 
    /* foe 内容填充 */
    senddata.opcode = FOE_DATA;
    senddata.reserved = 0;
    senddata.foe.packetno = ++mecm_foe_packetno;
    memcpy(senddata.data,mecm_foe_buf,size);
    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    if(wkc>0)
    {
        mecm_foe_filesize-=size;
        memset(&recvdata,0,sizeof(_foe_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            if(recvdata.mbxheader.type == TYPE_MBX_FOE)
            {
                switch (recvdata.opcode)
                {
                case FOE_ACK:
                    /* 判断数据包的编号 */
                    if(recvdata.foe.packetno == mecm_foe_packetno)
                    {
                        if(last_packet_flag ==1)
                        {
                            return FOE_TASK_STATE_STOP;
                        }
                        mecm_foe_write_data_handle(mecm_foe_buf,mecm_foe_filesize);
                        return FOE_TASK_STATE_DATA;
                    }else
                    {
                        mecm_foe_errcode = FOE_ERRCODE_PACKENO;
                        return FOE_TASK_STATE_ERR;
                    }                  
                    break;
                case FOE_ERR:
                    mecm_foe_err_handle(recvdata.foe.errcode);
                    return FOE_TASK_STATE_EXCEPT;
                    break;
                case FOE_BUSY:
                    mecm_foe_busy_handle(recvdata.foe.busy.done,recvdata.foe.busy.entire);
                    return FOE_TASK_STATE_RETRY;
                    break;
                default:
                    break;
                }
            }else
            {
                return FOE_TASK_STATE_EXCEPT;
            }
            
        }else
        {
            return FOE_TASK_STATE_EXCEPT;
        }
        
    }
    return FOE_TASK_STATE_EXCEPT;     
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_ack]
 * @description: FOE应答帧 
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_ack(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t datalen = 0;
    uint16_t maxsize = 0;
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    maxsize = mecm_slave[slave].standard_wl-12;
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006); 
    /* foe 内容填充 */
    senddata.opcode = FOE_ACK;
    senddata.reserved = 0;
    senddata.foe.packetno = mecm_foe_packetno;

    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    /* 最后一次应答，发完直接停止 */
    if(mecm_foe_ack_last_flag == 1)
    {
        return FOE_TASK_STATE_STOP;
    }
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_foe_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            if(recvdata.mbxheader.type == TYPE_MBX_FOE)
            {
                switch (recvdata.opcode)
                {
                case FOE_DATA:
                    /* 判断数据包的编号 */
                    if(recvdata.foe.packetno == ++mecm_foe_packetno)
                    {
                        datalen = recvdata.mbxheader.length-6;
                        mecm_foe_filesize = mecm_foe_filesize+datalen;
                        memcpy(mecm_foe_buf,recvdata.data,datalen);
                        mecm_foe_read_data_handle(mecm_foe_buf,mecm_foe_filesize);
                        /* 最后一包 */
                        if(datalen<maxsize)
                        {
                            mecm_foe_ack_last_flag = 1;
                        }
                        return FOE_TASK_STATE_ACK;
                    }else
                    {
                        mecm_foe_errcode = FOE_ERRCODE_PACKENO;
                        return FOE_TASK_STATE_ERR;
                    }
                                       
                    break;
                case FOE_ERR:
                    mecm_foe_err_handle(recvdata.foe.errcode);
                    return FOE_TASK_STATE_EXCEPT;
                    break;
                default:
                    break;
                }
            }else
            {
                return FOE_TASK_STATE_EXCEPT;
            }
            
        }else
        {
            return FOE_TASK_STATE_EXCEPT;
        }
        
    }
    return FOE_TASK_STATE_EXCEPT;     
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_err]
 * @description: FOE 错误帧
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_err(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t datalen = 0;
    uint16_t maxsize = 0;
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    maxsize = mecm_slave[slave].standard_wl-12;
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006); 
    /* foe 内容填充 */
    senddata.opcode = FOE_ERR;
    senddata.reserved = 0;
    senddata.foe.errcode = mecm_foe_errcode;

    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    return FOE_TASK_STATE_EXCEPT;     
}

/**
 * @******************************************************************************: 
 * @func: [mecm_foe_retry]
 * @description: FOE Busy重发
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [int] timeout 超时时间
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_foe_retry(uint16_t slave,int timeout)
{
    uint16_t address = 0;
    int wkc = 0;
    uint16_t size = 0;
    uint16_t datalen = 0;
    uint16_t maxsize = 0;
    uint8_t  last_packet_flag = 0;    /* 最后一包标志 */
    _foe_data_t senddata;
    _foe_data_t recvdata;   
    memset(&senddata,0,sizeof(_foe_data_t));
    memset(&recvdata,0,sizeof(_foe_data_t));
    /* 先读取一下接收邮箱，将其清空，防止等会去读从站回复的数据的时候读到脏数据 */
    wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,0);
    /* 填充地址，这个地址不会用来寻址，从站源码中目前也没有用到，可以写0 */
    senddata.mbxheader.address  = htoes(address);
    senddata.mbxheader.channel  = 0x00;
    senddata.mbxheader.priority = 0x00;
    senddata.mbxheader.type     = TYPE_MBX_FOE;
    senddata.mbxheader.cnt      = get_mailbox_cnt(slave);
    senddata.mbxheader.reserved = 0x00;   
    senddata.mbxheader.length   = htoes(0x0006+mecm_foe_retry_len); 
    /* foe 内容填充 */
    senddata.opcode = FOE_DATA;
    senddata.reserved = 0;
    senddata.foe.packetno = mecm_foe_packetno;
    memcpy(senddata.data,mecm_foe_buf,mecm_foe_retry_len);
    /* 发送请求 */
    wkc = mecm_mailbox_send(slave,(uint8_t *)&senddata,timeout);
    if(wkc>0)
    {
        memset(&recvdata,0,sizeof(_foe_data_t));
        wkc = mecm_mailbox_recv(slave,(uint8_t *)&recvdata,timeout);
        if(wkc>0)
        {
            if(recvdata.mbxheader.type == TYPE_MBX_FOE)
            {
                switch (recvdata.opcode)
                {
                case FOE_ACK:
                    /* 判断数据包的编号 */
                    if(recvdata.foe.packetno == mecm_foe_packetno)
                    {
                        if(last_packet_flag ==1)
                        {
                            return FOE_TASK_STATE_STOP;
                        }
                        mecm_foe_write_data_handle(mecm_foe_buf,mecm_foe_filesize);
                        return FOE_TASK_STATE_DATA;
                    }else
                    {
                        mecm_foe_errcode = FOE_ERRCODE_PACKENO;
                        return FOE_TASK_STATE_ERR;
                    }                  
                    break;
                case FOE_ERR:
                    mecm_foe_err_handle(recvdata.foe.errcode);
                    return FOE_TASK_STATE_EXCEPT;
                    break;
                case FOE_BUSY:
                    mecm_foe_busy_handle(recvdata.foe.busy.done,recvdata.foe.busy.entire);
                    return FOE_TASK_STATE_RETRY;
                    break;
                default:
                    break;
                }
            }else
            {
                return FOE_TASK_STATE_EXCEPT;
            }
            
        }else
        {
            return FOE_TASK_STATE_EXCEPT;
        }
        
    }
    return FOE_TASK_STATE_EXCEPT;        
}




__attribute__((weak)) void mecm_foe_read_data_handle(uint8_t *buf,uint16_t size)
{
    
}
__attribute__((weak)) void mecm_foe_write_data_handle(uint8_t *buf,uint16_t size)
{
    
}
__attribute__((weak)) void mecm_foe_err_handle(uint32_t errcode)
{
    
}
__attribute__((weak)) void mecm_foe_busy_handle(uint16_t done,uint16_t entire)
{
    
}
__attribute__((weak)) void mecm_foe_stop(void)
{
    
}
__attribute__((weak)) void mecm_foe_except(void)
{
    
}
