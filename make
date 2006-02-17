## $Id: make,v 1.28 2006-02-17 19:19:01 Trocotronic Exp $

CC=cl
LINK=link
RC=rc
DEBUG=1

#### SOPORTE ZLIB ####
ZLIB=1
ZLIB_INC_DIR="c:\dev\zlib"
ZLIB_LIB_DIR="c:\dev\zlib\dll32"
#
###### FIN ZLIB ######


#### SOPORTE SSL ####
SSL=1
#
OPENSSL_INC_DIR="c:\dev\openssl\include"
OPENSSL_LIB_DIR="C:\dev\openssl\out32dll"
#
###### FIN SSL ######

#### SOPORTE PTHREAD ####
PTHREAD_INC="C:\dev\pthreads"
PTHREAD_LIB="C:\dev\pthreads"
###### FIN PTHREAD ######

!IFDEF DEBUG
DBGCFLAG=/MD /G5 /Zi /W3
DBGLFLAG=/debug
MODDBGCFLAG=/LD $(DBGCFLAG)
!ELSE
DBGCFLAG=/MD /O2 /G5
MODDBGCFLAG=/LD /MD
!ENDIF 

!IFDEF ZLIB
ZLIBCFLAGS=/D USA_ZLIB
ZLIBOBJ=SRC/ZLIB.OBJ
ZLIBLIB=zlibwapi.lib
!IFDEF ZLIB_INC_DIR
ZLIB_INC=/I $(ZLIB_INC_DIR)
!ENDIF
!IFDEF ZLIB_LIB_DIR
ZLIB_LIB=/LIBPATH:$(ZLIB_LIB_DIR)
!ENDIF
!ENDIF

!IFDEF SSL
SSLCFLAGS=/D USA_SSL
SSLLIBS=ssleay32.lib libeay32.lib
SSLOBJ=SRC/SSL.OBJ
!IFDEF OPENSSL_INC_DIR
OPENSSL_INC=/I "$(OPENSSL_INC_DIR)"
!ENDIF
!IFDEF OPENSSL_LIB_DIR
OPENSSL_LIB=/LIBPATH:$(OPENSSL_LIB_DIR)
!ENDIF
!ENDIF

INC_FILES = /I ./INCLUDE /J $(ZLIB_INC) $(OPENSSL_INC) /I $(PTHREAD_INC) /nologo /D _WIN32
CFLAGS=$(DBGCFLAG) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/ /c 
LFLAGS=kernel32.lib user32.lib ws2_32.lib oldnames.lib shell32.lib comctl32.lib gdi32.lib iphlpapi.lib $(ZLIBLIB) \
	$(ZLIB_LIB) $(OPENSSL_LIB) $(SSLLIBS) /LIBPATH:$(PTHREAD_LIB) pthreadVC2.lib dbghelp.lib \
	/nologo $(DBGLFLAG) /out:Colossus.exe /def:Colossus.def /implib:Colossus.lib /NODEFAULTLIB:libcmt
EXP_OBJ_FILES=SRC/GUI.OBJ SRC/HASH.OBJ SRC/IRCD.OBJ SRC/IRCSPRINTF.OBJ SRC/MAIN.OBJ \
	SRC/MATCH.OBJ SRC/MD5.OBJ SRC/MODULOS.OBJ SRC/PARSECONF.OBJ SRC/PROTOCOLOS.OBJ \
	SRC/SMTP.OBJ SRC/SOCKS.OBJ SRC/SOPORTE.OBJ SRC/SQL.OBJ $(ZLIBOBJ) $(SSLOBJ) 
MOD_DLL=SRC/MODULOS/MX.DLL SRC/MODULOS/CHANSERV.DLL SRC/MODULOS/NICKSERV.DLL SRC/MODULOS/MEMOSERV.DLL \
	SRC/MODULOS/OPERSERV.DLL SRC/MODULOS/IPSERV.DLL SRC/MODULOS/PROXYSERV.DLL SRC/MODULOS/SMSSERV.DLL SRC/MODULOS/TVSERV.DLL
#	SRC/MODULOS/STATSERV.DLL SRC/MODULOS/LINKSERV.DLL
OBJ_FILES=$(EXP_OBJ_FILES) SRC/WIN32/COLOSSUS.RES SRC/DEBUG.OBJ
PROT_DLL=SRC/PROTOCOLOS/UNREAL.DLL SRC/PROTOCOLOS/P10.DLL
SQL_DLL=SRC/SQL/MYSQL.DLL SRC/SQL/POSTGRESQL.DLL
EXT_DLL=SRC/EXTENSIONES/UDB/UDB.DLL SRC/EXTENSIONES/HISPANO/HISPANO.DLL

