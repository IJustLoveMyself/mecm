/*
 * @******************************************************************************: 
 * @Description: 按摩机器人用户模块源文件
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-02-02 16:31:34
 * @LastEditors: gxf
 * @LastEditTime: 2024-04-02 17:59:05
 * @==============================================================================: 
 */

#include "AMConfig.h"
#include "bsp_iwdg.h"
#include "bsp_sys.h"
#include "bsp_systick.h"
#include "bsp_time.h"
#include "mecm_base.h"
#include "md_enet.h"
#include "mecm_port.h"
#include "mecm_config.h"
#include "mecm_pdo.h"

uint8_t recvbuf[1600];

/**
 * @******************************************************************************: 
 * @func: [bsp_init]
 * @description: 底层硬件初始化
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void bsp_init(void)
{
  sysTick_init();
  nvic_configuration();
  bsp_time2_init();
  mecm_eth0_init("eth0");
  mecm_config_init();
}



void test(void)
{
  uint16_t recvlen = 0;
    // mecm_processdata_all(1,1,20000);
    // delay_ms(10);
    enet_buf_recv_2(recvbuf,1600,&recvlen);
    enet_buf_send(recvbuf,recvlen);
}




void timeisr()
{
   if(timer_interrupt_flag_get(TASK_TIMER, TIMER_INT_UP) != RESET)  
  {
    timer_interrupt_flag_clear(TASK_TIMER, TIMER_INT_UP);    
  }  
}
