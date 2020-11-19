################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.c 

OBJS += \
./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.o 

C_DEPS += \
./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.d 


# Each subdirectory must supply rules for building sources it contributes
Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.o: ../Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM C Compiler'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -std=c99 '-DEFM32GG11B820F2048GL192=1' '-D__VFP_FP__=1' '-DDEBUG_EFM=1' '-DRETARGET_VCOM=1' '-DDEBUG=1' -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/App" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/App/cfg" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/App/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/rtos/kernel/source/ports/armv7-m/gnu" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/cfg" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/include" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/examples" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/middleware/glib/dmd" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/common/bsp" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/common/drivers" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/SLSTK3701A_EFM32GG11/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/CMSIS/Include" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/Device/SiliconLabs/EFM32GG11B/Include" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/common/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/dmadrv/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/dmadrv/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/gpiointerrupt/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/nvm/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/nvm/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/rtcdrv/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/rtcdrv/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/sleep/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/spidrv/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/spidrv/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/uartdrv/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/uartdrv/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/ustimer/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/ustimer/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emlib/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/middleware/glib" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/middleware/glib/glib" -O0 -Wall -c -fmessage-length=0 -mno-sched-prolog -fno-builtin -ffunction-sections -fdata-sections -MMD -MP -MF"Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.d" -MT"Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.o" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


