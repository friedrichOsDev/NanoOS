        [BITS 16]
        section .text

        global smp_trampoline_start
        global smp_trampoline_end
        global smp_trampoline_pml4
        global smp_trampoline_stack    ; MUST be 16 byte aligend
        global smp_trampoline_entry

        %define TRAMPOLINE_BASE 0x8000
        %define REL_ADDR(x) (TRAMPOLINE_BASE + ((x) - smp_trampoline_start))

smp_trampoline_start:
        cli
        cld

; 0. Setup Real Mode Segments & Temporary Stack
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, REL_ADDR(smp_trampoline_stack_tmp)

; 1. Protected Mode Transition
        lgdt [REL_ADDR(ap_gdt64_ptr)]

        mov eax, cr0
        or eax, 1
        mov cr0, eax

        jmp 0x08:REL_ADDR(ap_protected_mode_entry)

        [BITS 32]

ap_protected_mode_entry:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; 2. Control Registers & Long Mode Enable
; Activate PAE
        mov eax, cr4
        or eax, (1 << 5)
        mov cr4, eax

; Load CR3
        mov eax, [REL_ADDR(smp_trampoline_pml4)]
        mov cr3, eax

; Activate Long Mode & NX-Bit in EFER MSR
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8) | (1 << 11)
        wrmsr

; Enable Paging
        mov eax, cr0
        or eax, (1 << 31) | (1 << 16)
        mov cr0, eax

; Far Jump to Long Mode
        jmp 0x18:REL_ADDR(ap_long_mode_entry)

        [BITS 64]

ap_long_mode_entry:
; 3. Long Mode Data Segments & FPU/SSE Init
        mov ax, 0x20
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; FPU / SSE Hardware Init
        mov rax, cr0
        and rax, ~((1 << 2) | (1 << 3))
        or rax, (1 << 1)
        mov cr0, rax

        mov rax, cr4
        or rax, (1 << 9) | (1 << 10)
        mov cr4, rax

        fninit

        sub rsp, 16
        mov dword [rsp], 0x1F80
        ldmxcsr [rsp]
        add rsp, 16

; 4. Load Core Stack & Jump to Kernel Entry
        mov rax, [rel smp_trampoline_stack]
        mov rsp, rax

        mov rax, [rel smp_trampoline_entry]
        jmp rax

        align 16
ap_gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null
        dq 0x00CF9A000000FFFF          ; 0x08: 32-Bit Code
        dq 0x00CF92000000FFFF          ; 0x10: 32-Bit Data
        dq 0x00209A0000000000          ; 0x18: 64-Bit Code
        dq 0x0000920000000000          ; 0x20: 64-Bit Data
ap_gdt64_end:

        align 4
ap_gdt64_ptr:
        dw ap_gdt64_end - ap_gdt64_start - 1
        dd REL_ADDR(ap_gdt64_start)

        align 8
smp_trampoline_pml4:
        dq 0
smp_trampoline_stack:
        dq 0
smp_trampoline_entry:
        dq 0

        align 16
        times 256 db 0
smp_trampoline_stack_tmp:

smp_trampoline_end:
