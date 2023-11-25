#include <uk/asm.h>
#include <arm/cpu_defs.h> 

.global _boot
.global _libplat_entry

.set EL3_stack, _el3_stack
.set vector_base,   _vector_table
.set rvbar_base,    0xFD5C0040

ENTRY(_boot)
/* 
  Use only a single processor */
	mrs x0, mpidr_el1 
	and x0, x0, #0xFF
	cbz      x0, OKToRun
EndlessLoop0:
	wfi
	b EndlessLoop0

OKToRun:
	ldr x1, =vector_base
	msr VBAR_EL3,x1

	mrs  x0, MPIDR_EL1
	and  x0, x0, #0xFF
	mov  w0, w0

	ldr  w2, =rvbar_base
	mov  w3, #0x8
	mul  w0, w0, w3
	add  w2, w2, w0
	str  x1, [x2]
    /*Define stack pointer for current exception level*/
    ldr  x2,=EL3_stack
    mov  sp,x2

	orr      x0, x0, #(0x1 << 10)
	msr      CPTR_EL3, x0
	isb

	mov      x0, #0

	b _libplat_entry
END(_boot)
