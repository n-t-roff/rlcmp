#ifndef PROGRESS_H
#define PROGRESS_H

extern short progress;

void show_progress(const char *const path, char *buf);
void clear_progress_line(void);

#endif
