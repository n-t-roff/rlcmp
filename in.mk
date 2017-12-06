BIN=		rlcmp
PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/share/man
INCDIR=		$(PREFIX)/include
LIBDIR=		$(PREFIX)/lib

STRP=		-s
# Remove comment to enable mmap(2) + memcmp(3).  Else read(2) + memcmp(3) is
# used.  While it had been stated that mmap is faster than read, benchmarks
# on modern systems show that read(2) ist faster.
MMAP=		#-DMMAP_MEMCMP

OBJ=		main.o dir.o bst.o file.o
_CFLAGS=	$(CFLAGS) $(CPPFLAGS) $(DEFINES) \
		$(__CDBG) $(__CLDBG) \
		$(MMAP)
_LDFLAGS=	$(LDFLAGS) $(__CLDBG) -L${LIBDIR} -Wl,-rpath,${LIBDIR} \
		$(STRP)
LDADD=		$(LIB_AVLBST)

all: $(BIN)

install: $(BINDIR) $(MANDIR)/man1
	install $(BIN) $(BINDIR)/
	install -m 644 $(BIN).1 $(MANDIR)/man1/

uninstall:
	rm -f $(BINDIR)/$(BIN)
	rm -f $(MANDIR)/man1/$(BIN).1

clean:
	rm -f $(BIN) $(OBJ) *.gc??

distclean: clean
	rm -f Makefile config.log

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
