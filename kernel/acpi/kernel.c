#include <uacpi/types.h>
#include <uacpi/platform/arch_helpers.h>
#include <uacpi/kernel_api.h>

#include <ndk/ndk.h>
#include <ndk/kmem.h>

#define STUB panic("UACPI kernel API stub");

uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address,
					  uacpi_u8 byte_width,
					  uacpi_u64 *out_value)
{
	STUB;
}

uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address,
					   uacpi_u8 byte_width,
					   uacpi_u64 in_value)
{
	STUB;
}

uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address,
				      uacpi_u8 byte_width, uacpi_u64 *out_value)
{
	STUB;
}

uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address,
				       uacpi_u8 byte_width, uacpi_u64 in_value)
{
	STUB;
}

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
				 uacpi_handle *out_handle)
{
	STUB;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
	STUB;
}

uacpi_status uacpi_kernel_io_read(uacpi_handle, uacpi_size offset,
				  uacpi_u8 byte_width, uacpi_u64 *value)
{
	STUB;
}

uacpi_status uacpi_kernel_io_write(uacpi_handle, uacpi_size offset,
				   uacpi_u8 byte_width, uacpi_u64 value)
{
	STUB;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
	STUB;
}
void uacpi_kernel_unmap(void *addr, uacpi_size len)
{
	STUB;
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

void uacpi_kernel_log(uacpi_log_level, const uacpi_char *)
{
	STUB;
}

uacpi_u64 uacpi_kernel_get_ticks(void)
{
	STUB;
}

void uacpi_kernel_stall(uacpi_u8 usec)
{
	STUB;
}

void uacpi_kernel_sleep(uacpi_u64 msec)
{
	STUB;
}

uacpi_handle uacpi_kernel_create_mutex(void)
{
	STUB;
}

void uacpi_kernel_free_mutex(uacpi_handle)
{
	STUB;
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
	STUB;
}

uacpi_bool uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16)
{
	STUB;
}
void uacpi_kernel_release_mutex(uacpi_handle)
{
	STUB;
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
	STUB;
}
void uacpi_kernel_free_spinlock(uacpi_handle)
{
	STUB;
}

uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle)
{
	STUB;
}
void uacpi_kernel_spinlock_unlock(uacpi_handle, uacpi_cpu_flags)
{
	STUB;
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
