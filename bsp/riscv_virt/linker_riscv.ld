/**
 * @file linker_riscv.ld
 * @brief Linker Script for RISC-V RV32 Virt Platform
 *
 * @author Poonam Singh
 * @date 2026
 */

ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x80000000, LENGTH = 32M
    RAM (rwx) : ORIGIN = 0x82000000, LENGTH = 32M
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Reset vector & boot initialization */
    .text.init :
    {
        . = ALIGN(4);
        KEEP(*(.text.init))
        . = ALIGN(4);
    } > ROM

    /* Application code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > ROM

    _sidata = LOADADDR(.data);

    /* Data section */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > ROM

    /* BSS section */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
}
