[BITS 16]
[ORG 0x7C00]

; ===== stage 1 entry point =====
start16_stage1:
    ; initialize segment register and setup stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 
    
    ; save boot drive 
    mov [BOOT_DRIVE], dl

    ; print "S1" (stage 1)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '1'
    int 0x10

    ; try LBA read
    mov si, DAP
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .lba_failed

    ; jump to stage2
    jmp 0x0000:0x7E00

.lba_failed:
    ; print "LF" (LBA fallback)
    mov ah, 0x0E
    mov al, 'L'
    int 0x10
    mov al, 'F'
    int 0x10

    ; try CHS read
    mov ah, 0x02
    mov al, 0x04
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [BOOT_DRIVE]
    mov bx, 0x7E00
    int 0x13
    jc .chs_failed

    ; jump to stage2
    jmp 0x0000:0x7E00

.chs_failed:
    ; print "CE" (CHS error)
    mov ah, 0x0E
    mov al, 'C'
    int 0x10
    mov al, 'E'
    int 0x10
    hlt
    jmp $

; ===== data =====
BOOT_DRIVE: db 0
DAP:
    db 16         ; size of DAP = 16 bytes
    db 0          ; reserved
    dw 4          ; number of sectors to read
    dw 0x7E00     ; offset
    dw 0x0000     ; segment
    dq 1          ; starting LBA = 1 (stage 2 starts after bootsector)

; ===== padding and bootsignature =====
times 510 - ($ - $$) db 0
dw 0xAA55