OBJS=bdd.o flags.o hash.o ircd.o main.o match.o \
	md5.o modulos.o mysql.o parseconf.o smtp.o sprintf_irc.o 
SRC=$(OBJS:%.o=%.c)
MAKEARGS =	'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
		'INCLUDEDIR=${INCLUDEDIR}' 'RM=${RM}'
MAKE = make $(MAKEARGS)
INCLUDES = ../include/bdd.h ../include/comandos.h \
	../include/flags.h ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/mysql.h ../include/parseconf.h \
	../include/sprintf_irc.h ../include/struct.h
all: build
build: ircd mods
ircd: $(OBJS)
	$(CC) $(CFLAGS) -o colossus $(OBJS) $(IRCDLIBS) $(LDFLAGS)
mods:
	@if [ ! -r include ] ; then \
		ln -s ../include include; \
	fi
	cd modulos; $(MAKE) all
bdd.o: bdd.c $(INCLUDES)
	$(CC) $(CFLAGS) -c bdd.c
flags.o: flags.c $(INCLUDES)
	$(CC) $(CFLAGS) -c flags.c
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
mysql.o: mysql.c $(INCLUDES)
	$(CC) $(CFLAGS) -c mysql.c
parseconf.o: parseconf.c $(INCLUDES)
	$(CC) $(CFLAGS) -c parseconf.c
smtp.o: smtp.c $(INCLUDES)
	$(CC) $(CFLAGS) -c smtp.c
sprintf_irc.o: sprintf_irc.c $(INCLUDES)
	$(CC) $(CFLAGS) -c sprintf_irc.c
