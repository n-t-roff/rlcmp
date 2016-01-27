#define PATH_SIZ (PATH_MAX > 8192 ? PATH_MAX : 8192) /* for realpath() */
#define EXIT_DIFF() exit_code = 1
#define EXIT_ERROR 2
extern char *prog;
extern char path1[PATH_SIZ];
extern char path2[PATH_SIZ];
extern size_t path1len;
extern size_t path2len;
extern size_t pagesiz;
extern int exit_code;
extern int cmp_perm;
extern int cmp_time;
extern int cmp_usr;
extern int cmp_grp;
extern int cmp_depth;
extern int depth;
