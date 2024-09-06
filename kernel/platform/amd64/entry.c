#include "ndk/sched.h"
#include <limine.h>

#include <nyyconf.h>
#include <limine-generic.h>
#include <ndk/vm.h>
#include <ndk/time.h>
#include <dkit/console.h>

#include "cpuid.h"
#include "asm.h"
#include "apic.h"
#include "gdt.h"
#include "idt.h"
#include "early_acpi.h"
#include "hpet.h"

cpu_features_t g_features;

#define COM1 0x3f8

void cpudata_port_setup(cpudata_port_t *cpudata)
{
	cpudata->self = cpudata;
	wrmsr(kMsrGsBase, (uint64_t)cpudata->self);
}

cpudata_port_t *port_get_cpudata()
{
	cpudata_port_t *data = 0;
	asm volatile("mov %%gs:0x0, %0" : "=r"(data)::"memory");
	return data;
}

#ifdef CONFIG_SERIAL
static void serial_putc(int c)
{
	while (!(inb(COM1 + 5) & 0x20))
		;
	outb(COM1, c);
}

void serial_write(console_t *, const char *msg, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		serial_putc(msg[i]);
	}
}

static void serial_init()
{
	outb(COM1 + 1, 0x00); // Disable all interrupts
	outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(COM1 + 1, 0x00); //                  (hi byte)
	outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static console_t serial_console = {
	.name = "serial",
	.write = serial_write,
};

void _port_init_boot_consoles()
{
	serial_init();
	console_add(&serial_console);
}
#endif

void port_init_bsp(cpudata_t *bsp_data)
{
	cpu_gdt_init();
	cpu_idt_init();

	uint32_t a, b, c, d;
	cpuid(0x80000001, &a, &b, &c, &d);
	g_features.nx = (d >> 20) & 1;
	g_features.gbpages = (d >> 26) & 1;
	cpuid(1, &a, &b, &c, &d);
	g_features.pge = (d >> 13) & 1;
	g_features.pat = (d >> 16) & 1;
	g_features.pcid = (c >> 17) & 1;
}

void port_pat_setup()
{
#define PAT_UC 0x0L
#define PAT_WC 0x1L
#define PAT_WT 0x4L
#define PAT_WP 0x5L
#define PAT_WB 0x6L
#define PAT_UCMINUS 0x7L
#define PA_N(type, n) ((uint64_t)(type) << (n * 8))

	uint64_t pat = PA_N(PAT_WB, 0) | PA_N(PAT_WT, 1) |
		       PA_N(PAT_UCMINUS, 2) | PA_N(PAT_UC, 3) |
		       PA_N(PAT_WP, 4) | PA_N(PAT_WC, 5);
	wrmsr(kMsrPAT, pat);

#undef PA_N
#undef PAT_UC
#undef PAT_WC
#undef PAT_WT
#undef PAT_WB
#undef PAT_WP
#undef PAT_UCMINUS
}

static void port_init_cpu()
{
	port_pat_setup();
	uint64_t cr0 = read_cr0();
	// clear CD and NW
	cr0 &= ~(1 << 30);
	cr0 &= ~(1 << 29);
	// enable WP
	cr0 |= (1 << 16);
	write_cr0(cr0);
	// enable PGE and PSE
	// TODO: PCID?
	write_cr4(read_cr4() | (1 << 4) | (1 << 7));
	// enable NX
	if (g_features.nx)
		wrmsr(kMsrEfer, rdmsr(kMsrEfer) | (1 << 11));
}

void port_data_common_init(cpudata_t *data)
{
	cpu_gdt_load();
	cpu_idt_load();

	cpudata_setup(data);

	port_init_cpu();
}

extern void core_spinup();

void port_smp_entry(struct limine_smp_info *info)
{
	struct smp_info *nyy_info = (struct smp_info *)info->extra_argument;

	cpudata_t *localdata = &nyy_info->data;
	port_data_common_init(localdata);
	localdata->port_data.lapic_id = info->lapic_id;

	vm_port_activate(vm_kmap());

	apic_enable();

	// XXX: make this the idle thread of the cpu
	printk(INFO "entered kernel on core %d\n", info->lapic_id);

	bool ready = true;
	__atomic_store(&nyy_info->ready, &ready, __ATOMIC_RELEASE);

	core_spinup();
}

#define LIMINE_REQ __attribute__((used, section(".requests")))

LIMINE_REQ static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST,
	.revision = 0
};

void port_scheduler_init()
{
	acpi_early_set_rsdp(rsdp_request.response->address);
	assert(hpet_init() == 0);
	// init fallback clocks before apic_init
	apic_init();
	apic_enable();

	char vendor[13];
	cpuid_vendor(vendor);
	printk(DEBUG "CPU vendor: %s\n", vendor);
}
