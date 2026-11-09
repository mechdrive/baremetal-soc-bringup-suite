# ==============================================================================
# GDB Automated Silicon Bringup & Debug Script for ARM Cortex-M
# Author: Poonam Singh
# Target: QEMU mps2-an385 / lm3s6965evb (localhost:1234)
# ==============================================================================

set pagination off
set print pretty on

echo \n[GDB] Connecting to QEMU ARM Cortex-M GDB Stub...\n
target remote localhost:1234

# Load symbols from ELF image
file build/arm/baremetal_soc_arm.elf

echo [GDB] Setting hardware breakpoints at Reset_Handler, main, and HardFault_Handler...\n
break Reset_Handler
break main
break HardFault_Handler
break bringup_run_all

# Configure fault inspection hook
commands 3
    echo \n[GDB-CRASH-TRAP] Core triggered HardFault!\n
    info registers
    x/8xw $sp
    cfsr_inspect
end

define cfsr_inspect
    printf "CFSR  : 0x%08X\n", *(unsigned int*)0xE000ED28
    printf "HFSR  : 0x%08X\n", *(unsigned int*)0xE000ED2C
    printf "MMFAR : 0x%08X\n", *(unsigned int*)0xE000ED34
    printf "BFAR  : 0x%08X\n", *(unsigned int*)0xE000ED38
end

define dump_peripherals
    printf "=== PERIPHERAL REGISTER MAP INSPECTOR ===\n"
    printf "UART0 FR  (0x40004018): 0x%08X\n", *(unsigned int*)0x40004018
    printf "TIMER0 VAL(0x40000004): 0x%08X\n", *(unsigned int*)0x40000004
    printf "GPIO0 IDR (0x40010010): 0x%08X\n", *(unsigned int*)0x40010010
    printf "WDT VAL   (0x40008004): 0x%08X\n", *(unsigned int*)0x40008004
    printf "DMA0 STAT (0x40030000): 0x%08X\n", *(unsigned int*)0x40030000
    printf "=========================================\n"
end

echo [GDB] Running target firmware to main()...\n
continue
