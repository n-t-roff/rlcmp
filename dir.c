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
		printf("Different file types for %s and %s\n", path1, path2);
		EXIT_DIFF();
		if (n)
			n->data.i = DEL_NODE;
		return;
	}
	if (cmp_time && !S_ISDIR(stat1.st_mode) &&
	    (stat1.st_mtime != stat2.st_mtime)) {
		printf("Different modification time for %s and %s\n", path1,
		    path2);
		EXIT_DIFF();
		if (n)
			n->data.i = DEL_NODE;
		return;
	}
	if (cmp_perm && !S_ISLNK(stat1.st_mode) &&
	    (stat1.st_mode != stat2.st_mode)) {
		printf("Different permissions for %s and %s\n", path1, path2);
		EXIT_DIFF();
		if (!S_ISDIR(stat1.st_mode)) {
			if (n)
				n->data.i = DEL_NODE;
			return;
		}
	}
	if (cmp_usr && (stat1.st_uid != stat2.st_uid)) {
		printf("Different file owner for %s and %s\n", path1, path2);
		EXIT_DIFF();
		if (!S_ISDIR(stat1.st_mode)) {
			if (n)
				n->data.i = DEL_NODE;
			return;
		}
	}
	if (cmp_grp && (stat1.st_gid != stat2.st_gid)) {
		printf("Different group ID for %s and %s\n", path1, path2);
		EXIT_DIFF();
		if (!S_ISDIR(stat1.st_mode)) {
			if (n)
				n->data.i = DEL_NODE;
			return;
		}
	}
	if (S_ISDIR(stat1.st_mode)) {
		if (!n) /* Called from main() */
			dircmp();
		return;
	}
	if (n)
		n->data.i = DEL_NODE;
	if (stat1.st_size != stat2.st_size) {
		printf("Different sizes for %s and %s\n", path1, path2);
		EXIT_DIFF();
		return;
	}
	if (!stat1.st_size)
		return;
	if (S_ISREG(stat1.st_mode))
		filediff();
	else if (S_ISLNK(stat1.st_mode))
		linkdiff();
	else if (S_ISCHR(stat1.st_mode) || S_ISBLK(stat1.st_mode)) {
		if (stat1.st_rdev != stat2.st_rdev) {
			printf("Different special devices %s and %s\n", path1,
			    path2);
			EXIT_DIFF();
		}
	}
}

static void
procfile(struct bst_node *n) {
	size_t l;
	char *s = (char *)n->key.p;
	if (n->data.i == FILE_NOENT) {
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
	char *s = (char *)n->key.p;
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
