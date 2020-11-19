/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Common
*
*                             (c) Copyright 2017; Silicon Laboratories Inc.
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
*                                               RTOS PATH
*
* File : rtos_path.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _RTOS_PATH_H_
#define  _RTOS_PATH_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((!defined(RTOS_CPU_SEL)))
#error  "RTOS_CPU_SEL not defined, please #define RTOS_CPU_SEL to a known MCU. Supported MCUs can be found in common/include/rtos_opt_def.h."
#endif

#if (RTOS_CPU_SEL == RTOS_CPU_SEL_NONE)
#error  "RTOS_CPU_SEL must be #define'd to a known MCU. Supported MCUs can be found in common/include/rtos_opt_def.h."
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/
                                                                /* Auto selection of toolchain.                         */
#if ((!defined(RTOS_TOOLCHAIN_SEL)) || \
     (RTOS_TOOLCHAIN_SEL == RTOS_TOOLCHAIN_AUTO))

#if (defined(__IAR_SYSTEMS_ICC__))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_IAR
#elif defined(_MSC_VER)
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_VISUALSTUDIO
#elif (defined(__ARMCC_VERSION))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_ARMCC
#elif (defined(__RENESAS__))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_RXC
#elif (defined(ADI))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_VDSP
#elif (defined(__C30__))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_MPLAB_C30
#elif (defined(__GNUC__) || defined(__GNUG__))
#  if (defined(__TI_COMPILER_VERSION__))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_CCS
#  elif (defined(__arm__))                                      /* GNU for ARM arch.                                    */
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_GNU
#  elif (defined(__RL78__))                                     /* GNU for RL78 arch.                                   */
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_GNU
#  elif (defined(__RX__))                                       /* GNU for RX arch.                                     */
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_GNU
#  else                                                         /* Generic GNU toolchain.                               */
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_GNU
#  endif
#elif (defined(_ADI_COMPILER))
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN
#else
#error "Unable to detect toolchain. Set RTOS_CFG_TOOLCHAIN."
#endif

#else
#define RTOS_TOOLCHAIN      RTOS_TOOLCHAIN_SEL
#endif


/*
*********************************************************************************************************
*                                   CPU, INTERRUPT & TOOLCHAIN NAMES
*********************************************************************************************************
*/

                                                                /* CPU Selector.                                        */
#if ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM1)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM2)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM3)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM6)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM7)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM8)     || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM7TDMI) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM9TDMI) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM11)    || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V4))
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_ARM_V4

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM7EJ) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM9E)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM10E) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V5))
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_ARM_V5

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A5)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A53)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A57)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A15)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A17)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A7)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A8)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A9)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R4)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R5)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R7)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM41X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM42X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM48X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS0232) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS03x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS04x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS21x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS31x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V7_AR))
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_ARM_V7_AR

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M0)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M0P)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M1)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M3)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M4)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M7)   || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V7_M))
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_ARM_V7_M

#elif (RTOS_CPU_SEL == RTOS_CPU_SEL_RENESAS_RX)
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_RENESAS_RX

#elif (RTOS_CPU_SEL == RTOS_CPU_SEL_EMUL_POSIX)
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_EMUL_POSIX

#elif (RTOS_CPU_SEL == RTOS_CPU_SEL_EMUL_WIN32)
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_EMUL_WIN32

#elif (RTOS_CPU_SEL == RTOS_CPU_SEL_EMPTY)
#define  RTOS_CPU_PORT_NAME             RTOS_CPU_SEL_EMPTY
#else
#error  "Unsupported RTOS_CPU_SEL specified."
#endif


                                                                /* Toolchain Selector.                                  */
#if ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC)              || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30)          || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO)       || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP))
#define  RTOS_TOOLCHAIN_NAME            RTOS_TOOLCHAIN
#else
#error  "Unsupported RTOS_TOOLCHAIN specified."
#endif


                                                                /* CPU Interrupt controller Selector.                   */
#if ((!defined(RTOS_INT_CONTROLLER_SEL)) || \
     (RTOS_INT_CONTROLLER_SEL == RTOS_INT_CONTROLLER_AUTO))

#if ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A5)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A53) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A57) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A15) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A17) || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A7)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A8)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_A9)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R4)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R5)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_R7)  || \
     (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V7_AR))
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_ARMV7_AR_GIC

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M0)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M0P) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M1)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M3)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M4)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M7)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V7_M))
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_ARMV7_M

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_EMUL_POSIX) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_EMUL_WIN32))
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_NONE

#elif (RTOS_CPU_SEL == RTOS_CPU_SEL_RENESAS_RX)
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_RX

#elif ((RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM41X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM42X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_RM48X)        || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS0232) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS03x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS04x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS21x)  || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_TI_TMS570LS31x))
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_ARMV7_AR_VIM95

#else
#warning  "Unable to detect RTOS_INT_CONTROLLER_NAME based on RTOS_CPU_SEL."
#endif

#else
#define  RTOS_INT_CONTROLLER_NAME       RTOS_INT_CONTROLLER_SEL
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of rtos path module include.                     */
