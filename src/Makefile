## $Id: Makefile,v 1.30 2006-06-20 13:26:45 Trocotronic Exp $

OBJS=hash.o httpd.o ircd.o ircsprintf.o main.o match.o md5.o modulos.o\
	parseconf.o protocolos.o sql.o smtp.o socks.o soporte.o ssl.o zlib.o
SRC=$(OBJS:%.o=%.c)
MAKEARGS =	'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
		'LDFLAGS=${LDFLAGS}' 'INSTALL=${INSTALL}' \
		'INCLUDEDIR=${INCLUDEDIR}' 'CRYPTOLIB=${CRYPTOLIB}' \
		'RM=${RM}' 'CP=${CP}' 'TOUCH=${TOUCH}'
MAKE = make $(MAKEARGS)
INCLUDES = ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/protocolos.h ../include/parseconf.h \
	../include/ircsprintf.h ../include/struct.h
all: build
modulo:
	cd modulos; ${MAKE} 'FUENTE=$(FUENTE)' 'EXLIBS=${EXLIBS}' modulo
build: ircd mods prots sqldb exts
ircd: $(OBJS)
	$(CC) $(CFLAGS) $(CRYPTOLIB) -o colossus $(OBJS) $(IRCDLIBS) $(LDFLAGS) $(CRYPTOLIB)
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
	
hash.o: hash.c $(INCLUDES)
	$(CC) $(CFLAGS) -c hash.c
ircd.o: ircd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ircd.c
main.o: main.c $(INCLUDES)
	$(CC) $(CFLAGS) -c main.c
match.o: match.c $(INCLUDES)
	$(CC) $(CFLAGS) -c match.c
md5.o: md5.c $(INCLUDES)
	$(CC) $(CFLAGS) -c md5.c
modulos.o: modulos.c $(INCLUDES)
	$(CC) $(CFLAGS) -c modulos.c
protocolos.o: protocolos.c $(INCLUDES)
	$(CC) $(CFLAGS) -c protocolos.c
mysql.o: mysql.c $(INCLUDES)
	$(CC) $(CFLAGS) -c mysql.c
parseconf.o: parseconf.c $(INCLUDES)
	$(CC) $(CFLAGS) -c parseconf.c
smtp.o: smtp.c $(INCLUDES)
	$(CC) $(CFLAGS) -c smtp.c
socks.o: socks.c $(INCLUDES)
	$(CC) $(CFLAGS) -c socks.c
soporte.o: soporte.c $(INCLUDES)
	$(CC) $(CFLAGS) -c soporte.c
ircsprintf.o: ircsprintf.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ircsprintf.c
ssl.o: ssl.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ssl.c
sql.o: sql.c $(INCLUDES)
	$(CC) $(CFLAGS) -c sql.c
zlib.o: zlib.c $(INCLUDES)
	$(CC) $(CFLAGS) -c zlib.c
httpd.o: httpd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c httpd.c
clean:
	$(RM) -f *.o *.so *~ core colossus; \
	cd modulos; make clean; cd ..; \
	cd protocolos; make clean; cd ..; \
	cd extensiones; make clean; cd ..; \
	cd sql; make clean
	