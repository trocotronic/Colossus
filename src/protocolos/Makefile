## $Id: Makefile,v 1.1 2004-12-31 12:28:03 Trocotronic Exp $

INCLUDES = ../include/ircd.h ../include/md5.h \
	../include/modulos.h ../include/protocolos.h ../include/mysql.h ../include/parseconf.h \
	../include/sprintf_irc.h ../include/struct.h
R_MODULES=unreal.so p10.so
COMMANDS=unreal.o p10.o
MODULES=$(R_MODULES)
MODULEFLAGS=-fPIC -DPIC -shared
all: build
build: $(MODULES)
custommodule: $(MODULEFILE).c
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o $(MODULEFILE).so $(MODULEFILE).c 
unreal.so: unreal.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o unreal.so unreal.c
p10.so: p10.c $(INCLUDES)
	$(CC) $(CFLAGS) $(MODULEFLAGS) -o p10.so p10.c

clean:
	$(RM) -f *.o *.so *~ core
