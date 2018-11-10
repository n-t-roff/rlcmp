/*
 * Copyright (c) 2016, Carsten Kunze
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef USE_SYS_SYSMACROS_H
# include <sys/sysmacros.h>
#endif
#ifdef USE_SYS_MKDEV_H
# include <sys/mkdev.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_LIBAVLBST
# include <avlbst.h>
# include "bst.h"
#endif
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <search.h>
#include "main.h"
#include "dir.h"
#include "file.h"
#include "output.h"

struct stat stat1;

static void pathtoolong(char *, char *);
static void print_time(time_t);
static void print_type(mode_t, int);
static void print_uid(uid_t);
static void print_gid(gid_t);
static void time_cmp(void);
static void perm_cmp(void);
static void usr_cmp(void);
static void grp_cmp(void);

#ifdef HAVE_LIBAVLBST
static void procfilenode(struct bst_node *);
static void procdirnode(struct bst_node *);
static void delfile(struct bst *, struct bst_node *);
static void deldir(struct bst *, struct bst_node *);
static int name_cmp(union bst_val, union bst_val);

static struct bst dirents = { NULL, name_cmp };
#else
struct db_dirent {
	char *name;
	int stat;
};

static void procfilenode(const void *, const VISIT, const int);
static void procdirnode(const void *, const VISIT, const int);
static void procfile(char *, int *);
static void procdir(char *);
static int dirent_cmp(const void *, const void *);

static void *dirents;
#endif

struct stat stat2;

#define FILE_NOENT1 0 /* Not in path1 */
#define FILE_NOENT2 1 /* Not in path2 */
#define FILE_FOUND  2 /* Found in both trees */
#define DEL_NODE    3

void
dircmp(void) {
#ifdef HAVE_LIBAVLBST
	struct bst_node *bst = dirents.root;
	dirents.root = NULL;
#else
	struct db_dirent *dep, *dep2;
	void *vp;
	void *bst = dirents;
	dirents = NULL;
#endif
    DIR *dir  = opendir(path1);
    if (!dir) {
        error("opendir(%s): %s\n", path1, strerror(errno));
        set_exit_error();
		return;
	}
	while (1) {
		errno = 0;
        const struct dirent *dirent = readdir(dir);
        if (!dirent) {
			if (errno) {
                error("readdir(%s): %s\n", path1, strerror(errno));
				exit(EXIT_ERROR);
			}
			break;
		}
        const char *s = dirent->d_name;

		if (*s == '.' && (!s[1] || (s[1] == '.' && !s[2])))
			continue;

#ifdef HAVE_LIBAVLBST
        union bst_val k, v;
        k.p = strdup(s);
        v.i = FILE_NOENT2;
        avl_add(&dirents, k, v);
#else
		dep = malloc(sizeof(struct db_dirent));
		dep->name = strdup(s);
		dep->stat = FILE_NOENT2;
		tsearch(dep, &dirents, dirent_cmp);
#endif
	}
	if (closedir(dir) == -1) {
        error("closedir(%s): %s\n", path1, strerror(errno));
		exit(EXIT_ERROR);
	}
	if (!(dir = opendir(path2))) {
        error("opendir(%s): %s\n", path2, strerror(errno));
        set_exit_error();
		return;
	}
	while (1) {
#ifdef HAVE_LIBAVLBST
		struct bst_node *n;
		int i;
#endif
		errno = 0;
        const struct dirent *dirent = readdir(dir);
        if (!dirent) {
			if (errno) {
                error("readdir(%s): %s\n", path1, strerror(errno));
				exit(EXIT_ERROR);
			}
			break;
		}
        const char *s = dirent->d_name;

		if (*s == '.' && (!s[1] || (s[1] == '.' && !s[2])))
			continue;

#ifdef HAVE_LIBAVLBST
        union bst_val k, v;
        k.cp = s;
        if ((i = bst_srch(&dirents, k, &n))) {
            k.p = strdup(s);
            v.i = FILE_NOENT1;
            avl_add_at(&dirents, k, v, i, n);
        } else {
            v.i = FILE_FOUND;
            n->data = v;
        }
#else
		dep = malloc(sizeof(struct db_dirent));
		dep->name = strdup(s);
		dep->stat = FILE_NOENT1;
		vp = tsearch(dep, &dirents, dirent_cmp);
		dep2 = *(struct db_dirent **)vp;

		if (dep2 != dep) {
			/* Entry did exist already */

			free(dep->name);
			free(dep);
			dep2->stat = FILE_FOUND;
		}
#endif
	}

	if (closedir(dir) == -1) {
        error("closedir(%s): %s\n", path2, strerror(errno));
		exit(EXIT_ERROR);
	}
	path1[path1len++] = '/';
	path1[path1len  ] =  0 ;
	path2[path2len++] = '/';
	path2[path2len  ] =  0 ;
#ifdef HAVE_LIBAVLBST
	proctree(&dirents, procfilenode, delfile);
	proctree(&dirents, procdirnode , deldir );
	dirents.root = bst;
#else
	twalk(dirents, procfilenode);
	twalk(dirents, procdirnode );

	while (dirents) {
		dep = *(struct db_dirent **)dirents;
		tdelete(dep, &dirents, dirent_cmp);
		free(dep->name);
		free(dep);
	}

	dirents = bst;
#endif
	path1[--path1len] = 0;
	path2[--path2len] = 0;
}

