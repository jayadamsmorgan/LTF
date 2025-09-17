#include "taf_tui.h"

#include "cmd_parser.h"
#include "taf_hooks.h"
#include "taf_vars.h"
#include "version.h"

#include "internal_logging.h"
#include "util/da.h"
#include "util/string.h"
#include "util/time.h"
#include <picotui.h>

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Possible test result
typedef enum {
    PASSED = 0U,
    FAILED = 1U,
    RUNNING = 2U,
} ui_test_progress_state;

//  Helping struct for test representstion in UI
typedef struct {

    taf_log_level log_level;

    bool is_teardown;
    bool is_hooking;

    double current_test_progress;
    char *current_file;
    int current_line;
    char *current_line_str;

} ui_state_t;

static ui_state_t ui_state = {0};

static taf_state_t *taf_state = NULL;

static pico_t *ui = NULL;

static int log_level_to_palindex_map[] = {
    9, 1, 3, 4, 2, 6,
};

void taf_tui_set_test_progress(double progress) {
    ui_state.current_test_progress = progress;
}

static size_t sanitize_inplace(char *buf, size_t len) {
    size_t r = 0; /* read cursor */
    size_t w = 0; /* write cursor */

    while (r < len) {
        char c = buf[r];

        /* collapse CRLF → LF */
        if (c == '\r' && r + 1 < len && buf[r + 1] == '\n') {
            buf[w++] = '\n';
            r += 2;
            continue;
        }

        /* turn NUL into space (0x20) */
        if (c == '\0')
            c = ' ';

        buf[w++] = c;
        r++;
    }

    if (w < len) /* keep C-string semantics if we can */
        buf[w] = '\0';

    return w; /* new logical length */
}

char *ll_to_str[] = {
    "CRT", "ERR", "WRN", "INF", "DBG", "TRC",
};

void taf_tui_log(taf_state_test_t *test, taf_state_test_output_t *output) {

    // Filter for logs with lower log level
    if (output->level > ui_state.log_level) {
        return;
    }

    // Allocate space for logs  and remove inapropriate synbols from it
    char *tmp = malloc(output->msg_len + 1);
    memcpy(tmp, output->msg, output->msg_len);
    tmp[output->msg_len] = '\0';
    sanitize_inplace(tmp, output->msg_len);

    // Wrie Log Level information for current run
    pico_set_colors(ui, log_level_to_palindex_map[output->level], -1);
    pico_printf(ui, "[%s]", ll_to_str[output->level]);
    pico_print(ui, "");

    // Write time form test start (time in format hh:mm:ss)
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_printf(ui, "(%s)", output->date_time + 9);
    pico_print(ui, " ");

    // Write logs body
    pico_reset_colors(ui);
    pico_print_block(ui, tmp);
    free(tmp);
}

