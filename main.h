#ifndef MAIN_H
#define MAIN_H

#include <limits.h>
#include <sys/types.h>

#define PATH_SIZ (PATH_MAX > 1024*16 ? PATH_MAX : 1024*16) /* for realpath() */
#define BUFF_SIZ PATH_SIZ

#define SET_EXIT_DIFF() \
    do { \
        if (!exit_code) \
            exit_code = 1; \
    } while (0)

#define EXIT_ERROR 2
extern char *prog;
extern char path1[PATH_SIZ];
extern char path2[PATH_SIZ];
extern size_t path1len;
extern size_t path2len;
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
extern short ign_cont;

#endif
