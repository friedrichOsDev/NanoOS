; /**
; * @file multiboot.asm
; * @brief Multiboot2-Header für den Bootloader (z. B. GRUB2).
; * @details Definiert Magics, Framebuffer-Konfiguration, Relocation-Header und End-Tag.
; * @author friedrichOsDev
; */

        [BITS 32]
        section .multiboot
        align 8

; Multiboot2 Konstanten
        MULTIBOOT2_MAGIC equ 0xE85250D6
        ARCHITECTURE_I386 equ 0
        HEADER_LENGTH equ multiboot_header_end - multiboot_header_start
        CHECKSUM equ 0x100000000 - (MULTIBOOT2_MAGIC + ARCHITECTURE_I386 + HEADER_LENGTH)

; ==============================================================================
; Multiboot2 Header Struktur
; ==============================================================================
multiboot_header_start:
        dd MULTIBOOT2_MAGIC
        dd ARCHITECTURE_I386
        dd HEADER_LENGTH
        dd CHECKSUM

; 1. Framebuffer Tag (Grafikmodus-Anforderung: 1024x768 @ 32 BPP)
        align 8
        dw 5                           ; Type = Framebuffer
        dw 0                           ; Flags = 0 (Optional)
        dd 20                          ; Size = 20 Bytes
        dd 1024                        ; Breite in Pixeln
        dd 768                         ; Höhe in Pixeln
        dd 32                          ; Farbtiefe (Bits per Pixel)

; 2. Relocatable Header Tag
        align 8
        dw 10                          ; Type = Relocatable
        dw 0                           ; Flags = 0 (Optional)
        dd 24                          ; Size = 24 Bytes
        dd 0x00100000                  ; min_addr (1 MB Base)
        dd 0xFFFFFFFF                  ; max_addr
        dd 4096                        ; Alignment (4 KB)
        dd 0                           ; Preference (0 = Keine Präferenz)

; 3. End Tag (Schließt den Multiboot2 Header ab)
        align 8
        dw 0                           ; Type = End
        dw 0                           ; Flags = 0
        dd 8                           ; Size = 8 Bytes

multiboot_header_end:
