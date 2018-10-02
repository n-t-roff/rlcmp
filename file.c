/*
 * Copyright (c) 2016-2017, Carsten Kunze
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
/* because dir.h included below uses avlbst types */
#ifdef HAVE_LIBAVLBST
# include <avlbst.h>
#endif
#include <limits.h>
#include "main.h"
#include "dir.h"
#include "file.h"
#include "summary.h"
#include "progress.h"
#include "output.h"

static char buff1[BUFF_SIZ];
static char buff2[BUFF_SIZ];

/* Returns -1 on error, 1 for difference and 0 else. */

int
filediff(void) {
	int diff = 0;
#ifdef MMAP_MEMCMP
	off_t offs, left;
	size_t len;
	char *buf1, *buf2;
#endif
    if (ign_cont) {
        ++total_file_count; /* -C: Only count files */
        return 0;
    }
    if (progress)
        show_progress(path1, buff1);

    const int fd1 = open(path1, O_RDONLY);
    if (fd1 == -1) {
        error("open(%s): %s\n", path1, strerror(errno));
        set_exit_error();
        return -1;
    }
    const int fd2 = open(path2, O_RDONLY);
    if (fd2 == -1) {
        error("open(%s): %s\n", path2, strerror(errno));
        set_exit_error();
        goto cls1;
    }
#ifdef MMAP_MEMCMP
	offs = 0;
	left = stat1.st_size;
	while (left) {
		len = left < (off_t)pagesiz ? (size_t)left : (size_t)pagesiz;
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
			SET_EXIT_DIFF();
			diff = 1;
			left = (off_t)len;
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
#else
	while (1) {
        const ssize_t l1 = read(fd1, buff1, BUFF_SIZ);
		if (l1 == -1) {
            error("read(%s): %s\n", path1, strerror(errno));
            set_exit_error();
			break;
		}
        const ssize_t l2 = read(fd2, buff2, BUFF_SIZ);
		if (l2 == -1) {
            error("read(%s): %s\n", path2, strerror(errno));
            set_exit_error();
			break;
		}
		if (l1 != l2 ||
            memcmp(buff1, buff2, (size_t)l1)) {
            output("Different files %s and %s\n", path1, path2);
            set_exit_diff();
			diff = 1;
			break;
		}
        total_byte_count += l1;

        if (l1 < BUFF_SIZ) {
            ++total_file_count; /* count successfully compared files only */
            break;
        }
	}
#endif
	if (close(fd2) == -1) {
        error("close(%s): %s\n", path2, strerror(errno));
		exit(EXIT_ERROR);
	}
cls1:
	if (close(fd1) == -1) {
        error("close(%s): %s\n", path1, strerror(errno));
		exit(EXIT_ERROR);
	}
	return diff;
}

/* Returns -1 on error, 1 for difference and 0 else. */

int
linkdiff(void) {
    if (ign_cont) {
        ++total_file_count; /* -C: Only count files */
        return 0;
    }
    if (progress)
        show_progress(path1, buff1);

    const ssize_t l1 = readlink(path1, buff1, sizeof(buff1) - 1);
    if (l1 == -1) {
        error("readlink(%s): %s\n", path1, strerror(errno));
        set_exit_error();
		return -1;
	}
    const ssize_t l2 = readlink(path2, buff2, sizeof(buff2) - 1);
    if (l2 == -1) {
        error("readlink(%s): %s\n", path2, strerror(errno));
        set_exit_error();
		return -1;
	}
	buff1[l1] = 0;
	buff2[l2] = 0;

	if (l1 != l2 ||
        memcmp(buff1, buff2, (size_t)l1)) {
        output("Different links %s -> %s and %s -> %s\n",
		    path1, buff1, path2, buff2);
        set_exit_diff();
		return 1;
	}
    total_byte_count += l1;
    ++total_file_count;
	return 0;
}
