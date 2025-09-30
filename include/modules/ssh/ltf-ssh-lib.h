#ifndef MODULE_SSH2_LIB_H
#define MODULE_SSH2_LIB_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <libssh2.h>

#define SSH_LIBMOD_MT "ltf-libmod"
#define SSH_SOCKET_MT "ltf-socket"

// ts:init() -> bool or err msg
int l_module_ssh_lib_init(lua_State *L);
// ts:exit() -> bool 
int l_module_ssh_lib_exit(lua_State *L);

int l_module_ssh_lib_free(lua_State *L); // Do not use
int l_module_ssh_lib_hostkey_hash(lua_State *L); // Do not use

int l_module_ssh_banner_set(lua_State *L); // Do not use
int l_module_ssh_base64_decode(lua_State *L); // Do not use

int l_module_ssh_poll(lua_State *L); // Do not use
int l_module_ssh_poll_chanell_read(lua_State *L); // Do not use

int l_module_ssh_trace(lua_State *L); // Do not use
int l_module_ssh_trace_sethandler(lua_State *L); // Do not use



int l_module_ssh_socket_connect(lua_State *L);

int l_module_ssh_register_module(lua_State *L);


#endif // MODULE_SSH2_LIB_H
