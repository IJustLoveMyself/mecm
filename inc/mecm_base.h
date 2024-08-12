/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master Base 模块 用于实现底层通信协议
 * @Autor: gxf
 * @Date: 2023-06-02 15:52:44
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-01 21:03:25
 * @==============================================================================: 
 */
#ifndef _MECM_BASE_H
#define _MECM_BASE_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "mecm_type.h"


/* 每一帧中Datagram的最大长度 */
#define MECM_MAXDATAGRAM          (1518-14-2-4)
/* 发送和接收的数据帧的最大个数 */
#define MAXBUF                    5
/* 一帧数据中datagram的最大个数 */
#define MAXDATAGRAMS              10



#pragma pack(1)

/* ethernet 报头结构体 */
typedef struct{
    /* 目的地址 */
    uint8_t dst[6]; 
    /* 源地址 */ 
    uint8_t src[6];
    /* 以太类型 */
    uint16_t etype;
} mecm_ethernet_header_t;

/* ethercat 报头结构体*/
typedef struct{
    /* ethercat 数据报长度，不包含FCS */
    uint16_t clehgth:11;
    /* 预留,0 */
    uint16_t reserve:1;
    /* 协议类型 ESC只支持 0x1 */
    uint16_t ctype:4;
} mecm_ethercat_header_t;

/* ethercat 数据帧结构体 */
typedef struct{
    mecm_ethernet_header_t en_header;
    mecm_ethercat_header_t ec_header;
    uint8_t datagram[MECM_MAXDATAGRAM];
} mecm_fram_t;



/* datagram 报头结构体 */
typedef struct{
    /* ethercat 命令 */
    uint8_t cmd;
    /* 索引值，主机产生，不会被从机修改，可以用来判断数据重复或者丢失 */
    uint8_t idx;
    /* 地址 */
    union{
        /* 逻辑地址 */
        uint32_t logiaddr;
        struct{
            /* 从站地址 */
            uint16_t adp;
            /* 内存地址 */
            uint16_t ado;
        } np;
    }addr;
    union{
        /* 数据长度等信息 */
        uint16_t length;
        struct{
            /* 数据包中的数据长度 */
            uint16_t dlength:11;
            /* 预留,0 */
            uint16_t r:3;
            /* 帧循环,由从机的处理单元自动置位，用来识别该帧是否已经循环了 */
            uint16_t c:1;
            /* 该数据包是否是最后一包，1表示后续还有数据包 */
            uint16_t m:1;            
        } data;
    }info;         
    /* 所有从站ethercat事件请求寄存器的OR*/
    uint16_t irq;
} mecm_datagram_header_t;

typedef struct{
    /* datagram在帧中的偏移量 */
    uint16_t datagram_offset[MAXDATAGRAMS];
    /* datagram中数据的长度 ，不包含datagram header和wck */
    uint16_t datagram_length[MAXDATAGRAMS];
} mecm_datagram_info_t;

#pragma pack()

typedef struct{
    /* 发送帧缓存 */
    mecm_fram_t mecm_sendbuf[MAXBUF];
    /* 接收帧缓存 */
    mecm_fram_t mecm_recvbuf[MAXBUF];
    /* 发送帧索引 */
    uint16_t sendindex;
    /* 发送帧中的datagram 的信息 */
    mecm_datagram_info_t datagram_info[MAXBUF];
    /* 发送帧中datagram的个数 */  
    uint8_t datagrams[MAXBUF];  
} mecm_port_t;

typedef enum
{
   /* 无操作 */
   CMD_NOP          = 0x00,
   /* 自动递增读取 */
   CMD_APRD,
   /* 自动递增写入 */
   CMD_APWR,
   /* 自动递增读写 */
   CMD_APRW,
   /* 配置地址读取 */
   CMD_FPRD,
   /* 配置地址写入 */
   CMD_FPWR,
   /* 配置地址读写 */
   CMD_FPRW,
   /* 广播读取 */
   CMD_BRD,
   /* 广播写入 */
   CMD_BWR,
   /* 广播读写 */
   CMD_BRW,
   /* 逻辑内存读取 */
   CMD_LRD,
   /* 逻辑内存写入 */
   CMD_LWR,
   /* 逻辑内存读写 */
   CMD_LRW,
   /* 自动递增多次读写 */
   CMD_ARMW,
   /* 配置多次读写 */
   CMD_FRMW
   /** Reserved */
} mecm_type_e;


void mecm_base_init(void);
int mecm_communicate(uint8_t idx,uint8_t *buf,uint16_t length,uint32_t timeout);
uint8_t get_idx(void);
void mecm_clear_port(uint8_t idx);
uint16_t mecm_setup_datagram_logi(mecm_type_e cmd,uint8_t idx,uint16_t offset,uint32_t logiaddr,uint8_t *data,uint16_t length,uint8_t more);
uint16_t mecm_setup_datagram_np(mecm_type_e cmd,uint8_t idx,uint16_t offset,uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint8_t more);
int mecm_BRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_BWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_APRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_ARMW(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_APWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_FRMW(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_FPRD(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_FPWR(uint16_t adp,uint16_t ado,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_LRW(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_LWR(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout);
int mecm_LRD(uint32_t addr,uint8_t *data,uint16_t length,uint32_t timeout);


extern mecm_port_t mecm_port;

#ifdef __cplusplus
}
#endif

#endif
