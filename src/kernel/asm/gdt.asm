[BITS 32]
global gdt_flush

gdt_flush:
    mov eax, [esp + 4]    ; address of gdt_ptr structure
    lgdt [eax]            ; load gdt

    mov ax, 0x10          ; data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush_done  ; far jump to new code segment

.flush_done:
    ret
