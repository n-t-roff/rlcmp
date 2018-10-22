#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "trace_log.h"

#ifdef TRACE_LOG
static FILE *trace_fp;
#endif /* TRACE_LOG */

void trace_open(void)
{
#ifdef TRACE_LOG
    const char path[] = TRACE_LOG;
    const size_t path_len = strlen(path);
    const size_t buf_size = path_len + 32;
    char *const buf = malloc(buf_size);

    if (!buf) {
        trace_fp = stderr;
        return;
    }
    memcpy(buf, path, path_len);
    snprintf(buf + path_len, buf_size - path_len,
             "%lu", (unsigned long)getuid());
    trace_fp = fopen(buf, "w");
    free(buf);

    if (!trace_fp) {
        trace_fp = stderr;
        return;
    }
    setbuf(trace_fp, NULL);
#endif /* TRACE_LOG */
}

void trace_log(const char *const format, ...)
{
#ifndef TRACE_LOG
    (void)format;
#else
    va_list ap;
    va_start(ap, format);
    vfprintf(trace_fp, format, ap);
    va_end(ap);
#endif /* TRACE_LOG */
}
