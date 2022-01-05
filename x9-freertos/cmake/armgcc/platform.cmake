target_link_options(Platform PUBLIC
    -Wl,-u,cdc_data
    -Wl,-u,image_vector_table
    -Wl,-u,boot_data
    -Wl,-u,qspiflash_config
    -Wl,-u,pxCurrentTCB

    # Fix for Release build problems, it prevents over optimization of those functions
    -Wl,-u,LPI2C_GetCyclesForWidth
    -Wl,-u,LPI2C_MasterInit
)
