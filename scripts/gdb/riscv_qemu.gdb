# ==============================================================================
# GDB Automated Silicon Bringup & Debug Script for RISC-V RV32
# Author: Poonam Singh
# Target: QEMU virt (localhost:1234)
# ==============================================================================

set pagination off
set print pretty on

echo \n[GDB] Connecting to QEMU RISC-V GDB Stub...\n
target remote localhost:1234

# Load symbols from ELF image
file build/riscv/baremetal_soc_riscv.elf

echo [GDB] Setting breakpoints at _start, main, and trap_vector_entry...\n
break _start
break main
break trap_vector_entry

define csrs_inspect
    printf "=== RISC-V CSR STATUS ===\n"
    info registers mstatus mie mtvec mepc mcause mtval
    printf "=========================\n"
end

define dump_clint
    printf "=== CLINT STATUS ===\n"
    printf "MSIP     (0x02000000): 0x%08X\n", *(unsigned int*)0x02000000
    printf "MTIMECMP (0x02004000): 0x%08X\n", *(unsigned int*)0x02004000
    printf "MTIME    (0x0200BFF8): 0x%08X\n", *(unsigned int*)0x0200BFF8
    printf "====================\n"
end

echo [GDB] Continuing execution to main...\n
continue