/*------------------- TAF UI functions -------------------*/
static void taf_tui_project_header_render(pico_t *ui) {

    pico_reset_colors(ui); // better to do it

    /* Line 0: Main title */
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_clear_line(ui, 0);
    pico_ui_puts_yx(ui, 0, 0, "LTF ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
    pico_ui_puts_yx(ui, 0, 4, "v" TAF_VERSION);

    /* Line 1: | */
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_clear_line(ui, 1);
    pico_ui_puts_yx(ui, 1, 0, "│");

    /* Line 2: Project Information */
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_clear_line(ui, 2);
    pico_ui_puts_yx(ui, 2, 0, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 2, 3, "Project Information:");

    /* Line 3: Project name */
    pico_ui_clear_line(ui, 3);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 3, 0, "│  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 3, 3, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 3, 6, "Project: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, 3, 17, "%s", taf_state->project_name);

    /* Line 4: Project name */
    pico_ui_clear_line(ui, 4);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 4, 0, "│  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 4, 3, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 4, 6, "Target: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, 4, 17, "%s",
                      taf_state->target ? taf_state->target : "none");

    /* Line 5: Project tags */
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_clear_line(ui, 5);
    pico_ui_puts_yx(ui, 5, 0, "│  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 5, 3, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 5, 6, "Tags: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    size_t tags_count = da_size(taf_state->tags);
    size_t offset = 17;
    for (size_t i = 0; i < tags_count; ++i) {
        char **tag = da_get(taf_state->tags, i);
        pico_ui_printf_yx(ui, 5, offset, "%s ", *tag);
        offset += strlen(*tag) + 1;
    }

    /* Line 6: Project vars */
    pico_ui_clear_line(ui, 6);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 6, 0, "│  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 6, 3, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 6, 6, "Vars: ");
    size_t vars_count = da_size(taf_state->vars);
    offset = 17;
    for (size_t i = 0; i < vars_count; ++i) {
        taf_var_entry_t *e = da_get(taf_state->vars, i);
        pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
        pico_ui_printf_yx(ui, 6, offset, "%s:", e->name);
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_ui_printf_yx(ui, 6, offset + 1 + strlen(e->name), "%s ",
                          e->final_value);
        offset += strlen(e->name) + strlen(e->final_value) + 2;
    }

    /* Line 7: Project Log Level */
    pico_ui_clear_line(ui, 7);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 7, 0, "│  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 7, 3, "└─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 7, 6, "Log Level: ");
    pico_set_colors(ui, log_level_to_palindex_map[ui_state.log_level], -1);
    pico_ui_printf_yx(ui, 7, 17, "%s",
                      taf_log_level_to_str(ui_state.log_level));
    pico_reset_colors(ui);
}

static void taf_tui_test_progress_render(pico_t *ui) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);

    /* Line 8: Test Progress */
    pico_ui_clear_line(ui, 8);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 8, 0, "├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 8, 3, "Test Progress:");

    size_t tests_count = da_size(taf_state->tests);
    if (tests_count == 0)
        return;

    taf_state_test_t *test = da_get(taf_state->tests, tests_count - 1);

    /* Line 9: Test Name and millis from the start*/
    pico_ui_clear_line(ui, 9);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 9, 0, "│  ├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 9, 6, "Name: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, 9, 12, "%s", test->name);

    /* Line 10: Test Progress */
    pico_ui_clear_line(ui, 10);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 10, 0, "│  ├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 10, 6, "Progress: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
    pico_ui_printf_yx(ui, 10, 16, "%d%%",
                      (unsigned int)(ui_state.current_test_progress * 100));

    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, 10, strlen(test->name) + 13, "[ %lums ]",
                      millis_since_start());

    /* Line 11: Current Line in Test */
    if (test->status == TEST_STATUS_RUNNING) {
        char *file_str = ui_state.current_file;
        if (file_str) {
            size_t len = strlen(ui_state.current_file);
            if (len > 40) {
                file_str += len - 40;
            }
            pico_ui_clear_line(ui, 11);
            pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
            pico_ui_puts_yx(ui, 11, 0, "│  └─ ");
            pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
            pico_ui_puts_yx(ui, 11, 6, "Current Line: ");
            pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
            pico_ui_printf_yx(ui, 11, 20, "[%s%s:%d]", len > 40 ? "..." : "",
                              file_str, ui_state.current_line);
            pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
            pico_ui_printf_yx(
                ui, 11,
                20 + 4 + (len > 40 ? 3 : 0) + strlen(file_str) +
                    ((ui_state.current_line == 0)
                         ? 1
                         : (int)log10(fabs(ui_state.current_line)) + 1),
                "%s", ui_state.current_line_str);
        }
    }
    pico_reset_colors(ui);
}
static void taf_tui_summary_render(pico_t *ui, size_t size) {

    /* Line 12: Test Case Summary */
    pico_ui_clear_line(ui, size);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, size, 0, "└─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, size, 3, "Summary:");

    /* Line 13: Test Case Status */
    pico_ui_clear_line(ui, size + 1);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, size + 1, 0, "   ├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, size + 1, 6, "Test Case Status: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, size + 1, 24, "          |");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_puts_yx(ui, size + 1, 24, "Total:    ");
    pico_ui_printf_yx(ui, size + 1, 31, "%zu", taf_state->total_amount);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, size + 1, 36, "           |");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_ui_puts_yx(ui, size + 1, 36, "Passed:    ");
    pico_ui_printf_yx(ui, size + 1, 44, "%zu", taf_state->passed_amount);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
    pico_ui_puts_yx(ui, size + 1, 49, "Failed:     ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
    pico_ui_printf_yx(ui, size + 1, 57, "%zu", taf_state->failed_amount);
    pico_reset_colors(ui);
    /* Line 14: Test Elapsed Time */
    uint64_t ms;
    ms = millis_since_taf_start();
    const unsigned long minutes = ms / 60000;
    const unsigned long seconds = (ms / 1000) % 60;
    const unsigned long millis = ms % 1000;

    pico_ui_clear_line(ui, size + 2);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, size + 2, 0, "   └─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, size + 2, 6, "Elapsed Time: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, size + 2, 20, "%lum %lu.%03lus ", minutes, seconds,
                      millis);
    pico_reset_colors(ui);
}

