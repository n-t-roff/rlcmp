#ifndef TRACE_LOG_H
#define TRACE_LOG_H

#include <stdarg.h>

void trace_open(void);
void trace_log(const char *format, ...);

#endif /* TRACE_LOG_H */
