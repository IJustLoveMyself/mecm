/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master DC同步模块
 * @Autor: gxf
 * @Date: 2023-09-26 19:46:04
 * @LastEditors: gxf
 * @LastEditTime: 2024-04-08 11:06:11
 * @==============================================================================: 
 */
#include <string.h>
#include "mecm_dc.h"
#include "mecm_config.h"
#include "mecm_base.h"
#include "mecm_port.h"

slave_dc_t dc_slave[MAX_SLAVE_NUM];


static uint8_t mecm_find_pre_port(uint16_t slave,uint8_t currentport)
{
    uint8_t port = 0;
    /* 只使用了一个端口，那么端口为0 */
    if(dc_slave[slave].portcnt <= 1)
    {
        return 0;
    }
    /* 端口查询顺序为 0->2->1->3 */
    switch (currentport)
    {
    case 0:
        if(dc_slave[slave].port[2].active == 1)
        {
            port = 2;
            break;
        }
    case 2:
        if(dc_slave[slave].port[1].active == 1)
        {
            port = 1;
            break;
        }
    case 1:
        if(dc_slave[slave].port[3].active == 1)
        {
            port = 3;
            break;
        }
    case 3:
        port = 0;
        break;    
    default:
        break;
    }
    return port;
}

// uint8_t mecm_find_next_port(uint16_t slave,uint8_t currentport)
// {
//     uint8_t port = 0;
//     /* 只使用了一个端口，那么端口为0 */
//     if(dc_slave[slave].portcnt <= 1)
//     {
//         return 0;
//     }
//     /* 端口查询顺序为 0->3->1->2 */
//     switch (currentport)
//     {
//     case 0:
//         if(dc_slave[slave].port[3].active == 1)
//         {
//             port = 3;
//             break;
//         }
//     case 3:
//         if(dc_slave[slave].port[1].active == 1)
//         {
//             port = 1;
//             break;
//         }
//     case 1:
//         if(dc_slave[slave].port[2].active == 1)
//         {
//             port = 2;
//             break;
//         }
//     case 2:
//         port = 0;
//         break;    
//     default:
//         break;
//     }
//     return port;
// }

static int mecm_find_output_port(uint16_t slave)
{
    /* 只使用了一个端口，那么端口为0 */
    if(dc_slave[slave].portcnt <= 1)
    {
        return 0;
    }
    /* 端口查询顺序为 0->3->1->2 */
    /* 端口3已经稳定通信，并且还没有用来计算过拓扑 */
    if((dc_slave[slave].port[3].active == 1)&&(dc_slave[slave].port[3].use == 0))
    {
        return 3;
    }
    /* 端口2已经稳定通信，并且还没有用来计算过拓扑 */
    if((dc_slave[slave].port[1].active == 1)&&(dc_slave[slave].port[1].use == 0))
    {
        return 1;
    }
    /* 端口1已经稳定通信，并且还没有用来计算过拓扑 */
    if((dc_slave[slave].port[2].active == 1)&&(dc_slave[slave].port[2].use == 0))
    {
        return 2;
    }
    return -1;
}




