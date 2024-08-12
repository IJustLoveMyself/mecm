/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master Config 模块 用于实现从站配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:46:35
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 11:25:31
 * @==============================================================================: 
 */
#ifndef _MECM_CONFIG_H
#define _MECM_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "mecm_sm.h"
#include "mecm_fmmu.h"

#define NAMEMAXLEN        50     /* 名字的最大长度 */
#define MAX_SLAVE_NUM     10     /* 最大从站个数 */
#define MAX_SM_NUM        8      /* 每个从站中使用的SM的最大数 */
#define MAX_FMMU_NUM      8      /* 每个从站中使用的FMMU的最大数 */


#define COMMU_TIMEOUT             20000   /* Ethercat收发超时  单位us */
#define EEPROM_TIMEOUT            20000   /* EEPROM忙等待超时  单位us */
#define RETYR_TIMES               3       /* 寄存器配置失败是的重新配置次数 */

#define CONFIG_ADDR_OFFSET        0x1000  /* 从站起始地址的偏移量 */

#define TPDOASSIGN                0x1c13
#define RPDOASSIGN                0x1c12

enum
{
    
    ESC_STATE_NONE           = 0x00,     
    ESC_STATE_INIT           = 0x01,     
    ESC_STATE_PRE_OP         = 0x02,
    ESC_STATE_BOOT           = 0x03,
    ESC_STATE_SAFE_OP        = 0x04,
    ESC_STATE_OPERATIONAL    = 0x08,  
    ESC_STATE_ACK            = 0x10,     /* 应答，用于清除错误 */
    ESC_STATE_ERROR          = 0x10
};

enum{
    ESC_REG_TYPE        = 0x0000,
    ESC_REG_PORTDES     = 0x0007,
    ESC_REG_ESCSUP      = 0x0008,
    ESC_REG_STADR       = 0x0010,
    ESC_REG_ALIAS       = 0x0012,
    ESC_REG_DLCTL       = 0x0100,
    ESC_REG_DLPORT      = 0x0101,
    ESC_REG_DLALIAS     = 0x0103,
    ESC_REG_DLSTAT      = 0x0110,
    ESC_REG_ALCTL       = 0x0120,
    ESC_REG_ALSTAT      = 0x0130,
    ESC_REG_ALSTATCODE  = 0x0134,
    ESC_REG_PDICTL      = 0x0140,
    ESC_REG_IRQMASK     = 0x0200,
    ESC_REG_RXERR       = 0x0300,
    ESC_REG_FRXERR      = 0x0308,
    ESC_REG_EPUECNT     = 0x030C,
    ESC_REG_PECNT       = 0x030D,
    ESC_REG_PECODE      = 0x030E,
    ESC_REG_LLCNT       = 0x0310,
    ESC_REG_WDCNT       = 0x0442,
    ESC_REG_DCTIME0     = 0x0900,
    ESC_REG_DCTIME1     = 0x0904,
    ESC_REG_DCTIME2     = 0x0908,
    ESC_REG_DCTIME3     = 0x090C,
    ESC_REG_DCSYSTIME   = 0x0910,
    ESC_REG_DCLOCALTIME = 0x0918,
    ESC_REG_DCSYSOFFSET = 0x0920,
    ESC_REG_DCSYSDELAY  = 0x0928,
    ESC_REG_DCSYSDIFF   = 0x092C,
    ESC_REG_DCSPEEDCNT  = 0x0930,
    ESC_REG_DCTIMEFILT  = 0x0934,
    ESC_REG_DCCUC       = 0x0980,
    ESC_REG_DCSYNCACT   = 0x0981,
    ESC_REG_DCSTART0    = 0x0990,
    ESC_REG_DCCYCLE0    = 0x09A0,
    ESC_REG_DCCYCLE1    = 0x09A4
};



