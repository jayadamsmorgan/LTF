#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"
#include "modules/ssh/ltf-ssh-userauth.h"

int l_module_ssh_userauth_password(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, " l_module_ssh_userauth_password failed because "
                          "session was not initialized");
        return 2;
    }

    const char *usr = luaL_checkstring(L, 2);
    const char *pswd = luaL_checkstring(L, 3);

    int rc = libssh2_userauth_password(u->session, usr, pswd);

    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_userauth_password failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}
