cmake_minimum_required(VERSION 3.20)

if(CMAKE_C_COMPILER MATCHES "arm-none-eabi")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

project(balancecar C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/stm32f103c8tx_flash.ld)

set(PROJECT_SOURCES
    Start/startup_stm32f103c8tx_gcc.s
    Start/core_cm3.c
    Start/system_stm32f10x.c

    Library/misc.c
    Library/stm32f10x_adc.c
    Library/stm32f10x_bkp.c
    Library/stm32f10x_can.c
    Library/stm32f10x_cec.c
    Library/stm32f10x_crc.c
    Library/stm32f10x_dac.c
    Library/stm32f10x_dbgmcu.c
    Library/stm32f10x_dma.c
    Library/stm32f10x_exti.c
    Library/stm32f10x_flash.c
    Library/stm32f10x_fsmc.c
    Library/stm32f10x_gpio.c
    Library/stm32f10x_i2c.c
    Library/stm32f10x_iwdg.c
    Library/stm32f10x_pwr.c
    Library/stm32f10x_rcc.c
    Library/stm32f10x_rtc.c
    Library/stm32f10x_sdio.c
    Library/stm32f10x_spi.c
    Library/stm32f10x_tim.c
    Library/stm32f10x_usart.c
    Library/stm32f10x_wwdg.c

    System/Delay.c
    System/syscalls.c

    Hardware/Balance.c
    Hardware/Key.c
    Hardware/LED.c
    Hardware/Motortwo.c
    Hardware/MPU6050.c
    Hardware/OLED.c
    Hardware/OLED_Data.c
    Hardware/PWM.c

    User/main.c
    User/stm32f10x_it.c
)

set_source_files_properties(Hardware/OLED_Data.c PROPERTIES
    COMPILE_OPTIONS -Wno-missing-braces
)

add_executable(${PROJECT_NAME}.elf ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Start
    Library
    User
    System
    Hardware
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Wno-unused-parameter
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE m)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)

find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE arm-none-eabi-size)

if(ARM_OBJCOPY)
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
        COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMENT "Generating HEX and BIN images"
    )
endif()

if(ARM_SIZE)
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${ARM_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
        COMMENT "Printing firmware size"
    )
endif()