static int test_result_to_color(taf_state_test_t *test) {
    if (!strcmp(test->status_str, "PASSED")) {
        return PICO_COLOR_BRIGHT_GREEN;
    } else if (!strcmp(test->status_str, "FAILED")) {
        return PICO_COLOR_BRIGHT_RED;
    } else {
        return PICO_COLOR_BRIGHT_WHITE;
    }
}

static void taf_tui_test_run_result(pico_t *ui, size_t tests_count) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);

    /* Line 8: Test Progress */
    pico_ui_clear_line(ui, 8);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 8, 0, "├─ ");
    pico_ui_puts_yx(ui, 8, 3, "Test Results:");

    /* Line 9: Test Name and millis from the start*/
    for (size_t i = 0; i < tests_count - 1; ++i) {
        taf_state_test_t *test = da_get(taf_state->tests, i);
        pico_ui_clear_line(ui, 9 + i * 4);
        pico_ui_clear_line(ui, 10 + i * 4);
        pico_ui_clear_line(ui, 11 + i * 4);
        pico_ui_clear_line(ui, 12 + i * 4);

        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_ui_puts_yx(ui, 9 + i * 4, 0, "│  ├─ ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_ui_puts_yx(ui, 9 + i * 4, 6, "Name: ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
        pico_ui_printf_yx(ui, 9 + i * 4, 12, "%s ", test->name);

        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 1, 0, "│  │  ├─ ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 1, 9, "Result:    ");
        pico_set_colors(ui, test_result_to_color(test), -1);
        pico_ui_printf_yx(ui, 9 + i * 4 + 1, 19, "%s", test->status_str);
        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 2, 0, "│  │  ├─ ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 2, 9, "Started:  ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
        pico_ui_printf_yx(ui, 9 + i * 4 + 2, 19, "%s", test->started + 9);

        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 3, 0, "│  │  └─ ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_ui_puts_yx(ui, 9 + i * 4 + 3, 9, "Finished: ");
        pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
        pico_ui_printf_yx(ui, 9 + i * 4 + 3, 19, "%s", test->finished + 9);
    }

    size_t offset = tests_count - 1;
    taf_state_test_t *test = da_get(taf_state->tests, offset);
    pico_ui_clear_line(ui, 9 + offset * 4);
    pico_ui_clear_line(ui, 10 + offset * 4);
    pico_ui_clear_line(ui, 11 + offset * 4);
    pico_ui_clear_line(ui, 12 + offset * 4);

    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4, 0, "│  └─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4, 6, "Name: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_YELLOW, -1);
    pico_ui_printf_yx(ui, 9 + offset * 4, 12, "%s", test->name);

    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 1, 0, "│     ├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 1, 9, "Result:   ");
    pico_set_colors(ui, test_result_to_color(test), -1);
    pico_ui_printf_yx(ui, 9 + offset * 4 + 1, 19, "%s", test->status_str);

    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 2, 0, "│     ├─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 2, 9, "Started:  ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
    pico_ui_printf_yx(ui, 9 + offset * 4 + 2, 19, "%s", test->started + 9);

    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 3, 0, "│     └─ ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_ui_puts_yx(ui, 9 + offset * 4 + 3, 9, "Finished: ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
    pico_ui_printf_yx(ui, 9 + offset * 4 + 3, 19, "%s", test->finished + 9);
    pico_reset_colors(ui);
}

static void render_ui(pico_t *ui, void *ud) {
    (void)ud;

    pico_remove_cursor();

    taf_tui_project_header_render(ui);

    taf_tui_test_progress_render(ui);

    taf_tui_summary_render(ui, 12);
}

static void render_progress(pico_t *ui, void *ud) {
    (void)ud;

    taf_tui_test_progress_render(ui);

    taf_tui_summary_render(ui, 12);
}

static void render_result(pico_t *ui, void *ud) {
    (void)ud;

    // Remove strings before resize
    size_t tests_count = da_size(taf_state->tests);
    for (size_t i = 0; i < (tests_count * 4) + 12; ++i) {
        pico_ui_clear_line(ui, i);
    }
    // Resize strings
    pico_set_ui_rows(ui, 12 + tests_count * 4);

    taf_tui_project_header_render(ui);

    taf_tui_test_run_result(ui, (tests_count));

    taf_tui_summary_render(ui, tests_count * 4 + 9);
}

void taf_tui_test_started(taf_state_test_t *test) {

    render_ui(ui, NULL);

    //  "-" Gap  between logs
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    term_size_t terminal_size = get_term_size();
    for (int i = 0; i < terminal_size.cols - 1; ++i) {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_print(ui, "_");
    }
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_println(ui, "_");

    // Test Finished message
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " %s Started\n", test->name);
}

void taf_tui_defer_queue_started(taf_state_test_t *test) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " Defers Started For %s \n", test->name);
}

void taf_tui_defer_queue_finished(taf_state_test_t *test) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " Defers Finished For %s \n", test->name);
}

void taf_tui_defer_failed(taf_state_test_t *test,
                          taf_state_test_output_t *output) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_print(ui, " Defer failed with message: \n");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
    pico_print_block(ui, output->msg);
}

void taf_tui_test_finished(taf_state_test_t *test) {

    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");

    size_t errors_count = da_size(test->failure_reasons);

    // Check if there any failed reasons
    if (errors_count > 1) {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_print(ui, " Failure Reasons: \n");
        for (size_t i = 0; i < errors_count; ++i) {
            taf_state_test_output_t *error = da_get(test->failure_reasons, i);
            pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
            pico_printf(ui, " [%d] ", i + 1);
            pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
            pico_print_block(ui, error->msg);
        }
    } else if (errors_count == 1) {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_print(ui, " Failure Reason: \n");
        taf_state_test_output_t *error = da_get(test->failure_reasons, 0);
        pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
        pico_print_block(ui, error->msg);
    } else {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
        pico_printf(ui, " %s Finished\n", test->name);
    }
}

void taf_tui_tests_set_finished() { render_result(ui, NULL); }

void taf_tui_hook_started(taf_hook_fn fn) {

    render_ui(ui, NULL);

    char *str = "";
    switch (fn) {
    case 0:
        str = "Test Run Started";
        break;
    case 1:
        str = "Test Started";
        break;
    case 2:
        str = "Test Finished";
        break;
    case 3:
        str = "Test Run Finished";
        break;
    }
    // Hook Started message
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " Hooks Started: %s\n", str);
}