INCLUDES = ./include/ircd.h ./include/md5.h ./include/modulos.h ./include/parseconf.h ./include/protocolos.h \
	./include/ircsprintf.h ./include/sql.h ./include/struct.h ./include/ssl.h ./include/zip.h ./include/sistema.h 
MODCFLAGS=$(MODDBGCFLAG) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fesrc/modulos/ /Fosrc/modulos/ /D ENLACE_DINAMICO /D MODULE_COMPILE
MODLFLAGS=/link /def:src/modulos/modulos.def colossus.lib ws2_32.lib
PROTCFLAGS=$(MODDBGCFLAG) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fesrc/protocolos/ /Fosrc/protocolos/ /D ENLACE_DINAMICO /D MODULE_COMPILE
PROTLFLAGS=/link /def:src/protocolos/protocolos.def colossus.lib $(ZLIB_LIB) $(OPENSSL_LIB) $(SSLLIBS) ws2_32.lib
SQLCFLAGS=$(MODDBGCFLAG) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fesrc/sql/ /Fosrc/sql/ /D ENLACE_DINAMICO /D MODULE_COMPILE
SQLLFLAGS=/link /def:src/sql/sql.def colossus.lib

ALL: SETUP COLOSSUS.EXE MODULOS

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
	-@erase .\*.dll >NUL
	-@erase .\*.map >NUL
	-@erase colossus.lib >NUL
	-@erase modulos\*.dll >NUL
	-@erase modulos\*.pdb >NUL
	-@erase protocolos\*.dll >NUL
	-@erase protocolos\*.pdb >NUL
	-@erase sql\*.dll >NUL
	-@erase sql\*.pdb >NUL
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
	-@erase src\sql\*.exp >NUL
	-@erase src\sql\*.lib >NUL
	-@erase src\sql\*.pdb >NUL
	-@erase src\sql\*.ilk >NUL
	-@erase src\sql\*.dll >NUL
	-@erase src\sql\*.obj >NUL
	-@erase src\extensiones\udb\*.exp >NUL
	-@erase src\extensiones\udb\*.lib >NUL
	-@erase src\extensiones\udb\*.pdb >NUL
	-@erase src\extensiones\udb\*.ilk >NUL
	-@erase src\extensiones\udb\*.dll >NUL
	-@erase src\extensiones\udb\*.obj >NUL
	-@erase src\win32\colossus.res >NUL
	-@erase include\setup.h >NUL

SETUP: 
	-@copy src\win32\setup.h include\setup.h >NUL
	-@copy $(PTHREAD_LIB)\pthreadVC2.dll pthreadVC2.dll >NUL
        -@copy $(ZLIB_LIB_DIR)\zlibwapi.dll zlibwapi.dll >NUL

./COLOSSUS.EXE: $(OBJ_FILES)
        $(LINK) $(LFLAGS) $(OBJ_FILES) /MAP
        
!IFNDEF DEBUG
 @echo Version no Debug compilada ...
!ELSE
 @echo Version Debug compilada ... 
!ENDIF
        
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
        
src/ircsprintf.obj: src/ircsprintf.c $(INCLUDES)
        $(CC) $(CFLAGS) src/ircsprintf.c        
		
src/zlib.obj: src/zlib.c $(INCLUDES)
	$(CC) $(CFLAGS) src/zlib.c
	
src/ssl.obj: src/ssl.c $(INCLUDES)
	$(CC) $(CFLAGS) src/ssl.c
	
src/win32/colossus.res: src/win32/colossus.rc $(INCLUDES)
        $(RC) /l 0x409 /fosrc/win32/colossus.res /i ./include /i ./src \
              /d NDEBUG src/win32/colossus.rc	
              
src/gui.obj: src/win32/gui.c $(INCLUDES)
	$(CC) $(CFLAGS) src/win32/gui.c
	
src/sql.obj: src/sql.c $(INCLUDES)
	$(CC) $(CFLAGS) src/sql.c
	
	
# Modulos
	
MODULO: src/modulos/$(FUENTE).c
	$(CC) $(MODCFLAGS) src/modulos/$(FUENTE).c $(MODLFLAGS) $(PARAMS) /OUT:src/modulos/$(FUENTE).dll
	-@copy src\modulos\$(FUENTE).dll modulos\$(FUENTE).dll >NUL
	-@copy src\modulos\$(FUENTE).pdb modulos\$(FUENTE).pdb >NUL
	
FUNCION: src/funciones/$(FUENTE).c
	$(CC) $(MODCFLAGS) src/funciones/$(FUENTE).c /link /def:src/funciones/funciones.def colossus.lib ws2_32.lib $(PARAMS) src/modulos/$(MODULO).lib /OUT:src/funciones/$(FUENTE).dll
	-@copy src\funciones\$(FUENTE).dll funciones\$(FUENTE).dll >NUL
	
