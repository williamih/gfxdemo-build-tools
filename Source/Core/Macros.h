#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define FATAL(format, ...) \
    do { fprintf(stderr, "Fatal error!\n"); \
         fprintf(stderr, format, ##__VA_ARGS__); \
         abort(); \
    } while (0)

#define ASSERT assert

#endif // CORE_MACROS_H
