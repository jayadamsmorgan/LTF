#include "test_logs.h"

#include "cmd_parser.h"
#include "internal_logging.h"
#include "ltf_state.h"
#include "project_parser.h"

#include "util/files.h"
#include "util/time.h"

#include <json.h>

#include <stdio.h>
#include <string.h>

static ltf_log_level log_level;

static FILE *output_log_file;

static char *logs_dir;
static char *output_log_file_path;
static char *raw_log_file_path;

static ltf_state_t *ltf_state = NULL;

char *ltf_log_get_logs_dir() {
    //
    return logs_dir;
}

char *ltf_log_get_output_log_file_path() {
    //
    return output_log_file_path;
}

char *ltf_log_get_raw_log_file_path() {
    //
    return raw_log_file_path;
}

void ltf_log_test(ltf_state_test_t *test, ltf_state_test_output_t *output) {

    if (output->level <= log_level) {
        LOG("Writing to output log file...");
        fprintf(output_log_file, "[%s][%s][%s][%s:%d]: ", output->date_time,
                ltf_log_level_to_str(output->level), test->name, output->file,
                output->line);
        fwrite(output->msg, 1, output->msg_len, output_log_file);
        fputs("\n\n", output_log_file);
        LOG("Wrote to output log file.");
    }

    LOG("Successfully LTF logged.");
}

void ltf_log_test_started(ltf_state_test_t *test) {

    fprintf(output_log_file, "[%s][%s]: Test Started.\n\n", test->started,
            test->name);
    LOG("Wrote to output log file");
}

void ltf_log_test_finished(ltf_state_test_t *test) {

    if (test->status == TEST_STATUS_PASSED) {
        fprintf(output_log_file, "[%s][%s]: Test Passed.\n\n", test->finished,
                test->name);
        LOG("Wrote to output log file");
    }
    if (test->status == TEST_STATUS_FAILED) {
        fprintf(output_log_file, "[%s][%s]: Test Failed.\n\n", test->finished,
                test->name);
        LOG("Wrote to output log file");
    }
}

void ltf_log_defer_queue_started(ltf_state_test_t *test) {
    fprintf(output_log_file, "[%s][%s]: Defer Queue Started.\n\n",
            test->teardown_start, test->name);
    LOG("Wrote to output log file");
}

void ltf_log_defer_queue_finished(ltf_state_test_t *test) {
    fprintf(output_log_file, "[%s][%s]: Defer Queue Finished.\n\n",
            test->teardown_end, test->name);
    LOG("Wrote to output log file");
}

void ltf_log_defer_failed(ltf_state_test_t *test,
                          ltf_state_test_output_t *output) {
    fprintf(output_log_file,
            "[%s][%s]: Defer failed (%s at %d), traceback: \n%s\n\n",
            output->date_time, test->name, output->file, output->line,
            output->msg);
    LOG("Wrote to output log file");
}

void ltf_log_test_run_finished() {

    json_object *ltf_state_root = ltf_state_to_json(ltf_state);

    LOG("Saving raw log file...");
    if (json_object_to_file_ext(raw_log_file_path, ltf_state_root,
                                JSON_C_TO_STRING_SPACED |
                                    JSON_C_TO_STRING_PRETTY |
                                    JSON_C_TO_STRING_NOSLASHESCAPE) == -1) {
        LOG("Unable to save raw log file: %s", json_util_get_last_err());
    }
    LOG("Freeing JSON object...");
    json_object_put(ltf_state_root);

    char *latest_log = NULL;
    char *latest_raw = NULL;

    LOG("Flushing and closing output log file...");
    fflush(output_log_file);
    fclose(output_log_file);

    // Create 'latest' symlinks:
    project_parsed_t *proj = get_parsed_project();
    if (proj->multitarget) {
        asprintf(&latest_log, "%s/test_run_latest_output.log", logs_dir);
        asprintf(&latest_raw, "%s/test_run_latest_raw.json", logs_dir);
    } else {
        asprintf(&latest_log, "%s/test_run_latest_output.log", logs_dir);
        asprintf(&latest_raw, "%s/test_run_latest_raw.json", logs_dir);
    }

    LOG("Creating symlinks '%s' and '%s'...", latest_log, latest_raw);
    replace_symlink(output_log_file_path, latest_log);
    replace_symlink(raw_log_file_path, latest_raw);

    if (proj->multitarget) {
        free(latest_log);
        asprintf(&latest_log, "%s/logs/test_run_latest_output.log",
                 proj->project_path);
        free(latest_raw);
        asprintf(&latest_raw, "%s/logs/test_run_latest_raw.json",
                 proj->project_path);
        replace_symlink(output_log_file_path, latest_log);
        replace_symlink(raw_log_file_path, latest_raw);
    }

    free(latest_raw);
    free(latest_log);

    LOG("Successfully finalized LTF logging.");
}

void ltf_log_init(ltf_state_t *state) {

    LOG("Starting LTF test logging...");

    ltf_state = state;

    cmd_test_options *opts = cmd_parser_get_test_options();

    log_level = opts->log_level;
    LOG("Log level: %s", ltf_log_level_to_str(log_level));

    project_parsed_t *proj = get_parsed_project();

    if (proj->multitarget) {
        asprintf(&logs_dir, "%s/logs/%s", proj->project_path, opts->target);
    } else {
        asprintf(&logs_dir, "%s/logs", proj->project_path);
    }
    LOG("Logs directory path: %s", logs_dir);
    if (!directory_exists(logs_dir)) {
        LOG("Logs directory doesn't exist, creating...");
        if (create_directory(logs_dir, MKDIR_MODE)) {
            LOG("Unable to create logs directory.");
            internal_logging_deinit();
            exit(EXIT_FAILURE);
        }
    }

    char time[TS_LEN];
    get_date_time_now(time);

    asprintf(&raw_log_file_path, "%s/test_run_%s_raw.json", logs_dir, time);
    for (size_t i = 2; file_exists(raw_log_file_path); i++) {
        free(raw_log_file_path);
        asprintf(&raw_log_file_path, "%s/test_run_%s_raw(%zu).json", logs_dir,
                 time, i);
    }
    LOG("Raw log path: %s", raw_log_file_path);

    asprintf(&output_log_file_path, "%s/test_run_%s_output.log", logs_dir,
             time);
    for (size_t i = 0; file_exists(output_log_file_path); i++) {
        free(output_log_file_path);
        asprintf(&output_log_file_path, "%s/test_run_%s_output(%zu).log",
                 logs_dir, time, i);
    }
    LOG("Output log path: %s", output_log_file_path);

    output_log_file = fopen(output_log_file_path, "w");
    if (!output_log_file) {
        LOG("Unable to create output log file.");
        internal_logging_deinit();
        exit(EXIT_FAILURE);
    }
    LOG("Created output log file.");

    ltf_state_register_test_started_cb(state, ltf_log_test_started);
    ltf_state_register_test_finished_cb(state, ltf_log_test_finished);
    ltf_state_register_test_log_cb(state, ltf_log_test);
    ltf_state_register_test_teardown_started_cb(state,
                                                ltf_log_defer_queue_started);
    ltf_state_register_test_teardown_finished_cb(state,
                                                 ltf_log_defer_queue_finished);
    ltf_state_register_test_defer_failed_cb(state, ltf_log_defer_failed);
    ltf_state_register_test_run_finished_cb(state, ltf_log_test_run_finished);

    LOG("Successfully started LTF test logging.");
}

void ltf_log_free() {
    free(output_log_file_path);
    free(raw_log_file_path);
    free(logs_dir);
}
