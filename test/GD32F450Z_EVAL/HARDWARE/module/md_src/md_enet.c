/*
 * @******************************************************************************: 
 * @Description: 
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-06-01 20:38:09
 * @LastEditors: gxf
 * @LastEditTime: 2024-04-03 15:12:35
 * @==============================================================================: 
 */
#include "md_enet.h"


/* RxDMA/TxDMA 描述符  */
extern enet_descriptors_struct  rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];

/* ENET receive 缓冲区  */
extern uint8_t rx_buff[ENET_RXBUF_NUM][ENET_RXBUF_SIZE]; 

/* ENET transmit 缓冲区 */
extern uint8_t tx_buff[ENET_TXBUF_NUM][ENET_TXBUF_SIZE]; 

/* 收发描述符的地址 */
extern enet_descriptors_struct  *dma_current_txdesc;
extern enet_descriptors_struct  *dma_current_rxdesc;

unsigned char mac[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};


/**
 * @******************************************************************************: 
 * @func: [enet_clock_init]
 * @description: 使能以太网模块时钟
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static void enet_clock_init(void)
{
    /* 使能以太网时钟  */
    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);
}

/**
 * @******************************************************************************: 
 * @func: [enet_gpio_init]
 * @description: 以太网引脚初始化，使用了RMII接口
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static void enet_gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOG);
    rcu_periph_clock_enable(RCU_GPIOH);
    rcu_periph_clock_enable(RCU_GPIOI);

    /* 使能 SYSCFG clock */
    rcu_periph_clock_enable(RCU_SYSCFG);

    syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_RMII);

    /* RMII 模式引脚初始化 */

    /* PA1: ETH_RMII_REF_CLK */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1);

    /* PA2: ETH_MDIO */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_2);

    /* PA7: ETH_RMII_CRS_DV */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_7);

    gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_1);
    gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_2);
    gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_7);

    /* PB11: ETH_RMII_TX_EN */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11);

    /* PB12: ETH_RMII_TXD0 */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_12);

    /* PB13: ETH_RMII_TXD1 */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_13);

    gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_11);
    gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_12);
    gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_13);

    /* PC1: ETH_MDC */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1);

    /* PC4: ETH_RMII_RXD0 */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4);

    /* PC5: ETH_RMII_RXD1 */
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_5);

    gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_1);
    gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_4);
    gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_5);
}

/**
 * @******************************************************************************: 
 * @func: [enet_mac_dma_config]
 * @description: 以太网DMA和MAC初始化
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static int enet_mac_dma_config(void)
{
    int i;
    ErrStatus reval_state = ERROR;
    /* 复位，并等待完成 */
    enet_deinit();
    reval_state = enet_software_reset();
    if(ERROR == reval_state) {
        return ERROR;
    }
    /* 以太网初始化 配置网卡自协商模式、使能接收端校验和检测功能、接收所有的广播帧*/
    reval_state = enet_init(ENET_AUTO_NEGOTIATION, ENET_AUTOCHECKSUM_ACCEPT_FAILFRAMES, ENET_PROMISCUOUS_MODE);
    if(ERROR == reval_state) {
        return ERROR;
    }
    /* 设置MAC地址 */
    enet_mac_address_set(ENET_MAC_ADDRESS0, mac);
    /* 初始化收发的描述符，常规描述符，链结构*/
    enet_descriptors_chain_init(ENET_DMA_TX);
    enet_descriptors_chain_init(ENET_DMA_RX); 
    /* 设置接收描述符 RDES1的31位为0，接收完成后会立马产生中断*/
    for(i=0; i<ENET_RXBUF_NUM; i++){ 
        enet_rx_desc_immediate_receive_complete_interrupt(&rxdesc_tab[i]);
    }
    /* 使能硬件IP包头和数据域的校验和计算和插入 */
    for(i=0; i < ENET_TXBUF_NUM; i++){
        enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
    }
    // /* 正常中断汇总使能 接收中断属于正常中断 */
    // enet_interrupt_enable(ENET_DMA_INT_NIE);
    // /* 使能接收中断 */
    // enet_interrupt_enable(ENET_DMA_INT_RIE);
    /* 使能以太网 */
    enet_enable();  
    return reval_state;
}

/**
 * @******************************************************************************: 
 * @func: [enet_system_init]
 * @description: 初始化以太网模块 
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
int enet_system_init(void)
{
    ErrStatus reval_state = ERROR;
    /* 初始化以太网模块时钟 */
    enet_clock_init();
    /* 初始化以太网引脚 */
    enet_gpio_init();
    /* 初始化以太网MAC和DMA */
    reval_state = enet_mac_dma_config();
    return reval_state;
}


/**
 * @******************************************************************************: 
 * @func: [enet_buf_send]
 * @description: 以太网数据发送
 * @note: 
 * @author: gxf
 * @param [uint8_t] *buf  待发送数据的地址
 * @param [uint32_t] length 待发送数据的长度
 * @return [*]
 * @==============================================================================: 
 */
int enet_buf_send(uint8_t *buf,uint32_t length)
{
    ErrStatus reval_state = ERROR; 
    reval_state = enet_frame_transmit(buf,length);
    return  reval_state;
}

/**
 * @******************************************************************************: 
 * @func: [enet_buf_send]
 * @description: 以太网数据接收
 * @note: 
 * @author: gxf
 * @param [uint8_t] *buf 接收数据的地址
 * @param [uint32_t] length 接收数据缓存的最大长度
 * @return [*]
 * @==============================================================================: 
 */
int enet_buf_recv(uint8_t *buf,uint32_t length)
{
    ErrStatus reval_state;
    uint16_t len;
    uint8_t *buffer;
    if(enet_rxframe_size_get()){
        reval_state = enet_frame_receive(buf,length,0);
    }else{
        reval_state = 0;
    }    
    return reval_state;
}

int enet_buf_recv_2(uint8_t *buf,uint32_t length,uint16_t *recvlen)
{
    ErrStatus reval_state;
    uint16_t len;
    uint8_t *buffer;
    if(enet_rxframe_size_get()){
        reval_state = enet_frame_receive(buf,length,recvlen);
    }else{
        reval_state = 0;
    }    
    return reval_state;
}
