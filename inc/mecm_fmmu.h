/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master FMMU模块,用于对从站FMMU配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:44:13
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-25 10:42:15
 * @==============================================================================: 
 */
#ifndef _MECM_FMMU_H
#define _MECM_FMMU_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	
#include "mecm_type.h"



#define ESC_FMMU0_REG_BASE               0x600
#define ESC_FMMU0_REG_LOGI_START_ADDR    (0x600+0)
#define ESC_FMMU0_REG_LENGTH             (0x600+4)
#define ESC_FMMU0_REG_LOGI_START_BIT     (0x600+6)
#define ESC_FMMU0_REG_LOGI_STOP_BIT      (0x600+7)
#define ESC_FMMU0_REG_PHYS_START_ADDR    (0x600+8)
#define ESC_FMMU0_REG_PHYS_START_BIT     (0x600+0xA)
#define ESC_FMMU0_REG_TYPE               (0x600+0xB)
#define ESC_FMMU0_REG_ACTIVATE           (0x600+0xC)


typedef struct 
{
    uint32_t fmmu_logi_start_addr;     /* FMMU逻辑地址起始地址 */
    uint16_t fmmu_length;              /* FMMU逻辑地址长度 */
    uint16_t fmmu_phys_start_addr;     /* FMMU物理地址起始地址 */
    uint8_t  fmmu_logo_start_bit;      /* FMMU逻辑地址起始位 */
    uint8_t  fmmu_logo_stop_bit;       /* FMMU逻辑地址停止位 */
    uint8_t  fmmu_phys_start_bit;      /* FMMU物理地址起始位 */
    uint8_t  fmmu_type;                /* FMMU类型 */
    uint8_t  fmmu_activate;            /* FMMU激活 */
}mecm_fmmu_reg_t;


void mecm_fmmu_config_add(uint16_t slave,mecm_fmmu_reg_t *fmmuconfig);
int mecm_fmmu_config(uint16_t slave,uint8_t fmmu_cnt);

#ifdef __cplusplus
}
#endif

#endif
