## $Id: make,v 1.13 2005-03-14 14:18:05 Trocotronic Exp $

CC=cl
LINK=link
RC=rc
DEBUG=1

#### SOPORTE ZLIB ####
ZLIB=1
#
###### FIN ZLIB ######

#### SOPORTE UDB ####
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
DBGCFLAG=/MD /G5 /Zi /W3
DBGLFLAG=/debug
MODDBGCFLAG=/LDd $(DBGCFLAG)
!ELSE
DBGCFLAG=/MD /O2 /G5
MODDBGCFLAG=/LD /MD
!ENDIF 

!IFDEF UDB
UDBFLAGS=/D UDB
UDBOBJ=SRC/BDD.OBJ
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
LFLAGS=kernel32.lib user32.lib ws2_32.lib oldnames.lib shell32.lib comctl32.lib ./src/libmysql.lib $(ZLIBLIB) ./src/pthreadVC1.lib \
	$(OPENSSL_LIB) $(SSLLIBS) Dbghelp.lib \
	/nologo $(DBGLFLAG) /out:Colossus.exe /def:colossus.def /implib:colossus.lib
EXP_OBJ_FILES=SRC/GUI.OBJ SRC/HASH.OBJ SRC/IRCD.OBJ SRC/MAIN.OBJ \
	SRC/MATCH.OBJ SRC/MD5.OBJ SRC/MODULOS.OBJ SRC/MYSQL.OBJ SRC/PARSECONF.OBJ SRC/PROTOCOLOS.OBJ \
	SRC/SMTP.OBJ SRC/SOCKS.OBJ SRC/SOPORTE.OBJ SRC/SPRINTF_IRC.OBJ $(ZLIBOBJ) $(SSLOBJ) $(UDBOBJ)
MOD_DLL=SRC/MODULOS/CHANSERV.DLL SRC/MODULOS/NICKSERV.DLL SRC/MODULOS/MEMOSERV.DLL \
	SRC/MODULOS/OPERSERV.DLL SRC/MODULOS/IPSERV.DLL SRC/MODULOS/PROXYSERV.DLL 
#	SRC/MODULOS/STATSERV.DLL SRC/MODULOS/LINKSERV.DLL
OBJ_FILES=$(EXP_OBJ_FILES) SRC/WIN32/COLOSSUS.RES SRC/DEBUG.OBJ
!IFDEF UDB
PROT_DLL=SRC/PROTOCOLOS/UNREAL_UDB.DLL
!ELSE
PROT_DLL=SRC/PROTOCOLOS/UNREAL.DLL SRC/PROTOCOLOS/P10.DLL SRC/PROTOCOLOS/REDHISPANA.DLL
!ENDIF

INCLUDES = ./include/ircd.h ./include/md5.h \
	./include/modulos.h ./include/mysql.h ./include/parseconf.h ./include/protocolos.h \
	./include/sprintf_irc.h ./include/struct.h ./include/ssl.h ./include/zlib.h  \
	./include/sistema.h make
MODCFLAGS=$(MODDBGCFLAG) $(ZLIBCFLAGS) $(UDBFLAGS) $(SSLCFLAGS) /Fesrc/modulos/ /Fosrc/modulos/ /nologo /I ./INCLUDE $(OPENSSL_INC) /J /D MODULE_COMPILE $(UDBFLAGS)
MODLFLAGS=/link /def:src/modulos/modulos.def colossus.lib ./src/libmysql.lib $(OPENSSL_LIB) $(SSLLIBS)
PROTCFLAGS=$(MODDBGCFLAG) $(ZLIBCFLAGS) $(UDBFLAGS) $(SSLCFLAGS) /Fesrc/protocolos/ /Fosrc/protocolos/ /nologo /I ./INCLUDE $(OPENSSL_INC) /J /D MODULE_COMPILE $(UDBFLAGS)
PROTLFLAGS=/link /def:src/protocolos/protocolos.def colossus.lib ./src/libmysql.lib $(OPENSSL_LIB) $(SSLLIBS)

ALL: SETUP COLOSSUS.EXE MODULES

MX: src/modulos/mx.c $(INCLUDES)
	$(CC) $(MODDBGCFLAG) /Fesrc/modulos/ /Fosrc/modulos/ /nologo src/modulos/mx.c /link dnsapi.lib
	-@copy src\modulos\mx.dll mx.dll
	
AUTOCONF: src/win32/autoconf.c
	$(CC) src/win32/autoconf.c
	-@autoconf.exe

CLEAN:
	-@erase src\*.obj >NUL
	-@erase .\*.exe >NUL
	-@erase .\*.ilk >NUL
	-@erase .\*.pdb >NUL
	-@erase .\*.core >NUL
	-@erase .\*.exp >NUL
	-@erase colossus.lib >NUL
	-@erase src\modulos\*.exp >NUL
	-@erase src\modulos\*.lib >NUL
	-@erase src\modulos\*.pdb >NUL
	-@erase src\modulos\*.ilk >NUL
	-@erase src\modulos\*.dll >NUL
	-@erase src\modulos\*.obj >NUL
	-@erase src\protocolos\*.exp >NUL
	-@erase src\protocolos\*.lib >NUL
	-@erase src\protocolos\*.pdb >NUL
	-@erase src\protocolos\*.ilk >NUL
	-@erase src\protocolos\*.dll >NUL
	-@erase src\protocolos\*.obj >NUL
	-@erase src\win32\colossus.res >NUL
	-@erase include\setup.h >NUL

SETUP: 
	-@copy src\win32\setup.h include\setup.h >NUL

./COLOSSUS.EXE: $(OBJ_FILES) DEF
        $(LINK) $(LFLAGS) $(OBJ_FILES) /MAP
	
!IFNDEF DEBUG
 @echo Version no Debug compilada ...
!ELSE
 @echo Version Debug compilada ... 
!ENDIF

src/bdd.obj: src/bdd.c $(INCLUDES)
        $(CC) $(CFLAGS) src/bdd.c
        
src/debug.obj: src/win32/debug.c $(INCLUDES)
        $(CC) $(CFLAGS) src/win32/debug.c
  
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
        
src/protocolos.obj: src/protocolos.c $(INCLUDES)
        $(CC) $(CFLAGS) src/protocolos.c         
        
src/smtp.obj: src/smtp.c $(INCLUDES)
        $(CC) $(CFLAGS) src/smtp.c    
        
src/socks.obj: src/socks.c $(INCLUDES)
	$(CC) $(CFLAGS) src/socks.c 
	
src/soporte.obj: src/soporte.c $(INCLUDES)
	$(CC) $(CFLAGS) src/soporte.c
        
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
	
DEF: 
	$(CC) src/win32/def-clean.c
	dlltool --output-def colossus.def.in --export-all-symbols $(EXP_OBJ_FILES)
	def-clean colossus.def.in colossus.def
       
MODULES: $(MOD_DLL) $(PROT_DLL)
        
src/modulos/chanserv.dll: src/modulos/chanserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/chanserv.c $(MODLFLAGS)
        -@copy src\modulos\chanserv.dll modulos\chanserv.dll >NUL
        
src/modulos/nickserv.dll: src/modulos/nickserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/nickserv.c $(MODLFLAGS) ws2_32.lib src/modulos/chanserv.lib  
        -@copy src\modulos\nickserv.dll modulos\nickserv.dll >NUL
        
src/modulos/operserv.dll: src/modulos/operserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/operserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/operserv.c $(MODLFLAGS) src/modulos/memoserv.lib src/modulos/chanserv.lib
        -@copy src\modulos\operserv.dll modulos\operserv.dll >NUL
        
src/modulos/memoserv.dll: src/modulos/memoserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/memoserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	-@copy src\modulos\memoserv.dll modulos\memoserv.dll >NUL
	
src/modulos/ipserv.dll: src/modulos/ipserv.c $(INCLUDES) ./include/modulos/ipserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/ipserv.c $(MODLFLAGS)
	-@copy src\modulos\ipserv.dll modulos\ipserv.dll >NUL
		      
src/modulos/proxyserv.dll: src/modulos/proxyserv.c $(INCLUDES) ./include/modulos/proxyserv.h
	$(CC) $(MODCFLAGS) src/modulos/proxyserv.c $(MODLFLAGS)
	-@copy src\modulos\proxyserv.dll modulos\proxyserv.dll >NUL

src/modulos/statserv.dll: src/modulos/statserv.c $(INCLUDES) ./include/modulos/statserv.h
	$(CC) $(MODCFLAGS) src/modulos/statserv.c $(MODLFLAGS)
	-@copy src\modulos\statserv.dll modulos\statserv.dll >NUL
	
src/modulos/linkserv.dll: src/modulos/linkserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/linkserv.h
	$(CC) $(MODCFLAGS) src/modulos/linkserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	-@copy src\modulos\linkserv.dll modulos\linkserv.dll >NUL
	
src/protocolos/unreal.dll: src/protocolos/unreal.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/unreal.c $(PROTLFLAGS)
	-@copy src\protocolos\unreal.dll protocolos\unreal.dll >NUL

src/protocolos/p10.dll: src/protocolos/p10.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/p10.c $(PROTLFLAGS) ws2_32.lib
	-@copy src\protocolos\p10.dll protocolos\p10.dll >NUL
	
src/protocolos/redhispana.dll: src/protocolos/redhispana.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/redhispana.c $(PROTLFLAGS) ws2_32.lib
	-@copy src\protocolos\redhispana.dll protocolos\redhispana.dll >NUL
	
src/protocolos/unreal_udb.dll: src/protocolos/unreal.c $(INCLUDES) 
	-@copy src\protocolos\unreal.c src\protocolos\unreal_udb.c >NUL
	$(CC) $(PROTCFLAGS) src/protocolos/unreal_udb.c $(PROTLFLAGS)
	-@copy src\protocolos\unreal_udb.dll protocolos\unreal_udb.dll >NUL
	-@erase src\protocolos\unreal_udb.c >NUL
	