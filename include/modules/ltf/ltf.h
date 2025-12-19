#ifndef MODULE_LTF_H
#define MODULE_LTF_H

#include "ltf_state.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DEFER_LIST_KEY "ltf.defer.list"

/******************* API START ***********************/

// ltf:defer(defer_func: function, ...)
// ltf:defer(defer_func: function(status: string))
int l_module_ltf_defer(lua_State *L);

// ltf:get_current_target() -> target: string
int l_module_ltf_get_current_target(lua_State *L);

// ltf:get_active_tags() -> tags: [string]
int l_module_ltf_get_active_tags(lua_State *L);

// ltf:get_active_test_tags() -> tags: [string]
int l_module_ltf_get_active_test_tags(lua_State *L);

// ltf:get_var(var_name:string) -> value:string
int l_module_ltf_get_var(lua_State *L);

// ltf:get_vars() -> [table<string, string>]
int l_module_ltf_get_vars(lua_State *L);

// ltf:log(log_level: string, ...)
int l_module_ltf_log(lua_State *L);

// ltf:millis() -> ms:integer
int l_module_ltf_millis(lua_State *L);

// ltf:print(...)
int l_module_ltf_print(lua_State *L);

// reg_opts:
// - name: string
// - tags: [string]?
// - description: string?
// - vars: [string]?
// - body: fun()
//
// ltf:test(opts: reg_opts)
int l_module_ltf_register_test(lua_State *L);

// ltf_var:
// - values: [string]?
// - default: string?
//
// ltf:register_vars(vars: table<string, ltf_var>)
int l_module_ltf_register_vars(lua_State *L);

// ltf:register_secrets(secrets: [string])
int l_module_ltf_register_secrets(lua_State *L);

// ltf:sleep(ms: number)
int l_module_ltf_sleep(lua_State *L);

/******************* API END *************************/

// Register "ltf-main" module
int l_module_ltf_register_module(lua_State *L);

void l_module_ltf_init(ltf_state_t *state);

#endif // MODULE_LTF_H
