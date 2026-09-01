	[BITS   16]
	section .text

	global smp_trampoline_start
	global smp_trampoline_end

	global smp_trampoline_pml4
	global smp_trampoline_stack
	global smp_trampoline_entry

	%define TRAMPOLINE_BASE 0x8000
	%define REL_ADDR(x) (TRAMPOLINE_BASE + (x - smp_trampoline_start))

smp_trampoline_start:
	cli
	cld
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, REL_ADDR(smp_trampoline_stack_tmp)
	; mov sp, 0x7C00

	;    load temp gdt
	lgdt [REL_ADDR(ap_gdt32_ptr)]

	;   activate protected mode
	mov eax, cr0
	or  eax, 1
	mov cr0, eax

	;   Far Jump to 32-Bit Protected Mode (Deskriptor 0x08)
	jmp 0x08:REL_ADDR(ap_protected_mode_entry)

	[BITS 32]

ap_protected_mode_entry:
	mov ax, 0x10; 0x10 is 32-Bit Data in temp gdt
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	;   activate PAE
	mov eax, cr4
	or  eax, (1 << 5)
	mov cr4, eax

	;   load kernel_pml4_phys into cr3
	mov eax, [REL_ADDR(smp_trampoline_pml4)]
	mov cr3, eax

	;   activate long mode
	mov ecx, 0xC0000080
	rdmsr
	or  eax, (1 << 8) | (1 << 11); LME | NXE
	wrmsr

	;   activate paging
	mov eax, cr0
	or  eax, (1 << 31) | (1 << 16)
	mov cr0, eax

	;   Far Jump to 64-Bit Long Mode (Deskriptor 0x18)
	jmp 0x18:REL_ADDR(ap_long_mode_entry)

	[BITS 64]

ap_long_mode_entry:
	mov ax, 0x20; 0x20 is 64-Bit Data in temp gdt
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	;   load stack
	mov rsp, [REL_ADDR(smp_trampoline_stack)]

	;   jump to C code entry
	mov rax, [REL_ADDR(smp_trampoline_entry)]
	jmp rax

align 16

ap_gdt32_start:
	dq 0x0000000000000000; 0x00: Null
	dq 0x00CF9A000000FFFF; 0x08: 32-Bit Code
	dq 0x00CF92000000FFFF; 0x10: 32-Bit Data
	dq 0x00209A0000000000; 0x18: 64-Bit Code
	dq 0x0000920000000000; 0x20: 64-Bit Data

ap_gdt32_end:

	align 4

ap_gdt32_ptr:
	dw ap_gdt32_end - ap_gdt32_start - 1
	dd REL_ADDR(ap_gdt32_start)

	align 8
	smp_trampoline_pml4:  dq 0
	smp_trampoline_stack: dq 0
	smp_trampoline_entry: dq 0

align 16
    resb 256
smp_trampoline_stack_tmp:

smp_trampoline_end: