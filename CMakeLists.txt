cmake_minimum_required(VERSION 3.15)
project(baremetal-soc-bringup-suite C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_SIMULATION "Build host virtual SoC simulation engine" ON)

if(BUILD_SIMULATION OR NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Configuring Host Simulation Build (Pre-Silicon Virtual Bus)")
    add_definitions(-DSIMULATION=1)

    include_directories(
        drivers/include
        framework/include
        diag/include
        cli/include
        simulator
    )

    set(SIM_SOURCES
        drivers/src/uart.c
        drivers/src/timer.c
        drivers/src/gpio.c
        drivers/src/spi.c
        drivers/src/i2c.c
        drivers/src/wdt.c
        drivers/src/dma.c
        drivers/src/rcc.c
        framework/src/bringup_framework.c
        framework/src/diag_logger.c
        diag/src/diag_memory.c
        diag/src/diag_peripheral.c
        diag/src/diag_interrupt.c
        diag/src/diag_stress.c
        cli/src/bringup_cli.c
        simulator/sim_soc.c
        simulator/sim_main.c
    )

    add_executable(baremetal_sim ${SIM_SOURCES})

    enable_testing()
    add_test(NAME SiliconBringupRegression COMMAND baremetal_sim)

else()
    enable_language(ASM)
    message(STATUS "Configuring Cross-Compiled Target Build for ${ARCH}")
endif()
