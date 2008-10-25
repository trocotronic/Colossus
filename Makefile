## $Id: Makefile,v 1.37 2008/06/01 22:25:37 Trocotronic Exp $

CC=cl
LINK=link
RC=rc
MT=mt
DEBUG=1

### DEBUG POR CORE ###
#Esto debe comentarse cuando es una release
#NOCORE=1
#endif

#### SOPORTE ZLIB ####
ZLIB=1
ZLIB_INC_DIR="c:\dev\zlib"
ZLIB_LIB_DIR="c:\dev\zlib\dll32"
ZLIB_HEAD="./include/zip.h"
#
###### FIN ZLIB ######


#### SOPORTE SSL ####
SSL=1
#
OPENSSL_INC_DIR="c:\dev\openssl\include"
OPENSSL_LIB_DIR="C:\dev\openssl\lib"
#
###### FIN SSL ######

#### SOPORTE PTHREAD ####
PTHREAD_INC="C:\dev\pthreads"
PTHREAD_LIB="C:\dev\pthreads"
###### FIN PTHREAD ######

#### SOPORTE ICONV ####
ICONV_INC="C:\dev\libiconv\include"
ICONV_LIB="C:\dev\libiconv\lib"
###### FIN ICONV ######

#### SOPORTE MYSQL ####
MYSQL_INC="C:\dev\mysql-5.1.24-rc-win32\include"
MYSQL_LIB="C:\dev\mysql-5.1.24-rc-win32\Embedded\DLL\release"
###### FIN MYSQL ######

EXPAT_LIB="C:\dev\expat\lib\Release_static" libexpatMD.lib

!IFDEF DEBUG
DBGCFLAG=/MD /Zi /W1
DBGLFLAG=/debug
MODDBGCFLAG=/LD $(DBGCFLAG)
!ELSE
DBGCFLAG=/MD /O2 /W1
MODDBGCFLAG=/LD /MD
!ENDIF
!IFDEF NOCORE
DBGCFLAG=$(DBGCFLAG) /D NOCORE
!ENDIF

!IFDEF ZLIB
ZLIBCFLAGS=/D USA_ZLIB
ZLIBOBJ=SRC/ZLIB.OBJ
ZLIBLIB=zlibwapi.lib
!IFDEF ZLIB_INC_DIR
ZLIB_INC=/I $(ZLIB_INC_DIR)
!ENDIF
!IFDEF ZLIB_LIB_DIR
ZLIB_LIB=/LIBPATH:$(ZLIB_LIB_DIR) $(ZLIBLIB)
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

INC_FILES = /I ./INCLUDE $(ZLIB_INC) $(OPENSSL_INC) /I $(PTHREAD_INC)
CFLAGS=/J /D _WIN32 /D _CRT_NONSTDC_NO_DEPRECATE /D _CRT_SECURE_NO_DEPRECATE /D _USE_32BIT_TIME_T /nologo
EXECFLAGS=$(DBGCFLAG) $(CFLAGS) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/ /c
LFLAGS=advapi32.lib kernel32.lib user32.lib ws2_32.lib oldnames.lib shell32.lib comctl32.lib gdi32.lib iphlpapi.lib \
	$(ZLIB_LIB) $(OPENSSL_LIB) $(SSLLIBS) /LIBPATH:$(PTHREAD_LIB) /LIBPATH:$(EXPAT_LIB) pthreadVC2.lib dbghelp.lib \
	/nologo $(DBGLFLAG) /out:Colossus.exe /def:Colossus.def /implib:Colossus.lib \
	/LIBPATH:$(ICONV_LIB) iconv.lib /LIBPATH:$(MYSQL_LIB) libmysqld.lib \
	/NODEFAULTLIB:libcmt
EXP_OBJ_FILES=SRC/CORE.OBJ SRC/CRIPTO.OBJ SRC/EVENTOS.OBJ SRC/GUI.OBJ SRC/HASH.OBJ SRC/HTTPD.OBJ SRC/IPLOC.OBJ SRC/IRCD.OBJ \
	SRC/IRCSPRINTF.OBJ SRC/MAIN.OBJ SRC/MATCH.OBJ SRC/MD5.OBJ SRC/MISC.OBJ SRC/MODULOS.OBJ SRC/MSN.OBJ SRC/PARSECONF.OBJ \
	SRC/PROTOCOLOS.OBJ SRC/SMTP.OBJ SRC/SOCKS.OBJ SRC/SOCKSINT.OBJ SRC/SOPORTE.OBJ SRC/SQL.OBJ SRC/VERSION.OBJ $(ZLIBOBJ) $(SSLOBJ)
