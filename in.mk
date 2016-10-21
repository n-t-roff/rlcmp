PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/share/man
INCDIR=		$(PREFIX)/include
LIBDIR=		$(PREFIX)/lib
# Remove comment to enable mmap(2) + memcmp(3).  Else read(2) + memcmp(3) is
# used.  While it had been stated that mmap is faster than read, benchmarks
# on modern systems show that read(2) ist faster.
MMAP=		#-DMMAP_MEMCMP
BIN=		rlcmp
OBJ=		main.o dir.o bst.o file.o ver.o
_CFLAGS=	$(CFLAGS) $(CPPFLAGS) $(DEFINES) $(__CDBG) $(__SAN) \
		$(MMAP)
_LDFLAGS=	$(LDFLAGS) $(__CDBG) $(__SAN) -L${LIBDIR} -Wl,-rpath,${LIBDIR} \
		-s
LDADD=		$(LIB_AVLBST)

$(BIN):		$(OBJ)
		$(CC) $(_LDFLAGS) -o $@ $(OBJ) $(LDADD)

install:
		[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
		install $(BIN) $(BINDIR)/
		[ -d $(MANDIR)/man1 ] || mkdir -p $(MANDIR)/man1
		install -m 644 $(BIN).1 $(MANDIR)/man1/

uninstall:
		rm -f $(BINDIR)/$(BIN) $(MANDIR)/man1/$(BIN).1

clean:
		rm -f $(BIN) $(OBJ) *.gc??

distclean:	clean
		rm -f Makefile config.log

.c.o:
		$(CC) $(_CFLAGS) -I$(INCDIR) -c $<

main.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h
bst.o:		Makefile $(INCDIR)/avlbst.h
dir.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h bst.h
file.o:		Makefile $(INCDIR)/avlbst.h
