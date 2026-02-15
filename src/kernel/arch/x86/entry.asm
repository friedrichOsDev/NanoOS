[BITS 32]

section .text.entry
global _start
extern kernel_main

_start:
    mov esp, stack_top

    push ebx ; multiboot info pointer
    push eax ; multiboot magic

    extern sbss
    extern ebss

    mov edi, sbss
    mov ecx, ebss
    sub ecx, edi
    xor eax, eax
    rep stosb

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
