################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/startup_efm32gg11b.S 

OBJS += \
./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/startup_efm32gg11b.o 


# Each subdirectory must supply rules for building sources it contributes
Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/%.o: ../Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM Assembler'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -c -x assembler-with-cpp -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\bsp" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\common\drivers" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\hardware\kit\SLSTK3701A_EFM32GG11\config" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\Device\SiliconLabs\EFM32GG11B\Include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emdrv\common\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\emlib\inc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\App\cfg" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\cfg\gcc" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\include" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\examples" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium" -I"C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\SiliconLabs\sdks\exx32\platform\CMSIS\Include" '-DEFM32GG11B820F2048GL192=1' '-D__VFP_FP__=1' '-DRETARGET_VCOM=1' -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


