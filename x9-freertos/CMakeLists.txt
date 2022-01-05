SET(SDK_ENV_VARIABLE_NAME SEMIDRIVE_X9_SDK_DIR)
SET(SDK_DIR $ENV{${SDK_ENV_VARIABLE_NAME}})
STRING(REGEX REPLACE "\\\\" "/" SDK_DIR "${SDK_DIR}")

IF(NOT SDK_DIR)
    MESSAGE(FATAL_ERROR "***Please set ${SDK_ENV_VARIABLE_NAME} environment variable***")
ENDIF()
MESSAGE(STATUS "SDK_DIR: " ${SDK_DIR})

SET(FREERTOS_ENV_VARIABLE_NAME SEMIDRIVE_X9_FREERTOS_DIR)
set(FREERTOS_DIR $ENV{${FREERTOS_ENV_VARIABLE_NAME}})
if (NOT DEFINED FREERTOS_DIR)
    message(FATAL_ERROR "***${FREERTOS_ENV_VARIABLE_NAME} environment variable is undefined!***")
endif()
# set(FREERTOS_DIR ${SDK_DIR}/kernel/lk_wrapper)

#! [Platform CMakeLists]
MESSAGE(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(QuickUltralitePlatform PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sdrvlayerengine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sdrvlayerengine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem.cpp

    # ${SDK_DIR}/include/sys/types.h

    # {SDK_DIR}/platform/kunlun/safety/timer.c
    # ${SDK_DIR}/lib/heap/heap.h
    # ${SDK_DIR}/lib/heap/heap_wrapper.c

    # ${SDK_DIR}/exdev/display/include/sdm_display.h
    # ${SDK_DIR}/exdev/display/sdm_display.c

    # ${SDK_DIR}/hal/timer_hal/sd_timer_hal/src/timer_hal.c
    # ${SDK_DIR}/hal/disp_hal/sd_disp_hal/inc/disp_hal.h
    # ${SDK_DIR}/hal/disp_hal/sd_disp_hal/src/disp_hal.c
    # ${SDK_DIR}/hal/res/inc/res.h
    # ${SDK_DIR}/chipdev/timer/sd_timer/src/timer_drv.c
    # ${SDK_DIR}/chipcfg/generate/g9e/chip/chip_res.h

    # ${FREERTOS_DIR}/lk_event.c
    # ${FREERTOS_DIR}/lk_mutex.c
    # ${FREERTOS_DIR}/lk_semaphore.c
    # ${FREERTOS_DIR}/lk_thread.c
    # ${FREERTOS_DIR}/lk_timers.c
    # ${FREERTOS_DIR}/portable/MemMang/sdheap_4.c
    # ${FREERTOS_DIR}/portable/GCC/ARM_CR5/port.c
    # ${FREERTOS_DIR}/tasks.c
    # ${FREERTOS_DIR}/list.c

    ${PLATFORM_COMMON_SRC_DIR}/singlepointtoucheventdispatcher.cpp
    # ${PLATFORM_COMMON_SRC_DIR}/freertos/memalloc.cpp
    ${PLATFORM_COMMON_SRC_DIR}/freertos/os.c
)


target_include_directories(QuickUltralitePlatform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add platform specific include directories here

    ${SDK_DIR}/include
    ${SDK_DIR}/include/lib
    ${SDK_DIR}/include/uapi/uapi
    ${SDK_DIR}/include/sys
    ${SDK_DIR}/include/shared
    ${SDK_DIR}/include/kernel
    ${SDK_DIR}/include/shared/lk
    ${SDK_DIR}/arch/arm/include
    ${SDK_DIR}/arch/arm/arm-m/include
    ${SDK_DIR}/arch/arm/arm/include
    ${SDK_DIR}/platform/kunlun/safety
    ${SDK_DIR}/platform/kunlun/common/arm/platform
    ${SDK_DIR}/kernel/lk_wrapper
    ${SDK_DIR}/lib/io/include
    ${SDK_DIR}/lib/cbuf/include
    ${SDK_DIR}/lib/iovec
    ${SDK_DIR}/lib/heap
    ${SDK_DIR}/lib/system_config
    #${SDK_DIR}/lib/libc/include
    ${SDK_DIR}/exdev/display
    ${SDK_DIR}/exdev/display/include
    ${SDK_DIR}/exdev/lcm
    ${SDK_DIR}/hal/boardinfo
    ${SDK_DIR}/hal/disp_hal/sd_disp_hal/inc
    ${SDK_DIR}/hal/disp_hal/sd_disp_hal/lib/inc
    ${SDK_DIR}/hal/timer_hal/sd_timer_hal/inc
    ${SDK_DIR}/hal/g2dlite_hal
    ${SDK_DIR}/hal/g2dlite_hal/sd_g2dlite_hal/inc/
    ${SDK_DIR}/hal/res/inc
    ${SDK_DIR}/chipdev/timer/sd_timer/inc
    ${SDK_DIR}/chipcfg/generate/x9_mid/chip
    ${SDK_DIR}/chipcfg/generate/x9_mid/projects/serdes
    ${SDK_DIR}/chipcfg/generate/x9_mid/projects/serdes/safety
    ${SDK_DIR}/target/reference_x9/safety
    ${SDK_DIR}/chipdev/timer/sd_timer/inc
    # freeRTOS support
    ${FREERTOS_DIR}
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CR5
)

target_compile_options(
    QuickUltralitePlatform
    PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -Wno-implicit-fallthrough
)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc")
# set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

target_compile_definitions(QuickUltralitePlatform PRIVATE
    # Insert platform specific compile flags here
    # e.g. APPLICATION_ADDRESS=0x90000000
    # CPU_MIMXRT1176DVMAA_cm7
    # PRINTF_FLOAT_ENABLE=0
    # SCANF_FLOAT_ENABLE=0
    # PRINTF_ADVANCED_ENABLE=0
    # SCANF_ADVANCED_ENABLE=0
    # FSL_RTOS_FREE_RTOS
    # SDK_I2C_BASED_COMPONENT_USED=1
    # SERIAL_PORT_TYPE_UART=1
    # USE_SDRAM
    # XIP_BOOT_HEADER_ENABLE=1
    # XIP_EXTERNAL_FLASH=1
    # XIP_BOOT_HEADER_DCD_ENABLE=0
)

install_board_platform_packages()
#! [Platform CMakeLists]
