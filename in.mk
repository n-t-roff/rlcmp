BINDIR=		/usr/local/bin
MANDIR=		/usr/local/share/man
INCDIR=		/usr/local/include
LIBDIR=		/usr/local/lib
BIN=		rlcmp
OBJ=		main.o dir.o bst.o file.o

_CFLAGS=	$(CFLAGS) $(_CFLAGS_) -Wall
#_CFLAGS=	$(_CFLAGS_) -g -O0
_LDFLAGS=	-s

$(BIN):		$(OBJ)
		$(CC) $(LDFLAGS) $(_LDFLAGS) -L${LIBDIR} \
		    -Wl,-rpath,${LIBDIR} -o $@ $(OBJ) -lavlbst

install:
		[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
		install $(BIN) $(BINDIR)/
		[ -d $(MANDIR)/man1 ] || mkdir -p $(MANDIR)/man1
		install -m 644 $(BIN).1 $(MANDIR)/man1/

uninstall:
		rm -f $(BINDIR)/$(BIN) $(MANDIR)/man1/$(BIN).1

clean:
		rm -f $(BIN) $(OBJ) Makefile

.c.o:
		$(CC) $(_CFLAGS) $(CPPFLAGS) -I$(INCDIR) -c $<

main.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h
bst.o:		Makefile $(INCDIR)/avlbst.h
dir.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h bst.h
file.o:		Makefile $(INCDIR)/avlbst.h
