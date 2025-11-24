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
    ; print "MC" (memory map complete)
    mov ah, 0x0E
    mov al, 'M'
    int 0x10
    mov al, 'C'
    int 0x10

    ; get vbe info
    mov di, VBE_INFO_BLOCK
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .vbe_error

    ; print "VO" (VBE OK)
    mov ah, 0x0E
    mov al, 'V'
    int 0x10
    mov al, 'O'
    int 0x10

    ; save mode infos
    ; get pointer to video mode list
    mov ax, [VBE_INFO_BLOCK.video_mode_ptr + 2] ; segment
    mov es, ax
    mov si, [VBE_INFO_BLOCK.video_mode_ptr]     ; offset

.find_mode:
    mov cx, [es:si]     ; get mode number from list
    cmp cx, 0xFFFF      ; check for end of list marker
    je .no_modes

    mov di, MODE_INFO_BLOCK
    mov ax, 0x4F01
    int 0x10
    cmp ax, 0x004F
    jne .vbe_error

    jmp .vbe_done

.no_modes:
    ; print "NM" (no modes)
    mov ah, 0x0E
    mov al, 'N'
    int 0x10
    mov al, 'M'
    int 0x10
    hlt
    jmp $

.vbe_error:
    ; print "VE" (VBE error) and halt
    mov ah, 0x0E
    mov al, 'V'
    int 0x10
    mov al, 'E'
    int 0x10
    hlt
    jmp $

.vbe_done:
    ; print "GC" (graphics complete)
    mov ah, 0x0E
    mov al, 'G'
    int 0x10
    mov al, 'C'
    int 0x10
    hlt
    jmp $

; ===== data =====
BOOT_DRIVE: db 0
; VBE Info Block structure
VBE_INFO_BLOCK:
    .signature: times 4 db 0 
    .version: dw 0         
    .oem_string_ptr: dd 0         
    .capabilities: dd 0         
    .video_mode_ptr: dd 0         
    .total_memory: dw 0         
    .oem_sw_rev: dw 0         
    .oem_vendor_name_ptr: dd 0    
    .oem_product_name_ptr: dd 0   
    .oem_product_rev_ptr: dd 0    
    .reserved: times 222 db 0 
    .oem_data: times 256 db 0 

; VBE Mode Info Block structure
MODE_INFO_BLOCK:
    .attributes: dw 0               
    .window_a: db 0                 
    .window_b: db 0                 
    .granularity: dw 0              
    .window_size: dw 0              
    .segment_a: dw 0                
    .segment_b: dw 0                
    .win_func_ptr: dd 0             
    .pitch: dw 0                    
    .width: dw 0                    
    .height: dw 0                   
    .w_char: db 0                   
    .y_char: db 0                   
    .planes: db 0                   
    .bpp: db 0                      
    .banks: db 0                    
    .memory_model: db 0             
    .bank_size: db 0                
    .image_pages: db 0              
    .reserved0: db 0                

    ; direct Color fields
    .red_mask: db 0                 
    .red_position: db 0             
    .green_mask: db 0               
    .green_position: db 0           
    .blue_mask: db 0                
    .blue_position: db 0            
    .reserved_mask: db 0            
    .reserved_position: db 0        
    .direct_color_attributes: db 0  

    ; VBE 2.0 fields
    .framebuffer: dd 0              
    .off_screen_mem_off: dd 0       
    .off_screen_mem_size: dw 0      
    .reserved1: times 206 db 0      

; ===== padding =====
times 2048 - ($ - $$) db 0