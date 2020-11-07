################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include emlib/subdir.mk
-include CMSIS/EFM32GG11B/subdir.mk
-include BSP/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Giant_Gecko_blink.axf

# Tool invocations
Giant_Gecko_blink.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "Giant_Gecko_blink.ld" -Xlinker --gc-sections -Xlinker -Map="Giant_Gecko_blink.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o Giant_Gecko_blink.axf "./BSP/bsp_bcc.o" "./BSP/bsp_stk.o" "./BSP/bsp_stk_leds.o" "./BSP/bsp_trace.o" "./CMSIS/EFM32GG11B/startup_efm32gg11b.o" "./CMSIS/EFM32GG11B/system_efm32gg11b.o" "./emlib/em_assert.o" "./emlib/em_cmu.o" "./emlib/em_ebi.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./src/blink.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: Giant_Gecko_blink.hex'
	arm-none-eabi-objcopy -O ihex "Giant_Gecko_blink.axf" "Giant_Gecko_blink.hex"
	@echo ' '

	@echo 'Building bin file: Giant_Gecko_blink.bin'
	arm-none-eabi-objcopy -O binary "Giant_Gecko_blink.axf" "Giant_Gecko_blink.bin"
	@echo ' '

	@echo 'Building s37 file: Giant_Gecko_blink.s37'
	arm-none-eabi-objcopy -O srec "Giant_Gecko_blink.axf" "Giant_Gecko_blink.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "Giant_Gecko_blink.axf" -A
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) Giant_Gecko_blink.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