/**
 * @******************************************************************************: 
 * @func: [mecm_dc_get_topology]
 * @description: 计算整个网络的连接拓扑
 * @note: 拓扑的计算的前提是 所有从站输入端口都是端口0 且所有的从站都包含DC模块
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_dc_get_topology(void)
{
    int wkc = 0;
    uint16_t slave = 1;
    uint16_t slavec = 1;
    uint16_t configaddr;
    uint16_t data = 0;
    int output_port = 0;
    memset(dc_slave,0,sizeof(slave_dc_t)*MAX_SLAVE_NUM);
    /* 检查是否所有的从站都支持DC　*/
    for(slave = 0;slave<=mecm_slave_count;slave++)
    {
        if(mecm_slave[slave].dcflag == 0)
        {
            return 0;
        }
        configaddr = mecm_slave[slave].configaddr;
        /* 获取端口使用情况 */
        wkc = mecm_FPRD(configaddr,ESC_REG_DLSTAT,(uint8_t *)&data,2,COMMU_TIMEOUT);
        if(wkc<=0)
        {
            return 0;
        }
        data = etohs(data);
        /* port[0] 已经建立通信 */
        if((data&0x0200) == 0x0200)
        {
            dc_slave[slave].port[0].active = 1;
            dc_slave[slave].portcnt++;
        }
        /* port[1] 已经建立通信 */
        if((data&0x0800) == 0x0800)
        {
            dc_slave[slave].port[1].active = 1;
            dc_slave[slave].portcnt++;
        }
        /* port[2] 已经建立通信 */
        if((data&0x2000) == 0x2000)
        {
            dc_slave[slave].port[2].active = 1;
            dc_slave[slave].portcnt++;
        }
        /* port[3] 已经建立通信 */
        if((data&0x8000) == 0x8000)
        {
            dc_slave[slave].port[3].active = 1;
            dc_slave[slave].portcnt++;
        } 
    }
    /* 拓扑计算 */
    for(slave = 1;slave<=mecm_slave_count;slave++)
    {
        /* P0 端口是肯定使用了的 */
        dc_slave[slave].port[0].use = 1;     
        dc_slave[slave].usecnt++;   
        output_port = mecm_find_output_port(slave);
        switch (output_port)
        {
        case 0:
            /* 如果是最后一个从站 */
            if(slave == mecm_slave_count)
            {
                break;
            }
            /* 只有一个端口，那么后面的从站就不再这个从站后面，需要找节点从站 */
            slavec = slave;
            do
            {
                /* 离他最近的一个还有多余端口没用的从站就是节点从站 */
                if((dc_slave[slavec].portcnt-dc_slave[slavec].usecnt)>0)
                {
                    break;
                }
                slavec--;
            } while (1);
            dc_slave[slave+1].port[0].preslave = slavec;
            dc_slave[slave+1].port[0].preport = mecm_find_output_port(slavec);
            break;
        case 1:
            /* 记录下使用的端口号 */
            dc_slave[slave].port[1].use = 1;
            dc_slave[slave].usecnt++;
            dc_slave[slave+1].port[0].preslave = slave;
            dc_slave[slave+1].port[0].preport = 1;
            break;
        case 2:
            dc_slave[slave].port[2].use = 1;
            dc_slave[slave].usecnt++;
            dc_slave[slave+1].port[0].preslave = slave;
            dc_slave[slave+1].port[0].preport = 2;
            break;
        case 3:
            dc_slave[slave].port[3].use = 1;
            dc_slave[slave].usecnt++;
            dc_slave[slave+1].port[0].preslave = slave;
            dc_slave[slave+1].port[0].preport = 3;
            break;        
        default:
            break;
        }
    }
  
}

