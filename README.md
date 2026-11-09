# Bare-Metal ARM Cortex-M / RISC-V Silicon Bringup & Peripheral Diagnostic Framework

[![Silicon Bringup CI](https://github.com/mechdrive/baremetal-soc-bringup-suite/actions/workflows/ci.yml/badge.svg)](https://github.com/mechdrive/baremetal-soc-bringup-suite/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M%20%7C%20RISC--V%20RV32-blue.svg)]()
[![No-OS](https://img.shields.io/badge/Runtime-Bare--Metal%20%2F%20No--OS-brightgreen.svg)]()

> **Production-grade bare-metal silicon bringup, pre-silicon emulation, and register-level peripheral diagnostic framework engineered in C and Assembly for ARM Cortex-M and RISC-V microarchitectures.**

---

## 1. Executive Architecture Overview

When a new System-on-Chip (SoC) tapes out and first wafer samples arrive in the laboratory—or during pre-silicon FPGA emulation and Software-in-the-Loop (SIL) simulation—no operating system (Linux, RTOS) can execute until the foundational silicon fabric is qualified. 

The **`baremetal-soc-bringup-suite`** addresses this critical hardware-software bringup boundary. It provides a zero-dependency, standalone runtime with custom vector tables, reset handlers, linker scripts, register-level peripheral drivers, memory qualification algorithms, crash dump unwinding, and an interactive UART diagnostic shell.

```mermaid
flowchart TD
    subgraph PowerOn [Phase 1: Boot & Hardware Reset]
        POR[Power-On Reset / NRST] --> ResetHandler[arch/arm/startup_arm.s\narch/riscv/startup_riscv.S]
        ResetHandler --> StackInit[Initialize SP / Trap Vector]
        ResetHandler --> CRuntimeInit[Copy .data (ROM -> RAM)\nZero .bss Section]
    end

    subgraph SiliconQual [Phase 2-7: Automated Silicon Qualification]
        CRuntimeInit --> ClockInit[RCC: HSI/HSE & PLL Lock Guard]
        ClockInit --> MemDiag[Memory Subsystem Diagnostics\nWalking 1s/0s, March C-, LFSR]
        MemDiag --> BusFabric[Bus Fabric & MPU/PMP Protection]
        BusFabric --> PeriphDiag[Peripheral Subsystems\nUART, Timer, GPIO, SPI, I2C, WDT, DMA]
        PeriphDiag --> IrqDiag[NVIC / PLIC & Trap Latency Benchmark]
        IrqDiag --> StressBurnIn[Concurrent CPU March C- + DMA Contention]
    end

    subgraph UserInterface [Interactive Silicon Monitor]
        StressBurnIn --> ReportGen[Qualification Report Table & Metrics]
        ReportGen --> Shell[Interactive UART Bringup Shell\npeek, poke, dump, fill, mtest, diag]
    end
```

---

## 2. Core Capabilities & Architectural Pillars

### 2.1 Multi-Architecture Support
- **ARM Cortex-M (Cortex-M3 / Cortex-M4 / ARMv7-M)**:
  - Custom vector table (`g_pfnVectors`), `Reset_Handler`, `HardFault_Handler`.
  - Nested Vectored Interrupt Controller (NVIC) introspection, priority masking.
  - Configurable Fault Status Register (`CFSR`), `HFSR`, `MMFAR`, `BFAR` decoding.
  - Memory Protection Unit (`MPU`) region boundary configuration.
- **RISC-V (RV32I / RV32IM)**:
  - Boot entry `_start`, CSR management (`mstatus`, `mie`, `mtvec`, `mepc`, `mcause`, `mtval`).
  - Core Local Interruptor (`CLINT`) software and timer interrupts.
  - Platform-Level Interrupt Controller (`PLIC`) priority and enable registers.
  - Machine-mode trap frame decoding.

### 2.2 Deep Memory Subsystem Diagnostics
| Diagnostic Algorithm | Target Fault Mechanism | Test Methodology |
|:---------------------|:-----------------------|:-----------------|
| **Walking 1s** | Data bus stuck-at-0 & bridge short | Shifts `1U << bit` across 32 data lines with barrier |
| **Walking 0s** | Data bus stuck-at-1 | Shifts `~(1U << bit)` across 32 data lines |
| **Address Bus Integrity** | Address line open, short & aliasing | Power-of-two offset pattern write; base inverted check |
| **March C- (6-Pass)** | SRAM cell transitions, coupling faults (CF), address decoder faults (AF) | Standard industrial 6-pass silicon validation algorithm |
| **Galois LFSR Stress** | Dynamic cell retention & crosstalk | 32-bit maximal-period pseudo-random polynomial stream |

### 2.3 Production Peripheral Drivers
- **MMIO Abstraction (`drivers/include/mmio.h`)**: Volatile 8/16/32-bit register read/write primitives with architecture-specific data memory barriers (`dmb`, `fence iorw, iorw`).
- **High-Speed UART (`drivers/src/uart.c`)**: Dual-architecture support for ARM PL011/CMSDK and NS16550A with baud rate prescalers, FIFO buffers, and hardware loopback.
- **Core Timer & Delay (`drivers/src/timer.c`)**: Calibrated microsecond (`delay_us`) and millisecond (`delay_ms`) timebases, monotonic tick counters.
- **GPIO & Pin Control (`drivers/src/gpio.c`)**: Atomic Bit Set/Reset Register (`BSRR`) manipulation, mode configuration (Input, Output, Alt, Analog), pull-up/pull-down.
- **SPI Master (`drivers/src/spi.c`)**: Configurable CPOL/CPHA clock polarity/phase, baud rate division, full-duplex transfers, and loopback mode.
- **I2C Master & Recovery (`drivers/src/i2c.c`)**: Standard (100kHz) and Fast (400kHz) clock timing, START/STOP generation, ACK/NACK validation, and a **9-clock pulse bus unwedge sequence** to un-stall locked slave devices.
- **Watchdog Timer (`drivers/src/wdt.c`)**: Register lock/unlock sequencing (`0x1ACCE551`), countdown supervision, refreshing (petting), and test-driven system reset triggers.
- **Direct Memory Access (`drivers/src/dma.c`)**: 32-bit multi-channel block transfers, source/destination auto-increment, synchronous blocking and non-blocking modes.
- **Reset & Clock Control (`drivers/src/rcc.c`)**: HSI/HSE oscillator management, PLL lock timeout safeguards, peripheral clock gating, and sticky reset reason flag decoding (`POR`, `NRST`, `Software`, `IWDG`, `WWDG`, `BOR`).

---

## 3. Interactive Silicon Bringup Shell (UART Monitor)

Connect to UART0 (115200 baud, 8N1) to interact directly with hardware registers and diagnostics:

```text
==================================================================
  BARE-METAL SILICON BRINGUP CONSOLE - TYPE 'help' OR 'diag all'
==================================================================
bringup> help

================= SILICON BRINGUP MONITOR COMMANDS =================
  help                                  : Display command summary
  version                               : Show hardware, architecture & build info
  status                                : Show SoC clock, uptime & reset reason
  peek <addr>                           : Read 32-bit MMIO register (hex)
  poke <addr> <val>                     : Write 32-bit MMIO register (hex)
  dump <addr> <len_bytes>               : Memory hex/ASCII dump
  fill <addr> <words> <val>             : Fill memory range with 32-bit word
  mtest <addr> <words> [all|march|lfsr] : Run memory algorithms on block
  periph [list|test]                    : Scan or test SoC peripherals
  clocks                                : Query PLL & clock tree status
  gpio <read|write|toggle> <pin> [val]  : Inspect or drive GPIO pin
  spi send <hex_byte>                   : Transfer byte via SPI master
  i2c probe <7bit_hex>                  : Probe I2C slave address
  wdt [feed|trip]                       : Refresh or force watchdog timeout
  dma test                              : Run DMA block transfer test
  diag [all|phase <1-7>|test <id>]      : Run automated silicon test suites
  reboot                                : Perform software SoC reset
====================================================================

bringup> peek 0x40004018
[PEEK] [0x40004018] = 0x00000090 (144)

bringup> dump 0x20010000 32
--- Memory Dump: USER_DUMP (Base: 0x20010000, Length: 32 bytes) ---
0x20010000: EF BE AD DE 00 00 00 00  55 AA 55 AA FF FF 00 00  |.... ...U.U....|
0x20010010: 11 22 33 44 00 00 00 00  00 00 00 00 00 00 00 00  |."3D............|

bringup> mtest 0x20010000 256 march
[000142 ms] [INFO ] Starting memory diagnosis at 0x20010000 (256 words, algo: march)...
[000143 ms] [INFO ] March C- : PASS
[MTEST RESULT] >>> MEMORY REGION QUALIFIED: INTEGRITY VERIFIED <<<
```

---

## 4. Hardware Fault Unwinding & Crash Analysis

When hardware faults occur during bringup (e.g., accessing unclocked peripherals, misaligned reads, MPU permission violations), the framework intercepts the trap, extracts the stacked frame, decodes status registers, and outputs an actionable post-mortem:

```text
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                      *** ARM CORTEX-M HARDFAULT EXCEPTION ***                  
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
EXC_RETURN (LR) : 0xFFFFFFFD (Process (PSP) stack used)
R0  : 0x00000000  | R1  : 0x40022000  | R2  : 0x00000004  | R3 : 0x00000000
R12 : 0x20001000  | LR  : 0x0000045B  | PC  : 0x000008F2  | PSR: 0x61000000
CFSR: 0x00008200  | HFSR: 0x40000000
[CAUSE: BUS FAULT] Precise Data Bus Error | Faulting Bus Address: 0x40022004 | 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Halting core. Attach GDB to inspect state.
```

---

## 5. Repository Structure

```text
baremetal-soc-bringup-suite/
├── .github/
│   └── workflows/
│       └── ci.yml                     # Multi-arch automated build & QEMU CI pipeline
├── arch/
│   ├── arm/
│   │   ├── cpu_arm.h                  # ARM Cortex-M architectural registers & SCB
│   │   ├── fault_arm.c                # HardFault, MemManage, BusFault unwinder
│   │   └── startup_arm.s              # Assembly vector table & reset handler
│   └── riscv/
│       ├── cpu_riscv.h                # RISC-V CSR inline primitives
│       ├── trap_riscv.c               # Machine-mode trap & exception unwinder
│       └── startup_riscv.S            # Assembly boot entry (_start) & trap vector
├── bsp/
│   ├── arm_cortex_m/
│   │   └── linker_arm.ld              # Linker script for ARM Cortex-M (MPS2 / LM3S)
│   └── riscv_virt/
│       └── linker_riscv.ld            # Linker script for RISC-V Virt platform
├── drivers/
│   ├── include/
│   │   ├── mmio.h                     # Volatile MMIO primitives & memory barriers
│   │   ├── soc_regs.h                 # Peripheral memory map & register offsets
│   │   ├── uart.h                     # High-speed UART interface
│   │   ├── timer.h                    # Core timer & microsecond precision delays
│   │   ├── gpio.h                     # GPIO pin & port control
│   │   ├── spi.h                      # SPI master interface
│   │   ├── i2c.h                      # I2C master & 9-clock recovery interface
│   │   ├── wdt.h                      # Watchdog timer supervision interface
│   │   ├── dma.h                      # DMA block transfer interface
│   │   └── rcc.h                      # Reset and clock control interface
│   └── src/
│       ├── uart.c                     # UART driver implementation
│       ├── timer.c                    # Timer driver implementation
│       ├── gpio.c                     # GPIO driver implementation
│       ├── spi.c                      # SPI driver implementation
│       ├── i2c.c                      # I2C driver implementation
│       ├── wdt.c                      # Watchdog driver implementation
│       ├── dma.c                      # DMA driver implementation
│       └── rcc.c                      # RCC driver implementation
├── framework/
│   ├── include/
│   │   ├── diag_common.h              # Common test structures, enums & results
│   │   └── bringup_framework.h        # Assertion engine & test registry
│   └── src/
│       ├── bringup_framework.c        # Test harness runner & summary generator
│       └── diag_logger.c              # Logging engine & memory hex dumper
├── diag/
│   ├── include/
│   │   ├── diag_memory.h              # Memory test headers
│   │   ├── diag_peripheral.h          # Peripheral test headers
│   │   ├── diag_interrupt.h           # Interrupt & exception test headers
│   │   └── diag_stress.h              # Stress & concurrency test headers
│   └── src/
│       ├── diag_memory.c              # Walking 1s/0s, Address Aliasing, March C-, LFSR
│       ├── diag_peripheral.c          # Clock, UART, Timer, GPIO, SPI, I2C, WDT, DMA tests
│       ├── diag_interrupt.c           # NVIC/PLIC register, IRQ latency & dispatch tests
│       └── diag_stress.c              # Concurrent CPU March C- + DMA bus contention
├── cli/
│   ├── include/
│   │   └── bringup_cli.h              # Interactive monitor header
│   └── src/
│       └── bringup_cli.c              # Interactive UART command shell
├── app/
│   └── main.c                         # Firmware entry & orchestration pipeline
├── simulator/
│   ├── sim_soc.h                      # Virtual bus model header
│   ├── sim_soc.c                      # Register-level MMIO virtual SoC simulation
│   └── sim_main.c                     # Host simulation runner
├── scripts/
│   ├── gdb/
│   │   ├── arm_qemu.gdb               # GDB automation for ARM Cortex-M
│   │   ├── riscv_qemu.gdb             # GDB automation for RISC-V RV32
│   │   └── soc_inspect.py             # GDB Python extension for hardware inspection
│   ├── qemu/
│   │   ├── run_arm.sh / .ps1          # ARM QEMU launch scripts
│   │   └── run_riscv.sh / .ps1        # RISC-V QEMU launch scripts
│   └── verification/
│       └── run_bringup_regression.py  # Automated regression runner & report generator
├── docs/
│   └── test_report.md                 # Generated silicon qualification report
├── Makefile                           # Dual-architecture build system
├── CMakeLists.txt                     # CMake cross-platform configuration
└── README.md                          # Repository documentation
```

---

## 6. Build & Execution Guide

### 6.1 Native Automated Testbench (Zero Setup Required)
Run the automated verification testbench immediately using Python:
```bash
python scripts/verification/run_bringup_regression.py
```
This executes all 19 diagnostic test cases, verifies register state machines, and generates `docs/test_report.md`.

### 6.2 Compiling Bare-Metal ARM Firmware
Requires `arm-none-eabi-gcc`:
```bash
make ARCH=arm
# Output: build/arm/baremetal_soc_arm.elf, build/arm/baremetal_soc_arm.bin
```

Launch in QEMU:
```bash
make qemu ARCH=arm
# Or using the shell script:
./scripts/qemu/run_arm.sh
```

Debug with GDB:
```bash
# Terminal 1: Launch QEMU with GDB stub
make qemu-gdb ARCH=arm

# Terminal 2: Connect GDB
arm-none-eabi-gdb -x scripts/gdb/arm_qemu.gdb
```

### 6.3 Compiling Bare-Metal RISC-V Firmware
Requires `riscv64-unknown-elf-gcc`:
```bash
make ARCH=riscv
# Output: build/riscv/baremetal_soc_riscv.elf, build/riscv/baremetal_soc_riscv.bin
```

Launch in QEMU:
```bash
make qemu ARCH=riscv
# Or using the shell script:
./scripts/qemu/run_riscv.sh
```

---

## 7. Silicon Bringup Test Matrix & Qualification Status

```text
################################################################################
#                   SILICON BRINGUP & DIAGNOSTIC REPORT                       #
################################################################################
| ID | Test Name                    | Result  | Time (us)  | Details           |
+----+------------------------------+---------+------------+-------------------+
| 10 | MEM_WALK_ONES                | PASS    |         24 | OK                |
| 11 | MEM_WALK_ZEROS               | PASS    |         26 | OK                |
| 12 | MEM_ADDR_BUS_ALIAS           | PASS    |         42 | OK                |
| 13 | MEM_MARCH_C_MINUS            | PASS    |        118 | OK                |
| 14 | MEM_LFSR_STRESS              | PASS    |         95 | OK                |
| 20 | PERIPH_CLOCK_PLL             | PASS    |         85 | OK                |
| 21 | PERIPH_UART_LOOPBACK         | PASS    |         48 | OK                |
| 22 | PERIPH_TIMER_CALIB           | PASS    |       1012 | OK                |
| 23 | PERIPH_GPIO_TOGGLE           | PASS    |         18 | OK                |
| 24 | PERIPH_SPI_LOOPBACK          | PASS    |         34 | OK                |
| 25 | PERIPH_I2C_BUS_UNWEDGE       | PASS    |         76 | OK                |
| 26 | PERIPH_WDT_SUPERVISION       | PASS    |         52 | OK                |
| 27 | PERIPH_DMA_MEMCPY            | PASS    |         64 | OK                |
| 30 | INT_CONTROLLER_REGS          | PASS    |         15 | OK                |
| 31 | INT_SOFTWARE_TRIGGER         | PASS    |         22 | OK                |
| 32 | INT_LATENCY_PROFILE          | PASS    |          8 | OK                |
| 33 | BUS_MPU_PROTECTION           | PASS    |         30 | OK                |
| 40 | STRESS_BUS_CONCURRENCY       | PASS    |        210 | OK                |
| 41 | STRESS_BURST_BANDWIDTH       | PASS    |        320 | OK                |
+----+------------------------------+---------+------------+-------------------+
TOTAL: 19 | PASSED: 19 | FAILED: 0 | TIMEOUT: 0 | TIME: 2299 us
[SILICON HEALTH] >>> ALL BRINGUP TESTS PASSED: SILICON REVISION QUALIFIED <<<
################################################################################
```

---

## 8. License & Authorship

- **Author**: Poonam Singh ([GitHub: @mechdrive](https://github.com/mechdrive))
- **Role**: Embedded Systems & Silicon Bringup Engineer
- **License**: MIT License

