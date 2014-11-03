#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <errno.h>

static int sysbusctl(lua_State *L) {
	const char *control = luaL_checkstring(L, 1);
	if (strcmp(control, "device-query") == 0) {
		puts("Query command.");
	} else {
		luaL_error(L, "bad sysbusctl command: %s", control);
	}
	return 0;
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
    /* 'fread' can return > 0 *and* set the EOF flag. If next call to
       'getF' called 'fread', it might still wait for user input.
       The next check avoids this problem. */
	if (feof(lf->f)) {
		return NULL;
	}
	*size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);  /* read block */
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

int main() {
	lua_State *L = lua_newstate(allocator, NULL);
	if (L == NULL) {
		fputs("PANIC: Not enough memory.", stderr);
		return 1;
	}
	lua_atpanic(L, &panic);
	luaL_requiref(L, "_G", luaopen_base, 1);
	lua_pop(L, 1);
	luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
	lua_pop(L, 1);
	luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
	lua_pop(L, 1);
	luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
	lua_pop(L, 1);
	luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
	lua_pop(L, 1);
	luaL_requiref(L, LUA_BITLIBNAME, luaopen_bit32, 1);
	lua_pop(L, 1);
	lua_register(L, "sysbusctl", sysbusctl);
	int status = loadfile(L, "bios.lua");
	if (status) {
		fprintf(stderr, "PANIC: Failed to load bios.lua: %s\n", lua_tostring(L, -1));
		lua_close(L);
		return 1;
	}
	status = lua_pcall(L, 0, 0, 0);
	if (status) {
		fprintf(stderr, "PANIC: Failed to run bios.lua: %s\n", lua_tostring(L, -1));
		lua_close(L);
		return 1;
	}
	lua_close(L);
	return 0;
}
