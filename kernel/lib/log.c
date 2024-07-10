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
#include <stdatomic.h>
#include <string.h>

// XXX: maybe move this out from here...
static TAILQ_HEAD(console_list,
		  console) console_list = TAILQ_HEAD_INITIALIZER(console_list);
static spinlock_t console_list_lock = SPINLOCK_INITIALIZER();

spinlock_t printk_lock = SPINLOCK_INITIALIZER();

#define LOG_DEBUG "[ \x1b[35mDEBUG \x1b[0m] "
#define LOG_INFO "[ \x1b[32mINFO  \x1b[0m] "
#define LOG_WARN "[ \x1b[33mWARN  \x1b[0m] "
#define LOG_ERR "[ \x1b[31mERROR \x1b[0m] "
#define LOG_PANIC "[ \x1b[31mPANIC \x1b[0m] "

#define MSG_BUF_SIZE 256
// has to align to ^2!
#define MSGS_SIZE 1024

typedef struct log_message {
	size_t msg_sz;
	uint32_t seq;
	char buf[MSG_BUF_SIZE];
} logmessage_t;

typedef struct log_buffer {
	logmessage_t messages[MSGS_SIZE];
	volatile atomic_uint head;
	volatile atomic_uint tail;
	volatile atomic_uint seq;
} logbuffer_t;

void logbuffer_init(logbuffer_t *lb)
{
	lb->head = 0;
	lb->tail = 0;
	lb->seq = 0;
	memset(lb->messages, 0x0, sizeof(logmessage_t) * MSGS_SIZE);
}

void logbuffer_write(logbuffer_t *lb, const char *str, size_t size)
{
	uint32_t head = atomic_load(&lb->head);
	uint32_t next = (head + 1) & (MSGS_SIZE - 1);
	uint32_t seq = atomic_fetch_add(&lb->seq, 1);

	// XXX: do something on log buffer full?
	if (next == atomic_load(&lb->tail))
		return;

	logmessage_t *msg = &lb->messages[head];
	// XXX: size check
	memcpy(msg->buf, str, size);
	msg->buf[size] = '\0';
	msg->msg_sz = size;
	msg->seq = seq;
	atomic_store(&lb->head, next);
}

logmessage_t *logbuffer_read(logbuffer_t *lb)
{
	uint32_t tail = atomic_load(&lb->tail);

	if (atomic_load(&lb->head) == tail) {
		return NULL;
	}

	logmessage_t *res = &lb->messages[tail];
	atomic_store(&lb->tail, (tail + 1) & (MSGS_SIZE - 1));
	return res;
}

static logbuffer_t g_lb;

void _printk_init()
{
	logbuffer_init(&g_lb);
}

void _printk_consoles_write(const char *buf, size_t size)
{
	console_t *elm = nullptr;
	TAILQ_FOREACH(elm, &console_list, queue_entry)
	{
		if (!elm->write)
			continue;
		irql_t irql = spinlock_acquire(&elm->spinlock, HIGH_LEVEL);
		elm->write(elm, buf, size);
		spinlock_release(&elm->spinlock, irql);
	}
}

static void _printk_buffer_write(logbuffer_t *lb)
{
	logmessage_t *msg;
	while ((msg = logbuffer_read(lb)) != nullptr) {
		_printk_consoles_write(msg->buf, msg->msg_sz);
	}
}

void printk(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	size_t size;
	char buf[MSG_BUF_SIZE + 1];
	buf[MSG_BUF_SIZE] = '\0';
	int lvl = *fmt;

	// get log level
	switch (lvl) {
	// info
	case 1:
		fmt++;
		size = npf_snprintf(buf, MSG_BUF_SIZE, LOG_INFO);
		break;
	// warn
	case 2:
		fmt++;
		size = npf_snprintf(buf, MSG_BUF_SIZE, LOG_WARN);
		break;
	// error
	case 3:
		fmt++;
		size = npf_snprintf(buf, MSG_BUF_SIZE, LOG_ERR);
		break;
	// debug
	case 4:
		fmt++;
		size = npf_snprintf(buf, MSG_BUF_SIZE, LOG_DEBUG);
		break;
	// panic
	case 5:
		fmt++;
		size = npf_snprintf(buf, MSG_BUF_SIZE, LOG_PANIC);
		break;
	default:
		size = 0;
	}

	size += npf_vsnprintf(buf + size, MSG_BUF_SIZE - size, fmt, args);
	logbuffer_write(&g_lb, buf, size);

	va_end(args);

	_printk_buffer_write(&g_lb);

	if (lvl == 5) {
		panic();
	}
}

void console_add(console_t *console)
{
	irql_t irql = spinlock_acquire(&console_list_lock, HIGH_LEVEL);
	SPINLOCK_INIT(&console->spinlock);
	// TODO: print old messages
	TAILQ_INSERT_TAIL(&console_list, console, queue_entry);
	spinlock_release(&console_list_lock, irql);
}

void console_remove(console_t *console)
{
	irql_t irql = spinlock_acquire(&console_list_lock, HIGH_LEVEL);
	spinlock_acquire(&console->spinlock, HIGH_LEVEL);
	TAILQ_REMOVE(&console_list, console, queue_entry);
	spinlock_release(&console_list_lock, irql);
}
