#include <stdint.h>
#include <stdio.h>
#include "summary.h"
#include "main.h"

long total_file_count;
off_t total_byte_count;
short summary;

void output_summary(void) {
    fprintf(msg_fp, "%'ld files ", total_file_count);
    if (total_byte_count)
        fprintf(msg_fp, "(%'jd bytes) ", (intmax_t)total_byte_count);
    fprintf(msg_fp, "compared\n");
}
