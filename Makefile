## $Id: Makefile,v 1.18 2005-01-01 20:19:07 Trocotronic Exp $

CC=gcc
INCLUDEDIR=../include
IRCDLIBS=-ldl -lmysqlclient -lssl -lzlibc -lpthread
XCFLAGS=-pipe -g -O3 -fexpensive-optimizations -Wall -funsigned-char -export-dynamic -DUDB -DUSA_ZLIB -DUSA_SSL
CFLAGS=-I$(INCLUDEDIR) $(XCFLAGS)
SUBDIRS=src
LDFLAGS=
RM=/bin/rm
INSTALL=/usr/bin/install
TOUCH=/bin/touch
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

clean:
	$(RM) -f *~ \#* core *.orig include/*.orig
	@for i in $(SUBDIRS); do \
		echo "Limpiando $$i";\
		( cd $$i; ${MAKE} ${MAKEARGS} clean; ) \
	done
install: all
	$(INSTALL) -m 0700 -d ~/Colossus
	$(INSTALL) -m 0700 src/colossus ~/Colossus/colossus
	$(TOUCH) ~/Colossus/colossus.conf
	chmod 0600 ~/Colossus/colossus.conf
	$(INSTALL) -m 0700 -d ~/Colossus/modulos
	$(INSTALL) -m 0700 src/modulos/*.so ~/Colossus/modulos
	$(INSTALL) -m 0700 -d ~/Colossus/protocolos
	$(INSTALL) -m 0700 src/protocolos/*.so ~/Colossus/protocolos
