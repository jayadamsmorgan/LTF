#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "internal_logging.h"

#include "modules/ssh/ltf-ssh-session.h"

int l_module_ssh_session_init(lua_State *L) {
    l_ssh_session_t *u = lua_newuserdata(L, sizeof *u);
    u->session = libssh2_session_init();
    u->sock_fd = -1;

    if (!u->session) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "libssh2_session_init() failed");
    }
    luaL_getmetatable(L, "ssh-session");
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_session_handshake(lua_State *L) {

    l_ssh_session_t *u = luaL_checkudata(L, 1, "session");

    int fd = (int)luaL_checkinteger(L, 2); //  libssh2_socket_t have int type

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "Handshake failed because session was not initialized");
        return 2;
    }
    if (fd == -1) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "Handshake failed because socket was not initialized");
        return 2;
    }

    int rc = libssh2_session_handshake(u->session, fd);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_handshake failed with code: %d",
                        rc);
        return 2;
    }

    u->sock_fd = fd;
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_disconnect(lua_State *L) {}

int l_module_ssh_session_free(lua_State *L) {}

static const luaL_Reg session_fns[] = {
    {"init", l_module_ssh_session_init},
    {"handshake", l_module_ssh_session_handshake},
    {"disconnect", l_module_ssh_session_disconnect},
    {"free", l_module_ssh_session_free},
    {NULL, NULL},
};

int l_module_register_ssh_session(lua_State *L) {
    LOG("Registering ltf-ssh-session");
    lua_newtable(L);
    luaL_setfuncs(L, session_fns, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    LOG("Successfully registered ltf-ssh-session");
    return 1;
}
