[BITS 16]
[ORG 0x7C00]

; BOOTLOADER STAGE 1 - 16-BIT REAL MODE --- ENTRY POINT
start16_stage1:
    ; initialize segment registers and stack
    xor ax, ax ; ax = 0
    mov ds, ax ; data segment = 0
    mov es, ax ; extra segment = 0
    mov ss, ax ; stack segment = 0
    mov sp, 0x7C00 ; stack pointer = 0x7C00

    ; save boot drive number
    mov [BOOT_DRIVE], dl

    ; <DEBUG> print "S1"
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '1'
    int 0x10
    ; </DEBUG>

    ; load the next stage of the bootloader
    ; check lba extensions
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .lba_failed
    cmp bx, 0xAA55
    jne .lba_failed
    test cx, 1
    jz .lba_failed

    ; try lba read
    mov si, DAP
    mov ah, 0x42 ; lba read function
    mov dl, [BOOT_DRIVE] ; boot drive
    int 0x13 ; call BIOS disk service
    jc .lba_failed ; jump if carry flag set (error)

    ; <DEBUG> print "."
    mov ah, 0x0E
    mov al, '.'
    int 0x10
    ; </DEBUG>

    ; jump to the next stage if successful
    jmp 0x0000:0x7E00

.lba_failed:
    ; try chs read (fallback)
    mov ah, 0x02 ; chs read function
    mov al, 0x01 ; number of sectors to read
    mov ch, 0x00 ; cylinder 0
    mov cl, 0x02 ; sector 2
    mov dh, 0x00 ; head 0
    mov dl, [BOOT_DRIVE] ; boot drive
    mov bx, 0x7E00 ; buffer address
    int 0x13 ; call BIOS disk service
    jc .chs_failed ; jump if carry flag set (error)

    ; <DEBUG> print ".."
    mov ah, 0x0E
    mov al, '.'
    int 0x10
    ; </DEBUG>

    ; jump to the next stage if successful
    jmp 0x0000:0x7E00

.chs_failed:
    ; <DEBUG> print "E1"
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, '1'
    int 0x10
    ; </DEBUG>
    
    hlt
    jmp $
    
; DATA
; boot drive
BOOT_DRIVE: db 0
; disk address packet for lba read
DAP:
    db 16 ; size of DAP = 16 bytes
    db 0 ; reserved
    dw 1 ; number of sectors to read
    dw 0x7E00 ; offset
    dw 0x0000 ; segment
    dq 1 ; starting LBA = 1 (sector 2 - stage 2)

; PADDING AND BOOT SIGNATURE
times 510 - ($ - $$) db 0
dw 0xAA55