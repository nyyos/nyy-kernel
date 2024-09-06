#pragma once

#include <stdint.h>
#include <ndk/port.h>
#include <ndk/util.h>
#include <ndk/dpc.h>
#include <ndk/sched.h>
#include <ndk/time.h>

typedef struct cpudata {
	cpudata_port_t port_data;
	uint64_t softint_pending;
	dpc_queue_t dpc_queue;
	bool bsp;

	scheduler_t scheduler;
	dpc_t done_queue_dpc;
	thread_t idle_thread;
	thread_t *thread_current;
	thread_t *thread_next;

	timer_engine_t timer_engine;
	uint64_t next_deadline;
	dpc_t timer_update;
} cpudata_t;

void cpudata_setup(cpudata_t *cpudata);
void cpudata_port_setup(cpudata_port_t *cpudata);
cpudata_port_t *port_get_cpudata();

static inline cpudata_t *cpudata()
{
	return (cpudata_t *)port_get_cpudata();
}
