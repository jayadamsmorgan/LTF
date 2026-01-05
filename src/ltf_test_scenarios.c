#include "ltf_test_scenarios.h"

#include "util/files.h"
#include "util/kv.h"

#include <json.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    kv_pair_t *pairs;
    size_t count;
} kv_pairs_arr_t;

typedef struct {

    char *target;

    str_array_t tags;
    str_array_t tags_append;
    str_array_t tags_remove;

    kv_pairs_arr_t vars;
    kv_pairs_arr_t vars_append;
    str_array_t vars_remove;

    int log_level;
    bool no_logs;
    bool skip_hooks;
    bool headless;

    char *ltf_lib_path;

} ltf_test_scenario_cmd_t;

typedef struct ltf_test_scenario {

    char *file_path;

    char *include;
    struct ltf_test_scenario *parent;

    ltf_test_scenario_cmd_t cmd;

    str_array_t order;
    str_array_t order_append;
    str_array_t order_remove;

} ltf_test_scenario_t;

static int ltf_test_scenario_parse_err(const char *file_path, const char *err,
                                       ...)
    __attribute__((format(printf, 2, 3)));

static int ltf_test_scenario_parse_err(const char *file_path, const char *err,
                                       ...) {
    // TODO: get a full file_path instead ?

    fprintf(stderr, "Error parsing test scenario '%s':\n\t", file_path);
    va_list args;
    va_start(args, err);
    vfprintf(stderr, err, args);
    va_end(args);
    fprintf(stderr, "\n");

    return -1;
}

static void free_kv_pairs_arr(kv_pairs_arr_t *arr) {
    for (size_t i = 0; i < arr->count; ++i) {
        free(arr->pairs[i].key);
        free(arr->pairs[i].value);
    }
    free(arr->pairs);
    arr->pairs = NULL;
    arr->count = 0;
}

static void ltf_test_scenario_free(ltf_test_scenario_t *scenario) {
    if (scenario->parent) {
        ltf_test_scenario_free(scenario->parent);
        scenario->parent = NULL;
    }
    free(scenario->file_path);
    scenario->file_path = NULL;

    free(scenario->include);
    scenario->include = NULL;

    free(scenario->cmd.target);
    scenario->cmd.target = NULL;

    free_str_array(&scenario->cmd.tags);
    free_str_array(&scenario->cmd.tags_append);
    free_str_array(&scenario->cmd.tags_remove);

    free_kv_pairs_arr(&scenario->cmd.vars);
    free_kv_pairs_arr(&scenario->cmd.vars_append);
    free_str_array(&scenario->cmd.vars_remove);

    free(scenario->cmd.ltf_lib_path);
    scenario->cmd.ltf_lib_path = NULL;

    free_str_array(&scenario->order);
    free_str_array(&scenario->order_append);
    free_str_array(&scenario->order_remove);

    free(scenario);
}

static int json_array_to_str_array(const char *file_path, const char *name,
                                   json_object *o, str_array_t *out) {
    if (!json_object_is_type(o, json_type_array)) {
        return ltf_test_scenario_parse_err(
            file_path, "'%s' should be an array of strings.", name);
    }
    size_t array_sz = json_object_array_length(o);
    out->count = array_sz;
    if (array_sz != 0) {
        out->items = malloc(sizeof(*out->items) * array_sz);
    }
    for (size_t i = 0; i < array_sz; ++i) {
        json_object *include_obj = json_object_array_get_idx(o, i);
        if (!include_obj) {
            return ltf_test_scenario_parse_err(file_path,
                                               "Unable to parse '%s': %s", name,
                                               json_util_get_last_err());
        }
        if (!json_object_is_type(include_obj, json_type_string)) {
            return ltf_test_scenario_parse_err(
                file_path,
                "Unable to parse '%s': '%s' item with index %zu is not a "
                "string.",
                name, name, i);
        }
        out->items[i] = strdup(json_object_get_string(include_obj));
    }

    return 0;
}

