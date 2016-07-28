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
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <avlbst.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "main.h"
#include "dir.h"
#include "bst.h"
#include "file.h"

struct stat stat1;

static void procfile(struct bst_node *);
static void delfile(struct bst *, struct bst_node *);
static void procdir(struct bst_node *);
static void deldir(struct bst *, struct bst_node *);
static int name_cmp(union bst_val, union bst_val);
static void pathtoolong(char *, char *);
static void print_time(time_t);
static void print_type(mode_t, int);
static void print_uid(uid_t);
static void print_gid(gid_t);
static void time_cmp(void);
static void perm_cmp(void);
static void usr_cmp(void);
static void grp_cmp(void);

static struct bst dirents = { NULL, name_cmp };
static DIR *dir;
static struct dirent *dirent;
static struct stat stat2;

#define FILE_NOENT 0
#define FILE_FOUND 1
#define DEL_NODE   2

void
dircmp(void) {
	struct bst_node *bst = NULL;
	dirents.root = bst;
	if (!(dir = opendir(path1))) {
		fprintf(stderr, "%s: opendir \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		return;
	}
	while (1) {
		char *s;
		errno = 0;
		if (!(dirent = readdir(dir))) {
			if (errno) {
				fprintf(stderr,
				    "%s: readdir \"%s\" failed: %s\n",
				    prog, path1, strerror(errno));
				exit(EXIT_ERROR);
			}
			break;
		}
		s = dirent->d_name;
		if (*s == '.' && (!s[1] || (s[1] == '.' && !s[2])))
			continue;
		avl_add(&dirents, (union bst_val)(void *)strdup(s),
		    (union bst_val)(int)FILE_NOENT);
	}
	if (closedir(dir) == -1) {
		fprintf(stderr, "%s: closedir \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		exit(EXIT_ERROR);
	}
	if (!(dir = opendir(path2))) {
		fprintf(stderr, "%s: opendir \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		return;
	}
	while (1) {
		char *s;
		struct bst_node *n;
		errno = 0;
		if (!(dirent = readdir(dir))) {
			if (errno) {
				fprintf(stderr,
				    "%s: readdir \"%s\" failed: %s\n",
				    prog, path1, strerror(errno));
				exit(EXIT_ERROR);
			}
			break;
		}
		s = dirent->d_name;
		if (*s == '.' && (!s[1] || (s[1] == '.' && !s[2])))
			continue;
		if (bst_srch(&dirents, (union bst_val)(void *)s, &n))
			if (report_unexpect)
				printf("Only in %s/: %s\n", path2, s);
			else
				printf("Not in %s/: %s\n", path1, s);
		else
			n->data = (union bst_val)(int)FILE_FOUND;
	}
	if (closedir(dir) == -1) {
		fprintf(stderr, "%s: closedir \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		exit(EXIT_ERROR);
	}
	path1[path1len++] = '/';
	path1[path1len  ] =  0 ;
	path2[path2len++] = '/';
	path2[path2len  ] =  0 ;
	proctree(&dirents, procfile, delfile);
	proctree(&dirents, procdir , deldir );
	path1[--path1len] = 0;
	path2[--path2len] = 0;
}

void
typetest(struct bst_node *n) {
	if (lstat(path1, &stat1) == -1) {
		fprintf(stderr, "%s: lstat \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		exit(EXIT_ERROR);
	}
	if (lstat(path2, &stat2) == -1) {
		fprintf(stderr, "%s: lstat \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		exit(EXIT_ERROR);
	}
	if ((stat1.st_mode & S_IFMT) != (stat2.st_mode & S_IFMT)) {
		printf("Different file types for %s (", path1);
		print_type(stat1.st_mode, 0);
		printf(") and %s (", path2);
		print_type(stat2.st_mode, 0);
		printf(")\n");
		SET_EXIT_DIFF();
		if (n)
			n->data.i = DEL_NODE;
		return;
	}
	if (S_ISDIR(stat1.st_mode)) {
		if (!n) /* Called from main() */
			dircmp();
		if (cmp_perm)
			perm_cmp();
		if (cmp_usr)
			usr_cmp();
		if (cmp_grp)
			grp_cmp();
		return;
	}
	if (n)
		n->data.i = DEL_NODE;
	if (stat1.st_size != stat2.st_size) {
		printf("Different sizes for ");
		print_type(stat1.st_mode, 1);
		printf("s %s (%ju) and %s (%ju)\n", path1, stat1.st_size,
		    path2, stat2.st_size);
		SET_EXIT_DIFF();
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
			printf("Different %s devices %s (%u, %u) and "
			    "%s (%u, %u)\n",
			    S_ISCHR(stat1.st_mode) ? "character" : "block",
			    path1, major(stat1.st_rdev), minor(stat1.st_rdev),
			    path2, major(stat2.st_rdev), minor(stat2.st_rdev));
			SET_EXIT_DIFF();
			return;
		}
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
	if (stat1.st_mtime != stat2.st_mtime) {
		printf("Different modification time for ");
		print_type(stat1.st_mode, 1);
		printf("s %s (", path1);
		print_time(stat1.st_mtime);
		printf(") and %s (", path2);
		print_time(stat2.st_mtime);
		printf(")\n");
		SET_EXIT_DIFF();
	}
}

static void
perm_cmp(void) {
	if (!S_ISLNK(stat1.st_mode) && (stat1.st_mode != stat2.st_mode)) {
		printf("Different permissions for ");
		print_type(stat1.st_mode, 1);
		printf("s %s (%04o) and %s (%04o)\n",
		    path1, stat1.st_mode & 07777, path2, stat2.st_mode &
		    07777);
		SET_EXIT_DIFF();
	}
}

static void
usr_cmp(void) {
	if (stat1.st_uid != stat2.st_uid) {
		printf("Different file owner for ");
		print_type(stat1.st_mode, 1);
		printf("s %s (", path1);
		print_uid(stat1.st_uid);
		printf(") and %s (", path2);
		print_uid(stat2.st_uid);
		printf(")\n");
		SET_EXIT_DIFF();
	}
}

static void
grp_cmp(void) {
	if (stat1.st_gid != stat2.st_gid) {
		printf("Different group ID for ");
		print_type(stat1.st_mode, 1);
		printf("s %s (", path1);
		print_gid(stat1.st_gid);
		printf(") and %s (", path2);
		print_gid(stat2.st_gid);
		printf(")\n");
		SET_EXIT_DIFF();
	}
}

static void
print_type(mode_t m, int n) {
	if      (S_ISREG(m))
		fputs("regular file", stdout);
	else if (S_ISDIR(m))
		printf("director%s", n ? "ie" : "y");
	else if (S_ISLNK(m))
		fputs("symbolic link", stdout);
	else if (S_ISCHR(m))
		fputs("character device", stdout);
	else if (S_ISBLK(m))
		fputs("block device", stdout);
	else if (S_ISFIFO(m))
		fputs("FIFO", stdout);
	else if (S_ISSOCK(m))
		fputs("socket", stdout);
	else
		fputs("unknown", stdout);
}

static void
print_time(time_t t) {
	struct tm *tm;
	tm = localtime(&t);
	if (time(NULL) - t > 18 * 3600)
		printf("%d-%02d-%02d ", tm->tm_year + 1900,
		    tm->tm_mon + 1, tm->tm_mday);
	printf("%d:%02d", tm->tm_hour, tm->tm_min);
}

static void
print_uid(uid_t u) {
	struct passwd *p;
	if ((p = getpwuid(u)))
		fputs(p->pw_name, stdout);
	else
		printf("%d", u);
}

static void
print_gid(gid_t g) {
	struct group *p;
	if ((p = getgrgid(g)))
		fputs(p->gr_name, stdout);
	else
		printf("%d", g);
}

static void
procfile(struct bst_node *n) {
	size_t l;
	char *s = (char *)n->key.p;
	if (n->data.i == FILE_NOENT) {
		if (report_unexpect)
			printf("Only in %s: %s\n", path1, s);
		else
			printf("Not in %s: %s\n", path2, s);
		n->data.i = DEL_NODE;
		return;
	}
	l = strlen(s);
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
	typetest(n);
	path1[path1len] = 0;
	path2[path2len] = 0;
}

static void
delfile(struct bst *t, struct bst_node *n) {
	if (n->data.i == DEL_NODE) {
		free(n->key.p);
		bst_del_node(t, n);
	}
}

static void
procdir(struct bst_node *n) {
	size_t l;
	char *s;
	if (cmp_depth) {
		if (depth)
			depth--;
		else
			return;
	}
	s = (char *)n->key.p;
	l = strlen(s);
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

static void
deldir(struct bst *t, struct bst_node *n) {
	free(n->key.p);
	bst_del_node(t, n);
}

static int
name_cmp(union bst_val a, union bst_val b) {
	return strcmp(a.p, b.p);
}

static void
pathtoolong(char *p, char *f) {
	fprintf(stderr, "%s: Path buffer overflow for %s/%s\n", prog, p, f);
}
