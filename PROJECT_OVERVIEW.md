# Project Overview: Bare-metal ARM Cortex-M / RISC-V Silicon Bringup & Peripheral Diagnostic Framework

**Project Repository:** `baremetal-soc-bringup-suite`  
**Engineer:** Poonam Singh  
**Architecture:** ARM Cortex-M (ARMv7-M) & RISC-V (RV32IM)  
**Target Environment:** Silicon Bringup, Pre-Silicon Emulation (QEMU/FPGA), No-OS Automated Testbench  
**Tech Stack:** C, ARM/RISC-V Assembly, Makefile, CMake, QEMU, GDB, Python  

---

## 1. Executive Summary & Purpose

When a new silicon chip (microcontroller or SoC) tapes out and arrives in the laboratory as first silicon, standard operating systems such as Embedded Linux or RTOS cannot boot. At this initial phase:
- Clocks are unconfigured and PLLs are unlocked.
- Volatile memory (SRAM / DDR) has unknown cell retention and timing margins.
- Peripheral register addresses, interrupt routing, and bus fabrics are unvalidated.
- Silicon defects, bonding wires, solder bridges, or PCB trace defects may cause address/data line shorts or stuck-at faults.

The **`baremetal-soc-bringup-suite`** is a production-grade, zero-dependency bare-metal diagnostic framework written specifically for early silicon bringup and pre-silicon validation. It provides:
1. **Low-Level Startup Code & Linker Architecture**: Custom vector tables, stack setup, reset handlers in Assembly, and memory layout linker scripts.
2. **Volatile-Safe MMIO Drivers**: Strict memory-barrier-protected drivers for clocks, timers, UART, GPIO, SPI, I2C, Watchdog, DMA, and interrupt controllers.
3. **Deep Silicon Memory Qualification**: Implements industry-standard algorithmic memory tests: Walking 1s, Walking 0s, Address Bus Aliasing Isolation, 6-Pass March C-, and Galois LFSR pseudo-random stress patterns.
4. **Peripheral Diagnostic Harness**: Automated self-tests covering UART FIFO loopback, SPI master transfer, I2C 9-clock bus unwedge recovery, timer jitter profiling, watchdog feeding, and DMA throughput.
5. **Interactive Hardware Bringup Shell**: A UART-based CLI enabling lab engineers to peek/poke registers, dump memory, toggle GPIO pins, and trigger automated test suites.
6. **Exception & Crash Post-Mortem**: Intercepts Cortex-M HardFaults and RISC-V Traps, unwinding stacked register frames and decoding status registers (`CFSR`, `HFSR`, `MMFAR`, `BFAR`, `mcause`, `mepc`, `mtval`) to pinpoint exact hardware faults.

---

## 2. Silicon Bringup Lifecycle & Test Phases

The framework coordinates the boot and qualification sequence across 7 distinct phases:

```
[Phase 1: Boot & Power-On Reset]
  ├── Assembly Reset_Handler initializes SP and Trap Vector
  ├── Copies .data from Flash/ROM to SRAM; zeroes .bss
  └── Initializes Primary UART at 115200 baud for immediate telemetry
        │
[Phase 2: Clock Tree & Power Management]
  ├── Enables HSI/HSE oscillators; programs PLL multiplier/divider
  ├── Polls PLL lock status register with timeout protection
  └── Captures sticky reset reason flags (POR, NRST, WDT, Software)
        │
[Phase 3: SRAM & Memory Subsystem Qualification]
  ├── Walking 1s & Walking 0s (Data line stuck-at & bridge detection)
  ├── Address Bus Integrity (Discovers high-order aliasing & shorts)
  ├── 6-Pass March C- Algorithm (Detects transition, coupling & retention faults)
  └── Galois LFSR Pseudo-Random Pattern (Dynamic memory retention stress)
        │
[Phase 4: Bus Fabric & Core MPU/PMP Protection]
  ├── Configures Memory Protection Unit (MPU) region boundaries
  └── Validates privileged vs. unprivileged access restrictions
        │
[Phase 5: Peripheral Subsystem Health Verification]
  ├── UART: Transmit, receive, FIFO full/empty flags, loopback
  ├── Timer: Calibrates microsecond timebase and measures drift
  ├── GPIO: Tests atomic bit set/reset (BSRR), input latching, pin toggles
  ├── SPI: Full-duplex master exchange and loopback integrity
  ├── I2C: Device probe, register transfer, and 9-clock bus unwedge sequence
  ├── Watchdog: Validates countdown decrement, reload register, and feeding
  └── DMA: 32-bit block transfer integrity and completion interrupts
        │
[Phase 6: Interrupts & Exception Handling]
  ├── NVIC / PLIC: Tests interrupt enable (ISER), disable (ICER), and priority
  ├── Software IRQ Dispatch: Verifies vector dispatch and ISR execution
  └── Latency Benchmark: Measures CPU cycle overhead from trigger to ISR entry
        │
[Phase 7: Silicon Stress & Concurrent Bus Contention]
  ├── Background DMA transfer between SRAM blocks while CPU runs March C-
  ├── Burst memory throughput benchmarking (MB/s measurement)
  └── Qualification Report Generation & Transition into Interactive UART Shell
```

---

## 3. Deep Dive: Memory Diagnostic Algorithms

Silicon memory defects are subtle and cannot be caught by trivial `memset` or linear write-then-read loops. This framework implements algorithms recognized across semiconductor manufacturing:

### 3.1 Walking 1s & Walking 0s
- **Problem**: Physical bonding wires or PCB traces can suffer from **stuck-at-0** (shorted to ground), **stuck-at-1** (shorted to VCC), or **bridging** (two adjacent data pins shorted together).
- **Algorithm**:
  - For Walking 1s: Writes `0x00000001`, `0x00000002`, `0x00000004` ... up to `0x80000000` to the target memory cell, with memory barriers between each write and readback. If bit 2 is shorted to bit 3, writing `0x00000004` reads back `0x0000000C`, immediately flagging the fault.
  - For Walking 0s: Inverts the mask (`0xFFFFFFFE`, `0xFFFFFFFD`, etc.) to isolate stuck-at-1 pins.

### 3.2 Address Bus Aliasing Test
- **Problem**: If an address line (e.g., $A_7$) is broken or shorted to an adjacent line, reading/writing address `0x20000080` silently aliases to `0x20000000`, corrupting adjacent memory buffers without failing a simple single-cell test.
- **Algorithm**:
  1. Writes unique signatures (`0xAA550000 | offset`) to each power-of-two offset (`1, 2, 4, 8, 16...`) across the memory space.
  2. Writes an inverted pattern (`0x55AAFFFF`) at the base address `0x00000000`.
  3. Iterates through all power-of-two offsets and confirms none were overwritten by the base address write. Any aliased address line immediately triggers a diagnostic failure.

### 3.3 March C- Algorithm (6 Passes)
The March C- test is an industry-standard memory test with $10N$ complexity that detects Address Decoder Faults (AF), Stuck-at Faults (SAF), Transition Faults (TF), and Inversion/Idempotent Coupling Faults (CF):
1. $\Updownarrow (w0)$: Write $0$ to all cells in ascending address order.
2. $\Uparrow (r0, w1)$: Read $0$, then write $1$ in ascending address order.
3. $\Uparrow (r1, w0)$: Read $1$, then write $0$ in ascending address order.
4. $\Downarrow (r0, w1)$: Read $0$, then write $1$ in descending address order.
5. $\Downarrow (r1, w0)$: Read $1$, then write $0$ in descending address order.
6. $\Updownarrow (r0)$: Read $0$ from all cells to confirm final state.

### 3.4 Galois Linear Feedback Shift Register (LFSR)
- **Problem**: Memory cells may hold static patterns but flip under alternating, high-frequency charge distribution (pattern-sensitive faults).
- **Algorithm**: Implements a 32-bit maximal-period Galois polynomial ($x^{32} + x^{31} + x^{29} + x + 1$) to stream pseudo-random patterns through memory, then re-seeds the generator and validates readback.

---

## 4. Peripheral Driver Architecture & Key Highlights

