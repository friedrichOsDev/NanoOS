ENTRY(_start)
SECTIONS
{
    . = 0x100000;

    .text : {
        *(.text.entry)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        sbss = .;
        *(COMMON)
        *(.bss)
        ebss = .;
    }

    end = .;
}
