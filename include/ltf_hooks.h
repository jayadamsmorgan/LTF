#ifndef LTF_HOOKS_H
#define LTF_HOOKS_H

#include "ltf_state.h"

typedef enum {
    LTF_HOOK_FN_TEST_RUN_STARTED = 0,
    LTF_HOOK_FN_TEST_STARTED = 1,
    LTF_HOOK_FN_TEST_FINISHED = 2,
    LTF_HOOK_FN_TEST_RUN_FINISHED = 3,
} ltf_hook_fn;

typedef struct {
    int ref;
    ltf_hook_fn fn;
} ltf_hook_t;

void ltf_hooks_init(ltf_state_t *state);

void ltf_hooks_add_to_queue(ltf_hook_t hook);

void ltf_hooks_run(lua_State *L, ltf_hook_fn fn);

typedef void (*hook_cb)(ltf_hook_fn);
typedef void (*hook_err_cb)(ltf_hook_fn, const char *trace);
typedef void (*hook_log_cb)(ltf_state_test_output_t *);

void ltf_hooks_register_hook_started_cb(hook_cb cb);
void ltf_hooks_register_hook_finished_cb(hook_cb cb);
void ltf_hooks_register_hook_failed_cb(hook_err_cb cb);
void ltf_hooks_register_hook_log_cb(hook_log_cb cb);
void ltf_hooks_deinit(lua_State *L);

#endif // LTF_HOOKS_H
