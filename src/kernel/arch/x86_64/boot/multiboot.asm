; ==============================================================================
; @file multiboot.asm
; @brief Multiboot2 header for bootloaders (e.g., GRUB2).
; @details Defines magic numbers, framebuffer configuration, relocation header,
; and the end tag.
; @author friedrichOsDev
; ==============================================================================

        [BITS 32]
        section .multiboot
        align 8

; Multiboot2 Constants
        MULTIBOOT2_MAGIC equ 0xE85250D6
        ARCHITECTURE_I386 equ 0
        HEADER_LENGTH equ multiboot_header_end - multiboot_header_start
        CHECKSUM equ 0x100000000 - (MULTIBOOT2_MAGIC + ARCHITECTURE_I386 + HEADER_LENGTH)

; ==============================================================================
; Multiboot2 Header Structure
; ==============================================================================
multiboot_header_start:
        dd MULTIBOOT2_MAGIC
        dd ARCHITECTURE_I386
        dd HEADER_LENGTH
        dd CHECKSUM

; 1. Framebuffer Tag (Graphics mode request: 1024x768 @ 32 BPP)
        align 8
        dw 5                           ; Type = Framebuffer
        dw 0                           ; Flags = 0 (Optional)
        dd 20                          ; Size = 20 bytes
        dd 1024                        ; Width in pixels
        dd 768                         ; Height in pixels
        dd 32                          ; Bits per pixel (color depth)

; 2. Relocatable Header Tag
        align 8
        dw 10                          ; Type = Relocatable
        dw 0                           ; Flags = 0 (Optional)
        dd 24                          ; Size = 24 bytes
        dd 0x00100000                  ; min_addr (1 MB base)
        dd 0xFFFFFFFF                  ; max_addr
        dd 4096                        ; Alignment (4 KB)
        dd 0                           ; Preference (0 = No preference)

; 3. End Tag (Terminates the Multiboot2 header)
        align 8
        dw 0                           ; Type = End
        dw 0                           ; Flags = 0
        dd 8                           ; Size = 8 bytes

multiboot_header_end:
