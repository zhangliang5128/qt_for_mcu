target_compile_definitions(PlatformBSPConfig INTERFACE
    SKIP_SYSCLK_INIT
    $<$<COMPILE_LANGUAGE:ASM>:__STARTUP_CLEAR_BSS>
    $<$<COMPILE_LANGUAGE:ASM>:__STARTUP_INITIALIZE_NONCACHEDATA>

    CPU_MIMXRT1176DVMAA_cm7
    PRINTF_FLOAT_ENABLE=0
    SCANF_FLOAT_ENABLE=0
    PRINTF_ADVANCED_ENABLE=0
    SCANF_ADVANCED_ENABLE=0
    FSL_RTOS_FREE_RTOS
    SDK_I2C_BASED_COMPONENT_USED=1
    SERIAL_PORT_TYPE_UART=1
    USE_SDRAM
    XIP_BOOT_HEADER_ENABLE=1
    XIP_EXTERNAL_FLASH=1
    XIP_BOOT_HEADER_DCD_ENABLE=1
    VGLITE_POINT_FILTERING_FOR_SIMPLE_SCALE
    SDK_OS_FREE_RTOS
)

target_include_directories(PlatformBSPConfig INTERFACE
    ${QUL_BOARD_SDK_DIR}/boards/evkmimxrt1170
    ${QUL_BOARD_SDK_DIR}/boards/evkmimxrt1170/xip
    ${QUL_BOARD_SDK_DIR}/CMSIS/Core/Include
    ${QUL_BOARD_SDK_DIR}/components/gt911
    ${QUL_BOARD_SDK_DIR}/components/serial_manager
    ${QUL_BOARD_SDK_DIR}/components/uart
    ${QUL_BOARD_SDK_DIR}/components/video
    ${QUL_BOARD_SDK_DIR}/components/video/display
    ${QUL_BOARD_SDK_DIR}/components/video/display/dc
    ${QUL_BOARD_SDK_DIR}/components/video/display/dc/lcdifv2
    ${QUL_BOARD_SDK_DIR}/components/video/display/fbdev
    ${QUL_BOARD_SDK_DIR}/components/video/display/mipi_dsi_cmd
    ${QUL_BOARD_SDK_DIR}/components/video/display/rm68191
    ${QUL_BOARD_SDK_DIR}/components/video/display/rm68200
    ${QUL_BOARD_SDK_DIR}/devices/MIMXRT1176
    ${QUL_BOARD_SDK_DIR}/devices/MIMXRT1176/drivers
    ${QUL_BOARD_SDK_DIR}/devices/MIMXRT1176/utilities/debug_console
    ${QUL_BOARD_SDK_DIR}/devices/MIMXRT1176/utilities/str
    ${QUL_BOARD_SDK_DIR}/devices/MIMXRT1176/xip
    ${QUL_BOARD_SDK_DIR}/middleware/vglite/inc
    ${QUL_BOARD_SDK_DIR}/middleware/vglite/VGLite/rtos
    ${QUL_BOARD_SDK_DIR}/middleware/vglite/VGLiteKernel
    ${QUL_BOARD_SDK_DIR}/middleware/vglite/VGLiteKernel/rtos
    ${FREERTOS_DIR}/portable/${FREERTOS_PORT_DIR}/ARM_CM4F
    ${FREERTOS_DIR}/include
)
