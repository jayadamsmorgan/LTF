#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-lib.h"

int l_module_ssh_lib_init(lua_State *L) {
    int rc = libssh2_init(0); // 0 = default flags
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_init failed with code: %d", rc);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_lib_exit(lua_State *L) {
    libssh2_exit();
    lua_pushboolean(L, 1);
    return 1;
}

/*----------- registration ------------------------------------------*/

static const luaL_Reg module_fns[] = {
    {"init", l_module_ssh_lib_init},
    {"exit", l_module_ssh_lib_exit},
    {NULL, NULL},
};

int l_module_serial_register_module(lua_State *L) {
    LOG("Registering taf-ssh module...");
    LOG("Registering taf-ssh-lib")
    LOG("Registering taf-ssh-session")

    LOG("Successfully registered taf-ssh module.");
    return 1;
}
