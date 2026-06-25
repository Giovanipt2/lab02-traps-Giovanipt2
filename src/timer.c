#include <arch/timer.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <kernel/types.h>

/* Flag: 1 = user alarm is pending and should print "alarm" when the timer fires */
static volatile int alarm_active = 0;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	/* Enable the Supervisor Timer Interrupt Enable bit in sie */
	csr_set(CSR_SIE, CSR_SIE_STIE);
	/* Enable global supervisor interrupts in sstatus */
	csr_set(CSR_SSTATUS, (1UL << 1));
}

void timer_irq_disable()
{
	/* Clear the Supervisor Timer Interrupt Enable bit in sie */
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = csr_read(CSR_TIME);
	u64 future = now + secs * TIMER_FREQ;
	csr_write(CSR_STIMECMP, future);
	alarm_active = 1;
}

void timer_irq()
{
	if (alarm_active) {
		alarm_active = 0;
		/* Disarm the timer by setting stimecmp to the maximum value */
		csr_write(CSR_STIMECMP, (u64)-1);
		info("alarm\n");
	} else {
		/* Disarm silently — no recurring tick needed */
		csr_write(CSR_STIMECMP, (u64)-1);
	}
}
