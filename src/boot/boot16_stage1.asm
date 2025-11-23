[BITS 16]
[ORG 0x7C00]

; ===== stage 1 entry point =====
start16:
    ; initialize segment register and setup stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 

    ; print "S1" (stage 1)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '1'
    int 0x10
    
    ; getting data for kernel
    ; memory map (to 0x8000)
.mem_map_start:
    xor ebx, ebx        ; ebx must be 0 for the first call
    mov di, 0x8000      ; Destination buffer offset

.mem_map_repeat:
    cld ; ensure forward direction

    xor ax, ax
    mov es, ax ; es = 0

    mov cx, 20 ; number of bytes to clear
    mov al, 0

.mem_clear_loop:
    mov [es:di], al
    inc di
    loop .mem_clear_loop

    ; call E820
    xor eax, eax
    mov eax, 0xE820     ; Function number
    mov ecx, 20         ; Request a 20-byte ARD structure
    mov edx, 0x534D4150 ; 'SMAP' signature
    int 0x15
    jc .mem_map_error   ; if carry flag jump to error

    add di, 0           ; after mem clear loop di is incremented 20 times

    test ebx, ebx       ; if ebx = 0 jump to done else repeat
    jz .mem_map_done    
    jmp .mem_map_repeat

.mem_map_error:
    ; print "EM" (error memory map)
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, 'M'
    int 0x10
    hlt
    jmp $

.mem_map_done:
    ; continue here
    ; for now print 'C'
    mov ah, 0x0E
    mov al, 'C'
    int 0x10
    hlt
    jmp $

; ===== padding and bootsignature =====
times 510 - ($ - $$) db 0
dw 0xAA55