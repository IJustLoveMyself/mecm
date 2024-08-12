/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master SyncManager 模块 用于实现对SM的配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:43:11
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-30 11:27:35
 * @==============================================================================: 
 */
#ifndef _MECM_SM_H
#define _MECM_SM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	
#include "mecm_type.h"
//



#define ESC_SM0_REG_BASE           0x800
#define ESC_SM0_REG_STRART_ADDR    (0x800+0) 
#define ESC_SM0_REG_LENGTH         (0x800+2)
#define ESC_SM0_REG_CTR            (0x800+4)
#define ESC_SM0_REG_STATUS         (0x800+5)
#define ESC_SM0_REG_ACTIVATE       (0x800+6)
#define ESC_SM0_REG_PDI            (0x800+7)

/* 这些寄存器的内容从EEPROM中读取填充 */
typedef struct 
{
    uint16_t sm_start_address;         /* SM物理起始地址寄存器 */
    uint16_t sm_length;                /* SM通道长度寄存器 */
    union{
        uint8_t reg_byte;              /* SM通道控制寄存器*/
        struct {
            uint8_t operation_mode:2;  /* 运行模式 */
            uint8_t direction:2;       /* 方向 */
            uint8_t ecat_intr_enable:1;/* ECAT中断请求触发使能 */
            uint8_t pdi_intr_enable:1; /* PDI中断请求触发使能 */
            uint8_t wdt_intr_enable:1; /* WDT中断请求触发使能 */
            uint8_t reserve:1;
        }reg_bit;    
    }sm_ctr;
    uint8_t sm_status;                 /* SM状态寄存器，主站只读，不需要配置 */         
    union{
        uint8_t reg_byte;              /* SM通道控制寄存器*/
        struct {
            uint8_t enable:1;          /* SM通道使能 */
            uint8_t repeat:1;          /* SM请求重传 */
            uint8_t reserve:4;         
            uint8_t ecat_latch_event:1;/* ecat读写锁存事件 */
            uint8_t pdi_latch_event:1; /* pdi中断请求触发使能 */       
        }reg_bit;    
    }sm_activate;
    uint8_t sm_pdi_ctr;                /* PDI控制寄存器，主站只读，从站来控制SM，不需要配置 */
}mecm_sm_reg_t;


void mecm_sm_config_add(uint16_t slave,mecm_sm_reg_t *smconfig,uint8_t smtype);
int mecm_sm_config(uint16_t slave,uint8_t sm_cnt);

#ifdef __cplusplus
}
#endif

#endif
