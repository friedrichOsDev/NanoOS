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
    hlt
    jmp $

.mem_map_done:
    ; get vbe info
    mov di, VBE_INFO_BLOCK
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .vbe_error

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
    
    ; check if mode is 1680x1050x32
    mov ax, [MODE_INFO_BLOCK.width]
    cmp ax, 1680
    jne .next_mode

    mov ax, [MODE_INFO_BLOCK.height]
    cmp ax, 1050
    jne .next_mode

    mov al, [MODE_INFO_BLOCK.bpp]
    cmp al, 32
    jne .next_mode

    ; save the mode number
    mov [VBE_MODE], cx

    jmp .vbe_done ; found the mode

.no_modes:
    hlt
    jmp $

.next_mode:
    add si, 2 ; move to the next mode in the list
    jmp .find_mode

.vbe_error:
    hlt
    jmp $

.vbe_done:
    ; get acpi tables !!TODO!!
    jmp .acpi_done

.acpi_error:
    hlt
    jmp $
    
.acpi_done:
    ; set the video mode we found
    mov bx, [VBE_MODE]
    or bx, 0x4000 
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .vbe_error

    ; load stage 3
    mov si, DAP
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .lba_failed

    ; Sprung zu Stage 3 bei 0x1000:0000 (phys. 0x10000)
    jmp 0x1000:0x0000

.lba_failed:
    ; chs
    mov ah, 0x02
    mov al, 0x04
    mov ch, 0x00
    mov cl, 0x06
    mov dh, 0x00
    mov dl, [BOOT_DRIVE]
    mov bx, 0x1000  ; Set segment to 0x1000
    mov es, bx      ; ES = 0x1000
    mov bx, 0x0000  ; BX = 0x0000. Destination ES:BX = 0x1000:0x0000 (phys. 0x10000)
    int 0x13
    jc .chs_failed

    ; jump to stage3
    jmp 0x1000:0x0000

.chs_failed:
    hlt
    jmp $

; ===== data =====
BOOT_DRIVE: db 0
VBE_MODE: dw 0

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

; disk address packet for stage 3
DAP:
    db 16         ; size of DAP = 16 bytes
    db 0          ; reserved
    dw 4          ; number of sectors to read
    dw 0x0000     ; offset
    dw 0x1000     ; segment
    dq 5          ; starting LBA = 5

; ===== padding =====
times 2048 - ($ - $$) db 0
