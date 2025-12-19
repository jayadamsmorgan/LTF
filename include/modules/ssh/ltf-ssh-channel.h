#ifndef MODULE_SSH2_CHANNEL_H
#define MODULE_SSH2_CHANNEL_H

#include <lauxlib.h>
#include <libssh2.h>
#include <lua.h>
#include <lualib.h>

typedef struct {
    LIBSSH2_CHANNEL *channel;
    LIBSSH2_SESSION *session;
} l_ssh_channel_t;

#define SSH_CHANNEL_MT "ltf-ssh-channel"

int l_module_ssh_channel_close(lua_State *L);

int l_module_ssh_channel_eof(lua_State *L);

int l_module_ssh_channel_exec(lua_State *L);

int l_module_ssh_channel_flush(lua_State *L);
int l_module_ssh_channel_flush_stderr(lua_State *L);

int l_module_ssh_channel_get_exit_status(lua_State *L);

// module:open_channel(session) -> channel
int l_module_ssh_channel_open_ex(lua_State *L);
int l_module_ssh_channel_open_session(lua_State *L);

int l_module_ssh_channel_process_startup(lua_State *L);
int l_module_ssh_channel_read(lua_State *L);
int l_module_ssh_channel_read_ex(lua_State *L);
int l_module_ssh_channel_read_stderr(lua_State *L);
int l_module_ssh_channel_receive_window_adjust(lua_State *L);
int l_module_ssh_channel_request_auth_agent(lua_State *L);

// channel:request_pty(session, terminal) -> 0/err
int l_module_ssh_channel_request_pty(lua_State *L);
int l_module_ssh_channel_request_pty_ex(lua_State *L);

int l_module_ssh_channel_request_pty_size(lua_State *L);
int l_module_ssh_channel_request_pty_size_ex(lua_State *L);
int l_module_ssh_channel_send_eof(lua_State *L);
int l_module_ssh_channel_set_blocking(lua_State *L);
int l_module_ssh_channel_setenv(lua_State *L);
int l_module_ssh_channel_setenv_ex(lua_State *L);
int l_module_ssh_channel_shell(lua_State *L);
int l_module_ssh_channel_subsystem(lua_State *L);
int l_module_ssh_channel_wait_closed(lua_State *L);
int l_module_ssh_channel_wait_eof(lua_State *L);
int l_module_ssh_channel_window_read(lua_State *L);
int l_module_ssh_channel_window_read_ex(lua_State *L);
int l_module_ssh_channel_window_write(lua_State *L);
int l_module_ssh_channel_window_write_ex(lua_State *L);
int l_module_ssh_channel_write(lua_State *L);
int l_module_ssh_channel_write_ex(lua_State *L);
int l_module_ssh_channel_write_stderr(lua_State *L);
int l_module_ssh_channel_x11_req(lua_State *L);
int l_module_ssh_channel_x11_req_ex(lua_State *L);

int l_module_register_ssh_channel(lua_State *L);
#endif // MODULE_SSH2_CHANNEL_H