void
typetest(int *st)
{
	if (lstat(path1, &stat1) == -1) {
        error("lstat(%s): %s\n", path1, strerror(errno));
        set_exit_error();
		stat1.st_mode = 0;
	}

	if (lstat(path2, &stat2) == -1) {
        error("lstat(%s): %s\n", path2, strerror(errno));
        set_exit_error();
		stat2.st_mode = 0;
	}

	if (!stat1.st_mode || !stat2.st_mode) {
		/* Only possible on error */

		if (st)
			*st = DEL_NODE;

        set_exit_diff();
		return;
	}

    if (stat1.st_ino == stat2.st_ino &&
            stat1.st_dev == stat2.st_dev)
        return;

    if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT)) {
        if (!quiet) {
            output("Different file types for %s (", path1);
            print_type(stat1.st_mode, 0);
            fprintf(msg_fp, ") and %s (", path2);
            print_type(stat2.st_mode, 0);
            fprintf(msg_fp, ")\n");
        }

        if (st)
            *st = DEL_NODE;

        set_exit_diff();
        return;
    }

	if (S_ISDIR(stat1.st_mode)) {
		if (!st) /* Called from main() */
			dircmp();
		if (cmp_perm)
			perm_cmp();
		if (cmp_usr)
			usr_cmp();
		if (cmp_grp)
			grp_cmp();
		return;
	}

    if (st)
        *st = DEL_NODE;

    if (stat1.st_size != stat2.st_size) {
        if (!quiet) {
            output("Different sizes for ");
            print_type(stat1.st_mode, 1);
            fprintf(msg_fp, "s %s (%ju) and %s (%ju)\n", path1,
                   (uintmax_t)stat1.st_size, path2,
                   (uintmax_t)stat2.st_size);
        }
        set_exit_diff();
        return;
    }

    if (S_ISREG(stat1.st_mode)) {
        if (stat1.st_size && filediff())
            return;

    } else if (S_ISLNK(stat1.st_mode)) {
        if (stat1.st_size && linkdiff())
            return;

    } else if (S_ISCHR(stat1.st_mode) || S_ISBLK(stat1.st_mode)) {
        if (stat1.st_rdev != stat2.st_rdev) {
            output("Different %s devices %s (%lu, %lu) and "
                   "%s (%lu, %lu)\n",
                   S_ISCHR(stat1.st_mode) ? "character" : "block",
                   path1, (unsigned long)major(stat1.st_rdev),
                   (unsigned long)minor(stat1.st_rdev),
                   path2, (unsigned long)major(stat2.st_rdev),
                   (unsigned long)minor(stat2.st_rdev));
            set_exit_diff();
            return;
        }
    } else if (S_ISSOCK(stat1.st_mode) || S_ISFIFO(stat1.st_mode)) {
        return; /* compare not necessary */
    } else {
        /* unreached */
        error("%s: Unsupported file type\n", path1);
        exit_code = EXIT_ERROR;
    }

    if (cmp_time)
        time_cmp();
    if (cmp_perm)
        perm_cmp();
    if (cmp_usr)
        usr_cmp();
    if (cmp_grp)
        grp_cmp();
}

