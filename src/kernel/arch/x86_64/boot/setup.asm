; /**
; * @file setup.asm
; * @brief 32-Bit Einstiegspunkt (Bootloader -> Long Mode Übergang).
; * @details Speichert Multiboot-Parameter, prüft CPUID/Long-Mode, baut Init-Page-Tables auf, aktiviert Paging & Long Mode und initialisiert FPU/SSE.
; * @author friedrichOsDev
; */

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
; Einstiegspunkt vom Bootloader (32-Bit Protected Mode).
; ==============================================================================
_setup:
        cli                            ; Interrupts deaktivieren
        cld                            ; Direction Flag zurücksetzen

; 0. Multiboot-Parameter sichern & Hardware-Unterstützung prüfen
        mov [multiboot_info_ptr], ebx  ; Multiboot Info-Struktur Pointer
        mov [multiboot_magic], eax     ; Multiboot Magic Number

        call check_cpuid
        test eax, eax
        jz .no_long_mode

        call check_long_mode_support
        test eax, eax
        jz .no_long_mode

; 1. Initiales Paging aufbauen (Identity-Mapping & High-Half-Kernel-Mapping)
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
        add eax, 0x200000              ; 2 MB Schritte
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

; 2. Steuerregister & Long-Mode Aktivierung
; PAE (Physical Address Extension) in CR4 aktivieren
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

; Base der PML4-Tabelle in CR3 laden
        mov eax, boot_pml4
        mov cr3, eax

; Long Mode (LME) & NX-Bit im EFER MSR aktivieren
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8) | (1 << 11)
        wrmsr

; Paging aktivieren (CR0.PG = 1, CR0.WP = 1)
        mov eax, cr0
        or eax, (1 << 31) | (1 << 16)
        mov cr0, eax

; 64-Bit GDT laden
        lgdt [gdt64_ptr]

; Far Jump in den 64-Bit Long Mode
        jmp 0x08:init_long_mode

.no_long_mode:
        cli
.hang:
        hlt                            ; Stoppen, falls Long Mode nicht unterstützt wird
        jmp .hang

; ==============================================================================
; Prüffunktionen für CPUID und Long-Mode-Unterstützung
; ==============================================================================
check_cpuid:
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 1 << 21               ; ID-Bit in EFLAGS toggeln
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
        test edx, 1 << 29              ; Long-Mode Bit im EDX prüfen
        jz .no_lm
        mov eax, 1
        ret
.no_lm:
        xor eax, eax
        ret

; ==============================================================================
; 64-Bit Long Mode Initialisierung
; ==============================================================================
        [BITS 64]

init_long_mode:
; 3. 64-Bit Datensegmente & FPU/SSE Hardware-Init
        mov ax, 0x10                   ; 64-Bit Datensegment (0x10)
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; FPU / SSE aktivieren (CR0 & CR4)
        mov rax, cr0
        and rax, ~((1 << 2) | (1 << 3)) ; EM & TS löschen
        or rax, (1 << 1)               ; MP setzen
        mov cr0, rax

        mov rax, cr4
        or rax, (1 << 9) | (1 << 10)   ; OSFXSR & OSXMMEXCPT setzen
        mov cr4, rax

        fninit                         ; FPU zurücksetzen

        sub rsp, 16
        mov dword [rsp], 0x1F80        ; Standard-MXCSR-Wert laden
        ldmxcsr [rsp]
        add rsp, 16

; 4. Sprung zur Kernel-Einstiegsfunktion _entry
        and rsp, -16                   ; 16-Byte Stack Alignment garantieren
        mov rax, _entry
        jmp rax

; ==============================================================================
; GDT & Speichertabellen
; ==============================================================================
        align 8
gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null-Deskriptor
        dq 0x00209A0000000000          ; 0x08: 64-Bit Code Segment
        dq 0x0000920000000000          ; 0x10: 64-Bit Data Segment
gdt64_end:

gdt64_ptr:
        dw gdt64_end - gdt64_start - 1
        dq gdt64_start

        align 8
multiboot_info_ptr:
        dq 0
multiboot_magic:
        dq 0

; Paging-Puffer für die Initialisierung (4 KB ausgerichtet)
        align 4096
        boot_pml4: times 4096 db 0
        boot_pdpt: times 4096 db 0
        boot_pdpt_direct: times 4096 db 0
        boot_pd_low: times 4096 db 0
        boot_pd_high: times 4096 * 4 db 0
