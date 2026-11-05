; ==============================================================================
; @file setup.asm
; @brief 32-bit entry point (bootloader to long mode transition).
; @details Saves multiboot parameters, checks CPUID/long mode support, builds
; initial page tables, enables paging & long mode, and initializes FPU/SSE.
; @author friedrichOsDev
; ==============================================================================

        [BITS 32]
        section .setup

        global _setup
        global multiboot_info_ptr
        global multiboot_magic
        extern _entry

        KERNEL_VIRT_BASE equ 0xFFFFFFFF80000000
        PML4_INDEX equ (KERNEL_VIRT_BASE >> 39) & 0x1FF ; 511
        PDPT_INDEX equ (KERNEL_VIRT_BASE >> 30) & 0x1FF ; 510

; ==============================================================================
; _setup
; ==============================================================================
; Entry point from the bootloader (32-bit Protected Mode).
; ==============================================================================
_setup:
        cli                            ; Disable interrupts
        cld                            ; Clear direction flag

; 0. Save multiboot parameters & check hardware support
        mov [multiboot_info_ptr], ebx  ; Multiboot info structure pointer
        mov [multiboot_magic], eax     ; Multiboot magic number

        call check_cpuid
        test eax, eax
        jz .no_long_mode

        call check_long_mode_support
        test eax, eax
        jz .no_long_mode

; 1. Set up initial paging (Identity mapping & high-half kernel mapping)
        mov eax, boot_pdpt
        or eax, 0x3                    ; Present + Writable
        mov [boot_pml4], eax
        mov dword [boot_pml4 + 4], 0
        mov [boot_pml4 + PML4_INDEX * 8], eax
        mov dword [boot_pml4 + PML4_INDEX * 8 + 4], 0

        mov eax, boot_pdpt_direct
        or eax, 0x3
        mov [boot_pml4 + 256 * 8], eax
        mov dword [boot_pml4 + 256 * 8 + 4], 0

        mov eax, boot_pd_low
        or eax, 0x3
        mov [boot_pdpt], eax
        mov dword [boot_pdpt + 4], 0
        mov [boot_pdpt + PDPT_INDEX * 8], eax
        mov dword [boot_pdpt + PDPT_INDEX * 8 + 4], 0

        mov edi, boot_pdpt_direct
        mov ecx, 4
        mov eax, boot_pd_high
        or eax, 0x3

.link_direct_pds:
        mov [edi], eax
        mov dword [edi + 4], 0
        add edi, 8
        add eax, 4096
        loop .link_direct_pds

        mov edi, boot_pd_low
        mov ecx, 8
        mov eax, 0x83                  ; Present + Writable + Huge Page (2 MB)

.fill_kernel_low_pages:
        mov [edi], eax
        mov dword [edi + 4], 0
        add edi, 8
        add eax, 0x200000              ; 2 MB steps
        loop .fill_kernel_low_pages

        mov edi, boot_pd_high
        mov ecx, 2048
        mov eax, 0x83
        mov edx, 0

.fill_direct_mapping:
        mov [edi], eax
        mov [edi + 4], edx
        add edi, 8
        add eax, 0x200000
        adc edx, 0
        loop .fill_direct_mapping

; 2. Control registers & Long Mode activation
; Enable PAE (Physical Address Extension) in CR4
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

; Load PML4 table base into CR3
        mov eax, boot_pml4
        mov cr3, eax

; Enable Long Mode (LME) & NX bit in EFER MSR
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8) | (1 << 11)
        wrmsr

; Enable paging (CR0.PG = 1, CR0.WP = 1)
        mov eax, cr0
        or eax, (1 << 31) | (1 << 16)
        mov cr0, eax

; Load 64-bit GDT
        lgdt [gdt64_ptr]

; Far jump into 64-bit Long Mode
        jmp 0x08:init_long_mode

.no_long_mode:
        cli
.hang:
        hlt                            ; Halt execution if long mode is not supported
        jmp .hang

; ==============================================================================
; Check functions for CPUID and Long Mode support
; ==============================================================================
check_cpuid:
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 1 << 21               ; Toggle ID bit in EFLAGS
        push eax
        popfd
        pushfd
        pop eax
        push ecx
        popfd
        xor eax, ecx
        jz .not_supported
        mov eax, 1
        ret
.not_supported:
        xor eax, eax
        ret

check_long_mode_support:
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .no_lm

        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29              ; Check Long Mode bit in EDX
        jz .no_lm
        mov eax, 1
        ret
.no_lm:
        xor eax, eax
        ret

; ==============================================================================
; 64-bit Long Mode Initialization
; ==============================================================================
        [BITS 64]

init_long_mode:
; 3. Setup 64-bit data segments & FPU/SSE hardware initialization
        mov ax, 0x10                   ; 64-bit data segment selector (0x10)
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; Enable FPU / SSE (CR0 & CR4)
        mov rax, cr0
        and rax, ~((1 << 2) | (1 << 3)) ; Clear EM & TS
        or rax, (1 << 1)               ; Set MP
        mov cr0, rax

        mov rax, cr4
        or rax, (1 << 9) | (1 << 10)   ; Set OSFXSR & OSXMMEXCPT
        mov cr4, rax

        fninit                         ; Reset FPU

        sub rsp, 16
        mov dword [rsp], 0x1F80        ; Load standard MXCSR value
        ldmxcsr [rsp]
        add rsp, 16

; 4. Jump to kernel entry function _entry
        and rsp, -16                   ; Guarantee 16-byte stack alignment
        mov rax, _entry
        jmp rax

; ==============================================================================
; GDT & Memory Tables
; ==============================================================================
        align 8
gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null descriptor
        dq 0x00209A0000000000          ; 0x08: 64-bit code segment
        dq 0x0000920000000000          ; 0x10: 64-bit data segment
gdt64_end:

gdt64_ptr:
        dw gdt64_end - gdt64_start - 1
        dq gdt64_start

        align 8
multiboot_info_ptr:
        dq 0
multiboot_magic:
        dq 0

; Paging buffers for initialization (4 KB aligned)
        align 4096
        boot_pml4: times 4096 db 0
        boot_pdpt: times 4096 db 0
        boot_pdpt_direct: times 4096 db 0
        boot_pd_low: times 4096 db 0
        boot_pd_high: times 4096 * 4 db 0
