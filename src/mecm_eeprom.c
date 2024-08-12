/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master EEPROM模块,用于对从站EEPROM进行读写
 * @Autor: gxf
 * @Date: 2023-09-26 19:45:03
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-25 19:39:28
 * @==============================================================================: 
 */

#include <string.h>
#include "mecm_eeprom.h"
#include "mecm_config.h"
#include "mecm_port.h"
#include "mecm_base.h"

eeprom_info_t     eep_info;
eeprom_generals_t eep_generals;
eeprom_strings_t  eep_strings;
eeprom_sm_t eep_sm[MAX_SM_NUM]; 
uint8_t eep_nsm = 0;   /* 从站信息中SM的个数　*/
static uint8_t readbuf[50];

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_master]
 * @description: 主站接管EEPROM接口
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*]
 * @==============================================================================: 
 */
int mecm_eeprom_master(uint16_t slave)
{
    int wkc = 0,cnt = 0;
    uint8_t data = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    data = 2;
    /* 强制释放PDI的控制权 */
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CONFIG,&data,1,COMMU_TIMEOUT);
    } while((wkc <=0 )&&(cnt++ < RETYR_TIMES));    
    /* 获取控制权 */
    data = 0;
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CONFIG,&data,1,COMMU_TIMEOUT);
    } while((wkc <=0 )&&(cnt++ < RETYR_TIMES));
    return wkc;		
}

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_pdi]
 * @description: 从站MCU接管EEPROM接口
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*]
 * @==============================================================================: 
 */
int mecm_eeprom_pdi(uint16_t slave)
{
    int wkc = 0,cnt = 0;
    uint16_t configaddr = mecm_slave[slave].configaddr;
    uint8_t data = 1;
    /* 强制释放PDI的控制权 */
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CONFIG,&data,1,COMMU_TIMEOUT);
    } while((wkc <=0 )&&(cnt++ < RETYR_TIMES));
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_read]
 * @description: EEPROM读取
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint32_t] eeprom_addr 待读取的EEPROM地址
 * @param [uint64_t] *readdata 读取数据存放的地址
 * @return [*] <=0 读取失败
 * @==============================================================================: 
 */
int mecm_eeprom_read(uint16_t slave,uint32_t eeprom_addr,uint64_t *readdata)
{
    uint16_t configaddr = mecm_slave[slave].configaddr;
    uint16_t data = 0;
    int wkc = 0;
    int cnt = 0;
    uint8_t bitflag64 = 0;
    uint8_t buf[6];
    uint16_t cmd = 0x0100;
    uint64_t start_time = mecm_current_time_us();
    uint64_t stop_time = start_time+EEPROM_TIMEOUT;
    uint64_t readdata64 = 0;
    uint32_t readdata32 = 0;
    /* 等待EEPROM接口空闲 */
    do
    {
        /* 第一次发送没有延时，下次循环发送之前先延时一下 */
        if(cnt++) mecm_delay_us(300);
        /* 读取寄存器的值 */
        wkc = mecm_FPRD(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        /* 将数据转换为本地字节序 */
        data = etohs(data);
    } while (((wkc<=0)||(data&0x8000))&&(mecm_current_time_us()<stop_time));
    /* 通讯失败或者EEPROM接口忙，返回-1*/
    if((wkc<=0)||(data&0x8000))
        return -1;
    /* 支持字节数是8字节还是4字节 */
    bitflag64 = (data&0x0040)>>6;
    mecm_slave[slave].eep_read_64bit = bitflag64;
    /* 判断EEPROM 是否有错误，有的话就清除错误 */
    if(data&0x6000)
    {
        data = 0x0000;
        data = htoes(data);
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        if(wkc<=0) 
            return -1;
    }
    /* 发送EEPROM地址和读取指令 */
    cmd = 0x0100;
    buf[0] = htoes(cmd)&0xFF;
    buf[1] = (htoes(cmd)>>8)&0xFF;
    buf[2] = htoel(eeprom_addr)&0xFF;
    buf[3] = (htoel(eeprom_addr)>>8)&0xFF;
    buf[4] = (htoel(eeprom_addr)>>16)&0xFF;
    buf[5] = (htoel(eeprom_addr)>>24)&0xFF;
    cnt = 0;
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CTR_STA,buf,6,COMMU_TIMEOUT);
    } while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
    if(wkc<=0) 
        return -1;
    /* 忙等待 */
    cnt = 0;
    start_time = mecm_current_time_us();
    stop_time = start_time+EEPROM_TIMEOUT;
    do
    {
        /* 第一次发送没有延时，下次循环发送之前先延时一下 */
        if(cnt++) mecm_delay_us(300);
        /* 读取寄存器的值 */
        wkc = mecm_FPRD(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        /* 将数据转换为本地字节序 */
        data = etohs(data);
    } while (((wkc<=0)||(data&0x8000))&&(mecm_current_time_us()<stop_time));   
    if((wkc<=0)||(data&0x8000))
        return -1;
    /* 读取数据 */
    if(bitflag64)
    {
        cnt = 0;
        do
        {
            wkc = mecm_FPRD(configaddr,ESC_EEPROM_DATA,(uint8_t *)&readdata64,8,COMMU_TIMEOUT);
        } while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
        *readdata = etohll(readdata64);
        return wkc;
    }else
    {
        cnt = 0;
        do
        {
            wkc = mecm_FPRD(configaddr,ESC_EEPROM_DATA,(uint8_t *)&readdata32,4,COMMU_TIMEOUT);
        }while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
        *readdata = (uint64_t)etohll(readdata32);
        return wkc;
    }    
}


/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_write]
 * @description: 写入EEPROM数据
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint32_t] eeprom_addr EEPROM地址
 * @param [uint16_t] writedata 待写入数据
 * @return [*] <=0:写入失败
 * @==============================================================================: 
 */
int mecm_eeprom_write(uint16_t slave,uint32_t eeprom_addr,uint16_t writedata)
{
    uint16_t configaddr = mecm_slave[slave].configaddr;
    uint16_t data = 0;
    int wkc = 0;
    int cnt = 0;
    uint16_t cmd = 0x0100;
    uint64_t start_time = mecm_current_time_us();
    uint64_t stop_time = start_time+EEPROM_TIMEOUT;
    /* 等待EEPROM接口空闲 */
    do
    {
        /* 第一次发送没有延时，下次循环发送之前先延时一下 */
        if(cnt++) mecm_delay_us(300);
        /* 读取寄存器的值 */
        wkc = mecm_FPRD(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        /* 将数据转换为本地字节序 */
        data = etohs(data);
    } while (((wkc<=0)||(data&0x8000))&&(mecm_current_time_us()<stop_time));
    /* 通讯失败或者EEPROM接口忙，返回-1*/
    if((wkc<=0)||(data&0x8000))
        return -1;
    /* 判断EEPROM 是否有错误，有的话就清除错误 */
    if(data&0x6000)
    {
        data = 0x0000;
        data = htoes(data);
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        if(wkc<=0) 
            return -1;
    }
    /* 发送EEPROM地址 */
    eeprom_addr = htoel(eeprom_addr);
    cnt = 0;
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_ADDRESS,(uint8_t *)&eeprom_addr,4,COMMU_TIMEOUT);
    } while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
    if(wkc<=0) 
        return -1;
    /* 发送写数据 */
    writedata = htoes(writedata);
    cnt = 0;
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_DATA,(uint8_t *)&writedata,2,COMMU_TIMEOUT);
    } while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
    if(wkc<=0) 
        return -1;
    /* 发送写指令 */   
    cmd = 0x0201;
    cmd = htoes(cmd);
    cnt = 0;
    do
    {
        wkc = mecm_FPWR(configaddr,ESC_EEPROM_DATA,(uint8_t *)&cmd,2,COMMU_TIMEOUT);
    } while ((wkc<=0)&&(cnt++ < RETYR_TIMES));
    if(wkc<=0) 
        return -1;    
    /* 忙等待 */
    cnt = 0;
    start_time = mecm_current_time_us();
    stop_time = start_time+EEPROM_TIMEOUT;
    do
    {
        /* 第一次发送没有延时，下次循环发送之前先延时一下 */
        if(cnt++) mecm_delay_us(300);
        /* 读取寄存器的值 */
        wkc = mecm_FPRD(configaddr,ESC_EEPROM_CTR_STA,(uint8_t *)&data,2,COMMU_TIMEOUT);
        /* 将数据转换为本地字节序 */
        data = etohs(data);
    } while (((wkc<=0)||(data&0x8000))&&(mecm_current_time_us()<stop_time));   
    if((wkc<=0)||(data&0x8000))
        return -1;
    if(data&0x6000)
        return -1;
    return wkc;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_read_buf]
 * @description: EEPROM 连续读取
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint32_t] eeprom_addr EEPROM地址
 * @param [uint8_t] *buf 数据存储地址
 * @param [uint32_t] len 读取数据的长度，单位字节
 * @return [*] <=0 ：读取失败 
 * @==============================================================================: 
 */
