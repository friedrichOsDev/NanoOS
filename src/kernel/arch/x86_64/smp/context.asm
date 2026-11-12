; ==============================================================================
; @file context.asm
; @brief Implementation of context switching and thread entry stubs in x86_64 assembly.
; @details Saves and restores callee-saved registers and FPU state during context
; switches, and provides the bootstrap entry stub for new threads.
; @author friedrichOsDev
; ==============================================================================
        [BITS 64]
        section .text

        global switch_context
        global thread_entry_stub
        extern thread_exit

; ==============================================================================
; switch_context
; ==============================================================================
; Performs a context switch between threads by saving the current stack pointer
; and optional FPU state, then loading the next thread's context.
; Signature: void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp, void *prev_fpu_state, void *next_fpu_state)
; Parameters (System V AMD64 ABI):
; RDI = prev_rsp_ptr   (&prev->rsp)
; RSI = next_rsp       (Value of next->rsp)
; RDX = prev_fpu_state (Pointer to previous thread's FPU buffer, or NULL)
; RCX = next_fpu_state (Pointer to next thread's FPU buffer, or NULL)
; ==============================================================================
switch_context:
; 0. Save callee-saved registers onto the current thread's stack
        push rbp
        push rbx
        push r12
        push r13
        push r14
        push r15

; 1. Save current stack pointer reference
        mov [rdi], rsp

; 2. Conditionally save FPU state if buffer is provided
        test rdx, rdx
        jz .skip_save_fpu
        fxsave64 [rdx]

.skip_save_fpu:
; 3. Switch to the new stack pointer
        mov rsp, rsi

; 4. Conditionally restore FPU state if buffer is provided
        test rcx, rcx
        jz .skip_restore_fpu
        fxrstor64 [rcx]

.skip_restore_fpu:
; 5. Restore callee-saved registers from the new thread's stack
        pop r15
        pop r14
        pop r13
        pop r12
        pop rbx
        pop rbp

        ret

; ==============================================================================
; thread_entry_stub
; ==============================================================================
; Entry point executed when a newly created thread starts execution.
; Expected Register Setup upon Startup:
; R12 = Function pointer to the thread's main routine (thread_entry_t)
; R13 = Argument passed to the function (void *arg)
; ==============================================================================
thread_entry_stub:
; 0. Enable interrupts for the running thread. The scheduler lock has
;    already been released by sched_schedule(); never release it here.
        sti

; 1. Pass argument in RDI and invoke the thread routine
        mov rdi, r13
        call r12

; 2. Clean up thread gracefully if the main function returns
        call thread_exit

.hang:
        hlt                            ; Halt CPU execution
        jmp .hang
