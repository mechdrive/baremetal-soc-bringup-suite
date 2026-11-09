#!/usr/bin/env bash
# ==============================================================================
# Launch RISC-V RV32 Bare-Metal Silicon Bringup Suite in QEMU
# Author: Poonam Singh
# ==============================================================================
set -euo pipefail

ELF_IMAGE="${1:-build/riscv/baremetal_soc_riscv.elf}"

if [ ! -f "$ELF_IMAGE" ]; then
    echo "[ERROR] Image '$ELF_IMAGE' not found. Run 'make ARCH=riscv' first."
    exit 1
fi

echo "=================================================================="
echo " Starting QEMU RISC-V Emulation (Board: virt / RV32)"
echo " Image: $ELF_IMAGE"
echo " Press Ctrl+A then X to exit QEMU"
echo "=================================================================="

qemu-system-riscv32 \
    -M virt \
    -bios none \
    -nographic \
    -serial mon:stdio \
    -kernel "$ELF_IMAGE"
