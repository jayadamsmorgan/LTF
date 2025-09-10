#include "taf_logs.h"

#include "internal_logging.h"

#include "cmd_parser.h"
#include "keyword_status.h"
#include "project_parser.h"
#include "taf_state.h"

#include "taf_vars.h"
#include "util/files.h"

#include <json.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif // __APPLE__

#define RED_COLOR "\x1b[31m"
#define GREEN_COLOR "\x1b[32m"
#define YELLOW_COLOR "\x1b[33m"
#define BLUE_COLOR "\x1b[34m"
#define MAGENTA_COLOR "\x1b[35m"
#define CYAN_COLOR "\x1b[36m"

#define END_COLOR "\x1b[0m"

static taf_state_t *taf_logs_info_load_log(cmd_logs_info_options *opts) {

    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal_logging.\n");
        return NULL;
    }

    LOG("Starting taf logs info...");

    char log_file_path[PATH_MAX];

    if (!strcmp(opts->arg, "latest")) {
        LOG("Getting latest log...");
        if (project_parser_parse()) {
            internal_logging_deinit();
            return NULL;
        }
        project_parsed_t *proj = get_parsed_project();
        snprintf(log_file_path, PATH_MAX, "%s/logs/test_run_latest_raw.json",
                 proj->project_path);
    } else if (file_exists(opts->arg)) {
        snprintf(log_file_path, PATH_MAX, "%s", opts->arg);
    } else {
        LOG("Log file '%s' not found.", log_file_path);
        fprintf(stderr, "Log file %s not found.\n", opts->arg);
        internal_logging_deinit();
        return NULL;
    }

    LOG("Log path: %s", log_file_path);

    LOG("Getting JSON object...");
    json_object *root = json_object_from_file(log_file_path);
    taf_state_t *taf_state = taf_state_from_json(root);
    if (!taf_state || !taf_state->os || !taf_state->os_version) {
        LOG("Log file is incorrect or corrupt");
        fprintf(stderr, "Log file %s is either incorrect or corrupt.\n",
                log_file_path);
        internal_logging_deinit();
        return NULL;
    }

    return taf_state;
}

static void taf_logs_info_print_header(taf_state_t *taf_state) {
    printf("TAF Log Info for Project '%s':\n", taf_state->project_name);

    printf("├── TAF version: %s\n", taf_state->taf_version);
    printf("├── Host OS: %s\n", taf_state->os_version);
    printf("├── Started: %s\n", taf_state->started);
    printf("├── Finished: %s\n", taf_state->finished);

    if (taf_state->target && *taf_state->target)
        printf("├── Target: %s\n", taf_state->target);

    if (da_size(taf_state->tags)) {
        printf("├── Tags:\n");
        da_foreach(taf_state->tags, char *, tag) {
            const char *ch = tag_i == tags_count - 1 ? "└" : "│";
            printf("│   %s── %s\n", ch, *tag);
        }
    }

    if (da_size(taf_state->vars)) {
        printf("├── Variables:\n");
        da_foreach(taf_state->vars, taf_var_entry_t, var) {
            const char *ch = var_i == vars_count - 1 ? "└" : "│";
            printf("│   %s── %s = %s\n", ch, var->name, var->final_value);
        }
    }

    printf("└── Total Tests Performed: %zu\n\n", taf_state->total_amount);
}

static const char *log_level_color_map[6] = {
    RED_COLOR, RED_COLOR, YELLOW_COLOR, BLUE_COLOR, GREEN_COLOR, CYAN_COLOR,
};

static void taf_logs_info_print_test_outputs(da_t *outputs, const char *type,
                                             bool has_next) {
    size_t outputs_count = da_size(outputs);
    if (outputs_count == 0) {
        return;
    }

    const char *ch = has_next ? "├" : "└";
    printf("%s── %s:\n", ch, type);

    ch = has_next ? "│" : " ";

    da_foreach(outputs, taf_state_test_output_t, output) {
        const char *ch2 = output_i == outputs_count - 1 ? "└" : "├";
        printf("%s   %s── [%s][%s%s" END_COLOR "]: %s\n", ch, ch2,
               output->date_time, log_level_color_map[output->level],
               taf_log_level_to_str(output->level), output->msg);
    }
}

static void ensure_depth(da_t *v, size_t needed) {
    size_t cur = da_size(v);
    if (cur < needed) {
        da_resize(v, needed);
        for (size_t i = cur; i < needed; ++i) {
            bool z = false;
            da_set(v, i, &z);
        }
    }
}

static void taf_logs_info_put_prefix(da_t *has_more_at_level, size_t level) {
    for (size_t i = 0; i < level; ++i) {
        bool *more = (i < da_size(has_more_at_level))
                         ? (bool *)da_get(has_more_at_level, i)
                         : NULL;
        fputs((more && *more) ? "│       " : "        ", stdout);
    }
}

static void taf_logs_info_print_prop(da_t *has_more_at_level, size_t level,
                                     bool under_node_continues,
                                     bool is_last_prop, const char *label,
                                     const char *value) {
    taf_logs_info_put_prefix(has_more_at_level, level);
    fputs(under_node_continues ? "│   " : "    ", stdout);
    fputs(is_last_prop ? "└──" : "├──", stdout);
    fputc(' ', stdout);
    fputs(label, stdout);
    if (value)
        fputs(value, stdout);
    fputc('\n', stdout);
}

