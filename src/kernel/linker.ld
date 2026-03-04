ENTRY(_setup)

SECTIONS
{
    . = 0x00100000;
    _kernel_start_phys = .;
    _kernel_virt_base = 0xC0000000;
    _kernel_start = _kernel_start_phys + _kernel_virt_base;
    
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    }

    .setup ALIGN(4K) : 
    {
        *(.setup)
        . = ALIGN(4K);
        boot_page_directory = .;
        . += 4K;
        boot_page_table1 = .;
        . += 4K;
        boot_page_table_zero_window = .;
        . += 4K;
    }

    . += _kernel_virt_base;

    .text ALIGN(4K) : AT(ADDR(.text) - _kernel_virt_base)
    {
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

    .bss ALIGN(4K) : 
    {
        sbss = .;
        *(COMMON)
        *(.bss*)
        ebss = .;
    }

    _kernel_end = .;
    _kernel_end_phys = _kernel_end - _kernel_virt_base;
}
