#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "modules/ssh/ltf-ssh-channel.h"

int l_module_ssh_channel_open_session(lua_State *L) {}

int l_module_ssh_channel_request_pty(lua_State *L) {}

int l_module_ssh_channel_exec(lua_State *L) {}

int l_module_ssh_channel_shell(lua_State *L) {}

int l_module_ssh_channel_write(lua_State *L) {}

int l_module_ssh_channel_read(lua_State *L) {}

int l_module_ssh_channel_close(lua_State *L) {}

int l_module_ssh_channel_free(lua_State *L) {}
