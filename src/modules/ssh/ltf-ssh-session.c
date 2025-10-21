#include "modules/ssh/ltf-ssh-session.h"
#include "internal_logging.h"
#include "modules/ssh/ltf-ssh-userauth.h"
#include <arpa/inet.h>
#include <errno.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <modules/ssh/ltf-ssh-lib.h>
/* ---------- CONSTRUCTOR / METHODS ---------- */

static int waitsocket(libssh2_socket_t socket_fd, LIBSSH2_SESSION *session) {
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
    FD_SET(socket_fd, &fd);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

    dir = libssh2_session_block_directions(session);

    if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select((int)(socket_fd + 1), readfd, writefd, NULL, &timeout);

    return rc;
}

int l_module_ssh_waitsocket(lua_State *L) {

    int fd = (int)luaL_checkinteger(L, 1); // libssh2_socket_t is 'int' here
    l_ssh_session_t *u = luaL_checkudata(L, 2, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "Session was not initialized");
        return 2;
    }
    if (fd == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "No valid socket descriptor was provided");
        return 2;
    }

    int rc = waitsocket(fd, u->session);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "waitsocket() failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

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

int l_module_ssh_socket_init(lua_State *L) {
    const char *ip = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);

    int fd = ssh_socket_connect_ipv4(ip, port);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "ssh_socket_connect_ipv4 failed with fd: %d", fd);
        return 2;
    }

    lua_pushinteger(L, fd);
    return 1;
}

int l_module_ssh_socket_free(lua_State *L) {
    int fd = (int)luaL_checkinteger(L, 1);

    if (fd < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "invalid socket fd");
        return 2;
    }
    shutdown(fd, SHUT_RDWR);

    if (close(fd) != 0) {
        int e = errno;
        lua_pushnil(L);
        lua_pushfstring(L, "close failed: %s", strerror(e));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_init(lua_State *L) {

    l_ssh_session_t *u = lua_newuserdata(L, sizeof *u);

    u->session = NULL;
    u->sock_fd = -1;

    u->session = libssh2_session_init();
    if (!u->session) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "libssh2_session_init() failed");
        return 2;
    }

    luaL_getmetatable(L, SSH_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_session_handshake(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    int fd = (int)luaL_checkinteger(L, 2); // libssh2_socket_t is 'int' here

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "Handshake failed because session was not initialized");
        return 2;
    }

    int rc = libssh2_session_handshake(u->session, fd);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_handshake failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }

    u->sock_fd = fd;
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_disconnect(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    const char *st = luaL_checkstring(L, 2);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(L, "Session disconnection failed because no actual "
                          "session was provided");
        return 2;
    }

    int rc = libssh2_session_disconnect(u->session, st);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_disconnect failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_free(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(
            L, "Session release failed because no actual session was provided");
        return 2;
    }

    int rc = libssh2_session_free(u->session);
    if (rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "libssh2_session_free failed with code: %s",
                        ssh_err_to_str(rc));
        return 2;
    }

    u->session = NULL; // to exclude double session_free procedure
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_set_timeout(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(
            L, "Timeout setup failed because no actual session was provided");
        return 2;
    }

    // Timeout in milliseconds:
    long t = luaL_checkinteger(L, 2);

    libssh2_session_set_timeout(u->session, t);
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_set_read_timeout(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(
            L,
            "Read timeout setup failed because no actual session was provided");
        return 2;
    }

    // Timeout in milliseconds:
    long t = luaL_checkinteger(L, 2);

    libssh2_session_set_read_timeout(u->session, t);
    lua_pushboolean(L, 1);
    return 1;
}

int l_module_ssh_session_last_error(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);

    if (!u->session) {
        lua_pushnil(L);
        lua_pushstring(
            L,
            "Session last error failed because no actual session was provided");
        return 2;
    }

    int ret = libssh2_session_last_error(u->session, NULL, NULL, 0);
    lua_pushfstring(L, "%s", ssh_err_to_str(ret));
    return 2;
}

/* ---------- DESTRUCTOR (GC) ---------- */

int l_session_ssh_gc(lua_State *L) {
    l_ssh_session_t *u = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (u && u->session) {
        libssh2_session_disconnect(u->session, "Normal Shutdown");
        libssh2_session_free(u->session);
        u->session = NULL;
    }

    if (u->sock_fd != -1) {
        shutdown(u->sock_fd, SHUT_RDWR);
        close(u->sock_fd);
        u->sock_fd = -1;
    }

    return 0;
}
/* ---------- REGISTRATION ---------- */

static const luaL_Reg session_methods[] = {
    {"handshake", l_module_ssh_session_handshake},
    {"disconnect", l_module_ssh_session_disconnect},
    {"free", l_module_ssh_session_free},
    {"userauth_password", l_module_ssh_userauth_password},
    {"set_timeout", l_module_ssh_session_set_timeout},
    {"set_read_timeout", l_module_ssh_session_set_read_timeout},
    {"get_last_error", l_module_ssh_session_last_error},
    {NULL, NULL}};

int l_module_register_ssh_session(lua_State *L) {
    LOG("Registering ltf-ssh-session");

    if (luaL_newmetatable(L, SSH_SESSION_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, session_methods, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_session_ssh_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-session");
    return 0;
}