static void
time_cmp(void) {
    if ((!ign_link_time || !S_ISLNK(stat1.st_mode)) &&
            stat1.st_mtime != stat2.st_mtime)
    {
        if (!quiet) {
            output("Different modification time for ");
            print_type(stat1.st_mode, 1);
            fprintf(msg_fp, "s %s (", path1);
            print_time(stat1.st_mtime);
            fprintf(msg_fp, ") and %s (", path2);
            print_time(stat2.st_mtime);
            fprintf(msg_fp, ")\n");
        }
        set_exit_diff();
    }
}

static void
perm_cmp(void) {
    if (!S_ISLNK(stat1.st_mode) &&
            (!ign_dir_perm || !S_ISDIR(stat1.st_mode)) &&
            (stat1.st_mode != stat2.st_mode))
    {
        if (!quiet) {
            output("Different permissions for ");
            print_type(stat1.st_mode, 1);
            fprintf(msg_fp, "s %s (%04o) and %s (%04o)\n",
                   path1, (unsigned)stat1.st_mode & 07777,
                   path2, (unsigned)stat2.st_mode & 07777);
        }
        set_exit_diff();
    }
}

static void
usr_cmp(void) {
    if (stat1.st_uid != stat2.st_uid) {
        if (!quiet) {
            output("Different file owner for ");
            print_type(stat1.st_mode, 1);
            fprintf(msg_fp, "s %s (", path1);
            print_uid(stat1.st_uid);
            fprintf(msg_fp, ") and %s (", path2);
            print_uid(stat2.st_uid);
            fprintf(msg_fp, ")\n");
        }
        set_exit_diff();
    }
}

static void
grp_cmp(void) {
    if (stat1.st_gid != stat2.st_gid) {
        if (!quiet) {
            output("Different group ID for ");
            print_type(stat1.st_mode, 1);
            fprintf(msg_fp, "s %s (", path1);
            print_gid(stat1.st_gid);
            fprintf(msg_fp, ") and %s (", path2);
            print_gid(stat2.st_gid);
            fprintf(msg_fp, ")\n");
        }
        set_exit_diff();
    }
}

static void
print_type(mode_t m, int n) {
	if      (S_ISREG(m))
        fputs("regular file", msg_fp);
	else if (S_ISDIR(m))
        fprintf(msg_fp, "director%s", n ? "ie" : "y");
	else if (S_ISLNK(m))
        fputs("symbolic link", msg_fp);
	else if (S_ISCHR(m))
        fputs("character device", msg_fp);
	else if (S_ISBLK(m))
        fputs("block device", msg_fp);
	else if (S_ISFIFO(m))
        fputs("FIFO", msg_fp);
	else if (S_ISSOCK(m))
        fputs("socket", msg_fp);
	else
        fputs("unknown", msg_fp);
}

static void
print_time(time_t t) {
	struct tm *tm;
	tm = localtime(&t);
	if (time(NULL) - t > 18 * 3600)
        fprintf(msg_fp, "%d-%02d-%02d ", tm->tm_year + 1900,
		    tm->tm_mon + 1, tm->tm_mday);
    fprintf(msg_fp, "%d:%02d", tm->tm_hour, tm->tm_min);
}