### 4.1 Strict MMIO Memory Barriers (`mmio.h`)
Compiler optimization can reorder, cache, or eliminate memory accesses. In `mmio.h`:
- All memory pointers are strictly `volatile uint32_t *`.
- Architecture-specific barriers are injected:
  - ARM Cortex-M: `__asm__ volatile ("dmb 0xF" ::: "memory")` and `isb 0xF`.
  - RISC-V: `__asm__ volatile ("fence iorw, iorw" ::: "memory")`.

### 4.2 I2C 9-Clock Bus Unwedge Recovery Sequence
When an I2C slave (e.g., an EEPROM or sensor) is interrupted during a read cycle, it may hold the SDA data line LOW waiting for the master to send more clock pulses. Standard I2C re-initialization fails because the bus appears permanently busy.
The driver implements the standard recovery protocol:
1. Reconfigures SCL and SDA from alternate function to GPIO outputs.
2. Generates 9 sequential clock pulses on SCL. This steps the slave's internal state machine until it finishes the byte and releases SDA HIGH.
3. Generates a manual STOP condition (SDA LOW while SCL is HIGH, then SDA HIGH).
4. Restores I2C peripheral mode.

### 4.3 Watchdog Supervision & ARM Register Lock Key
To protect watchdog timers from runaway code or wild pointers, the driver unlocks the controller using the ARM standard security key `0x1ACCE551`, configures the reload window, pets the counter, and immediately relocks the register with `0x00000000`.

### 4.4 Multi-Channel DMA with Bus Contention Stress
The DMA controller driver supports 32-bit memory-to-memory block transfers. In Phase 7, the framework launches a background DMA memory copy while simultaneously executing CPU March C- iterations on adjacent memory blocks, verifying that the SoC bus matrix arbitrates properly without bus stall or data corruption.

---

## 5. Interactive Bringup Console Commands

The UART bringup shell supports the following commands:

| Command | Arguments | Description | Example |
|:--------|:----------|:------------|:--------|
| `help` | None | Displays command syntax and options | `help` |
| `version` | None | Displays silicon revision, microarchitecture, and toolchain | `version` |
| `status` | None | Shows system uptime, clock frequency, and reset cause | `status` |
| `peek` | `<hex_addr>` | Reads 32-bit MMIO register value | `peek 0x40004018` |
| `poke` | `<hex_addr> <val>` | Writes 32-bit MMIO register value | `poke 0x40010018 0x00000020` |
| `dump` | `<hex_addr> [len]` | Displays formatted hex and ASCII memory dump | `dump 0x20010000 64` |
| `fill` | `<addr> <words> <val>` | Fills memory range with 32-bit word | `fill 0x20010000 128 0xDEADBEEF` |
| `mtest` | `<addr> <words> [algo]`| Runs memory test (`all`, `march`, `lfsr`, `walk1`, `walk0`) | `mtest 0x20010000 256 all` |
| `gpio` | `<action> <pin> [val]` | Reads, writes, or toggles digital GPIO pin | `gpio toggle 5` |
| `clocks` | None | Queries PLL multipliers and clock status | `clocks` |
| `diag` | `[all\|phase\|test]` | Executes automated qualification test suites | `diag all` |
| `reboot` | None | Triggers software system reset via SCB AIRCR | `reboot` |

---

## 6. How to Build, Run & Verify

### Step 1: Automated Verification Testbench (Any Platform)
```bash
python scripts/verification/run_bringup_regression.py
```
**Expected Output:** All 19 tests pass in ~2.3 ms, generating `docs/test_report.md`.

### Step 2: Build ARM Cortex-M Bare-Metal Firmware
```bash
make ARCH=arm
```
Builds `build/arm/baremetal_soc_arm.elf` and binary `build/arm/baremetal_soc_arm.bin`.

### Step 3: Run in QEMU (ARM MPS2 Board)
```bash
make qemu ARCH=arm
# Or run script directly:
./scripts/qemu/run_arm.sh
```

### Step 4: Build & Run RISC-V Firmware
```bash
make ARCH=riscv
make qemu ARCH=riscv
# Or run script directly:
./scripts/qemu/run_riscv.sh
```

### Step 5: Debug with GDB
```bash
# In Terminal 1:
make qemu-gdb ARCH=arm

# In Terminal 2:
arm-none-eabi-gdb -x scripts/gdb/arm_qemu.gdb
```

---

## 7. Silicon Bringup Test Matrix & Results

```
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

