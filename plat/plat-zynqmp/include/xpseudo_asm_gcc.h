#ifndef __XPSUEDO_ASM_GCC_H
#define __XPSUEDO_ASM_GCC_H

#include <arm/arm64/cpu.h>

#define mfcp(regs)	SYSREG_READ(regs)

#define mtcptlbi(reg) TLBI(reg)

#define mtcp(regs,val) SYSREG_WRITE(regs,(val))
#define mtcpdc(regs,val) DC_WRITE(regs,val)
#define mtcpic(regs,val) IC_WRITE(regs,val)
#define mtcpicall(regs) IC_READ(regs)
#define mfcpsr()  SYSREG_READ32(DAIF)
#define mtcpsr(v)  SYSREG_WRITE32(DAIF,(v))

#define dsb() DSB()

#define isb() ISB()

/* Count leading zeroes (clz) */                                                                                                                               
#define clz(arg)    ({u8 rval; \
					 __asm__ __volatile__(\
					   "clz    %0,%1"\
					   : "=r" (rval) : "r" (arg)\
					 );\
					 rval;\
					})

#endif /* __XPSUEDO_ASM_GCC_H */
