INCLUDES = ../include/bdd.h ../include/comandos.h \
	../include/flags.h ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/mysql.h ../include/parseconf.h \
	../include/sprintf_irc.h ../include/struct.h
R_MODULES=chanserv.so nickserv.so operserv.so ipserv.so \
	memoserv.so statserv.so proxyserv.so linkserv.so
COMMANDS=chanserv.o nickserv.o operserv.o ipserv.o \
	memoserv.o statserv.o proxyserv.o linkserv.o	
MODULES=$(R_MODULES)
MODULEFLAGS=-fPIC -DPIC -shared
all: build
build: $(MODULES)
custommodule: $(MODULEFILE).c
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o $(MODULEFILE).so $(MODULEFILE).c 
chanserv.so: chanserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o chanserv.so chanserv.c
nickserv.so: nickserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o nickserv.so nickserv.c
operserv.so: operserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o operserv.so operserv.c
ipserv.so: ipserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o ipserv.so ipserv.c
memoserv.so: memoserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o memoserv.so memoserv.c
statserv.so: statserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o statserv.so statserv.c
proxyserv.so: proxyserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o proxyserv.so proxyserv.c
linkserv.so: linkserv.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o linkserv.so linkserv.c
