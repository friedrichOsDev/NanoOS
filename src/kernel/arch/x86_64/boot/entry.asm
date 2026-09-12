        [BITS 64]
        section .text.entry

        global _entry
        global stack_bottom
        global stack_top

        extern kernel_init
        extern multiboot_magic
        extern multiboot_info_ptr
        extern sbss
        extern ebss

_entry:
; 0. Setup Stack Pointer & Reset Frame Pointer
        mov rsp, stack_top
        xor rbp, rbp                   ; Null-Framepointer für Stack Tracing

; 1. Clear BSS Section (Quadword aligned + Bytes)
        mov rdi, sbss
        mov rcx, ebss
        sub rcx, rdi
        jz .bss_done

        mov rdx, rcx
        shr rcx, 3
        xor rax, rax
        rep stosq

        mov rcx, rdx
        and rcx, 7
        rep stosb

.bss_done:
; 2. System V ABI 16-Byte Stack Alignment for Call
        and rsp, -16                   ; Ensure 16-byte alignment

; 3. Jump to Kernel Main Entry
; void kernel_init(uint64_t magic, uint64_t info_ptr)
        mov rdi, [rel multiboot_magic]
        mov rsi, [rel multiboot_info_ptr]
        call kernel_init

.hang:
        cli
.loop:
        hlt
        jmp .loop

        section .bss
        align 16

stack_bottom:
        resb 16384                     ; 16 KB Stack Space
stack_top:
