#include "ltf_secrets.h"

#include "internal_logging.h"
#include "project_parser.h"

#include "util/da.h"
#include "util/files.h"
#include "util/kv.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_COLORED "\x1b[31mERROR:\x1b[0m "
#define WARNING_COLORED "\x1b[33mWARNING:\x1b[0m "

static char *rtrim(char *s) {
    if (!s)
        return s;
    size_t n = strlen(s);
    while (n && (unsigned char)s[n - 1] <= ' ')
        s[--n] = '\0';
    return s;
}
static char *ltrim(char *s) {
    if (!s)
        return s;
    size_t i = 0;
    while (s[i] && (unsigned char)s[i] <= ' ')
        i++;
    return s + i;
}

static char *trim(char *s) { return rtrim(ltrim(s)); }

static int sbuf_append(char **dst, size_t *len, size_t *cap, const char *src,
                       size_t add) {
    if (*len + add + 1 > *cap) {
        size_t ncap = (*cap ? *cap : 64);
        while (ncap < *len + add + 1)
            ncap *= 2;
        char *tmp = (char *)realloc(*dst, ncap);
        if (!tmp)
            return -1;
        *dst = tmp;
        *cap = ncap;
    }
    memcpy(*dst + *len, src, add);
    *len += add;
    (*dst)[*len] = '\0';
    return 0;
}

// Helper to finalize a multiline value into the list and reset state.
static int finish_multiline(da_t *out, char **ml_key, char **buf, size_t *blen,
                            size_t *bcap, int *in_multiline) {
    kv_pair_t pair = {
        .key = *ml_key ? *ml_key : strdup(""),
        .value = *buf ? *buf : strdup(""),
    };
    if (!pair.key || !pair.value) {
        free(pair.key);
        free(pair.value);
        return -1;
    }
    // hand off ownership to array element
    if (!da_append(out, &pair)) {
        free(pair.key);
        free(pair.value);
        return -2;
    }

    *ml_key = NULL;
    *buf = NULL;
    *blen = 0;
    *bcap = 0;
    *in_multiline = 0;
    return 0;
}

int ltf_secrets_parse_file(da_t *out) {

    project_parsed_t *proj = get_parsed_project();

    char *secret_vars_path = NULL;
    if (asprintf(&secret_vars_path, "%s/.secrets", proj->project_path) < 0) {
        fprintf(stderr, ERROR_COLORED "OOM building secrets path\n");
        return -100;
    }
    if (!file_exists(secret_vars_path)) {
        free(secret_vars_path);
        LOG("No secrets file found.");
        return 0;
    }

    FILE *f = fopen(secret_vars_path, "rb");
    if (!f)
        return -1;

    char *line = NULL;
    size_t n = 0;
    ssize_t m;

    int in_multiline = 0;
    char *ml_key = NULL;
    char *buf = NULL;
    size_t blen = 0, bcap = 0;

    while ((m = getline(&line, &n, f)) != -1) {
        // strip CR/LF
        while (m > 0 && (line[m - 1] == '\n' || line[m - 1] == '\r'))
            line[--m] = '\0';

        if (in_multiline) {
            const char *p = line;

            // If the closing """ starts this line (ignoring leading
            // whitespace), don't include this line content and strip exactly
            // ONE trailing '\n' that we previously appended after the last
            // content line.
            const char *q = p;
            while (*q && isspace((unsigned char)*q))
                q++;

            if (strncmp(q, "\"\"\"", 3) == 0) {
                if (blen > 0 && buf && buf[blen - 1] == '\n') {
                    buf[--blen] = '\0';
                }
                if (finish_multiline(out, &ml_key, &buf, &blen, &bcap,
                                     &in_multiline) != 0)
                    goto oom;
                continue; // ignore anything after closing delimiter line
            }

            // closing delimiter somewhere later on the line?
            const char *clos = strstr(p, "\"\"\"");
            if (clos) {
                // append content before closing
                if (clos > p) {
                    if (sbuf_append(&buf, &blen, &bcap, p,
                                    (size_t)(clos - p)) != 0)
                        goto oom;
                }
                if (finish_multiline(out, &ml_key, &buf, &blen, &bcap,
                                     &in_multiline) != 0)
                    goto oom;
                continue; // ignore trailing chars after closing """
            } else {
                if (*p && sbuf_append(&buf, &blen, &bcap, p, strlen(p)) != 0)
                    goto oom;
                if (sbuf_append(&buf, &blen, &bcap, "\n", 1) != 0)
                    goto oom;
                continue;
            }
        }

        // not in multiline: ignore blank
        char *s = trim(line);
        if (*s == '\0')
            continue;

        char *eq = strchr(s, '=');
        if (!eq)
            continue; // ignore malformed

        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);
        if (*key == '\0')
            continue;

        // triple quoted?
        if (strncmp(val, "\"\"\"", 3) == 0) {
            val += 3;
            char *clos = strstr(val, "\"\"\"");
            if (clos) {
                *clos = '\0';
                kv_pair_t pair = {
                    .key = strdup(key),
                    .value = strdup(val),
                };
                if (!pair.key || !pair.value) {
                    free(pair.key);
                    free(pair.value);
                    goto oom;
                }
                if (!da_append(out, &pair))
                    goto oom;
            } else {
                ml_key = strdup(key);
                if (!ml_key)
                    goto oom;
                buf = NULL;
                blen = bcap = 0;
                if (*val) {
                    if (sbuf_append(&buf, &blen, &bcap, val, strlen(val)) != 0)
                        goto oom;
                    if (sbuf_append(&buf, &blen, &bcap, "\n", 1) != 0)
                        goto oom;
                }
                in_multiline = 1;
            }
        } else {
            kv_pair_t pair = {
                .key = strdup(key),
                .value = strdup(val),
            };
            if (!pair.key || !pair.value) {
                free(pair.key);
                free(pair.value);
                goto oom;
            }
            if (!da_append(out, &pair))
                goto oom;
        }
    }

    if (in_multiline) {
        if (finish_multiline(out, &ml_key, &buf, &blen, &bcap, &in_multiline) !=
            0)
            goto oom;
    }

    fclose(f);
    free(line);
    free(secret_vars_path);
    return 0;

