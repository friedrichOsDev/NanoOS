; ==============================================================================
; @file entry.asm
; @brief Kernel entry point in 64-bit Long Mode.
; @details Clears the BSS section, aligns the stack for the System V AMD64 ABI,
; and invokes kernel_init().
; @author friedrichOsDev
; ==============================================================================

        [BITS 64]
        section .text.entry

        global _entry
        global stack_bottom
        global kernel_stack_top

        extern kernel_init
        extern multiboot_magic
        extern multiboot_info_ptr
        extern sbss
        extern ebss

; ==============================================================================
; _entry
; ==============================================================================
; Entry point executed after switching to 64-bit mode via setup.asm.
; ==============================================================================
_entry:
; 0. Set up stack pointer and clear frame pointer
        mov rsp, kernel_stack_top
        xor rbp, rbp                   ; Zero out frame pointer for stack tracing

; 1. Clear BSS section (quadword-aligned blocks + remaining bytes)
        mov rdi, sbss
        mov rcx, ebss
        sub rcx, rdi
        jz .bss_done

        mov rdx, rcx
        shr rcx, 3                     ; Number of 8-byte blocks
        xor rax, rax
        rep stosq                      ; Fill with 0 (64-bit blocks)

        mov rcx, rdx
        and rcx, 7                     ; Remaining leftover bytes (0-7)
        rep stosb                      ; Fill remaining bytes sequentially with 0

.bss_done:
; 2. Ensure System V ABI 16-byte stack alignment for function calls
        and rsp, -16                   ; Align stack to a 16-byte boundary

; 3. Jump to the main kernel initialization function
; Signature: void kernel_init(uint64_t magic, uint64_t info_ptr)
        mov rdi, [rel multiboot_magic] ; 1st argument (RDI): Multiboot2 magic value
        mov rsi, [rel multiboot_info_ptr] ; 2nd argument (RSI): Multiboot2 info pointer
        call kernel_init

.hang:
        cli                            ; Disable interrupts in case kernel_init returns
.loop:
        hlt                            ; Halt CPU execution
        jmp .loop

; ==============================================================================
; Buffer and memory region allocated for the initial stack
; ==============================================================================
        section .bss
        align 16

kernel_stack_bottom:
        resb 16384                     ; 16 KB kernel stack storage
kernel_stack_top:
