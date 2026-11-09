#!/usr/bin/env bash
# ==============================================================================
# Launch ARM Cortex-M Bare-Metal Silicon Bringup Suite in QEMU
# Author: Poonam Singh
# ==============================================================================
set -euo pipefail

ELF_IMAGE="${1:-build/arm/baremetal_soc_arm.elf}"

if [ ! -f "$ELF_IMAGE" ]; then
    echo "[ERROR] Image '$ELF_IMAGE' not found. Run 'make ARCH=arm' first."
    exit 1
fi

echo "=================================================================="
echo " Starting QEMU ARM Cortex-M Emulation (Board: mps2-an385 / Cortex-M3)"
echo " Image: $ELF_IMAGE"
echo " Press Ctrl+A then X to exit QEMU"
echo "=================================================================="

qemu-system-arm \
    -M mps2-an385 \
    -cpu cortex-m3 \
    -nographic \
    -serial mon:stdio \
    -kernel "$ELF_IMAGE"
