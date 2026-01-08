#ifndef LTF_VARS_H
#define LTF_VARS_H

#include "util/da.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *name;

    bool is_scalar; /* true if original value was a string */
    char *scalar;   /* set when is_scalar == true */

    da_t *values;    /* optional, duplicated strings */
    char *def_value; /* optional default string */

    char *final_value;
} ltf_var_entry_t;

void ltf_register_vars(da_t *vars);

int ltf_parse_vars();

da_t *ltf_get_vars();

void ltf_free_vars();

#endif // LTF_VARS_H