int mecm_eeprom_read_buf(uint16_t slave,uint32_t eeprom_addr,uint8_t *buf,uint32_t len)
{
    int i = 0;
    int j = 0;
    int wkc = 0;
    int step = 0;
    uint64_t readdata = 0;
    /* 主站获取控制权 */
    wkc = mecm_eeprom_master(slave);
    if(wkc <=0 ) return -1;
    /* 读取数据 */
    for(i = 0;i<len;i+=step)
    {
        wkc = mecm_eeprom_read(slave,eeprom_addr,&readdata);
        if(wkc <=0 ) return -1;
        /* 8字节读取 */
        if(mecm_slave[slave].eep_read_64bit)
        {
            if((len-i)<8)
            {
                for(j=0;j<len-i;j++)
                {
                    buf[i+j] = readdata&0xFF;
                    readdata = readdata>>8;
                }
            }else
            {
                buf[i]   = readdata&0xFF;
                buf[i+1] = (readdata >> 8)&0xFF;
                buf[i+2] = (readdata >> 16)&0xFF;
                buf[i+3] = (readdata >> 24)&0xFF;
                buf[i+4] = (readdata >> 32)&0xFF;
                buf[i+5] = (readdata >> 40)&0xFF;
                buf[i+6] = (readdata >> 48)&0xFF;
                buf[i+7] = (readdata >> 56)&0xFF;
            }
            step = 8;
            eeprom_addr+=4;
        }else
        {
            if((len-i)<4)
            {
                for(j=0;j<len-i;j++)
                {
                    buf[i+j] = readdata&0xFF;
                    readdata = readdata>>8;
                }
            }else
            {
                buf[i]   = readdata&0xFF;
                buf[i+1] = (readdata >> 8)&0xFF;
                buf[i+2] = (readdata >> 16)&0xFF;
                buf[i+3] = (readdata >> 24)&0xFF;
            }
            step = 4;
            eeprom_addr+=2;
        }       
    }
    /* 交出控制权 */
    wkc = mecm_eeprom_pdi(slave);
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_write]
 * @description: EEPROM 连续写
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint32_t] eeprom_addr EEPROM地址
 * @param [uint8_t] *buf　待写入数据
 * @param [uint32_t] len　数据长度，单位字节，2的倍数
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_write_buf(uint16_t slave,uint32_t eeprom_addr,uint8_t *buf,uint32_t len)
{
    int i = 0;
    int wkc = 0;
    uint16_t writedata = 0;
    if(len%2 !=0 ) return -1;
    /* 主站获取控制权 */
    wkc = mecm_eeprom_master(slave);
    if(wkc <=0 ) return -1; 
    for ( i = 0; i < len; i+=2)
    {
        writedata = buf[i]|(buf[i+1]<<8);
        mecm_eeprom_write(slave,eeprom_addr,writedata);
        eeprom_addr++;
    }
    /* 交出控制权 */
    wkc = mecm_eeprom_pdi(slave);
    return wkc;      
}

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_findsii]
 * @description: 查找SII信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @param [uint16_t] types 待查找的信息编号 
 * @param [uint32_t] *getaddr 信息数据所在的地址
 * @param [uint16_t] *getlength 信息数据的长度
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_findsii(uint16_t slave,uint16_t types,uint32_t *getaddr,uint16_t *getlength)
{
    uint32_t start_addr = 0x0040;
    uint16_t length = 0;
    uint16_t currenttypes  = 0;
    /* 查找字符串类型所在的位置 */
    do
    {
        start_addr = start_addr + length;
        mecm_eeprom_read_buf(slave,start_addr,readbuf,4);
        /* 数据类型 */
        currenttypes  = readbuf[0]|(readbuf[1]<<8);
        /* 数据长度 */
        length = readbuf[2]|(readbuf[3]<<8);
        /* 数据起始位置 */
        start_addr = start_addr+2;
    } while ((currenttypes != types)&&(currenttypes != tEND));  
    /* 类型不存在 */
    if(types == tEND)
        return -1; 
    *getaddr =  start_addr;
    *getlength = length;
    return 1;  
}


/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_get_sii_info]
 * @description: 获取接口信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_get_sii_info(uint16_t slave)
{
    int ret = 0;
    uint32_t eeprom_addr = 0;
    memset(readbuf,0,20);
    /* 读取从站别名 */
    eeprom_addr = 0x0004;
    ret = mecm_eeprom_read_buf(slave,eeprom_addr,readbuf,2);
    if(ret<=0) return ret;
    eep_info.aliasaddr = readbuf[0]|(readbuf[1]<<8);
    /* 读取Bootstrap邮箱配置 */
    eeprom_addr = 0x0014;
    memset(readbuf,0,20);
    ret = mecm_eeprom_read_buf(slave,eeprom_addr,readbuf,18);
    if(ret<=0) return ret;
    eep_info.bootstrap_wo = readbuf[0]|(readbuf[1]<<8);
    eep_info.bootstrap_wl = readbuf[2]|(readbuf[3]<<8);
    eep_info.bootstrap_ro = readbuf[4]|(readbuf[5]<<8);
    eep_info.bootstrap_rl = readbuf[6]|(readbuf[7]<<8);
    eep_info.standard_wo  = readbuf[8]|(readbuf[9]<<8);
    eep_info.standard_wl  = readbuf[10]|(readbuf[11]<<8);
    eep_info.standard_ro  = readbuf[12]|(readbuf[13]<<8);
    eep_info.standard_rl  = readbuf[14]|(readbuf[15]<<8);
    eep_info.mbx_protocol = readbuf[16]|(readbuf[17]<<8);
    return ret;
}





