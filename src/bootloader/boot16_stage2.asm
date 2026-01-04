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

    ; save boot drive number
    mov [BOOT_DRIVE], dl

    ; getting memory map
    call do_e820
    jc .e820_failed ; if carry set, e820 failed

    ; set VESA mode 1680x1050x32
    mov ax, 1680
    mov bx, 1050
    mov cl, 32
    call vbe_set_mode
    jc .vbe_set_mode_failed ; if carry set, vbe_set_mode failed

	; load the next stage of the bootloader
    ; try lba read
    mov si, DAP
    mov ah, 0x42 ; lba read function
    mov dl, [BOOT_DRIVE] ; boot drive
    int 0x13 ; call BIOS disk service
    jc .lba_failed ; jump if carry flag set (error)

    ; restore boot drive for stage 3
    mov dl, [BOOT_DRIVE]

    ; jump to the next stage if successful
    jmp 0x1000:0x0000

.lba_failed:
    ; try chs read (fallback)
    mov ah, 0x02 ; chs read function
    mov al, 0x01 ; number of sectors to read
    mov ch, 0x00 ; cylinder 0
    mov cl, 0x04 ; sector 4 (LBA 3)
    mov dh, 0x00 ; head 0
    mov dl, [BOOT_DRIVE] ; boot drive
    mov bx, 0x1000 ; buffer address
	mov es, bx
    mov bx, 0x0000 ; offset
    int 0x13 ; call BIOS disk service
    jc .chs_failed ; jump if carry flag set (error)	

    ; restore boot drive for stage 3
    mov dl, [BOOT_DRIVE]

    ; jump to the next stage if successful
    jmp 0x1000:0x0000

.chs_failed:
    ; <DEBUG> print "E2"
	mov bx, 0x000F
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, '2'
    int 0x10
    ; </DEBUG>
    
    hlt
    jmp $

.e820_failed:
    ; <DEBUG> print "E8"
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, '8'
    int 0x10
    ; </DEBUG>

    hlt
    jmp $

.vbe_set_mode_failed:
    ; <DEBUG> print "EV"
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, 'V'
    int 0x10
    ; </DEBUG>

    hlt
    jmp $

; SUBROUTINES
; get memory map via int 0x15, eax=0xe820
mmap_entries equ 0x8200
do_e820:
    mov di, 0x8204          ; Set di to 0x8004. Otherwise this code will get stuck in `int 0x15` after some entries are fetched 
	xor ebx, ebx		; ebx must be 0 to start
	xor bp, bp		; keep an entry count in bp
	mov edx, 0x0534D4150	; Place "SMAP" into edx
	mov eax, 0xe820
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes
	int 0x15
	jc short .failed	; carry set on first call means "unsupported function"
	mov edx, 0x0534D4150	; Some BIOSes apparently trash this register?
	cmp eax, edx		; on success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx		; ebx = 0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xe820		; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes again
	int 0x15
	jc short .e820f		; carry set means "end of list already reached"
	mov edx, 0x0534D4150	; repair potentially trashed register
.jmpin:
	jcxz .skipent		; skip any 0 length entries
	cmp cl, 20		; got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1	; if so: is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8]	; get lower uint32_t of memory region length
	or ecx, [es:di + 12]	; "or" it with upper uint32_t to test for zero
	jz .skipent		; if length uint64_t is 0, skip entry
	inc bp			; got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx		; if ebx resets to 0, list is complete
	jne short .e820lp
.e820f:
	mov [es:mmap_entries], bp	; store the entry count
	clc			; there is "jc" on end of list to this point, so the carry must be cleared
	ret
.failed:
	stc			; "function unsupported" error exit
	ret

; vbe_set_mode:
; Sets a VESA mode
; In\	AX = Width
; In\	BX = Height
; In\	CL = Bits per pixel
; Out\	FLAGS = Carry clear on success
; Out\	Width, height, bpp, physical buffer, all set in vbe_screen structure

vbe_set_mode:
	mov [.width], ax
	mov [.height], bx
	mov [.bpp], cl

	push es					; some VESA BIOSes destroy ES, or so I read
	mov ax, 0x4F00				; get VBE BIOS info
	mov di, vbe_info_block
	int 0x10
	pop es

	cmp ax, 0x4F				; BIOS doesn't support VBE?
	jne .error

	mov ax, word[vbe_info_block.video_modes]
	mov [.offset], ax
	mov ax, word[vbe_info_block.video_modes+2]
	mov [.segment], ax

	mov ax, [.segment]
	mov fs, ax
	mov si, [.offset]