void taf_tui_hook_finished(taf_hook_fn fn) {
    // Update UI
    render_ui(ui, NULL);

    // Update log
    char *str = "";
    switch (fn) {
    case 0:
        str = "Test Run Started";
        break;
    case 1:
        str = "Test Started";
        break;
    case 2:
        str = "Test Finished";
        break;
    case 3:
        str = "Test Run Finished";
        break;
    }
    // Test Finished message
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " Hooks Finished: %s\n", str);

    //  "-" Gap  between logs
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    term_size_t terminal_size = get_term_size();
    for (int i = 0; i < terminal_size.cols - 1; ++i) {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_print(ui, "_");
    }
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_println(ui, "_");
}

void taf_tui_hook_failed(taf_hook_fn fn, const char *msg) {

    // Update UI
    render_ui(ui, NULL);

    // Update log
    char *str = "";
    switch (fn) {
    case 0:
        str = "Test Run Started";
        break;
    case 1:
        str = "Test Started";
        break;
    case 2:
        str = "Test Finished";
        break;
    case 3:
        str = "Test Run Finished";
        break;
    }

    // Test Finished message
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF]");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_printf(ui, " Hooks Failed: %s\n", str);
    pico_set_colors(ui, PICO_COLOR_BRIGHT_GREEN, -1);
    pico_print(ui, "[TAF] ");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_WHITE, -1);
    pico_print(ui, "Failure Reason:\n");
    pico_set_colors(ui, PICO_COLOR_BRIGHT_RED, -1);
    pico_print_block(ui, msg);

    //  "-" Gap  between logs
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    term_size_t terminal_size = get_term_size();
    for (int i = 0; i < terminal_size.cols - 1; ++i) {
        pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
        pico_print(ui, "_");
    }
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_println(ui, "_");
}

int taf_tui_init(taf_state_t *state) {

    LOG("Start TUI init");

    taf_state = state;

    // taf_state_register_test_run_started_cb();
    taf_state_register_test_started_cb(state, taf_tui_test_started);
    taf_state_register_test_finished_cb(state, taf_tui_test_finished);
    taf_state_register_test_log_cb(state, taf_tui_log);
    taf_state_register_test_teardown_started_cb(state,
                                                taf_tui_defer_queue_started);
    taf_state_register_test_teardown_finished_cb(state,
                                                 taf_tui_defer_queue_finished);
    taf_state_register_test_defer_failed_cb(state, taf_tui_defer_failed);
    taf_state_register_test_run_finished_cb(state, taf_tui_tests_set_finished);

    taf_hooks_register_hook_started_cb(taf_tui_hook_started);
    taf_hooks_register_hook_finished_cb(taf_tui_hook_finished);
    taf_hooks_register_hook_failed_cb(taf_tui_hook_failed);

    // Get project information
    cmd_test_options *opts = cmd_parser_get_test_options();
    ui_state.log_level = opts->log_level;

    // To  write Unicode
    setlocale(LC_ALL, "");

    // UI inintialization
    ui = pico_init(15, render_ui, NULL);
    if (!ui)
        return 1;
    pico_attach(ui);
    pico_install_sigint_handler(ui);
    return 0;
}

void taf_tui_deinit() {

    LOG("Start TUI deinit");

    // Turn off UI and free resources
    pico_shutdown(ui);

    // Restore cursor
    pico_restore_cursor();

    // Shift to new string
    puts("\n");

    // If tests were launched without log:
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (!opts->no_logs) {
        puts("For more info: 'taf logs info latest'");
    }

    // Freing resources
    pico_free(ui);
}

void taf_tui_update() {

    // UI panel update
    pico_redraw_ui(ui);
}
void taf_tui_set_current_line(const char *file, int line,
                              const char *line_str) {
    free(ui_state.current_file);
    free(ui_state.current_line_str);
    ui_state.current_file = strdup(file);
    ui_state.current_line = line;
    ui_state.current_line_str = string_strip(line_str);
    render_progress(ui, NULL);
}
