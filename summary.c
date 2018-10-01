#include <stdint.h>
#include <stdio.h>
#include "summary.h"

long total_file_count;
off_t total_byte_count;
short summary;

void output_summary(void) {
    printf("%'ld files (%'jd bytes) compared\n",
           total_file_count, (intmax_t)total_byte_count);
}
