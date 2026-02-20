[BITS 32]

section .setup
global _setup
extern _start
extern boot_page_table1
extern boot_page_table_zero_window
extern boot_page_directory

_setup:
    ; save multiboot data
    mov edx, eax
    mov ebp, ebx

    ; fill boot boot_page_table1
    mov edi, boot_page_table1
    mov eax, 0x00000003 ; Flags: present, read/write
    mov ecx, 1024
.fill_table:
    mov [edi], eax
    add edi, 4
    add eax, 4096
    loop .fill_table

    ; map kernel
    mov eax, boot_page_table1
    or eax, 0x003 ; Flags: present, read/write
    mov [boot_page_directory + 0], eax ; identity mapping
    mov [boot_page_directory + 768 * 4], eax ; map kernel to 0xC0000000

    ; map zero window
    mov eax, boot_page_table_zero_window
    or eax, 0x003
    mov [boot_page_directory + 1022 * 4], eax

    ; enable paging
    mov eax, boot_page_directory
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; restore multiboot data
    mov eax, edx
    mov ebx, ebp

    lea ecx, [_start]
    jmp ecx
    