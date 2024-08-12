/*
 * @******************************************************************************: 
 * @Description: 
 * @Version: v1.0.0
 * @Autor: gxf
 * @Date: 2023-03-21 16:56:24
 * @LastEditors: gxf
 * @LastEditTime: 2023-11-01 21:10:45
 * @==============================================================================: 
 */
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "bsp_systick.h"
#include "md_app.h"




int main(void)
{ 
    bsp_init();
    while(1)
    {
        test();
    }
}
