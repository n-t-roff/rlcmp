#ifndef MAIN_H
#define MAIN_H

#include <limits.h>
#include <sys/types.h>

#define PATH_SIZ (PATH_MAX > 1024*16 ? PATH_MAX : 1024*16) /* for realpath() */
#define BUFF_SIZ PATH_SIZ

#define EXIT_DIFF  1
#define EXIT_ERROR 2

void set_exit_diff(void);
void set_exit_error(void);

extern char *prog;
extern char path1[PATH_SIZ];
extern char path2[PATH_SIZ];
extern size_t path1len;
extern size_t path2len;
extern size_t ini_path1len;
extern long pagesiz;
extern int exit_code;
extern int cmp_perm;
extern int cmp_time;
extern int cmp_usr;
extern int cmp_grp;
extern int cmp_depth;
extern int depth;
extern int report_unexpect;
extern int ign_dir_perm;
extern int ign_link_time;
extern FILE *msg_fp;
extern short ign_cont;
extern short quiet;
extern short exit_on_error;
extern short ignore_missing;

#endif
