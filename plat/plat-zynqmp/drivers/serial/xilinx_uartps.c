/* SPDX-License-Identifier: ISC */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratries Europe GmbH
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <libfdt.h>
#include <errno.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <arm/cpu.h>
#include <uk/essentials.h>
#include <xparameters.h>

#define UART_FIFO_SIZE  64


/* Register definitions for Cadence UART Controller.
 */
#define CDNC_UART_CTRL_OFF  0x00        /* Control Register. */
#define CDNC_UART_CTRL_REG_STOPBRK  (1<<8)
#define CDNC_UART_CTRL_REG_STARTBRK (1<<7)
#define CDNC_UART_CTRL_REG_TORST    (1<<6)
#define CDNC_UART_CTRL_REG_TX_DIS   (1<<5)
#define CDNC_UART_CTRL_REG_TX_EN    (1<<4)
#define CDNC_UART_CTRL_REG_RX_DIS   (1<<3)
#define CDNC_UART_CTRL_REG_RX_EN    (1<<2)
#define CDNC_UART_CTRL_REG_TXRST    (1<<1)
#define CDNC_UART_CTRL_REG_RXRST    (1<<0)

#define CDNC_UART_MODE_OFF  0x01        /* Mode Register. */
#define CDNC_UART_MODE_REG_CHMOD_R_LOOP (3<<8)  /* [9:8] - channel mode */
#define CDNC_UART_MODE_REG_CHMOD_L_LOOP (2<<8)
#define CDNC_UART_MODE_REG_CHMOD_AUTECHO (1<<8)
#define CDNC_UART_MODE_REG_STOP2    (2<<6)  /* [7:6] - stop bits */
#define CDNC_UART_MODE_REG_PAR_NONE (4<<3)  /* [5:3] - parity type */
#define CDNC_UART_MODE_REG_PAR_MARK (3<<3)
#define CDNC_UART_MODE_REG_PAR_SPACE    (2<<3)
#define CDNC_UART_MODE_REG_PAR_ODD  (1<<3)
#define CDNC_UART_MODE_REG_PAR_EVEN (0<<3)
#define CDNC_UART_MODE_REG_6BIT     (3<<1)  /* [2:1] - character len */
#define CDNC_UART_MODE_REG_7BIT     (2<<1)
#define CDNC_UART_MODE_REG_8BIT     (0<<1)
#define CDNC_UART_MODE_REG_CLKSEL   (1<<0)

#define CDNC_UART_IEN_OFF   0x02        /* Interrupt registers. */
#define CDNC_UART_IDIS_OFF  0x03
#define CDNC_UART_IMASK_OFF 0x4
#define CDNC_UART_ISTAT_OFF 0x5
#define CDNC_UART_INT_TXOVR     (1<<12)
#define CDNC_UART_INT_TXNRLYFUL     (1<<11) /* tx "nearly" full */
#define CDNC_UART_INT_TXTRIG        (1<<10)
#define CDNC_UART_INT_DMSI      (1<<9)  /* delta modem status */
#define CDNC_UART_INT_RXTMOUT       (1<<8)
#define CDNC_UART_INT_PARITY        (1<<7)
#define CDNC_UART_INT_FRAMING       (1<<6)
#define CDNC_UART_INT_RXOVR     (1<<5)
#define CDNC_UART_INT_TXFULL        (1<<4)
#define CDNC_UART_INT_TXEMPTY       (1<<3)
#define CDNC_UART_INT_RXFULL        (1<<2)
#define CDNC_UART_INT_RXEMPTY       (1<<1)
#define CDNC_UART_INT_RXTRIG        (1<<0)
#define CDNC_UART_INT_ALL       0x1FFF

#define CDNC_UART_BAUDGEN_OFF   0x6
#define CDNC_UART_RX_TIMEO_OFF  0x7
#define CDNC_UART_RX_WATER_OFF 0x8

#define CDNC_UART_MODEM_CTRL_OFF 0x9
#define CDNC_UART_MODEM_CTRL_REG_FCM    (1<<5)  /* automatic flow control */
#define CDNC_UART_MODEM_CTRL_REG_RTS    (1<<1)
#define CDNC_UART_MODEM_CTRL_REG_DTR    (1<<0)

