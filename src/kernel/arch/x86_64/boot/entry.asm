	[BITS   64]
	section .text.entry
	global  _entry
	global  stack_top
	extern  kernel_init
	extern  multiboot_magic
	extern  multiboot_info_ptr

_entry:
	;      zero bss section
	extern sbss
	extern ebss

	mov rdi, sbss
	mov rcx, ebss
	sub rcx, rdi
	shr rcx, 3
	xor rax, rax
	rep stosq

	;   load 64-bit stack
	mov rsp, stack_top

	;    void kernel_init(uint64_t magic, uint64_t info_ptr)
	mov  rdi, [multiboot_magic]
	mov  rsi, [multiboot_info_ptr]
	call kernel_init

.hang:
	cli
	hlt
	jmp .hang

section .bss
align   16

stack_bottom:
	resb 16384; 16 KB Stack

stack_top:
