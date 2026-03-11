#include "keyword_status.h"

#include "util/lua_hooks.h"
#include "util/time.h"

#include "internal_logging.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static da_t *keyword_statuses = NULL;
static da_t *keyword_stack = NULL;
static ltf_state_t *ltf_state = NULL;
static char *blacklist_dir = NULL;
static bool test_running = false;

static void keyword_status_test_started(ltf_state_test_t *) {
    test_running = true;
    keyword_statuses = da_init(10, sizeof(keyword_status_t));
}

static void keyword_status_test_finished(ltf_state_test_t *) {
    test_running = false;
    ltf_state_test_set_keyword_statuses(ltf_state, keyword_statuses);
    // Freeing keywords is handled in ltf_state.c,
    // so here we just NULLify them just in case
    keyword_statuses = NULL;
}

static void call_hook(lua_State *L, lua_Debug *ar, const char *src) {
    if (!test_running)
        return;

    lua_getinfo(L, "nSf", ar);

    // Filter all "C" functions
    if (strcmp(ar->what, "Lua") != 0)
        return;

    // Filter all functions without name
    if (!ar->name)
        return;

    // Parent keyword
    keyword_status_t *parent = NULL;
    if (da_size(keyword_stack) > 0) {
        parent = *(keyword_status_t **)da_get(keyword_stack,
                                              da_size(keyword_stack) - 1);
    }

    // Filter child function from ltf libs
    bool ignored = false;
    if (parent && ar->source && strstr(ar->source, blacklist_dir) &&
        strstr(parent->file, blacklist_dir))
        ignored = true;

    char time[TS_LEN];
    get_date_time_now(time);

    keyword_status_t ks = {.children = da_init(10, sizeof(keyword_status_t)),
                           .name = strdup(ar->name ? ar->name : "(undefined)"),
                           .started = strdup(time),
                           .file = strdup(src),
                           .line = ar->linedefined,
                           .finished = NULL,
                           .ignored = ignored};

    keyword_status_t *added = NULL;

    if (!ignored) {

        if (parent && !parent->ignored) {
            da_append(parent->children, &ks);
            added = da_get(parent->children, da_size(parent->children) - 1);
        } else {
            da_append(keyword_statuses, &ks);
            added = da_get(keyword_statuses, da_size(keyword_statuses) - 1);
        }

    } else {

        added = malloc(sizeof(keyword_status_t));

        *added = (keyword_status_t){.children = NULL,
                                    .name = NULL,
                                    .started = NULL,
                                    .file = NULL,
                                    .line = 0,
                                    .finished = NULL,
                                    .ignored = true};
    }

    da_append(keyword_stack, &added);
    lua_pop(L, 1);
}

static void ret_hook(lua_State *L, lua_Debug *ar, const char *src) {

    if (!test_running)
        return;

    lua_getinfo(L, "nSf", ar); // nS: name/what, f: push function

    // Filter all "C" functions
    if (strcmp(ar->what, "Lua") != 0)
        return;

    // Filter all functions without name
    if (!ar->name)
        return;

    size_t sz = da_size(keyword_stack);
    if (sz == 0)
        return;

    // Parent keyword
    keyword_status_t *parent = NULL;
    if (da_size(keyword_stack) > 0) {
        parent = *(keyword_status_t **)da_get(keyword_stack,
                                              da_size(keyword_stack) - 1);
    }

    keyword_status_t *kw = NULL;
    if (!da_pop(keyword_stack, sz - 1, &kw) || kw == NULL)
        return;

    if (!kw->ignored) {
        char time[TS_LEN];
        get_date_time_now(time);
        kw->finished = strdup(time);
    }
}

void keyword_status_init(ltf_state_t *state, const char *_blacklist_dir) {
    ltf_state = state;
    blacklist_dir = strdup(_blacklist_dir);
    keyword_stack = da_init(32, sizeof(keyword_status_t *));
    lua_hooks_add(LUA_HOOKCALL, call_hook);
    lua_hooks_add(LUA_HOOKRET, ret_hook);
    lua_hooks_add(LUA_HOOKTAILCALL, call_hook);

    ltf_state_register_test_started_cb(state, keyword_status_test_started);
    ltf_state_register_test_finished_cb(state, keyword_status_test_finished);
}
