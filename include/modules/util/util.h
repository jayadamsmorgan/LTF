#ifndef MODULE_LTF_H
#define MODULE_LTF_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/******************* API START ***********************/

// file_info:
// - type: "directory"|"file"|"symlink"
// - path: string
// - size: integer
// - permissions: integer
//
// util.file_info(path: string) -> result: file_info
int l_module_util_file_info(lua_State *L);

// util.resolve_symlink(path: string) -> resolved: string
int l_module_util_resolve_symlink(lua_State *L);

/******************* API END *************************/

// Register "ltf-util" module
int l_module_util_register_module(lua_State *L);

#endif // MODULE_LTF_H
