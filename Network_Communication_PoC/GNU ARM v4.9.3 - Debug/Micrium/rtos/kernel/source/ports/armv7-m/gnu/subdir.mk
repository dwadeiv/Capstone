################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.c 

S_UPPER_SRCS += \
../Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_a.S 

OBJS += \
./Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_a.o \
./Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.o 

C_DEPS += \
./Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.d 


# Each subdirectory must supply rules for building sources it contributes
Micrium/rtos/kernel/source/ports/armv7-m/gnu/%.o: ../Micrium/rtos/kernel/source/ports/armv7-m/gnu/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM Assembler'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -c -x assembler-with-cpp -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\bsp" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\drivers" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\SLSTK3701A_EFM32GG11\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\Device\SiliconLabs\EFM32GG11B\Include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\common\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emlib\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App\cfg" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\cfg\gcc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\examples" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\CMSIS\Include" '-DEFM32GG11B820F2048GL192=1' '-D__VFP_FP__=1' '-DRETARGET_VCOM=1' -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.o: ../Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM C Compiler'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -std=c99 '-DEFM32GG11B820F2048GL192=1' '-D__VFP_FP__=1' '-DDEBUG_EFM=1' '-DRETARGET_VCOM=1' '-DDEBUG=1' -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App\cfg" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\rtos\kernel\source\ports\armv7-m\gnu" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\cfg" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\examples" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\middleware\glib\dmd" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\bsp" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\drivers" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\SLSTK3701A_EFM32GG11\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\CMSIS\Include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\Device\SiliconLabs\EFM32GG11B\Include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\common\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\dmadrv\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\dmadrv\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\gpiointerrupt\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\nvm\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\nvm\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\rtcdrv\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\rtcdrv\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\sleep\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\spidrv\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\spidrv\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\uartdrv\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\uartdrv\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\ustimer\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\ustimer\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emlib\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\middleware\glib" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\middleware\glib\glib" -O0 -Wall -c -fmessage-length=0 -mno-sched-prolog -fno-builtin -ffunction-sections -fdata-sections -MMD -MP -MF"Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.d" -MT"Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.o" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