DEF: 
	$(CC) src/win32/def-clean.c
	dlltool --output-def colossus.def.in --export-all-symbols $(EXP_OBJ_FILES)
	def-clean colossus.def.in colossus.def
       
MODULOS: $(MOD_DLL) $(PROT_DLL) $(SQL_DLL) $(EXT_DLL)
    
src/modulos/mx.dll: src/modulos/mx.c $(INCLUDES)
	$(CC) $(MODDBGCFLAG) /Fesrc/modulos/ /Fosrc/modulos/ /nologo src/modulos/mx.c /link dnsapi.lib
	-@copy src\modulos\mx.dll modulos\mx.dll >NUL

src/modulos/chanserv.dll: src/modulos/chanserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/chanserv.c $(MODLFLAGS)
        -@copy src\modulos\chanserv.dll modulos\chanserv.dll >NUL
	-@copy src\modulos\chanserv.pdb modulos\chanserv.pdb >NUL
        
src/modulos/nickserv.dll: src/modulos/nickserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/nickserv.c $(MODLFLAGS) ws2_32.lib src/modulos/chanserv.lib  
        -@copy src\modulos\nickserv.dll modulos\nickserv.dll >NUL
	-@copy src\modulos\nickserv.pdb modulos\nickserv.pdb >NUL
        
src/modulos/operserv.dll: src/modulos/operserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/operserv.h ./include/modulos/nickserv.h
        $(CC) $(MODCFLAGS) src/modulos/operserv.c $(MODLFLAGS) src/modulos/memoserv.lib src/modulos/chanserv.lib
        -@copy src\modulos\operserv.dll modulos\operserv.dll >NUL
	-@copy src\modulos\operserv.pdb modulos\operserv.pdb >NUL
        
src/modulos/memoserv.dll: src/modulos/memoserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/memoserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	-@copy src\modulos\memoserv.dll modulos\memoserv.dll >NUL
	-@copy src\modulos\memoserv.pdb modulos\memoserv.pdb >NUL
	
src/modulos/ipserv.dll: src/modulos/ipserv.c $(INCLUDES) ./include/modulos/ipserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/ipserv.c $(MODLFLAGS)
	-@copy src\modulos\ipserv.dll modulos\ipserv.dll >NUL
	-@copy src\modulos\ipserv.pdb modulos\ipserv.pdb >NUL
		      
src/modulos/proxyserv.dll: src/modulos/proxyserv.c $(INCLUDES) ./include/modulos/proxyserv.h
	$(CC) $(MODCFLAGS) src/modulos/proxyserv.c $(MODLFLAGS)
	-@copy src\modulos\proxyserv.dll modulos\proxyserv.dll >NUL
	-@copy src\modulos\proxyserv.pdb modulos\proxyserv.pdb >NUL
	
src/modulos/tvserv.dll: src/modulos/tvserv.c $(INCLUDES) ./include/modulos/tvserv.h
	$(CC) $(MODCFLAGS) src/modulos/tvserv.c $(MODLFLAGS)
	-@copy src\modulos\tvserv.dll modulos\tvserv.dll >NUL
	-@copy src\modulos\tvserv.pdb modulos\tvserv.pdb >NUL
	
src/modulos/smsserv.dll: src/modulos/smsserv.c $(INCLUDES) ./include/modulos/smsserv.h
	$(CC) $(MODCFLAGS) src/modulos/smsserv.c $(MODLFLAGS)
	-@copy src\modulos\smsserv.dll modulos\smsserv.dll >NUL
	-@copy src\modulos\smsserv.pdb modulos\smsserv.pdb >NUL

src/modulos/statserv.dll: src/modulos/statserv.c $(INCLUDES) ./include/modulos/statserv.h
	$(CC) $(MODCFLAGS) src/modulos/statserv.c $(MODLFLAGS)
	
src/modulos/linkserv.dll: src/modulos/linkserv.c $(INCLUDES) ./include/modulos/chanserv.h ./include/modulos/linkserv.h
	$(CC) $(MODCFLAGS) src/modulos/linkserv.c $(MODLFLAGS) src/modulos/chanserv.lib
		
src/protocolos/unreal.dll: src/protocolos/unreal.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/unreal.c $(PROTLFLAGS)
	-@copy src\protocolos\unreal.dll protocolos\unreal.dll >NUL
	-@copy src\protocolos\unreal.pdb protocolos\unreal.pdb >NUL

src/protocolos/p10.dll: src/protocolos/p10.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/p10.c $(PROTLFLAGS)
	-@copy src\protocolos\p10.dll protocolos\p10.dll >NUL
	-@copy src\protocolos\p10.pdb protocolos\p10.pdb >NUL
	