/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_get_strings]
 * @description: 获取字符串信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_get_strings(uint16_t slave)
{
    uint32_t eeprom_addr = 0;
    uint16_t length = 0;
    uint8_t i = 0;
    int wkc = 0;

    uint16_t byte_addr = 0; /* 字节地址，在EEPROM中的第几个字节 */
    uint16_t byte_lens = 0; /* 字符串长度，单位字节 */
    /* 查找字符串类型所在的位置 */
    wkc = mecm_eeprom_findsii(slave,tSTRINGS,&eeprom_addr,&length);
    if(wkc <= 0) 
        return -1;
    /* 根据索引号来读取字符串中的名字 */
    if(eep_generals.nameidx == 0)
        return -1;
    i = eep_generals.nameidx;
    /* 转换为字节地址,获取第一个字符串的长度在第多少字节 */
    byte_addr = eeprom_addr*2+1;
    i = 1;
    while(1)
    {
        /* 读取字符串长度 */
        mecm_eeprom_read_buf(slave,byte_addr>>1,readbuf,2);
        /* 奇数地址，那么长度保存在第二个字节 */
        if(byte_addr&0x01)
            byte_lens = readbuf[1];
        else
            byte_lens = readbuf[0];
        byte_addr++;
        /* 读取对应索引的字符串 */
        if(i == eep_generals.nameidx)
        {           
            
            memset(eep_strings.slavename,0,NAMEMAXLEN);
            if(byte_addr&0x01)
            {
                /* 多读了一个字节，所以读取长度+1 */
                wkc = mecm_eeprom_read_buf(slave,byte_addr>>1,readbuf,byte_lens+1);
                memcpy(eep_strings.slavename,&(readbuf[1]),byte_lens);
            }
            else
            {
                wkc = mecm_eeprom_read_buf(slave,byte_addr>>1,readbuf,byte_lens+1);
                memcpy(eep_strings.slavename,readbuf,byte_lens);                
            }

            break;
        }
        i++;
        byte_addr = byte_addr+byte_lens;
    }
    return wkc;
}


/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_get_generals]
 * @description: 获取通用信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_get_generals(uint16_t slave)
{
    uint32_t eeprom_addr = 0;
    uint16_t length = 0;
    int wkc = 0;
    /* 查找general类型所在的位置 */
    wkc = mecm_eeprom_findsii(slave,tGeneral,&eeprom_addr,&length);
    if(wkc <= 0) 
        return -1;   
    wkc = mecm_eeprom_read_buf(slave,eeprom_addr,readbuf,length*2);
    if(wkc <=0)
        return -1;
    memcpy(&eep_generals,readbuf,32);
    /* 大小端转换 */
    eep_generals.current_EBus = etohs(eep_generals.current_EBus);
    eep_generals.phy_port = etohs(eep_generals.phy_port);
    return wkc;
}

/**
 * @******************************************************************************: 
 * @func: [mecm_eeprom_get_sm]
 * @description: 获取SM信息
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号
 * @return [*] <=0:失败
 * @==============================================================================: 
 */
int mecm_eeprom_get_sm(uint16_t slave)
{
    uint32_t eeprom_addr = 0;
    uint16_t length = 0;
    int i = 0;
    int wkc = 0;
    /* 查找SM类型所在的位置 */
    wkc = mecm_eeprom_findsii(slave,tSyncM,&eeprom_addr,&length);
    if(wkc <= 0) 
        return -1;    
    
    for(i=0;i<(length/4);i++)
    {
        eep_nsm++;
        wkc = mecm_eeprom_read_buf(slave,eeprom_addr,readbuf,8);   
        if(wkc <=0)
            return -1;
        memcpy(&(eep_sm[i]),readbuf,8);
        /* 大小端转换 */
        eep_sm[i].phy_start_addr = etohs(eep_sm[i].phy_start_addr);
        eep_sm[i].length = etohs(eep_sm[i].length);
        eeprom_addr+=4;
    }
    return wkc;

}
