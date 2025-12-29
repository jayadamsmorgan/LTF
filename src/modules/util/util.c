#include "modules/util/util.h"

#include "internal_logging.h"

#include <lauxlib.h>
#include <linux/limits.h>
#include <lua.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ---------- helpers ---------- */

static char *xgetcwd_dyn(void) {
    size_t cap = 128;
    for (;;) {
        char *buf = (char *)malloc(cap);
        if (!buf)
            return NULL;
        if (getcwd(buf, cap))
            return buf;
        free(buf);
        if (errno != ERANGE)
            return NULL;
        cap *= 2;
    }
}

/* Make absolute path (lexical): join with cwd if relative, normalize ., .., //.
 * Does NOT touch filesystem and does NOT resolve symlinks.
 * Returns malloc'd string or NULL (errno set).
 */
static char *make_path_abs_lexical(const char *path) {
    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    char *raw = NULL;

    if (path[0] == '/') {
        raw = strdup(path);
        if (!raw)
            return NULL;
    } else {
        char *cwd = xgetcwd_dyn();
        if (!cwd)
            return NULL;

        size_t need = strlen(cwd) + 1 + strlen(path) + 1;
        raw = (char *)malloc(need);
        if (!raw) {
            free(cwd);
            return NULL;
        }
        snprintf(raw, need, "%s/%s", cwd, path);
        free(cwd);
    }

    /* normalize raw -> (in-place rebuild using a temp token buffer) */
    char *tmp = strdup(raw);
    if (!tmp) {
        free(raw);
        return NULL;
    }

    /* count max segments */
    size_t max_segs = 1;
    for (char *p = tmp; *p; p++)
        if (*p == '/')
            max_segs++;

    char **segs = (char **)malloc(max_segs * sizeof(char *));
    if (!segs) {
        free(tmp);
        free(raw);
        return NULL;
    }

    size_t sp = 0;

    /* parse segments manually; write NULs into tmp */
    char *p = tmp;
    while (*p) {
        while (*p == '/')
            p++;
        if (!*p)
            break;

        char *seg = p;
        while (*p && *p != '/')
            p++;
        if (*p)
            *p++ = '\0';

        if (strcmp(seg, ".") == 0 || seg[0] == '\0') {
            continue;
        }
        if (strcmp(seg, "..") == 0) {
            if (sp > 0)
                sp--;
            continue;
        }
        segs[sp++] = seg;
    }

    /* rebuild into raw */
    size_t w = 0;
    raw[w++] = '/';
    for (size_t i = 0; i < sp; i++) {
        size_t n = strlen(segs[i]);
        memcpy(raw + w, segs[i], n);
        w += n;
        if (i + 1 < sp)
            raw[w++] = '/';
    }
    raw[w] = '\0';

    free(segs);
    free(tmp);

    return raw;
}

static const char *mode_to_type(mode_t m) {
    if (S_ISREG(m))
        return "file";
    if (S_ISDIR(m))
        return "directory";
    if (S_ISLNK(m))
        return "symlink";
    return NULL;
}

int l_module_util_file_info(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    struct stat sb;
    if (lstat(path, &sb) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            lua_pushnil(L);
            return 1;
        }
        luaL_error(L, "lstat('%s') failed: %s", path, strerror(errno));
        return 0;
    }

    const char *type = mode_to_type(sb.st_mode);
    if (!type) {
        luaL_error(L, "Unknown file type for '%s' (mode=%o)", path,
                   (unsigned)sb.st_mode);
        return 0;
    }

    char *abs = make_path_abs_lexical(path);
    if (!abs) {
        luaL_error(L, "make_path_abs_lexical('%s') failed: %s", path,
                   strerror(errno));
        return 0;
    }

    lua_newtable(L);

    lua_pushstring(L, type);
    lua_setfield(L, -2, "type");

    lua_pushstring(L, abs);
    lua_setfield(L, -2, "path");

    lua_pushinteger(L, (lua_Integer)sb.st_size);
    lua_setfield(L, -2, "size");

    lua_pushinteger(L, (lua_Integer)(sb.st_mode));
    lua_setfield(L, -2, "permissions");

    free(abs);
    return 1;
}

int l_module_util_resolve_symlink(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    struct stat sb;
    if (lstat(path, &sb) != 0) {
        // "symlink does not exist" -> error
        luaL_error(L, "lstat('%s') failed: %s", path, strerror(errno));
        return 0;
    }

    if (!S_ISLNK(sb.st_mode)) {
        luaL_error(L, "not a symlink: '%s'", path);
        return 0;
    }

    errno = 0;
    char *rp = realpath(path, NULL); // resolves symlink
    if (!rp) {
        // link exists but doesn't lead anywhere -> nil
        if (errno == ENOENT || errno == ENOTDIR) {
            lua_pushnil(L);
            return 1;
        }
        luaL_error(L, "realpath('%s') failed: %s", path, strerror(errno));
        return 0;
    }

    lua_pushstring(L, rp);
    free(rp);
    return 1;
}

static const luaL_Reg module_fns[] = {
    {"file_info", l_module_util_file_info},
    {"resolve_symlink", l_module_util_resolve_symlink},
    {NULL, NULL},
};

int l_module_util_register_module(lua_State *L) {
    LOG("Registering ltf-util module...");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);

    LOG("Successfully registered ltf-helper module.");
    return 1;
}
