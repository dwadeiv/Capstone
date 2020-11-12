/*
*********************************************************************************************************
*                                              Micrium OS
*                                                 CPU
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           CPU PORT SELECTOR
*
* File : cpu_port_sel.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _CPU_PORT_SEL_H_
#define  _CPU_PORT_SEL_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_opt_def.h>
#include  <rtos/common/include/rtos_path.h>

#ifndef CPU_PORT_PATH
#if   (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V5)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv5/realview/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/armv5/gnu/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv5/iar/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv7-ar/armcc/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_CCS)
#include  "../source/ports/armv7-ar/ccs/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/armv7-ar/gnu/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv7-ar/iar/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv7-m/armcc/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_CCS)
#include  "../source/ports/armv7-m/ccs/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/armv7-m/gnu/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv7-m/iar/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_POSIX)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/posix/gnu/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_WIN32)
#if   ((RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_VISUALSTUDIO) || \
       (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU))
#include  "../source/ports/win32/visualstudio/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_RENESAS_RX)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/rx/gnu/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/rx/iar/cpu_port.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_RXC)
#include  "../source/ports/rx/rxc/cpu_port.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMPTY)
#include  "../source/ports/empty/cpu_port.h"
#else
#warning  "Unknown port"
#endif
#else
#include  CPU_PORT_PATH
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of CPU port selector module include.             */
