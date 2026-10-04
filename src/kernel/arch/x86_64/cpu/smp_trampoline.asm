; /**
;  * @file smp_trampoline.asm
;  * @brief Real-Mode Trampoline Code für das Aufwachen von Application Processors (APs) im SMP-Betrieb.
;  * @details Führt den schrittweisen Übergang durch: 16-Bit Real Mode -> 32-Bit Protected Mode -> 64-Bit Long Mode.
;  * @author friedrichOsDev
;  */

        [BITS 16]
        section .text

        global smp_trampoline_start
        global smp_trampoline_end
        global smp_trampoline_pml4
        global smp_trampoline_stack    ; MUSS 16-Byte aligned sein
        global smp_trampoline_entry

        %define TRAMPOLINE_BASE 0x8000
        %define REL_ADDR(x) (TRAMPOLINE_BASE + ((x) - smp_trampoline_start))

; ==============================================================================
; 16-Bit Real Mode Einstiegspunkt (Initiiert durch SIPI)
; ==============================================================================
smp_trampoline_start:
        cli                            ; Interrupts deaktivieren
        cld                            ; Richtungs-Flag für String-Instruktionen zurücksetzen

; 0. Segmentregister & temporären Stack im Real Mode aufsetzen
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, REL_ADDR(smp_trampoline_stack_tmp)

; 1. Übergang in den 32-Bit Protected Mode
        lgdt [REL_ADDR(ap_gdt64_ptr)]  ; Temporäre GDT laden

        mov eax, cr0
        or eax, 1                      ; Protected Mode Bit (PE) setzen
        mov cr0, eax

        jmp 0x08:REL_ADDR(ap_protected_mode_entry) ; Far Jump zum Leeren der Pipeline

; ==============================================================================
; 32-Bit Protected Mode
; ==============================================================================
        [BITS 32]

ap_protected_mode_entry:
        mov ax, 0x10                   ; Datensegment-Selektor (0x10) laden
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; 2. Steuerregister & Long Mode Aktivierung
; PAE (Physical Address Extension) in CR4 aktivieren
        mov eax, cr4
        or eax, (1 << 5)
        mov cr4, eax

; PML4-Adresse (CR3) laden
        mov eax, [REL_ADDR(smp_trampoline_pml4)]
        mov cr3, eax

; Long Mode (LME) & No-Execute (NX) im EFER MSR aktivieren
        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8) | (1 << 11)
        wrmsr

; Paging aktivieren (CR0.PG = 1, CR0.WP = 1)
        mov eax, cr0
        or eax, (1 << 31) | (1 << 16)
        mov cr0, eax

; Far Jump in den 64-Bit Long Mode
        jmp 0x18:REL_ADDR(ap_long_mode_entry)

; ==============================================================================
; 64-Bit Long Mode
; ==============================================================================
        [BITS 64]

ap_long_mode_entry:
; 3. 64-Bit Datensegmente & FPU/SSE Initialisierung
        mov ax, 0x20                   ; 64-Bit Datensegment (0x20)
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

; FPU- & SSE-Hardware-Unterstützung aktivieren
        mov rax, cr0
        and rax, ~((1 << 2) | (1 << 3)) ; EM & TS Bits löschen
        or rax, (1 << 1)               ; MP Bit setzen
        mov cr0, rax

        mov rax, cr4
        or rax, (1 << 9) | (1 << 10)   ; OSFXSR & OSXMMEXCPT Bits setzen
        mov cr4, rax

        fninit                         ; FPU initialisieren

        sub rsp, 16
        mov dword [rsp], 0x1F80        ; Standard-MXCSR-Wert (Exceptions maskiert)
        ldmxcsr [rsp]
        add rsp, 16

; 4. Ziel-Stack laden & Sprung in die Kernel-Einstiegsfunktion
        mov rax, [rel smp_trampoline_stack]
        mov rsp, rax

        and rsp, -16                   ; 16-Byte Stack Alignment garantieren

        mov rax, [rel smp_trampoline_entry]
        jmp rax                        ; Sprung in die C-Kernel-Logik für APs

; ==============================================================================
; Datenstrukturen & Pufferspeicher
; ==============================================================================
        align 16
ap_gdt64_start:
        dq 0x0000000000000000          ; 0x00: Null-Deskriptor
        dq 0x00CF9A000000FFFF          ; 0x08: 32-Bit Code
        dq 0x00CF92000000FFFF          ; 0x10: 32-Bit Daten
        dq 0x00209A0000000000          ; 0x18: 64-Bit Code
        dq 0x0000920000000000          ; 0x20: 64-Bit Daten
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
smp_trampoline_stack_tmp:              ; Temporärer 16-Bit Stack

smp_trampoline_end: