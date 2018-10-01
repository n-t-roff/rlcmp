#ifndef SUMMARY_H
#define SUMMARY_H

#include <sys/types.h>

extern long total_file_count;
extern off_t total_byte_count;
extern short summary;

void output_summary(void);

#endif
