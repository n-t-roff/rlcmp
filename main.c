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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_LIBAVLBST
# include <avlbst.h>
#endif
#include <sys/stat.h>
#include <locale.h>
#include "ver.h"
#include "main.h"
#include "dir.h"
#include "term_info.h"
#include "summary.h"
#include "progress.h"
#include "output.h"
#include "trace_log.h"

char *prog;
char path1[PATH_SIZ];
char path2[PATH_SIZ];
size_t path1len;
size_t path2len;
size_t ini_path1len;
long pagesiz;
int exit_code;
static char **args;
int cmp_perm;
int cmp_time;
int cmp_usr;
int cmp_grp;
int cmp_depth;
int depth;
int report_unexpect;
int ign_dir_perm;
int ign_link_time;
FILE *msg_fp;
short ign_cont;
short quiet;
short exit_on_error;
short ignore_missing;
short follow_cli_symlinks;
char wait_flag;

static void usage(const char *) __attribute__ ((noreturn));

int
main(int argc, char **argv) {
	static int noopts;
	static char *s;
	static int c;
    setlocale(LC_ALL, "");
    msg_fp = stdout;
    trace_open();
    prog = *argv++;
    --argc;
    while (!noopts && argc && *(s = *argv) == '-') {
        ++argv;
        --argc;
        while ((c = *++s)) {
			switch (c) {
			case '-':
				noopts = 1;
                break;
			case 'A':
				ign_dir_perm = 1;
				ign_link_time = 1;
				/* fall through */
			case 'a':
				cmp_perm = 1;
				cmp_time = 1;
				cmp_usr  = 1;
				cmp_grp  = 1;
				break;
			case 'C':
				ign_cont = 1;
				break;
            case 'D':
                ign_dir_perm = 1;
                break;
            case 'd':
				cmp_depth = 1;
                if (!(c = *++s) && argc) {
                    s = *argv++;
                    --argc;
                    c = *s;
				}
				if (c < '0' || c > '9')
					usage("Option -d needs a "
					    "number as argument");
                depth = atoi(s);
                goto next;
            case 'E':
                msg_fp = stderr;
                break;
            case 'e':
                exit_on_error = 1;
                break;
			case 'g':
				cmp_grp  = 1;
				break;
            case 'H':
                follow_cli_symlinks = 1;
                break;
			case 'L':
				ign_link_time = 1;
				break;
            case 'M':
                if (!(c = *++s) && argc) {
                    s = *argv++;
                    --argc;
                    c = *s;
                }
                if (s[1])
                    c = 0; /* Illegal argument */
                switch (c) {
                case '0': ignore_missing = ~0; break;
                case '1': ignore_missing =  1; break;
                case '2': ignore_missing =  2; break;
                default:
                    usage("Argument for option -M must be 0, 1, or 2");
                }
                goto next;
			case 'm':
				cmp_perm = 1;
				break;
			case 'o':
				report_unexpect = 1;
				break;
            case 'p':
                progress = 1;
                break;
            case 'q':
                quiet = 1;
                break;
            case 's':
                summary = 1;
                break;
			case 't':
				cmp_time = 1;
				break;
			case 'u':
				cmp_usr  = 1;
				break;
			case 'V':
				printf("%s "VERSION"\n"
				       "\tCompile option(s): "
				    "use "
#ifdef MMAP_MEMCMP
				    "mmap"
#else
				    "read"
#endif
				    "(2) for compare, "
#ifdef HAVE_LIBAVLBST
				    "libavlbst"
#else
				    "tsearch"
#endif
				    "\n", prog);
				exit(0);
            case 'W':
                wait_flag = 1;
                break;
			default:
				fprintf(stderr, "%s: Unknown option '%c'\n",
				    prog, c);
				usage(NULL);
			}
		}
next:
        ;
	}
	if (argc != 2)
		usage("Wrong number of arguments");

	args = argv;

    if (lstat(*argv, &stat1) == -1)
    {
        fprintf(stderr, "%s: lstat \"%s\" failed: %s\n", prog, *argv, strerror(errno));
        exit(EXIT_ERROR);
    }
    if (!follow_cli_symlinks && S_ISLNK(stat1.st_mode))
    {
        strncpy(path1, *argv, PATH_SIZ - 1);
        path1[PATH_SIZ - 1] = 0;
    }
    else if (!realpath(*argv, path1))
    {
        fprintf(stderr, "%s: realpath \"%s\" failed: %s\n", prog, *argv, strerror(errno));
        exit(EXIT_ERROR);
    }

    ini_path1len = path1len = strlen(path1);
	argv++;

    if (lstat(*argv, &stat2) == -1) {
        fprintf(stderr, "%s: lstat \"%s\" failed: %s\n", prog, *argv, strerror(errno));
        exit(EXIT_ERROR);
    }
    if (!follow_cli_symlinks && S_ISLNK(stat2.st_mode))
    {
        strncpy(path2, *argv, PATH_SIZ - 1);
        path2[PATH_SIZ - 1] = 0;
    }
    else if (!realpath(*argv, path2))
    {
        fprintf(stderr, "%s: realpath \"%s\" failed: %s\n", prog, *argv, strerror(errno));
		exit(EXIT_ERROR);
	}

    path2len = strlen(path2);

    if (follow_cli_symlinks)
    {
        if (stat(path1, &stat1) == -1)
        {
            fprintf(stderr, "%s: stat \"%s\" failed: %s\n", prog, path1, strerror(errno));
            exit(EXIT_ERROR);
        }
        if (stat(path2, &stat2) == -1)
        {
            fprintf(stderr, "%s: stat \"%s\" failed: %s\n", prog, path2, strerror(errno));
            exit(EXIT_ERROR);
        }
    }

    if (stat1.st_ino == stat2.st_ino && stat1.st_dev == stat2.st_dev)
    {
        return 0;
    }

#ifdef MMAP_MEMCMP
	errno = 0;
	if ((pagesiz = sysconf(_SC_PAGESIZE)) == -1) {
		fprintf(stderr, "%s: sysconf(_SC_PAGESIZE) failed: ", prog);
		if (errno)
			perror(NULL);
		else
			fputs("Not supported\n", stderr);
		return EXIT_ERROR;
	}
#endif
    term_info_init();
    progress_init();
    output_init(prog, quiet, clear_progress_line, msg_fp);
    typetest(NULL);
    if (summary) {
        clear_progress_line();
        print_elapsed_time();
        putchar(' ');
        output_summary();
    }
    return exit_code;
}

static void usage(const char *s) {
	if (s)
        fprintf(stderr, "%s: %s\n", prog, s);
	exit(EXIT_ERROR);
}

void set_exit_diff(void) {
    if (!exit_code)
        exit_code = EXIT_DIFF;
    if (exit_on_error)
        exit(exit_code);
}

void set_exit_error(void) {
    exit_code = EXIT_ERROR;
    if (exit_on_error)
        exit(exit_code);
}
