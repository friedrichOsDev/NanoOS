[BITS 32]

section .text
global load_page_directory
global enable_paging
global disable_paging

load_page_directory:
    mov eax, [esp + 4]  ; get the address from the stack
    mov cr3, eax
    ret

enable_paging:
    mov eax, cr0
    or eax, 0x80000000  ; set the paging bit (bit 31)
    mov cr0, eax
    jmp .flush
.flush:
    ret

disable_paging:
    mov eax, cr0
    and eax, 0x7FFFFFFF  ; clear the paging bit (bit 31)
    mov cr0, eax
    jmp .flush_disable
.flush_disable:
    ret
    