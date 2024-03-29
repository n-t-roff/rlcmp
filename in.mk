BIN = rlcmp
MAN = rlcmp
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man
INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

TRACE_LOG = #-DTRACE_LOG='"/tmp/.$(BIN)_trace_"'
STRP = -s

OBJ = \
	main.o dir.o bst.o file.o term_info.o summary.o progress.o \
	output.o unit_prefix.o format_time.o trace_log.o
_CFLAGS = \
	$(CFLAGS) $(CPPFLAGS) $(DEFINES) $(__CDBG) $(__CLDBG) \
	-I$(INCDIR) \
	$(INCDIR_CURSES) \
	$(TRACE_LOG) #-DDEBUG_DELAY
_LDFLAGS = \
	$(LDFLAGS) $(__CLDBG) $(STRP) \
	-L${LIBDIR} -Wl,-rpath,${LIBDIR} \
	$(RPATH_CURSES) $(LIBDIR_CURSES) \
	-pthread  -lpthread
LDADD = $(LIB_AVLBST) $(LIB_CURSES)

all: $(BIN)

install: $(BINDIR) $(MANDIR)/man1
	install $(BIN) $(BINDIR)/
	install -m 644 $(MAN).1 $(MANDIR)/man1/

uninstall:
	rm -f $(BINDIR)/$(BIN)
	rm -f $(MANDIR)/man1/$(MAN).1

clean:
	rm -f $(BIN) $(OBJ) *.gc??

distclean: clean
	rm -f Makefile config.log compat.h

$(BINDIR) $(MANDIR)/man1:
	mkdir -p $@

$(BIN): $(OBJ)
	$(CC) $(_CFLAGS) $(_LDFLAGS) -o $@ $(OBJ) $(LDADD)

.c.o:
	$(CC) $(_CFLAGS) -c $<

main.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h
bst.o:		Makefile $(INCDIR)/avlbst.h
dir.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h bst.h
file.o:		Makefile $(INCDIR)/avlbst.h
