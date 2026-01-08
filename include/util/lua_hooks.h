#ifndef UTIL_LUA_HOOKS_H
#define UTIL_LUA_HOOKS_H

#include "util/da.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef void (*lua_hook_fn)(lua_State *L, lua_Debug *ar, const char *src);

void lua_hooks_add(int type, lua_hook_fn fn);

void lua_hooks_init(lua_State *L, da_t *file_whitelist);

void lua_hooks_deinit();

#endif // UTIL_LUA_HOOKS_H
