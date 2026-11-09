/**
 * @file startup_arm.s
 * @brief ARM Cortex-M Vector Table & Reset Handler Assembly
 *
 * @author Poonam Singh
 * @date 2026
 */

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

    .global g_pfnVectors
    .global Reset_Handler
    .global HardFault_Handler
    .global Default_Handler

    /* Memory symbols defined in linker script */
    .word _sidata
    .word _sdata
    .word _edata
    .word _sbss
    .word _ebss

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack      /* Initialize stack pointer */

    /* Copy initialized data from Flash to SRAM */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4

LoopCopyDataInit:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   CopyDataInit

    /* Zero fill the bss section */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     LoopFillZerobss

FillZerobss:
    str  r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp  r2, r4
    bcc  FillZerobss

    /* Call main application */
    bl    main

LoopForever:
    wfi
    b     LoopForever
    .size Reset_Handler, .-Reset_Handler

    /* HardFault Handler trampoline: passes stack pointer to C handler */
    .section .text.HardFault_Handler
    .global HardFault_Handler
    .type HardFault_Handler, %function
HardFault_Handler:
    tst   lr, #4
    ite   eq
    mrseq r0, msp
    mrsne r0, psp
    mov   r1, lr
    b     arm_fault_handler_c
    .size HardFault_Handler, .-HardFault_Handler

    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler

    /* Minimal Vector Table */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler

    /* External Peripheral Interrupts */
    .word UART0_IRQHandler
    .word UART1_IRQHandler
    .word Timer0_IRQHandler
    .word Timer1_IRQHandler
    .word GPIO0_IRQHandler
    .word GPIO1_IRQHandler
    .word SPI0_IRQHandler
    .word I2C0_IRQHandler
    .word DMA0_IRQHandler
    .word WDT_IRQHandler

    /* Provide weak aliases for all handlers pointing to Default_Handler */
    .weak NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak MemManage_Handler
    .thumb_set MemManage_Handler,Default_Handler

    .weak BusFault_Handler
    .thumb_set BusFault_Handler,Default_Handler

    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler,Default_Handler

    .weak SVC_Handler
    .thumb_set SVC_Handler,Default_Handler

    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler,Default_Handler

    .weak PendSV_Handler
    .thumb_set PendSV_Handler,Default_Handler

    .weak SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler

    .weak UART0_IRQHandler
    .thumb_set UART0_IRQHandler,Default_Handler

    .weak UART1_IRQHandler
    .thumb_set UART1_IRQHandler,Default_Handler

    .weak Timer0_IRQHandler
    .thumb_set Timer0_IRQHandler,Default_Handler

    .weak Timer1_IRQHandler
    .thumb_set Timer1_IRQHandler,Default_Handler

    .weak GPIO0_IRQHandler
    .thumb_set GPIO0_IRQHandler,Default_Handler

    .weak GPIO1_IRQHandler
    .thumb_set GPIO1_IRQHandler,Default_Handler

    .weak SPI0_IRQHandler
    .thumb_set SPI0_IRQHandler,Default_Handler

    .weak I2C0_IRQHandler
    .thumb_set I2C0_IRQHandler,Default_Handler

    .weak DMA0_IRQHandler
    .thumb_set DMA0_IRQHandler,Default_Handler

    .weak WDT_IRQHandler
    .thumb_set WDT_IRQHandler,Default_Handler
