	[BITS   64]
	section .text

	global switch_context
	global thread_entry_stub
	extern scheduler_release_initial_lock
	extern thread_exit

	; void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp)
	; RDI = &prev->rsp
	; RSI = next->rsp

switch_context:
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	mov [rdi], rsp; save current RSP to RDI
	mov rsp, rsi; set new RSP

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret

	; Expected:
	; R12 = Functionpointer (thread_entry_t)
	; R13 = Argument (void *arg)

thread_entry_stub:
	call scheduler_release_initial_lock

	sti ; activate interrupts for the new thread

	;    System V ABI: First Argument in RDI
	mov  rdi, r13
	call r12

	call thread_exit

.hang:
	hlt
	jmp .hang
