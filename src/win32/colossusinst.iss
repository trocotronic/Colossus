; $Id: colossusinst.iss,v 1.11 2005-11-01 14:12:14 Trocotronic Exp $

; Instalador de Colossus

; Definiciones de soporte
#define USA_SSL
; Uncomment the above line to package an SSL build
#define USA_ZLIB

[Setup]
AppName=Colossus
AppVerName=Colossus 1.2b
AppPublisher=Trocotronic
AppPublisherURL=http://www.rallados.net
AppSupportURL=http://www.rallados.net
AppUpdatesURL=http://www.rallados.net
AppMutex=ColossusMutex,Global\ColossusMutex
DefaultDirName={pf}\Colossus
DefaultGroupName=Colossus
AllowNoIcons=yes
Compression=lzma
MinVersion=4.0.1111,4.0.1381
OutputDir=../../

[Tasks]
Name: "desktopicon"; Description: "Crear un icono en el &escriptorio"; GroupDescription: "Iconos adicionales:"
Name: "quicklaunchicon"; Description: "Crear un icono en el &menú rápido"; GroupDescription: "Iconos adicionales:"; Flags: unchecked
#ifdef USA_SSL
Name: "makecert"; Description: "&Crear certificado"; GroupDescription: "Opciones SSL:";
Name: "enccert"; Description: "&Encriptar certificado"; GroupDescription: "Opciones SSL:"; Flags: unchecked;
#endif

[Files]
Source: "..\..\Colossus.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Colossus.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\cambios"; DestDir: "{app}"; DestName: "cambios.txt"; Flags: ignoreversion
Source: "..\..\proximamente"; DestDir: "{app}"; DestName: "proximamente.txt"; Flags: ignoreversion
Source: "..\..\pthreadVC2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\colossusdoc.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\modulos\chanserv.dll"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\nickserv.dll"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\operserv.dll"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\memoserv.dll"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\ipserv.dll"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\chanserv.pdb"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\nickserv.pdb"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\operserv.pdb"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\memoserv.pdb"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\modulos\ipserv.pdb"; DestDir: "{app}\modulos"; Flags: ignoreversion
Source: "..\..\protocolos\*.dll"; DestDir: "{app}\protocolos"; Flags: ignoreversion
Source: "..\..\protocolos\*.pdb"; DestDir: "{app}\protocolos"; Flags: ignoreversion
Source: "..\..\sql\*.dll"; DestDir: "{app}\sql"; Flags: ignoreversion
Source: "..\..\sql\*.pdb"; DestDir: "{app}\sql"; Flags: ignoreversion
#ifdef USA_SSL
Source: "c:\dev\openssl\out32dll\openssl.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "c:\dev\openssl\out32dll\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "c:\dev\openssl\out32dll\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: ".\makecert.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: ".\encpem.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\ssl.cnf"; DestDir: "{app}"; Flags: ignoreversion
#endif
#ifdef USA_ZLIB
Source: "..\..\zlibwapi.dll"; DestDir: "{app}"; Flags: ignoreversion
#endif
Source: isxdl.dll; DestDir: {tmp}; Flags: dontcopy

[Dirs]
Name: "{app}\tmp"

[UninstallDelete]
Type: files; Name: "{app}\dbghelp.dll"

[Code]
function isxdl_Download(hWnd: Integer; URL, Filename: PChar): Integer;
external 'isxdl_Download@files:isxdl.dll stdcall';
function isxdl_SetOption(Option, Value: PChar): Integer;
external 'isxdl_SetOption@files:isxdl.dll stdcall';
const dbgurl = 'http://www.rallados.net/descargas/dbghelp.dll';
const crturl = 'http://www.rallados.net/descargas/msvcr70.dll';
const msvurl = 'http://www.rallados.net/descargas/MSVCRTD.DLL';
var didDbgDl,didCrtDl,didMsCrtDl: Boolean;

