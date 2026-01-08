#ifndef LTF_TUI_H
#define LTF_TUI_H

#include "ltf_state.h"

// UI initialization
int ltf_tui_init(ltf_state_t *state);

// UI deinitialization
void ltf_tui_deinit();

// UI update function; should be called every time pannel should be updated
void ltf_tui_update();

void tui_render_result(void *ud);

void ltf_tui_set_test_progress(double progress);

void ltf_tui_set_current_line(const char *file, int line, const char *line_str);

#endif // LTF_TUI_H