.find_mode:
	mov dx, [fs:si]
	add si, 2
	mov [.offset], si
	mov [.mode], dx
	mov ax, 0
	mov fs, ax

	cmp word [.mode], 0xFFFF		; end of list?
	je .error

	push es
	mov ax, 0x4F01				; get VBE mode info
	mov cx, [.mode]
	mov di, mode_info_block
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

	mov ax, [.width]
	cmp ax, [mode_info_block.width]
	jne .next_mode

	mov ax, [.height]
	cmp ax, [mode_info_block.height]
	jne .next_mode

	mov al, [.bpp]
	cmp al, [mode_info_block.bpp]
	jne .next_mode

	; If we make it here, we've found the correct mode!
	mov ax, [.width]
	mov word[vbe_screen.width], ax
	mov ax, [.height]
	mov word[vbe_screen.height], ax
	mov eax, [mode_info_block.framebuffer]
	mov dword[vbe_screen.physical_buffer], eax
	mov ax, [mode_info_block.pitch]
	mov word[vbe_screen.bytes_per_line], ax
	mov eax, 0
	mov al, [.bpp]
	mov byte[vbe_screen.bpp], al
	shr eax, 3
	mov dword[vbe_screen.bytes_per_pixel], eax

	mov ax, [.width]
	shr ax, 3
	dec ax
	mov word[vbe_screen.x_cur_max], ax

	mov ax, [.height]
	shr ax, 4
	dec ax
	mov word[vbe_screen.y_cur_max], ax

	; Set the mode
	push es
	mov ax, 0x4F02
	mov bx, [.mode]
	or bx, 0x4000			; enable LFB
	mov di, 0			; not sure if some BIOSes need this... anyway it doesn't hurt
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

	clc
	ret

.next_mode:
	mov ax, [.segment]
	mov fs, ax
	mov si, [.offset]
	jmp .find_mode

.error:
	stc
	ret

.width				dw 0
.height				dw 0
.bpp				db 0
.segment			dw 0
.offset				dw 0
.mode				dw 0

; DATA
; boot drive
BOOT_DRIVE: db 0
; disk address packet for lba read
DAP:
    db 16 ; size of DAP = 16 bytes
    db 0 ; reserved
    dw 1 ; number of sectors to read
    dw 0x0000 ; offset
    dw 0x1000 ; segment
    dq 3 ; starting LBA = 3 (sector 4 - stage 3)

; PADDING
times 1024 - ($ - $$) db 0

absolute 0x9000

vbe_info_block:
    .signature:      resb 4
    .version:        resw 1
    .oem_string_ptr: resd 1
    .capabilities:   resd 1
    .video_modes:    resd 1  ; Offset 14 (0xE)
    .video_memory:   resw 1
    .software_rev:   resw 1
    .vendor:         resd 1
    .product_name:   resd 1
    .product_rev:    resd 1
    .reserved:       resb 222
    .oem_data:       resb 256

mode_info_block:
    .attributes:     resw 1
    .window_a:       resb 1
    .window_b:       resb 1
    .granularity:    resw 1
    .window_size:    resw 1
    .segment_a:      resw 1
    .segment_b:      resw 1
    .win_func_ptr:   resd 1
    .pitch:          resw 1  ; Offset 16 (0x10)
    .width:          resw 1  ; Offset 18 (0x12)
    .height:         resw 1  ; Offset 20 (0x14)
    .w_char:         resb 1
    .y_char:         resb 1
    .planes:         resb 1
    .bpp:            resb 1  ; Offset 25 (0x19)
    .banks:          resb 1
    .memory_model:   resb 1
    .bank_size:      resb 1
    .image_pages:    resb 1
    .reserved0:      resb 1
    .red_mask:       resb 1
    .red_position:   resb 1
    .green_mask:     resb 1
    .green_position: resb 1
    .blue_mask:      resb 1
    .blue_position:  resb 1
    .rsv_mask:       resb 1
    .rsv_position:   resb 1
    .direct_color_attributes: resb 1
    .framebuffer:    resd 1  ; Offset 40 (0x28)
    .offscreen_mem_off: resd 1
    .offscreen_mem_size: resw 1
    .reserved1:      resb 206

vbe_screen:
    .width:           resw 1
    .height:          resw 1
    .bpp:             resb 1
    .bytes_per_line:  resw 1
    .physical_buffer: resd 1
    .bytes_per_pixel: resd 1
    .x_cur_max:       resw 1
    .y_cur_max:       resw 1