static void
print_uid(uid_t u) {
	struct passwd *p;
	if ((p = getpwuid(u)))
        fputs(p->pw_name, msg_fp);
	else
        fprintf(msg_fp, "%d", u);
}

static void
print_gid(gid_t g) {
	struct group *p;
	if ((p = getgrgid(g)))
        fputs(p->gr_name, msg_fp);
	else
        fprintf(msg_fp, "%d", g);
}

#ifdef HAVE_LIBAVLBST
static void
procfilenode(struct bst_node *n)
{
	char *s = n->key.p;
	int *st = &n->data.i;
#else
static void
procfilenode(const void *n, const VISIT which, const int d)
{
	struct db_dirent *e;

	(void)d;

	switch (which) {
	case postorder:
	case leaf:
		e = *(struct db_dirent * const *)n;
		procfile(e->name, &e->stat);
		break;
	default:
		;
	}
}

static void
procfile(char *s, int *st)
{
#endif
    switch (*st) {
    case FILE_NOENT1:
        if (ignore_missing & 1)
            return;
        if (report_unexpect)
            output("Only in %s: %s\n", path2, s);
        else
            output("Not in %s: %s\n", path1, s);
        set_exit_diff();
        return;
    case FILE_NOENT2:
        if (ignore_missing & 2)
            return;
        if (report_unexpect)
            output("Only in %s: %s\n", path1, s);
        else
            output("Not in %s: %s\n", path2, s);
        set_exit_diff();
        return;
    }
    const size_t l = strlen(s);

	if (path1len + l > PATH_SIZ) {
		pathtoolong(path1, s);
		return;
	}
	if (path2len + l > PATH_SIZ) {
		pathtoolong(path2, s);
		return;
	}
	memcpy(path1 + path1len, s, l);
	path1[path1len + l] = 0;
	memcpy(path2 + path2len, s, l);
	path2[path2len + l] = 0;
	typetest(st);
	path1[path1len] = 0;
	path2[path2len] = 0;
	return;
}

#ifdef HAVE_LIBAVLBST
static void
delfile(struct bst *t, struct bst_node *n) {
	if (n->data.i != FILE_FOUND) {
		free(n->key.p);
		bst_del_node(t, n);
	}
}
#endif

#ifdef HAVE_LIBAVLBST
static void
procdirnode(struct bst_node *n)
{
	char *s = n->key.p;
#else
static void
procdirnode(const void *n, const VISIT which, const int d)
{
	struct db_dirent *e;

	(void)d;

	switch (which) {
	case postorder:
	case leaf:
		e = *(struct db_dirent * const *)n;

		if (e->stat == FILE_FOUND)
			procdir(e->name);

		break;
	default:
		;
	}
}

static void
procdir(char *s)
{
#endif
	if (cmp_depth) {
		if (depth)
			depth--;
		else
			return;
	}
    const size_t l = strlen(s);

	if (path1len + l > PATH_SIZ) {
		pathtoolong(path1, s);
		return;
	}
	if (path2len + l > PATH_SIZ) {
		pathtoolong(path2, s);
		return;
	}
	memcpy(path1 + path1len, s, l);
	memcpy(path2 + path2len, s, l);
	path1len += l;
	path2len += l;
	path1[path1len] = 0;
	path2[path2len] = 0;
	dircmp();
	path1len -= l;
	path2len -= l;
	path1[path1len] = 0;
	path2[path2len] = 0;
	depth++;
}

#ifdef HAVE_LIBAVLBST
static void
deldir(struct bst *t, struct bst_node *n) {
	free(n->key.p);
	bst_del_node(t, n);
}

static int
name_cmp(union bst_val a, union bst_val b) {
	return strcmp(a.p, b.p);
}
#else
static int
dirent_cmp(const void *a, const void *b)
{
	return strcmp(
	    ((const struct db_dirent *)a)->name,
	    ((const struct db_dirent *)b)->name);
}
#endif

static void
pathtoolong(char *p, char *f) {
    error("Path buffer overflow for %s/%s\n", p, f);
}
