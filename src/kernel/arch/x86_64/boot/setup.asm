        [BITS 32]
        section .setup

        global _setup
        global multiboot_info_ptr
        global multiboot_magic
        extern _entry

        KERNEL_VIRT_BASE equ 0xFFFFFFFF80000000
        PML4_INDEX equ (KERNEL_VIRT_BASE >> 39) & 0x1FF ; 511
        PDPT_INDEX equ (KERNEL_VIRT_BASE >> 30) & 0x1FF ; 510

_setup:
        cli
        cld

; 0. Save Multiboot Parameters & HW Checks
        mov [multiboot_info_ptr], ebx
        mov [multiboot_magic], eax

        call check_cpuid
        test eax, eax
        jz .no_long_mode

        call check_long_mode_support
        test eax, eax
        jz .no_long_mode

; 1. Setup Page Tables
        mov eax, boot_pdpt
        or eax, 0x3
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
        mov eax, 0x83

.fill_kernel_low_pages:
        mov [edi], eax
        mov dword [edi + 4], 0
        add edi, 8
        add eax, 0x200000
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

; 2. Control Registers & Long Mode Enable
; Activate PAE
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

; Load CR3
        mov eax, boot_pml4
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

; Load 64-Bit GDT
        lgdt [gdt64_ptr]

; Far Jump to Long Mode
        jmp 0x08:init_long_mode

.no_long_mode:
        cli
.hang:
        hlt
        jmp .hang

check_cpuid:
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 1 << 21
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
        test edx, 1 << 29
        jz .no_lm
        mov eax, 1
        ret
.no_lm:
        xor eax, eax
        ret

        [BITS 64]

init_long_mode:
; 3. Long Mode Data Segments & FPU/SSE Init
        mov ax, 0x10
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

; 4. Jump to Kernel Entry
        and rsp, -16
        mov rax, _entry
        jmp rax

        align 8
gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null
        dq 0x00209A0000000000          ; 0x18: 64-Bit Code
        dq 0x0000920000000000          ; 0x20: 64-Bit Data
gdt64_end:

gdt64_ptr:
        dw gdt64_end - gdt64_start - 1
        dq gdt64_start

        align 8
multiboot_info_ptr:
        dq 0
multiboot_magic:
        dq 0

        align 4096
        boot_pml4: times 4096 db 0
        boot_pdpt: times 4096 db 0
        boot_pdpt_direct: times 4096 db 0
        boot_pd_low: times 4096 db 0
        boot_pd_high: times 4096 * 4 db 0