/**
 * @******************************************************************************: 
 * @func: [mecm_dc_get_delaytime]
 * @description: 计算传输延时 
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_dc_get_delaytime(void)
{   
    uint32_t data = 0;
    uint16_t slave = 1;
    uint64_t mastertime = 0;
    uint16_t configaddr;
    uint8_t tdiff = 0;  /* 数据直接转发和经过port[0]的处理单元转发的时间差，平均为20~40ns*/
    uint32_t tdelay_slave = 0;
    uint32_t tdelay_preslave = 0;
    uint32_t tdelay = 0;  /* 两个从站之间的传输延时 */
    uint32_t tdelay2 = 0;  /* 分支节点中端口0与当前端口的延时 */
    uint16_t preslave = 1;
    uint8_t  slave_port = 0;
    uint8_t  slave_inport = 0;
    uint8_t  preslave_port = 0;
    uint8_t  preslave_inport = 0;
    int wkc = 0;
    /* BWR或FPWR 写寄存器0x900执行写操作，从站锁存时间 */
    mecm_BWR(0,ESC_REG_DCTIME0,(uint8_t *)&data,sizeof(uint32_t),COMMU_TIMEOUT);
    /* 读取从站各端口的锁存时间 用于计算传输延时 */
    for(slave = 1;slave<=mecm_slave_count;slave++)
    {
        configaddr = mecm_slave[slave].configaddr;
        wkc = mecm_FPRD(configaddr,ESC_REG_DCTIME0,(uint8_t *)&data,4,COMMU_TIMEOUT);
        dc_slave[slave].port[0].porttime = etohl(data);
        wkc = mecm_FPRD(configaddr,ESC_REG_DCTIME1,(uint8_t *)&data,4,COMMU_TIMEOUT);
        dc_slave[slave].port[1].porttime = etohl(data);
        wkc = mecm_FPRD(configaddr,ESC_REG_DCTIME2,(uint8_t *)&data,4,COMMU_TIMEOUT);
        dc_slave[slave].port[2].porttime = etohl(data);
        wkc = mecm_FPRD(configaddr,ESC_REG_DCTIME3,(uint8_t *)&data,4,COMMU_TIMEOUT);
        dc_slave[slave].port[3].porttime = etohl(data);
    } 
    /* 从站1和主站相连，默认传输延时为 0，如果想要提高精度，可以测量主站与第一个从站之间的延时 */
    dc_slave[1].delaytime = 0;
    configaddr = mecm_slave[1].configaddr;
    data = htoel(dc_slave[1].delaytime);
    wkc = mecm_FPWR(configaddr,ESC_REG_DCSYSDELAY,(uint8_t *)&data,4,COMMU_TIMEOUT);
    /* 计算传输延时 默认所有的从站都支持DC，所以直接选取从站1作为参考从站*/
    for(slave = 2;slave<=mecm_slave_count;slave++) 
    {
        configaddr = mecm_slave[slave].configaddr;
        /* 默认数据流入是从端口0开始 */
        slave_port = 0;
        /* 前一个使用的端口  */
        slave_inport = mecm_find_pre_port(slave,slave_inport);
        /* 相连的前一个从站 */
        preslave = dc_slave[slave].port[0].preslave;
        /* 前一个从站的数据流出端口 */
        preslave_port = dc_slave[slave].port[0].preport;
        /* 查前一个使用的端口 */
        preslave_inport = mecm_find_pre_port(preslave,preslave_port);
        /* 两个相连的从站的编号差值不是1，说明上一个从站是节点从站，其上还有其他分支 */
        if(slave - preslave>1)
        {
           if(dc_slave[preslave].port[preslave_port].porttime<dc_slave[preslave].port[preslave_inport].porttime)
            {
                /* 4个端口的计数是32位的，当累加到4294967295的时候会重新从0计数 */
                tdelay_preslave = 4294967296-dc_slave[preslave].port[preslave_inport].porttime+dc_slave[preslave].port[preslave_port].porttime;
            }else
            {
                tdelay_preslave = dc_slave[preslave].port[preslave_port].porttime-dc_slave[preslave].port[preslave_inport].porttime;
            }
            if(dc_slave[slave].port[slave_inport].porttime<dc_slave[slave].port[slave_port].porttime)
            {
                /* 4个端口的计数是32位的，当累加到4294967295的时候会重新从0计数 */
                tdelay_slave = 4294967296-dc_slave[slave].port[slave_port].porttime+dc_slave[slave].port[slave_inport].porttime;
            }else
            {
                tdelay_slave = dc_slave[slave].port[slave_inport].porttime-dc_slave[slave].port[slave_port].porttime;
            }
            /* 计算分支所用的延时 */
            if(dc_slave[slave_inport].port[preslave_inport].porttime<dc_slave[preslave].port[0].porttime) 
            {
                /* 4个端口的计数是32位的，当累加到4294967295的时候会重新从0计数 */
                tdelay2 = 4294967296-dc_slave[preslave].port[0].porttime+dc_slave[slave_inport].port[preslave_inport].porttime ;

            }else
            {
                tdelay2 = dc_slave[slave_inport].port[preslave_inport].porttime - dc_slave[preslave].port[0].porttime;
            }
            /* 从分支走过来的没有经过数据处理单元，所以没有tdiff */
            tdelay = ( tdelay_preslave-tdelay_slave)/2+tdelay2;                       
        }else
        {
            /* 两个从站是相连的的，中间没有分支 计算两个节点之间的传输延时 */
            if(dc_slave[preslave].port[preslave_port].porttime<dc_slave[preslave].port[preslave_inport].porttime)
            {
                /* 4个端口的计数是32位的，当累加到4294967295的时候会重新从0计数 */
                tdelay_preslave = 4294967296-dc_slave[preslave].port[preslave_inport].porttime+dc_slave[preslave].port[preslave_port].porttime;
            }else
            {
                tdelay_preslave = dc_slave[preslave].port[preslave_port].porttime-dc_slave[preslave].port[preslave_inport].porttime;
            }
            if(dc_slave[slave].port[slave_inport].porttime<dc_slave[slave].port[slave_port].porttime)
            {
                /* 4个端口的计数是32位的，当累加到4294967295的时候会重新从0计数 */
                tdelay_slave = 4294967296-dc_slave[slave].port[slave_port].porttime+dc_slave[slave].port[slave_inport].porttime;
            }else
            {
                tdelay_slave = dc_slave[slave].port[slave_inport].porttime-dc_slave[slave].port[slave_port].porttime;
            }           
            tdelay = ( tdelay_preslave-tdelay_slave+tdiff)/2;         
        }
        dc_slave[slave].delaytime = dc_slave[preslave].delaytime+tdelay;
        data = htoel(dc_slave[slave].delaytime);
        wkc = mecm_FPWR(configaddr,ESC_REG_DCSYSDELAY,(uint8_t *)&data,4,COMMU_TIMEOUT);
    }
}

