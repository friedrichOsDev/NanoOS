	[BITS   32]
	section .multiboot
	align   8

	;        Multiboot Magic Numbers
	MULTIBOOT2_MAGIC equ 0xE85250D6
	ARCHITECTURE_I386 equ 0
	HEADER_LENGTH equ multiboot_header_end - multiboot_header_start
	CHECKSUM equ 0x100000000 - (MULTIBOOT2_MAGIC + ARCHITECTURE_I386 + HEADER_LENGTH)

multiboot_header_start:
	dd MULTIBOOT2_MAGIC
	dd ARCHITECTURE_I386
	dd HEADER_LENGTH
	dd CHECKSUM

	; Optional: Other Tags (Framebuffer, ...)

	;     Framebuffer Tag
	align 8
	dw    5
	dw    0
	dd    20
	dd    0
	dd    0
	dd    0

	;     Relocatable Header Tag
	align 8
	dw    10
	dw    0
	dd    24
	dd    0x00100000
	dd    0xFFFFFFFF
	dd    4096
	dd    0

	;     End Tag
	align 8
	dw    0; type = 0
	dw    0; flags = 0
	dd    8; size = 8

multiboot_header_end:
