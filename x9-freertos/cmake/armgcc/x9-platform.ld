/* Linker script to configure memory regions.
 * Need modifying for a specific board.
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */
MEMORY
{
  FLASH (rx)               : ORIGIN = 0x08000000, LENGTH = 2048K /* internal flash */
  RAM (xrw)                : ORIGIN = 0x20000000, LENGTH = 512K /* internal sram */
  SDRAM (xrw)              : ORIGIN = 0xc0400000,  LENGTH = 8M /* external sdram memory */
  QSPI (rx)                : ORIGIN = 0x90000000, LENGTH = 64M /* external flash */
}

_estack = 0x20080000;    /* end of internal SRAM */

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text)
        *(.text*)
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)
        *(.eh_frame*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        _etext = .;

        *(.rodata)
        *(.rodata*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    _sidata = LOADADDR(.data);

    /* Location counter can end up 2byte aligned with narrow Thumb code but
       __etext is assumed by startup code to be the LMA of a section in RAM
       which must be 4byte aligned */
    __etext = ALIGN (4);

    .data :
    {
        _sdata = .;
        __data_start__ = _sdata;
        *(vtable)
        *(.data)
        *(.data*)

        . = ALIGN(4);
        /* All data end */
        _edata = .;
        __data_end__ = _edata;

    } > RAM AT> FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > RAM

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __bss_end__, "region .bss overflowed with stack")

    AssetDataPreload :
    {
        . = ALIGN(4);
        _preloadable_assetdata_dst_begin = .;
        *(AssetDataPreload)
        . = ALIGN(4);
        _preloadable_assetdata_dst_end = .;
    } > SDRAM AT > QSPI

    _preloadable_assetdata_src = LOADADDR(AssetDataPreload);

    .heap (COPY):
    {
        __end__ = .;
        PROVIDE(end = .);
        __HeapStart = .;
        *(.heap*)
        __HeapLimit = .;
    } > SDRAM

    AssetDataKeepInFlash :
    {
        . = ALIGN(4);
        *(AssetDataKeepInFlash)
    } > QSPI
}
