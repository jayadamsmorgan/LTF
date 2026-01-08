#ifndef LTF_SECRETS_H
#define LTF_SECRETS_H

#include "util/da.h"

int ltf_secrets_parse_file(da_t *out);

void ltf_register_secrets(da_t *new);

da_t *ltf_get_secrets();

int ltf_parse_secrets();

void ltf_free_secrets();

#endif // LTF_SECRETS_H
