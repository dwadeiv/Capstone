################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/Source_files/subdir.mk
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
all: DPLL.axf

# Tool invocations
DPLL.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "DPLL.ld" -Xlinker --gc-sections -Xlinker -Map="DPLL.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o DPLL.axf "./BSP/bsp_bcc.o" "./BSP/bsp_stk.o" "./BSP/bsp_stk_leds.o" "./BSP/bsp_trace.o" "./CMSIS/EFM32GG11B/startup_efm32gg11b.o" "./CMSIS/EFM32GG11B/system_efm32gg11b.o" "./emlib/em_assert.o" "./emlib/em_cmu.o" "./emlib/em_ebi.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./src/Source_files/HW_delay.o" "./src/Source_files/SI7021.o" "./src/Source_files/app.o" "./src/Source_files/ble.o" "./src/Source_files/cmu.o" "./src/Source_files/gpio.o" "./src/Source_files/i2c.o" "./src/Source_files/letimer.o" "./src/Source_files/leuart.o" "./src/Source_files/scheduler.o" "./src/Source_files/sleep_routines.o" "./src/Source_files/timer.o" "./src/main.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: DPLL.hex'
	arm-none-eabi-objcopy -O ihex "DPLL.axf" "DPLL.hex"
	@echo ' '

	@echo 'Building bin file: DPLL.bin'
	arm-none-eabi-objcopy -O binary "DPLL.axf" "DPLL.bin"
	@echo ' '

	@echo 'Building s37 file: DPLL.s37'
	arm-none-eabi-objcopy -O srec "DPLL.axf" "DPLL.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "DPLL.axf" -A
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) DPLL.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
