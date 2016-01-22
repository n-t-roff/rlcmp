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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <avlbst.h>
#include <limits.h>
#include "main.h"
#include "dir.h"

void
filediff(void) {
	int fd1, fd2;
	off_t offs, left;
	size_t len;
	char *buf1, *buf2;
	if ((fd1 = open(path1, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: open \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		return;
	}
	if ((fd2 = open(path1, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: open \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		goto cls1;
	}
	offs = 0;
	left = stat1.st_size;
	while (left) {
		len = left < pagesiz ? left : pagesiz;
		if ((buf1 = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd1, offs))
		    == MAP_FAILED) {
			fprintf(stderr, "%s: mmap \"%s\" failed: %s\n", prog,
			    path1, strerror(errno));
			exit(EXIT_ERROR);
		}
		if ((buf2 = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd2, offs))
		    == MAP_FAILED) {
			fprintf(stderr, "%s: mmap \"%s\" failed: %s\n", prog,
			    path2, strerror(errno));
			exit(EXIT_ERROR);
		}
		if (memcmp(buf1, buf2, len)) {
			printf("Different files %s and %s\n", path1, path2);
			EXIT_DIFF();
			left = len;
		}
		if (msync(buf1, len, MS_INVALIDATE) == -1) {
			fprintf(stderr, "%s: msync \"%s\" failed: %s\n", prog,
			    path1, strerror(errno));
		}
		if (msync(buf2, len, MS_INVALIDATE) == -1) {
			fprintf(stderr, "%s: msync \"%s\" failed: %s\n", prog,
			    path2, strerror(errno));
		}
		if (munmap(buf2, len) == -1) {
			fprintf(stderr, "%s: munmap \"%s\" failed: %s\n",
			    prog, path2, strerror(errno));
			exit(EXIT_ERROR);
		}
		if (munmap(buf1, len) == -1) {
			fprintf(stderr, "%s: munmap \"%s\" failed: %s\n",
			    prog, path1, strerror(errno));
			exit(EXIT_ERROR);
		}
		offs += len;
		left -= len;
	}
	if (close(fd2) == -1) {
		fprintf(stderr, "%s: close \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		exit(EXIT_ERROR);
	}
cls1:
	if (close(fd1) == -1) {
		fprintf(stderr, "%s: close \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		exit(EXIT_ERROR);
	}
}

void
linkdiff(void) {
	static char buf1[PATH_SIZ];
	static char buf2[PATH_SIZ];
	ssize_t l1, l2;
	if ((l1 = readlink(path1, buf1, sizeof buf1)) == -1) {
		fprintf(stderr, "%s: readlink \"%s\" failed: %s\n", prog,
		    path1, strerror(errno));
		return;
	}
	if ((l2 = readlink(path2, buf2, sizeof buf2)) == -1) {
		fprintf(stderr, "%s: readlink \"%s\" failed: %s\n", prog,
		    path2, strerror(errno));
		return;
	}
	if (l1 != l2) {
		printf("Different link length for %s and %s\n", path1, path2);
		EXIT_DIFF();
		return;
	}
	if (memcmp(buf1, buf2, l1)) {
		printf("Different symlinks %s and %s\n", path1, path2);
		EXIT_DIFF();
	}
}
