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
#include <pthread.h>
/* because dir.h included below uses avlbst types */
#ifdef HAVE_LIBAVLBST
# include <avlbst.h>
#endif
#include <limits.h>
#include <time.h>
#include "main.h"
#include "dir.h"
#include "file.h"
#include "summary.h"
#include "progress.h"
#include "output.h"

static void create_thread(pthread_t *id, void *(*routine) (void *), void *arg);
static void join_thread(pthread_t id, void **value);
static void *map(void *arg);
static void *cmp(void *arg);

struct map_input
{
    int fd;
    off_t offs;
    size_t len;
    const char *path;
};

struct cmp_input
{
    size_t len;
    char *buf1;
    char *buf2;
};

static char buff1[BUFF_SIZ];
static char buff2[BUFF_SIZ];

/* Returns -1 on error, 1 for difference and 0 else. */

int filediff(void)
{
    time_t start_time;
    if (wait_flag)
    {
        start_time = time(NULL);
    }
    int diff = 0;
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
    if (use_mmap)
    {
        off_t offs = 0;
        off_t left = stat1.st_size;
        size_t len;
        char *buf1;
        char *buf2;
        struct cmp_input cmp_input;
        struct map_input map_input1;
        struct map_input map_input2;
        int valid = 0;
        pthread_t map_thread;
        pthread_t cmp_thread;
        void *map_value;
        void *cmp_value;
        map_input1.fd = fd1;
        map_input2.fd = fd2;
        map_input1.path = path1;
        map_input2.path = path2;
        int use_single_thread = single_thread || left < (off_t)pagesiz ? 1 : 0;
        while (left && !diff) {
            if (valid && !single_thread)
            {
                create_thread(&cmp_thread, cmp, &cmp_input);
            }
            len = left < (off_t)pagesiz ? (size_t)left : (size_t)pagesiz;
            map_input1.len = len;
            map_input1.offs = offs;
            if (use_single_thread)
            {
                buf1 = map(&map_input1);
            }
            else
            {
                create_thread(&map_thread, map, &map_input1);
            }
            map_input2.len = len;
            map_input2.offs = offs;
            buf2 = map(&map_input2);
            if (!use_single_thread)
            {
                join_thread(map_thread, &map_value);
                buf1 = map_value;
            }
            offs += len;
            left -= len;
            if (valid)
            {
                if (single_thread)
                {
                    cmp_value = cmp(&cmp_input);
                }
                else
                {
                    join_thread(cmp_thread, &cmp_value);
                }
                if (cmp_value)
                {
                    diff = 1;
                    break;
                }
            }
            valid = 1;
            cmp_input.len = len;
            cmp_input.buf1 = buf1;
            cmp_input.buf2 = buf2;
            total_byte_count += len;
        }
        if (!diff)
        {
            if (!cmp((void *)&cmp_input))
            {
                ++total_file_count; /* count successfully compared files only */
            }
            else
            {
                diff = 1;
                total_byte_count -= len;
            }
        }
    }
    else
    {
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
    }
    if (close(fd2) == -1) {
        error("close(%s): %s\n", path2, strerror(errno));
        exit(EXIT_ERROR);
    }
cls1:
    if (close(fd1) == -1) {
        error("close(%s): %s\n", path1, strerror(errno));
        exit(EXIT_ERROR);
    }
    if (wait_flag)
    {
        time_t duration = time(NULL) - start_time;
        if (duration)
        {
            sleep(duration);
        }
    }
    return diff;
}

void create_thread(pthread_t *id, void *(*routine) (void *), void *arg)
{
    int thread_error = pthread_create(id, NULL, routine, arg);
    if (thread_error != 0)
    {
        error("pthread_create() failed: %s\n", strerror(thread_error));
        exit(EXIT_ERROR);
    }
}

void join_thread(pthread_t id, void **value)
{
    int thread_error = pthread_join(id, value);
    if (thread_error != 0)
    {
        error("pthread_create() failed: %s\n", strerror(thread_error));
        exit(EXIT_ERROR);
    }
}

void *map(void *arg)
{
    struct map_input *map_input = (struct map_input *)arg;
    char *buf;
    if ((buf = mmap(NULL, map_input->len, PROT_READ, MAP_PRIVATE, map_input->fd, map_input->offs)) == MAP_FAILED)
    {
        fprintf(stderr, "%s: mmap \"%s\" failed: %s\n", prog, map_input->path, strerror(errno));
        exit(EXIT_ERROR);
    }
    return buf;
}

void *cmp(void *arg)
{
    struct cmp_input *cmp_input = (struct cmp_input *)arg;
    size_t len = cmp_input->len;
    char *buf1 = cmp_input->buf1;
    char *buf2 = cmp_input->buf2;
    intptr_t diff = 0;
    if (memcmp(buf1, buf2, len))
    {
        printf("Different files %s and %s\n", path1, path2);
        set_exit_diff();
        diff = 1;
    }
    if (msync(buf1, len, MS_INVALIDATE) == -1)
    {
        fprintf(stderr, "%s: msync \"%s\" failed: %s\n", prog, path1, strerror(errno));
    }
    if (msync(buf2, len, MS_INVALIDATE) == -1)
    {
        fprintf(stderr, "%s: msync \"%s\" failed: %s\n", prog, path2, strerror(errno));
    }
    if (munmap(buf2, len) == -1)
    {
        fprintf(stderr, "%s: munmap \"%s\" failed: %s\n", prog, path2, strerror(errno));
        exit(EXIT_ERROR);
    }
    if (munmap(buf1, len) == -1)
    {
        fprintf(stderr, "%s: munmap \"%s\" failed: %s\n", prog, path1, strerror(errno));
        exit(EXIT_ERROR);
    }
    return (void *)diff;
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
