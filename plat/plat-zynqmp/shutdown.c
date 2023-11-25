#include <errno.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/irq.h>
#include <uk/print.h>
#include <uk/plat/bootstrap.h>

static void cpu_halt(void) __noreturn;

void ukplat_terminate(enum ukplat_gstate request __unused)
{
	uk_pr_info("Unikraft halted\n");
	/* Try to make system off */
	system_off();
	/*
	 * If we got here, there is no way to initiate "shutdown" on virtio
	 * without ACPI, so just halt.
	 */
	cpu_halt();
}

static void cpu_halt(void)
{
	__CPU_HALT();
}
