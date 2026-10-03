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

; Macro for exporting ISR symbols
        %macro EXPORT_ISR 1
        global isr%1
        %endmacro
        %assign i 0
        %rep 32
        EXPORT_ISR i
        %assign i i+1
        %endrep

; Macro for exporting IRQ symbols
        %macro EXPORT_IRQ 1
        global irq%1
        %endmacro
        %assign i 0
        %rep 16
        EXPORT_IRQ i
        %assign i i+1
        %endrep

; ISR Stubs
        %macro ISR_NOERR 1
isr%1:
        push qword 0                   ; Dummy Error Code
        push qword %1                  ; Interrupt Vector
        jmp common_isr_stub
        %endmacro

        %macro ISR_ERR 1
isr%1:
        push qword %1                  ; Interrupt Vector with HW Error Code
        jmp common_isr_stub
        %endmacro

; IRQ Stub
        %macro IRQ_STUB 1
irq%1:
        push qword 0                   ; Dummy Error Code
        push qword (32 + %1)           ; Mapped Interrupt Vector
        jmp common_irq_stub
        %endmacro

; Vector Definitions
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

        %assign i 0
        %rep 16
        IRQ_STUB i
        %assign i i+1
        %endrep

; Unified Context Saver Macro (Ensures strict 16-byte alignment compliance)
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

common_isr_stub:
        SAVE_CONTEXT
        mov rdi, rsp                   ; Pass struct registers* as 1st arg
        call isr_handler
        RESTORE_CONTEXT
        add rsp, 16                    ; Clean interrupt vector & error code
        iretq

common_irq_stub:
        SAVE_CONTEXT
        mov rdi, rsp
        call irq_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

idt_load:
        lidt [rdi]
        ret

idt_enable:
        sti
        ret

idt_disable:
        cli
        ret

spurious_handler_stub:
        iretq

ipi_reschedule_stub:
        push qword 0
        push qword 0xFD                ; Vector 253
        SAVE_CONTEXT
        mov rdi, rsp
        call reschedule_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

ipi_stop_stub:
        push qword 0
        push qword 0xFC                ; Vector 252
        SAVE_CONTEXT
        mov rdi, rsp
        call stop_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

lapic_timer_stub:
        push qword 0
        push qword 0xFE                ; Vector 254
        SAVE_CONTEXT
        mov rdi, rsp
        call lapic_timer_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq

ipi_tlb_shootdown_stub:
        push qword 0
        push qword 0xFB                ; Vector 251
        SAVE_CONTEXT
        mov rdi, rsp
        call tlb_shootdown_ipi_handler
        RESTORE_CONTEXT
        add rsp, 16
        iretq
