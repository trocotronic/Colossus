## $Id: Makefile,v 1.6 2004-09-17 23:33:52 Trocotronic Exp $

CC=gcc
INCLUDEDIR=../include
IRCDLIBS=-ldl -lmysqlclient
XCFLAGS=-pipe -g -O3 -fexpensive-optimizations -Wall -funsigned-char -export-dynamic -DUDB
CFLAGS=-I$(INCLUDEDIR) $(XCFLAGS)
SUBDIRS=src
LDFLAGS=
RM=/bin/rm
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
		echo "Cleaning $$i";\
		( cd $$i; ${MAKE} ${MAKEARGS} clean; ) \
	done
	fi
