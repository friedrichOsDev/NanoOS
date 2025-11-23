[BITS 16]

; ===== stage 2 entry point =====
start16_stage2:
    ; reinitialize segment register and setup stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 
    
    ; save boot drive
    mov [BOOT_DRIVE], dl

    ; print "S2" (stage 2)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '2'
    int 0x10

    ; getting memory map
    xor ebx, ebx        ; ebx must be 0 for the first call
    mov di, 0x9000      ; Destination buffer offset

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
    ; print "ME" (memory map error)
    mov ah, 0x0E
    mov al, 'M'
    int 0x10
    mov al, 'E'
    int 0x10
    hlt
    jmp $

.mem_map_done:
    ; for now print 'C'
    mov ah, 0x0E
    mov al, 'C'
    int 0x10

    hlt
    jmp $

; ===== data =====
BOOT_DRIVE: db 0

; ===== padding =====
times 2048 - ($ - $$) db 0