MOD_DLL=SRC/MODULOS/CHANSERV.DLL SRC/MODULOS/NICKSERV.DLL SRC/MODULOS/MEMOSERV.DLL \
	SRC/MODULOS/OPERSERV.DLL SRC/MODULOS/IPSERV.DLL SRC/MODULOS/PROXYSERV.DLL SRC/MODULOS/SMSSERV.DLL SRC/MODULOS/TVSERV.DLL \
	SRC/MODULOS/NEWSSERV.DLL SRC/MODULOS/HELPSERV.DLL SRC/MODULOS/LOGSERV.DLL SRC/MODULOS/NOTESERV.DLL SRC/MODULOS/GAMESERV.DLL \
	SRC/MODULOS/STATSERV.DLL
#	SRC/MODULOS/LINKSERV.DLL
OBJ_FILES=$(EXP_OBJ_FILES) SRC/WIN32/COLOSSUS.RES SRC/DEBUG.OBJ
PROT_DLL=SRC/PROTOCOLOS/UNREAL.DLL SRC/PROTOCOLOS/P10.DLL
EXT_DLL=SRC/EXTENSIONES/UDB/UDB.DLL SRC/EXTENSIONES/HISPANO/HISPANO.DLL

MODCFLAGS=$(MODDBGCFLAG) $(CFLAGS) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fesrc/modulos/ /Fosrc/modulos/ /D ENLACE_DINAMICO /D MODULE_COMPILE
MODLFLAGS=/link /def:src/modulos/modulos.def colossus.lib ws2_32.lib $(ZLIB_LIB) $(OPENSSL_LIB) $(SSLLIBS)
PROTCFLAGS=$(MODDBGCFLAG) $(CFLAGS) $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fesrc/protocolos/ /Fosrc/protocolos/ /D ENLACE_DINAMICO /D MODULE_COMPILE
PROTLFLAGS=/link /def:src/protocolos/protocolos.def colossus.lib $(ZLIB_LIB) $(OPENSSL_LIB) $(SSLLIBS) ws2_32.lib

ALL: SETUP CONFVER Colossus.exe MODULOS

AUTOCONF: src/win32/autoconf.c
	$(CC) src/win32/autoconf.c
	-@autoconf.exe

CONFVER: src/utils/confver.c
	$(CC) /D _WIN32 src/utils/confver.c
	-@confver.exe
	-@erase confver.exe

ACT111:
	$(CC) src/utils/actualiza_111.c
	-@copy actualiza_111.exe utils/actualiza_111.exe
	-@erase actualiza_111.exe

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
	-@erase src\modulos\*.exp >NUL
	-@erase src\modulos\*.lib >NUL
	-@erase src\modulos\*.pdb >NUL
	-@erase src\modulos\*.ilk >NUL
	-@erase src\modulos\*.dll >NUL
	-@erase src\modulos\*.obj >NUL
	-@erase src\modulos\*.manifest >NUL
	-@erase src\protocolos\*.exp >NUL
	-@erase src\protocolos\*.lib >NUL
	-@erase src\protocolos\*.pdb >NUL
	-@erase src\protocolos\*.ilk >NUL
	-@erase src\protocolos\*.dll >NUL
	-@erase src\protocolos\*.obj >NUL
	-@erase src\protocolos\*.manifest >NUL
	-@erase src\extensiones\udb\*.exp >NUL
	-@erase src\extensiones\udb\*.lib >NUL
	-@erase src\extensiones\udb\*.pdb >NUL
	-@erase src\extensiones\udb\*.ilk >NUL
	-@erase src\extensiones\udb\*.dll >NUL
	-@erase src\extensiones\udb\*.obj >NUL
	-@erase src\extensiones\udb\*.manifest >NUL
	-@erase src\win32\colossus.res >NUL
	-@erase include\setup.h >NUL

SETUP:
	-@copy src\win32\setup.h include\setup.h >NUL
	-@copy $(PTHREAD_LIB)\pthreadVC2.dll pthreadVC2.dll >NUL
      -@copy $(ZLIB_LIB_DIR)\zlibwapi.dll zlibwapi.dll >NUL
      -@copy $(ICONV_LIB)\iconv.dll iconv.dll >NUL
      -@copy $(MYSQL_LIB)\libmysqld.dll libmysqld.dll >NUL

Colossus.exe: $(OBJ_FILES)
	$(LINK) $(LFLAGS) $(OBJ_FILES) /MAP
