ENTRY(_setup)

SECTIONS
{
    . = 0x00100000;
    _kernel_start_phys = .;

    _kernel_virt_base = 0xFFFFFFFF80000000;

    /* Identity Mapped */
    .multiboot ALIGN(4K) :
    {
        KEEP(*(.multiboot))
    }

    .setup ALIGN(4K) :
    {
        *(.setup)
    }

    _kernel_setup_end_phys = .;

    /* VMA (Virtual Memory Address) is at 0xFFFF80000... */
    .text (_kernel_setup_end_phys + _kernel_virt_base) : AT(_kernel_setup_end_phys)
    {
        _kernel_start = .;
        *(.text.entry)
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - _kernel_virt_base)
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - _kernel_virt_base)
    {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - _kernel_virt_base)
    {
        sbss = .;
        *(COMMON)
        *(.bss*)
        ebss = .;
    }

    _kernel_end = .;
    _kernel_end_phys = _kernel_end - _kernel_virt_base;
}