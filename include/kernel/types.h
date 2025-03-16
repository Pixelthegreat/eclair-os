#ifndef ECLAIR_TYPES_H
#define ECLAIR_TYPES_H

/* include basic types */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef long kssize_t;

/* misc */
#define ALIGN(x, sz) (((x) + ((sz)-1)) & ~(sz-1))

#define MIN(x, y) ((x) < (y)? (x): (y))
#define MAX(x, y) ((x) > (y)? (x): (y))

#endif /* ECLAIR_TYPES_H */
