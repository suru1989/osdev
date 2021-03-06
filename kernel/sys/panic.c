/* vim: tabstop=4 shiftwidth=4 noexpandtab
 *
 * Panic functions
 */
#include <system.h>

void kernel_halt() {
	kprintf("\n System Halted!\n\n");

	while (1) {
		IRQ_OFF;
		PAUSE;
	}
}

void halt_and_catch_fire(char * error_message, const char * file, int line, struct regs * regs) {
	IRQ_OFF;
	kprintf("\033[1;37;44m");
	kprintf("HACF: %s\n", error_message);
	kprintf("Proc: %d\n", getpid());
	kprintf("File: %s\n", file);
	kprintf("Line: %d\n", line);
	if (regs) {
		kprintf("Registers at interrupt:\n");
		kprintf("eax=0x%x ebx=0x%x\n", regs->eax, regs->ebx);
		kprintf("ecx=0x%x edx=0x%x\n", regs->ecx, regs->edx);
		kprintf("esp=0x%x ebp=0x%x\n", regs->esp, regs->ebp);
		kprintf("Error code: 0x%x\n",  regs->err_code);
		kprintf("EFLAGS:     0x%x\n",  regs->eflags);
		kprintf("User ESP:   0x%x\n",  regs->useresp);
		kprintf("eip=0x%x\n",          regs->eip);
	}
	kprintf("This process has been descheduled.\n");
	kprintf("\033[0m");
	kexit(1);
}

void assert_failed(const char *file, uint32_t line, const char *desc) {
	IRQ_OFF;
	kprintf("Kernel Assertion Failed: %s\n", desc);
	kprintf("File: %s\n", file);
	kprintf("Line: %d\n", line);
	kernel_halt();
}
