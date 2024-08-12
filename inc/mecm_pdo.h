/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master PDO模块 用于实现PDO通信
 * @Autor: gxf
 * @Date: 2023-11-01 17:45:41
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-02 11:04:57
 * @==============================================================================: 
 */
#ifndef _MECM_PDO_H
#define _MECM_PDO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"	

int mecm_processdata_all(uint8_t mapping_mode,uint8_t get_alstate, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif
