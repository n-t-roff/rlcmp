#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdbool.h>

void output_init(const char *const prog, const bool quiet,
                 void (*pre_cmd)(void));
int output(const char *const fmt, ...);
int error(const char *const fmt, ...);

#endif
