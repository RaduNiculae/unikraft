.section .dtb.init.rodata,"a"
.balign 16
.global __uk_dtb_start;
__dtb_start:
.incbin "libzynqmpplat/u-boot.dtb"
__uk_dtb_end:
.global __uk_dtb_end;
.balign 16

__uk_dtb_start: .dword __dtb_start
