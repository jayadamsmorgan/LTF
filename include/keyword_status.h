#ifndef KEYWORD_STATUS_H
#define KEYWORD_STATUS_H

#include "ltf_state.h"

#include "util/da.h"

typedef struct {

    da_t *children;

    char *name;
    char *started;
    char *finished; // Nullable

    char *file;
    int line;

} keyword_status_t;

void keyword_status_init(ltf_state_t *state);

#endif // KEYWORD_STATUS_H