static int json_object_to_kv_pairs_arr(const char *file_path, const char *name,
                                       json_object *o, kv_pairs_arr_t *out) {
    if (!json_object_is_type(o, json_type_object)) {
        return ltf_test_scenario_parse_err(file_path,
                                           "'%s' should be an object.", name);
    }

    size_t obj_len = json_object_object_length(o);
    out->count = obj_len;
    if (obj_len == 0) {
        return 0;
    }

    out->pairs = malloc(sizeof(kv_pair_t) * obj_len);
    json_object_iter iter;
    size_t i = 0;
    json_object_object_foreachC(o, iter) {
        if (!iter.val || !iter.key) {
            return ltf_test_scenario_parse_err(file_path,
                                               "Unable to parse '%s'", name);
        }
        if (!json_object_is_type(iter.val, json_type_string)) {
            return ltf_test_scenario_parse_err(
                file_path,
                "Unable to parse '%s': '%s.%s' value should be a string", name,
                name, iter.key);
        }
        out->pairs[i].key = strdup(iter.key);
        out->pairs[i].value = strdup(json_object_get_string(iter.val));
        i++;
    }

    return 0;
}

static int json_string_to_string(const char *file_path, const char *name,
                                 json_object *o, char **out) {
    if (json_object_is_type(o, json_type_string)) {
        *out = strdup(json_object_get_string(o));
        return 0;
    }

    return ltf_test_scenario_parse_err(
        file_path, "Unable to parse '%s': '%s' should be a string.", name,
        name);
}

#define ERR_CHECK(X)                                                           \
    do {                                                                       \
        err = (X);                                                             \
        if (err) {                                                             \
            return err;                                                        \
        }                                                                      \
    } while (0)

static int ltf_test_scenario_from_json(const char *file_path,
                                       ltf_test_scenario_t *out) {
    json_object *o = json_object_from_file(file_path);
    if (!o) {
        return ltf_test_scenario_parse_err(file_path, "JSON parsing error: %s",
                                           json_util_get_last_err());
    }

    json_object *v;
    int err;

    if (json_object_object_get_ex(o, "include", &v)) {
        ERR_CHECK(
            json_string_to_string(file_path, "include", v, &out->include));
    }

    if (json_object_object_get_ex(o, "cmd", &v)) {
        if (!json_object_is_type(v, json_type_object)) {
            return ltf_test_scenario_parse_err(file_path,
                                               "'cmd' should be an object.");
        }
        json_object *cmd_v;
        if (json_object_object_get_ex(v, "target", &cmd_v)) {
            ERR_CHECK(json_string_to_string(file_path, "cmd.target", cmd_v,
                                            &out->cmd.target));
        }
        if (json_object_object_get_ex(v, "skip_hooks", &cmd_v)) {
            if (!json_object_is_type(cmd_v, json_type_boolean)) {
                return ltf_test_scenario_parse_err(
                    file_path, "'cmd.no_logs' should be a boolean.");
            }
            out->cmd.skip_hooks = json_object_get_boolean(cmd_v);
        }
        if (json_object_object_get_ex(v, "log_level", &cmd_v)) {
            char *log_level_str;
            ERR_CHECK(json_string_to_string(file_path, "cmd.log_level", cmd_v,
                                            &log_level_str));
            int level = ltf_log_level_from_str(log_level_str);
            if (level == -1) {
                int res = ltf_test_scenario_parse_err(
                    file_path, "Unknown log level '%s'", log_level_str);
                free(log_level_str);
                return res;
            }
            free(log_level_str);
            out->cmd.log_level = level;
        } else {
            out->cmd.log_level = -1;
        }
        if (json_object_object_get_ex(v, "no_logs", &cmd_v)) {
            if (!json_object_is_type(cmd_v, json_type_boolean)) {
                return ltf_test_scenario_parse_err(
                    file_path, "'cmd.no_logs' should be a boolean.");
            }
            out->cmd.no_logs = json_object_get_boolean(cmd_v);
        }
        if (json_object_object_get_ex(v, "headless", &cmd_v)) {
            if (!json_object_is_type(cmd_v, json_type_boolean)) {
                return ltf_test_scenario_parse_err(
                    file_path, "'cmd.headless' should be a boolean.");
            }
            out->cmd.headless = json_object_get_boolean(cmd_v);
        }
        if (json_object_object_get_ex(v, "ltf_lib_path", &cmd_v)) {
            ERR_CHECK(json_string_to_string(file_path, "cmd.ltf_lib_path",
                                            cmd_v, &out->cmd.ltf_lib_path));
        }
        if (json_object_object_get_ex(v, "tags", &cmd_v)) {
            ERR_CHECK(json_array_to_str_array(file_path, "cmd.tags:", cmd_v,
                                              &out->cmd.tags));
        }
        if (json_object_object_get_ex(v, "tags:append", &cmd_v)) {
            ERR_CHECK(json_array_to_str_array(file_path, "cmd.tags:append",
                                              cmd_v, &out->cmd.tags_append));
        }
        if (json_object_object_get_ex(v, "tags:remove", &cmd_v)) {
            ERR_CHECK(json_array_to_str_array(file_path, "cmd.tags:remove",
                                              cmd_v, &out->cmd.tags_remove));
        }
        if (json_object_object_get_ex(v, "vars", &cmd_v)) {
            ERR_CHECK(json_object_to_kv_pairs_arr(file_path, "cmd.vars", cmd_v,
                                                  &out->cmd.vars));
        }
        if (json_object_object_get_ex(v, "vars:append", &cmd_v)) {
            ERR_CHECK(json_object_to_kv_pairs_arr(
                file_path, "cmd.vars:append", cmd_v, &out->cmd.vars_append));
        }
        if (json_object_object_get_ex(v, "vars:remove", &cmd_v)) {
            ERR_CHECK(json_array_to_str_array(file_path, "vars:remove", cmd_v,
                                              &out->cmd.vars_remove));
        }
    }

    if (json_object_object_get_ex(o, "order", &v)) {
        ERR_CHECK(json_array_to_str_array(file_path, "order", v, &out->order));
    }
    if (json_object_object_get_ex(o, "order:append", &v)) {
        ERR_CHECK(json_array_to_str_array(file_path, "order:append", v,
                                          &out->order_append));
    }
    if (json_object_object_get_ex(o, "order:remove", &v)) {
        ERR_CHECK(json_array_to_str_array(file_path, "order:remove", v,
                                          &out->order_remove));
    }

    json_object_put(o);

    return 0;
}

static char *include_file_path(const char *file_path, const char *include) {
    char *eq = strrchr(file_path, '/');
    if (!eq) {
        char *ret;
        asprintf(&ret, "%s.json", include);
        return ret;
    }

    char *dir_path = strndup(file_path, strlen(file_path) - strlen(eq + 1));

    char *ret;
    asprintf(&ret, "%s%s.json", dir_path, include);

    free(dir_path);

    return ret;
}

static void remove_from_string_da(da_t *da, str_array_t *arr) {
    for (size_t i = 0; i < arr->count; ++i) {
        for (size_t j = 0; j < da_size(da); ++j) {
            char *value = NULL;
            // peek
            char **slot = da_get(da, j);
            if (slot && *slot && strcmp(arr->items[i], *slot) == 0) {
                // extract a copy so we can free safely
                if (da_pop(da, j, &value)) {
                    free(value);
                }
                break;
            }
        }
    }
}

static void append_to_string_da(da_t *da, str_array_t *arr) {
    for (size_t i = 0; i < arr->count; ++i) {
        char *cpy = strdup(arr->items[i]);
        da_append(da, &cpy);
    }
}

static void replace_string_da(da_t *da, str_array_t *arr) {
    size_t sz = da_size(da);
    for (size_t i = sz; i-- > 0;) {
        char *tmp;
        if (da_pop(da, i, &tmp)) {
            free(tmp);
        }
    }
    append_to_string_da(da, arr);
}

static void remove_from_kv_pair_da(da_t *da, str_array_t *arr) {
    for (size_t i = 0; i < arr->count; ++i) {
        for (size_t j = 0; j < da_size(da); ++j) {
            kv_pair_t *pair = da_get(da, j);
            if (pair && strcmp(pair->key, arr->items[i]) == 0) {
                kv_pair_t tmp;
                if (da_pop(da, j, &tmp)) {
                    free(tmp.key);
                    free(tmp.value);
                }
                break;
            }
        }
    }
}

static void append_to_kv_pair_da(da_t *da, kv_pairs_arr_t *arr) {
    for (size_t i = 0; i < arr->count; ++i) {
        kv_pair_t copy = {
            .value = strdup(arr->pairs[i].value),
            .key = strdup(arr->pairs[i].key),
        };
        da_append(da, &copy);
    }
}

static void replace_kv_pair_da(da_t *da, kv_pairs_arr_t *arr) {
    size_t sz = da_size(da);
    for (size_t i = sz; i-- > 0;) {
        kv_pair_t tmp;
        if (da_pop(da, i, &tmp)) {
            free(tmp.key);
            free(tmp.value);
        }
    }
    append_to_kv_pair_da(da, arr);
}

static ltf_test_scenario_parsed_t parsed = {
    .log_level = LTF_LOG_LEVEL_INFO,
};

static int ltf_test_scenario_parse_file(const char *file_path,
                                        ltf_test_scenario_t **out) {
    if (!file_path) {
        return ltf_test_scenario_parse_err(file_path, "File path is NULL");
    }

    if (!file_exists(file_path)) {
        return ltf_test_scenario_parse_err(file_path, "File does not exist");
    }

    int err;
    ltf_test_scenario_t *s = calloc(1, sizeof(ltf_test_scenario_t));

    ERR_CHECK(ltf_test_scenario_from_json(file_path, s));
    s->file_path = strdup(file_path);

    if (s->include) {
        char *inc_file_path = include_file_path(file_path, s->include);
        if (!inc_file_path) {
            return ltf_test_scenario_parse_err(
                file_path, "Error getting relative path for file include '%s'",
                s->include);
        }
        ERR_CHECK(ltf_test_scenario_parse_file(inc_file_path, &s->parent));
        free(inc_file_path);
    }

    if (s->cmd.target) {
        if (parsed.target) {
            free(parsed.target);
        }
        parsed.target = strdup(s->cmd.target);
    }
    parsed.headless = s->cmd.headless;
    parsed.no_logs = s->cmd.no_logs;
    parsed.skip_hooks = s->cmd.skip_hooks;
    if (s->cmd.ltf_lib_path) {
        if (parsed.ltf_lib_path) {
            free(parsed.ltf_lib_path);
        }
        parsed.ltf_lib_path = strdup(s->cmd.ltf_lib_path);
    }
    if (s->cmd.log_level != -1) {
        parsed.log_level = s->cmd.log_level;
    }
    if (s->cmd.tags_append.count != 0) {
        append_to_string_da(parsed.tags, &s->cmd.tags_append);
    }
    if (s->cmd.tags_remove.count != 0) {
        remove_from_string_da(parsed.tags, &s->cmd.tags_remove);
    }
    if (s->cmd.tags.count != 0) {
        replace_string_da(parsed.tags, &s->cmd.tags);
    }
    if (s->cmd.vars_append.count != 0) {
        append_to_kv_pair_da(parsed.vars, &s->cmd.vars_append);
    }
    if (s->cmd.vars_remove.count != 0) {
        remove_from_kv_pair_da(parsed.vars, &s->cmd.vars_remove);
    }
    if (s->cmd.vars.count != 0) {
        replace_kv_pair_da(parsed.vars, &s->cmd.vars);
    }
    if (s->order_append.count != 0) {
        append_to_string_da(parsed.order, &s->order_append);
    }
    if (s->order_remove.count != 0) {
        remove_from_string_da(parsed.order, &s->order_remove);
    }
    if (s->order.count != 0) {
        replace_string_da(parsed.order, &s->order);
    }

    *out = s;

    return 0;
}

int ltf_test_scenario_parse(const char *file_path,
                            ltf_test_scenario_parsed_t *out) {
    parsed.order = da_init(3, sizeof(char *));
    parsed.tags = da_init(3, sizeof(char *));
    parsed.vars = da_init(3, sizeof(kv_pair_t));
    ltf_test_scenario_t *sc;
    int res = ltf_test_scenario_parse_file(file_path, &sc);
    if (res)
        return res;

    ltf_test_scenario_free(sc);

    *out = parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.log_level = LTF_LOG_LEVEL_INFO;

    return 0;
}
