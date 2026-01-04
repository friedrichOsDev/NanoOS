[BITS 16]
[ORG 0x7E00]

; BOOTLADER STAGE 2 - 16-BIT REAL MODE --- ENTRY POINT
start16_stage2:
    ; reinitialize segment registers and stack
    xor ax, ax ; ax = 0
    mov ds, ax ; data segment = 0
    mov es, ax ; extra segment = 0
    mov ss, ax ; stack segment = 0
    mov sp, 0x7C00 ; stack pointer = 0x7C00

    ; <DEBUG> print "S2"
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '2'
    int 0x10
    ; </DEBUG>

    hlt 
    jmp $
