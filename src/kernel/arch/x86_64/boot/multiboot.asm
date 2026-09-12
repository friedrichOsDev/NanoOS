        [BITS 32]
        section .multiboot
        align 8

; Multiboot2 Constants
        MULTIBOOT2_MAGIC equ 0xE85250D6
        ARCHITECTURE_I386 equ 0
        HEADER_LENGTH equ multiboot_header_end - multiboot_header_start
        CHECKSUM equ 0x100000000 - (MULTIBOOT2_MAGIC + ARCHITECTURE_I386 + HEADER_LENGTH)

multiboot_header_start:
        dd MULTIBOOT2_MAGIC
        dd ARCHITECTURE_I386
        dd HEADER_LENGTH
        dd CHECKSUM

; 1. Framebuffer Tag
        align 8
        dw 5                           ; type = Framebuffer
        dw 0                           ; flags = 0 (Optional)
        dd 20                          ; size = 20 Bytes
        dd 1024                        ; width
        dd 768                         ; height
        dd 32                          ; bpp

; 2. Relocatable Header Tag
        align 8
        dw 10                          ; type = Relocatable
        dw 0                           ; flags = 0 (Optional)
        dd 24                          ; size = 24 Bytes
        dd 0x00100000                  ; min_addr (1 MB)
        dd 0xFFFFFFFF                  ; max_addr
        dd 4096                        ; align (4 KB)
        dd 0                           ; preference (0 = none)

; 3. End Tag
        align 8
        dw 0                           ; type = End
        dw 0                           ; flags = 0
        dd 8                           ; size = 8 Bytes

multiboot_header_end:
