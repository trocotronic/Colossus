//
// $Id: colossus.rc.tpl,v 1.2 2008/02/13 16:16:11 Trocotronic Exp $
//

//Microsoft Developer Studio generated resource script.
//
#include "resource.h"

#define APSTUDIO_READONLY_SYMBOLS
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 2 resource.
//
#define APSTUDIO_HIDDEN_SYMBOLS
#include "windows.h"
#undef APSTUDIO_HIDDEN_SYMBOLS
#include "resource.h"
#include "winver.h"

/////////////////////////////////////////////////////////////////////////////
#undef APSTUDIO_READONLY_SYMBOLS

/////////////////////////////////////////////////////////////////////////////
// Spanish (Modern) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ESN)
#ifdef _WIN32
LANGUAGE LANG_SPANISH, SUBLANG_SPANISH_MODERN
#pragma code_page(1252)
#endif //_WIN32


/////////////////////////////////////////////////////////////////////////////
//
// 24
//
1 24 MOVEABLE PURE "colossus.manifest"

 
#ifdef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// TEXTINCLUDE
//

1 TEXTINCLUDE DISCARDABLE 
BEGIN
    "resource.h\0"
END

2 TEXTINCLUDE DISCARDABLE 
BEGIN
    "#define APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "#include ""windows.h""\r\n"
    "#undef APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "#include ""resource.h""\r\n"
    "#include ""winver.h""\r\n"
    "\0"
END

3 TEXTINCLUDE DISCARDABLE 
BEGIN
    "\r\n"
    "\0"
END

#endif    // APSTUDIO_INVOKED


#ifndef _MAC
/////////////////////////////////////////////////////////////////////////////
//
// Version
//

VER_COLOSSUS VERSIONINFO
 FILEVERSION 1,9,2,0
 PRODUCTVERSION 1,9,2,0
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "0c0a04b0"
        BEGIN
            VALUE "Comments", "\0"
            VALUE "CompanyName", "Redyc\0"
            VALUE "FileDescription", "Colossus - Servicios de red\0"
            VALUE "FileVersion", "1.9b\0"
            VALUE "InternalName", "Colossus\0"
            VALUE "LegalCopyright", "Copyright © 2004-2008\0"
            VALUE "LegalTrademarks", "\0"
            VALUE "OriginalFilename", "\0"
            VALUE "PrivateBuild", "\0"
            VALUE "ProductName", "Colossus\0"
            VALUE "ProductVersion", "1.9b\0"
            VALUE "SpecialBuild", "\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0xc0a, 1200
    END
END

#endif    // !_MAC


/////////////////////////////////////////////////////////////////////////////
//
// Dialog
//

COLOSSUS DIALOG DISCARDABLE  0, 0, 205, 80
STYLE DS_MODALFRAME | DS_3DLOOK | DS_NOFAILCREATE | DS_CENTER | 
    WS_MINIMIZEBOX | WS_POPUP | WS_CAPTION | WS_SYSMENU
FONT 8, "Tahoma"
BEGIN
    GROUPBOX        "Opciones disponibles",IDC_STATIC,5,5,195,70,BS_LEFT
    CONTROL         "Conectar",BT_CON,"Button",BS_AUTOCHECKBOX | BS_PUSHLIKE,10,15,90,15
    PUSHBUTTON      "Refrescar",BT_REHASH,105,15,90,15,NOT WS_TABSTOP
    PUSHBUTTON      "Configuración",BT_CONF,10,35,90,15,NOT WS_TABSTOP
    PUSHBUTTON      "Ayuda",BT_AYUDA,105,35,90,15,NOT WS_TABSTOP
    CONTROL         "Complemento MSN",BT_MSN,"Button",BS_AUTOCHECKBOX | BS_PUSHLIKE,10,55,90,15
    CONTROL         "Mostrar información",BT_OCU,"Button",BS_AUTOCHECKBOX | BS_PUSHLIKE,105,55,90,15
    GROUPBOX        "Información",IDC_STATIC,5,80,195,60,BS_LEFT
    EDITTEXT        EDT_INFO,10,90,185,45,ES_MULTILINE | ES_READONLY | WS_VSCROLL
END

CONFERROR DIALOG DISCARDABLE  0, 0, 331, 178
STYLE DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION
CAPTION "Error de configuración"
FONT 8, "Tahoma"
BEGIN
    DEFPUSHBUTTON   "Aceptar",IDOK,134,157,50,14
    EDITTEXT        EDT_ERR,7,7,317,142,ES_MULTILINE | ES_READONLY | WS_VSCROLL
END

SSLPASS DIALOG DISCARDABLE  0, 0, 174, 57
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "SSL Contraseña "
FONT 8, "Tahoma"
BEGIN
    DEFPUSHBUTTON   "Aceptar",IDOK,33,35,50,14
    PUSHBUTTON      "Cancelar",IDCANCEL,89,35,50,14
    EDITTEXT        IDC_PASS,11,12,152,14,ES_PASSWORD | ES_AUTOHSCROLL
END


ACERCA DIALOG DISCARDABLE  0, 0, 163, 113
STYLE DS_MODALFRAME | DS_3DLOOK | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Acerca de Colossus..."
FONT 8, "Tahoma"
BEGIN
    LTEXT           "Autor: Trocotronic",IDC_STATIC,10,10,76,8	
    LTEXT           "©2004-2008",IDC_VER,10,20,76,8
    CONTROL         "http://www.redyc.com/",IDC_WEB,"Button",BS_OWNERDRAW,10,30,76,8
    CONTROL         BMP_LOGO,IDC_STATIC,"Static",SS_BITMAP,111,3,64,64
    GROUPBOX        "Dependencias",IDC_STATIC,5,47,150,60,BS_LEFT
    LTEXT           "OpenSSL: ",IDC_STATIC,14,56,40,8
    LTEXT           "Desconocido",IDC_OPENV,75,56,78,8
    LTEXT           "ZLib: ",IDC_STATIC,14,66,25,8
    LTEXT           "Desconocido",IDC_ZLIBV,75,66,78,8
    LTEXT           "Cliente SQL: ",IDC_STATIC,14,76,60,8
    LTEXT           "Desconocido",IDC_SQLCV,75,76,78,8
    LTEXT           "Servidor SQL: ",IDC_STATIC,14,86,60,8
    LTEXT           "Desconocido",IDC_SQLSV,75,86,78,8
    LTEXT           "PThread: ",IDC_STATIC,14,96,30,8
    LTEXT           "Desconocido",IDC_PTHV,75,96,78,8
END

PREGUNTACAMPO DIALOG DISCARDABLE  0, 0, 174, 67
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION " "
FONT 8, "Tahoma"
BEGIN
    DEFPUSHBUTTON   "Aceptar",IDOK,33,45,50,14
    PUSHBUTTON      "Cancelar",IDCANCEL,89,45,50,14
    LTEXT           "",IDC_OPENV,14,8,160,8
    EDITTEXT        IDC_PASS,11,22,152,14, ES_AUTOHSCROLL
END


/////////////////////////////////////////////////////////////////////////////
//
// Menu
//

MENU_CONFIG MENU DISCARDABLE 
BEGIN
    POPUP "Configuración"
    BEGIN
        MENUITEM "colossus.&conf",              IDM_CONF
        MENUITEM SEPARATOR
    END
END

MENU_SYSTRAY MENU DISCARDABLE 
BEGIN
    POPUP "Systray"
    BEGIN
        MENUITEM "&Refrescar",                  IDM_REHASH
        MENUITEM "&Configuración",              IDM_CONFIG
        MENUITEM "&Ayuda",                      IDM_AYUDA_MENU
        MENUITEM "&Detener",                    IDM_SHUTDOWN
    END
END

MENU_HELP MENU DISCARDABLE 
BEGIN
    POPUP "Ayuda"
    BEGIN
        MENUITEM "&Ayuda",                  IDM_AYUDA
        MENUITEM SEPARATOR
        MENUITEM "&Acerca de Colossus...",  IDM_ACERCA
    END
END


/////////////////////////////////////////////////////////////////////////////
//
// DESIGNINFO
//

#ifdef APSTUDIO_INVOKED
GUIDELINES DESIGNINFO DISCARDABLE 
BEGIN
    "COLOSSUS", DIALOG
    BEGIN
        BOTTOMMARGIN, 126
    END

    "SSLPASS", DIALOG
    BEGIN
        LEFTMARGIN, 7
        RIGHTMARGIN, 167
        TOPMARGIN, 7
        BOTTOMMARGIN, 50
    END
END
#endif    // APSTUDIO_INVOKED


/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
ICO_MAIN               ICON    DISCARDABLE     "icon1.ico"

/////////////////////////////////////////////////////////////////////////////
//
// Bitmap
//

BMP_LOGO                BITMAP  DISCARDABLE     "colossus.bmp"

#endif    // Spanish (Modern) resources
/////////////////////////////////////////////////////////////////////////////



#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//


/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED
