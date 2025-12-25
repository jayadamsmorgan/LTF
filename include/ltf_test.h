#ifndef LTF_TEST_H
#define LTF_TEST_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

void ltf_mark_test_failed();

void register_ltf_libs(lua_State *L);

int load_lua_dir(const char *dir_path, lua_State *L);

int ltf_test();

#endif // LTF_TEST_H
