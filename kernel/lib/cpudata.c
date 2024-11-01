#include <ndk/cpudata.h>
#include <ndk/kmem.h>

static int cap = 0;
static spinlock_t list_lock = SPINLOCK_INITIALIZER();
cpudata_t **g_cpudatalist = nullptr;

void cpudata_set(int idx, cpudata_t *data)
{
	int old = spinlock_acquire_intr(&list_lock);
	if (cap < idx + 1) {
		cpudata_t **newlist = kmalloc(sizeof(cpudata_t *) * (idx + 1));
		for (int i = 0; i < cap; i++) {
			newlist[i] = g_cpudatalist[i];
		}
		g_cpudatalist = newlist;
		cap = idx + 1;
	}
	g_cpudatalist[idx] = data;
	spinlock_release_intr(&list_lock, old);
}
