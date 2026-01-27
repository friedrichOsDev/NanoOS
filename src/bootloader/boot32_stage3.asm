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

    mov [BOOT_DRIVE], dl ; save boot drive number

    ; load kernel to 0x20000 (Buffer) using LBA read
    ; We load 50 sectors (25KB) to be safe for a growing kernel
    mov si, DAP_KERNEL
    mov ah, 0x42        ; Extended Read
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .lba_failed     ; Error?

.continue:
    ; Enable A20 Line (Fast A20 method)
    in al, 0x92
    or al, 2
    out 0x92, al

    cli ; disable interrupts

    ; load GDT
    lgdt [gdt_ptr] 

    ; set pe bit in cr0 to enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword 0x08:(start32_stage3 + 0x10000)

.lba_failed:
    ; try chs
    mov ax, 0x2000
    mov es, ax
    xor bx, bx          ; Destination ES:BX = 0x2000:0000
    mov ah, 0x02        ; BIOS Read Sectors
    mov al, 64          ; Number of sectors (64 sectors = 32KB)
    mov ch, 0           ; Cylinder 0
    mov cl, 5           ; Start at Sector 5
    mov dh, 0           ; Head 0
    mov dl, [BOOT_DRIVE]; Boot drive
    int 0x13            ; Call BIOS
    jc .chs_failed

    jmp .continue

.chs_failed:
    ; <DEBUG> Print 'E3'
    mov ah, 0x0E
    mov bx, 0x000F
    mov al, 'E'
    int 0x10
    mov al, '3'
    int 0x10
    ; </DEBUG>

    hlt
    jmp $

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

    ; copy kernel (0x20000) to 0x100000
    call copy_kernel

    ; jump to kernel start
    jmp 0x08:0x100000

; SUBROUTINES
; copy kernel 0x20000 to 0x100000
copy_kernel:
    cld                 ; Ensure forward copy
    mov esi, 0x20000    ; source address
    mov edi, 0x100000   ; destination address
    mov ecx, 8192       ; number of dwords to copy (64 sectors * 512 / 4)
    rep movsd           ; copy ECX dwords from [ESI] to [EDI]
    ret

; DATA
; boot drive number
BOOT_DRIVE: db 0

; Disk Address Packet for Kernel Load
DAP_KERNEL:
    db 0x10     ; size of packet
    db 0        ; reserved
    dw 64       ; number of sectors to read (32KB)
    dw 0x0000   ; offset
    dw 0x2000   ; segment (0x20000)
    dq 4        ; starting LBA (Sector 5)

; PADDING to ensure full sector size
times 512 - ($ - $$) db 0