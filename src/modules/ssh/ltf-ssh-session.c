#include "modules/ssh/ltf-ssh-session.h"

#include "modules/ssh/ltf-ssh-lib.h"

#include "internal_logging.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static int ssh_socket_connect_ipv4(const char *ip_str, int port) {
    libssh2_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == LIBSSH2_INVALID_SOCKET) {
        fprintf(stderr, "failed to create socket.\n");
        return -1;
    }
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip_str, &sin.sin_addr) != 1) {
        close(sock);
        errno = EINVAL;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        int saved_errno = errno;
        close(sock);
        errno = saved_errno;
        return -1;
    }

    return sock;
}

int l_module_ssh_session_init_userpass(lua_State *L) {

    const char *ip = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    const char *user = luaL_checkstring(L, 3);
    const char *password = luaL_checkstring(L, 4);

    LIBSSH2_SESSION *session = NULL;

    session = libssh2_session_init();
    if (!session) {
        lua_pushnil(L);
        lua_pushstring(L, "libssh2_session_init() failed");
        return 2;
    }

    libssh2_session_set_timeout(session, 60000);

    l_ssh_session_t *u = lua_newuserdata(L, sizeof *u);
    u->session = session;
    u->sock_fd = -1;
    u->ip = strdup(ip);
    u->port = port;
    u->method = SSH_AUTH_USERPASS;
    u->userpass.user = strdup(user);
    u->userpass.password = strdup(password);

    luaL_getmetatable(L, SSH_SESSION_MT);
    lua_setmetatable(L, -2);

    return 1;
}

int l_module_ssh_session_connect(lua_State *L) {

    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        luaL_error(
            L, "Session connect failed because session was already closed.");
        return 0;
    }

    if (u->sock_fd != -1) {
        luaL_error(
            L,
            "Session connect failed because connection already established.");
        return 0;
    }

    int sock_fd = ssh_socket_connect_ipv4(u->ip, u->port);
    if (sock_fd < 0) {
        luaL_error(L, "ssh_socket_connect_ipv4 failed: %d", sock_fd);
        return 0;
    }
    u->sock_fd = sock_fd;

    int rc = libssh2_session_handshake(u->session, u->sock_fd);
    if (rc) {
        luaL_error(L, "libssh2_session_handshake failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    switch (u->method) {
    case SSH_AUTH_USERPASS:
        rc = libssh2_userauth_password(u->session, u->userpass.password,
                                       u->userpass.user);
        break;
    default:
        luaL_error(L, "Unknown SSH auth method %d", u->method);
        return 0;
    }

    if (rc) {
        luaL_error(L, "libssh2_userauth failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_session_disconnect(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    const char *desc = luaL_optstring(L, 2, "User terminated connection.");

    if (!u->session) {
        lua_pushstring(
            L, "Session disconnect failed because session have been closed.");
        return 1;
    }

    if (u->sock_fd == -1) {
        lua_pushstring(L, "Session disconnect failed because session is "
                          "already disconnected.");
        return 1;
    }

    int rc = libssh2_session_disconnect(u->session, desc);
    if (rc) {
        lua_pushfstring(L, "libssh2_session_disconnect failed with code: %s",
                        ssh_err_to_str(rc));
        return 1;
    }

    shutdown(u->sock_fd, SHUT_RDWR);

    if (close(u->sock_fd) != 0) {
        int e = errno;
        lua_pushfstring(L, "Unable to close socket: %s", strerror(e));
        return 1;
    }

    u->sock_fd = -1;

    return 0;
}

int l_module_ssh_session_close(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushstring(
            L, "Session close failed because no actual session was provided");
        return 1;
    }

    if (u->sock_fd != -1) {
        int res = l_module_ssh_session_disconnect(L);
        if (res) {
            return res;
        }
    }

    int rc = libssh2_session_free(u->session);
    if (rc) {
        lua_pushfstring(L, "libssh2_session_free failed with code: %s",
                        ssh_err_to_str(rc));
        return 1;
    }

    u->session = NULL;
    u->sock_fd = -1;
    u->port = -1;
    free(u->ip);

    switch (u->method) {
    case SSH_AUTH_USERPASS:
        free(u->userpass.user);
        u->userpass.user = NULL;
        free(u->userpass.password);
        u->userpass.password = NULL;
        break;
    default:
        break;
    }

    return 0;
}

/* ---------- DESTRUCTOR (GC) ---------- */

int l_session_ssh_gc(lua_State *L) {
    //
    return l_module_ssh_session_close(L);
}
/* ---------- REGISTRATION ---------- */

static const luaL_Reg session_methods[] = {
    {"close", l_module_ssh_session_close},
    {"connect", l_module_ssh_session_connect},
    {"disconnect", l_module_ssh_session_disconnect},
    {NULL, NULL},
};

int l_module_register_ssh_session(lua_State *L) {
    LOG("Registering ltf-ssh-session");

    luaL_newmetatable(L, SSH_SESSION_MT);
    lua_newtable(L);
    luaL_setfuncs(L, session_methods, 0);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, l_session_ssh_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushstring(L, "locked");
    lua_setfield(L, -2, "__metatable");
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-session");
    return 0;
}
