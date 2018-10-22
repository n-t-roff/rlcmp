# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

HEADERS = \
   bst.h \
   dir.h \
   file.h \
   main.h \
    ver.h \
    term_info.h \
    summary.h \
    progress.h \
    output.h \
    unit_prefix.h \
    format_time.h \
    trace_log.h

SOURCES = \
   bst.c \
   dir.c \
   file.c \
   main.c \
    term_info.c \
    summary.c \
    progress.c \
    output.c \
    unit_prefix.c \
    format_time.c \
    trace_log.c

DEFINES = \
    HAVE_LIBAVLBST \
    USE_SYS_SYSMACROS_H \
    TRACE_LOG=\'\"\"\'
