#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <sys/socket.h>

#include "internal_logging.h"

#include "modules/ssh/ltf-ssh-channel.h"
#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"

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

int l_module_ssh_socket_init(lua_State *L) { return 1; }

/*----------- registration ------------------------------------------*/

static const luaL_Reg module_fns[] = {
    {"init", l_module_ssh_lib_init},
    {"exit", l_module_ssh_lib_exit},
    {"session_init", l_module_ssh_session_init},
    {"channel_init", l_module_ssh_channel_init},
    {"socket_init", l_module_ssh_socket_init},
    {NULL, NULL},
};

static const luaL_Reg socket_fns[] = {
    {"connect", l_module_ssh_socket_connect},
    {NULL, NULL},

};

int l_module_register_ssh_socket(lua_State *L) {
    LOG("Registering ltf-ssh-socket");
    luaL_newmetatable(L, "ltf-socket");
    lua_newtable(L);
    luaL_setfuncs(L, socket_fns, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    LOG("Successfully registered ltf-ssh-socket");
    return 1;
}

int l_module_ssh_register_module(lua_State *L) {
    LOG("Registering taf-ssh module...");

    l_module_register_ssh_socket(L);
    l_module_register_ssh_session(L);

    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Successfully registered taf-ssh module.");
    return 1;
}