src/protocolos/redhispana.dll: src/protocolos/redhispana.c $(INCLUDES) 
	$(CC) $(PROTCFLAGS) src/protocolos/redhispana.c $(PROTLFLAGS) ws2_32.lib
	-@copy src\protocolos\redhispana.dll protocolos\redhispana.dll >NUL
	-@copy src\protocolos\redhispana.pdb protocolos\redhispana.pdb >NUL

src/sql/mysql.dll: src/sql/mysql.c $(INCLUDES)
	$(CC) $(SQLCFLAGS) /I "C:\Archivos de Programa\MySQL\MySQL Server 5.0\include" src/sql/mysql.c \
	$(SQLLFLAGS) /LIBPATH:"C:\Archivos de Programa\MySQL\MySQL Server 5.0\lib\opt" mysqlclient.lib \
	user32.lib ws2_32.lib Advapi32.lib libcmt.lib  /NODEFAULTLIB:msvcrt /LIBPATH:$(PTHREAD_LIB) pthreadVC2.lib 
	-@copy src\sql\mysql.dll sql\mysql.dll >NUL
	-@copy src\sql\mysql.pdb sql\mysql.pdb >NUL

src/sql/postgresql.dll: src/sql/postgresql.c $(INCLUDES)
	$(CC) $(SQLCFLAGS) /I "C:\Archivos de programa\PostgreSQL\8.1\include" src/sql/postgresql.c \
	$(SQLLFLAGS) /LIBPATH:"C:\Archivos de programa\PostgreSQL\8.1\lib\ms" libpq.lib \
	user32.lib /LIBPATH:$(PTHREAD_LIB) pthreadVC2.lib 
	-@copy src\sql\postgresql.dll sql\postgresql.dll >NUL
	-@copy src\sql\postgresql.pdb sql\postgresql.pdb >NUL

UDB_OBJ=SRC/EXTENSIONES/UDB/BDD.OBJ \
	SRC/EXTENSIONES/UDB/UDB.OBJ \
	SRC/EXTENSIONES/UDB/CHANSERV.OBJ \
	SRC/EXTENSIONES/UDB/NICKSERV.OBJ \
	SRC/EXTENSIONES/UDB/OPERSERV.OBJ \
	SRC/EXTENSIONES/UDB/IPSERV.OBJ
src/extensiones/udb/udb.obj: src/extensiones/udb/udb.c
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/udb.c
src/extensiones/udb/bdd.obj: src/extensiones/udb/bdd.c $(INCLUDES)
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/bdd.c
src/extensiones/udb/chanserv.obj: src/extensiones/udb/chanserv.c $(INCLUDES) ./include/modulos/chanserv.h
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/chanserv.c
src/extensiones/udb/ipserv.obj: src/extensiones/udb/ipserv.c $(INCLUDES) ./include/modulos/ipserv.h
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/ipserv.c
src/extensiones/udb/nickserv.obj: src/extensiones/udb/nickserv.c $(INCLUDES) ./include/modulos/nickserv.h
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/nickserv.c
src/extensiones/udb/operserv.obj: src/extensiones/udb/operserv.c $(INCLUDES) ./include/modulos/operserv.h
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/udb/operserv.c
src/extensiones/udb/udb.dll: $(UDB_OBJ)
	$(LINK) colossus.lib /NODEFAULTLIB:libcmt /dll /debug $(UDB_OBJ) \
	/out:src/extensiones/udb/udb.dll /def:src/modulos/modulos.def /implib:src/extensiones/udb/udb.lib \
	src/protocolos/unreal.lib \
	src/modulos/chanserv.lib \
	src/modulos/nickserv.lib \
	src/modulos/operserv.lib \
	src/modulos/ipserv.lib
	-@copy src\extensiones\udb\udb.dll protocolos\extensiones\udb.dll >NUL
	-@copy src\extensiones\udb\udb.pdb protocolos\extensiones\udb.pdb >NUL

HIS_OBJ=SRC/EXTENSIONES/HISPANO/HISPANO.OBJ
src/extensiones/hispano/hispano.obj: src/extensiones/hispano/hispano.c
	$(CC) $(DBGCFLAG) /Fesrc/extensiones/hispano/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/hispano/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/hispano/hispano.c
src/extensiones/hispano/hispano.dll: $(HIS_OBJ)
	$(LINK) colossus.lib /NODEFAULTLIB:libcmt /dll /debug $(HIS_OBJ) \
	/out:src/extensiones/hispano/hispano.dll /def:src/modulos/modulos.def /implib:src/extensiones/hispano/hispano.lib ws2_32.lib \
	src/protocolos/p10.lib 
	-@copy src\extensiones\hispano\hispano.dll protocolos\extensiones\hispano.dll >NUL
	-@copy src\extensiones\hispano\hispano.pdb protocolos\extensiones\hispano.pdb >NUL
