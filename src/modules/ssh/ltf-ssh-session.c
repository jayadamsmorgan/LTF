#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/taf-ssh-session.h"

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

    l_ssh_session_t *u = check_ssh_session(L, 1);

    int fd = (int)luaL_checkinteger(L, 2);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "Handshake failed because session was not initialized");
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
