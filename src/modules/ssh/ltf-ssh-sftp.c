#include "modules/ssh/ltf-ssh-sftp.h"
#include "internal_logging.h"
#include "modules/ssh/ltf-ssh-session.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-lib.h"
#include <stdlib.h>
#define DEFAULT_CHUNK_SIZE 4096
/* ---------- CONSTRUCTOR / METHODS ---------- */

int l_module_ssh_sftp_init(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!s->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_init() failed because "
                          "session was not initialized");
        return 2;
    }

    l_sftp_session_t *u = lua_newuserdata(L, sizeof *u);
    u->sftp_session = libssh2_sftp_init(s->session);
    u->sftp_handle = NULL;
    u->session = s->session;
    if (u->sftp_session == NULL) {
        u->session = NULL;
        lua_pushnil(L);
        lua_pushstring(L, "libssh2_sftp_init failed");
        return 2;
    }
    luaL_getmetatable(L, SFTP_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_sftp_open(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_open() failed because "
                          "sftp_session was not initialized");

        return 2;
    }
    if (u->sftp_handle) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_open() failed because "
                          "sftp_handle for this sftp_session already exist");

        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_open() failed because "
                          "session was not initialized");

        return 2;
    }

    const char *filename = luaL_checkstring(L, 2);
    unsigned long flags = luaL_checknumber(L, 3);
    long mode = luaL_checknumber(L, 4);
    u->sftp_handle = libssh2_sftp_open(u->sftp_session, filename, flags, mode);

    if (!u->sftp_handle) {
        int rc = libssh2_sftp_last_error(u->sftp_session);
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_sftp_open() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_sftp_read(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_read() failed because "
                          "sftp_session was not initialized");
        return 2;
    }
    if (!u->sftp_handle) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_read() failed because "
                          "sftp_handle was not initialized");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_read() failed because "
                          "session was not initialized");
        return 2;
    }
    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_read() failed: size is negative");
        return 2;
    }

    /* cap size to some reasonable max to avoid huge mallocs */
    size_t len = (size_t)v;
    if (len == 0)
        len = DEFAULT_CHUNK_SIZE;
    const size_t MAX_CHUNK = 64 * 1024 * 1024; // 64MB safety cap
    if (len > MAX_CHUNK)
        len = MAX_CHUNK;

    char *buf = (char *)malloc(len);
    if (!buf) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_read() failed: out of memory");
        return 2;
    }

    ssize_t rc = libssh2_sftp_read(u->sftp_handle, buf, len);

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
                    L, "l_module_ssh_sftp_read() failed with code %d: %s", rc,
                    msg);
            } else {
                lua_pushnil(L);
                lua_pushfstring(
                    L, "l_module_ssh_sftp_read() failed with code %d", rc);
            }
            return 2;
        }
    }
}

int l_module_ssh_sftp_write(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_write() failed because "
                          "sftp_session was not initialized");
        return 2;
    }
    if (!u->sftp_handle) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_write() failed because "
                          "sftp_handle was not initialized");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_write() failed because "
                          "session was not initialized");
        return 2;
    }
    const char *s = luaL_checkstring(L, 2);

    int v = luaL_checkinteger(L, 3);
    if (v < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_write() failed because "
                          "argument 3 is negative");
        return 2;
    }
    size_t len = (size_t)v;
    if (sizeof(s) < len) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_write() failed because size of "
                          "string is biger then 3d argument");
        return 2;
    }

    int rc = libssh2_sftp_write(u->sftp_handle, s, len);

    // No error and full buf was written
    if (rc < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "l_module_ssh_sftp_write() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushinteger(L, rc);
    return 1;
}

int l_module_ssh_sftp_close(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_close() failed because "
                          "sftp_session was not initialized");
        return 2;
    }
    if (!u->sftp_handle) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_close() failed because "
                          "sftp_handle was not initialized");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_close() failed because "
                          "session was not initialized");
        return 2;
    }
    int rc = libssh2_sftp_close(u->sftp_handle);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "l_module_ssh_sftp_close() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    u->sftp_handle = NULL;
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_sftp_shutdown(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_shutdown() failed because "
                          "sftp_session was not initialized");
        return 2;
    }
    if (u->sftp_handle) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_shutdown() failed because "
                          "sftp_handle still exist, call sftp_close before");
        return 2;
    }
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "l_module_ssh_sftp_shutdown() failed because "
                          "session was not initialized");
        return 2;
    }
    int rc = libssh2_sftp_shutdown(u->sftp_session);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "l_module_ssh_sftp_shutdown() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    u->sftp_session = NULL;
    u->session = NULL;
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_sftp_last_error(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u || !u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "sftp_session not initialized");
        return 2;
    }

    int err = libssh2_sftp_last_error(u->sftp_session);
    lua_pushinteger(L, err);
    return 1;
}

int l_module_ssh_sftp_stat_ex(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u || !u->sftp_session) {
        lua_pushnil(L);
        lua_pushstring(L, "sftp_session not initialized");
        return 2;
    }

    const char *path = luaL_checkstring(L, 2);
    LIBSSH2_SFTP_ATTRIBUTES fileinfo;

    int rc = libssh2_sftp_stat_ex(u->sftp_session, path, strlen(path),
                                  LIBSSH2_SFTP_STAT, &fileinfo);
    if (rc < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_sftp_stat_ex returned error: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/* ---------- DESTRUCTOR (GC) ---------- */
int l_sftp_session_gc(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (u && u->sftp_session) {
        libssh2_sftp_close(u->sftp_handle);
        libssh2_sftp_shutdown(u->sftp_session);
        u->sftp_session = NULL;
        u->sftp_handle = NULL;
        u->session = NULL;
    }
    return 0;
}
/* ---------- REGISTRATION ---------- */
static const luaL_Reg sftp_methods[] = {
    {"open", l_module_ssh_sftp_open},
    {"write", l_module_ssh_sftp_write},
    {"read", l_module_ssh_sftp_read},
    {"last_error", l_module_ssh_sftp_last_error},
    {"stat_ex", l_module_ssh_sftp_stat_ex},
    {"close", l_module_ssh_sftp_close},
    {"shutdown", l_module_ssh_sftp_shutdown},
    {NULL, NULL}};

int l_module_register_ssh_sftp(lua_State *L) {
    LOG("Registering ltf-ssh-sftp");

    if (luaL_newmetatable(L, SFTP_SESSION_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, sftp_methods, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_sftp_session_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-sftp");
    return 0;
}
