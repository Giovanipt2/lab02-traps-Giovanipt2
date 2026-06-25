#include <kernel/serial.h>
#include <kernel/mm.h>
#include <arch/plic.h>
#include <arch/csr.h>
#include <arch/spinlock.h>
#include <arch/io.h>
#include <kernel/types.h>

/*
 * The NS16550A serial device is at physical address 0x10000000.
 * Since the kernel runs with virtual memory enabled and the direct map
 * covers the first 32 GiB of physical memory starting at KERNEL_DIRECT_MAP_START,
 * we access the device at the corresponding virtual address.
 */
#define SERIAL_VBASE ((volatile u8 *)(0x10000000UL + KERNEL_DIRECT_MAP_START))

/* Hart 0 is the only hart we use */
#define SERIAL_HART 0

#define SERIAL_BUF_SIZE 256

/* Internal serial device state */
static struct {
	struct spinlock lock;
	char buf[SERIAL_BUF_SIZE];
	size_t len;
} dev;

/* Helper: read a byte-wide MMIO register */
static inline u8 serial_reg_read(u64 reg)
{
	return ioread8((void *)(SERIAL_VBASE + reg));
}

/* Helper: write a byte-wide MMIO register */
static inline void serial_reg_write(u8 val, u64 reg)
{
	iowrite8(val, (void *)(SERIAL_VBASE + reg));
}

void serial_init()
{
	spin_init(&dev.lock);
	dev.len = 0;

	/*
	 * Enable the "Received Data Available" interrupt in the IER
	 * so that we get an interrupt whenever a byte arrives.
	 */
	serial_reg_write((u8)SERIAL_IER_ERBFI, SERIAL_IER);

	/*
	 * Enable the hardware FIFO via the FCR so that the device
	 * buffers incoming bytes internally.
	 */
	serial_reg_write((u8)(SERIAL_FCR_FIFO_ENABLE |
			       SERIAL_FCR_RX_FIFO_CLEAR |
			       SERIAL_FCR_TX_FIFO_CLEAR), SERIAL_FCR);
}

void serial_irq_enable()
{
	/*
	 * Configure the PLIC so that IRQ 10 (the UART IRQ) is routed to hart 0.
	 * 1. Set the priority of IRQ 10 to 1 (must be > threshold)
	 * 2. Set the threshold for hart 0 to 0 (accept any priority > 0)
	 * 3. Enable IRQ 10 for hart 0
	 * 4. Allow the CPU to receive external interrupts (SEIE bit in sie)
	 */
	plic_irq_set_priority((u32)IRQ_SERIAL, 1);
	plic_hart_set_threshold(SERIAL_HART, 0);
	plic_hart_enable_irq(SERIAL_HART, (u32)IRQ_SERIAL);

	/* Enable Supervisor External Interrupt Enable in sie */
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	/* Claim the interrupt from the PLIC */
	u32 irq = plic_hart_claim_irq(SERIAL_HART);

	if (irq == (u32)IRQ_SERIAL) {
		/*
		 * Drain all available bytes from the UART RBR while the
		 * LSR Data Ready bit is set.  Hold the spinlock while
		 * we modify the shared buffer.
		 */
		spin_lock(&dev.lock);
		while (serial_reg_read(SERIAL_LSR) & (u8)SERIAL_LSR_DTR) {
			u8 c = serial_reg_read(SERIAL_RBR);
			if (dev.len < SERIAL_BUF_SIZE) {
				dev.buf[dev.len++] = (char)c;
			}
		}
		spin_unlock(&dev.lock);
	}

	/* Notify the PLIC that we finished handling this interrupt */
	if (irq)
		plic_hart_complete_irq(SERIAL_HART, irq);
}

size_t serial_read(char *buf)
{
	/*
	 * Use the irqsave variant so that we don't deadlock if a serial
	 * interrupt fires while we're holding the lock.
	 */
	u64 flags = spin_lock_irqsave(&dev.lock);
	size_t n = dev.len;
	for (size_t i = 0; i < n; i++) {
		buf[i] = dev.buf[i];
	}
	dev.len = 0;
	spin_unlock_irqrestore(&dev.lock, flags);
	return n;
}

void serial_putc(char c)
{
	/* Busy-wait until the Transmitter Holding Register is empty */
	while (!(serial_reg_read(SERIAL_LSR) & (u8)SERIAL_LSR_THRE))
		;
	serial_reg_write((u8)c, SERIAL_THR);
}

void serial_puts(char *str)
{
	while (*str != '\0') {
		serial_putc(*str);
		str++;
	}
}
