#include "modules/util/util.h"

#include "internal_logging.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int l_module_util_file_info(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    const char *type = NULL;
    bool link = false;

    struct stat sb;
    if (lstat(path, &sb)) {
        lua_pushnil(L);
        return 1;
    }

    if (S_ISLNK(sb.st_mode)) {
        link = true;
        if (stat(path, &sb)) {
            lua_pushnil(L);
            return 1;
        }
    }

    if (S_ISREG(sb.st_mode)) {
        type = "file";
    } else if (S_ISDIR(sb.st_mode)) {
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
        int ret = readlink(path, buf, sizeof(buf) - 1);
        if (ret == -1) {
            lua_pop(L, 1);
            luaL_error(L, "readlink() failed");
            return 0;
        }
        lua_pushlstring(L, buf, ret);
    } else {
        lua_pushstring(L, path);
    }
    lua_setfield(L, -2, "resolved_path");

    lua_pushinteger(L, (lua_Integer)sb.st_size);
    lua_setfield(L, -2, "size");

    char *absolute = realpath(path, NULL);
    if (!absolute) {
        lua_pop(L, 1);
        luaL_error(L, "realpath() failed");
        return 0;
    }
    lua_pushstring(L, absolute);
    lua_setfield(L, -2, "path");
    free(absolute);

    return 1;
}

static const luaL_Reg module_fns[] = {
    {"file_info", l_module_util_file_info},
    {NULL, NULL},
};

int l_module_util_register_module(lua_State *L) {
    LOG("Registering ltf-util module...");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);

    LOG("Successfully registered ltf-helper module.");
    return 0;
}
