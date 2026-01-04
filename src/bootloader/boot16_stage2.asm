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

    ; <DEBUG> print "S2"
    mov ah, 0x0E
    mov al, 'S'
    int 0x10
    mov al, '2'
    int 0x10
    ; </DEBUG>

    ; getting memory map
    call do_e820
    jc .e820_failed ; if carry set, e820 failed

    ; <DEBUG> print "MMOK"
    mov ah, 0x0E
    mov al, 'M'
    int 0x10
    mov al, 'M'
    int 0x10
    mov al, 'O'
    int 0x10
    mov al, 'K'
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

; SUBROUTINES
; get memory map via int 0x15, eax=0xe820
mmap_entries equ 0x8000
do_e820:
    mov di, 0x8004          ; Set di to 0x8004. Otherwise this code will get stuck in `int 0x15` after some entries are fetched 
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

; DATA
; boot drive
BOOT_DRIVE: db 0

; PADDING
times 512 - ($ - $$) db 0