#define CDNC_UART_MODEM_STAT_OFF 0xa
#define CDNC_UART_MODEM_STAT_REG_FCMS   (1<<8)  /* flow control mode (rw) */
#define CDNC_UART_MODEM_STAT_REG_DCD    (1<<7)
#define CDNC_UART_MODEM_STAT_REG_RI (1<<6)
#define CDNC_UART_MODEM_STAT_REG_DSR    (1<<5)
#define CDNC_UART_MODEM_STAT_REG_CTS    (1<<4)
#define CDNC_UART_MODEM_STAT_REG_DDCD   (1<<3)  /* change in DCD (w1tc) */
#define CDNC_UART_MODEM_STAT_REG_TERI   (1<<2)  /* trail edge ring (w1tc) */
#define CDNC_UART_MODEM_STAT_REG_DDSR   (1<<1)  /* change in DSR (w1tc) */
#define CDNC_UART_MODEM_STAT_REG_DCTS   (1<<0)  /* change in CTS (w1tc) */

#define CDNC_UART_CHAN_STAT_OFF 0xb        /* Channel status register. */
#define CDNC_UART_CHAN_STAT_REG_TXNRLYFUL (1<<14) /* tx "nearly" full */
#define CDNC_UART_CHAN_STAT_REG_TXTRIG  (1<<13)
#define CDNC_UART_CHAN_STAT_REG_FDELT   (1<<12)
#define CDNC_UART_CHAN_STAT_REG_TXACTIVE (1<<11)
#define CDNC_UART_CHAN_STAT_REG_RXACTIVE (1<<10)
#define CDNC_UART_CHAN_STAT_REG_TXFULL  (1<<4)
#define CDNC_UART_CHAN_STAT_REG_TXEMPTY (1<<3)
#define CDNC_UART_CHAN_STAT_REG_RXEMPTY (1<<1)
#define CDNC_UART_CHAN_STAT_REG_RXTRIG  (1<<0)

#define CDNC_UART_FIFO_OFF      0xC        /* Data FIFO (tx and rx) */
#define CDNC_UART_BAUDDIV_OFF   0xD
#define CDNC_UART_FLOWDEL_OFF   0xE
#define CDNC_UART_TX_WATER_OFF  0xF


/*
 * Xilinx UARTPS base address
 */
#if defined(XPAR_PSU_UART_1_BASEADDR)
static uint8_t uart_initialized = 1;
static uk_reg32_t uart_bas = XPAR_PSU_UART_1_BASEADDR;
static int baud_rate = 115200;
static int clock_rate = XPAR_PSU_UART_1_UART_CLK_FREQ_HZ;
#else
static uint8_t uart_initialized = 0;
static uk_reg32_t uart_bas;
static int baud_rate, clock_rate;
#endif

