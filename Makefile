# ==============================================================================
# Bare-Metal ARM Cortex-M & RISC-V Silicon Bringup & Diagnostic Framework
# Author: Poonam Singh
# ==============================================================================

# Target Architecture: 'arm' or 'riscv'
ARCH ?= arm

BUILD_DIR = build/$(ARCH)
SIM_BUILD_DIR = build/sim

CC_HOST = gcc
PYTHON  = python

ifeq ($(ARCH), arm)
    CROSS_COMPILE ?= arm-none-eabi-
    CPU           ?= -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
    ARCH_FLAGS    ?= -DARCH_ARM=1
    LDSCRIPT      ?= bsp/arm_cortex_m/linker_arm.ld
    STARTUP_SRC   ?= arch/arm/startup_arm.s arch/arm/fault_arm.c
    QEMU_BIN      ?= qemu-system-arm
    QEMU_FLAGS    ?= -M mps2-an385 -cpu cortex-m3 -nographic -serial mon:stdio
else ifeq ($(ARCH), riscv)
    CROSS_COMPILE ?= riscv64-unknown-elf-
    CPU           ?= -march=rv32im -mabi=ilp32
    ARCH_FLAGS    ?= -DARCH_RISCV=1
    LDSCRIPT      ?= bsp/riscv_virt/linker_riscv.ld
    STARTUP_SRC   ?= arch/riscv/startup_riscv.S arch/riscv/trap_riscv.c
    QEMU_BIN      ?= qemu-system-riscv32
    QEMU_FLAGS    ?= -M virt -bios none -nographic -serial mon:stdio
endif

CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)gcc -x assembler-with-cpp
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# Common Include Directories
INCLUDES = \
    -Idrivers/include \
    -Iframework/include \
    -Idiag/include \
    -Icli/include \
    -Iarch/$(ARCH) \
    -Ibsp/$(ARCH)

# Common C Source Files
COMMON_SRCS = \
    drivers/src/uart.c \
    drivers/src/timer.c \
    drivers/src/gpio.c \
    drivers/src/spi.c \
    drivers/src/i2c.c \
    drivers/src/wdt.c \
    drivers/src/dma.c \
    drivers/src/rcc.c \
    framework/src/bringup_framework.c \
    framework/src/diag_logger.c \
    diag/src/diag_memory.c \
    diag/src/diag_peripheral.c \
    diag/src/diag_interrupt.c \
    diag/src/diag_stress.c \
    cli/src/bringup_cli.c \
    app/main.c

# Compilation Flags
CFLAGS = \
    $(CPU) \
    $(ARCH_FLAGS) \
    $(INCLUDES) \
    -O2 \
    -Wall \
    -Wextra \
    -ffreestanding \
    -fno-builtin \
    -nostdlib \
    -g3

LDFLAGS = \
    $(CPU) \
    -T$(LDSCRIPT) \
    -nostartfiles \
    -nodefaultlibs \
    -Wl,-Map=$(BUILD_DIR)/baremetal_soc_$(ARCH).map \
    -Wl,--gc-sections

TARGET_ELF = $(BUILD_DIR)/baremetal_soc_$(ARCH).elf
TARGET_BIN = $(BUILD_DIR)/baremetal_soc_$(ARCH).bin

.PHONY: all clean qemu qemu-gdb sim test help

all: $(TARGET_BIN)

$(TARGET_ELF): $(COMMON_SRCS) $(STARTUP_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "[CC/LD] Linking $@"
	$(CC) $(CFLAGS) $(STARTUP_SRC) $(COMMON_SRCS) $(LDFLAGS) -o $@
	@$(SIZE) $@

$(TARGET_BIN): $(TARGET_ELF)
	@echo "[OBJCOPY] Generating binary $@"
	$(OBJCOPY) -O binary $< $@

qemu: $(TARGET_ELF)
	@echo "[QEMU] Launching $(ARCH) emulation..."
	$(QEMU_BIN) $(QEMU_FLAGS) -kernel $<

qemu-gdb: $(TARGET_ELF)
	@echo "[QEMU] Waiting for GDB connection on localhost:1234..."
	$(QEMU_BIN) $(QEMU_FLAGS) -s -S -kernel $<

# ------------------------------------------------------------------------------
# Host Simulation & Verification Engine Target
# ------------------------------------------------------------------------------
SIM_SRCS = \
    $(COMMON_SRCS) \
    simulator/sim_soc.c \
    simulator/sim_main.c

SIM_CFLAGS = \
    -DSIMULATION=1 \
    -Idrivers/include \
    -Iframework/include \
    -Idiag/include \
    -Icli/include \
    -Isimulator \
    -O2 -Wall -g

sim:
	@mkdir -p $(SIM_BUILD_DIR)
	@echo "[HOST-CC] Building virtual SoC simulation executable..."
	$(CC_HOST) $(SIM_CFLAGS) $(SIM_SRCS) -o $(SIM_BUILD_DIR)/baremetal_sim
	@echo "[HOST-SIM] Executing simulation regression..."
	./$(SIM_BUILD_DIR)/baremetal_sim

test:
	@echo "[TEST] Running automated silicon bringup regression harness..."
	$(PYTHON) scripts/verification/run_bringup_regression.py

clean:
	@rm -rf build

help:
	@echo "Bare-Metal Silicon Bringup Suite Makefile"
	@echo "========================================="
	@echo "  make ARCH=arm        : Build ARM Cortex-M bare-metal ELF/bin"
	@echo "  make ARCH=riscv      : Build RISC-V RV32 bare-metal ELF/bin"
	@echo "  make qemu ARCH=arm   : Run in QEMU ARM emulator"
	@echo "  make qemu ARCH=riscv : Run in QEMU RISC-V emulator"
	@echo "  make qemu-gdb        : Run in QEMU with GDB stub on port 1234"
	@echo "  make sim             : Build & run host virtual SoC simulation"
	@echo "  make test            : Run full automated Python regression"
	@echo "  make clean           : Remove build artifacts"
