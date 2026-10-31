ENTRY(Reset_Handler)

MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
}

SECTIONS {
    .text : {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.vector_table))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    _sidata = LOADADDR(.data);

    .data : {
        . = ALIGN(4) ;
        _sdata = .;
        *(.data*);
        . = ALIGN(4) ;
        _edata = .;
    } > RAM AT > FLASH

    .bss : {
        . = ALIGN(4);
        _sbss = . ;
        *(.bss*)
        . = ALIGN(4);
        _ebss = .;
    } > RAM


    _estack = ORIGIN(RAM) + LENGTH(RAM);
}