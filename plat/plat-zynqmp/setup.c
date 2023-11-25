#include <uk/config.h>
#include <arm/arm64/cpu.h>
#include <zynqmp/config.h>
#include <uk/essentials.h>
#include <libfdt.h>
#include <arm/mm.h>
#include <uk/plat/common/sections.h>
#include <xparameters.h>
#include <uk/essentials.h>
#include <rtc/rtc.h>

#define STACK_ADDRESS   (XPAR_PSU_OCM_RAM_0_S_AXI_HIGHADDR  - 0xFFF)

struct zynqmpplat_config _libzynqmpplat_cfg = { 0 };
static const char *appname = CONFIG_UK_NAME;

extern char *_heap_start;
extern char *_heap_end;
extern char *_STACK_SIZE;
extern char *_HEAP_SIZE;
extern char *__uk_dtb_start;

extern void _libplat_newstack(uint64_t stack_start,
		              void (*tramp)(void *), void *arg);
extern int _libplat_rtc_init(const void *dtb __maybe_unused);

extern uint64_t rtc_boot_ticks;
smcc_psci_callfn_t smcc_psci_call;
struct zynqmpplat_config _libzynqmpplat_cfg;


const void *ukplat_dtb_get(void)
{
	uk_pr_info("Dtb address %p\n", _libzynqmpplat_cfg.dtb.base);
	return (uintptr_t)_libzynqmpplat_cfg.dtb.base;
}

static void _libzynqplat_mem_setup(void)
{
	__u8 *page_table_end;
	__u32 size;

	_libzynqmpplat_cfg.pagetable.base = ALIGN_DOWN((uintptr_t)__END,
			                              __PAGE_SIZE);
	_libzynqmpplat_cfg.pagetable.len   = ALIGN_UP(page_table_size,
			                                __PAGE_SIZE);
	page_table_end = _libzynqmpplat_cfg.pagetable.base +
		_libzynqmpplat_cfg.pagetable.len;

	_libzynqmpplat_cfg.heap.base = heap_ptr;
	_libzynqmpplat_cfg.heap.len = heap_size;

	_libzynqmpplat_cfg.bstack.base = stack_ptr;
	_libzynqmpplat_cfg.bstack.len = stack_size;

}

size_t _libzynqmpplat_heap_size(size_t heap_avail)
{
	return ((uintptr_t)_libzynqmpplat_cfg.heap.len) - heap_avail;
}

size_t _libzynqmpplat_stack_size(uintptr_t curr_sp)
{
	return ((uintptr_t)_libzynqmpplat_cfg.bstack.base) - curr_sp;
}


void _libzynqmpplat_entry2(void)
{
	/* Initialize the zynq platform */
	//XFsbl_Initialize(&_libzynqmpplat_cfg.xfsblps);


#ifdef CONFIG_ZYNQMP_UARTPS
	/* TODO:move the page table initialization here */
	_libplat_init_console(_libzynqmpplat_cfg.dtb.base);
#endif /* CONFIG_ZYNQMP_UARTPS */

	/* Initialize the interrupt controller */
	intctrl_init();

#ifdef CONFIG_ZYNQMP_XLNX_RTC
	/* Initialize the boot tick */
	_libplat_rtc_init(_libzynqmpplat_cfg.dtb.base);
#endif /* CONFIG_ZYNQMP_XLNX_RTC */


	ukplat_entry_argp(DECONST(char *, appname),
              NULL, 0);
}

void _libplat_start(void *dtb_pointer __unused)
{
	int ret;

	memset(&_libzynqmpplat_cfg, 0, sizeof(_libzynqmpplat_cfg));
	if ((ret = fdt_check_header(__uk_dtb_start)))
		UK_CRASH("Invalid DTB: %s\n", fdt_strerror(ret));

	_libzynqmpplat_cfg.dtb.base = __uk_dtb_start;
	_libzynqmpplat_cfg.dtb.len = fdt_totalsize(__uk_dtb_start);

	_libzynqplat_mem_setup();
	_libzynqmpplat_entry2();
}
