## $Id: make,v 1.2 2004-09-11 15:49:00 Trocotronic Exp $

CC=cl
LINK=link
RC=rc
DEBUG=1

#### ZIPLINKS SUPPORT ####
#To enable ziplinks support you must have zlib installed on your system
#you can get a pre-built zlib library from http://www.winimage.com/zLibDll/
#
#
#To enable ziplinks uncomment the next line:
#USE_ZIPLINKS=1
#
#If your zlib library and include files are not in your compiler's
#default locations, specify the locations here:
#ZLIB_INC_DIR="c:\dev\zlib"
#ZLIB_LIB_DIR="c:\dev\zlib\dll32"
#
#
###### END ZIPLINKS ######

#### ZIPLINKS SUPPORT ####
#Soporte UDB
UDB=1
#
###### FIN UDB ######

!IFDEF DEBUG
DBGCFLAG=/MD /Zi
DBGLFLAG=/debug
MODDBGCFLAG=/LDd /MD /Zi
!ELSE
DBGCFLAG=/MD /O2 /G5
MODDBGCFLAG=/LD /MD
!ENDIF 

!IFDEF UDB
UDBFLAGS=/D UDB
!ENDIF 

!IFDEF USE_ZIPLINKS
ZIPCFLAGS=/D ZIP_LINKS
ZIPOBJ=SRC/ZIP.OBJ
ZIPLIB=zlibwapi.lib
!IFDEF ZLIB_INC_DIR
ZLIB_INC=/I "$(ZLIB_INC_DIR)"
!ENDIF
!IFDEF ZLIB_LIB_DIR
ZLIB_LIB=/LIBPATH:"$(ZLIB_LIB_DIR)"
!ENDIF
!ENDIF

CFLAGS=$(DBGCFLAG) /I ./INCLUDE /J $(ZLIB_INC) /Fosrc/ /nologo $(ZIPCFLAGS) $(UDBFLAGS) /D _WIN32 /c 
LFLAGS=kernel32.lib user32.lib ws2_32.lib oldnames.lib shell32.lib comctl32.lib ./src/libmysql.lib $(ZLIB_LIB) $(ZIPLIB) Dbghelp.lib \
	/nologo $(DBGLFLAG) /out:Colossus.exe /def:colossus.def /implib:colossus.lib
OBJ_FILES=SRC/BDD.OBJ SRC/DEBUG.OBJ SRC/FLAGS.OBJ SRC/HASH.OBJ SRC/IRCD.OBJ SRC/MAIN.OBJ \
	SRC/MATCH.OBJ SRC/MD5.OBJ SRC/MODULOS.OBJ SRC/MYSQL.OBJ SRC/PARSECONF.OBJ SRC/SMTP.OBJ \
	SRC/SPRINTF_IRC.OBJ $(ZIPOBJ) SRC/COLOSSUS.RES SRC/GUI.OBJ
BOT_DLL=SRC/MODULOS/CHANSERV.DLL SRC/MODULOS/NICKSERV.DLL SRC/MODULOS/MEMOSERV.DLL SRC/MODULOS/OPERSERV.DLL \
	SRC/MODULOS/IPSERV.DLL SRC/MODULOS/PROXYSERV.DLL SRC/MODULOS/STATSERV.DLL SRC/MODULOS/LINKSERV.DLL \
	SRC/MODULOS/MX.DLL
INCLUDES = ./include/bdd.h ./include/comandos.h \
	./include/flags.h ./include/ircd.h ./include/md5.h \
	./include/modulos.h ./include/mysql.h ./include/parseconf.h \
	./include/sprintf_irc.h ./include/struct.h
MODCFLAGS=$(MODDBGCFLAG) /Fesrc/modulos/ /Fosrc/modulos/ /nologo /I ./INCLUDE /J /D MODULE_COMPILE $(UDBFLAGS)
MODLFLAGS=/link /def:src/modulos/modulos.def colossus.lib ./src/libmysql.lib

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
	-@erase src\colossus.res >NUL

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
		
src/zip.obj: src/zip.c $(INCLUDES)
	$(CC) $(CFLAGS) src/zip.c
	
src/colossus.res: src/colossus.rc $(INCLUDES)
        $(RC) /l 0x409 /fosrc/colossus.res /i ./include /i ./src \
              /d NDEBUG src/colossus.rc	
              
src/gui.obj: src/gui.c $(INCLUDES)
	$(CC) $(CFLAGS) src/gui.c
	
	
# Modulos
	
CUSTOMMODULE: src/modulos/$(MODULEFILE).c
	$(CC) $(MODCFLAGS) src/modulos/$(MODULEFILE).c $(MODLFLAGS) $(PARAMS) /OUT:src/modulos/$(MODULEFILE).dlk
       
MODULES: $(BOT_DLL)
        
src/modulos/chanserv.dll: src/modulos/chanserv.c $(INCLUDES)
        $(CC) $(MODCFLAGS) src/modulos/chanserv.c $(MODLFLAGS)
        
src/modulos/nickserv.dll: src/modulos/nickserv.c $(INCLUDES)
        $(CC) $(MODCFLAGS) src/modulos/nickserv.c $(MODLFLAGS) ws2_32.lib src/modulos/chanserv.lib  
        
src/modulos/operserv.dll: src/modulos/operserv.c $(INCLUDES)
        $(CC) $(MODCFLAGS) src/modulos/operserv.c $(MODLFLAGS) src/modulos/memoserv.lib src/modulos/chanserv.lib
        
src/modulos/memoserv.dll: src/modulos/memoserv.c $(INCLUDES)
	$(CC) $(MODCFLAGS) src/modulos/memoserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	
src/modulos/ipserv.dll: src/modulos/ipserv.c $(INCLUDES)
	$(CC) $(MODCFLAGS) src/modulos/ipserv.c $(MODLFLAGS)	
		      
src/modulos/proxyserv.dll: src/modulos/proxyserv.c $(INCLUDES)
	$(CC) $(MODCFLAGS) src/modulos/proxyserv.c $(MODLFLAGS)

src/modulos/statserv.dll: src/modulos/statserv.c $(INCLUDES)
	$(CC) $(MODCFLAGS) src/modulos/statserv.c $(MODLFLAGS)
	
src/modulos/mx.dll: src/modulos/mx.c $(INCLUDES)
	$(CC) $(MODDBGCFLAG) /Fesrc/modulos/ /Fosrc/modulos/ /nologo src/modulos/mx.c /link dnsapi.lib
	-@copy src\modulos\mx.dll mx.dll
	
src/modulos/linkserv.dll: src/modulos/linkserv.c $(INCLUDES)
	$(CC) $(MODCFLAGS) src/modulos/linkserv.c $(MODLFLAGS) src/modulos/chanserv.lib
	