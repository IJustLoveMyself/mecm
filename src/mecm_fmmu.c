/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master FMMU模块,用于对从站FMMU配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:44:27
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-01 16:35:29
 * @==============================================================================: 
 */
#include <string.h>
#include "mecm_fmmu.h"
#include "mecm_base.h"
#include "mecm_config.h"
/**
 * @******************************************************************************: 
 * @func: [mecm_fmmu_config]
 * @description: FMMU 配置，配置信息一般是根据SM配置来决定
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号1，2，3，4，5...
 * @param [uint8_t] sm_cnt SM编号，0，1，2，3...
 * @return [*] wkc<=0:配置失败
 * @==============================================================================: 
 */
int mecm_fmmu_config(uint16_t slave,uint8_t fmmu_cnt)
{
    int wkc = 0,cnt = 0;
    uint8_t buf[16];
    uint32_t tmp = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    /* 获取逻辑地址起始地址 */
    tmp = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_logi_start_addr;
    memset(buf,0,16);
    buf[0] = tmp&0xFF;
    buf[1] = (tmp>>8)&0xFF;
    buf[2] = (tmp>>16)&0xFF;
    buf[3] = (tmp>>24)&0xFF;
    /* 获取逻辑地址映射长度 */
    tmp = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_length;

    buf[4] = tmp&0xFF;
    buf[5] = (tmp>>8)&0xFF;
    buf[6] = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_logo_start_bit;
    buf[7] = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_logo_stop_bit;
    tmp = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_phys_start_addr;

    buf[8] = tmp&0xFF;
    buf[9] = (tmp>>8)&0xFF;
    buf[10] = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_phys_start_bit;
    buf[11] = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_type;
    buf[12] = mecm_slave[slave].rFMMU[fmmu_cnt].fmmu_activate;
    /* 配置FMMU */
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_FMMU0_REG_BASE+fmmu_cnt*16,buf,16,COMMU_TIMEOUT);
    } while((wkc <=0 )&&(cnt++ < RETYR_TIMES));
    
    
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_fmmu_config_add]
 * @description: 添加FMMU配置
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*]
 * @==============================================================================: 
 */
void mecm_fmmu_config_add(uint16_t slave,mecm_fmmu_reg_t *fmmuconfig)
{
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_logi_start_addr      = fmmuconfig->fmmu_logi_start_addr;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_length               = fmmuconfig->fmmu_length;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_phys_start_addr      = fmmuconfig->fmmu_phys_start_addr;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_logo_start_bit       = fmmuconfig->fmmu_logo_start_bit;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_logo_stop_bit        = fmmuconfig->fmmu_logo_stop_bit;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_phys_start_bit       = fmmuconfig->fmmu_phys_start_bit;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_type                 = fmmuconfig->fmmu_type;
    mecm_slave[slave].rFMMU[mecm_slave[slave].nFMMU].fmmu_activate             = fmmuconfig->fmmu_activate;
    mecm_slave[slave].nFMMU++;
}

