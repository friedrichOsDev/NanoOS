        [BITS 64]
        section .text

        global gdt_flush
        global tss_load

; void gdt_flush(uint64_t gdt_ptr_phys)
; RDI = Pointer to GDTR structure (gdtp)
gdt_flush:
        lgdt [rdi]

; Far Jump to reload CS (Code Segment = 0x08)
        push 0x08
        lea rax, [rel .reload_cs]
        push rax
        retfq

.reload_cs:
; Reload all Data Segment Registers (Kernel Data = 0x10)
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        ret

; void tss_load(uint16_t selector)
; RDI = TSS Segment Selector
tss_load:
        ltr di
        ret
