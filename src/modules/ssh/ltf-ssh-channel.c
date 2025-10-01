#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-channel.h"

int l_module_ssh_channel_open_session(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!s->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_open_session() failed because "
                          "session was not initialized");
        return 2;
    }

    l_ssh_channel_t *u = lua_newuserdata(L, sizeof *u);
    u->channel = libssh2_channel_open_session(s->session);
    u->session = s->session;

    luaL_getmetatable(L, SSH_CHANNEL_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_channel_request_pty(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_request_pty() failed because "
                          "channel is not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_request_pty() failed because "
                          "session is not exist");
        return 2;
    }
    const char *s = luaL_checkstring(L, 2);

    int rc = libssh2_channel_request_pty(u->channel, s);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(
            L, "l_module_ssh_channel_request_pty() failed with code: %d", rc);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_exec(lua_State *L) {}

int l_module_ssh_channel_shell(lua_State *L) {}

int l_module_ssh_channel_write(lua_State *L) {}

int l_module_ssh_channel_read(lua_State *L) {}

int l_module_ssh_channel_close(lua_State *L) {}

int l_module_ssh_channel_free(lua_State *L) {}
