
// Adaptation of RXI's vec.h, licensed under MIT.

#ifndef POSIX_VEC_H
#define POSIX_VEC_H
#include <ichor/alloc.h>
#include <stdc-shim/string.h>

static inline void vec_expand(void **data, size_t *length, size_t *capacity, int memsz)
{
    if (*length == *capacity)
    {
        *capacity = (*capacity == 0) ? 1 : (*capacity * 2);

        *data = ichor_realloc(*data, *capacity * memsz);
    }
}

#define Vec(T)           \
    struct               \
    {                    \
        T *data;         \
        size_t length;   \
        size_t capacity; \
    }

#define vec_init(v) memset((v), 0, sizeof(*(v)))

#define vec_push(v, val)                                          \
    vec_expand((void **)&(v)->data, &(v)->length, &(v)->capacity, \
               sizeof(*(v)->data));                               \
    (v)->data[(v)->length++] = (val)

#define vec_deinit(v) free((v)->data)

#define vec_pop(v) \
    (v)->data[--((v)->length)]

#endif