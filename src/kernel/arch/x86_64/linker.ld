ENTRY(_setup)

SECTIONS
{
    /* Physical Base (1 MB) */
    . = 0x00100000;
    kernel_start_phys = .;

    /* Virtual Higher-Half Base */
    kernel_virt_base = 0xFFFFFFFF80000000;

    /* --- Identity Mapped Low-Memory Sections (32-Bit Setup) --- */

    .multiboot ALIGN(4K) :
    {
        KEEP(*(.multiboot))
    }

    .setup ALIGN(4K) :
    {
        *(.setup)
    }

    . = ALIGN(4K);
    kernel_setup_end_phys = .;

    /* --- Higher-Half Sections (64-Bit Execution) --- */

    . = kernel_virt_base + kernel_setup_end_phys;

    .text ALIGN(4K) : AT(kernel_setup_end_phys)
    {
        kernel_start = .;
        *(.text.entry)
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(LOADADDR(.text) + (ADDR(.rodata) - ADDR(.text)))
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(LOADADDR(.text) + (ADDR(.data) - ADDR(.text)))
    {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(LOADADDR(.text) + (ADDR(.bss) - ADDR(.text)))
    {
        sbss = .;
        *(COMMON)
        *(.bss*)
        . = ALIGN(8);
        ebss = .;
    }

    /* Kernel End-Symbole */
    . = ALIGN(4K);
    kernel_end = .;
    kernel_end_phys = kernel_end - kernel_virt_base;
}