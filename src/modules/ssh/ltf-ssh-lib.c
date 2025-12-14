#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdatomic.h>
#include <sys/socket.h>

#include "internal_logging.h"

#include "modules/ssh/ltf-ssh-channel.h"
#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"
#include "modules/ssh/ltf-ssh-sftp.h"

static atomic_bool lib_ssh_inited = false;

static const char *unknown_err_output = "UNKNOWN";

const char *ssh_err_to_str(int code) {
    for (int i = 0; ssh_error_map[i].name != NULL; ++i) {
        if (ssh_error_map[i].code == code)
            return ssh_error_map[i].name;
    }
    return unknown_err_output;
}

/******************* API ***********************/

int l_module_ssh_lib_init(lua_State *L) {
    if (lib_ssh_inited) {
        return 0;
    }

    int rc = libssh2_init(0); // 0 = default flags
    if (rc) {
        lua_pushfstring(L, "libssh2_init failed with code: %s",
                        ssh_err_to_str(rc));
        return 1;
    }

    lib_ssh_inited = true;
    return 0;
}

int l_module_ssh_lib_exit(lua_State *) {
    if (!lib_ssh_inited) {
        return 0;
    }

    libssh2_exit();
    lib_ssh_inited = false;
    return 0;
}

/******************* DESTRUCTORS ***********************/

static int l_module_ssh_gc(lua_State *L) {
    LOG("Invoked ltf-ssh-lib GC (module finalizer)");
    return l_module_ssh_lib_exit(L);
}

/******************* REGISTRATION ***********************/

static const luaL_Reg module_fns[] = {
    {"lib_init", l_module_ssh_lib_init},
    {"session_init_userpass", l_module_ssh_session_init_userpass},
    {"channel_init", l_module_ssh_channel_open_session},
    {"sftp_init", l_module_ssh_sftp_init},
    {"lib_exit", l_module_ssh_lib_exit},
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
    l_module_register_ssh_sftp(L);

    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);

    void **ud = (void **)lua_newuserdata(L, sizeof(void *));
    *ud = NULL;
    luaL_getmetatable(L, SSH_LIB_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "_finalizer");

    LOG("Successfully registered ltf-ssh module.");
    return 1;
}
