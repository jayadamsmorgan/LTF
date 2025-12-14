#include "modules/ssh/ltf-ssh-sftp.h"

#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"

#include "internal_logging.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <stdlib.h>

#define DEFAULT_CHUNK_SIZE 4096

int l_module_ssh_sftp_init(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!s->session) {
        luaL_error(L, "l_module_ssh_sftp_init() failed because "
                      "session was not initialized");
        return 0;
    }

    l_sftp_session_t *u = lua_newuserdata(L, sizeof *u);
    u->sftp_session = libssh2_sftp_init(s->session);
    u->sftp_handle = NULL;
    u->session = s->session;
    if (u->sftp_session == NULL) {
        u->session = NULL;
        luaL_error(L, "libssh2_sftp_init failed");
        return 0;
    }
    luaL_getmetatable(L, SFTP_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_sftp_open(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "sftp_session was not initialized");

        return 0;
    }
    if (u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "sftp_handle for this sftp_session already exist");

        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "session was not initialized");

        return 0;
    }

    const char *filename = luaL_checkstring(L, 2);
    unsigned long flags = luaL_checknumber(L, 3);
    long mode = luaL_checknumber(L, 4);
    int open_type = luaL_checkinteger(L, 5);

    u->sftp_handle = libssh2_sftp_open_ex(u->sftp_session, filename,
                                          (unsigned int)strlen(filename), flags,
                                          mode, open_type);

    if (!u->sftp_handle) {
        int rc = libssh2_sftp_last_error(u->sftp_session);
        luaL_error(L, "libssh2_sftp_open() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_sftp_read(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (!u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "sftp_handle was not initialized");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "session was not initialized");
        return 0;
    }
    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        luaL_error(L, "l_module_ssh_sftp_read() failed: size is negative");
        return 0;
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
        luaL_error(L, "l_module_ssh_sftp_read() failed: out of memory");
        return 0;
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
        free(buf);
        const char *msg = NULL;
        msg = ssh_err_to_str(rc);
        if (msg && msg[0]) {
            luaL_error(L, "l_module_ssh_sftp_read() failed with code %d: %s",
                       rc, msg);
        } else {
            luaL_error(L, "l_module_ssh_sftp_read() failed with code %d", rc);
        }
        return 0;
    }
}

int l_module_ssh_sftp_write(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (!u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "sftp_handle was not initialized");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "session was not initialized");
        return 0;
    }
    const char *s = luaL_checkstring(L, 2);

    int v = luaL_checkinteger(L, 3);
    if (v < 0) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "argument 3 is negative");
        return 0;
    }
    size_t len = (size_t)v;
    if (sizeof(s) < len) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because size of "
                      "string is biger then 3d argument");
        return 0;
    }

    int rc = libssh2_sftp_write(u->sftp_handle, s, len);

    if (rc < 0) {
        luaL_error(L, "l_module_ssh_sftp_write() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    // No error and full buf was written
    lua_pushinteger(L, rc);
    return 1;
}

int l_module_ssh_sftp_close(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_close() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (!u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_close() failed because "
                      "sftp_handle was not initialized");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_close() failed because "
                      "session was not initialized");
        return 0;
    }
    int rc = libssh2_sftp_close(u->sftp_handle);
    if (rc) {
        luaL_error(L, "l_module_ssh_sftp_close() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    u->sftp_handle = NULL;

    return 0;
}

int l_module_ssh_sftp_shutdown(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_shutdown() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_shutdown() failed because "
                      "sftp_handle still exist, call sftp_close before");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_shutdown() failed because "
                      "session was not initialized");
        return 0;
    }
    int rc = libssh2_sftp_shutdown(u->sftp_session);
    if (rc) {
        luaL_error(L, "l_module_ssh_sftp_shutdown() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }
    u->sftp_session = NULL;
    u->session = NULL;

    return 0;
}

int l_module_ssh_sftp_file_info(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u || !u->sftp_session) {
        luaL_error(L, "sftp_session not initialized");
        return 0;
    }

    const char *path = luaL_checkstring(L, 2);
    LIBSSH2_SFTP_ATTRIBUTES fileinfo;
    bool link = false;
    const char *type = NULL;

    int rc = libssh2_sftp_stat_ex(u->sftp_session, path, strlen(path),
                                  LIBSSH2_SFTP_LSTAT, &fileinfo);

    if (rc < 0) {
        unsigned long fx = libssh2_sftp_last_error(u->sftp_session);
        if (fx == LIBSSH2_FX_NO_SUCH_FILE || fx == LIBSSH2_FX_NO_SUCH_PATH) {
            // File does not exist
            lua_pushnil(L);
            return 1;
        }
        luaL_error(L, "libssh2_sftp_stat_ex returned error: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    if (LIBSSH2_SFTP_S_ISLNK(fileinfo.permissions)) {
        link = true;
        rc = libssh2_sftp_stat_ex(u->sftp_session, path, strlen(path),
                                  LIBSSH2_SFTP_LSTAT, &fileinfo);
        if (rc < 0) {
            luaL_error(L, "libssh2_sftp_stat_ex returned error: %s",
                       ssh_err_to_str(rc));
            return 0;
        }
    }

    if (LIBSSH2_SFTP_S_ISREG(fileinfo.permissions)) {
        type = "file";
    } else if (LIBSSH2_SFTP_S_ISDIR(fileinfo.permissions)) {
        type = "directory";
    } else {
        luaL_error(L, "Unknown type of file for '%s'", path);
        return 0;
    }

    lua_newtable(L);
    lua_pushboolean(L, link);
    lua_setfield(L, -2, "is_symlink");

    lua_pushstring(L, type);
    lua_setfield(L, -2, "type");

    if (link) {
        char buf[PATH_MAX];
        rc = libssh2_sftp_readlink(u->sftp_session, path, buf, sizeof(buf) - 1);
        if (rc < 0) {
            lua_pop(L, 1);
            luaL_error(L, "libssh2_sftp_readlink() failed");
            return 0;
        }
        lua_pushlstring(L, buf, rc);
    } else {
        lua_pushstring(L, path);
    }
    lua_setfield(L, -2, "resolved_path");

    lua_pushinteger(L, (lua_Integer)fileinfo.filesize);
    lua_setfield(L, -2, "size");

    char buf[PATH_MAX];
    rc = libssh2_sftp_realpath(u->sftp_session, path, buf, sizeof(buf) - 1);
    if (rc < 0) {
        lua_pop(L, 1);
        luaL_error(L, "libssh2_sftp_realpath() failed");
        return 0;
    }
    lua_pushlstring(L, buf, rc);
    lua_setfield(L, -2, "path");

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
    {"file_info", l_module_ssh_sftp_file_info},
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
