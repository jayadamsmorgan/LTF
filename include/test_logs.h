#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "ltf_state.h"

char *ltf_log_get_logs_dir();

char *ltf_log_get_output_log_file_path();

char *ltf_log_get_raw_log_file_path();

void ltf_log_init(ltf_state_t *state);

void ltf_log_free();

#endif // TEST_LOGS_H
