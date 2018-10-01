#include <stdio.h>
#include <stdarg.h>
#include "output.h"
#include "main.h"
#include "progress.h"

int output(const char *const fmt, ...) {
    clear_progress_line();
    va_list ap;
    va_start(ap, fmt);
    int size = vprintf(fmt, ap);
    va_end(ap);
    return size;
}

int error(const char *const fmt, ...) {
    clear_progress_line();
    fprintf(stderr, "%s: ", prog);
    va_list ap;
    va_start(ap, fmt);
    int size = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return size;
}
