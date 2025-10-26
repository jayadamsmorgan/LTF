#ifndef LTF_TEST_SCENARIOS_H
#define LTF_TEST_SCENARIOS_H

#include "ltf_log_level.h"

#include <util/da.h>

typedef struct {
    char *target;
    da_t *tags; // [char *]
    da_t *vars; // [kv_pair_t]
    ltf_log_level log_level;
    bool no_logs;
    bool headless;
    char *ltf_lib_path;
    da_t *order; // [char *]
} ltf_test_scenario_parsed_t;

int ltf_test_scenario_parse(const char *file_path,
                            ltf_test_scenario_parsed_t *out);

#endif // LTF_TEST_SCENARIOS_H
