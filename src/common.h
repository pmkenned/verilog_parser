#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NELEMS(X) sizeof(X)/sizeof(X[0])

typedef struct {
    char * p;
    size_t len;
    size_t cap;
} Buffer;

void die(const char * fmt, ...);
Buffer read_file(const char * filename);
//const char * parse_int_strerror(int errnum);
//int parse_int(const char * s, int * x);

#endif /* COMMON_H */
