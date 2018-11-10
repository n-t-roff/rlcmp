#include <stdio.h>
#include <stdarg.h>
#include "output.h"
#include "main.h"

static void (*pre_cmd_)(void);
static const char *prog_name_;
static bool quiet_;

void output_init(const char *const prog_name, const bool q,
                 void (*pre_cmd)(void))
{
    prog_name_ = prog_name;
    quiet_ = q;
    pre_cmd_ = pre_cmd;
}

int output(const char *const fmt, ...) {
    if (quiet_)
        return 0;
    pre_cmd_();
    va_list ap;
    va_start(ap, fmt);
    int size = vfprintf(msg_fp, fmt, ap);
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
