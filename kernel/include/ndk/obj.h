#pragma once

#include <ndk/ndk.h>
#include <stddef.h>

typedef struct obj_header obj_header_t;
typedef struct thread thread_t;

typedef struct obj_ops {
	void (*free)(obj_header_t *hdr);
} obj_ops_t;

typedef struct obj_header {
	spinlock_t object_lock;
	size_t refcnt;
	obj_ops_t *ops;
	int type;
	int signalcount;

	int waitercount;
	TAILQ_HEAD(waitblock_list, wait_block) waitblock_list;
} obj_header_t;

enum {
	kObjTypeAnon = 0,
	kObjTypeEvent,
	kObjTypeEventNotif,
	kObjTypeTimer,
	kObjTypeMutex,
};

#define OBJ_ACQUIRE(obj) \
	spinlock_acquire_raise(&((obj_header_t *)(obj))->object_lock);

#define OBJ_ACQUIRE_ELEVATED(obj) \
	spinlock_acquire(&((obj_header_t *)(obj))->object_lock);

#define OBJ_RELEASE_ELEVATED(obj) \
	spinlock_release(&((obj_header_t *)(obj))->object_lock);

#define OBJ_RELEASE(obj, ipl) \
	spinlock_release_lower(&((obj_header_t *)(obj))->object_lock, ipl);

void obj_init(void *hdr, int type);

void obj_retain(void *hdr);
void obj_release(void *hdr);

void obj_acquire(obj_header_t *hdr, thread_t *thread);
bool obj_is_signaled(obj_header_t *hdr, thread_t *thread);
// lock held
void obj_satisfy(obj_header_t *hdr, bool all, int status);
