## $Id: Makefile,v 1.11 2005-02-20 16:39:17 Trocotronic Exp $

OBJS=bdd.o hash.o ircd.o main.o match.o md5.o modulos.o mysql.o \
	parseconf.o protocolos.o smtp.o socks.o soporte.o sprintf_irc.o ssl.o zlib.o
SRC=$(OBJS:%.o=%.c)
MAKEARGS =	'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
		'LDFLAGS=${LDFLAGS}' 'BINDIR=${BINDIR}' 'INSTALL=${INSTALL}' \
		'INCLUDEDIR=${INCLUDEDIR}' 'CRYPTOLIB=${CRYPTOLIB}' \
		'RM=${RM}' 'CP=${CP}' 'TOUCH=${TOUCH}'
MAKE = make $(MAKEARGS)
INCLUDES = ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/protocolos.h ../include/mysql.h ../include/parseconf.h \
	../include/sprintf_irc.h ../include/struct.h
all: build
build: ircd mods prots
ircd: $(OBJS)
	$(CC) $(CFLAGS) $(CRYPTOLIB) -o colossus $(OBJS) $(IRCDLIBS) $(LDFLAGS) $(CRYPTOLIB)
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
	
bdd.o: bdd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c bdd.c
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
sprintf_irc.o: sprintf_irc.c $(INCLUDES)
	$(CC) $(CFLAGS) -c sprintf_irc.c
ssl.o: ssl.c $(INCLUDES)
	$(CC) $(CFLAGS) -c ssl.c
zlib.o: zlib.c $(INCLUDES)
	$(CC) $(CFLAGS) -c zlib.c
clean:
	$(RM) -f *.o *.so *~ core colossus; \
	cd modulos; make clean
