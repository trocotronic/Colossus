## $Id: make,v 1.5 2004-10-01 18:55:20 Trocotronic Exp $

CC=cl
LINK=link
RC=rc
DEBUG=1

#### SOPORTE ZLIB ####
ZLIB=1
#
###### FIN ZLIB ######

#### SOPORTE UDB ####
#Soporte UDB
UDB=1
#
###### FIN UDB ######

#### SOPORTE SSL ####
SSL=1
#
OPENSSL_INC_DIR="c:\openssl\include"
OPENSSL_LIB_DIR="c:\openssl\lib"
#
###### FIN SSL ######

!IFDEF DEBUG
DBGCFLAG=/MD /Zi /W3
DBGLFLAG=/debug
MODDBGCFLAG=/LDd /MD /Zi /W3
!ELSE
DBGCFLAG=/MD /O2 /G5
MODDBGCFLAG=/LD /MD
!ENDIF 

!IFDEF UDB
UDBFLAGS=/D UDB
!ENDIF 

!IFDEF ZLIB
ZLIBCFLAGS=/D USA_ZLIB
ZLIBOBJ=SRC/ZLIB.OBJ
ZLIBLIB=src/zdll.lib
!ENDIF

!IFDEF SSL
SSLCFLAGS=/D USA_SSL
SSLLIBS=ssleay32.lib libeay32.lib
SSLOBJ=SRC/SSL.OBJ
!IFDEF OPENSSL_INC_DIR
OPENSSL_INC=/I "$(OPENSSL_INC_DIR)"
!ENDIF
!IFDEF OPENSSL_LIB_DIR
OPENSSL_LIB=/LIBPATH:"$(OPENSSL_LIB_DIR)"
!ENDIF
!ENDIF

CFLAGS=$(DBGCFLAG) /I ./INCLUDE /J $(OPENSSL_INC) /Fosrc/ /nologo $(ZLIBCFLAGS) $(UDBFLAGS) $(SSLCFLAGS) /D _WIN32 /c 
LFLAGS=kernel32.lib user32.lib ws2_32.lib oldnames.lib shell32.lib comctl32.lib ./src/libmysql.lib $(ZLIBLIB) $(OPENSSL_LIB) $(SSLLIBS) Dbghelp.lib \
	/nologo $(DBGLFLAG) /out:Colossus.exe /def:colossus.def /implib:colossus.lib
OBJ_FILES=SRC/BDD.OBJ SRC/DEBUG.OBJ SRC/FLAGS.OBJ SRC/HASH.OBJ SRC/IRCD.OBJ SRC/MAIN.OBJ \
	SRC/MATCH.OBJ SRC/MD5.OBJ SRC/MODULOS.OBJ SRC/MYSQL.OBJ SRC/PARSECONF.OBJ SRC/SMTP.OBJ \
	SRC/SPRINTF_IRC.OBJ $(ZLIBOBJ) $(SSLOBJ) SRC/WIN32/COLOSSUS.RES SRC/GUI.OBJ
BOT_DLL=SRC/MODULOS/CHANSERV.DLL SRC/MODULOS/NICKSERV.DLL SRC/MODULOS/MEMOSERV.DLL SRC/MODULOS/OPERSERV.DLL \
	SRC/MODULOS/IPSERV.DLL SRC/MODULOS/PROXYSERV.DLL SRC/MODULOS/STATSERV.DLL SRC/MODULOS/LINKSERV.DLL \
	SRC/MODULOS/MX.DLL
INCLUDES = ./include/bdd.h ./include/comandos.h \
	./include/flags.h ./include/ircd.h ./include/md5.h \
	./include/modulos.h ./include/mysql.h ./include/parseconf.h \
	./include/sprintf_irc.h ./include/struct.h
MODCFLAGS=$(MODDBGCFLAG) $(SSLCFLAGS) /Fesrc/modulos/ /Fosrc/modulos/ /nologo /I ./INCLUDE $(OPENSSL_INC) /J /D MODULE_COMPILE $(UDBFLAGS)
MODLFLAGS=/link /def:src/modulos/modulos.def colossus.lib ./src/libmysql.lib $(OPENSSL_LIB) $(SSLLIBS)

ALL: COLOSSUS.EXE MODULES

CLEAN:
       -@erase src\*.obj >NUL
       -@erase .\*.exe >NUL
       -@erase .\*.ilk >NUL
       -@erase .\*.pdb >NUL
       -@erase .\*.core >NUL
       -@erase .\*.exp >NUL
       -@erase src\modulos\*.dll >NUL
       -@erase src\modulos\*.obj >NUL
       -@erase colossus.lib >NUL
       -@erase src\modulos\*.exp >NUL
	-@erase src\modulos\*.lib >NUL
	-@erase src\modulos\*.pdb >NUL
	-@erase src\modulos\*.ilk >NUL
	-@erase src\win32\colossus.res >NUL

./COLOSSUS.EXE: $(OBJ_FILES)
        $(LINK) $(LFLAGS) $(OBJ_FILES) /MAP
	
!IFNDEF DEBUG
 @echo Non Debug version built 
!ELSE
 @echo Debug version built ... 
!ENDIF

src/bdd.obj: src/bdd.c $(INCLUDES)
        $(CC) $(CFLAGS) src/bdd.c
        
src/debug.obj: src/debug.c $(INCLUDES)
        $(CC) $(CFLAGS) src/debug.c

src/flags.obj: src/flags.c $(INCLUDES)
        $(CC) $(CFLAGS) src/flags.c
        
src/hash.obj: src/hash.c $(INCLUDES)
        $(CC) $(CFLAGS) src/hash.c        
        
src/ircd.obj: src/ircd.c $(INCLUDES)
        $(CC) $(CFLAGS) src/ircd.c        
        
src/main.obj: src/main.c $(INCLUDES)
        $(CC) $(CFLAGS) src/main.c        
        
src/match.obj: src/match.c $(INCLUDES)
        $(CC) $(CFLAGS) src/match.c        
        
src/md5.obj: src/md5.c $(INCLUDES)
        $(CC) $(CFLAGS) src/md5.c        
        
src/modulos.obj: src/modulos.c $(INCLUDES)
        $(CC) $(CFLAGS) src/modulos.c        
        
src/mysql.obj: src/mysql.c $(INCLUDES)
        $(CC) $(CFLAGS) src/mysql.c        
        
src/parseconf.obj: src/parseconf.c $(INCLUDES)
        $(CC) $(CFLAGS) src/parseconf.c        
        
src/smtp.obj: src/smtp.c $(INCLUDES)
        $(CC) $(CFLAGS) src/smtp.c        
        
src/sprintf_irc.obj: src/sprintf_irc.c $(INCLUDES)
        $(CC) $(CFLAGS) src/sprintf_irc.c        
		
src/zlib.obj: src/zlib.c $(INCLUDES)
	$(CC) $(CFLAGS) src/zlib.c
	
src/ssl.obj: src/ssl.c $(INCLUDES)
	$(CC) $(CFLAGS) src/ssl.c
	
src/win32/colossus.res: src/win32/colossus.rc $(INCLUDES)
        $(RC) /l 0x409 /fosrc/win32/colossus.res /i ./include /i ./src \
              /d NDEBUG src/win32/colossus.rc	
              
src/gui.obj: src/win32/gui.c $(INCLUDES)
	$(CC) $(CFLAGS) src/win32/gui.c
	
	
# Modulos
	
CUSTOMMODULE: src/modulos/$(MODULEFILE).c
	$(CC) $(MODCFLAGS) src/modulos/$(MODULEFILE).c $(MODLFLAGS) $(PARAMS) /OUT:src/modulos/$(MODULEFILE).dlk
       
MODULES: $(BOT_DLL)
        
src/modulos/chanserv.dll: src/modulos/chanserv.c $(INCLUDES) ./include/chanserv.h
        $(CC) $(MODCFLAGS) src/modulos/chanserv.c $(MODLFLAGS)
        
src/modulos/nickserv.dll: src/modulos/nickserv.c $(INCLUDES) ./include/chanserv.h ./include/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/nickserv.c $(MODLFLAGS) ws2_32.lib src/modulos/chanserv.lib  
        
src/modulos/operserv.dll: src/modulos/operserv.c $(INCLUDES) ./include/chanserv.h ./include/memoserv.h ./include/operserv.h
        $(CC) $(MODCFLAGS) src/modulos/operserv.c $(MODLFLAGS) src/modulos/memoserv.lib src/modulos/chanserv.lib
        
src/modulos/memoserv.dll: src/modulos/memoserv.c $(INCLUDES) ./include/chanserv.h ./include/memoserv.h
	$(CC) $(MODCFLAGS) src/modulos/memoserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	
src/modulos/ipserv.dll: src/modulos/ipserv.c $(INCLUDES) ./include/ipserv.h
	$(CC) $(MODCFLAGS) src/modulos/ipserv.c $(MODLFLAGS)	
		      
src/modulos/proxyserv.dll: src/modulos/proxyserv.c $(INCLUDES) ./include/proxyserv.h
	$(CC) $(MODCFLAGS) src/modulos/proxyserv.c $(MODLFLAGS)

src/modulos/statserv.dll: src/modulos/statserv.c $(INCLUDES) ./include/statserv.h
	$(CC) $(MODCFLAGS) src/modulos/statserv.c $(MODLFLAGS)
	
src/modulos/mx.dll: src/modulos/mx.c $(INCLUDES)
	$(CC) $(MODDBGCFLAG) /Fesrc/modulos/ /Fosrc/modulos/ /nologo src/modulos/mx.c /link dnsapi.lib
	-@copy src\modulos\mx.dll mx.dll
	
src/modulos/linkserv.dll: src/modulos/linkserv.c $(INCLUDES) ./include/chanserv.h ./include/linkserv.h
	$(CC) $(MODCFLAGS) src/modulos/linkserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	