#        $(MT) -manifest $@.manifest -outputresource:$@;1
#        -@erase $@.manifest >NUL

!IFNDEF DEBUG
 @echo Version no Debug compilada ...
!ELSE
 @echo Version Debug compilada ...
!ENDIF

src/debug.obj: src/win32/debug.c ./include/struct.h ./include/ircd.h
        $(CC) $(EXECFLAGS) src/win32/debug.c

src/hash.obj: src/hash.c ./include/struct.h ./include/ircd.h
        $(CC) $(EXECFLAGS) src/hash.c

src/httpd.obj: src/httpd.c ./include/struct.h ./include/httpd.h
        $(CC) $(EXECFLAGS) src/httpd.c

src/iploc.obj: src/ircd.c ./include/struct.h ./include/iploc.h
        $(CC) /I "C:\dev\expat\lib" $(EXECFLAGS) src/iploc.c

src/ircd.obj: src/ircd.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/iploc.h
        $(CC) $(EXECFLAGS) src/ircd.c

src/main.obj: src/main.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/socksint.h $(ZIP_HEAD)
        $(CC) $(EXECFLAGS) src/main.c

src/match.obj: src/match.c
        $(CC) $(EXECFLAGS) src/match.c

src/md5.obj: src/md5.c ./include/md5.h ./include/ircsprintf.h
        $(CC) $(EXECFLAGS) src/md5.c

src/modulos.obj: src/modulos.c ./include/struct.h ./include/ircd.h ./include/modulos.h
        $(CC) $(EXECFLAGS) src/modulos.c

src/msn.obj: src/msn.c ./include/struct.h ./include/ircd.h ./include/msn.h
        $(CC) $(EXECFLAGS) src/msn.c

src/parseconf.obj: src/parseconf.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/msn.h
        $(CC) $(EXECFLAGS) src/parseconf.c

src/protocolos.obj: src/protocolos.c ./include/struct.h ./include/ircd.h ./include/protocolos.h
        $(CC) $(EXECFLAGS) src/protocolos.c

src/smtp.obj: src/smtp.c ./include/struct.h ./include/httpd.h
        $(CC) $(EXECFLAGS) src/smtp.c

src/socks.obj: src/socks.c ./include/struct.h ./include/ircd.h $(ZIP_HEAD)
	$(CC) $(EXECFLAGS) src/socks.c

src/soporte.obj: src/soporte.c ./include/struct.h ./include/ircd.h ./include/modulos.h
	$(CC) $(EXECFLAGS) /I $(ICONV_INC) src/soporte.c

src/ircsprintf.obj: src/ircsprintf.c ./include/ircsprintf.h
	$(CC) $(EXECFLAGS) src/ircsprintf.c

src/zlib.obj: src/zlib.c ./include/struct.h $(ZIP_HEAD)
	$(CC) $(EXECFLAGS) src/zlib.c

src/ssl.obj: src/ssl.c ./include/struct.h ./include/ircd.h
	$(CC) $(EXECFLAGS) src/ssl.c

src/version.obj: src/version.c cambios
	$(CC) $(EXECFLAGS) src/version.c

src/win32/colossus.res: src/win32/colossus.rc
        $(RC) /l 0x409 /fosrc/win32/colossus.res /i ./include /i ./src \
              /d NDEBUG src/win32/colossus.rc

src/gui.obj: src/win32/gui.c ./include/struct.h ./include/ircd.h ./include/struct.h ./include/modulos.h ./src/win32/resource.h $(ZIP_HEAD)
	$(CC) $(EXECFLAGS) src/win32/gui.c

src/sql.obj: src/sql.c ./include/struct.h ./include/ircd.h
	$(CC) $(EXECFLAGS) /I $(MYSQL_INC) src/sql.c

src/eventos.obj: src/eventos.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h
	$(CC) $(EXECFLAGS) src/eventos.c

src/cripto.obj: src/cripto.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/md5.h
	$(CC) $(EXECFLAGS) src/cripto.c

src/socksint.obj: src/socksint.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/socksint.h
	$(CC) $(EXECFLAGS) src/socksint.c

src/core.obj: src/core.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/socksint.h
	$(CC) $(EXECFLAGS) src/core.c

src/misc.obj: src/misc.c ./include/struct.h
	$(CC) $(EXECFLAGS) src/misc.c

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

MODULOS: $(MOD_DLL) $(PROT_DLL) $(EXT_DLL)

