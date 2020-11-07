################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include service/subdir.mk
-include ports/source/gnu/subdir.mk
-include kernel/source/subdir.mk
-include emlib/subdir.mk
-include cpu/source/subdir.mk
-include common/source/rtos/subdir.mk
-include common/source/platform_mgr/subdir.mk
-include common/source/lib/subdir.mk
-include common/source/kal/subdir.mk
-include common/source/common/subdir.mk
-include common/source/collections/subdir.mk
-include common/source/auth/subdir.mk
-include common/subdir.mk
-include SystemView/subdir.mk
-include CMSIS/EFM32GG11B/subdir.mk
-include BSP/siliconlabs/generic/source/subdir.mk
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
all: SLSTK3701A_micriumos_blink.axf

# Tool invocations
SLSTK3701A_micriumos_blink.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "SLSTK3701A_micriumos_blink.ld" -Xlinker --gc-sections -Xlinker -Map="SLSTK3701A_micriumos_blink.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o SLSTK3701A_micriumos_blink.axf "./BSP/bsp_bcc.o" "./BSP/bsp_stk.o" "./BSP/bsp_stk_leds.o" "./BSP/bsp_trace.o" "./BSP/siliconlabs/generic/source/bsp_cpu.o" "./BSP/siliconlabs/generic/source/bsp_os.o" "./CMSIS/EFM32GG11B/startup_efm32gg11b.o" "./CMSIS/EFM32GG11B/system_efm32gg11b.o" "./SystemView/SEGGER_RTT.o" "./SystemView/SEGGER_RTT_ASM_ARMv7M.o" "./SystemView/SEGGER_SYSVIEW.o" "./SystemView/SEGGER_SYSVIEW_Config_MicriumOSKernel.o" "./SystemView/SEGGER_SYSVIEW_MicriumOSKernel.o" "./common/sl_slist.o" "./common/source/auth/auth.o" "./common/source/collections/bitmap.o" "./common/source/collections/map.o" "./common/source/collections/slist.o" "./common/source/common/common.o" "./common/source/kal/kal_kernel.o" "./common/source/lib/lib_ascii.o" "./common/source/lib/lib_math.o" "./common/source/lib/lib_mem.o" "./common/source/lib/lib_str.o" "./common/source/platform_mgr/platform_mgr.o" "./common/source/rtos/rtos_err_str.o" "./cpu/source/cpu_core.o" "./emlib/em_assert.o" "./emlib/em_cmu.o" "./emlib/em_core.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_rtcc.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./kernel/source/os_cfg_app.o" "./kernel/source/os_core.o" "./kernel/source/os_dbg.o" "./kernel/source/os_flag.o" "./kernel/source/os_mem.o" "./kernel/source/os_mon.o" "./kernel/source/os_msg.o" "./kernel/source/os_mutex.o" "./kernel/source/os_prio.o" "./kernel/source/os_q.o" "./kernel/source/os_sem.o" "./kernel/source/os_stat.o" "./kernel/source/os_task.o" "./kernel/source/os_time.o" "./kernel/source/os_tmr.o" "./kernel/source/os_var.o" "./ports/source/gnu/armv7m_cpu_a.o" "./ports/source/gnu/armv7m_os_cpu_a.o" "./ports/source/gnu/armv7m_os_cpu_c.o" "./service/sl_sleeptimer.o" "./service/sl_sleeptimer_hal_rtcc.o" "./src/ex_main.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: SLSTK3701A_micriumos_blink.hex'
	arm-none-eabi-objcopy -O ihex "SLSTK3701A_micriumos_blink.axf" "SLSTK3701A_micriumos_blink.hex"
	@echo ' '

	@echo 'Building bin file: SLSTK3701A_micriumos_blink.bin'
	arm-none-eabi-objcopy -O binary "SLSTK3701A_micriumos_blink.axf" "SLSTK3701A_micriumos_blink.bin"
	@echo ' '

	@echo 'Building s37 file: SLSTK3701A_micriumos_blink.s37'
	arm-none-eabi-objcopy -O srec "SLSTK3701A_micriumos_blink.axf" "SLSTK3701A_micriumos_blink.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "SLSTK3701A_micriumos_blink.axf" -A
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) SLSTK3701A_micriumos_blink.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
