#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-scp.h"

#include "modules/ssh/ltf-ssh-channel.h"
#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"

int l_module_ssh_scp_recv2(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!s->session) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_scp_recv2() failed because "
                          "session was not initialized");
        return 3;
    }

    const char *path = luaL_checkstring(L, 2);

    l_ssh_channel_t *u = lua_newuserdata(L, sizeof *u);

    libssh2_struct_stat fileinfo;
    memset(&fileinfo, 0, sizeof(fileinfo));

    u->channel = libssh2_scp_recv2(s->session, path, &fileinfo);
    u->session = s->session;

    if (!u->channel) {
        int rc = libssh2_session_last_error(u->session, NULL, NULL, 0);
        u->session = NULL;
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_scp_recv2 failedwith code: %s",
                        ssh_err_to_str(rc));

        return 3;
    }
    luaL_getmetatable(L, SSH_CHANNEL_MT);
    lua_setmetatable(L, -2);

    lua_newtable(L);
    lua_pushinteger(L, (lua_Integer)fileinfo.st_mode);
    lua_setfield(L, -2, "mode");
    lua_pushinteger(L, (lua_Integer)fileinfo.st_size);
    lua_setfield(L, -2, "size");
    lua_pushinteger(L, (lua_Integer)fileinfo.st_uid);
    lua_setfield(L, -2, "uid");
    lua_pushinteger(L, (lua_Integer)fileinfo.st_gid);
    lua_setfield(L, -2, "gid");
    lua_pushinteger(L, (lua_Integer)fileinfo.st_mtime);
    lua_setfield(L, -2, "mtime");
    lua_pushinteger(L, (lua_Integer)fileinfo.st_atime);
    lua_setfield(L, -2, "atime");

    lua_pushnil(L);
    return 3;
}

int l_module_ssh_scp_send(lua_State *L) {
    l_ssh_session_t *sess_ud =
        (l_ssh_session_t *)luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!sess_ud || !sess_ud->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_scp_send() failed because "
                          "session was not initialized");
        return 2;
    }

    const char *path = luaL_checkstring(L, 2);
    long mode = (long)luaL_checkinteger(L, 3);
    lua_Number nsize = luaL_checknumber(L, 4);
    if (nsize < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "negative size");
        return 2;
    }
    size_t size = (size_t)(unsigned long long)nsize;

    LIBSSH2_CHANNEL *ch =
        libssh2_scp_send(sess_ud->session, path, (long)mode, size);
    if (!ch) {
        int rc = libssh2_session_last_error(sess_ud->session, NULL, NULL, 0);
        sess_ud->session = NULL;
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_scp_send failedwith code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }

    l_ssh_channel_t *ch_ud =
        (l_ssh_channel_t *)lua_newuserdata(L, sizeof *ch_ud);
    memset(ch_ud, 0, sizeof(*ch_ud));
    ch_ud->channel = ch;
    ch_ud->session = sess_ud->session;
    luaL_getmetatable(L, SSH_CHANNEL_MT);
    lua_setmetatable(L, -2);

    lua_pushnil(L);
    return 2;
}

int l_module_ssh_scp_send64(lua_State *L) {
    l_ssh_session_t *sess_ud =
        (l_ssh_session_t *)luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!sess_ud || !sess_ud->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_scp_send64() failed because "
                          "session was not initialized");
        return 2;
    }

    const char *path = luaL_checkstring(L, 2);
    int mode = luaL_checkinteger(L, 3);

    // Accept size as Lua number or integer; convert to 64-bit
    unsigned long long size64 = 0;

    if (lua_isinteger(L, 4)) {
        size64 = (unsigned long long)lua_tointeger(L, 4);
    } else {
        lua_Number nn = luaL_checknumber(L, 4);
        if (nn < 0) {
            lua_pushnil(L);
            lua_pushstring(L,
                           "l_module_ssh_scp_send64() failed because size < 0");
            return 2;
        }
        size64 = (unsigned long long)nn;
    }
    time_t mtime = (time_t)luaL_optinteger(L, 5, 0);
    time_t atime = (time_t)luaL_optinteger(L, 6, 0);

    LIBSSH2_CHANNEL *ch =
        libssh2_scp_send64(sess_ud->session, path, (int)mode,
                           (libssh2_uint64_t)size64, mtime, atime);
    if (!ch) {
        int rc = libssh2_session_last_error(sess_ud->session, NULL, NULL, 0);
        sess_ud->session = NULL;
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_scp_send64 failedwith code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }

    l_ssh_channel_t *ch_ud =
        (l_ssh_channel_t *)lua_newuserdata(L, sizeof *ch_ud);
    memset(ch_ud, 0, sizeof(*ch_ud));
    ch_ud->channel = ch;
    ch_ud->session = sess_ud->session;
    luaL_getmetatable(L, SSH_CHANNEL_MT);
    lua_setmetatable(L, -2);

    lua_pushnil(L);
    return 2;
}
