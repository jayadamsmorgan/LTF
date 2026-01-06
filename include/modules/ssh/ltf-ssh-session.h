#ifndef MODULE_SSH2_SESSION_H
#define MODULE_SSH2_SESSION_H

#include <lauxlib.h>
#include <libssh2.h>
#include <lua.h>
#include <lualib.h>
#include <util/da.h>

typedef enum {
    SSH_AUTH_USERPASS,
} l_ssh_session_auth_method;

typedef struct {
    char *user;
    char *password;
} l_ssh_session_auth_userpass_t;

typedef struct {
    LIBSSH2_SESSION *session;
    libssh2_socket_t sock_fd;

    char *ip;
    int port;

    l_ssh_session_auth_method method;
    union {
        l_ssh_session_auth_userpass_t userpass;
    };
    da_t *active_channels;
} l_ssh_session_t;

#define SSH_SESSION_MT "ltf-ssh-session"

/******************* API START ***********************/

// low.session_init_userpass(
//      ip:string,
//      port:integer,
//      user:string,
//      password:string
// )
int l_module_ssh_session_init_userpass(lua_State *L);

// session:close(self:session)
int l_module_ssh_session_close(lua_State *L);

// session:connect(self: session)
int l_module_ssh_session_connect(lua_State *L);

// session:disconnect(self: session)
int l_module_ssh_session_disconnect(lua_State *L);

/******************* API END *************************/

// Register session funcitons in "ltf-ssh" module
int l_module_register_ssh_session(lua_State *L);

#endif // MODULE_SSH2_SESSION_H
