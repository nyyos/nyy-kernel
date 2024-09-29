#include <uacpi/types.h>
#include <uacpi/platform/arch_helpers.h>
#include <uacpi/kernel_api.h>
#include <ndk/mutex.h>
#include <ndk/sched.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>
#include <ndk/time.h>
#include <ndk/vm.h>

#ifdef AMD64
#include "asm.h"
#endif

#define STUB panic("UACPI kernel API stub");

uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address,
					  uacpi_u8 width, uacpi_u64 *out)
{
	void *ptr = uacpi_kernel_map(address, width);
	switch (width) {
	case 1:
		*out = *(volatile uint8_t *)ptr;
		break;
	case 2:
		*out = *(volatile uint16_t *)ptr;
		break;
	case 4:
		*out = *(volatile uint32_t *)ptr;
		break;
	case 8:
		*out = *(volatile uint64_t *)ptr;
		break;
	default:
		uacpi_kernel_unmap(ptr, width);
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address,
					   uacpi_u8 width, uacpi_u64 in)
{
	void *ptr = uacpi_kernel_map(address, width);

	switch (width) {
	case 1:
		*(volatile uint8_t *)ptr = in;
		break;
	case 2:
		*(volatile uint16_t *)ptr = in;
		break;
	case 4:
		*(volatile uint32_t *)ptr = in;
		break;
	case 8:
		*(volatile uint64_t *)ptr = in;
		break;
	default:
		uacpi_kernel_unmap(ptr, width);
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	uacpi_kernel_unmap(ptr, width);
	return UACPI_STATUS_OK;
}

#ifdef AMD64
uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr addr, uacpi_u8 width,
				      uacpi_u64 *out)
{
	switch (width) {
	case 1:
		*out = inb(addr);
		break;
	case 2:
		*out = inw(addr);
		break;
	case 4:
		*out = inl(addr);
		break;
	default:
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr addr, uacpi_u8 width,
				       uacpi_u64 value)
{
	switch (width) {
	case 1:
		outb(addr, value);
		break;
	case 2:
		outw(addr, value);
		break;
	case 4:
		outl(addr, value);
		break;
	default:
		return UACPI_STATUS_INVALID_ARGUMENT;
	}

	return UACPI_STATUS_OK;
}
#else
uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr addr, uacpi_u8 width,
				      uacpi_u64 *out)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr addr, uacpi_u8 width,
				       uacpi_u64 value)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
#endif

uacpi_status uacpi_kernel_pci_read(uacpi_pci_address *address,
				   uacpi_size offset, uacpi_u8 byte_width,
				   uacpi_u64 *value)
{
	STUB;
}

uacpi_status uacpi_kernel_pci_write(uacpi_pci_address *address,
				    uacpi_size offset, uacpi_u8 byte_width,
				    uacpi_u64 value)
{
	STUB;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
				 uacpi_handle *outhandle)
{
	*outhandle = (uacpi_handle)base;
	return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
	(void)handle;
}

uacpi_status uacpi_kernel_io_read(uacpi_handle handle, uacpi_size offset,
				  uacpi_u8 width, uacpi_u64 *out)
{
	return uacpi_kernel_raw_io_read((uacpi_io_addr)handle + offset, width,
					out);
}

uacpi_status uacpi_kernel_io_write(uacpi_handle handle, uacpi_size offset,
				   uacpi_u8 width, uacpi_u64 value)
{
	return uacpi_kernel_raw_io_write((uacpi_io_addr)handle + offset, width,
					 value);
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
	size_t offset = addr % PAGE_SIZE;
	len = PAGE_ALIGN_UP(len + offset);
	addr = PAGE_ALIGN_DOWN(addr);
	uintptr_t vaddr = (uintptr_t)vm_map_allocate(vm_kmap(), len);
	for (int i = 0; i < len / PAGE_SIZE; i++) {
		vm_port_map(vm_kmap(), PADDR(addr + i * PAGE_SIZE),
			    VADDR(vaddr + i * PAGE_SIZE), kVmUncached, kVmAll);
	}
	return (void *)vaddr + offset;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len)
{
	return;
}

void *uacpi_kernel_alloc(uacpi_size size)
{
	return kmalloc(size);
}

void *uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
	return kcalloc(count, size);
}

void uacpi_kernel_free(void *mem, uacpi_size size_hint)
{
	kfree(mem, size_hint);
}

void uacpi_kernel_log(uacpi_log_level lvl, const uacpi_char *str)
{
	switch (lvl) {
	case UACPI_LOG_DEBUG:
		printk(DEBUG "acpi: %s", str);
		break;
	case UACPI_LOG_TRACE:
		printk(TRACE "acpi: %s", str);
		break;
	case UACPI_LOG_INFO:
		printk(INFO "acpi: %s", str);
		break;
	case UACPI_LOG_WARN:
		printk(WARN "acpi: %s", str);
		break;
	case UACPI_LOG_ERROR:
		printk(ERR "acpi: %s", str);
		break;
	default:
		printk("[<invalid>] acpi: %s", str);
	}
}

uacpi_u64 uacpi_kernel_get_ticks(void)
{
	return clocksource()->current_nanos() / 100;
}

void uacpi_kernel_stall(uacpi_u8 usec)
{
	time_delay(US2NS(usec));
}

void uacpi_kernel_sleep(uacpi_u64 msec)
{
	sched_sleep(MS2NS(msec));
}

uacpi_handle uacpi_kernel_create_mutex(void)
{
	mutex_t *mutex = kmalloc(sizeof(mutex_t));
	if (mutex == nullptr)
		return nullptr;
	mutex_init(mutex);
	return mutex;
}

void uacpi_kernel_free_mutex(uacpi_handle mutex)
{
	kfree(mutex, sizeof(mutex_t));
}

uacpi_handle uacpi_kernel_create_event(void)
{
	STUB;
}

void uacpi_kernel_free_event(uacpi_handle)
{
	STUB;
}

uacpi_thread_id uacpi_kernel_get_thread_id(void)
{
	return curthread();
}

uacpi_bool uacpi_kernel_acquire_mutex(uacpi_handle mutex, uacpi_u16 timeout)
{
	if (timeout == 0xFFFF) {
		// inifnite wait
		mutex_acquire(mutex);
		return UACPI_TRUE;
	}
	STUB;
}

void uacpi_kernel_release_mutex(uacpi_handle mutex)
{
	mutex_release(mutex);
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16)
{
	STUB;
}

void uacpi_kernel_signal_event(uacpi_handle)
{
	STUB;
}

void uacpi_kernel_reset_event(uacpi_handle)
{
	STUB;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *)
{
	STUB;
}

uacpi_status
uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler,
				       uacpi_handle ctx,
				       uacpi_handle *out_irq_handle)
{
	STUB;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler,
						      uacpi_handle irq_handle)
{
	STUB;
}

uacpi_handle uacpi_kernel_create_spinlock(void)
{
	spinlock_t *lock = kmalloc(sizeof(spinlock_t));
	if (lock == nullptr)
		return nullptr;

	SPINLOCK_INIT(lock);
	return lock;
}
void uacpi_kernel_free_spinlock(uacpi_handle lock)
{
	kfree(lock, sizeof(spinlock_t));
}

uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle lock)
{
	return spinlock_acquire_intr(lock);
}
void uacpi_kernel_spinlock_unlock(uacpi_handle lock, uacpi_cpu_flags old)
{
	spinlock_release_intr(lock, old);
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type, uacpi_work_handler,
					uacpi_handle ctx)
{
	STUB;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void)
{
	STUB;
}
