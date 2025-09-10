#ifndef UTIL_DA_H
#define UTIL_DA_H

#include <stdbool.h>
#include <stddef.h>

#define da_foreach(da, type, var)                                              \
    for (size_t var##_i = 0, var##s_count = da_size(da);                       \
         (da) && var##_i < var##s_count; ++var##_i)                            \
        for (type *var = (type *)da_get((da), var##_i); var; var = NULL)

typedef struct da_t da_t;

da_t *da_init(size_t init_capacity, size_t elem_size); // elem_size > 0
void da_free(da_t *da);

bool da_append(da_t *da, const void *elem);        // returns false on OOM
void *da_get(da_t *da, size_t index);              // NULL if OOB
const void *da_cget(const da_t *da, size_t index); // const view, NULL if OOB
bool da_set(da_t *da, size_t index, const void *elem);

size_t da_size(const da_t *da);
size_t da_capacity(const da_t *da);

bool da_reserve(da_t *da,
                size_t min_capacity); // grow to at least min_capacity
bool da_resize(
    da_t *da,
    size_t new_size);    // da_reserve to at least new_size + set new size
void da_clear(da_t *da); // keep capacity, reset size to 0

#endif /* UTIL_DA_H */
