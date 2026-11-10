; Embedded NanoOS PSF2 bitmap font.
; Keep this file in the compositor directory so the existing recursive
; assembly source discovery includes it automatically.

BITS 64
SECTION .rodata

GLOBAL nanoos_font_start
GLOBAL nanoos_font_end

nanoos_font_start:
    incbin "assets/font.psf"
nanoos_font_end:

SECTION .note.GNU-stack noalloc noexec nowrite progbits