static void taf_logs_info_print_keyword_rec(keyword_status_t *keyword,
                                            da_t *has_more_at_level,
                                            size_t level,
                                            bool is_last_sibling) {
    /* Set ancestor continuation for this level (affects deeper siblings) */
    ensure_depth(has_more_at_level, level + 1);
    {
        bool cont = !is_last_sibling;
        da_set(has_more_at_level, level, &cont);
    }

    /* Node header */
    taf_logs_info_put_prefix(has_more_at_level, level);
    fputs(is_last_sibling ? "└──" : "├──", stdout);
    printf("[%s]\n", keyword->name);

    /* Facts */
    size_t children_count = da_size(keyword->children);
    bool have_children = (children_count != 0);
    bool finished = (keyword->finished && *keyword->finished);

    /* Properties: Started, (Finished), Declaration, (Children:) */
    int prop_total = 2 + (finished ? 1 : 0) + (have_children ? 1 : 0);
    int idx = 0;
    bool last_prop;

    /* Started */
    last_prop = (++idx == prop_total);
    taf_logs_info_print_prop(has_more_at_level, level, !is_last_sibling,
                             last_prop, "Started: ", keyword->started);

    /* Finished */
    if (finished) {
        last_prop = (++idx == prop_total);
        taf_logs_info_print_prop(has_more_at_level, level, !is_last_sibling,
                                 last_prop, "Finished: ", keyword->finished);
    }

    /* Declaration */
    char decl_buf[PATH_MAX + 32];
    snprintf(decl_buf, sizeof(decl_buf), "%s:%d", keyword->file, keyword->line);
    last_prop = (++idx == prop_total);
    taf_logs_info_print_prop(has_more_at_level, level, !is_last_sibling,
                             last_prop, "Declaration: ", decl_buf);

    if (have_children) {
        /* Children header is last prop when children exist.
           Under-node vertical should also respect is_last_sibling. */
        last_prop = (++idx == prop_total); /* true */
        taf_logs_info_print_prop(has_more_at_level, level, !is_last_sibling,
                                 last_prop, "Children:", NULL);

        /* Recurse */
        for (size_t i = 0; i < children_count; ++i) {
            keyword_status_t *child = da_get(keyword->children, i);
            bool child_is_last = (i == children_count - 1);
            taf_logs_info_print_keyword_rec(child, has_more_at_level, level + 1,
                                            child_is_last);
        }
    }
}

static void taf_logs_info_print_test_keyword_tree(taf_state_test_t *test) {
    size_t count = da_size(test->keyword_statuses);
    if (count == 0) {
        return;
    }

    da_t *has_more_at_level = da_init(4, sizeof(bool)); /* grows as needed */

    for (size_t i = 0; i < count; ++i) {
        keyword_status_t *kw = da_get(test->keyword_statuses, i);
        bool is_last = (i == count - 1);
        taf_logs_info_print_keyword_rec(kw, has_more_at_level, 0, is_last);
    }

    da_free(has_more_at_level);
}

static void taf_logs_info_print_test(taf_state_test_t *test,
                                     cmd_logs_info_options *opts) {
    /*
        ├─└│
     */
    const char *ch;
    printf("Test '%s':\n", test->name);
    if (da_size(test->tags)) {
        printf("├── Tags:\n");
        da_foreach(test->tags, char *, tag) {
            ch = tag_i == tags_count - 1 ? "└" : "├";
            printf("│   %s── %s\n", ch, *tag);
        }
    }

    printf("├── Started: %s\n", test->started);
    printf("├── Finished: %s\n", test->finished);

    bool has_failure_reasons = da_size(test->failure_reasons) != 0;
    bool has_outputs = da_size(test->outputs) != 0;
    bool has_teardown_outputs = da_size(test->teardown_outputs) != 0;
    bool has_teardown_errors = da_size(test->teardown_errors) != 0;

    bool has_keywords =
        opts->keyword_tree && da_size(test->keyword_statuses) != 0;

    ch = (opts->include_outputs &&
          (has_failure_reasons || has_outputs || has_teardown_outputs ||
           has_teardown_errors)) ||
                 has_keywords
             ? "├"
             : "└";

    printf("%s── Status: %s\n", ch, test->status_str);

    if (opts->include_outputs) {
        taf_logs_info_print_test_outputs(
            test->failure_reasons, "Failure reasons",
            has_outputs || has_teardown_outputs || has_teardown_errors ||
                has_keywords);
        taf_logs_info_print_test_outputs(
            test->outputs, "Outputs",
            has_teardown_outputs || has_teardown_errors || has_keywords);
        taf_logs_info_print_test_outputs(
            test->teardown_outputs, "Teardown Outputs", has_teardown_errors);
        taf_logs_info_print_test_outputs(test->teardown_errors,
                                         "Teardown Errors", has_keywords);
    }
    printf("\n");

    if (opts->keyword_tree) {
        taf_logs_info_print_test_keyword_tree(test);
    }
    printf("\n");
}

int taf_logs_info() {

    cmd_logs_info_options *opts = cmd_parser_get_logs_info_options();

    taf_state_t *taf_state = taf_logs_info_load_log(opts);
    if (!taf_state) {
        return EXIT_FAILURE;
    }

    taf_logs_info_print_header(taf_state);

    size_t tests_amount = da_size(taf_state->tests);
    for (size_t i = 0; i < tests_amount; i++) {
        taf_state_test_t *test = da_get(taf_state->tests, i);
        taf_logs_info_print_test(test, opts);
    }

    internal_logging_deinit();

    project_parser_free();

    return EXIT_SUCCESS;
}
