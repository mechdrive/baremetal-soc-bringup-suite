# PowerShell runner for QEMU ARM Cortex-M
param(
    [string]$ElfImage = "build/arm/baremetal_soc_arm.elf"
)

if (-not (Test-Path $ElfImage)) {
    Write-Error "[ERROR] Image '$ElfImage' not found. Run 'make ARCH=arm' first."
    exit 1
}

Write-Host "=================================================================="
Write-Host " Starting QEMU ARM Cortex-M Emulation (Board: mps2-an385 / Cortex-M3)"
Write-Host " Image: $ElfImage"
Write-Host " Press Ctrl+A then X to exit QEMU"
Write-Host "=================================================================="

& qemu-system-arm `
    -M mps2-an385 `
    -cpu cortex-m3 `
    -nographic `
    -serial mon:stdio `
    -kernel $ElfImage

