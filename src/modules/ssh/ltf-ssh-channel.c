#include "modules/ssh/ltf-ssh-channel.h"
#include "internal_logging.h"
#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#define DEFAULT_CHUNK_SIZE 4096
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
                          "channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_request_pty() failed because "
                          "session does not exist");
        return 2;
    }
    const char *s = luaL_checkstring(L, 2);

    int rc = libssh2_channel_request_pty(u->channel, s);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_request_pty() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_exec(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_exec() failed because "
                          "channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_exec() failed because "
                          "session associated with channel does not exist");
        return 2;
    }

    const char *s = luaL_checkstring(L, 2);

    int rc = libssh2_channel_exec(u->channel, s);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_exec() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_shell(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_shell() failed because "
                          "channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_shell() failed because "
                          "session associated with channel does not exist");
        return 2;
    }

    int rc = libssh2_channel_shell(u->channel);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_shell() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_write(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_shell() failed because "
                          "channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_shell() failed because "
                          "session associated with channel does not exist");
        return 2;
    }
    const char *s = luaL_checkstring(L, 2);

    int v = luaL_checkinteger(L, 3);
    if (v < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_shell() failed because "
                          "argument 3 is negative");
        return 2;
    }

    size_t len = (size_t)v;

    int rc = libssh2_channel_write(u->channel, s, len);

    // No error and full buf was written
    if (rc < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_write() failed with code: %d", rc);
        return 2;
    } else if (rc != v) {
        lua_pushnil(L);
        lua_pushfstring(
            L, "Failed to write complete command: %s, wrote only %d bytes", s,
            rc);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_read(lua_State *L) {
    l_ssh_channel_t *u =
        (l_ssh_channel_t *)luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u || !u->channel) {
        lua_pushnil(L);
        lua_pushstring(
            L, "l_module_ssh_channel_read() failed: channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_read() failed: session "
                          "associated with channel does not exist");
        return 2;
    }

    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "l_module_ssh_channel_read() failed: size is negative");
        return 2;
    }
    /* cap size to some reasonable max to avoid huge mallocs */
    size_t len = (size_t)v;
    if (len == 0)
        len = DEFAULT_CHUNK_SIZE;
    const size_t MAX_CHUNK =
        64 * 1024 * 1024; /* 64MB safety cap — при желании поменяй */
    if (len > MAX_CHUNK)
        len = MAX_CHUNK;

    char *buf = (char *)malloc(len);
    if (!buf) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_read() failed: out of memory");
        return 2;
    }

    ssize_t rc = libssh2_channel_read(u->channel, buf, len);

    if (rc > 0) {
        lua_pushlstring(L, buf, (size_t)rc);
        free(buf);
        return 1;
    } else if (rc == 0) {
        /* EOF */
        lua_pushlstring(L, "", 0);
        free(buf);
        return 1;
    } else {
        /* rc < 0 : error code */
        /* special case EAGAIN */
        if (rc == LIBSSH2_ERROR_EAGAIN) {
            free(buf);
            lua_pushnil(L);
            lua_pushstring(L, "EAGAIN");
            return 2;
        } else {
            free(buf);
            const char *msg = NULL;
            msg = ssh_err_to_str(rc);
            if (msg && msg[0]) {
                lua_pushnil(L);
                lua_pushfstring(
                    L, "libssh2_channel_read() failed with code %d: %s", rc,
                    msg);
            } else {
                lua_pushnil(L);
                lua_pushfstring(L, "libssh2_channel_read() failed with code %d",
                                rc);
            }
            return 2;
        }
    }
}
int l_module_ssh_channel_read_stderr(lua_State *L) {
    l_ssh_channel_t *u =
        (l_ssh_channel_t *)luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u || !u->channel) {
        lua_pushnil(L);
        lua_pushstring(
            L, "l_module_ssh_channel_read() failed: channel does not exist");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_read() failed: session "
                          "associated with channel does not exist");
        return 2;
    }

    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "l_module_ssh_channel_read() failed: size is negative");
        return 2;
    }
    /* cap size to some reasonable max to avoid huge mallocs */
    size_t len = (size_t)v;
    if (len == 0)
        len = DEFAULT_CHUNK_SIZE;
    const size_t MAX_CHUNK =
        64 * 1024 * 1024; /* 64MB safety cap — при желании поменяй */
    if (len > MAX_CHUNK)
        len = MAX_CHUNK;

    char *buf = (char *)malloc(len);
    if (!buf) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_read() failed: out of memory");
        return 2;
    }

    ssize_t rc = libssh2_channel_read_stderr(u->channel, buf, len);

    if (rc > 0) {
        lua_pushlstring(L, buf, (size_t)rc);
        free(buf);
        return 1;
    } else if (rc == 0) {
        /* EOF */
        lua_pushlstring(L, "", 0);
        free(buf);
        return 1;
    } else {
        /* rc < 0 : error code */
        /* special case EAGAIN */
        if (rc == LIBSSH2_ERROR_EAGAIN) {
            free(buf);
            lua_pushnil(L);
            lua_pushstring(L, "EAGAIN");
            return 2;
        } else {
            free(buf);
            const char *msg = NULL;
            msg = ssh_err_to_str(rc);
            if (msg && msg[0]) {
                lua_pushnil(L);
                lua_pushfstring(
                    L, "libssh2_channel_read() failed with code %d: %s", rc,
                    msg);
            } else {
                lua_pushnil(L);
                lua_pushfstring(L, "libssh2_channel_read() failed with code %d",
                                rc);
            }
            return 2;
        }
    }
}

int l_module_ssh_channel_close(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_close() failed because "
                          "channel does not exist");
        return 2;
    }

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_close() failed because "
                          "session associated with channel does not exist");
        return 2;
    }

    int rc = libssh2_channel_close(u->channel);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_close() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_channel_free(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_free() failed because "
                          "channel does not exist");
        return 2;
    }

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_channel_free() failed because "
                          "session associated with channel does not exist");
        return 2;
    }

    int rc = libssh2_channel_free(u->channel);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_channel_free() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    u->channel = NULL;
    lua_pushboolean(L, 1);
    return 1;
}

/* ---------- DESTRUCTOR (GC) ---------- */

int l_channel_ssh_gc(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (u && u->channel) {
        libssh2_channel_close(u->channel);
        libssh2_channel_free(u->channel);
        u->channel = NULL;
        u->session = NULL;
    }
    return 0;
}

/* ---------- REGISTRATION ---------- */

static const luaL_Reg channel_methods[] = {
    {"request_pty", l_module_ssh_channel_request_pty},
    {"exec", l_module_ssh_channel_exec},
    {"shell", l_module_ssh_channel_shell},
    {"write", l_module_ssh_channel_write},
    {"read", l_module_ssh_channel_read},
    {"read_stderr", l_module_ssh_channel_read_stderr},
    {"close", l_module_ssh_channel_close},
    {"free", l_module_ssh_channel_free},
    {NULL, NULL}};

int l_module_register_ssh_channel(lua_State *L) {
    LOG("Registering ltf-ssh-channel");

    if (luaL_newmetatable(L, SSH_CHANNEL_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, channel_methods, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_channel_ssh_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-channel");
    return 0;
}
