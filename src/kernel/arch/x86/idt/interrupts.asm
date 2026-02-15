[BITS 32]
section .text

; Define external handlers
extern isr_handler
extern irq_handler

; Export global symbols
global idt_load

%macro EXPORT_ISR 1
    global isr%1
%endmacro
%assign i 0
%rep 32
    EXPORT_ISR i
%assign i i+1
%endrep

%macro EXPORT_IRQ 1
    global irq%1
%endmacro
%assign i 0
%rep 16
    EXPORT_IRQ i
%assign i i+1
%endrep


; Interrupt Service Routine
%macro ISR_NOERR 1
isr%1:
    push byte 0
    push byte %1
    jmp common_isr_stub
%endmacro

%macro ISR_ERR 1
isr%1:
    push byte %1
    jmp common_isr_stub
%endmacro

; All 32 ISRs
ISR_NOERR 0  ; Division By Zero
ISR_NOERR 1  ; Debug
ISR_NOERR 2  ; Non Maskable Interrupt
ISR_NOERR 3  ; Breakpoint
ISR_NOERR 4  ; Into Detected Overflow
ISR_NOERR 5  ; Out of Bounds
ISR_NOERR 6  ; Invalid Opcode
ISR_NOERR 7  ; No Coprocessor
ISR_ERR   8  ; Double Fault
ISR_NOERR 9  ; Coprocessor Segment Overrun
ISR_ERR   10 ; Bad TSS
ISR_ERR   11 ; Segment Not Present
ISR_ERR   12 ; Stack Fault
ISR_ERR   13 ; General Protection Fault
ISR_ERR   14 ; Page Fault
ISR_NOERR 15 ; Unknown Interrupt
ISR_NOERR 16 ; x87 FPU Error
ISR_ERR   17 ; Alignment Check
ISR_NOERR 18 ; Machine Check
ISR_NOERR 19 ; SIMD Floating Point
ISR_NOERR 20 ; Virtualization Exception
ISR_NOERR 21 ; Control Protection Exception
%assign i 22
%rep 10
    ISR_NOERR i
%assign i i+1
%endrep

; Interrupt Request Service Routine
%macro IRQ_STUB 1
irq%1:
    push byte 0
    push byte (32 + %1)
    jmp common_irq_stub
%endmacro

; All 16 IRQs
%assign i 0
%rep 16
    IRQ_STUB i
%assign i i+1
%endrep

; Common Stubs
common_isr_stub:
    pusha ; Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    mov ax, ds ; Save DATA segment selector
    push eax

    mov ax, 0x10 ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax ; Restore DATA segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa ; Restore EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    add esp, 8 ; Remove error code and interrupt number from stack
    iret

common_irq_stub:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret
