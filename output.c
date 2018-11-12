#include <stdarg.h>
#include "output.h"

static void (*pre_cmd_)(void);
static const char *prog_name_;
static FILE *file_ptr_;
static bool quiet_;

void output_init(const char *const prog, const bool quiet,
                 void (*const pre_cmd)(void), FILE *const file_ptr)
{
    prog_name_ = prog;
    quiet_ = quiet;
    pre_cmd_ = pre_cmd;
    file_ptr_ = file_ptr ? file_ptr : stdout;
}

int output(const char *const fmt, ...) {
    if (quiet_)
        return 0;
    pre_cmd_();
    va_list ap;
    va_start(ap, fmt);
    int size = vfprintf(file_ptr_, fmt, ap);
    va_end(ap);
    return size;
}

int error(const char *const fmt, ...) {
    pre_cmd_();
    fprintf(stderr, "%s: ", prog_name_);
    va_list ap;
    va_start(ap, fmt);
    int size = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return size;
}
