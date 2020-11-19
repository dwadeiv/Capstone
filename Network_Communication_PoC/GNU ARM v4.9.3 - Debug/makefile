################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include SiliconLabs/sdks/exx32/platform/middleware/glib/glib/subdir.mk
-include SiliconLabs/sdks/exx32/platform/middleware/glib/dmd/display/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emlib/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/ustimer/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/uartdrv/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/spidrv/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/sleep/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/nvm/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/nvm/config/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/gpiointerrupt/src/subdir.mk
-include SiliconLabs/sdks/exx32/platform/emdrv/dmadrv/src/subdir.mk
-include SiliconLabs/sdks/exx32/hardware/kit/common/drivers/subdir.mk
-include SiliconLabs/sdks/exx32/hardware/kit/common/bsp/subdir.mk
-include Micrium/rtos/net/source/util/subdir.mk
-include Micrium/rtos/net/source/tcpip/subdir.mk
-include Micrium/rtos/net/source/ssl_tls/subdir.mk
-include Micrium/rtos/net/source/dns/subdir.mk
-include Micrium/rtos/net/source/dhcp/subdir.mk
-include Micrium/rtos/net/source/cmd/subdir.mk
-include Micrium/rtos/kernel/source/ports/armv7-m/gnu/subdir.mk
-include Micrium/rtos/kernel/source/subdir.mk
-include Micrium/rtos/drivers/net/source/subdir.mk
-include Micrium/rtos/cpu/source/ports/interrupt/armv7-m/subdir.mk
-include Micrium/rtos/cpu/source/ports/armv7-m/gnu/subdir.mk
-include Micrium/rtos/cpu/source/subdir.mk
-include Micrium/rtos/common/source/shell/subdir.mk
-include Micrium/rtos/common/source/rtos/subdir.mk
-include Micrium/rtos/common/source/ring_buf/subdir.mk
-include Micrium/rtos/common/source/platform_mgr/subdir.mk
-include Micrium/rtos/common/source/op_lock/subdir.mk
-include Micrium/rtos/common/source/logging/subdir.mk
-include Micrium/rtos/common/source/lib/subdir.mk
-include Micrium/rtos/common/source/kal/subdir.mk
-include Micrium/rtos/common/source/common/subdir.mk
-include Micrium/rtos/common/source/collections/subdir.mk
-include Micrium/rtos/common/source/clk/subdir.mk
-include Micrium/rtos/common/source/auth/subdir.mk
-include Micrium/examples/net/core_init/subdir.mk
-include Micrium/examples/net/subdir.mk
-include Micrium/examples/common/common/subdir.mk
-include Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/subdir.mk
-include Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/subdir.mk
-include Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/subdir.mk
-include App/src/subdir.mk
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
all: Micrium-OS-Net-Lab-1_4.axf

