[BITS 32]

section .text
global cpu_hlt
global cpu_pause

cpu_hlt:
    hlt
    ret

cpu_pause:
    pause
    ret