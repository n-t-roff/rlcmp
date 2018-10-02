#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "progress.h"
#include "term_info.h"
#include "summary.h"
#include "main.h"

static const time_t period = 10;
short progress;

void progress_init(void) {
    print_time(); /* set t0 */
}

inline static int time_elapsed(void) {
    static time_t t1;
    time_t t2 = time(NULL);
    if (!t1)
        t1 = t2;
    else if (t2 - t1 >= period)
    {
        t1 = t2;
        return 1;
    }
    return 0;
}

int print_time(void) {
    static time_t t0;
    if (!t0) {
        t0 = time(NULL);
        return 0;
    }
    time_t dt = time(NULL) - t0;
    int size = 0;
    time_t t = dt / (60 * 60); /* hours */
    if (t) {
        size += printf("%ld:", t);
        dt %= 60 * 60;
    }
    t = dt / 60; /* minutes */
    size += printf("%02ld:", t);
    if (t)
        dt %= 60;
    size += printf("%02ld ", dt);
    return size;
}

void show_progress(const char *const path, char *buf) {
    if (!time_elapsed())
        return;
    ti_clr_eol();
    int cols_left = ti_get_cols();
    if (cols_left <= 0)
        return;
    cols_left -= print_time();
    cols_left -= printf("%'ldF %'jdB ",
                        total_file_count, (intmax_t)total_byte_count);
    if (cols_left <= 0)
        return;
    memcpy(buf, path + ini_path1len + 1, (size_t)cols_left);
    buf[cols_left] = 0;
    printf("%s\r", buf);
    fflush(stdout);
}

void clear_progress_line(void) {
    if (progress)
        ti_clr_eol();
}
