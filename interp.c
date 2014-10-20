#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int sysbusctl(lua_State *L) {
	const char *control = luaL_checkstring(L, 1);
	if (strcmp(control, "device-query") == 0) {
		puts("Query command.");
	} else {
		luaL_error(L, "bad sysbusctl command: %s", control);
	}
	return 0;
}

int main() {
	lua_State *L = luaL_newstate();
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
	int status = luaL_loadfile(L, "sandbox.lua");
	if (status) {
		fprintf(stderr, "Failed to load: %s\n", lua_tostring(L, -1));
		return 1;
	}
	status = lua_pcall(L, 0, 0, 0);
	if (status) {
		fprintf(stderr, "Failed to run: %s\n", lua_tostring(L, -1));
		return 1;
	}
	lua_close(L);
	return 0;
}
