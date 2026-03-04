[BITS 32]

section .text.entry
global _start
extern kernel_main

_start:
    mov esi, eax
    mov ebp, ebx

    extern sbss
    extern ebss

    mov edi, sbss
    mov ecx, ebss
    sub ecx, edi
    xor eax, eax
    rep stosb

    mov esp, stack_top

    push ebp ; multiboot info pointer
    push esi ; multiboot magic 

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16

stack_bottom:
    resb 16384
stack_top:
