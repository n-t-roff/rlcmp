PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/share/man
INCDIR=		$(PREFIX)/include
LIBDIR=		$(PREFIX)/lib
BIN=		rlcmp
OBJ=		main.o dir.o bst.o file.o
_CFLAGS=	$(CFLAGS) $(CPPFLAGS) $(DEFINES) $(__CDBG) $(__SAN) -Wall \
		-Wextra
_LDFLAGS=	$(LDFLAGS) $(__SAN) \
		-s

$(BIN):		$(OBJ)
		$(CC) $(_LDFLAGS) -L${LIBDIR} \
		    -Wl,-rpath,${LIBDIR} -o $@ $(OBJ) -lavlbst

install:
		[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
		install $(BIN) $(BINDIR)/
		[ -d $(MANDIR)/man1 ] || mkdir -p $(MANDIR)/man1
		install -m 644 $(BIN).1 $(MANDIR)/man1/

uninstall:
		rm -f $(BINDIR)/$(BIN) $(MANDIR)/man1/$(BIN).1

clean:
		rm -f $(BIN) $(OBJ)

distclean:	clean
		rm -f Makefile config.log

.c.o:
		$(CC) $(_CFLAGS) -I$(INCDIR) -c $<

main.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h
bst.o:		Makefile $(INCDIR)/avlbst.h
dir.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h bst.h
file.o:		Makefile $(INCDIR)/avlbst.h
