#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int sysbusctl(lua_State *L) {
	puts("Hello, World!");
	return 0;
}

/*int load_bios_string(lua_State *L, char *fname) {
	FILE *target = fopen(fname, "r");
	if (target == NULL) {
		perror(fname);
		return 1;
	}
	fseek(target, 0, SEEK_END);
	long size = ftell(target);
	fseek(target, 0, SEEK_SET);
	char *buffer = (char *) malloc(size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate buffer to hold %d for %s.\n", size + 1, fname);
		free(buffer);
		fclose(target); // ignore additional errors
		return 1;
	}
	if (size != fread(buffer, sizeof(char), size, target)) {
		perror(fname);
		free(buffer);
		fclose(target); // ignore additional errors
		return 1;
	}
	if (fclose(target) != 0) {
		perror(fname);
		free(buffer);
		return 1;
	}
	buffer[size] = '\0';
	lua_pushstring(L, buffer);
	free(buffer);
	return 0;
}*/

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
	/*status = load_bios_string(L, "bios.lua");
	if (status) {
		return 1;
	}
	status = lua_pcall(L, 1, 0, 0);*/
	status = lua_pcall(L, 0, 0, 0);
	if (status) {
		fprintf(stderr, "Failed to run: %s\n", lua_tostring(L, -1));
		return 1;
	}
	lua_close(L);
	return 0;
}
