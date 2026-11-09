/**
 * @file linker_arm.ld
 * @brief Linker Script for ARM Cortex-M Silicon Bringup Platform (MPS2 / LM3S)
 *
 * Defines Flash ROM, SRAM, Vector Table, Stack & Heap memory boundaries.
 *
 * @author Poonam Singh
 * @date 2026
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);    /* End of RAM -> top of stack */
_min_heap_size  = 0x4000;              /* 16 KB Minimum Heap */
_min_stack_size = 0x4000;              /* 16 KB Minimum Stack */

SECTIONS
{
    /* Vector table placed at start of Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code and constants */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Load Address of .data in Flash */
    _sidata = LOADADDR(.data);

    /* Initialized data placed into RAM at runtime */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Uninitialized data in RAM */
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

    /* Heap and stack reservation and boundary guard check */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _min_heap_size;
        . = . + _min_stack_size;
        . = ALIGN(8);
    } > RAM

    /* Discard unused sections */
    /DISCARD/ :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        *(.ARM.exidx*)
    }
}