# Tool invocations
Micrium-OS-Net-Lab-1_4.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "C:\Users\dante\SimplicityStudio\v4_workspace\Micrium-OS-Net-Lab-1_4\Micrium\bsp\siliconlabs\slstk3701a_efm32gg11\cfg\gcc\efm32gg11b.ld" -Xlinker --gc-sections -Xlinker -Map="Micrium-OS-Net-Lab-1_4.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o Micrium-OS-Net-Lab-1_4.axf "./SiliconLabs/sdks/exx32/platform/emlib/src/em_acmp.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_adc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_aes.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_assert.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_burtc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_can.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_cmu.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_core.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_cryotimer.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_crypto.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_csen.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_dac.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_dbg.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_dma.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_ebi.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_emu.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_gpcrc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_gpio.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_i2c.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_idac.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_int.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_lcd.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_ldma.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_lesense.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_letimer.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_leuart.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_mpu.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_msc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_opamp.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_pcnt.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_prs.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_qspi.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_rmu.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_rtc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_rtcc.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_system.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_timer.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_usart.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_vcmp.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_vdac.o" "./SiliconLabs/sdks/exx32/platform/emlib/src/em_wdog.o" "./SiliconLabs/sdks/exx32/platform/emdrv/ustimer/src/ustimer.o" "./SiliconLabs/sdks/exx32/platform/emdrv/uartdrv/src/uartdrv.o" "./SiliconLabs/sdks/exx32/platform/emdrv/spidrv/src/spidrv.o" "./SiliconLabs/sdks/exx32/platform/emdrv/sleep/src/sleep.o" "./SiliconLabs/sdks/exx32/platform/emdrv/nvm/src/nvm.o" "./SiliconLabs/sdks/exx32/platform/emdrv/nvm/src/nvm_hal.o" "./SiliconLabs/sdks/exx32/platform/emdrv/nvm/config/nvm_config.o" "./SiliconLabs/sdks/exx32/platform/emdrv/gpiointerrupt/src/gpiointerrupt.o" "./SiliconLabs/sdks/exx32/platform/emdrv/dmadrv/src/dmadrv.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/display.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/displayls013b7dh06.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/displaypalemlib.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/retargetio.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/retargetserial.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/drivers/udelay.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/bsp/bsp_bcc.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/bsp/bsp_stk.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/bsp/bsp_stk_leds.o" "./SiliconLabs/sdks/exx32/hardware/kit/common/bsp/bsp_trace.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/bmp.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_bitmap.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_circle.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_font_narrow_6x8.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_font_normal_8x8.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_font_number_16x20.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_line.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_polygon.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_rectangle.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/glib/glib_string.o" "./SiliconLabs/sdks/exx32/platform/middleware/glib/dmd/display/dmd_display.o" "./App/src/app.o" "./App/src/app_client.o" "./App/src/app_graphics.o" "./App/src/app_server.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/bsp_cpu.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/bsp_led.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/bsp_net_ether_gem.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/bsp_os.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/gcc/startup_efm32gg11b.o" "./Micrium/bsp/siliconlabs/slstk3701a_efm32gg11/source/startup/system_efm32gg11b.o" "./Micrium/examples/common/common/ex_common_externalize_cfg.o" "./Micrium/examples/common/common/ex_common_init.o" "./Micrium/examples/net/core_init/ex_net_core_init.o" "./Micrium/examples/net/ex_network_init.o" "./Micrium/rtos/common/source/auth/auth.o" "./Micrium/rtos/common/source/clk/clk.o" "./Micrium/rtos/common/source/clk/clk_cmd.o" "./Micrium/rtos/common/source/collections/bitmap.o" "./Micrium/rtos/common/source/collections/map.o" "./Micrium/rtos/common/source/collections/slist.o" "./Micrium/rtos/common/source/common/common.o" "./Micrium/rtos/common/source/kal/kal_kernel.o" "./Micrium/rtos/common/source/kal/kal_os2.o" "./Micrium/rtos/common/source/lib/lib_ascii.o" "./Micrium/rtos/common/source/lib/lib_math.o" "./Micrium/rtos/common/source/lib/lib_mem.o" "./Micrium/rtos/common/source/lib/lib_str.o" "./Micrium/rtos/common/source/logging/logging.o" "./Micrium/rtos/common/source/op_lock/op_lock.o" "./Micrium/rtos/common/source/platform_mgr/platform_mgr.o" "./Micrium/rtos/common/source/ring_buf/ring_buf.o" "./Micrium/rtos/common/source/rtos/rtos_err_str.o" "./Micrium/rtos/common/source/shell/shell.o" "./Micrium/rtos/common/source/shell/shell_cmd.o" "./Micrium/rtos/cpu/source/cpu_core.o" "./Micrium/rtos/cpu/source/ports/armv7-m/gnu/cpu_a.o" "./Micrium/rtos/cpu/source/ports/armv7-m/gnu/cpu_c.o" "./Micrium/rtos/cpu/source/ports/interrupt/armv7-m/cpu_int_armv7-m.o" "./Micrium/rtos/drivers/net/source/net_drv_ether_gem.o" "./Micrium/rtos/drivers/net/source/net_drv_phy_generic.o" "./Micrium/rtos/kernel/source/os_cfg_app.o" "./Micrium/rtos/kernel/source/os_core.o" "./Micrium/rtos/kernel/source/os_dbg.o" "./Micrium/rtos/kernel/source/os_flag.o" "./Micrium/rtos/kernel/source/os_mem.o" "./Micrium/rtos/kernel/source/os_mon.o" "./Micrium/rtos/kernel/source/os_msg.o" "./Micrium/rtos/kernel/source/os_mutex.o" "./Micrium/rtos/kernel/source/os_prio.o" "./Micrium/rtos/kernel/source/os_q.o" "./Micrium/rtos/kernel/source/os_sem.o" "./Micrium/rtos/kernel/source/os_stat.o" "./Micrium/rtos/kernel/source/os_task.o" "./Micrium/rtos/kernel/source/os_tick.o" "./Micrium/rtos/kernel/source/os_time.o" "./Micrium/rtos/kernel/source/os_tmr.o" "./Micrium/rtos/kernel/source/os_var.o" "./Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_a.o" "./Micrium/rtos/kernel/source/ports/armv7-m/gnu/os_cpu_c.o" "./Micrium/rtos/net/source/cmd/dns_client_cmd.o" "./Micrium/rtos/net/source/cmd/net_cmd.o" "./Micrium/rtos/net/source/cmd/net_cmd_args_parser.o" "./Micrium/rtos/net/source/cmd/net_cmd_output.o" "./Micrium/rtos/net/source/cmd/net_cmd_sock.o" "./Micrium/rtos/net/source/cmd/net_cmd_wifi.o" "./Micrium/rtos/net/source/cmd/smtp_client_cmd.o" "./Micrium/rtos/net/source/cmd/sntp_client_cmd.o" "./Micrium/rtos/net/source/dhcp/dhcp.o" "./Micrium/rtos/net/source/dhcp/dhcp_client.o" "./Micrium/rtos/net/source/dns/dns_client.o" "./Micrium/rtos/net/source/dns/dns_client_cache.o" "./Micrium/rtos/net/source/dns/dns_client_req.o" "./Micrium/rtos/net/source/dns/dns_client_task.o" "./Micrium/rtos/net/source/ssl_tls/net_secure_mocana.o" "./Micrium/rtos/net/source/tcpip/net.o" "./Micrium/rtos/net/source/tcpip/net_app.o" "./Micrium/rtos/net/source/tcpip/net_arp.o" "./Micrium/rtos/net/source/tcpip/net_ascii.o" "./Micrium/rtos/net/source/tcpip/net_bsd.o" "./Micrium/rtos/net/source/tcpip/net_buf.o" "./Micrium/rtos/net/source/tcpip/net_cache.o" "./Micrium/rtos/net/source/tcpip/net_conn.o" "./Micrium/rtos/net/source/tcpip/net_ctr.o" "./Micrium/rtos/net/source/tcpip/net_dad.o" "./Micrium/rtos/net/source/tcpip/net_icmp.o" "./Micrium/rtos/net/source/tcpip/net_icmpv4.o" "./Micrium/rtos/net/source/tcpip/net_icmpv6.o" "./Micrium/rtos/net/source/tcpip/net_if.o" "./Micrium/rtos/net/source/tcpip/net_if_802x.o" "./Micrium/rtos/net/source/tcpip/net_if_ether.o" "./Micrium/rtos/net/source/tcpip/net_if_loopback.o" "./Micrium/rtos/net/source/tcpip/net_if_wifi.o" "./Micrium/rtos/net/source/tcpip/net_igmp.o" "./Micrium/rtos/net/source/tcpip/net_ip.o" "./Micrium/rtos/net/source/tcpip/net_ipv4.o" "./Micrium/rtos/net/source/tcpip/net_ipv6.o" "./Micrium/rtos/net/source/tcpip/net_mldp.o" "./Micrium/rtos/net/source/tcpip/net_ndp.o" "./Micrium/rtos/net/source/tcpip/net_sock.o" "./Micrium/rtos/net/source/tcpip/net_stat.o" "./Micrium/rtos/net/source/tcpip/net_task.o" "./Micrium/rtos/net/source/tcpip/net_tcp.o" "./Micrium/rtos/net/source/tcpip/net_tmr.o" "./Micrium/rtos/net/source/tcpip/net_udp.o" "./Micrium/rtos/net/source/tcpip/net_util.o" "./Micrium/rtos/net/source/tcpip/net_wifi_mgr.o" "./Micrium/rtos/net/source/util/net_base64.o" "./Micrium/rtos/net/source/util/net_dict.o" "./Micrium/rtos/net/source/util/net_fs_port_native.o" "./Micrium/rtos/net/source/util/net_sha1.o" "./Micrium/rtos/net/source/util/net_svc_task.o" -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: Micrium-OS-Net-Lab-1_4.hex'
	arm-none-eabi-objcopy -O ihex "Micrium-OS-Net-Lab-1_4.axf" "Micrium-OS-Net-Lab-1_4.hex"
	@echo ' '

	@echo 'Building bin file: Micrium-OS-Net-Lab-1_4.bin'
	arm-none-eabi-objcopy -O binary "Micrium-OS-Net-Lab-1_4.axf" "Micrium-OS-Net-Lab-1_4.bin"
	@echo ' '

	@echo 'Building s37 file: Micrium-OS-Net-Lab-1_4.s37'
	arm-none-eabi-objcopy -O srec "Micrium-OS-Net-Lab-1_4.axf" "Micrium-OS-Net-Lab-1_4.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "Micrium-OS-Net-Lab-1_4.axf" -A
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) Micrium-OS-Net-Lab-1_4.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
