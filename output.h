#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdbool.h>
#include <stdio.h>

void output_init(const char *prog, bool quiet,
                 void (*pre_cmd)(void), FILE *file_ptr);
int output(const char *const fmt, ...);
int error(const char *const fmt, ...);

#endif
