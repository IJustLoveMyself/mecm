/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master EEPROM模块,用于对从站EEPROM进行读写
 * @Autor: gxf
 * @Date: 2023-09-26 19:44:53
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-30 20:40:00
 * @==============================================================================: 
 */
#ifndef _MECM_EEPROM_H
#define _MECM_EEPROM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	
#include "mecm_config.h"

#define ESC_EEPROM_CONFIG            0x500
#define ESC_EEPROM_PDI_ACCESS_STATE  0x501
#define ESC_EEPROM_CTR_STA           0x502
#define ESC_EEPROM_ADDRESS           0x504
#define ESC_EEPROM_DATA              0x508        

#define tNOP                         0x0000
#define tSTRINGS                     0x000A
#define tDataTypes                   0x0014
#define tGeneral                     0x001E
#define tFMMU                        0x0028
#define tSyncM                       0x0029
#define tTXPDO                       0x0032
#define tRXPDO                       0x0033
#define tDC                          0x003C
#define tEND                         0xFFFF



typedef struct 
{
    
    uint16_t aliasaddr;                /* 从站别名 */
    uint16_t bootstrap_wo;             /* BootStrap状态下主站写邮箱的地址偏移 */
    uint16_t bootstrap_wl;             /* BootStrap状态下主站写邮箱的长度 */
    uint16_t bootstrap_ro;             /* BootStrap状态下主站读邮箱的地址偏移 */
    uint16_t bootstrap_rl;             /* BootStrap状态下主站读邮箱的长度 */
    uint16_t standard_wo;              /* standard状态下主站写邮箱的地址偏移 */
    uint16_t standard_wl;              /* standard状态下主站写邮箱的长度 */
    uint16_t standard_ro;              /* standard状态下主站读邮箱的地址偏移 */
    uint16_t standard_rl;              /* standard状态下主站读邮箱的长度 */
    uint16_t mbx_protocol;             /* 邮箱支持的协议 */
} eeprom_info_t;


typedef struct 
{
    char slavename[NAMEMAXLEN];        /* 从站名字 */
}eeprom_strings_t;

#pragma pack(1)
typedef struct 
{
    uint8_t  groupidx;                 /* 组信息，Strings的索引，例如为1，表示Strings中第一个string是组信息；0表示没有 */
    uint8_t  imgidx;
    uint8_t  orderidx;
    uint8_t  nameidx;
    uint8_t  reserved1;
    uint8_t  coe_details;
    uint8_t  foe_details;
    uint8_t  eoe_details;
    uint8_t  soe_channels;
    uint8_t  ds402_channels;
    uint8_t  sysmanclass;
    uint8_t  flags;
    int16_t  current_EBus;
    uint8_t  pad1[2];
    uint16_t phy_port;
    uint8_t  pad2[14];
}eeprom_generals_t;



typedef struct 
{
    uint16_t phy_start_addr;
    uint16_t length;
    uint8_t  ctr_register;
    uint8_t  sta_register;
    uint8_t  enable;
    uint8_t  sm_type;
}eeprom_sm_t;


#pragma pack()

extern eeprom_info_t     eep_info;
extern eeprom_generals_t eep_generals;
extern eeprom_strings_t  eep_strings;
extern eeprom_sm_t eep_sm[MAX_SM_NUM]; 
extern uint8_t eep_nsm;


int mecm_eeprom_master(uint16_t slave);
int mecm_eeprom_pdi(uint16_t slave);
int mecm_eeprom_read(uint16_t slave,uint32_t eeprom_addr,uint64_t *readdata);
int mecm_eeprom_write(uint16_t slave,uint32_t eeprom_addr,uint16_t writedata);
int mecm_eeprom_read_buf(uint16_t slave,uint32_t eeprom_addr,uint8_t *buf,uint32_t len);
int mecm_eeprom_write_buf(uint16_t slave,uint32_t eeprom_addr,uint8_t *buf,uint32_t len);
int mecm_eeprom_findsii(uint16_t slave,uint16_t types,uint32_t *getaddr,uint16_t *getlength);
int mecm_eeprom_get_sii_info(uint16_t slave);
int mecm_eeprom_get_strings(uint16_t slave);
int mecm_eeprom_get_generals(uint16_t slave);
int mecm_eeprom_get_sm(uint16_t slave);
#ifdef __cplusplus
}
#endif

#endif
