/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Kernel
*
*                             (c) Copyright 2009; Silicon Laboratories Inc.
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
*                                         KERNEL PORT SELECTOR
*
* File : os_port_sel.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _OS_PORT_SEL_H_
#define  _OS_PORT_SEL_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_opt_def.h>
#include  <rtos/common/include/rtos_path.h>

#ifndef OS_PORT_PATH
#if   (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V5)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv5/realview/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv5/iar/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv7-ar/armcc/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_CCS)
#include  "../source/ports/armv7-ar/ccs/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/armv7-ar/gnu/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv7-ar/iar/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/armv7-m/armcc/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_CCS)
#include  "../source/ports/armv7-m/ccs/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/armv7-m/gnu/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/armv7-m/iar/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_POSIX)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/posix/gnu/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_WIN32)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_ARMCC)
#include  "../source/ports/win32/armcc/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_CCS)
#include  "../source/ports/win32/ccs/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/win32/gnu/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/win32/iar/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_VISUALSTUDIO)
#include  "../source/ports/win32/visualstudio/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_RENESAS_RX)
#if   (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_GNU)
#include  "../source/ports/rx/gnu/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_IAR)
#include  "../source/ports/rx/iar/os_cpu.h"
#elif (RTOS_TOOLCHAIN_NAME == RTOS_TOOLCHAIN_RXC)
#include  "../source/ports/rx/rxc/os_cpu.h"
#else
#warning  "Unknown toolchain"
#endif
#elif (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMPTY)
#include  "../source/ports/empty/os_cpu.h"
#else
#warning  "Unknown port"
#endif
#else
#include  OS_PORT_PATH
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of Kernel port selector module include.          */
