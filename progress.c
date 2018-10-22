#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "progress.h"
#include "term_info.h" /* ti_clr_eol, ti_get_cols */
#include "summary.h" /* total_file_count, ... */
#include "main.h" /* ini_path1len */
#include "unit_prefix.h"
#include "format_time.h"

static const time_t period = 10;
short progress;

void progress_init(void) {
    print_elapsed_time(); /* set t0 */
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

int print_elapsed_time(void) {
    static time_t t0;
    if (!t0) {
        t0 = time(NULL);
        return 0;
    }
    return FormatTime.time_t_to_hour_min_sec(NULL, 0, NULL, time(NULL) - t0);
}

void show_progress(const char *const path, char *buf) {
    if (!time_elapsed())
        return;
    ti_clr_eol();
    int cols_left = ti_get_cols();
    if (cols_left <= 0)
        return;
    cols_left -= print_elapsed_time();
    if (total_file_count) {
        --cols_left; putchar(' ');
        cols_left -= UnitPrefix.unit_prefix(NULL, 0, NULL, total_file_count,
                                            UnitPrefix.decimal);
        cols_left -= fputs(" files ", stdout);
        if (total_byte_count) {
            cols_left -= UnitPrefix.unit_prefix(NULL, 0, NULL,
                                                total_byte_count, 0);
            cols_left -= fputs("B ", stdout);
        }
    }
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
