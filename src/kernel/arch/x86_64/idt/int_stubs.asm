; ==============================================================================
; @file int_stubs.asm
; @brief Assembly stubs for IDT loading, ISRs, IRQs, and IPI handlers.
; @author friedrichOsDev
; ==============================================================================

        [BITS 64]
        section .text

        extern isr_handler
        extern irq_handler
        extern reschedule_ipi_handler
        extern stop_ipi_handler
        extern lapic_timer_handler
        extern tlb_shootdown_ipi_handler

        global idt_load
        global idt_enable
        global idt_disable
        global spurious_handler_stub
        global ipi_reschedule_stub
        global ipi_stop_stub
        global lapic_timer_stub
        global ipi_tlb_shootdown_stub

; Macro to export ISR symbols
        %macro EXPORT_ISR 1
        global isr%1
        %endmacro
        %assign i 0
        %rep 32
        EXPORT_ISR i
        %assign i i+1
        %endrep

; Macro to export IRQ symbols
        %macro EXPORT_IRQ 1
        global irq%1
        %endmacro
        %assign i 0
        %rep 16
        EXPORT_IRQ i
        %assign i i+1
        %endrep

; ISR stub without an error code (pushes a dummy 0 onto the stack)
        %macro ISR_NOERR 1
isr%1:
        push qword 0                   ; Dummy error code
        push qword %1                  ; Interrupt vector
        jmp common_isr_stub
        %endmacro

; ISR stub with an error code (hardware already pushes an error code onto the stack)
        %macro ISR_ERR 1
isr%1:
        push qword %1                  ; Interrupt vector with hardware error code
        jmp common_isr_stub
        %endmacro

; IRQ stub (remapped starting from vector 32)
        %macro IRQ_STUB 1
irq%1:
        push qword 0                   ; Dummy error code
        push qword (32 + %1)           ; Remapped interrupt vector
        jmp common_irq_stub
        %endmacro

; Vector definitions (ISRs 0 - 31)
        ISR_NOERR 0
        ISR_NOERR 1
        ISR_NOERR 2
        ISR_NOERR 3
        ISR_NOERR 4
        ISR_NOERR 5
        ISR_NOERR 6
        ISR_NOERR 7
        ISR_ERR 8                      ; Double Fault
        ISR_NOERR 9
        ISR_ERR 10
        ISR_ERR 11
        ISR_ERR 12
        ISR_ERR 13                     ; General Protection Fault
        ISR_ERR 14                     ; Page Fault
        ISR_NOERR 15
        ISR_NOERR 16
        ISR_ERR 17
        ISR_NOERR 18
        ISR_NOERR 19
        ISR_NOERR 20
        ISR_NOERR 21
        %assign i 22
        %rep 10
        ISR_NOERR i
        %assign i i+1
        %endrep

; IRQ definitions (IRQs 0 - 15)
        %assign i 0
        %rep 16
        IRQ_STUB i
        %assign i i+1
        %endrep

; Saves the entire register context (maintaining 16-byte stack alignment)
        %macro SAVE_CONTEXT 0
        push rax
        push rbx
        push rcx
        push rdx
        push rsi
        push rdi
        push rbp
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        %endmacro

; Calls a C handler with the System V AMD64 stack alignment guaranteed.
; RBX is safe as a temporary because its interrupted value is already saved
; in the register frame and the C ABI requires callees to preserve RBX.
        %macro CALL_ALIGNED 1
        mov rbx, rsp                   ; Preserve the exact register-frame address
        mov rdi, rsp                   ; First argument: struct registers*
        and rsp, -16                   ; Align stack before CALL (callee enters at 8 mod 16)
        call %1
        mov rsp, rbx                   ; Restore the unmodified interrupt frame
        %endmacro

; Restores the saved register context
        %macro RESTORE_CONTEXT 0
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rbp
        pop rdi
        pop rsi
        pop rdx
        pop rcx
        pop rbx
        pop rax
        %endmacro

; ==============================================================================
; Common handler for CPU exceptions (ISRs)
; ==============================================================================
common_isr_stub:
        SAVE_CONTEXT
        CALL_ALIGNED isr_handler
        RESTORE_CONTEXT
        add rsp, 16                    ; Remove vector and error code from stack
        iretq

; ==============================================================================
; Common handler for hardware interrupts (IRQs)
; ==============================================================================
common_irq_stub:
        SAVE_CONTEXT
        CALL_ALIGNED irq_handler
        RESTORE_CONTEXT
        add rsp, 16                    ; Remove vector and error code from stack
        iretq

; ==============================================================================
; IDT control functions
; ==============================================================================

; void idt_load(uint64_t idt_ptr) -> RDI = Pointer to IDTR
idt_load:
        lidt [rdi]
        ret

; Stub for spurious interrupts (ignores the interrupt)
spurious_handler_stub:
        iretq

; ==============================================================================
; APIC & Inter-Processor Interrupt (IPI) stubs
; ==============================================================================

; Reschedule IPI stub (Vector 253 / 0xFD)
ipi_reschedule_stub:
        push qword 0
        push qword 0xFD                ; Vector 253
        SAVE_CONTEXT
        CALL_ALIGNED reschedule_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

; CPU-Stop IPI stub (Vector 252 / 0xFC)
ipi_stop_stub:
        push qword 0
        push qword 0xFC                ; Vector 252
        SAVE_CONTEXT
        CALL_ALIGNED stop_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

; Local APIC timer stub (Vector 254 / 0xFE)
lapic_timer_stub:
        push qword 0
        push qword 0xFE                ; Vector 254
        SAVE_CONTEXT
        CALL_ALIGNED lapic_timer_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

; TLB-Shootdown IPI stub (Vector 251 / 0xFB)
ipi_tlb_shootdown_stub:
        push qword 0
        push qword 0xFB                ; Vector 251
        SAVE_CONTEXT
        CALL_ALIGNED tlb_shootdown_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq
