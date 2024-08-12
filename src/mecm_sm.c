/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master SyncManager 模块 用于实现对SM的配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:43:20
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-31 18:49:24
 * @==============================================================================: 
 */


#include "mecm_sm.h"
#include "mecm_type.h"
#include "mecm_base.h"
#include "mecm_config.h"


/**
 * @******************************************************************************: 
 * @func: [mecm_sm_config]
 * @description: SM 配置，配置信息应该事先由EEPROM中读取出来或者手动填充
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号1，2，3，4，5...
 * @param [uint8_t] sm_cnt SM编号，0，1，2，3...
 * @return [*] ret<=0:配置失败
 * @==============================================================================: 
 */
int mecm_sm_config(uint16_t slave,uint8_t sm_cnt)
{
    int wkc = 0;
    uint8_t buf[8];
    uint16_t tmp = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    /* 获取SM配置的起始地址 */
    tmp = mecm_slave[slave].rSM[sm_cnt].sm_start_address;
    /* 大小端转换，然后将配置保存在临时数组中，之后一起配置 */
    buf[0] = tmp&0xFF;
    buf[1] = (tmp>>8)&0xFF;
    /* 获取SM配置的长度 */
    tmp = mecm_slave[slave].rSM[sm_cnt].sm_length;
    /* 大小端转换，然后将配置保存在临时数组中，之后一起配置 */
    buf[2] = tmp&0xFF;
    buf[3] = (tmp>>8)&0xFF;
    /* 获取SM配置的控制寄存器 */
    buf[4] = mecm_slave[slave].rSM[sm_cnt].sm_ctr.reg_byte;
    buf[5] = 0;
    buf[6] = mecm_slave[slave].rSM[sm_cnt].sm_activate.reg_byte;
    buf[7] = 0;
    /* 配置SM */
    wkc = mecm_FPWR(configaddr,ESC_SM0_REG_STRART_ADDR+sm_cnt*8,buf,8,COMMU_TIMEOUT);
    return wkc;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_sm_config_add]
 * @description: 添加SM配置
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave
 * @param [mecm_sm_reg_t] *smconfig 待添加的配置
 * @param [uint8_t] smtype SM的类型：1：主站邮箱输出 2：主站邮箱输入 3：主站过程数据输出 4：从站过程数据输入
 * @return [*]
 * @==============================================================================: 
 */
void mecm_sm_config_add(uint16_t slave,mecm_sm_reg_t *smconfig,uint8_t smtype)
{
    mecm_slave[slave].rSM[mecm_slave[slave].nSM].sm_start_address     = smconfig->sm_start_address;
    mecm_slave[slave].rSM[mecm_slave[slave].nSM].sm_length            = smconfig->sm_length;
    mecm_slave[slave].rSM[mecm_slave[slave].nSM].sm_ctr.reg_byte      = (smconfig->sm_ctr).reg_byte;
    mecm_slave[slave].rSM[mecm_slave[slave].nSM].sm_activate.reg_byte = (smconfig->sm_activate).reg_byte;
    mecm_slave[slave].rSMtype[mecm_slave[slave].nSM] = smtype;
    mecm_slave[slave].nSM++;
}

