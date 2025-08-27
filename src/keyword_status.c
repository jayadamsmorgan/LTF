#include "keyword_status.h"

#include "util/lua_hooks.h"
#include "util/time.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static da_t *keyword_statuses = NULL;

static size_t stack_count = 0;

static taf_state_t *taf_state = NULL;

static bool test_running = false;

static void keyword_status_test_started(taf_state_test_t *) {
    test_running = true;
    stack_count = 0;
    keyword_statuses = da_init(10, sizeof(keyword_status_t));
}

static void keyword_status_test_finished(taf_state_test_t *) {
    test_running = false;
    taf_state_test_set_keyword_statuses(taf_state, keyword_statuses);
    stack_count = 0;
    // Freeing keywords is handled in taf_state.c,
    // so here we just NULLify them just in case
    keyword_statuses = NULL;
}

static da_t *get_stack_by_level(size_t stack_count) {
    da_t *cur = keyword_statuses;
    while (stack_count > 2) {
        size_t size = da_size(cur);
        assert(size != 0);
        keyword_status_t *status = da_get(cur, size - 1);
        cur = status->children;
        stack_count--;
    }
    return cur;
}

static void call_hook(lua_State *, lua_Debug *ar, const char *src) {

    if (!test_running)
        return;

    stack_count++;

    if (stack_count == 1) {
        // Means we just entered test scope.
        return;
    }

    da_t *stack = get_stack_by_level(stack_count);

    char time[TS_LEN];
    get_date_time_now(time);

    keyword_status_t ks = {
        .children = da_init(10, sizeof(keyword_status_t)),
        .name = strdup(ar->name ? ar->name : "(undefined)"),
        .started = strdup(time),
        .file = strdup(src),
        .line = ar->linedefined,
        .finished = NULL,
    };

    da_append(stack, &ks);
}

static void ret_hook(lua_State *, lua_Debug *, const char *) {

    if (!test_running)
        return;

    stack_count--;

    if (stack_count == 0) {
        // Means we just exited test scope.
        // The test was either passed or marked failed.
        // If it would truly error and fail, we should not get here.
        // That's why the final handling to the taf_state should be in
        // `keyword_status_test_finished()`
        return;
    }

    char time[TS_LEN];
    get_date_time_now(time);

    da_t *stack = get_stack_by_level(stack_count + 1);
    size_t size = da_size(stack);
    assert(size != 0);
    keyword_status_t *status = da_get(stack, size - 1);
    status->finished = strdup(time);
}

void keyword_status_init(taf_state_t *state) {
    taf_state = state;
    lua_hooks_add(LUA_HOOKCALL, call_hook);
    lua_hooks_add(LUA_HOOKRET, ret_hook);

    taf_state_register_test_started_cb(state, keyword_status_test_started);
    taf_state_register_test_finished_cb(state, keyword_status_test_finished);
}
