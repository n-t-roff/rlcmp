BINDIR=		/usr/local/bin
MANDIR=		/usr/local/share/man
INCDIR=		/usr/local/include
LIBDIR=		/usr/local/lib
BIN=		rlcmp
OBJ=		main.o dir.o bst.o file.o
DBG=		-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls \
		-fsanitize=undefined \
		-fsanitize=integer \
		-fsanitize=address
_CFLAGS=	$(CFLAGS) $(CPPFLAGS) $(DBG) -Wall -Wextra
_LDFLAGS=	$(LDFLAGS) $(DBG) -s

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

.c.o:
		$(CC) $(_CFLAGS) -I$(INCDIR) -c $<

main.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h
bst.o:		Makefile $(INCDIR)/avlbst.h
dir.o:		Makefile $(INCDIR)/avlbst.h main.h dir.h bst.h
file.o:		Makefile $(INCDIR)/avlbst.h
