#include "format_time.h"

static int time_t_to_hour_min_sec(char *const buf,
                                  const size_t bufsiz,
                                  FILE *file_ptr,
                                  time_t tot_sec);

const struct format_time FormatTime = {
    .time_t_to_hour_min_sec = time_t_to_hour_min_sec
};

static int time_t_to_hour_min_sec(char *const buf,
                                  const size_t bufsiz,
                                  FILE *file_ptr,
                                  time_t tot_sec)
{
    if (!file_ptr)
        file_ptr = stdout;
    int size = 0;
    time_t t = tot_sec / (60 * 60); /* hours */
    if (t) {
        if (buf)
            size += snprintf(buf, bufsiz, "%ld:", t);
        else
            size += fprintf(file_ptr, "%ld:", t);

        tot_sec %= 60 * 60;
    }
    t = tot_sec / 60; /* minutes */

    if (buf)
        size += snprintf(buf, bufsiz, "%02ld:", t);
    else
        size += fprintf(file_ptr, "%02ld:", t);

    if (t)
        tot_sec %= 60;

    if (buf)
        size += snprintf(buf, bufsiz, "%02ld ", tot_sec);
    else
        size += fprintf(file_ptr, "%02ld ", tot_sec);

    return size;
}
