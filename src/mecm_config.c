/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master 配置模块，用于对从站进行配置
 * @Autor: gxf
 * @Date: 2023-09-26 19:46:46
 * @LastEditors: gxf
 * @LastEditTime: 2024-08-09 11:24:58
 * @==============================================================================: 
 */
#include <string.h>
#include "mecm_config.h"
#include "mecm_base.h"
#include "mecm_eeprom.h"
#include "mecm_coe.h"
#include "mecm_pdo.h"
/* 从站信息 从站信息记录从[1]开始 */
mecm_slave_t mecm_slave[MAX_SLAVE_NUM];
/* 从站个数 */
uint8_t mecm_slave_count;

/**
 * @******************************************************************************: 
 * @func: [mecm_init]
 * @description: 存储内存初始化
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void mecm_memery_init(void)
{
    mecm_slave_count = 0;
    memset(mecm_slave,0,sizeof(mecm_slave_t)*MAX_SLAVE_NUM);
}

/**
 * @******************************************************************************: 
 * @func: [mecm_scan_slave]
 * @description: 从站扫描
 * @note: 
 * @author: gxf
 * @return [*] >0:扫描成功 
 * @==============================================================================: 
 */
int mecm_scan_slave(void)
{
    int wkc = 0;
    uint8_t data;
    /* 不使用从站别名进行配置寻址 */
    data = 0;
    mecm_BWR(0x0000,ESC_REG_DLALIAS,&data,sizeof(data),COMMU_TIMEOUT);
    /* 重置从站为Init (应答位写1清除AL错误)*/
    data = ESC_STATE_INIT|ESC_STATE_ACK;
    mecm_BWR(0x0000,ESC_REG_ALCTL,&data,sizeof(data),COMMU_TIMEOUT);
    /* 获取从站个数 */
    wkc = mecm_BRD(0x0000,ESC_REG_TYPE,&data,sizeof(data),COMMU_TIMEOUT);
    if(wkc>0)
    {
        if(wkc<MAX_SLAVE_NUM)
            mecm_slave_count = wkc;
        else
            return 0;
    }
    return wkc; 
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_configaddr]
 * @description: 设置从站配置寻址的地址
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
int mecm_set_configaddr(void)
{
    uint8_t slave = 1;
    uint16_t adp = 0;
    uint16_t configaddr = 0;
    int wkc = 0;
    for(slave=1;slave<=mecm_slave_count;slave++)
    {
        adp = (uint16_t)(1-slave);
        configaddr = CONFIG_ADDR_OFFSET+slave;
        configaddr = htoes(configaddr);
        wkc = mecm_APWR(adp,ESC_REG_STADR,(uint8_t *)&configaddr,2,COMMU_TIMEOUT);
        if(wkc<=0) 
        {
            return wkc;
        }  
        configaddr = 0;
        wkc = mecm_APRD(adp,ESC_REG_STADR,(uint8_t *)&configaddr,2,COMMU_TIMEOUT);
        if(wkc<=0) 
        {
            return wkc;
        }  
        mecm_slave[slave].configaddr = etohs(configaddr);
    }
    return wkc;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_get_eeprom_info]
 * @description: 获取从站在EEPROM中的信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_get_eeprom_info(uint16_t slave)
{
    uint8_t i = 0;
    mecm_sm_reg_t smconfig;
    int wkc = 0;   
    /* 获取接口信息 */
    wkc = mecm_eeprom_get_sii_info(slave);
    if(wkc<=0) 
    {
        return wkc;
    }  
    mecm_slave[slave].aliasaddr    = eep_info.aliasaddr;
    mecm_slave[slave].bootstrap_wo = eep_info.bootstrap_wo; 
    mecm_slave[slave].bootstrap_wl = eep_info.bootstrap_wl;
    mecm_slave[slave].bootstrap_ro = eep_info.bootstrap_ro;
    mecm_slave[slave].bootstrap_rl = eep_info.bootstrap_rl;
    mecm_slave[slave].standard_wo  = eep_info.standard_wo;
    mecm_slave[slave].standard_wl  = eep_info.standard_wl;
    mecm_slave[slave].standard_ro  = eep_info.standard_ro;
    mecm_slave[slave].standard_rl  = eep_info.standard_rl;
    /* 程序中strings的获取依赖idx，所以generals要在strings前面获取 */
    wkc = mecm_eeprom_get_generals(slave);
    if(wkc<=0) 
    {
        return wkc;
    }  
    /* 获取名字 */
    wkc = mecm_eeprom_get_strings(slave);
    if(wkc<=0) 
    {
        return wkc;
    }           
    memcpy(mecm_slave[slave].slavename,eep_strings.slavename,NAMEMAXLEN);   
    /* 获取SM配置 */
    wkc = mecm_eeprom_get_sm(slave); 
    if(wkc<=0) 
    {
        return wkc;
    }  
    for(i=0;i<eep_nsm;i++)
    {
        memset((uint8_t *)&smconfig,0,sizeof(mecm_sm_reg_t));
        smconfig.sm_start_address = eep_sm[i].phy_start_addr;
        smconfig.sm_length        = eep_sm[i].length;
        smconfig.sm_ctr.reg_byte  = eep_sm[i].ctr_register;
        smconfig.sm_activate.reg_bit.enable = eep_sm[i].enable;
        mecm_sm_config_add(slave,&smconfig,eep_sm[i].sm_type);
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_get_eeprom_info_all]
 * @description: 获取所有从站在EEPROM中的信息
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
int mecm_get_eeprom_info_all(void)
{
    uint16_t slave = 1;
    uint8_t i = 0;
    int wkc = 0;   
    for ( slave = 1; slave <= mecm_slave_count; slave++)
    {
        /* 获取接口信息 */
        wkc = mecm_get_eeprom_info(slave);
        if(wkc<=0) 
        {
            return wkc;
        }  
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_sm_mailbox]
 * @description: 配置邮箱通信使用的SM，在Init状态下进行配置
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
int mecm_set_sm_mailbox(void)
{
    uint16_t slave = 1;
    int wkc = 0; 
    uint8_t i = 0;  
    for ( slave = 1; slave <= mecm_slave_count; slave++)
    {
        for(i=0;i<mecm_slave[slave].nSM;i++)
        {
            /* 根据类型来判断，在这里只配置邮箱通信用的SM */
            if((mecm_slave[slave].rSMtype[i] == 1)||(mecm_slave[slave].rSMtype[i] == 2))
            {
                wkc = mecm_sm_config(slave,i);
                if(wkc <=0 )
                {
                    return wkc;
                }                
            }          
        } 
        wkc = mecm_state_change(slave,ESC_STATE_PRE_OP|ESC_STATE_ACK); 
        if(wkc <=0 )
        {
            return wkc;
        }  
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_sm_fmmu_pdo1]
 * @description: 配置SM和FMMU提供PDO通信方式1 ,映射顺序为  RX1，TX1，RX2，TX2...RXn,TXn
 * @note: 映射以字节映射
 * @author: gxf
 * @param [uint8_t] *startaddr 用于映射的内存的起始地址 
 * @param [uint32_t] len 用于映射的地址大小
 * @return [*] <=0：配置失败
 * @==============================================================================: 
 */
int mecm_set_sm_fmmu_pdo1(uint8_t *startaddr,uint32_t len)
{
    uint16_t slave = 1;
    int wkc = 0;
    uint8_t i = 0;
    uint8_t *endaddr = startaddr+len;
    if(startaddr == 0)
    {
        return 0;
    }  
    mecm_fmmu_reg_t fmmuconfig;
    memset(startaddr,0,len);
    for ( slave = 1; slave <= mecm_slave_count; slave++)
    {
        /* 配置PDO使用的SM，并根据SM的配置来获取FMMU的配置　*/
        for(i=0;i<mecm_slave[slave].nSM;i++)
        {
            /* 用于RPDO的SM */
            if(mecm_slave[slave].rSMtype[i] == 3)
            {
                memset((uint8_t *)&fmmuconfig,0,sizeof(fmmuconfig));
                fmmuconfig.fmmu_logi_start_addr = (uint32_t)startaddr;
                
                /* 如果程序中RPDO的映射长度不是0，并且长度于EEPROM中的SM的长度不一样，那么以从站中映射的长度为准 */
                if((mecm_slave[slave].rSM[i].sm_length != mecm_slave[slave].rpdobyte)&&(mecm_slave[slave].rpdobyte!=0))
                {
                    mecm_slave[slave].rSM[i].sm_length = mecm_slave[slave].rpdobyte;
                }
                /* SM的长度不为0就激活SM（可能原本EEPROM中的SM长度为0，但是后来程序中的RPDO有改变，长度不为0） */
                if(mecm_slave[slave].rSM[i].sm_length !=0 )
                {
                    mecm_slave[slave].rSM[i].sm_activate.reg_byte = 0x01;
                }
                fmmuconfig.fmmu_length = mecm_slave[slave].rSM[i].sm_length;
                fmmuconfig.fmmu_phys_start_addr = mecm_slave[slave].rSM[i].sm_start_address;
                fmmuconfig.fmmu_logo_start_bit  = 0;
                fmmuconfig.fmmu_logo_stop_bit   = 7;
                fmmuconfig.fmmu_phys_start_bit  = 0;
                fmmuconfig.fmmu_type            = 0x02;
                fmmuconfig.fmmu_activate        = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
                mecm_fmmu_config_add(slave,&fmmuconfig);
                if(fmmuconfig.fmmu_activate)
                {
                    mecm_slave[slave].rpdoaddr = startaddr;
                    mecm_slave[slave].logrpdoaddr = (uint32_t)startaddr;
                    startaddr += fmmuconfig.fmmu_length;
                    /* 超出内存的大小了 */
                    if(startaddr>endaddr)
                    {
                        return 0;
                    }
                }
                wkc = mecm_sm_config(slave,i);
                if(wkc<=0)
                {
                    return wkc;
                }                
            }
            /* 用于TPDO的SM */
            if(mecm_slave[slave].rSMtype[i] == 4)   
            {
                memset((uint8_t *)&fmmuconfig,0,sizeof(fmmuconfig));
                fmmuconfig.fmmu_logi_start_addr = (uint32_t)startaddr;
                /* 如果程序中RPDO的映射长度不是0，并且长度于EEPROM中的SM的长度不一样，那么以从站中映射的长度为准 */
                if((mecm_slave[slave].rSM[i].sm_length != mecm_slave[slave].rpdobyte)&&(mecm_slave[slave].tpdobyte!=0))
                {
                    mecm_slave[slave].rSM[i].sm_length = mecm_slave[slave].tpdobyte;
                }
                /* SM的长度不为0就激活SM（可能原本EEPROM中的SM长度为0，但是后来程序中的RPDO有改变，长度不为0） */
                if(mecm_slave[slave].rSM[i].sm_length !=0 )
                {
                    mecm_slave[slave].rSM[i].sm_activate.reg_byte = 0x01;
                }
                fmmuconfig.fmmu_length = mecm_slave[slave].rSM[i].sm_length;
                fmmuconfig.fmmu_phys_start_addr = mecm_slave[slave].rSM[i].sm_start_address;
                fmmuconfig.fmmu_logo_start_bit  = 0;
                fmmuconfig.fmmu_logo_stop_bit   = 7;
                fmmuconfig.fmmu_phys_start_bit  = 0;
                fmmuconfig.fmmu_type            = 0x01;
                fmmuconfig.fmmu_activate        = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
                mecm_fmmu_config_add(slave,&fmmuconfig);
                if(fmmuconfig.fmmu_activate)
                {
                    mecm_slave[slave].tpdoaddr = startaddr;
                    mecm_slave[slave].logtpdoaddr = (uint32_t)startaddr;
                    startaddr += fmmuconfig.fmmu_length;
                    /* 超出内存的大小了 */
                    if(startaddr>endaddr)
                    {
                        return 0;
                    }
                }   
                wkc = mecm_sm_config(slave,i); 
                if(wkc<=0)
                {
                    return wkc;
                }        
            }      
        } 
        /* 配置FMMU */
        for(i = 0;i<mecm_slave[slave].nFMMU;i++)
        {
            wkc = mecm_fmmu_config(slave,i);
            if(wkc<=0)
            {
                return wkc;
            }
        }
        /* 跳转到Safe OP */
        wkc = mecm_state_change(slave,ESC_STATE_SAFE_OP); 
        if(wkc <=0 )
        {
            return wkc;
        }  
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_sm_fmmu_pdo2]
 * @description: 配置SM和FMMU提供PDO通信方式2 映射顺序为RX1，RX2，RX3...RXn,TX1,TX2,TX3...TXn
 * @note: 映射以字节映射
 * @author: gxf
 * @param [uint8_t] *startaddr 用于映射的内存的起始地址 
 * @param [uint32_t] len 用于映射的地址大小
 * @return [*] <=0：配置失败
 * @==============================================================================: 
 */
int mecm_set_sm_fmmu_pdo2(uint8_t *startaddr,uint32_t len)
{
    uint16_t slave = 1;
    int wkc = 0;
    uint8_t i = 0;
    uint8_t *endaddr = startaddr+len;
    if(startaddr == 0)
    {
        return 0;
    }  
    mecm_fmmu_reg_t fmmuconfig;
    memset(startaddr,0,len);
    for ( slave = 1; slave <= mecm_slave_count; slave++)
    {
        /* 配置PDO使用的SM，并根据SM的配置来获取FMMU的配置　*/
        for(i=0;i<mecm_slave[slave].nSM;i++)
        {
            /* 用于RPDO的SM */
            if(mecm_slave[slave].rSMtype[i] == 3)
            {
                memset((uint8_t *)&fmmuconfig,0,sizeof(fmmuconfig));
                fmmuconfig.fmmu_logi_start_addr = (uint32_t)startaddr;
                
                /* 如果程序中RPDO的映射长度不是0，并且长度于EEPROM中的SM的长度不一样，那么以从站中映射的长度为准 */
                if((mecm_slave[slave].rSM[i].sm_length != mecm_slave[slave].rpdobyte)&&(mecm_slave[slave].rpdobyte!=0))
                {
                    mecm_slave[slave].rSM[i].sm_length = mecm_slave[slave].rpdobyte;
                }
                /* SM的长度不为0就激活SM（可能原本EEPROM中的SM长度为0，但是后来程序中的RPDO有改变，长度不为0） */
                if(mecm_slave[slave].rSM[i].sm_length !=0 )
                {
                    mecm_slave[slave].rSM[i].sm_activate.reg_byte = 0x01;
                }
                fmmuconfig.fmmu_length = mecm_slave[slave].rSM[i].sm_length;
                fmmuconfig.fmmu_phys_start_addr = mecm_slave[slave].rSM[i].sm_start_address;
                fmmuconfig.fmmu_logo_start_bit  = 0;
                fmmuconfig.fmmu_logo_stop_bit   = 7;
                fmmuconfig.fmmu_phys_start_bit  = 0;
                fmmuconfig.fmmu_type            = 0x02;
                fmmuconfig.fmmu_activate        = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
                mecm_fmmu_config_add(slave,&fmmuconfig);
                if(fmmuconfig.fmmu_activate)
                {
                    mecm_slave[slave].rpdoaddr = startaddr;
                    mecm_slave[slave].logrpdoaddr = (uint32_t)startaddr;
                    startaddr += fmmuconfig.fmmu_length;
                    /* 超出内存的大小了 */
                    if(startaddr>endaddr)
                    {
                        return 0;
                    }
                }
                wkc = mecm_sm_config(slave,i);
                if(wkc<=0)
                {
                    return wkc;
                }                
            }
        }
    }
    for ( slave = 1; slave <= mecm_slave_count; slave++)
    {
        /* 配置PDO使用的SM，并根据SM的配置来获取FMMU的配置　*/
        for(i=0;i<mecm_slave[slave].nSM;i++)
        {
            /* 用于TPDO的SM */
            if(mecm_slave[slave].rSMtype[i] == 4)   
            {
                memset((uint8_t *)&fmmuconfig,0,sizeof(fmmuconfig));
                fmmuconfig.fmmu_logi_start_addr = (uint32_t)startaddr;
                /* 如果程序中RPDO的映射长度不是0，并且长度于EEPROM中的SM的长度不一样，那么以从站中映射的长度为准 */
                if((mecm_slave[slave].rSM[i].sm_length != mecm_slave[slave].rpdobyte)&&(mecm_slave[slave].tpdobyte!=0))
                {
                    mecm_slave[slave].rSM[i].sm_length = mecm_slave[slave].tpdobyte;
                }
                /* SM的长度不为0就激活SM（可能原本EEPROM中的SM长度为0，但是后来程序中的RPDO有改变，长度不为0） */
                if(mecm_slave[slave].rSM[i].sm_length !=0 )
                {
                    mecm_slave[slave].rSM[i].sm_activate.reg_byte = 0x01;
                }
                fmmuconfig.fmmu_length = mecm_slave[slave].rSM[i].sm_length;
                fmmuconfig.fmmu_phys_start_addr = mecm_slave[slave].rSM[i].sm_start_address;
                fmmuconfig.fmmu_logo_start_bit  = 0;
                fmmuconfig.fmmu_logo_stop_bit   = 7;
                fmmuconfig.fmmu_phys_start_bit  = 0;
                fmmuconfig.fmmu_type            = 0x01;
                fmmuconfig.fmmu_activate        = mecm_slave[slave].rSM[i].sm_activate.reg_byte;
                mecm_fmmu_config_add(slave,&fmmuconfig);
                if(fmmuconfig.fmmu_activate)
                {
                    mecm_slave[slave].tpdoaddr = startaddr;
                    mecm_slave[slave].logtpdoaddr = (uint32_t)startaddr;
                    startaddr += fmmuconfig.fmmu_length;
                    /* 超出内存的大小了 */
                    if(startaddr>endaddr)
                    {
                        return 0;
                    }
                }   
                wkc = mecm_sm_config(slave,i); 
                if(wkc<=0)
                {
                    return wkc;
                }        
            }      
        } 
    }

    for(i=0;i<mecm_slave[slave].nSM;i++)
    {
        /* 配置FMMU */
        for(i = 0;i<mecm_slave[slave].nFMMU;i++)
        {
            wkc = mecm_fmmu_config(slave,i);
            if(wkc<=0)
            {
                return wkc;
            }
        }
        /* 跳转到Safe OP */
        wkc = mecm_state_change(slave,ESC_STATE_SAFE_OP); 
        if(wkc <=0 )
        {
            return wkc;
        }  
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_set_sm_fmmu_pdo]
 * @description: 配置SM和FMMU提供PDO通信方式
 * @note: 映射以字节映射
 * @author: gxf
 * @param [uint8_t] *startaddr 用于映射的内存的起始地址 
 * @param [uint32_t] len 用于映射的地址大小
 * @param [uint8_t] mode 映射方式 1：映射方式1 其他，映射方式2
 * @return [*] <=0：配置失败
 * @==============================================================================: 
 */
int mecm_set_sm_fmmu_pdo(uint8_t *startaddr,uint32_t len,uint8_t mode)
{
    int wkc = 0;
    if(mode == 1)
    {
        wkc = mecm_set_sm_fmmu_pdo1(startaddr,len);
    }else
    {
        wkc = mecm_set_sm_fmmu_pdo2(startaddr,len);
    }
    
}


/**
 * @******************************************************************************: 
 * @func: [mecm_get_pdo_size]
 * @description: 获取PDO的映射长度
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void mecm_get_pdo_size(void)
{
    uint16_t slave = 1;
    for (slave = 1; slave <= mecm_slave_count; slave++)
    {
        mecm_slave[slave].tpdobit = get_pdo_map(slave, TPDOASSIGN);
        mecm_slave[slave].rpdobit = get_pdo_map(slave, RPDOASSIGN);
        mecm_slave[slave].tpdobyte = (mecm_slave[slave].tpdobit+7)/8;
        mecm_slave[slave].rpdobyte = (mecm_slave[slave].rpdobit+7)/8;
    }
  
}

/**
 * @******************************************************************************: 
 * @func: [mecm_config_other]
 * @description: 获取从站其他一些寄存器信息
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
int mecm_config_other(void)
{
    int wkc = 0;
    uint16_t slave = 1;
    uint16_t configaddr;
    uint16_t data = 0;

    int16_t findslave = 0;
    uint16_t preslave  = 0;
    uint8_t  portcnt   = 0;

    for (slave = 1; slave <= mecm_slave_count; slave++)
    {
        configaddr = mecm_slave[slave].configaddr;
        /* 读取从站是否支持DC */
        wkc = mecm_FPRD(configaddr,ESC_REG_ESCSUP,(uint8_t *)&data,2,COMMU_TIMEOUT);
        if(wkc<=0)
        {
            return 0;
        }
        if((etohs(data)&0x04) == 0x04)
        {
            mecm_slave[slave].dcflag = 1;
        }else
        {
            mecm_slave[slave].dcflag = 0;
        }
        /* 获取从站端口使用情况 */
        wkc = mecm_FPRD(configaddr,ESC_REG_DLSTAT,(uint8_t *)&data,2,COMMU_TIMEOUT);
        if(wkc<=0)
        {
            return 0;
        }
        data = etohs(data);
        mecm_slave[slave].portcnt = 0;
        mecm_slave[slave].activeport = 0;
        /* port0 已经建立通信 */
        if((data&0x0200) == 0x0200)
        {
            mecm_slave[slave].portcnt++;
            mecm_slave[slave].activeport|= 0x01;
        }
        /* port1 已经建立通信 */
        if((data&0x0800) == 0x0800)
        {
            mecm_slave[slave].portcnt++;
            mecm_slave[slave].activeport|= 0x02;
        }
        /* port2 已经建立通信 */
        if((data&0x2000) == 0x2000)
        {
            mecm_slave[slave].portcnt++;
            mecm_slave[slave].activeport|= 0x04;
        }
        /* port3 已经建立通信 */
        if((data&0x8000) == 0x8000)
        {
            mecm_slave[slave].portcnt++;
            mecm_slave[slave].activeport|= 0x08;
        }
        /* 计算当前从站的前一个从站 slave1的前一个从站为0，表示主站 
           默认从站的输入端口是端口0，如果不是端口0，可能会计算错误 */
        mecm_slave[slave].preslave = 0;
        if(slave>1)
        {
            findslave = 0;
            preslave = slave-1;
            do
            {
                /* 根据从站的端口使用数量来判断目标从站是否与该从站相连 */
                portcnt = mecm_slave[preslave].portcnt;
                /* 只是用了一个端口，说明该从站是一个分支的终点，
                该分支与目标从站在某一点中分开，分支占用节点从站的一个端口，所以-1，目标从站与节点从站相连*/
                if(portcnt == 1)
                {
                    findslave--;
                }
                /* 使用了三个端口，是一个节点从站，一个为输入端口，所以+2.
                如果findslave+2=2，目标从站与该节点从站相连
                如果findslave+2=1，目标从站与该节点从站相连
                如果findslave+2=0，节点从站两个端口都被占用了，目标从站没有与节点相连 */
                if(portcnt == 3)
                {
                    findslave+=2;
                }
                /* 使用了四个端口，一个为输入端口，所以+3.
                如果那么findslave+3=3，目标从站与该节点相连
                如果那么findslave+3=2，目标从站与该节点相连
                如果那么findslave+3=1，目标从站与该节点相连
                如果那么findslave+3=0，目标从站没有该节点相连 */                
                if(portcnt == 4)
                {
                    findslave+=3;
                }
                if(((findslave>=1)&&(portcnt>1))||(preslave == 1))
                {
                    mecm_slave[slave].preslave = preslave;
                    break;
                }
                preslave--;
            }while(1);           
        }
    } 
    return wkc;    
}
/**
 * @******************************************************************************: 
 * @func: [mecm_state_change]
 * @description: 状态改变
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号 0:广播请求所有的从站
 * @param [uint16_t] state 请求状态
 * @return [*] <=0：失败
 * @==============================================================================: 
 */
int mecm_state_change(uint16_t slave,uint16_t state)
{
    uint16_t configaddr = mecm_slave[slave].configaddr;
    int wkc = 0;
    state = htoes(state);
    if(slave == 0)
    {
        wkc = mecm_BWR(0,ESC_REG_ALCTL,(uint8_t *)&state,2,COMMU_TIMEOUT);
    }else
    {
        wkc = mecm_FPWR(configaddr,ESC_REG_ALCTL,(uint8_t *)&state,2,COMMU_TIMEOUT);
    }
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_state_read]
 * @description: 从站状态读取
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号，0表示所有广播读取所有从站
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_state_read(uint16_t slave)
{
    uint16_t i = 0;
    uint16_t slavestate = 0;
    uint16_t code = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    int wkc = 0;
    if(slave == 0)
    {       
        wkc = mecm_BWR(0,ESC_REG_ALCTL,(uint8_t *)&slavestate,2,COMMU_TIMEOUT);
        mecm_slave[slave].state = slavestate;
        /* 有从站读取失败，则认为全部都失败 */
        if(wkc>0)
        {
            if(wkc != mecm_slave_count)
            {
                wkc = 0;
            }else
            {
                /* 从站有错误，轮询一下错误码 */
                if(mecm_slave[slave].state&0x10)
                {
                    for (i = 1; i < mecm_slave_count; i++)
                    {
                        configaddr = mecm_slave[i].configaddr;
                        mecm_FPRD(configaddr,ESC_REG_ALSTATCODE,(uint8_t *)&code,2,COMMU_TIMEOUT);
                        mecm_slave[i].alstatuscode = code;                        
                    }
                }
            }            
        }       
    }else
    {
        wkc = mecm_FPRD(configaddr,ESC_REG_ALCTL,(uint8_t *)&slavestate,2,COMMU_TIMEOUT);
        if(wkc>0)
        {
            mecm_slave[slave].state = slavestate;
            /* 从站有错误，读一下错误码 */
            if(mecm_slave[slave].state&0x10)
            {
                mecm_FPRD(configaddr,ESC_REG_ALSTATCODE,(uint8_t *)&code,2,COMMU_TIMEOUT);
                mecm_slave[slave].alstatuscode = code;
            }
        }
    }
    return wkc;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_config_init]
 * @description: 从站初始化配置
 * @note: 
 * @author: gxf
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
uint8_t mapbuf[100];
void mecm_config_init(void)
{
    /* 内存初始化 */
    mecm_memery_init();
    /* 扫描从站 并初始化从站进入Init */
    mecm_scan_slave();
    /* 配置地址 */
    mecm_set_configaddr();
    /* 读取EEPROM信息 并请求preop */
    mecm_get_eeprom_info_all();
    /* 获取从站一些寄存器的信息 */
    mecm_config_other();
    /* 配置邮箱 */
    mecm_set_sm_mailbox();
    /* 获取从站的PDO映射 */
    mecm_get_pdo_size();
    /* 设置FMMU映射和过程数据使用的SM　*/
    mecm_set_sm_fmmu_pdo(mapbuf,100,1);
    /* 请求OP */
    mecm_state_change(1,ESC_STATE_OPERATIONAL);
    mecm_processdata_all(1,1,20000);
}
