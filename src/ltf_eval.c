#include "ltf_eval.h"
#include <stdio.h>
#include <stdlib.h>

#include "cmd_parser.h"
#include "internal_logging.h"
#include "ltf_test.h"
#include "project_parser.h"
#include "util/files.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

static char *project_lib_dir_path = NULL;

int ltf_eval() {

    int exitcode = EXIT_FAILURE;

    cmd_eval_options *opts = cmd_parser_get_eval_options();

    if (!file_exists(opts->name)) {
        LOG("File not found: %s ", opts->name);
        fprintf(stderr, "File not found: %s ", opts->name);
        return exitcode;
    }

    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal logging.\n");
        return exitcode;
    }

    LOG("Creating Lua state...");
    lua_State *L = luaL_newstate();
    LOG("Opening Lua libs...");
    luaL_openlibs(L);
    register_ltf_libs(L);

    LOG("Trying  to pars project libs...");

    const char *project = file_find_upwards(".ltf.json");

    if (project) {
        LOG("We are in existing project directory");
        if (project_parser_parse()) {
            return EXIT_FAILURE;
        }

        asprintf(&project_lib_dir_path, "%s/lib", project);

        LOG("Project lib directory path: %s", project_lib_dir_path);
        if (load_lua_dir(project_lib_dir_path, L) == -2) {
            LOG("Error while trying to upload libraries");
            goto deinit;
        } else {
            LOG("Libs were successfully uploaded ");
        }

    } else {
        LOG("We are not in existing project directory. Skip project libs "
            "loading");
    }

    int rc = luaL_dofile(L, opts->name);
    if (rc != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        LOG("Lua error: %s", err);
        lua_pop(L, 1);
    } else {
        exitcode = EXIT_SUCCESS;
    }

deinit:

    LOG("Tidying up...");
    lua_close(L);
    cmd_parser_free_eval_options();
    free(project_lib_dir_path);
    internal_logging_deinit();
    return exitcode;
}
