; ==============================================================================
; @file smp_trampoline.asm
; @brief Real-mode trampoline code for waking up Application Processors (APs) in SMP mode.
; @details Performs the step-by-step transition: 16-bit Real Mode -> 32-bit Protected Mode -> 64-bit Long Mode.
; @author friedrichOsDev
; ==============================================================================

        [BITS 16]
        section .text

        global smp_trampoline_start
        global smp_trampoline_end
        global smp_trampoline_pml4
        global smp_trampoline_stack    ; MUST be 16-byte aligned
        global smp_trampoline_entry

        %define TRAMPOLINE_BASE 0x8000
        %define REL_ADDR(x) (TRAMPOLINE_BASE + ((x) - smp_trampoline_start))

; ==============================================================================
; 16-Bit Real Mode Entry Point (Initiated via SIPI)
; ==============================================================================
smp_trampoline_start:
        cli                            ; Disable interrupts
        cld                            ; Clear direction flag for string operations

; 0. Set up segment registers and temporary stack in Real Mode
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, REL_ADDR(smp_trampoline_stack_tmp)

; 1. Transition to 32-bit Protected Mode
        lgdt [REL_ADDR(ap_gdt64_ptr)]  ; Load temporary GDT

        mov eax, cr0
        or eax, 1                      ; Set Protected Mode Enable (PE) bit
        mov cr0, eax

        jmp 0x08:REL_ADDR(ap_protected_mode_entry) ; Far jump to flush instruction pipeline

; ==============================================================================
; 32-Bit Protected Mode
; ==============================================================================
        [BITS 32]

ap_protected_mode_entry:
        mov ax, 0x10                   ; Load data segment selector (0x10)
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; 2. Control Registers & Long Mode Activation
; Enable PAE (Physical Address Extension) in CR4
        mov eax, cr4
        or eax, (1 << 5)
        mov cr4, eax

; Load PML4 address (CR3)
        mov eax, [REL_ADDR(smp_trampoline_pml4)]
        mov cr3, eax

; Enable Long Mode (LME) and No-Execute (NX) in EFER MSR
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8) | (1 << 11)
        wrmsr

; Enable Paging (CR0.PG = 1, CR0.WP = 1)
        mov eax, cr0
        or eax, (1 << 31) | (1 << 16)
        mov cr0, eax

; Far jump into 64-bit Long Mode
        jmp 0x18:REL_ADDR(ap_long_mode_entry)

; ==============================================================================
; 64-Bit Long Mode
; ==============================================================================
        [BITS 64]

ap_long_mode_entry:
; 3. Setup 64-bit data segments & FPU/SSE initialization
        mov ax, 0x20                   ; 64-bit data segment selector (0x20)
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; Enable FPU & SSE hardware support
        mov rax, cr0
        and rax, ~((1 << 2) | (1 << 3)) ; Clear EM & TS bits
        or rax, (1 << 1)               ; Set MP bit
        mov cr0, rax

        mov rax, cr4
        or rax, (1 << 9) | (1 << 10)   ; Set OSFXSR & OSXMMEXCPT bits
        mov cr4, rax

        fninit                         ; Initialize FPU

        sub rsp, 16
        mov dword [rsp], 0x1F80        ; Default MXCSR value (masked exceptions)
        ldmxcsr [rsp]
        add rsp, 16

; 4. Load target stack and jump to kernel entry function
        mov rax, [rel smp_trampoline_stack]
        mov rsp, rax

        and rsp, -16                   ; Guarantee 16-byte stack alignment
        sub rsp, 8                     ; sub 8 Bytes abziehen (simulate return address for ´call´)

        mov rax, [rel smp_trampoline_entry]
        jmp rax                        ; Sprung in die C-Funktion smp_ap_main

; ==============================================================================
; Data Structures & Buffers
; ==============================================================================
        align 16
ap_gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null descriptor
        dq 0x00CF9A000000FFFF          ; 0x08: 32-bit code segment
        dq 0x00CF92000000FFFF          ; 0x10: 32-bit data segment
        dq 0x00209A0000000000          ; 0x18: 64-bit code segment
        dq 0x0000920000000000          ; 0x20: 64-bit data segment
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
smp_trampoline_stack_tmp:              ; Temporary 16-bit stack

smp_trampoline_end:
