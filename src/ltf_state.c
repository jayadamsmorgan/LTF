#include "ltf_state.h"

#include "cmd_parser.h"
#include "keyword_status.h"
#include "project_parser.h"
#include "ltf_hooks.h"
#include "ltf_test.h"
#include "ltf_vars.h"
#include "version.h"

#include "util/os.h"
#include "util/time.h"

#include <assert.h>
#include <string.h>

static inline char *jdup_string(struct json_object *o) {
    return o ? strdup(json_object_get_string(o)) : NULL;
}

static inline void add_string_if(json_object *obj, const char *key,
                                 const char *val) {
    if (val)
        json_object_object_add(obj, key, json_object_new_string(val));
}

static inline void add_string_len_if(json_object *obj, const char *key,
                                     const char *val, size_t len) {
    if (val)
        json_object_object_add(obj, key,
                               json_object_new_string_len(val, (int)len));
}

#define JGET_STR_DUP(OBJ, KEY, DEST)                                           \
    do {                                                                       \
        struct json_object *tmp__;                                             \
        if (json_object_object_get_ex((OBJ), (KEY), &tmp__)) {                 \
            (DEST) = jdup_string(tmp__);                                       \
        }                                                                      \
    } while (0)

#define JGET_INT(OBJ, KEY, DEST)                                               \
    do {                                                                       \
        struct json_object *tmp__;                                             \
        if (json_object_object_get_ex((OBJ), (KEY), &tmp__)) {                 \
            (DEST) = json_object_get_int(tmp__);                               \
        }                                                                      \
    } while (0)

static json_object *
ltf_state_test_output_to_json(const ltf_state_test_output_t *output) {
    json_object *o = json_object_new_object();
    add_string_if(o, "file", output->file);
    json_object_object_add(o, "line", json_object_new_int(output->line));
    add_string_if(o, "date_time", output->date_time);
    add_string_if(o, "level", ltf_log_level_to_str(output->level));
    add_string_len_if(o, "msg", output->msg, output->msg_len);
    return o;
}

static void ltf_state_test_output_from_json(json_object *jo,
                                            ltf_state_test_output_t *out) {
    memset(out, 0, sizeof *out);
    JGET_STR_DUP(jo, "file", out->file);
    JGET_STR_DUP(jo, "date_time", out->date_time);
    JGET_STR_DUP(jo, "msg", out->msg);
    if (out->msg)
        out->msg_len = strlen(out->msg);

    struct json_object *lvl;
    if (json_object_object_get_ex(jo, "level", &lvl))
        out->level = ltf_log_level_from_str(json_object_get_string(lvl));

    JGET_INT(jo, "line", out->line);
}

static json_object *da_strings_to_json_array(const da_t *arr) {
    json_object *a = json_object_new_array();
    size_t n = da_size((da_t *)arr);
    for (size_t i = 0; i < n; ++i) {
        const char *s = *(char *const *)da_get((da_t *)arr, i);
        json_object_array_add(a, json_object_new_string(s ? s : ""));
    }
    return a;
}

static da_t *json_array_to_da_strings(json_object *a) {
    if (!a || !json_object_is_type(a, json_type_array))
        return NULL;
    size_t n = (size_t)json_object_array_length(a);
    da_t *out = da_init(n, sizeof(char *));
    for (size_t i = 0; i < n; ++i) {
        json_object *ji = json_object_array_get_idx(a, (int)i);
        const char *s = json_object_get_string(ji);
        char *dup = s ? strdup(s) : NULL;
        if (!da_append(out, &dup)) {
            da_free(out);
            return NULL;
        }
    }
    return out;
}

static da_t *json_object_to_vars(json_object *v) {
    if (!v || !json_object_is_type(v, json_type_object))
        return NULL;

    da_t *out = da_init(1, sizeof(ltf_var_entry_t));
    json_object_iter i;
    json_object_object_foreachC(v, i) {
        ltf_var_entry_t e = {0};
        e.name = strdup(i.key);
        if (json_object_is_type(i.val, json_type_string)) {
            e.final_value = strdup(json_object_get_string(i.val));
        }
        da_append(out, &e);
    }

    return out;
}

static json_object *ltf_state_vars_to_json(da_t *vars) {
    json_object *o = json_object_new_object();

    size_t vars_size = da_size(vars);
    for (size_t i = 0; i < vars_size; ++i) {
        ltf_var_entry_t *var = da_get(vars, i);
        json_object_object_add(o, var->name,
                               json_object_new_string(var->final_value));
    }

    return o;
}

static json_object *da_outputs_to_json_array(const da_t *arr) {
    json_object *a = json_object_new_array();
    size_t n = da_size((da_t *)arr);
    for (size_t i = 0; i < n; ++i) {
        const ltf_state_test_output_t *o =
            (const ltf_state_test_output_t *)da_get((da_t *)arr, i);
        json_object_array_add(a, ltf_state_test_output_to_json(o));
    }
    return a;
}

static da_t *json_array_to_da_outputs(json_object *a) {
    if (!a || !json_object_is_type(a, json_type_array))
        return NULL;
    size_t n = (size_t)json_object_array_length(a);
    da_t *out = da_init(n, sizeof(ltf_state_test_output_t));
    for (size_t i = 0; i < n; ++i) {
        json_object *ji = json_object_array_get_idx(a, (int)i);
        ltf_state_test_output_t item;
        ltf_state_test_output_from_json(ji, &item);
        if (!da_append(out, &item)) {
            da_free(out);
            return NULL;
        }
    }
    return out;
}

static json_object *da_keywords_to_json_array(const da_t *arr);

static da_t *json_array_to_da_keywords(json_object *a);

static json_object *
ltf_state_test_keyword_to_json(const keyword_status_t *keyword) {
    json_object *o = json_object_new_object();
    add_string_if(o, "name", keyword->name);
    add_string_if(o, "started", keyword->started);
    add_string_if(o, "finished", keyword->finished);
    add_string_if(o, "file", keyword->file);
    json_object_object_add(o, "line", json_object_new_int(keyword->line));
    json_object_object_add(o, "children",
                           da_keywords_to_json_array(keyword->children));
    return o;
}

static void ltf_state_test_keyword_from_json(json_object *jk,
                                             keyword_status_t *out) {
    memset(out, 0, sizeof *out);
    JGET_STR_DUP(jk, "name", out->name);
    JGET_STR_DUP(jk, "started", out->started);
    JGET_STR_DUP(jk, "finished", out->finished);
    JGET_STR_DUP(jk, "file", out->file);

    JGET_INT(jk, "line", out->line);

    json_object *tmp;

    if (json_object_object_get_ex(jk, "children", &tmp))
        out->children = json_array_to_da_keywords(tmp);
}

static json_object *da_keywords_to_json_array(const da_t *arr) {
    json_object *a = json_object_new_array();
    size_t n = da_size((da_t *)arr);
    for (size_t i = 0; i < n; ++i) {
        const keyword_status_t *s =
            (const keyword_status_t *)da_get((da_t *)arr, i);
        json_object_array_add(a, ltf_state_test_keyword_to_json(s));
    }
    return a;
}

static da_t *json_array_to_da_keywords(json_object *a) {
    if (!a || !json_object_is_type(a, json_type_array))
        return NULL;
    size_t n = (size_t)json_object_array_length(a);
    da_t *out = da_init(n, sizeof(keyword_status_t));
    for (size_t i = 0; i < n; ++i) {
        json_object *jk = json_object_array_get_idx(a, (int)i);
        keyword_status_t item;
        ltf_state_test_keyword_from_json(jk, &item);
        if (!da_append(out, &item)) {
            da_free(out);
            return NULL;
        }
    }

    return out;
}

