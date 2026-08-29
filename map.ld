ENTRY(Reset_Handler)

MEMORY {
    FLASH (rw) : ORIGIN = 0x08000000, LENGTH = 128k
    MEMORY (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
    PERIPH (rw) : ORIGIN = 0x40000000, LENGTH = 512M
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