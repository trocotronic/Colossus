## $Id: Makefile,v 1.9 2004-09-17 23:47:58 Trocotronic Exp $

CC=gcc
INCLUDEDIR=../include
IRCDLIBS=-ldl -lmysqlclient
XCFLAGS=-pipe -g -O3 -fexpensive-optimizations -Wall -funsigned-char -export-dynamic -DUDB
CFLAGS=-I$(INCLUDEDIR) $(XCFLAGS)
SUBDIRS=src
LDFLAGS=
RM=/bin/rm
INSTALL=/usr/bin/install
MAKEARGS =	'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
		'INCLUDEDIR=${INCLUDEDIR}' 'RM=${RM}' 'LDFLAGS=$(LDFLAGS)'
all:	build
build:
	@for i in $(SUBDIRS); do \
		echo "Building $$i";\
		( cd $$i; ${MAKE} ${MAKEARGS} build; ) \
	done
	@echo ' __________________________________________________ '
	@echo '| Compilación terminada.                           |'
	@echo '|__________________________________________________|'
	${MAKE} install
clean:
	$(RM) -f *~ \#* core *.orig include/*.orig
	@for i in $(SUBDIRS); do \
		echo "Cleaning $$i";\
		( cd $$i; ${MAKE} ${MAKEARGS} clean; ) \
	done
install: all
	$(INSTALL) -m 0700 -d ~/Colossus
	$(INSTALL) -m 0700 src/colossus ~/Colossus/src/colossus
	$(TOUCH) ~/Colossus/colossus.conf
	chmod 0600 ~/Colossus/colossus.conf
	$(INSTALL) -m 0700 servicios ~/Colossus
	$(INSTALL) -m 0700 -d ~/Colossus/modulos
	$(INSTALL) -m 0700 src/modulos/*.so ~/Colossus/modulos
