/*
 * @******************************************************************************: 
 * @Description: GD32F470 SYS模块配置源文件
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-02-02 16:31:34
 * @LastEditors: gxf
 * @LastEditTime: 2023-06-02 11:51:08
 * @==============================================================================: 
 */

 

#include "bsp_sys.h"
#include "AMConfig.h"


/**
 * @******************************************************************************: 
 * @func: [nvic_configuration]
 * @description: 中断优先级配置
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void nvic_configuration(void){
  /* 配置为组2 */
  nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
  nvic_irq_enable(SysTick_IRQn,0,2);
  nvic_irq_enable(TIMER2_IRQn,0,2);
//  nvic_irq_enable(ENET_IRQn, 2, 0);
}
