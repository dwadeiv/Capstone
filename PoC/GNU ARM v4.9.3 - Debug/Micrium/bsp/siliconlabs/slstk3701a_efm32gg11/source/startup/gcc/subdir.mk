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
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -c -x assembler-with-cpp -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/common/bsp" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/common/drivers" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/hardware/kit/SLSTK3701A_EFM32GG11/config" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/Device/SiliconLabs/EFM32GG11B/Include" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emdrv/common/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/emlib/inc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/App" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/App/cfg" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/cfg/gcc" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/include" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium/examples" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/Micrium" -I"/Users/dwadeiv/Documents/Desktop_shit/Capstone/Code/PoC/SiliconLabs/sdks/exx32/platform/CMSIS/Include" '-DEFM32GG11B820F2048GL192=1' '-D__VFP_FP__=1' '-DRETARGET_VCOM=1' -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


