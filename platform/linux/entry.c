#define _GNU_SOURCE

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ndk/cpudata.h"
#include "ndk/ndk.h"
#include "ndk/irql.h"
#include "ndk/util.h"
#include "ndk/vm.h"

// basically the number of threads spawned
#define LINUX_KCPU_COUNT 1
#if LINUX_KCPU_COUNT < 1
#define LINUXLINUX_KCPU_COUNT 1
#endif

#define PMEM_PAGECNT (MiB(8) / PAGE_SIZE)

size_t PAGE_SIZE;

static cpudata_t g_cpudatas[LINUX_KCPU_COUNT];
static __thread cpudata_port_t *t_cpudata;

void pac_putc(int ch, void *)
{
	putc_unlocked(ch, stdout);
}

void cpudata_port_setup(cpudata_port_t *cpudata)
{
	t_cpudata = cpudata;
}

cpudata_port_t *get_port_cpudata()
{
	return t_cpudata;
}

void irql_set(irql_t irql)
{
}
irql_t irql_current()
{
	return 0;
}

int main()
{
	SPINLOCK_INIT(&g_pac_lock);
	PAGE_SIZE = getpagesize();

	cpudata_setup(&g_cpudatas[0]);
	cpudata()->bsp = true;

	pac_printf("Nyy/linux (" __DATE__ " " __TIME__ ")\n");
	pac_printf("system pagesize: 0x%lx\n", PAGE_SIZE);

	int mfd = memfd_create("physical memory", 0);
	ftruncate(mfd, PAGE_SIZE * PMEM_PAGECNT);
	void *ret = mmap(0, PMEM_PAGECNT * PAGE_SIZE, PROT_NONE,
			 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (ret == MAP_FAILED) {
		perror("nyylinux hhdm");
		return EXIT_FAILURE;
	}

	REAL_HHDM_START = PADDR(ret);

	for (int i = 0; i < PMEM_PAGECNT; i++) {
		if (mmap((void *)(REAL_HHDM_START.addr + i * PAGE_SIZE),
			 PAGE_SIZE, PROT_READ | PROT_WRITE,
			 MAP_FIXED | MAP_SHARED, mfd,
			 i * PAGE_SIZE) == MAP_FAILED) {
			perror("nyylinux hhdm map");
			return EXIT_FAILURE;
		}
	}

	pm_initialize();
	pm_add_region(PADDR(0), PMEM_PAGECNT * PAGE_SIZE);
	vmstat_dump();

	return EXIT_SUCCESS;
}