typedef struct
{
    uint8_t  slavename[NAMEMAXLEN];     /* 从站名字 */
    uint16_t state;                     /* 从站当前状态 */
    uint8_t  pditype;                   /* 从站PDI接口类型 */
    uint16_t alstatuscode;              /* 从站当前错误码 */
    uint16_t configaddr;                /* 从站配置地址 */
    uint16_t aliasaddr;                 /* 从站别名 */
    uint16_t bootstrap_wo;              /* BootStrap状态下主站写邮箱的地址偏移 */
    uint16_t bootstrap_wl;              /* BootStrap状态下主站写邮箱的长度 */
    uint16_t bootstrap_ro;              /* BootStrap状态下主站读邮箱的地址偏移 */
    uint16_t bootstrap_rl;              /* BootStrap状态下主站读邮箱的长度 */
    uint16_t standard_wo;               /* standard状态下主站写邮箱的地址偏移 */
    uint16_t standard_wl;               /* standard状态下主站写邮箱的长度 */
    uint16_t standard_ro;               /* standard状态下主站读邮箱的地址偏移 */
    uint16_t standard_rl;               /* standard状态下主站读邮箱的长度 */    
    mecm_sm_reg_t  rSM[MAX_SM_NUM];     /* 从站SM寄存器配置 */
    uint8_t  rSMtype[MAX_SM_NUM];       /* SM类型 */
    uint8_t  nSM;                       /* 从站使用的SM个数 */
    mecm_fmmu_reg_t rFMMU[MAX_FMMU_NUM];/* 从站FMMU寄存器配置 */
    uint8_t nFMMU;                      /* 从站使用的FMMU个数 */
    uint8_t eep_read_64bit;             /* 从站是否支持8字节读取*/
    uint8_t mbx_cnt;                    /* 邮箱头中的cnt 从站会检查cnt是否为0或是否改变 */
    uint32_t tpdobit;                   /* TPDO 映射的长度，单位bit*/
    uint32_t rpdobit;                   /* RPDO 映射的长度，单位bit*/
    uint32_t tpdobyte;                  /* TPDO 映射的长度，单位byte*/
    uint32_t rpdobyte;                  /* RPDO 映射的长度，单位yte*/
    uint8_t* tpdoaddr;                  /* TPDO本地地址 */
    uint8_t* rpdoaddr;                  /* RPDO本地地址 */
    uint32_t logtpdoaddr;               /* 从站TPDO FMMU映射的起始地址*/
    uint32_t logrpdoaddr;               /* 从站RPDO FMMU映射的起始地址*/
    uint16_t dcflag;                    /* DC时钟是否可用 1:可用*/
    uint8_t  preslave;                  /* 与本从站相连的前一个从站的编号 */
    uint8_t  portcnt;                   /* 端口使用数量 */ 
    uint8_t  activeport;                /* 端口使用情况 bit0~3代表端口0~3 0表示未激活，1表示激活*/
    uint32_t port0time;                 /* 数据帧到达端口0的时间，用于计算传输延时 */
    uint32_t port1time;                 /* 数据帧到达端口1的时间，用于计算传输延时 */
    uint32_t port2time;                 /* 数据帧到达端口2的时间，用于计算传输延时 */
    uint32_t port3time;                 /* 数据帧到达端口3的时间，用于计算传输延时 */
    uint32_t portdelay;                 /* 在DC拓扑中与前一个从站的传输延时 */
}mecm_slave_t;


extern mecm_slave_t mecm_slave[MAX_SLAVE_NUM];
extern uint8_t mecm_slave_count;



void mecm_config_init(void);
void mecm_memery_init(void);
int mecm_scan_slave(void);
int mecm_set_configaddr(void);
int mecm_get_eeprom_info_all(void);
int mecm_get_eeprom_info(uint16_t slave);
int mecm_set_sm_mailbox(void);
int mecm_set_sm_fmmu_pdo1(uint8_t *startaddr,uint32_t len);
int mecm_set_sm_fmmu_pdo2(uint8_t *startaddr,uint32_t len);
void mecm_get_pdo_size(void);
int mecm_state_change(uint16_t slave,uint16_t state);
int mecm_state_read(uint16_t slave);
#ifdef __cplusplus
}
#endif

#endif

