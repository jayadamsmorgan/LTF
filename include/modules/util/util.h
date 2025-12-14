#ifndef MODULE_LTF_H
#define MODULE_LTF_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/******************* API START ***********************/

// file_info:
// - absolute_path: string
// - type: "directory"|"file"
// - size: integer
// - is_symlink: boolean
// - resolved_path: string
//
// util:file_info(path: string) -> result: file_info
int l_module_util_file_info(lua_State *L);

/******************* API END *************************/

// Register "ltf-util" module
int l_module_util_register_module(lua_State *L);

#endif // MODULE_LTF_H