function NextButtonClick(CurPage: Integer): Boolean;
var
dbghelp,tmp,output: String;
msvcrt: String;
msvcrtd: String;
m: String;
hWnd,answer: Integer;
begin

    if ((CurPage = wpReady)) then begin
      dbghelp := ExpandConstant('{sys}\dbghelp.dll');
      output := ExpandConstant('{app}\dbghelp.dll');
      msvcrt := ExpandConstant('{sys}\msvcr70.Dll');
      msvcrtd := ExpandConstant('{sys}\MSVCRTD.dll');
      GetVersionNumbersString(dbghelp,m);
    if (NOT FileExists(msvcrt)) then begin
      answer := MsgBox('Colossus necesita la librería MS C Runtime 7.0. ¿Quiere instalarla?', mbConfirmation, MB_YESNO);
      if answer = IDYES then begin
        tmp := ExpandConstant('{tmp}\msvcr70.Dll');
        isxdl_SetOption('title', 'Descargando msvcr70.dll');
        hWnd := StrToInt(ExpandConstant('{wizardhwnd}'));
        if isxdl_Download(hWnd, crturl, tmp) = 0 then begin
          MsgBox('La descarga e instalación  de msvcr70.dll han fallado. Deben instalarse a mano. Puede descargar el archivo de http://www.rallados.net/descargas/msvcr70.dll', mbInformation, MB_OK);
        end else
          didCrtDl := true;
      end else
        MsgBox('Debe instalar a mano msvcr70.dll. Puede descargar el archivo de http://www.rallados.net/descargas/msvcr70.dll', mbInformation, MB_OK);
    end;

    if (NOT FileExists(output)) then begin
          if (NOT FileExists(dbghelp)) then
        m := StringOfChar('0',1);
      if (StrToInt(m[1]) < 5) then begin
        answer := MsgBox('Se requiere dbghelp.dll versión 5.1. ¿Quiere descargarla?', mbConfirmation, MB_YESNO);
        if answer = IDYES then begin
          tmp := ExpandConstant('{tmp}\dbghelp.dll');
          isxdl_SetOption('title', 'Descargando dbghelp.dll');
          hWnd := StrToInt(ExpandConstant('{wizardhwnd}'));
          if isxdl_Download(hWnd, dbgurl, tmp) = 0 then begin
            MsgBox('La descarga e instalación  de dbghelp.dll han fallado. Deben instalarse a mano. Puede descargar el archivo de http://www.rallados.net/descargas/dbghelp.dll', mbInformation, MB_OK);
          end else
            didDbgDl := true;
        end else
        MsgBox('Debe instalar a mano dbghelp.dll. Puede descargar el archivo de http://www.rallados.net/descargas/dbghelp.dll', mbInformation, MB_OK);
      end;
    end;
  end;
  Result := true;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
input,output: String;
begin
  if (CurStep = ssPostInstall) then begin
    if (didDbgDl) then begin
      input := ExpandConstant('{tmp}\dbghelp.dll');
      output := ExpandConstant('{app}\dbghelp.dll');
      FileCopy(input, output, true);
    end;
    if (didCrtDl) then begin
      input := ExpandConstant('{tmp}\msvcr70.dll');
      output := ExpandConstant('{sys}\msvcr70.dll');
      FileCopy(input, output, true);
    end;
    if (didMsCrtDl) then begin
      input := ExpandConstant('{tmp}\msvcrtd.dll');
      output := ExpandConstant('{sys}\msvcrtd.dll');
      FileCopy(input, output, true);
    end;
  end;
end;

[Icons]
Name: "{group}\Colossus"; Filename: "{app}\colossus.exe"; WorkingDir: "{app}"
Name: "{group}\Desinstalar Colossus"; Filename: "{uninstallexe}"; WorkingDir: "{app}"
Name: "{group}\Documentación"; Filename: "{app}\colossusdoc.html"; WorkingDir: "{app}"
Name: "{userdesktop}\Colossus"; Filename: "{app}\colossus.exe"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Colossus"; Filename: "{app}\colossus.exe"; WorkingDir: "{app}"; Tasks: quicklaunchicon
#ifdef USA_SSL
Name: "{group}\Hacer Certificado"; Filename: "{app}\makecert.bat"; WorkingDir: "{app}"
Name: "{group}\Cifrar Certificado"; Filename: "{app}\encpem.bat"; WorkingDir: "{app}"
#endif

[Run]
Filename: "{app}\colossusdoc.html"; Description: "Ver documentación"; Parameters: ""; Flags: postinstall skipifsilent shellexec runmaximized
Filename: "notepad"; Description: "Ver cambios"; Parameters: "{app}\cambios.txt"; Flags: postinstall skipifsilent shellexec runmaximized
#ifdef USA_SSL
Filename: "{app}\makecert.bat"; Tasks: makecert
Filename: "{app}\encpem.bat"; WorkingDir: "{app}"; Tasks: enccert
#endif

[UninstallRun]

[Languages]
Name: "Castellano"; MessagesFile: "compiler:Languages\Spanish.isl"

