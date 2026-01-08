#include "util/lua_hooks.h"

#include "internal_logging.h"

#include "util/da.h"
#include "util/string.h"

static da_t *lua_call_hooks = NULL;
static da_t *lua_ret_hooks = NULL;
static da_t *lua_line_hooks = NULL;

static da_t *_file_whitelist = NULL;

static void master_hook(lua_State *L, lua_Debug *ar) {
    if (!lua_getinfo(L, "nSl", ar)) {
        return;
    }
    const char *src = ar->source;
    if (src[0] == '@')
        src++;

    size_t whitelist_count = da_size(_file_whitelist);
    bool found = false;
    for (size_t i = 0; i < whitelist_count; ++i) {
        char **file = da_get(_file_whitelist, i);
        if (file && *file && string_has_prefix(src, *file)) {
            found = true;
            break;
        }
    }
    if (!found)
        return;

    da_t *hooks;

    switch (ar->event) {
    case LUA_HOOKCALL: {
        hooks = lua_call_hooks;
        break;
    }
    case LUA_HOOKRET: {
        hooks = lua_ret_hooks;
        break;
    }
    case LUA_HOOKLINE: {
        hooks = lua_line_hooks;
        break;
    }
    default:
        return;
    }
    size_t size = da_size(hooks);
    for (size_t i = 0; i < size; ++i) {
        lua_hook_fn *fn = da_get(hooks, i);
        if (fn && *fn) {
            (*fn)(L, ar, src);
        }
    }
}

void lua_hooks_add(int type, lua_hook_fn fn) {
    switch (type) {
    case LUA_HOOKCALL: {
        da_append(lua_call_hooks, &fn);
        break;
    }
    case LUA_HOOKRET: {
        da_append(lua_ret_hooks, &fn);
        break;
    }
    case LUA_HOOKLINE: {
        da_append(lua_line_hooks, &fn);
        break;
    }
    default: {
        LOG("Unknown hook type %d, cannot register.", type);
        return;
    }
    }
}

void lua_hooks_init(lua_State *L, da_t *file_whitelist) {
    lua_call_hooks = da_init(1, sizeof(lua_hook_fn));
    lua_ret_hooks = da_init(1, sizeof(lua_hook_fn));
    lua_line_hooks = da_init(1, sizeof(lua_hook_fn));

    _file_whitelist = file_whitelist;

    lua_sethook(L, master_hook, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
}

void lua_hooks_deinit() {
    da_free(lua_call_hooks);
    da_free(lua_ret_hooks);
    da_free(lua_line_hooks);
}
