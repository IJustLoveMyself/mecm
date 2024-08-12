/*
 * @******************************************************************************: 
 * @Description: GD32F470 IWDG模块配置头文件
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-02-02 16:31:34
 * @LastEditors: gxf
 * @LastEditTime: 2023-02-17 20:58:39
 * @==============================================================================: 
 */


#ifndef __BSP_IWDG_H
#define __BSP_IWDG_H

void FWDG_Init(void);
void FWDG_Feed(void);
void sgm_wdg_feed(void);
void bsp_sgm_wdg_init(void);
#endif
