#include "modules/ssh/ltf-ssh-channel.h"

#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"

#include "internal_logging.h"
#include "util/da.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <stdlib.h>

#define DEFAULT_CHUNK_SIZE 4096

l_ssh_channel_t *check_channel_udata(lua_State *L) {
    l_ssh_channel_t *u = luaL_checkudata(L, 1, SSH_CHANNEL_MT);
    if (!u->channel) {
        luaL_error(L, "channel does not exist");
        return NULL;
    }
    if (!u->session) {
        luaL_error(L, "session associated with channel does not exist");
        return NULL;
    }

    return u;
}

int l_module_ssh_channel_open_session(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!s->session) {
        luaL_error(L, "l_module_ssh_channel_open_session() failed because "
                      "session was not initialized");
        return 0;
    }

    l_ssh_channel_t *u = lua_newuserdata(L, sizeof *u);
    u->channel = libssh2_channel_open_session(s->session);
    u->session = s;
    if (u->channel == NULL) {
        int rc = libssh2_session_last_error(u->session->session, NULL, NULL, 0);
        u->session = NULL;
        luaL_error(L, "libssh2_session_last_error failed with code: %s",
                   ssh_err_to_str(rc));

        return 0;
    }

    if (!da_append(u->session->active_channels, &u)) {
        luaL_error(L, "Out of memory");
        return 0;
    }

    luaL_getmetatable(L, SSH_CHANNEL_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_channel_request_pty(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    const char *term = luaL_checkstring(L, 2);

    int rc = libssh2_channel_request_pty(u->channel, term);
    if (rc) {
        luaL_error(L, "libssh2_channel_request_pty() failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_request_pty_size(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    int width = luaL_checkinteger(L, 2);
    int height = luaL_checkinteger(L, 3);
    if (width <= 0 || height <= 0) {
        luaL_error(L, "libssh2_channel_request_pty_size() failed because width "
                      "or height is <= 0)");
        return 0;
    }
    int rc = libssh2_channel_request_pty_size(u->channel, width, height);
    if (rc) {
        luaL_error(L, "libssh2_channel_request_pty_size() failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_flush(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    int rc = libssh2_channel_flush(u->channel);
    if (rc) {
        luaL_error(L, "libssh2_channel_flush() failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_flush_stderr(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    int rc = libssh2_channel_flush_stderr(u->channel);
    if (rc) {
        luaL_error(L, "libssh2_channel_flush_stderr() failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_get_exit_status(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    int rc = libssh2_channel_get_exit_status(u->channel);
    lua_pushinteger(L, rc);
    return 1;
}

int l_module_ssh_channel_exec(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    const char *s = luaL_checkstring(L, 2);

    int rc = libssh2_channel_exec(u->channel, s);

    if (rc) {
        luaL_error(L, "libssh2_channel_exec() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_channel_shell(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    int rc = libssh2_channel_shell(u->channel);

    if (rc) {
        luaL_error(L, "libssh2_channel_shell() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_channel_write(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);

    int rc = libssh2_channel_write(u->channel, s, len);

    if (rc < 0) {
        luaL_error(L, "libssh2_channel_write() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }
    if (rc != (int)len) {
        luaL_error(L,
                   "Failed to write complete command: %s, wrote only %s bytes",
                   s, ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_channel_read_helper(lua_State *L, bool read_stderr) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        luaL_error(L, "l_module_ssh_channel_read() failed: size is negative");
        return 0;
    }
    /* cap size to some reasonable max to avoid huge mallocs */
    size_t len = (size_t)v;
    if (len == 0)
        len = DEFAULT_CHUNK_SIZE;

    const size_t MAX_CHUNK = 64 * 1024 * 1024; // 64MB

    if (len > MAX_CHUNK)
        len = MAX_CHUNK;

    char *buf = (char *)malloc(len);
    if (!buf) {
        luaL_error(L, "l_module_ssh_channel_read() failed: out of memory");
        return 0;
    }

    ssize_t rc = read_stderr ? libssh2_channel_read_stderr(u->channel, buf, len)
                             : libssh2_channel_read(u->channel, buf, len);

    if (rc > 0) {
        lua_pushlstring(L, buf, (size_t)rc);
        free(buf);
        return 1;
    }

    if (rc == 0) {
        /* EOF */
        lua_pushlstring(L, "", 0);
        free(buf);
        return 1;
    }

    /* rc < 0 : error code */
    free(buf);

    /* special case EAGAIN */
    if (rc == LIBSSH2_ERROR_EAGAIN) {
        luaL_error(L, "libssh2_channel_read() failed: EAGAIN");
        return 0;
    }

    const char *msg = NULL;
    msg = ssh_err_to_str(rc);
    if (msg && msg[0]) {
        luaL_error(L, "libssh2_channel_read() failed with code %d: %s", rc,
                   msg);
    } else {
        luaL_error(L, "libssh2_channel_read() failed with code %d", rc);
    }

    return 0;
}

int l_module_ssh_channel_read(lua_State *L) {
    return l_module_ssh_channel_read_helper(L, false);
}

int l_module_ssh_channel_read_stderr(lua_State *L) {
    return l_module_ssh_channel_read_helper(L, true);
}

int l_module_ssh_channel_close(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    int rc = libssh2_channel_close(u->channel);
    if (rc) {
        luaL_error(L, "libssh2_channel_close() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    libssh2_channel_free(u->channel);
    int cleanup = 0;
    size_t size = da_size(u->session->active_channels);
    for (size_t i = 0; i < size; ++i) {
        l_ssh_channel_t **val = da_get(u->session->active_channels, i);
        if (*val == u) {
            da_remove(u->session->active_channels, i);
            cleanup++;
            size--;
        }
    }
    if (cleanup != 1) {
        luaL_error(L, "Something wrong with channels pointers");
        return 0;
    }

    u->channel = NULL;
    u->session = NULL;
    return 0;
}

int l_module_ssh_channel_setenv(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    const char *varname = luaL_checkstring(L, 2);
    const char *value = luaL_checkstring(L, 3);

    int rc = libssh2_channel_setenv(u->channel, varname, value);
    if (rc) {
        luaL_error(L, "libssh2_channel_setenv failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_send_eof(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    int rc = libssh2_channel_send_eof(u->channel);
    if (rc) {
        luaL_error(L, "libssh2_channel_send_eof failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

int l_module_ssh_channel_eof(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    int rc = libssh2_channel_eof(u->channel);
    if (rc < 0) {
        luaL_error(L, "libssh2_channel_eof failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    lua_pushboolean(L, rc == 1);
    return 1;
}

int l_module_ssh_channel_wait_eof(lua_State *L) {
    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    int rc = libssh2_channel_wait_eof(u->channel);
    if (rc) {
        luaL_error(L, "libssh2_channel_send_eof failed with code: %s",
                   ssh_err_to_str(rc));
    }
    return 0;
}

/* ---------- DESTRUCTOR (GC) ---------- */

int l_channel_ssh_gc(lua_State *L) {

    l_ssh_channel_t *u = check_channel_udata(L);
    if (!u) {
        return 0;
    }

    if (!u->channel) {
        return 0;
    }
    libssh2_channel_close(u->channel);

    libssh2_channel_free(u->channel);

    size_t size = da_size(u->session->active_channels);
    for (size_t i = 0; i < size; ++i) {
        l_ssh_channel_t **val = da_get(u->session->active_channels, i);
        if (*val == u) {
            da_remove(u->session->active_channels, i);
        }
    }
    u->channel = NULL;
    u->session = NULL;

    return 0;
}
/* ---------- REGISTRATION ---------- */

static const luaL_Reg channel_methods[] = {
    {"exec", l_module_ssh_channel_exec},
    {"shell", l_module_ssh_channel_shell},
    {"get_exit_status", l_module_ssh_channel_get_exit_status},
    {"flush", l_module_ssh_channel_flush},
    {"flush_stderr", l_module_ssh_channel_flush_stderr},
    {"write", l_module_ssh_channel_write},
    {"read", l_module_ssh_channel_read},
    {"read_stderr", l_module_ssh_channel_read_stderr},
    {"request_pty", l_module_ssh_channel_request_pty},
    {"request_pty_size", l_module_ssh_channel_request_pty_size},
    {"setenv", l_module_ssh_channel_setenv},
    {"send_eof", l_module_ssh_channel_send_eof},
    {"eof", l_module_ssh_channel_eof},
    {"wait_eof", l_module_ssh_channel_wait_eof},
    {"close", l_module_ssh_channel_close},
    {NULL, NULL},
};

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
