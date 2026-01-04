[BITS 16]
[ORG 0x0000]

; BOOTLADER STAGE 3 - 16-BIT REAL MODE --- ENTRY POINT
start16_stage3:
    ; reinitialize segment registers and stack
    mov ax, 0x1000 ; Segment 0x1000
    mov ds, ax     ; data segment = 0x1000
    mov es, ax     ; extra segment = 0x1000
    xor ax, ax
    mov ss, ax     ; stack segment = 0
    mov sp, 0x7C00 ; stack pointer = 0x7C00
    
    ; save boot drive number
    mov [BOOT_DRIVE], dl

    cli ; disable interrupts

    ; load GDT
    lgdt [gdt_ptr] 

    ; set pe bit in cr0 to enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword 0x08:(start32_stage3 + 0x10000)

; GDT
gdt_start:
    dq 0x0000000000000000 ; null descriptor
    dq 0x00CF9A000000FFFF ; code segment descriptor
    dq 0x00CF92000000FFFF ; data segment descriptor
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1 ; limit
    dd gdt_start + 0x10000     ; base

[BITS 32]

; BOOTLADER STAGE 3 - 32-BIT PROTECTED MODE --- ENTRY POINT
start32_stage3:
    ; Set up 32-bit data segment registers
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    ; Set stack pointer

    hlt
    jmp $
    
; DATA
; Boot drive number
BOOT_DRIVE: db 0

; PADDING to ensure full sector size
times 512 - ($ - $$) db 0