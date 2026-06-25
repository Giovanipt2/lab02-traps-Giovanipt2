#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/serial.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq(u64 cause)
{
	switch (cause) {
	case TRAP_TIMER_IRQ:
		timer_irq();
		break;
	case TRAP_EXTERNAL_IRQ:
		serial_irq();
		break;
	default:
		warn("unhandled IRQ: cause=0x%x\n", (u32)cause);
		break;
	}
}

void handle_exception(u64 cause)
{
	u64 sepc  = csr_read(CSR_SEPC);
	u64 stval = csr_read(CSR_STVAL);

	switch (cause) {
	case EXCEPTION_INST_PAGE_FAULT:
	case EXCEPTION_LOAD_PAGE_FAULT:
	case EXCEPTION_STORE_PAGE_FAULT:
		panic("page fault: cause=%lu sepc=0x%x stval=0x%x\n",
		      cause, (u32)sepc, (u32)stval);
		break;
	case EXCEPTION_INST_ACCESS_FAULT:
	case EXCEPTION_LOAD_ACCESS_FAULT:
	case EXCEPTION_STORE_ACCESS_FAULT:
		panic("access fault: cause=%lu sepc=0x%x stval=0x%x\n",
		      cause, (u32)sepc, (u32)stval);
		break;
	default:
		panic("unhandled exception: cause=%lu sepc=0x%x stval=0x%x\n",
		      cause, (u32)sepc, (u32)stval);
		break;
	}
}

void trap_setup()
{
	/* Write the address of our assembly entry point to stvec */
	csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
	u64 cause = csr_read(CSR_SCAUSE);

	if (cause & TRAP_IRQ_BIT) {
		/* Asynchronous interrupt */
		handle_irq(cause);
	} else {
		/* Synchronous exception */
		handle_exception(cause);
	}
}

void hart_irq_enable()
{
	/* Set SIE bit (bit 1) in sstatus to enable interrupts globally */
	csr_set(CSR_SSTATUS, (1UL << 1));
}

u64 hart_irq_save()
{
	/* Read and clear SIE bit atomically; return old sstatus */
	u64 sstatus = csr_read(CSR_SSTATUS);
	csr_clear(CSR_SSTATUS, (1UL << 1));
	return sstatus;
}

void hart_irq_restore(u64 flags)
{
	/* Restore the SIE bit from the saved flags */
	if (flags & (1UL << 1))
		csr_set(CSR_SSTATUS, (1UL << 1));
	else
		csr_clear(CSR_SSTATUS, (1UL << 1));
}

void hart_irq_disable()
{
	/* Clear SIE bit in sstatus to disable interrupts globally */
	csr_clear(CSR_SSTATUS, (1UL << 1));
}
