ENTRY(_setup)

SECTIONS
{
    . = 0x00100000;
    kernel_start_phys = .;

    kernel_virt_base = 0xFFFFFFFF80000000;

    /* Identity Mapped */
    .multiboot ALIGN(4K) :
    {
        KEEP(*(.multiboot))
    }

    .setup ALIGN(4K) :
    {
        *(.setup)
    }

    kernel_setup_end_phys = .;

    /* VMA (Virtual Memory Address) is at 0xFFFF80000... */
    .text (kernel_setup_end_phys + kernel_virt_base) : AT(kernel_setup_end_phys)
    {
        kernel_start = .;
        *(.text.entry)
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - kernel_virt_base)
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - kernel_virt_base)
    {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - kernel_virt_base)
    {
        sbss = .;
        *(COMMON)
        *(.bss*)
        ebss = .;
    }

    kernel_end = .;
    kernel_end_phys = kernel_end - kernel_virt_base;
}