static json_object *ltf_state_test_to_json(const ltf_state_test_t *t) {
    json_object *o = json_object_new_object();

    add_string_if(o, "name", t->name);
    add_string_if(o, "description", t->description);
    add_string_if(o, "started", t->started);
    add_string_if(o, "finished", t->finished);
    add_string_if(o, "teardown_start", t->teardown_start);
    add_string_if(o, "status", t->status_str);

    json_object_object_add(o, "tags", da_strings_to_json_array(t->tags));
    json_object_object_add(o, "failure_reasons",
                           da_outputs_to_json_array(t->failure_reasons));
    json_object_object_add(o, "output", da_outputs_to_json_array(t->outputs));
    json_object_object_add(o, "teardown_output",
                           da_outputs_to_json_array(t->teardown_outputs));
    json_object_object_add(o, "teardown_errors",
                           da_outputs_to_json_array(t->teardown_errors));
    json_object_object_add(o, "keywords",
                           da_keywords_to_json_array(t->keyword_statuses));

    return o;
}

static void ltf_state_test_from_json(json_object *jt, da_t *tests) {

    ltf_state_test_t t = {0};

    JGET_STR_DUP(jt, "name", t.name);
    JGET_STR_DUP(jt, "description", t.description);
    JGET_STR_DUP(jt, "started", t.started);
    JGET_STR_DUP(jt, "finished", t.finished);
    JGET_STR_DUP(jt, "teardown_start", t.teardown_start);
    JGET_STR_DUP(jt, "status", t.status_str);

    json_object *tmp;

    if (json_object_object_get_ex(jt, "tags", &tmp))
        t.tags = json_array_to_da_strings(tmp);

    if (json_object_object_get_ex(jt, "failure_reasons", &tmp))
        t.failure_reasons = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "output", &tmp))
        t.outputs = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "teardown_output", &tmp))
        t.teardown_outputs = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "teardown_errors", &tmp))
        t.teardown_errors = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "keywords", &tmp))
        t.keyword_statuses = json_array_to_da_keywords(tmp);

    da_append(tests, &t);
}

/* ----- state <-> json (public API) ------------------------------------- */

json_object *ltf_state_to_json(ltf_state_t *state) {
    if (!state)
        return NULL;

    json_object *root = json_object_new_object();

    add_string_if(root, "project_name", state->project_name);
    add_string_if(root, "ltf_version", state->ltf_version);
    add_string_if(root, "os", state->os);
    add_string_if(root, "os_version", state->os_version);
    add_string_if(root, "started", state->started);
    add_string_if(root, "finished", state->finished);
    add_string_if(root, "target", state->target);

    json_object_object_add(root, "variables",
                           ltf_state_vars_to_json(state->vars));

    json_object_object_add(root, "tags",
                           state->tags ? da_strings_to_json_array(state->tags)
                                       : json_object_new_array());

    json_object *tests_arr = json_object_new_array();
    size_t tests_count = da_size(state->tests);
    for (size_t i = 0; i < tests_count; ++i) {
        ltf_state_test_t *test = da_get(state->tests, i);
        json_object_array_add(tests_arr, ltf_state_test_to_json(test));
    }
    json_object_object_add(root, "tests", tests_arr);

    json_object_object_add(root, "total_amount",
                           json_object_new_int((int)state->total_amount));
    json_object_object_add(root, "passed_amount",
                           json_object_new_int((int)state->passed_amount));
    json_object_object_add(root, "failed_amount",
                           json_object_new_int((int)state->failed_amount));
    json_object_object_add(root, "finished_amount",
                           json_object_new_int((int)state->finished_amount));

    return root;
}

ltf_state_t *ltf_state_from_json(json_object *root) {
    if (!root || !json_object_is_type(root, json_type_object))
        return NULL;

    ltf_state_t *state = (ltf_state_t *)calloc(1, sizeof *state);
    if (!state)
        return NULL;

    JGET_STR_DUP(root, "project_name", state->project_name);
    JGET_STR_DUP(root, "ltf_version", state->ltf_version);
    JGET_STR_DUP(root, "os", state->os);
    JGET_STR_DUP(root, "os_version", state->os_version);
    JGET_STR_DUP(root, "started", state->started);
    JGET_STR_DUP(root, "finished", state->finished);
    JGET_STR_DUP(root, "target", state->target);

    json_object *tmp;

    if (json_object_object_get_ex(root, "variables", &tmp))
        state->vars = json_object_to_vars(tmp);

    if (json_object_object_get_ex(root, "tags", &tmp))
        state->tags = json_array_to_da_strings(tmp);

    if (json_object_object_get_ex(root, "tests", &tmp) &&
        json_object_is_type(tmp, json_type_array)) {

        size_t n = (size_t)json_object_array_length(tmp);
        state->total_amount = n;
        state->tests = da_init(n, sizeof(ltf_state_test_t));

        for (size_t i = 0; i < n; ++i) {
            json_object *jt = json_object_array_get_idx(tmp, (int)i);
            ltf_state_test_from_json(jt, state->tests);
        }
    }

    JGET_INT(root, "passed_amount", state->passed_amount);
    JGET_INT(root, "failed_amount", state->failed_amount);
    JGET_INT(root, "finished_amount", state->finished_amount);

    return state;
}

void ltf_state_test_started(ltf_state_t *state, test_case_t *test_case) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_t test = {0};
    test.started = strdup(time);
    test.name = strdup(test_case->name);
    test.description = test_case->desc ? strdup(test_case->desc) : NULL;
    test.status = TEST_STATUS_RUNNING;
    size_t tags_size = da_size(test_case->tags);
    test.tags = da_init(tags_size, sizeof(char *));
    for (size_t i = 0; i < tags_size; ++i) {
        char **tag = da_get(test_case->tags, i);
        char *cpy = strdup(*tag);
        da_append(test.tags, &cpy);
    }
    test.outputs = da_init(1, sizeof(ltf_state_test_output_t));
    test.failure_reasons = da_init(1, sizeof(ltf_state_test_output_t));
    test.teardown_outputs = da_init(1, sizeof(ltf_state_test_output_t));
    test.teardown_errors = da_init(1, sizeof(ltf_state_test_output_t));

    da_append(state->tests, &test);

    size_t count = da_size(state->test_started_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_cb *cb = da_get(state->test_started_cbs, i);
        if (cb && *cb) {
            (*cb)(&test);
        }
    }
}

static ltf_state_test_t *ltf_state_get_current_test(ltf_state_t *state) {
    size_t tests_count = da_size(state->tests);
    assert(tests_count != 0);
    ltf_state_test_t *test = da_get(state->tests, tests_count - 1);
    return test;
}

void ltf_state_log(ltf_state_t *state, ltf_log_level level, const char *file,
                   int line, const char *buffer, size_t buffer_len) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_output_t o = {
        .file = file ? strdup(file) : strdup("unknown"),
        .line = line,
        .level = level,
        .date_time = strdup(time),
        .msg = strndup(buffer, buffer_len),
        .msg_len = buffer_len,
    };

    if (state->current_stage == TEST_STAGE) {
        ltf_state_test_t *test = ltf_state_get_current_test(state);

        switch (test->status) {
        case TEST_STATUS_RUNNING:
            da_append(test->outputs, &o);
            if (level == LTF_LOG_LEVEL_ERROR) {
                ltf_state_test_output_t o = {
                    .file = file ? strdup(file) : strdup("unknown"),
                    .line = line,
                    .level = level,
                    .date_time = strdup(time),
                    .msg = strndup(buffer, buffer_len),
                    .msg_len = buffer_len,
                };
                da_append(test->failure_reasons, &o);
                ltf_mark_test_failed();
            }
            break;
        case TEST_STATUS_TEARDOWN_AFTER_PASSED:
        case TEST_STATUS_TEARDOWN_AFTER_FAILED:
            da_append(test->teardown_outputs, &o);
            break;
        default:
            break;
        }

        size_t count = da_size(state->test_log_cbs);
        for (size_t i = 0; i < count; ++i) {
            test_log_cb *cb = da_get(state->test_log_cbs, i);
            if (cb && *cb) {
                (*cb)(test, &o);
            }
        }
    } else if (state->current_stage == HOOK_STAGE) {
        size_t count = da_size(state->hook_log_cbs);
        for (size_t i = 0; i < count; ++i) {
            hook_log_cb *cb = da_get(state->hook_log_cbs, i);
            if (cb && *cb) {
                (*cb)(&o);
            }
        }
    }
}

void ltf_state_test_set_keyword_statuses(ltf_state_t *state, da_t *statuses) {
    ltf_state_test_t *test = ltf_state_get_current_test(state);
    test->keyword_statuses = statuses;
}

void ltf_state_test_defer_queue_started(ltf_state_t *state) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_t *test = ltf_state_get_current_test(state);

    test->teardown_start = strdup(time);

    if (test->status == TEST_STATUS_FAILED)
        test->status = TEST_STATUS_TEARDOWN_AFTER_FAILED;
    if (test->status == TEST_STATUS_PASSED)
        test->status = TEST_STATUS_TEARDOWN_AFTER_PASSED;

    size_t count = da_size(state->test_teardown_started_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_cb *cb = da_get(state->test_teardown_started_cbs, i);
        if (cb && *cb) {
            (*cb)(test);
        }
    }
}

void ltf_state_test_defer_failed(ltf_state_t *state, const char *file, int line,
                                 const char *msg) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_output_t o = {
        .file = file ? strdup(file) : strdup("unknown"),
        .line = line,
        .msg = strdup(msg),
        .msg_len = strlen(msg),
        .date_time = strdup(time),
    };

    ltf_state_test_t *test = ltf_state_get_current_test(state);

    da_append(test->teardown_errors, &o);

    size_t count = da_size(state->test_defer_failed_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_log_cb *cb = da_get(state->test_defer_failed_cbs, i);
        if (cb && *cb) {
            (*cb)(test, &o);
        }
    }
}

void ltf_state_test_defer_queue_finished(ltf_state_t *state) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_t *test = ltf_state_get_current_test(state);

    test->teardown_end = strdup(time);

    if (test->status == TEST_STATUS_TEARDOWN_AFTER_FAILED)
        test->status = TEST_STATUS_FAILED;
    if (test->status == TEST_STATUS_TEARDOWN_AFTER_PASSED)
        test->status = TEST_STATUS_PASSED;

    size_t count = da_size(state->test_teardown_finished_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_cb *cb = da_get(state->test_teardown_finished_cbs, i);
        if (cb && *cb) {
            (*cb)(test);
        }
    }
}

void ltf_state_test_passed(ltf_state_t *state) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_t *test = ltf_state_get_current_test(state);

    test->status = TEST_STATUS_PASSED;
    test->status_str = strdup("PASSED");
    test->finished = strdup(time);
    state->passed_amount++;
    state->finished_amount++;

    size_t count = da_size(state->test_finished_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_cb *cb = da_get(state->test_finished_cbs, i);
        if (cb && *cb) {
            (*cb)(test);
        }
    }
}

void ltf_state_test_failed(ltf_state_t *state, const char *file, int line,
                           const char *msg) {
    char time[TS_LEN];
    get_date_time_now(time);

    ltf_state_test_t *test = ltf_state_get_current_test(state);

    test->finished = strdup(time);
    test->status = TEST_STATUS_FAILED;
    test->status_str = strdup("FAILED");
    state->finished_amount++;
    state->failed_amount++;

    if (msg) {
        ltf_state_test_output_t o = {
            .file = file ? strdup(file) : strdup("unknown"),
            .date_time = strdup(time),
            .msg = strdup(msg),
            .msg_len = strlen(msg),
            .line = line,
            .level = LTF_LOG_LEVEL_CRITICAL,
        };
        da_append(test->failure_reasons, &o);
    }

    size_t count = da_size(state->test_finished_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_cb *cb = da_get(state->test_finished_cbs, i);
        if (cb && *cb) {
            (*cb)(test);
        }
    }
}

void ltf_state_test_run_started(ltf_state_t *state) {
    char time[TS_LEN];
    get_date_time_now(time);

    state->started = strdup(time);

    size_t count = da_size(state->test_run_started_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_run_cb *cb = da_get(state->test_run_started_cbs, i);
        if (cb && *cb) {
            (*cb)();
        }
    }
}

void ltf_state_test_run_finished(ltf_state_t *state) {
    char time[TS_LEN];
    get_date_time_now(time);

    state->finished = strdup(time);

    size_t count = da_size(state->test_run_finished_cbs);
    for (size_t i = 0; i < count; ++i) {
        test_run_cb *cb = da_get(state->test_run_finished_cbs, i);
        if (cb && *cb) {
            (*cb)();
        }
    }
}

ltf_state_t *ltf_state_new() {
    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();

    size_t amount = da_size(test_case_get_all());

    ltf_state_t *ltf_state = calloc(1, sizeof *ltf_state);
    ltf_state->project_name = strdup(proj->project_name);
    ltf_state->tests = da_init(amount, sizeof(ltf_state_test_t));
    ltf_state->total_amount = amount;
    size_t tags_size = da_size(opts->tags);
    ltf_state->tags = da_init(tags_size, sizeof(char *));
    ltf_state->current_stage = TEST_STAGE;
    for (size_t i = 0; i < tags_size; i++) {
        char **tag = da_get(opts->tags, i);
        char *to_cpy = strdup(*tag);
        da_append(ltf_state->tags, &to_cpy);
    }
    da_t *vars = ltf_get_vars();
    if (vars) {
        size_t vars_size = da_size(vars);
        ltf_state->vars = da_init(vars_size, sizeof(ltf_var_entry_t));
        for (size_t i = 0; i < vars_size; ++i) {
            ltf_var_entry_t *e = da_get(vars, i);
            size_t values_count = da_size(e->values);
            da_t *values_cpy = da_init(values_count, sizeof(char *));
            for (size_t j = 0; j < values_count; ++j) {
                char **value = da_get(e->values, j);
                char *tmp = strdup(*value);
                da_append(values_cpy, &tmp);
            }
            ltf_var_entry_t cpy = {
                .name = strdup(e->name),
                .values = values_cpy,
                .is_scalar = e->is_scalar,
                .scalar = e->scalar ? strdup(e->scalar) : NULL,
                .def_value = e->def_value ? strdup(e->def_value) : NULL,
                .final_value = strdup(e->final_value),
            };
            da_append(ltf_state->vars, &cpy);
        }
    }

    ltf_state->test_run_started_cbs = da_init(1, sizeof(test_run_cb));
    ltf_state->test_run_finished_cbs = da_init(1, sizeof(test_run_cb));
    ltf_state->test_started_cbs = da_init(1, sizeof(test_cb));
    ltf_state->test_finished_cbs = da_init(1, sizeof(test_cb));
    ltf_state->test_teardown_finished_cbs = da_init(1, sizeof(test_cb));
    ltf_state->test_teardown_started_cbs = da_init(1, sizeof(test_cb));
    ltf_state->test_defer_failed_cbs = da_init(1, sizeof(test_log_cb));
    ltf_state->test_log_cbs = da_init(1, sizeof(test_log_cb));

    ltf_state->ltf_version = strdup(LTF_VERSION);
    if (opts->target) {
        ltf_state->target = strdup(opts->target);
    } else {
        ltf_state->target = NULL;
    }
#if defined(__APPLE__)
    ltf_state->os = strdup("macos");
#elif defined(__linux__)
    ltf_state->os = strdup("linux");
#elif defined(_WIN32) || defined(_WIN64)
    ltf_state->os = strdup("windows");
#else
    ltf_state->os = strdup("unknown");
#endif
    ltf_state->os_version = get_os_string();

    return ltf_state;
}

void ltf_state_register_test_run_started_cb(ltf_state_t *state,
                                            test_run_cb cb) {
    da_append(state->test_run_started_cbs, (void *)&cb);
}

void ltf_state_register_test_run_finished_cb(ltf_state_t *state,
                                             test_run_cb cb) {
    da_append(state->test_run_finished_cbs, (void *)&cb);
}

void ltf_state_register_test_started_cb(ltf_state_t *state, test_cb cb) {
    da_append(state->test_started_cbs, (void *)&cb);
}

void ltf_state_register_test_finished_cb(ltf_state_t *state, test_cb cb) {
    da_append(state->test_finished_cbs, (void *)&cb);
}

void ltf_state_register_test_log_cb(ltf_state_t *state, test_log_cb cb) {
    da_append(state->test_log_cbs, (void *)&cb);
}

