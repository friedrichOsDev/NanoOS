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
    xor ebx, ebx       
    mov di, 0x8000 
.mem_map_repeat:
    mov eax, 0xE820     ; EAX = function number
    mov ecx, 24         ; ECX = size of the buffer
    mov edx, 0x534D4150 ; EDX = 'SMAP' signature

    int 0x15
    jc .mem_map_error   ; if carry flag jump to error

    add di, 24          ; add 24 bytes to buffer
    test ebx, ebx       ; if EBX = 0 jump to done else repeat
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