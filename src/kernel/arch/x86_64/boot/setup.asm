[BITS 32]
section .setup
global _setup
global multiboot_info_ptr
global multiboot_magic
extern _entry

KERNEL_VIRT_BASE equ 0xFFFFFFFF80000000
PML4_INDEX       equ (KERNEL_VIRT_BASE >> 39) & 0x1FF  ; 511
PDPT_INDEX       equ (KERNEL_VIRT_BASE >> 30) & 0x1FF  ; 510

_setup:
    ; Save GRUB Multiboot2 address and magic
    mov [multiboot_info_ptr], ebx
    mov [multiboot_magic], eax

    ; 0. support checks
    call check_cpuid
    cmp eax, 1
    jne .no_long_mode

    call check_long_mode_support
    cmp eax, 1
    jne .no_long_mode

    ; 1. init page tables
    ; 1.1 pml4
    mov eax, boot_pdpt
    or eax, 0x3
    mov [boot_pml4], eax
    mov [boot_pml4 + PML4_INDEX * 8], eax

    ; 1.2 pdpt
    mov eax, boot_pd
    or eax, 0x3
    mov [boot_pdpt], eax
    mov [boot_pdpt + PDPT_INDEX * 8], eax

    ; 1.3 pd
    mov eax, 0x0 | 0x83
    mov [boot_pd], eax

    ; 2. activate PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 3. load PML4 in CR3
    mov eax, boot_pml4
    mov cr3, eax

    ; 4. activate Long Mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 5. enable paging
    mov eax, cr0
    or eax, 1 << 31 ; set PG-bit (bit 31)
    mov cr0, eax

    ; 6. load 64-bit gdt
    lgdt [gdt64_pointer]

    ; 7. jump to 64-bit
    jmp 0x08:init_long_mode

.no_long_mode:
    hlt
    jmp $

; From OSDev Wiki
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jnz .supported
    mov eax, 0
    ret
.supported:
    mov eax, 1
    ret

; From OSDev Wiki
check_long_mode_support:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .not_supported

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .not_supported
    mov eax, 1
    ret
.not_supported:
    mov eax, 0
    ret

[BITS 64]
init_long_mode:
    ; 64-bit mode

    ; 8. reset datasegments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 8. jump to the higher half
    mov rax, _entry
    jmp rax

.hang:
    ; 9. if kernel returns
    cli
    hlt
    jmp .hang

; 64-bit gdt
align 8
gdt64:
    .Null: dq 0                  ; null
    .Code: dq 0x00209A0000000000 ; code: Long Mode + Present + Exec/Read
    .Data: dq 0x0000920000000000 ; data: Present + Writable
gdt64_pointer:
    dw $ - gdt64 - 1
    dq gdt64

; multiboot2 info
align 8
multiboot_info_ptr: dq 0
multiboot_magic: dq 0

; page tables
align 4096
boot_pml4: times 4096 db 0
boot_pdpt: times 4096 db 0
boot_pd:   times 4096 db 0