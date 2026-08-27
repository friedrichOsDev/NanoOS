	[BITS   64]
	section .text

	extern isr_handler
	extern irq_handler

	global idt_load
	global idt_enable
	global idt_disable
	global spurious_handler_stub

	;       Macro for exporting ISR symbols
	%macro  EXPORT_ISR 1
	global  isr%1
	%endmacro
	%assign i 0
	%rep    32
	EXPORT_ISR i
	%assign i i+1
	%endrep

	;       Macro for exporting IRQ symbols
	%macro  EXPORT_IRQ 1
	global  irq%1
	%endmacro
	%assign i 0
	%rep    16
	EXPORT_IRQ i
	%assign i i+1
	%endrep

	;      ISR stubs
	%macro ISR_NOERR 1

isr%1:
	push 0; Dummy Error Code
	push %1; Interrupt Nummer
	jmp  common_isr_stub
%endmacro

%macro ISR_ERR 1

isr%1:
	push %1; Interrupt Nummer
	jmp  common_isr_stub
%endmacro

	;      IRQ stub
	%macro IRQ_STUB 1

irq%1:
	push byte 0
	push byte (32 + %1)
	jmp  common_irq_stub
%endmacro

	;       All 32 ISRs
	ISR_NOERR 0
	ISR_NOERR 1
	ISR_NOERR 2
	ISR_NOERR 3
	ISR_NOERR 4
	ISR_NOERR 5
	ISR_NOERR 6
	ISR_NOERR 7
	ISR_ERR   8  ; Double Fault
	ISR_NOERR 9
	ISR_ERR   10
	ISR_ERR   11
	ISR_ERR   12
	ISR_ERR   13 ; General Protection Fault
	ISR_ERR   14 ; Page Fault
	ISR_NOERR 15
	ISR_NOERR 16
	ISR_ERR   17
	ISR_NOERR 18
	ISR_NOERR 19
	ISR_NOERR 20
	ISR_NOERR 21
	%assign i 22
	%rep    10
	ISR_NOERR i
	%assign i i+1
	%endrep

	;       All 16 IRQs
	%assign i 0
	%rep    16
	IRQ_STUB i
	%assign i i+1
	%endrep

common_isr_stub:
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
	mov  rdi, rsp
	mov  rbp, rsp
	and  rsp, ~0xF

	call isr_handler

	mov rsp, rbp
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

	add rsp, 16

	iretq

common_irq_stub:
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

	mov rdi, rsp
	mov rbp, rsp
	and rsp, ~0xF

	call irq_handler

	mov rsp, rbp
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
