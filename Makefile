## $Id: Makefile,v 1.3 2004-09-14 04:25:24 Trocotronic Exp $

CC=gcc
INCLUDEDIR=../include
IRCDLIBS=-ldl -lmysqlclient
XCFLAGS=-pipe -g -O3 -fexpensive-optimization -funsigned-char -export-dynamic
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