static int setup_zynq_uartps(uk_reg32_t bas, uint32_t baudrate, uint32_t clk_ref)
{
	uint32_t mode_reg_value = 0;
#if 0
	switch (databits) {
	case 6:
		mode_reg_value |= CDNC_UART_MODE_REG_6BIT;
		break;
	case 7:
	        mode_reg_value |= CDNC_UART_MODE_REG_7BIT;
		break;
	case 8:
	default:
	        mode_reg_value |= CDNC_UART_MODE_REG_8BIT;
		break;
	}
	if (stopbits == 2)
	            mode_reg_value |= CDNC_UART_MODE_REG_STOP2;

	switch (parity) {
	case UART_PARITY_MARK:
		mode_reg_value |= CDNC_UART_MODE_REG_PAR_MARK;
		break;
	case UART_PARITY_SPACE:
		mode_reg_value |= CDNC_UART_MODE_REG_PAR_SPACE;
		break;
	case UART_PARITY_ODD:
		mode_reg_value |= CDNC_UART_MODE_REG_PAR_ODD;
		break;
	case UART_PARITY_EVEN:
	        mode_reg_value |= CDNC_UART_MODE_REG_PAR_EVEN;
		break;
	case UART_PARITY_NONE:
	default:
		mode_reg_value |= CDNC_UART_MODE_REG_PAR_NONE;
		break;
        }
	ioreg_write32((const volatile void *)(
			bas + CDNC_UART_MODE_REG), mode_reg_value);
#endif /* DISABLE_CODE */

	uint32_t baudgen, bauddiv;
	uint32_t best_bauddiv, best_baudgen, best_error;
	uint32_t baud_out, err;

	best_bauddiv = 0;
	best_baudgen = 0;
	best_error = ~0;

	/* Try all possible bauddiv values and pick best match. */
	for (bauddiv = 4; bauddiv <= 255; bauddiv++) {
		baudgen = (clk_ref + (baudrate * (bauddiv + 1)) / 2) /
			  (baudrate * (bauddiv + 1));
		if (baudgen < 1 || baudgen > 0xffff)
			continue;

		baud_out = clk_ref / (baudgen * (bauddiv + 1));
		err = baud_out > baudrate ?
		    baud_out - baudrate : baudrate - baud_out;

		if (err < best_error) {
			best_error = err;
			best_bauddiv = bauddiv;
			best_baudgen = baudgen;
		}
	}

	if (best_bauddiv > 0) {
		ioreg_write32((bas + CDNC_UART_BAUDDIV_OFF),
			      best_bauddiv);
		ioreg_write32(bas + CDNC_UART_BAUDGEN_OFF,
			      best_baudgen);
		return 0;
	} else
		return -1; /* out of range */
}

static void init_zynq_uartps(uk_reg32_t bas)
{
	/* Reset RX and TX. */
	ioreg_write32((bas + CDNC_UART_CTRL_OFF),
		      CDNC_UART_CTRL_REG_RXRST | CDNC_UART_CTRL_REG_TXRST);
	/* Interrupts all off. */
	ioreg_write32(bas + CDNC_UART_IDIS_OFF, CDNC_UART_INT_ALL);
	ioreg_write32(bas + CDNC_UART_ISTAT_OFF, CDNC_UART_INT_ALL);
	/* Clear delta bits. */
	ioreg_write32(bas + CDNC_UART_MODEM_STAT_OFF,
		      CDNC_UART_MODEM_STAT_REG_DDCD |
		      CDNC_UART_MODEM_STAT_REG_TERI |
		      CDNC_UART_MODEM_STAT_REG_DDSR |
		      CDNC_UART_MODEM_STAT_REG_DCTS);
	/* RX FIFO water level, stale timeout */
	ioreg_write32(bas + CDNC_UART_RX_WATER_OFF, UART_FIFO_SIZE/2);
	ioreg_write32(bas + CDNC_UART_RX_TIMEO_OFF, 10);
	/* TX FIFO water level (not used.) */
	ioreg_write32(bas + CDNC_UART_TX_WATER_OFF,
		      UART_FIFO_SIZE/2);
	/* Bring RX and TX online. */
	ioreg_write32(bas + CDNC_UART_CTRL_OFF,
		      CDNC_UART_CTRL_REG_RX_EN |
		      CDNC_UART_CTRL_REG_TX_EN |
		      CDNC_UART_CTRL_REG_TORST |
		      CDNC_UART_CTRL_REG_STOPBRK);
	/* Set DTR and RTS. */
	ioreg_write32(bas + CDNC_UART_MODEM_CTRL_OFF,
		      CDNC_UART_MODEM_CTRL_REG_DTR |
		      CDNC_UART_MODEM_CTRL_REG_RTS);
}

