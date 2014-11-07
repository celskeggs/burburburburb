#ifndef CPU_H
#define CPU_H

#include "hardware.h"
#include <pthread.h>
#include <stdbool.h>
#include <lua.h>

struct cpu_ent {
	struct hw_ent *hardware;
	struct hw_sysbus *iter_head;
	struct hw_notify *interrupt;
	bool thread_exists;
	bool thread_complete;
	pthread_t thread_state;
	lua_State *L;
};

struct cpu_ent *cpu_new();
bool cpu_start(struct cpu_ent *cpu); // not threadsafe!
bool cpu_collect(struct cpu_ent *cpu, bool wait); // not threadsafe!

#endif

