/*
 * @******************************************************************************: 
 * @Description: GD32F470 IWDG模块配置源文件
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-02-02 16:31:34
 * @LastEditors: gxf
 * @LastEditTime: 2023-04-21 13:36:41
 * @==============================================================================: 
 */

#include "bsp_iwdg.h"
#include "AMConfig.h"
#include "gd32f4xx_fwdgt.h"

static u8 sgm_flag = 0;

/**
 * @******************************************************************************: 
 * @func: [FWDG_Init]
 * @description: 看门狗初始化 独立看门狗使用的独立的32KHZ时钟，该时钟不是十分精确
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void FWDG_Init(void)
{
  fwdgt_write_enable();
  fwdgt_config(FWDGT_VALUE, FWDGT_PSC_DIV32);
  fwdgt_write_disable();
}

/**
 * @******************************************************************************: 
 * @func: [FWDG_Feed]
 * @description: 喂狗
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void FWDG_Feed(void)
{
  fwdgt_counter_reload();
}

void bsp_sgm_wdg_init(void)
{
    rcu_periph_clock_enable(SGM_GPIO_CLK);
    gpio_mode_set(SGM_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SGM_GPIO);
    gpio_output_options_set(SGM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SGM_GPIO);
    gpio_output_options_set(SGM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SGM_GPIO);
    gpio_bit_set(SGM_GPIO_PORT,SGM_GPIO);
}

void sgm_wdg_feed(void)
{
  if(sgm_flag == 0) 
    SGM_WDI_L;
  else
    SGM_WDI_H ;
  sgm_flag = !sgm_flag;
}