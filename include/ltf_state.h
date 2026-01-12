#ifndef RAW_LOG_H
#define RAW_LOG_H

#include "ltf_log_level.h"
#include "test_case.h"

#include "util/da.h"

#include <json.h>

#include <stddef.h>

typedef struct {
    char *file;
    int line;
    char *date_time;
    ltf_log_level level;
    char *msg;
    size_t msg_len;
} ltf_state_test_output_t;

typedef enum {
    TEST_STATUS_RUNNING = 0U,
    TEST_STATUS_TEARDOWN_AFTER_FAILED = 1U,
    TEST_STATUS_TEARDOWN_AFTER_PASSED = 2U,
    TEST_STATUS_FAILED = 3U,
    TEST_STATUS_PASSED = 4U,
} ltf_state_test_status;

typedef enum {
    HOOK_STAGE = 0U,
    TEST_STAGE = 1U,
    TEARDOWN_STAGE = 2U,
} ltf_state_stage_t;

typedef struct {
    char *name;
    char *description;
    char *started;
    char *finished;
    char *teardown_start;
    char *teardown_end;

    ltf_state_test_status status;
    char *status_str;

    da_t *tags;

    da_t *failure_reasons;
    da_t *outputs;
    da_t *teardown_outputs;
    da_t *teardown_errors;

    da_t *keyword_statuses;

} ltf_state_test_t;

typedef void (*test_run_cb)();
typedef void (*test_cb)(ltf_state_test_t *);
typedef void (*test_log_cb)(ltf_state_test_t *, ltf_state_test_output_t *);

typedef struct {
    char *project_name;
    char *ltf_version;
    char *os;
    char *os_version;
    char *started;
    char *finished;
    char *target;

    size_t total_amount;
    size_t passed_amount;
    size_t failed_amount;
    size_t finished_amount;

    da_t *vars;

    da_t *tags;

    da_t *tests;

    da_t *hooks_test_run_started;
    da_t *hooks_test_started;
    da_t *hooks_test_finished;
    da_t *hooks_test_run_finished;

    ltf_state_stage_t current_stage;

    da_t *hook_started_cbs;  // hook_cb
    da_t *hook_finished_cbs; // hook_cb
    da_t *hook_failed_cbs;   // hook_err_cb
    da_t *hook_log_cbs;      // hook_log_cb

    da_t *test_run_started_cbs;  // test_run_cb
    da_t *test_run_finished_cbs; // test_run_cb

    da_t *test_started_cbs;  // test_cb
    da_t *test_finished_cbs; // test_cb
    da_t *test_log_cbs;      // test_log_cb

    da_t *test_teardown_started_cbs;  // test_cb
    da_t *test_teardown_finished_cbs; // test_cb
    da_t *test_defer_failed_cbs;      // test_log_cb

} ltf_state_t;

json_object *ltf_state_to_json(ltf_state_t *log);

ltf_state_t *ltf_state_from_json(json_object *obj);

ltf_state_t *ltf_state_new();

void ltf_state_test_run_started(ltf_state_t *state);

void ltf_state_test_run_finished(ltf_state_t *state);

void ltf_state_test_failed(ltf_state_t *state, const char *file, int line,
                           const char *msg);

void ltf_state_test_passed(ltf_state_t *state);

void ltf_state_test_set_keyword_statuses(ltf_state_t *state, da_t *statuses);

void ltf_state_test_defer_queue_finished(ltf_state_t *state);

void ltf_state_test_defer_failed(ltf_state_t *state, const char *file, int line,
                                 const char *msg);

void ltf_state_test_defer_queue_started(ltf_state_t *state);

void ltf_state_log(ltf_state_t *state, ltf_log_level level, const char *file,
                   int line, const char *buffer, size_t buffer_len);

void ltf_state_test_started(ltf_state_t *state, test_case_t *test_case);

void ltf_state_free(ltf_state_t *state);

void ltf_state_register_vars(ltf_state_t *ltf_state);

void ltf_state_register_test_run_started_cb(ltf_state_t *state, test_run_cb cb);
void ltf_state_register_test_run_finished_cb(ltf_state_t *state,
                                             test_run_cb cb);
void ltf_state_register_test_started_cb(ltf_state_t *state, test_cb cb);
void ltf_state_register_test_finished_cb(ltf_state_t *state, test_cb cb);
void ltf_state_register_test_log_cb(ltf_state_t *state, test_log_cb cb);
void ltf_state_register_test_teardown_started_cb(ltf_state_t *state,
                                                 test_cb cb);
void ltf_state_register_test_teardown_finished_cb(ltf_state_t *state,
                                                  test_cb cb);
void ltf_state_register_test_defer_failed_cb(ltf_state_t *state,
                                             test_log_cb cb);

#endif // RAW_LOG_H
