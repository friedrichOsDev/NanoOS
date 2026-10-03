; /**
;  * @file context.asm
;  * @brief Implementierung des Kontextwechsels und des Thread-Einstiegs-Stubs in x86_64 Assembler.
;  * @author friedrichOsDev
;  */
        [BITS 64]
        section .text

        global switch_context
        global thread_entry_stub
        extern scheduler_release_initial_lock
        extern thread_exit

; ==============================================================================
; void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp,
; void *prev_fpu_state, void *next_fpu_state)
; ==============================================================================
; Parameter (System V ABI):
; RDI = prev_rsp_ptr   (&prev->rsp)
; RSI = next_rsp       (Wert von next->rsp)
; RDX = prev_fpu_state (Zeiger auf FPU-Puffer des vorherigen Threads, ggf. NULL)
; RCX = next_fpu_state (Zeiger auf FPU-Puffer des nächsten Threads, ggf. NULL)
; ==============================================================================
switch_context:
        push rbp
        push rbx
        push r12
        push r13
        push r14
        push r15

        mov [rdi], rsp

        test rdx, rdx
        jz .skip_save_fpu
        fxsave64 [rdx]

.skip_save_fpu:
        mov rsp, rsi

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

; ==============================================================================
; thread_entry_stub
; ==============================================================================
; Einstiegspunkt für neu initialisierte Threads.

; Erwartete Registerbelegung beim Start:
; R12 = Funktionszeiger der Thread-Hauptfunktion (thread_entry_t)
; R13 = Argument für die Funktion (void *arg)
; ==============================================================================
thread_entry_stub:
        call scheduler_release_initial_lock

        sti

        mov rdi, r13
        call r12

        call thread_exit

.hang:
        hlt
        jmp .hang
