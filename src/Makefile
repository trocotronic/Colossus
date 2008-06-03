## $Id: Makefile,v 1.41 2008/06/01 11:44:46 Trocotronic Exp $

OBJS=core.o cripto.o eventos.o hash.o httpd.o ircd.o ircsprintf.o main.o match.o md5.o \
	misc.o modulos.o msn.o parseconf.o protocolos.o smtp.o socks.o socksint.o \
	soporte.o sql.o ssl.o version.o zlib.o
SRC=$(OBJS:%.o=%.c)
MAKEARGS =	'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
		'LDFLAGS=${LDFLAGS}' 'INSTALL=${INSTALL}' \
		'INCLUDEDIR=${INCLUDEDIR}' 'CRYPTOLIB=${CRYPTOLIB}' \
		'RM=${RM}' 'CP=${CP}' 'TOUCH=${TOUCH}' 'SHELL=${SHELL}'
MAKE = make $(MAKEARGS)
INCLUDES = ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/protocolos.h ../include/parseconf.h \
	../include/ircsprintf.h ../include/struct.h ../include/socksint.h ../include/version.h
all: build
modulo:
	cd modulos; ${MAKE} 'FUENTE=$(FUENTE)' 'EXLIBS=${EXLIBS}' modulo
modall:
	cd modulos; ${MAKE} 'EXLIBS=${EXLIBS}' modall
build: ircd mods prots exts
ircd: $(OBJS)
	$(CC) $(CFLAGS) $(CRYPTOLIB) -o colossus $(OBJS) $(IRCDLIBS) $(LDFLAGS) $(CRYPTOLIB) `mysql_config --libmysqld-libs`
	ar -r colossus.a $(OBJS)
mods:
	@if [ ! -r include ] ; then \
		ln -s ../include include; \
	fi
	cd modulos; $(MAKE) all
	
prots:
	@if [ ! -r include ] ; then \
		ln -s ../include include; \
	fi
	cd protocolos; $(MAKE) all
	
sqldb:
	@if [ ! -r include ] ; then \
		ln -s ../include include; \
	fi
	cd sql; $(MAKE) all

exts:
	@if [ ! -r include ] ; then \
		ln -s ../include include; \
	fi
	cd extensiones; $(MAKE) all

core.o: core.c $(INCLUDES)
	$(CC) $(CFLAGS) -c core.c
cripto.o: cripto.c $(INCLUDES)
	$(CC) $(CFLAGS) -c cripto.c
eventos.o: eventos.c $(INCLUDES)
	$(CC) $(CFLAGS) -c eventos.c
hash.o: hash.c $(INCLUDES)
	$(CC) $(CFLAGS) -c hash.c
httpd.o: httpd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c httpd.c
ircd.o: ircd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ircd.c
ircsprintf.o: ircsprintf.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ircsprintf.c
main.o: main.c $(INCLUDES)
	$(CC) $(CFLAGS) -c main.c
match.o: match.c $(INCLUDES)
	$(CC) $(CFLAGS) -c match.c
md5.o: md5.c $(INCLUDES)
	$(CC) $(CFLAGS) -c md5.c
misc.o: misc.c $(INCLUDES)
	$(CC) $(CFLAGS) -c misc.c
modulos.o: modulos.c $(INCLUDES)
	$(CC) $(CFLAGS) -c modulos.c
msn.o: msn.c $(INCLUDES)
	$(CC) $(CFLAGS) -c msn.c
parseconf.o: parseconf.c $(INCLUDES)
	$(CC) $(CFLAGS) -c parseconf.c
protocolos.o: protocolos.c $(INCLUDES)
	$(CC) $(CFLAGS) -c protocolos.c
smtp.o: smtp.c $(INCLUDES)
	$(CC) $(CFLAGS) -c smtp.c
socks.o: socks.c $(INCLUDES)
	$(CC) $(CFLAGS) -c socks.c
socksint.o: socksint.c $(INCLUDES)
	$(CC) $(CFLAGS) -c socksint.c
soporte.o: soporte.c $(INCLUDES)
	$(CC) $(CFLAGS) -c soporte.c
sql.o: sql.c $(INCLUDES)
	$(CC) $(CFLAGS) -I/usr/local/mysql/include -c sql.c
ssl.o: ssl.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ssl.c
version.o: version.c ../include/version.h
	$(CC) $(CFLAGS) -c version.c
zlib.o: zlib.c $(INCLUDES)
	$(CC) $(CFLAGS) -c zlib.c

clean:
	$(RM) -f *.o *.so *~ core colossus; \
	cd modulos; make clean; cd ..; \
	cd protocolos; make clean; cd ..; \
	cd extensiones; make clean; cd ..; \
	cd sql; make clean
	