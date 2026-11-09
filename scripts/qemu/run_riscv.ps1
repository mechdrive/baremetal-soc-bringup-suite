# PowerShell runner for QEMU RISC-V RV32
param(
    [string]$ElfImage = "build/riscv/baremetal_soc_riscv.elf"
)

if (-not (Test-Path $ElfImage)) {
    Write-Error "[ERROR] Image '$ElfImage' not found. Run 'make ARCH=riscv' first."
    exit 1
}

Write-Host "=================================================================="
Write-Host " Starting QEMU RISC-V Emulation (Board: virt / RV32)"
Write-Host " Image: $ElfImage"
Write-Host " Press Ctrl+A then X to exit QEMU"
Write-Host "=================================================================="

& qemu-system-riscv32 `
    -M virt `
    -bios none `
    -nographic `
    -serial mon:stdio `
    -kernel $ElfImage
