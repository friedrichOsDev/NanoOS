; /**
; * @file gdt_flush.asm
; * @brief Assembler-Funktionen zum Flushen der GDT und Laden des TSS.
; * @author friedrichOsDev
; */
        [BITS 64]
        section .text

        global gdt_flush
        global tss_load

; ==============================================================================
; void gdt_flush(uint64_t gdt_ptr_phys)
; ==============================================================================
; Parameter (System V ABI):
; RDI = Zeiger auf die GDTR-Struktur (gdtp)
; ==============================================================================
gdt_flush:
        lgdt [rdi]

        push 0x08
        lea rax, [rel .reload_cs]
        push rax
        retfq

.reload_cs:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        ret

; ==============================================================================
; void tss_load(uint16_t selector)
; ==============================================================================
; Parameter (System V ABI):
; RDI = TSS-Segment-Selektor (untere 16 Bit: DI)
; ==============================================================================
tss_load:
        ltr di
        ret