src/modulos/chanserv.dll: src/modulos/chanserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/chanserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/chanserv.c $(MODLFLAGS)
	-@copy src\modulos\chanserv.dll modulos\chanserv.dll >NUL
	-@copy src\modulos\chanserv.pdb modulos\chanserv.pdb >NUL

src/modulos/nickserv.dll: src/modulos/nickserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/md5.h ./include/modulos/chanserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/nickserv.c $(MODLFLAGS) ws2_32.lib src/modulos/chanserv.lib
	-@copy src\modulos\nickserv.dll modulos\nickserv.dll >NUL
	-@copy src\modulos\nickserv.pdb modulos\nickserv.pdb >NUL

src/modulos/operserv.dll: src/modulos/operserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/operserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/operserv.c $(MODLFLAGS) src/modulos/memoserv.lib src/modulos/chanserv.lib
	-@copy src\modulos\operserv.dll modulos\operserv.dll >NUL
	-@copy src\modulos\operserv.pdb modulos\operserv.pdb >NUL

src/modulos/memoserv.dll: src/modulos/memoserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/chanserv.h ./include/modulos/memoserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/memoserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	-@copy src\modulos\memoserv.dll modulos\memoserv.dll >NUL
	-@copy src\modulos\memoserv.pdb modulos\memoserv.pdb >NUL

src/modulos/ipserv.dll: src/modulos/ipserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/ipserv.h ./include/modulos/nickserv.h
	$(CC) $(MODCFLAGS) src/modulos/ipserv.c $(MODLFLAGS)
	-@copy src\modulos\ipserv.dll modulos\ipserv.dll >NUL
	-@copy src\modulos\ipserv.pdb modulos\ipserv.pdb >NUL

src/modulos/proxyserv.dll: src/modulos/proxyserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/proxyserv.h
	$(CC) $(MODCFLAGS) src/modulos/proxyserv.c $(MODLFLAGS)
	-@copy src\modulos\proxyserv.dll modulos\proxyserv.dll >NUL
	-@copy src\modulos\proxyserv.pdb modulos\proxyserv.pdb >NUL

src/modulos/tvserv.dll: src/modulos/tvserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/modulos/tvserv.h
	$(CC) $(MODCFLAGS) src/modulos/tvserv.c $(MODLFLAGS)
	-@copy src\modulos\tvserv.dll modulos\tvserv.dll >NUL
	-@copy src\modulos\tvserv.pdb modulos\tvserv.pdb >NUL

src/modulos/smsserv.dll: src/modulos/smsserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/modulos/smsserv.h
	$(CC) $(MODCFLAGS) src/modulos/smsserv.c $(MODLFLAGS)
	-@copy src\modulos\smsserv.dll modulos\smsserv.dll >NUL
	-@copy src\modulos\smsserv.pdb modulos\smsserv.pdb >NUL

src/modulos/newsserv.dll: src/modulos/newsserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/modulos/chanserv.h ./include/modulos/nickserv.h ./include/modulos/newsserv.h
	$(CC) /I "C:\dev\expat\lib" $(MODCFLAGS) src/modulos/newsserv.c $(MODLFLAGS) src/modulos/chanserv.lib /LIBPATH:$(EXPAT_LIB)
	-@copy src\modulos\newsserv.dll modulos\newsserv.dll >NUL
	-@copy src\modulos\newsserv.pdb modulos\newsserv.pdb >NUL

src/modulos/helpserv.dll: src/modulos/helpserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/httpd.h ./include/modulos/helpserv.h ./include/modulos/chanserv.h
	$(CC) $(MODCFLAGS) src/modulos/helpserv.c $(MODLFLAGS)
	-@copy src\modulos\helpserv.dll modulos\helpserv.dll >NUL
	-@copy src\modulos\helpserv.pdb modulos\helpserv.pdb >NUL

src/modulos/logserv.dll: src/modulos/logserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/md5.h ./include/modulos/logserv.h ./include/modulos/nickserv.h ./include/modulos/chanserv.h
	$(CC) $(MODCFLAGS) src/modulos/logserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	-@copy src\modulos\logserv.dll modulos\logserv.dll >NUL
	-@copy src\modulos\logserv.pdb modulos\logserv.pdb >NUL

src/modulos/noteserv.dll: src/modulos/noteserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/noteserv.h
	$(CC) $(MODCFLAGS) src/modulos/noteserv.c $(MODLFLAGS)
	-@copy src\modulos\noteserv.dll modulos\noteserv.dll >NUL
	-@copy src\modulos\noteserv.pdb modulos\noteserv.pdb >NUL

GAMESERV_FILES=src/modulos/gameserv.c src/modulos/gameserv/kyrhos.c src/modulos/gameserv/bidle.c
JUEGOS=/D BIDLE
src/modulos/gameserv.dll: ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/modulos/gameserv.h ./include/modulos/gameserv/kyrhos.h ./include/modulos/gameserv/bidle.h $(GAMESERV_FILES)
	$(CC) $(MODDBGCFLAG) $(CFLAGS) /Fesrc/modulos/gameserv/ $(INC_FILES) /Fosrc/modulos/gameserv/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE $(JUEGOS) $(GAMESERV_FILES) $(MODLFLAGS) /out:src/modulos/gameserv.dll
	-@copy src\modulos\gameserv.dll modulos\gameserv.dll >NUL
	-@copy src\modulos\gameserv.pdb modulos\gameserv.pdb >NUL

src/modulos/statserv.dll: src/modulos/statserv.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/httpd.h ./include/modulos/statserv.h
	$(CC) $(MODCFLAGS) src/modulos/statserv.c $(MODLFLAGS)
	-@copy src\modulos\statserv.dll modulos\statserv.dll >NUL
	-@copy src\modulos\statserv.pdb modulos\statserv.pdb >NUL

src/protocolos/unreal.dll: src/protocolos/unreal.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h ./include/md5.h $(ZIP_HEAD)
	$(CC) $(PROTCFLAGS) src/protocolos/unreal.c $(PROTLFLAGS)
	-@copy src\protocolos\unreal.dll protocolos\unreal.dll >NUL
	-@copy src\protocolos\unreal.pdb protocolos\unreal.pdb >NUL

src/protocolos/p10.dll: src/protocolos/p10.c ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h
	$(CC) $(PROTCFLAGS) src/protocolos/p10.c $(PROTLFLAGS)
	-@copy src\protocolos\p10.dll protocolos\p10.dll >NUL
	-@copy src\protocolos\p10.pdb protocolos\p10.pdb >NUL

UDB_FILES=src/extensiones/udb/udb.c \
	src/extensiones/udb/bdd.c \
	src/extensiones/udb/chanserv.c \
	src/extensiones/udb/ipserv.c \
	src/extensiones/udb/nickserv.c \
	src/extensiones/udb/operserv.c
UDB_LIBS=src/protocolos/unreal.lib src/modulos/nickserv.lib src/modulos/ipserv.lib src/modulos/chanserv.lib src/modulos/operserv.lib
src/extensiones/udb/udb.dll: $(UDB_FILES) ./include/struct.h ./include/ircd.h ./include/modulos.h ./include/protocolos.h src/extensiones/udb/bdd.h ./include/modulos/nickserv.h ./include/modulos/chanserv.h ./include/modulos/operserv.h ./include/modulos/ipserv.h $(ZIP_HEAD)
	$(CC) $(CFLAGS) $(MODDBGCFLAG) /Fesrc/extensiones/udb/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/udb/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE $(UDB_FILES) $(MODLFLAGS) $(UDB_LIBS) /out:src/extensiones/udb/udb.dll
	-@copy src\extensiones\udb\udb.dll protocolos\extensiones\udb.dll >NUL
	-@copy src\extensiones\udb\udb.pdb protocolos\extensiones\udb.pdb >NUL

HIS_OBJ=SRC/EXTENSIONES/HISPANO/HISPANO.OBJ
src/extensiones/hispano/hispano.obj: src/extensiones/hispano/hispano.c
	$(CC) $(DBGCFLAG) $(CFLAGS) /Fesrc/extensiones/hispano/ $(INC_FILES) $(ZLIBCFLAGS) $(SSLCFLAGS) /Fosrc/extensiones/hispano/ \
	/D ENLACE_DINAMICO /D MODULE_COMPILE /c src/extensiones/hispano/hispano.c
src/extensiones/hispano/hispano.dll: $(HIS_OBJ)
	$(LINK) colossus.lib /NODEFAULTLIB:libcmt /dll /debug $(HIS_OBJ) \
	/out:src/extensiones/hispano/hispano.dll /def:src/modulos/modulos.def /implib:src/extensiones/hispano/hispano.lib ws2_32.lib \
	src/protocolos/p10.lib
	-@copy src\extensiones\hispano\hispano.dll protocolos\extensiones\hispano.dll >NUL
	-@copy src\extensiones\hispano\hispano.pdb protocolos\extensiones\hispano.pdb >NUL
