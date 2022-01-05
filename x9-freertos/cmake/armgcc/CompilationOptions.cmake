#! [Example CompilationOptions.cmake]
add_compile_definitions(
    # Archtitecture specific compile definitions
    USE_HAL_DRIVER
)

add_compile_options(
    # Archtitecture specific compile options
    -mcpu=cortex-r5
    -mfloat-abi=hard
    -mfpu=vfpv3-d16
)

add_compile_definitions(
    QUL_STATIC_NO_PRELOAD_ASSET_SEGMENT=AssetDataKeepInFlash
    QUL_STATIC_ASSET_SEGMENT=AssetDataPreload
)

add_link_options(
    # Archtitecture specific link options
    -mfloat-abi=hard
    -mfpu=vfpv3-d16
    -mcpu=cortex-r5
)

#! [Example CompilationOptions.cmake]
