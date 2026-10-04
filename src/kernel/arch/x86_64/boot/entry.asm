; /**
; * @file entry.asm
; * @brief Kernel-Einstiegspunkt im 64-Bit Long Mode.
; * @details Bereinigt die BSS-Sektion, richtet den Stack für die System V ABI aus und ruft kernel_init() auf.
; * @author friedrichOsDev
; */

        [BITS 64]
        section .text.entry

        global _entry
        global stack_bottom
        global stack_top

        extern kernel_init
        extern multiboot_magic
        extern multiboot_info_ptr
        extern sbss
        extern ebss

; ==============================================================================
; _entry
; ==============================================================================
; Einstiegspunkt nach der Initialisierung des 64-Bit-Modus durch setup.asm.
; ==============================================================================
_entry:
; 0. Stack-Pointer aufsetzen & Frame-Pointer zurücksetzen
        mov rsp, stack_top
        xor rbp, rbp                   ; Null-Framepointer für Stack-Tracing

; 1. BSS-Sektion löschen (Quadword-aligned + verbleibende Bytes)
        mov rdi, sbss
        mov rcx, ebss
        sub rcx, rdi
        jz .bss_done

        mov rdx, rcx
        shr rcx, 3                     ; Anzahl der 8-Byte-Blöcke
        xor rax, rax
        rep stosq                      ; Mit 0 füllen (64-Bit Blöcke)

        mov rcx, rdx
        and rcx, 7                     ; Verbleibende Rest-Bytes (0-7)
        rep stosb                      ; Restliche Bytes nacheinander mit 0 füllen

.bss_done:
; 2. System V ABI 16-Byte Stack-Alignment für Funktionsaufruf sicherstellen
        and rsp, -16                   ; Stack auf 16-Byte-Grenze ausrichten

; 3. Sprung in die Kernel-Hauptfunktion
; Signature: void kernel_init(uint64_t magic, uint64_t info_ptr)
        mov rdi, [rel multiboot_magic] ; 1. Argument (RDI): Multiboot2 Magic
        mov rsi, [rel multiboot_info_ptr] ; 2. Argument (RSI): Multiboot2 Info-Pointer
        call kernel_init

.hang:
        cli                            ; Interrupts deaktivieren, falls kernel_init zurückkehrt
.loop:
        hlt                            ; CPU anhalten
        jmp .loop

; ==============================================================================
; Puffer & Speicherbereich für den Initial-Stack
; ==============================================================================
        section .bss
        align 16

stack_bottom:
        resb 16384                     ; 16 KB Kernel-Stack-Speicher
stack_top:
