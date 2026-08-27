	[BITS   64]
	section .text

	global gdt_flush

gdt_flush:
	lgdt [rdi]

	push 0x08
	lea  rax, [rel .flush]
	push rax
	retfq

.flush:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	ret

global tss_load

tss_load:
	ltr di
	ret