void ltf_state_register_test_teardown_started_cb(ltf_state_t *state,
                                                 test_cb cb) {
    da_append(state->test_teardown_started_cbs, (void *)&cb);
}

void ltf_state_register_test_teardown_finished_cb(ltf_state_t *state,
                                                  test_cb cb) {
    da_append(state->test_teardown_finished_cbs, (void *)&cb);
}

void ltf_state_register_test_defer_failed_cb(ltf_state_t *state,
                                             test_log_cb cb) {
    da_append(state->test_defer_failed_cbs, (void *)&cb);
}

static void ltf_state_test_output_free(ltf_state_test_output_t *o) {
    if (!o)
        return;
    free(o->file);
    free(o->date_time);
    free(o->msg);
    o->file = o->date_time = o->msg = NULL;
}

static void da_free_vars(da_t *vars) {
    if (!vars)
        return;
    size_t n = da_size(vars);
    for (size_t i = 0; i < n; ++i) {
        ltf_var_entry_t *var = da_get(vars, i);
        if (var) {
            free(var->name);
            free(var->def_value);
            free(var->final_value);
            free(var->scalar);
            size_t values_size = da_size(var->values);
            for (size_t j = 0; j < values_size; j++) {
                char **value = da_get(var->values, j);
                free(*value);
            }
            da_free(var->values);
        }
    }
    da_free(vars);
}

static void da_free_strings(da_t *arr) {
    if (!arr)
        return;
    size_t n = da_size(arr);
    for (size_t i = 0; i < n; ++i) {
        char **p = da_get(arr, i);
        if (p && *p) {
            free(*p);
        }
    }
    da_free(arr);
}

static void da_free_outputs(da_t *arr) {
    if (!arr)
        return;
    size_t n = da_size(arr);
    for (size_t i = 0; i < n; ++i) {
        ltf_state_test_output_t *o = da_get(arr, i);
        ltf_state_test_output_free(o);
    }
    da_free(arr);
}

static void free_keyword_status(keyword_status_t *status) {
    size_t children_count = da_size(status->children);
    for (size_t i = 0; i < children_count; ++i) {
        keyword_status_t *child = da_get(status->children, i);
        free_keyword_status(child);
    }
    da_free(status->children);
    free(status->name);
    free(status->started);
    free(status->finished);
    free(status->file);
}

static void ltf_state_test_free(ltf_state_test_t *t) {
    if (!t)
        return;

    free(t->name);
    free(t->description);
    free(t->started);
    free(t->finished);
    free(t->teardown_start);
    free(t->status_str);

    size_t keyword_statuses_count = da_size(t->keyword_statuses);
    for (size_t i = 0; i < keyword_statuses_count; ++i) {
        keyword_status_t *status = da_get(t->keyword_statuses, i);
        free_keyword_status(status);
    }

    da_free_strings(t->tags);
    da_free_outputs(t->failure_reasons);
    da_free_outputs(t->outputs);
    da_free_outputs(t->teardown_outputs);
    da_free_outputs(t->teardown_errors);
}

static void ltf_state_tests_free(da_t *tests) {
    size_t tests_count = da_size(tests);
    for (size_t i = 0; i < tests_count; ++i) {
        ltf_state_test_t *test = da_get(tests, i);
        ltf_state_test_free(test);
    }
    da_free(tests);
}

void ltf_state_free(ltf_state_t *state) {
    if (!state)
        return;

    free(state->project_name);
    free(state->ltf_version);
    free(state->os);
    free(state->os_version);
    free(state->started);
    free(state->finished);
    free(state->target);

    da_free(state->test_run_started_cbs);
    da_free(state->test_run_finished_cbs);
    da_free(state->test_started_cbs);
    da_free(state->test_finished_cbs);
    da_free(state->test_teardown_started_cbs);
    da_free(state->test_teardown_finished_cbs);
    da_free(state->test_defer_failed_cbs);
    da_free(state->test_log_cbs);

    da_free_vars(state->vars);
    da_free_strings(state->tags);

    ltf_state_tests_free(state->tests);

    free(state);
}
