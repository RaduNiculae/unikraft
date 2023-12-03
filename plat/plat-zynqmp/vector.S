#include <uk/asm.h>

.global _libplat_entry

.org 0
.section .text.init, "x"
ENTRY(_boot_zynqmp)
b _libplat_entry
END(_boot_zynqmp)
