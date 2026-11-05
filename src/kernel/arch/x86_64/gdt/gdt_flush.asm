; ==============================================================================
; @file gdt_flush.asm
; @brief Assembly routines for flushing the GDT and loading the TSS.
; @author friedrichOsDev
; ==============================================================================

        [BITS 64]
        section .text

        global gdt_flush
        global tss_load

; ==============================================================================
; void gdt_flush(uint64_t gdt_ptr_phys)
; ==============================================================================
; Parameters (System V ABI):
;   RDI - Pointer to the GDTR structure (gdtp)
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
; Parameters (System V ABI):
;   RDI - TSS segment selector (lower 16 bits: DI)
; ==============================================================================
tss_load:
        ltr di
        ret
