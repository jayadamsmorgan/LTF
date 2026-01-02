#include "ltf_vars.h"

#include "cmd_parser.h"

#include "util/kv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_COLORED "\x1b[31mERROR:\x1b[0m "
#define WARNING_COLORED "\x1b[33mWARNING:\x1b[0m "

static da_t *vars = NULL;

void ltf_register_vars(da_t *new) {
    if (!new)
        return;

    if (!vars)
        vars = da_init(1, sizeof(ltf_var_entry_t));

    size_t new_size = da_size(new);
    for (size_t i = 0; i < new_size; ++i) {
        ltf_var_entry_t *e = da_get(new, i);
        da_append(vars, e);
    }

    da_free(new);
}

da_t *ltf_get_vars() { return vars; }

int ltf_parse_vars() {

    int res = 0;

    cmd_test_options *opts = cmd_parser_get_test_options();
    size_t opts_vars_count = da_size(opts->vars);

    size_t registered_count = da_size(vars);

    // Search for duplicates in registered variables:
    da_t *found_idxs = da_init(1, sizeof(size_t));
    for (size_t i = 0; i < registered_count; ++i) {
        for (size_t j = 0; j < registered_count; ++j) {
            if (i == j) {
                continue;
            }
            bool already_found = false;
            for (size_t k = 0; k < da_size(found_idxs); ++k) {
                size_t *idx = da_get(found_idxs, k);
                if (*idx == j || *idx == i) {
                    already_found = true;
                    break;
                }
            }
            if (already_found) {
                continue;
            }
            ltf_var_entry_t *l = da_get(vars, i);
            ltf_var_entry_t *r = da_get(vars, j);
            if (strcmp(l->name, r->name) == 0) {
                da_append(found_idxs, &j);
                fprintf(stderr,
                        ERROR_COLORED
                        "Variable '%s' was registered more than once.\n",
                        l->name);
                res = -1;
            }
        }
    }
    da_free(found_idxs);

    // Search for duplicates in specified variables (CLI, scenarios, etc.)
    found_idxs = da_init(1, sizeof(size_t));
    for (size_t i = 0; i < opts_vars_count; ++i) {
        for (size_t j = 0; j < opts_vars_count; ++j) {
            if (i == j) {
                continue;
            }
            bool already_found = false;
            for (size_t k = 0; k < da_size(found_idxs); ++k) {
                size_t *idx = da_get(found_idxs, k);
                if (*idx == j || *idx == i) {
                    already_found = true;
                    break;
                }
            }
            if (already_found) {
                continue;
            }
            kv_pair_t *l = da_get(opts->vars, i);
            kv_pair_t *r = da_get(opts->vars, j);
            if (strcmp(l->key, r->key) == 0) {
                da_append(found_idxs, &j);
                fprintf(stderr,
                        ERROR_COLORED
                        "Value for variable '%s' was specified more "
                        "than once.\n",
                        l->key);
                res = -1;
            }
        }
    }
    da_free(found_idxs);

    for (size_t i = 0; i < registered_count; ++i) {
        ltf_var_entry_t *e = da_get(vars, i);
        kv_pair_t *found = NULL;
        for (size_t j = 0; j < opts_vars_count; ++j) {
            kv_pair_t *p = da_get(opts->vars, j);
            if (strcmp(e->name, p->key) == 0) {
                found = p;
                break;
            }
        }
        if (e->is_scalar) {
            if (found) {
                fprintf(stderr,
                        ERROR_COLORED "Constant variable '%s' redefined.\n",
                        e->name);
                res = -1;
                continue;
            }
            e->final_value = strdup(e->scalar);
            continue;
        }

        if (!found && e->def_value) {
            e->final_value = strdup(e->def_value);
            continue;
        }

        size_t values_count = da_size(e->values);

        if (!found) {
            if (values_count != 0) {
                fprintf(stderr,
                        ERROR_COLORED "Value for variable '%s' is not "
                                      "specified. Allowed values: ",
                        e->name);
                for (size_t j = 0; j < values_count; ++j) {
                    char **value = da_get(e->values, j);
                    fprintf(stderr, "'%s'", *value);
                    if (j < values_count - 1) {
                        fprintf(stderr, ", ");
                    }
                }
                fprintf(stderr,
                        ". Please specify it with `--vars %s=value` or add "
                        "default value in declaration.\n",
                        e->name);
            } else {
                fprintf(
                    stderr,
                    ERROR_COLORED
                    "Value for variable '%s' is not specified. "
                    "Please specify it with `--vars %s=value` or add default "
                    "value in declaration.\n",
                    e->name, e->name);
            }
            res = -1;
            continue;
        }

        if (values_count == 0) {
            e->final_value = strdup(found->value);
            continue;
        }

        for (size_t j = 0; j < values_count; ++j) {
            char **value = da_get(e->values, j);
            if (strcmp(*value, found->value) == 0) {
                // found
                e->final_value = strdup(found->value);
                break;
            }
        }
        if (e->final_value) {
            continue;
        }

        fprintf(stderr,
                ERROR_COLORED
                "Value for variable '%s' is not an allowed value. "
                "Allowed values: ",
                e->name);
        for (size_t j = 0; j < values_count; ++j) {
            char **value = da_get(e->values, j);
            fprintf(stderr, "'%s'", *value);
            if (j < values_count - 1) {
                fprintf(stderr, ", ");
            }
        }
        fprintf(stderr, "\n");
        res = -1;
    }

    for (size_t j = 0; j < opts_vars_count; ++j) {
        kv_pair_t *opt = da_get(opts->vars, j);
        bool found = false;
        for (size_t i = 0; i < registered_count; ++i) {
            ltf_var_entry_t *reg = da_get(vars, i);
            if (strcmp(reg->name, opt->key) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        printf(WARNING_COLORED "Variable '%s' was not registered.\n", opt->key);
    }

    return res;
}

void ltf_free_vars() {
    if (!vars)
        return;
    size_t vars_count = da_size(vars);
    for (size_t i = 0; i < vars_count; ++i) {
        ltf_var_entry_t *e = da_get(vars, i);
        free(e->scalar);
        free(e->def_value);
        free(e->name);
        free(e->final_value);
        size_t values_count = da_size(e->values);
        for (size_t j = 0; j < values_count; ++j) {
            char **value = da_get(e->values, j);
            free(*value);
        }
        da_free(e->values);
    }
    da_free(vars);
    vars = NULL;
}