oom:
    fclose(f);
    free(line);
    free(ml_key);
    free(buf);
    free(secret_vars_path);
    return -3;
}

static da_t *secrets = NULL;

void ltf_register_secrets(da_t *new) {
    if (!new)
        return;

    if (!secrets)
        secrets = da_init(1, sizeof(kv_pair_t));

    size_t size = da_size(new);
    for (size_t i = 0; i < size; ++i) {
        char **name = da_get(new, i);
        kv_pair_t p = {.key = *name, .value = NULL};
        da_append(secrets, &p);
    }
    da_free(new);
}

da_t *ltf_get_secrets() {
    //
    return secrets;
}

static void free_any_secrets(da_t *secrets) {
    if (!secrets)
        return;

    size_t secrets_count = da_size(secrets);
    for (size_t i = 0; i < secrets_count; ++i) {
        kv_pair_t *secret = da_get(secrets, i);
        free(secret->value);
        free(secret->key);
    }
    da_free(secrets);
}

int ltf_parse_secrets() {
    da_t *specified = da_init(1, sizeof(kv_pair_t));

    int res = ltf_secrets_parse_file(specified);
    if (res)
        return res;

    size_t registered_size = da_size(secrets);
    size_t specified_size = da_size(specified);

    // Check for duplicates in registered:
    da_t *found_idxs = da_init(1, sizeof(size_t));
    for (size_t i = 0; i < registered_size; ++i) {
        for (size_t j = 0; j < registered_size; ++j) {
            if (i == j) {
                continue;
            }
            bool found = false;
            for (size_t k = 0; k < da_size(found_idxs); ++k) {
                size_t *idx = da_get(found_idxs, k);
                if (*idx == i || *idx == j) {
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }

            kv_pair_t *l = da_get(secrets, i);
            kv_pair_t *r = da_get(secrets, j);

            if (strcmp(l->key, r->key) == 0) {
                da_append(found_idxs, &j);
                fprintf(stderr,
                        ERROR_COLORED
                        "Secret '%s' was registered more than once.\n",
                        l->key);
                res = -1;
            }
        }
    }
    da_free(found_idxs);

    // Check for duplicates in specified:
    found_idxs = da_init(1, sizeof(size_t));
    for (size_t i = 0; i < specified_size; ++i) {
        for (size_t j = 0; j < specified_size; ++j) {
            if (i == j) {
                continue;
            }
            bool found = false;
            for (size_t k = 0; k < da_size(found_idxs); ++k) {
                size_t *idx = da_get(found_idxs, k);
                if (*idx == i || *idx == j) {
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }
            kv_pair_t *l = da_get(specified, i);
            kv_pair_t *r = da_get(specified, j);
            if (strcmp(l->key, r->key) == 0) {
                da_append(found_idxs, &j);
                fprintf(stderr,
                        ERROR_COLORED
                        "Value for secret '%s' was specified more than once.\n",
                        l->key);
                res = -1;
            }
        }
    }
    da_free(found_idxs);

    for (size_t i = 0; i < registered_size; ++i) {
        kv_pair_t *reg = da_get(secrets, i);
        bool found = false;
        for (size_t j = 0; j < specified_size; ++j) {
            kv_pair_t *spec = da_get(specified, j);
            if (strcmp(reg->key, spec->key) == 0) {
                // found
                reg->value = strdup(spec->value);
                found = true;
                break;
            }
        }
        if (found)
            continue;
        fprintf(stderr,
                ERROR_COLORED
                "Value for secret '%s' is not provided. Please add it "
                "in '.secrets' file.\n",
                reg->key);
        res = -1;
    }

    for (size_t i = 0; i < specified_size; ++i) {
        kv_pair_t *spec = da_get(specified, i);
        bool found = false;
        for (size_t j = 0; j < registered_size; ++j) {
            kv_pair_t *reg = da_get(secrets, j);
            if (strcmp(spec->key, reg->key) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        fprintf(stderr, WARNING_COLORED "Secret '%s' was not registered.\n",
                spec->key);
    }

    free_any_secrets(specified);

    return res;
}

void ltf_free_secrets() {
    //
    free_any_secrets(secrets);
}
