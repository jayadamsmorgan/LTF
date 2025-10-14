#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <sys/socket.h>

#include "internal_logging.h"

#include "modules/ssh/ltf-ssh-channel.h"
#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"
#include "modules/ssh/ltf-ssh-userauth.h"

/******************* API ***********************/

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

/******************* DESTRUCTORS ***********************/

static int l_module_ssh_gc(lua_State *L) {
    LOG("Invoked ltf-ssh-lib GC (module finalizer)");
    libssh2_exit();
    return 0;
}

/******************* REGISTRATION ***********************/

static const luaL_Reg module_fns[] = {
    {"lib_init", l_module_ssh_lib_init},
    {"session_init", l_module_ssh_session_init},
    {"socket_init", l_module_ssh_socket_init},
    {"socket_free", l_module_ssh_socket_free},
    {"channel_init", l_module_ssh_channel_open_session},
    {NULL, NULL},
};

static const luaL_Reg libmod_fns[] = {
    {"lib_exit", l_module_ssh_lib_exit},
    {NULL, NULL},
};

int l_module_register_ssh_libmod(lua_State *L) {
    LOG("Registering ltf-ssh-libmod");

    if (luaL_newmetatable(L, SSH_LIBMOD_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, libmod_fns, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_module_ssh_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);
    LOG("Successfully registered ltf-ssh-libmod");
    return 0;
}

int l_module_ssh_register_module(lua_State *L) {
    LOG("Registering ltf-ssh module...");

    l_module_register_ssh_libmod(L);
    l_module_register_ssh_session(L);
    l_module_register_ssh_channel(L);
    // l_module_register_ssh_scp(L);
    // l_module_register_ssh_sftp(L);

    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);

    void **ud = (void **)lua_newuserdata(L, sizeof(void *));
    *ud = NULL;
    luaL_getmetatable(L, SSH_LIBMOD_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "_finalizer");

    LOG("Successfully registered ltf-ssh module.");
    return 1;
}
