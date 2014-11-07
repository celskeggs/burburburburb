#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lauxlib.h>
#include <lualib.h>
#include <errno.h>

#define NEW(x) ((x*) malloc(sizeof(x)))

static int sysbusctl(lua_State *L) {
	struct cpu_ent *cpu = (struct cpu_ent *) lua_touserdata(L, lua_upvalueindex(1));
	uint16_t control = luaL_checkint(L, 1);
	if (cpu->hardware == NULL) {
		lua_pushinteger(L, 0xFFFF);
		return 1;
	}
	uint16_t intout = 0xFFFF;
	if (control >= 0x100 && control <= 0x1FF) { // exchange byte with busid
		struct hw_sysbus *bus = hw_bus_lookup(cpu->hardware, control - 0x100);
		if (bus != NULL) {
			intout = hw_exchange_bus(bus, luaL_checkint(L, 3));
		}
		lua_pushinteger(L, intout);
		return 1;
	}
	switch (control) {
	case 0: // system check
		intout = 0xCA72;
		break;
	case 1: // begin enumeration
		cpu->iter_head = cpu->hardware->sysbus_head;
		intout = cpu->iter_head != NULL;
		break;
	case 2: // continue enumeration
		if (cpu->iter_head != NULL) {
			cpu->iter_head = cpu->iter_head->next;
			intout = cpu->iter_head != NULL;
		}
		break;
	case 3: // end enumeration - optional
		cpu->iter_head = NULL;
		intout = 0x0000;
		break;
	case 4: // get typeid from enumeration
		if (cpu->iter_head != NULL) {
			intout = cpu->iter_head->typeid;
		}
		break;
	case 5: // get busid from enumeration
		if (cpu->iter_head != NULL) {
			intout = cpu->iter_head->busid;
		}
		break;
	case 6: // set busid in enumeration
		if (cpu->iter_head != NULL) {
			cpu->iter_head->busid = luaL_checkint(L, 2);
			intout = cpu->iter_head->busid;
		}
		break;
	case 127: // print
		printf("[DEBUG-IO] %s\n", luaL_checkstring(L, 2));
		intout = 0x0000;
		break;
	}
	lua_pushinteger(L, intout);
	return 1;
}

// Some code in this file based off of lauxlib.c

static void *allocator (void *ud, void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

static int panic (lua_State *L) {
	fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));
	return 0;
}

typedef struct LoadF {
	FILE *f;  /* file being read */
	char buff[8192];  /* area for reading file */
} LoadF;

static const char *getF (lua_State *L, void *ud, size_t *size) {
	LoadF *lf = (LoadF *)ud;
	/* 'fread' can return > 0 *and* set the EOF flag. If next call to 'getF' called 'fread', it might still wait for user input. The next check avoids this problem. */
	if (feof(lf->f)) {
		return NULL;
	}
	*size = fread(lf->buff, 1, sizeof(lf->buff), lf->f); /* read block */
	return lf->buff;
}

static int loadfile(lua_State *L, const char *filename) {
	LoadF lf;
	lf.f = fopen(filename, "r");
	if (lf.f == NULL) {
		lua_pushstring(L, strerror(errno));
		return 1;
	}
	int status = lua_load(L, getF, &lf, filename, "t");
	if (ferror(lf.f)) {
		lua_pushstring(L, strerror(errno));
		status = 1;
	}
	fclose(lf.f);
	return status;
}

struct cpu_ent *cpu_new() {
	struct cpu_ent *out = NEW(struct cpu_ent);
	out->hardware = NULL;
	out->iter_head = NULL;
	out->thread_exists = false;
	out->L = lua_newstate(allocator, NULL);
	if (out->L == NULL) {
		fputs("PANIC: Not enough memory.\n", stderr);
		return NULL;
	}
	lua_atpanic(out->L, &panic);
	luaL_requiref(out->L, "_G", luaopen_base, 1);
	lua_pop(out->L, 1);
	luaL_requiref(out->L, LUA_COLIBNAME, luaopen_coroutine, 1);
	lua_pop(out->L, 1);
	luaL_requiref(out->L, LUA_STRLIBNAME, luaopen_string, 1);
	lua_pop(out->L, 1);
	luaL_requiref(out->L, LUA_TABLIBNAME, luaopen_table, 1);
	lua_pop(out->L, 1);
	luaL_requiref(out->L, LUA_MATHLIBNAME, luaopen_math, 1);
	lua_pop(out->L, 1);
	luaL_requiref(out->L, LUA_BITLIBNAME, luaopen_bit32, 1);
	lua_pop(out->L, 1);
	lua_pushlightuserdata(out->L, out);
	lua_pushcclosure(out->L, sysbusctl, 1);
	lua_setglobal(out->L, "sysbusctl");
	int status = loadfile(out->L, "bios.lua");
	if (status) {
		fprintf(stderr, "PANIC: Failed to load bios.lua: %s\n", lua_tostring(out->L, -1));
		lua_close(out->L);
		free(out);
		return NULL;
	}
	out->hardware = hw_new();
	return out;
}

static void *cpu_main(void *arg) {
	struct cpu_ent *cpu = (struct cpu_ent *) arg;
	int status = lua_pcall(cpu->L, 0, 0, 0);
	cpu->thread_complete = true;
	if (status) {
		fprintf(stderr, "PANIC: Failed to run bios.lua: %s\n", lua_tostring(cpu->L, -1));
	}
	return NULL;
}

// not threadsafe!
bool cpu_start(struct cpu_ent *cpu) {
	if (cpu->thread_exists) {
		fputs("PANIC: Thread exists!\n", stderr);
		return false;
	}
	cpu->thread_complete = false;
	cpu->thread_exists = true;
	int status = pthread_create(&cpu->thread_state, NULL, cpu_main, cpu);
	if (status != 0) {
		errno = status;
		perror("starting lua worker thread");
		cpu->thread_exists = false;
		return false;
	}
	return true;
}

// not threadsafe!
bool cpu_collect(struct cpu_ent *cpu, bool wait) {
	if (!cpu->thread_exists) {
		return true;
	}
	if (!cpu->thread_complete && !wait) {
		return false;
	}
	int status = pthread_join(cpu->thread_state, NULL);
	cpu->thread_exists = false;
	if (status != 0) {
		errno = status;
		perror("joining lua worker thread");
	}
	return true;
}

