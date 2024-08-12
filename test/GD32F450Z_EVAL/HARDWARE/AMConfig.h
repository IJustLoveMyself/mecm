#ifndef __AMCONFIG_H
#define __AMCONFIG_H
	
#include "gd32f4xx.h"

/**
 * ****************************************************************************
 * Module 配置，用于配置模块
 * ****************************************************************************
 */

#define PRINTF_USART                       UART7        /* 设置printf重定向的串口 */
#define TASK_TIMER                         TIMER2       /* 任务所用定时器为定时器1 */
/**
 * ****************************************************************************
 * BSP 配置，用于底层文件
 * ****************************************************************************
 */
/* 软件看门狗时间配置 */
#define FWDGT_VALUE                        2000   /* 大约2000ms  */

/* USART 端口配置
   总共右6个串口 其中 USART0、 USART5 挂在APB2上
   USART1、 USART2、 USART3、 USART4 挂在APB1上
*/

/* USART0 配置 管脚PA9、PA10、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART0_BAUDRATE                    115200  

#define USART0_RX_GPIO_PORT                GPIOA
#define USART0_RX_GPIO_CLK                 RCU_GPIOA
#define USART0_RX_PIN                      GPIO_PIN_10


#define USART0_TX_GPIO_PORT                GPIOA
#define USART0_TX_GPIO_CLK                 RCU_GPIOA
#define USART0_TX_PIN                      GPIO_PIN_9


/* USART1 配置 管脚PA2、PA3、波特率115200*/
#define USART1_BAUDRATE                    115200  

#define USART1_RX_GPIO_PORT                GPIOD
#define USART1_RX_GPIO_CLK                 RCU_GPIOD
#define USART1_RX_PIN                      GPIO_PIN_6


#define USART1_TX_GPIO_PORT                GPIOD
#define USART1_TX_GPIO_CLK                 RCU_GPIOD
#define USART1_TX_PIN                      GPIO_PIN_5


/* USART2 配置 管脚PB10、PB11、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART2_BAUDRATE                    115200  

#define USART2_RX_GPIO_PORT                GPIOD
#define USART2_RX_GPIO_CLK                 RCU_GPIOD
#define USART2_RX_PIN                      GPIO_PIN_9

#define USART2_TX_GPIO_PORT                GPIOD
#define USART2_TX_GPIO_CLK                 RCU_GPIOD
#define USART2_TX_PIN                      GPIO_PIN_8

/* USART3 配置 管脚PC10、PC11、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART3_BAUDRATE                    115200  

#define USART3_RX_GPIO_PORT                GPIOC
#define USART3_RX_GPIO_CLK                 RCU_GPIOC
#define USART3_RX_PIN                      GPIO_PIN_11

#define USART3_TX_GPIO_PORT                GPIOC
#define USART3_TX_GPIO_CLK                 RCU_GPIOC
#define USART3_TX_PIN                      GPIO_PIN_10


/* USART4 配置 管脚PC12、PD2、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART4_BAUDRATE                    115200  

#define USART4_RX_GPIO_PORT                GPIOD
#define USART4_RX_GPIO_CLK                 RCU_GPIOD
#define USART4_RX_PIN                      GPIO_PIN_2

#define USART4_TX_GPIO_PORT                GPIOC
#define USART4_TX_GPIO_CLK                 RCU_GPIOC
#define USART4_TX_PIN                      GPIO_PIN_12


/* USART5 配置 管脚PG9、PG14、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART5_BAUDRATE                    115200  

#define USART5_RX_GPIO_PORT                GPIOG
#define USART5_RX_GPIO_CLK                 RCU_GPIOG
#define USART5_RX_PIN                      GPIO_PIN_9

#define USART5_TX_GPIO_PORT                GPIOG
#define USART5_TX_GPIO_CLK                 RCU_GPIOG
#define USART5_TX_PIN                      GPIO_PIN_14

/* USART6 配置 管脚PC6、PC7、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART6_BAUDRATE                    115200  

#define USART6_RX_GPIO_PORT                GPIOE
#define USART6_RX_GPIO_CLK                 RCU_GPIOE
#define USART6_RX_PIN                      GPIO_PIN_7

#define USART6_TX_GPIO_PORT                GPIOE
#define USART6_TX_GPIO_CLK                 RCU_GPIOE
#define USART6_TX_PIN                      GPIO_PIN_8

/* USART7 配置 管脚PC6、PC7、波特率115200 8数据位、1停止位、无奇偶校验位 */
#define USART7_BAUDRATE                    115200  

#define USART7_RX_GPIO_PORT                GPIOE
#define USART7_RX_GPIO_CLK                 RCU_GPIOE
#define USART7_RX_PIN                      GPIO_PIN_0

#define USART7_TX_GPIO_PORT                GPIOE
#define USART7_TX_GPIO_CLK                 RCU_GPIOE
#define USART7_TX_PIN                      GPIO_PIN_1


/* 定时器1配置 定时器的输入时钟频率为200M*/
/* 定时器1配置 重装值1000 预分频值199+1 1ms一次中断 用于任务轮询*/

#define TIM2_CAR                           1000     /* 自动重装值 */
#define TIM2_PSC                           199      /* 预分频系数 */


/* 外部看门狗喂狗引脚 */
#define SGM_GPIO                         GPIO_PIN_9              /* IMU INT2信号输入引脚定义*/ 
#define SGM_GPIO_PORT                    GPIOB                   /* IMU INT2信号输入端口定义*/
#define SGM_GPIO_CLK                     RCU_GPIOB               /* IMU INT2信号输入时钟定义*/
#define SGM_WDI_H                        gpio_bit_set(SGM_GPIO_PORT,SGM_GPIO)
#define SGM_WDI_L                        gpio_bit_reset(SGM_GPIO_PORT,SGM_GPIO)


/* 中断重映射 */
#define timeisr                 TIMER2_IRQHandler     /* task 的轮询定时器映射到定时器5中断 */
#endif






