#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include "ltf_log_level.h"
#include "ltf_test_scenarios.h"

#include "util/da.h"

#include <stdbool.h>

typedef enum {
    CMD_INIT,
    CMD_TEST,
    CMD_EVAL,
    CMD_LOGS_INFO,
    CMD_HELP,
    CMD_TARGET_ADD,
    CMD_TARGET_REMOVE,
    CMD_UNKNOWN,
} cmd_category;

typedef struct {
    char *project_name;
    bool multitarget;

    bool internal_logging;
} cmd_init_options;

typedef struct {
    // TODO
    bool something;

    bool interactive;
} cmd_config_options;

typedef struct {
    da_t *tags;

    da_t *vars;

    bool no_logs;
    ltf_log_level log_level;

    bool skip_hooks;

    char *target;

    bool internal_logging;

    char *custom_ltf_lib_path;

    bool headless;

    ltf_test_scenario_parsed_t scenario;
    bool scenario_parsed;
} cmd_test_options;

typedef struct {
    char *arg;

    bool include_outputs;
    bool keyword_tree;

    bool internal_logging;
} cmd_logs_info_options;

typedef struct {
    char *target;
    bool internal_logging;
} cmd_target_add_options;

typedef struct {
    char *target;
    bool internal_logging;
} cmd_target_remove_options;

typedef struct {
    char *name;
    da_t *args;
    bool internal_logging;
} cmd_eval_options;

cmd_category cmd_parser_parse(int argc, char **argv);

cmd_init_options *cmd_parser_get_init_options();
cmd_config_options *cmd_parser_get_config_options();
cmd_test_options *cmd_parser_get_test_options();
cmd_logs_info_options *cmd_parser_get_logs_info_options();
cmd_target_add_options *cmd_parser_get_target_add_options();
cmd_target_remove_options *cmd_parser_get_target_remove_options();
cmd_eval_options *cmd_parser_get_eval_options();

void cmd_parser_free_init_options();
void cmd_parser_free_test_options();
void cmd_parser_free_eval_options();

#endif // CMD_PARSER_H
