#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#include <nanoprintf.h>

#include <ndk/ndk.h>
#include <dkit/console.h>
#include <sys/queue.h>
#include <stdarg.h>

static TAILQ_HEAD(console_list,
		  console) console_list = TAILQ_HEAD_INITIALIZER(console_list);
static spinlock_t console_list_lock = SPINLOCK_INITIALIZER();
spinlock_t printk_lock = SPINLOCK_INITIALIZER();

// TODO: could print previous log output here
void console_add(console_t *console)
{
	irql_t irql = spinlock_acquire(&console_list_lock, HIGH_LEVEL);
	TAILQ_INSERT_TAIL(&console_list, console, queue_entry);
	spinlock_release(&console_list_lock, irql);
}

void console_remove(console_t *console)
{
	irql_t irql = spinlock_acquire(&console_list_lock, HIGH_LEVEL);
	TAILQ_REMOVE(&console_list, console, queue_entry);
	spinlock_release(&console_list_lock, irql);
}

void _printk_consoles_write(const char *buf, size_t size)
{
	console_t *elm = nullptr;
	TAILQ_FOREACH(elm, &console_list, queue_entry)
	{
		if (!elm->write)
			continue;
		elm->write(elm, buf, size);
	}
}

#define PRINTK_BUF 256
#define LOG_DEBUG "[ \x1b[35mDEBUG \x1b[0m] "
#define LOG_INFO "[ \x1b[32mINFO  \x1b[0m] "
#define LOG_WARN "[ \x1b[33mWARN  \x1b[0m] "
#define LOG_ERR "[ \x1b[31mERROR \x1b[0m] "
#define LOG_PANIC "[ \x1b[31mPANIC \x1b[0m] "
void printk_locked(const char *msg, ...)
{
	char buf[PRINTK_BUF];
	size_t size;
	int lvl = *msg;

	va_list argp;
	va_start(argp, msg);

	// get log level
	switch (lvl) {
	// info
	case 1:
		msg++;
		size = npf_snprintf(buf, PRINTK_BUF, LOG_INFO);
		break;
	// warn
	case 2:
		msg++;
		size = npf_snprintf(buf, PRINTK_BUF, LOG_WARN);
		break;
	// error
	case 3:
		msg++;
		size = npf_snprintf(buf, PRINTK_BUF, LOG_ERR);
		break;
	// debug
	case 4:
		msg++;
		size = npf_snprintf(buf, PRINTK_BUF, LOG_DEBUG);
		break;
	// panic
	case 5:
		msg++;
		size = npf_snprintf(buf, PRINTK_BUF, LOG_PANIC);
		break;
	default:
		size = 0;
	}

	size += npf_vsnprintf(buf + size, PRINTK_BUF - size, msg, argp);

	va_end(argp);

	_printk_consoles_write(buf, size);

	if (lvl == 5) {
		panic();
	}
}
