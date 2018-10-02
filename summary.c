#include <stdint.h>
#include <stdio.h>
#include "summary.h"

long total_file_count;
off_t total_byte_count;
short summary;

void output_summary(void) {
    printf("%'ld files ", total_file_count);
    if (total_byte_count)
        printf("(%'jd bytes) ", (intmax_t)total_byte_count);
    printf("compared\n");
}
