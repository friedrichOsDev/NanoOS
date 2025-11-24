[BITS 16]
[ORG 0x10000]

; ===== stage 3 entry point =====
start16_stage3:
    mov ax, cs
    mov ds, ax
    
    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword 0x08:start32_stage3

; ===== GDT =====
gdt_start:
    dq 0x0000000000000000 ; null descriptor
    dq 0x00CF9A000000FFFF ; 32-bit code segment
    dq 0x00CF92000000FFFF ; 32-bit data segment
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]

start32_stage3:
    ; setup segment and stack
    mov eax, 0x0010       
    mov ds, eax         
    mov es, eax         
    mov fs, eax         
    mov gs, eax         
    mov ss, eax         
    mov esp, 0xF00000



    hlt
    jmp $
