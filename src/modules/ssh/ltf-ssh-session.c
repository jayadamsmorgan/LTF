#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "internal_logging.h"
#include "modules/ssh/ltf-ssh-session.h"

/* ---------- CONSTRUCTOR / METHODS ---------- */

int l_module_ssh_session_init(lua_State *L) {
    l_ssh_session_t *u = lua_newuserdata(L, sizeof *u);
    u->session = libssh2_session_init();
    u->sock_fd = -1;

    if (!u->session) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "libssh2_session_init() failed");
        return 2;
    }

    luaL_getmetatable(L, SSH_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_session_handshake(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    int fd = (int)luaL_checkinteger(L, 2); // libssh2_socket_t is 'int' here

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

int l_module_ssh_session_disconnect(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    const char *st = luaL_checkstring(L, 2);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "Session disconnection failed because no actual "
                          "session was provided");
        return 2;
    }

    int rc = libssh2_session_disconnect(u->session, st);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_disconnect failed with code: %d",
                        rc);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_free(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(
            L, "Session release failed because no actual session was provided");
        return 2;
    }

    int rc = libssh2_session_free(u->session);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_free failed with code: %d", rc);
        return 2;
    }

    u->session = NULL; // to exclude double session_free procedure
    lua_pushboolean(L, 1);
    return 1;
}

/* ---------- DESTRUCTOR (GC) ---------- */

int l_session_ssh_gc(lua_State *L) {
    l_ssh_session_t *u =
        (l_ssh_session_t *)luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (u && u->session) {
        libssh2_session_free(u->session);
        u->session = NULL;
    }
    return 0;
}

/* ---------- REGISTRATION ---------- */

static const luaL_Reg session_methods[] = {
    {"handshake", l_module_ssh_session_handshake},
    {"disconnect", l_module_ssh_session_disconnect},
    {"free", l_module_ssh_session_free},
    {NULL, NULL}};

int l_module_register_ssh_session(lua_State *L) {
    LOG("Registering ltf-ssh-session");

    if (luaL_newmetatable(L, SSH_SESSION_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, session_methods, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_session_ssh_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-session");
    return 0;
}