void _libplat_init_console(const void *dtb_base __maybe_unused)
{
	int offset, len, naddr, nsize, rc, clock_cells;
	const uint64_t *regs;
	uint64_t reg_uart_bas;
	uint32_t *baud_rate_ref = NULL;
	uint32_t *clock_ref, *clock_freq_ref, *clock_cells_prop, clock_phandle;

#ifndef CONFIG_ZYNQMP_LIBOFW 
	offset = fdt_node_offset_by_compatible(dtb_base,
					-1, "xlnx,xuartps");
	if (offset < 0)
		UK_CRASH("No console UART found!\n");

	naddr = fdt_address_cells(dtb_base, offset);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper address cells!\n");

	nsize = fdt_size_cells(dtb_base, offset);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper size cells!\n");

	regs = fdt_getprop(dtb_base, offset, "reg", &len);
	if (regs == NULL || (len < (int)sizeof(fdt32_t) * (naddr + nsize)))
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, len);

	reg_uart_bas = fdt64_to_cpu(regs[0]);
	uart_bas = reg_uart_bas;

	baud_rate_ref = fdt_getprop(dtb_base, offset, "current-speed", &len);
	if (!baud_rate_ref)
		baud_rate = 115200;
	else 
		baud_rate = fdt32_to_cpu(baud_rate_ref[0]);
	clock_ref = fdt_getprop(dtb_base, offset, "clocks",
				&len);
	if (!clock_ref)
		UK_CRASH("uart clock was not found\n");

	/** 
	 * Search the clock node for the properties of clock-frequency
	 * after reading the clock-cells
	 */
	clock_phandle = fdt32_to_cpu(clock_ref[0]);
	offset = fdt_node_offset_by_phandle(dtb_base, clock_phandle);
	if (offset < 0)
		UK_CRASH("uart clock was not found through the handle\n");

	clock_cells_prop = fdt_getprop(dtb_base, offset, "#clock-cells", &len);
	if (!clock_cells_prop)
		UK_CRASH("Clock cell property was missing\n");

	clock_cells = fdt32_to_cpu(clock_cells_prop[0]); 
	if (clock_cells == 0) {
		/* Single clock producer */
		clock_freq_ref = fdt_getprop(dtb_base, offset,
				"clock-frequency", &len);
		if (!clock_freq_ref)
			UK_CRASH("Clock frequency property was missing\n");
		clock_rate = fdt32_to_cpu(*clock_freq_ref);
	}

#else
	if (!uart_bas)
		UK_CRASH("No console UART found!\n");
	reg_uart_bas = (uk_reg32_t )uart_bas;
#endif /* CONFIG_ZYNQMP_LIBOFW */

	init_zynq_uartps(reg_uart_bas);
	rc = setup_zynq_uartps(reg_uart_bas, baud_rate, clock_rate);
	if (rc < 0)
		UK_CRASH("UART setup failed!\n");

	uart_initialized = 1;
	uk_pr_info("Zynq PS UART initialized\n");
}

int ukplat_coutd(const char *str, uint32_t len)
{
	return ukplat_coutk(str, len);
}

static void xuartps_write(char a)
{
	/*
	 * Avoid using the UART before base address initialized,
	 * or CONFIG_KVM_EARLY_DEBUG_PL011_UART doesn't be enabled.
	 */
	if (!uart_initialized)
		return;

	/* Wait until TX FIFO becomes empty */
}

static void xuartps_putc(char a)
{
	if (!uart_initialized)
		return;

	while ((ioreg_read32(
		(uart_bas + CDNC_UART_CHAN_STAT_OFF)) &
		    CDNC_UART_CHAN_STAT_REG_TXFULL) !=
			    0);
	ioreg_write32((uart_bas + CDNC_UART_FIFO_OFF), a);
	while ((ioreg_read32(
		uart_bas + CDNC_UART_CHAN_STAT_OFF) &
		CDNC_UART_CHAN_STAT_REG_TXEMPTY) == 0);
}

/* Try to get data from pl011 UART without blocking */
static int xuartps_getc(void)
{
	/*
	 * Avoid using the UART before base address initialized
	 */
	if (!uart_initialized)
		return -1;

	/* If RX FIFO is empty, return -1 immediately */
	if (ioreg_read32((uart_bas + CDNC_UART_CHAN_STAT_OFF)) &
			  CDNC_UART_CHAN_STAT_REG_RXEMPTY)
		return -1;

	return (int) (ioreg_read32(uart_bas + CDNC_UART_FIFO_OFF) & 0xff);
}

int ukplat_coutk(const char *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		xuartps_putc(buf[i]);
	return len;
}

int ukplat_cink(char *buf, unsigned int maxlen)
{
	int ret;
	unsigned int num = 0;

	while (num < maxlen && (ret = xuartps_getc()) >= 0) {
		*(buf++) = (char) ret;
		num++;
	}

	return (int) num;
}