/**
 * @******************************************************************************: 
 * @func: [mecm_dc_get_offset]
 * @description: 计算时钟偏移
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
static int mecm_dc_get_offset(void)
{
    uint64_t systime = 0;
    uint64_t localtime = 0;
    uint64_t data = 0;
    uint16_t slave = 1;
    uint16_t configaddr;
    int64_t offset = 0;
    int wkc = 0;
    for(slave = 1;slave<=mecm_slave_count;slave++)
    {
        configaddr = mecm_slave[slave].configaddr;
        /* 用主站时间来作为初始化的系统时钟 */
        systime = mecm_current_time_ns();
        data = 0;
        /* 读取从站本地时钟 */
        wkc = mecm_FPRD(configaddr,ESC_REG_DCLOCALTIME,(uint8_t *)&data,8,COMMU_TIMEOUT);
        localtime = etohll(data);
        offset = localtime - systime - dc_slave[slave].delaytime;
        data = htoell(offset); 
        /* 配置时钟偏移 */
        wkc = mecm_FPWR(configaddr,ESC_REG_DCSYSOFFSET,(uint8_t *)&data,8,COMMU_TIMEOUT);
    }
    
}
/**
 * @******************************************************************************: 
 * @func: [mecm_dc_init]
 * @description: DC时钟同步的初始化，包括计算传输延时和时钟偏移
 * @note: 
 * @author: gxf
 * @return [*]
 * @==============================================================================: 
 */
void mecm_dc_init(void)
{
    uint16_t data = 0;
    /* 计算连接拓扑结构 */
    mecm_dc_get_topology();
    /* 配置传输延时 */
    mecm_dc_get_delaytime();
    /* 配置时钟偏移 */
    mecm_dc_get_offset();
    /* 初始化过滤器，初始的值为寄存器复位的默认值 使用默认值就可以满足需求 */
    data = htoes(0x1000);
    mecm_BWR(0x0000,ESC_REG_DCSPEEDCNT,(uint8_t *)&data,sizeof(data),COMMU_TIMEOUT);
    data = htoes(0x0C04);
    mecm_BWR(0x0000,ESC_REG_DCTIMEFILT,(uint8_t *)&data,sizeof(data),COMMU_TIMEOUT);    
}

/**
 * @******************************************************************************: 
 * @func: [mecm_sync0_init]
 * @description: sync0同步信号初始化
 * @note: 
 * @author: gxf
 * @param [uint16_t] slave 从站编号  
 * @param [uint32_t] cycltime 周期时间，单位us
 * @param [uint32_t] starttime 开始时间，单位us
 * @return [*]
 * @==============================================================================: 
 */
void mecm_sync0_init(uint16_t slave,uint32_t cycltime, uint32_t starttime)
{
    int wkc;
    uint16_t configaddr = 0;
    uint8_t data1 = 0;
    uint64_t local_time = 0;
    uint64_t sync0_start_time = 0;
    configaddr = mecm_slave[slave].configaddr;
    /* 转化为ns */
    cycltime = cycltime*1000;
    starttime = starttime*1000;
    /* 首先关闭sync0输出 */
    data1 = 0;
    wkc = mecm_FPWR(configaddr,ESC_REG_DCSYNCACT,(uint8_t *)&data1,1,COMMU_TIMEOUT);
    /* 主站获取访问权限 */
    data1 = 0;
    wkc = mecm_FPWR(configaddr,ESC_REG_DCCUC,(uint8_t *)&data1,1,COMMU_TIMEOUT);
    /* 读取从站的系统时间 */
    wkc = mecm_FPRD(configaddr,ESC_REG_DCSYSTIME,(uint8_t *)&local_time,8,COMMU_TIMEOUT);
    local_time = etohll(local_time);
    /* 设置sync0信号开始的时间 */
    sync0_start_time = ((local_time + cycltime)/cycltime)*cycltime+starttime;
    sync0_start_time = htoell(sync0_start_time);
    wkc = mecm_FPWR(configaddr,ESC_REG_DCSTART0,(uint8_t *)&sync0_start_time,8,COMMU_TIMEOUT);
    /* 设置同步周期 */
    cycltime = htoel(cycltime);
    wkc = mecm_FPWR(configaddr,ESC_REG_DCCYCLE0,(uint8_t *)&cycltime,4,COMMU_TIMEOUT);
    /* 激活sync0 */
    data1 = 0x03;
    wkc = mecm_FPWR(configaddr,ESC_REG_DCSYNCACT,(uint8_t *)&data1,1,COMMU_TIMEOUT);
}