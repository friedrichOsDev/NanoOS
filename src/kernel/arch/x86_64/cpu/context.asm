        [BITS 64]
        section .text

        global switch_context
        global thread_entry_stub
        extern scheduler_release_initial_lock
        extern thread_exit

; void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp,
; void *prev_fpu_state, void *next_fpu_state)
; System V ABI:
; RDI = prev_rsp_ptr   (&prev->rsp)
; RSI = next_rsp       (next->rsp value)
; RDX = prev_fpu_state (prev->fpu_state pointer, may be NULL)
; RCX = next_fpu_state (next->fpu_state pointer, may be NULL)

switch_context:
        push rbp
        push rbx
        push r12
        push r13
        push r14
        push r15

; Save current RSP into prev->rsp
        mov [rdi], rsp

; Save prev FPU/SSE state (RDX = prev_fpu_state)
        test rdx, rdx
        jz .skip_save_fpu
        fxsave64 [rdx]

.skip_save_fpu:
; Switch to next stack (RSI = next_rsp value)
        mov rsp, rsi

; Restore next FPU/SSE state (RCX = next_fpu_state)
        test rcx, rcx
        jz .skip_restore_fpu
        fxrstor64 [rcx]

.skip_restore_fpu:
        pop r15
        pop r14
        pop r13
        pop r12
        pop rbx
        pop rbp

        ret

; Expected:
; R12 = Functionpointer (thread_entry_t)
; R13 = Argument (void *arg)

thread_entry_stub:
        call scheduler_release_initial_lock

        sti                            ; activate interrupts for the new thread

; System V ABI: First Argument in RDI
        mov rdi, r13
        call r12

        call thread_exit

.hang:
        hlt
        jmp .hang
