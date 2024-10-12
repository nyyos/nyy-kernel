#include <ndk/cpudata.h>
#include <ndk/sched.h>
#include <ndk/irq.h>
#include <ndk/kmem.h>
#include <ndk/vm.h>
#include <ndk/time.h>
#include <DevKit/acpi.h>
#include <DevKit/DevKit.h>

static cpudata_t bsp_data;

static void kmain_threaded(void *, void *);
[[gnu::noreturn]] static void core_spinup();

void early_port_bsp_init(cpudata_t *bsp_data);
void early_port_cpu_common_init(cpudata_t *data);
void early_port_bootinfo();
void early_port_output();
void early_port_add_pmem();
void early_port_map_memory();
void early_port_post_kmem();
void early_port_post_acpi();
uintptr_t early_port_get_rsdp();

void port_start_cores();

void idle_thread_fn(void *, void *)
{
	assert(port_int_state() != 0);
	for (;;) {
		printk(INFO "cpu %d is idle\n", cpudata()->cpu_id);
		port_wait_nextint();
	}
}

void kickstart_kmain(void *, void *)
{
	thread_t kmain_thread;

	sched_init_thread(&kmain_thread, kmain_threaded, kPriorityHigh, NULL,
			  NULL);
	sched_resume(&kmain_thread);

	/* after launching kmain become idle thread */
	idle_thread_fn(NULL, NULL);
}

[[gnu::noreturn]] void kmain()
{
	printk_init();
	early_port_output();

	early_port_bootinfo();

	// initialize basic structures (e.g. GDT/IDT on x86, query CPU features ...)
	early_port_bsp_init(&bsp_data);
	// responsible for loading cpudata
	early_port_cpu_common_init(&bsp_data);
	bsp_data.bsp = true;

	/* basic memory init */
	pm_init();
	early_port_add_pmem();
	printk(INFO "pm ready\n");
	vm_port_init_map(vm_kmap());
	early_port_map_memory();
	vm_port_activate(vm_kmap());
	va_kernel_init();

	/* extended init */
	sched_init_thread(&cpudata()->idle_thread, kickstart_kmain, 0, NULL,
			  NULL);

	sched_jump_into_idle_thread();
	core_spinup();
}

[[gnu::noreturn]] static void core_spinup()
{
	irql_lower(IRQL_PASSIVE);
	for (;;) {
		port_enable_ints();
		port_wait_nextint();
	}
}

extern void test_fn();
extern void libkern_init();

static void kmain_threaded(void *, void *)
{
	printk(INFO "entered threaded kmain\n");
	kmem_init();
	sched_kmem_init();

	early_port_post_kmem();
	irq_init();
	time_init();

	uintptr_t rsdp = early_port_get_rsdp();
	acpi_early_init(rsdp);
	early_port_post_acpi();

#ifdef CONFIG_SMP
	port_start_cores();
#endif

	libkern_init();
	dkit_init();
	test_fn();

#if 0 
	extern uintptr_t __stack_chk_guard;
	printk(WARN "stack chk guard: %ld\n", __stack_chk_guard);
#endif

	// XXX: maybe reuse?
	sched_exit_destroy();
}
