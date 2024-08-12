/*
 * @******************************************************************************: 
 * @Description: Mini Ethercat Master type模块 用于实现自定义类型
 * @Autor: gxf
 * @Date: 2023-06-02 16:02:42
 * @LastEditors: gxf
 * @LastEditTime: 2023-10-27 18:58:19
 * @==============================================================================: 
 */


#ifndef _MECM_TYPE_H
#define _MECM_TYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>	

enum
{
  TYPE_MBX_ERR = 0x00,  /* 错误 */
  TYPE_MBX_EOE = 0x02,  /* 邮箱协议为EOE*/
  TYPE_MBX_COE,         /* 邮箱协议为COE*/
  TYPE_MBX_FOE,         /* 邮箱协议为FOE*/
  TYPE_MBX_SOE          /* 邮箱协议为SOE*/
};

/* 默认小端 */
#define MECM_LITTLE_ENDIAN

/* 大小端的处理 默认定义主机是小端 */
#if !defined(MECM_BIG_ENDIAN) && defined(MECM_LITTLE_ENDIAN)
  /* 将本地数据转换为小端字节序 */
  #define htoes(A) (A)
  #define htoel(A) (A)
  #define htoell(A) (A)
  /* 将小端转换为本地字节序*/
  #define etohs(A) (A)
  #define etohl(A) (A)
  #define etohll(A) (A)
  /* 将数据转换为网络字节序 */
  #define htons(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
                    (((uint16_t)(A) & 0x00ff) << 8))
  #define htonl(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
                    (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                    (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                    (((uint32_t)(A) & 0x000000ff) << 24))
  #define htonll(A) ((((uint64_t)(A) & (uint64)0xff00000000000000ULL) >> 56) | \
                     (((uint64_t)(A) & (uint64)0x00ff000000000000ULL) >> 40) | \
                     (((uint64_t)(A) & (uint64)0x0000ff0000000000ULL) >> 24) | \
                     (((uint64_t)(A) & (uint64)0x000000ff00000000ULL) >> 8)  | \
                     (((uint64_t)(A) & (uint64)0x00000000ff000000ULL) << 8)  | \
                     (((uint64_t)(A) & (uint64)0x0000000000ff0000ULL) << 24) | \
                     (((uint64_t)(A) & (uint64)0x000000000000ff00ULL) << 40) | \
                     (((uint64_t)(A) & (uint64)0x00000000000000ffULL) << 56))
#elif defined(MECM_BIG_ENDIAN) && !defined(MECM_LITTLE_ENDIAN)
  /* 将本地数据转换为小端字节序 */
  #define htoes(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
                    (((uint16_t)(A) & 0x00ff) << 8))
  #define htoel(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
                    (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                    (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                    (((uint32_t)(A) & 0x000000ff) << 24))
  #define htoell(A) ((((uint64_t)(A) & (uint64)0xff00000000000000ULL) >> 56) | \
                     (((uint64_t)(A) & (uint64)0x00ff000000000000ULL) >> 40) | \
                     (((uint64_t)(A) & (uint64)0x0000ff0000000000ULL) >> 24) | \
                     (((uint64_t)(A) & (uint64)0x000000ff00000000ULL) >> 8)  | \
                     (((uint64_t)(A) & (uint64)0x00000000ff000000ULL) << 8)  | \
                     (((uint64_t)(A) & (uint64)0x0000000000ff0000ULL) << 24) | \
                     (((uint64_t)(A) & (uint64)0x000000000000ff00ULL) << 40) | \
                     (((uint64_t)(A) & (uint64)0x00000000000000ffULL) << 56))
  /* 将小端转换为本地字节序*/
  #define etohs  htoes
  #define etohl  htoel
  #define etohll htoell 
  /* 将数据转换为网络字节序 */
  #define htons(A) (A)
  #define htonl(A) (A)
  #define htonll(A) (A)
#else
  #error "Must define one of MECM_BIG_ENDIAN or MECM_LITTLE_ENDIAN"
#endif


#ifdef __cplusplus
}
#endif

#endif

