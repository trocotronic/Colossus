/*
 * $Id: gui.c,v 1.12 2005-12-25 21:15:06 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "resource.h"
#ifdef USA_ZLIB
#include "zip.h"
#endif
#ifdef UDB
#include "bdd.h"
#endif
#include <commctrl.h>
#include <windows.h>

extern void InitDebug(void);
extern int IniciaPrograma(int, char **);

NOTIFYICONDATA SysTray;
HINSTANCE hInst;
HWND hwMain = NULL, hwConfError = NULL;
UINT WM_TASKBARCREATED;
OSVERSIONINFO VerInfo;
char SO[256];
HANDLE hThreadPrincipal;
void CleanUpSegv(int sig)
{
	Shell_NotifyIcon(NIM_DELETE ,&SysTray);
}
void CleanUp(void)
{
	Shell_NotifyIcon(NIM_DELETE ,&SysTray);
}
void TaskBarCreated() 
{
	HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(ICO_MAIN), IMAGE_ICON, 16, 16, 0);
	SysTray.cbSize = sizeof(NOTIFYICONDATA);
	SysTray.hIcon = hIcon;
	SysTray.hWnd = hwMain;
	SysTray.uCallbackMessage = WM_USER;
	SysTray.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
	SysTray.uID = 0;
	lstrcpy(SysTray.szTip, COLOSSUS_VERSION);
	Shell_NotifyIcon(NIM_ADD ,&SysTray);
}
LRESULT CALLBACK MainDLG(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ConfErrorDLG(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AcercaDLG(HWND, UINT, WPARAM, LPARAM);
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	WSADATA wsaData;
	InitCommonControls();
	WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
	hWnd = CreateDialog(hInstance, "COLOSSUS", 0, (DLGPROC)MainDLG);
	hwMain = hWnd;
	hInst = hInstance; 
	atexit(CleanUp);
	TaskBarCreated();
	ShowWindow(hWnd, SW_SHOW);
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);
	strcpy(SO, "Windows ");
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) 
	{
		if (VerInfo.dwMajorVersion == 4) 
		{
			if (VerInfo.dwMinorVersion == 0) 
			{
				strcat(SO, "95 ");
				if (!strcmp(VerInfo.szCSDVersion," C"))
					strcat(SO, "OSR2 ");
			}
			else if (VerInfo.dwMinorVersion == 10) 
			{
				strcat(SO, "98 ");
				if (!strcmp(VerInfo.szCSDVersion, " A"))
					strcat(SO, "SE ");
			}
			else if (VerInfo.dwMinorVersion == 90)
				strcat(SO, "Me ");
		}
	}
	else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) 
	{
		if (VerInfo.dwMajorVersion == 3 && VerInfo.dwMinorVersion == 51)
			strcat(SO, "NT 3.51 ");
		else if (VerInfo.dwMajorVersion == 4 && VerInfo.dwMinorVersion == 0)
			strcat(SO, "NT 4.0 ");
		else if (VerInfo.dwMajorVersion == 5) 
		{
			if (VerInfo.dwMinorVersion == 0)
				strcat(SO, "2000 ");
			else if (VerInfo.dwMinorVersion == 1) 
				strcat(SO, "XP ");
			else if (VerInfo.dwMinorVersion == 2)
				strcat(SO, "Server 2003 ");
		}
		strcat(SO, VerInfo.szCSDVersion);
	}
	if (SO[strlen(SO)-1] == ' ')
		SO[strlen(SO)-1] = 0;
	InitDebug();
	if (WSAStartup(MAKEWORD(1, 1), &wsaData))
	{
		MessageBox(hWnd, "No se ha podido inicializar winsock.", "Error", MB_OK|MB_ICONERROR);
		exit(-1);
	}
	if (IniciaPrograma(__argc, __argv))
		exit(-1);
	ShowWindow(hWnd, SW_SHOW);
	if ((hThreadPrincipal = (HANDLE)_beginthread(LoopPrincipal, 0, NULL)) < 0)
	{
		MessageBox(hWnd, "Ha sido imposible crear el thread.", "Error", MB_OK|MB_ICONERROR);
		exit(-1);
	}
	while (GetMessage(&msg, NULL, 0, 0))
    	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return FALSE;
}
LRESULT CALLBACK MainDLG(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HMENU hTray, hConfig, hHelp;
	POINT p;
	if (message == WM_TASKBARCREATED)
	{
		TaskBarCreated();
		return TRUE;
	}
	switch (message)
	{
		case WM_INITDIALOG: 
		{
			ShowWindow(hDlg, SW_HIDE);
			SetWindowText(hDlg, COLOSSUS_VERSION);
			hTray = GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(MENU_SYSTRAY)), 0);
			SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, 
				(LPARAM)(HICON)LoadImage(NULL, "src/win32/icon1.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE));
			SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, 
				(LPARAM)(HICON)LoadImage(NULL, "src/win32/icon1.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE));
			return TRUE;
		}
		case WM_CLOSE: 
		{
			if (MessageBox(hDlg, "¿Quieres cerrar Colossus?", "¿Estás seguro?", MB_YESNO|MB_ICONQUESTION) == IDNO)
				return 0;
			else 
				CierraColossus(0);
		}
		case WM_SIZE: 
		{
			if (wParam & SIZE_MINIMIZED)
				ShowWindow(hDlg, SW_HIDE);
			return 0;
		}
		case WM_DESTROY:
			return 0;
		case WM_USER: 
		{
			switch(LOWORD(lParam)) 
			{
				case WM_LBUTTONDBLCLK:
					ShowWindow(hDlg, SW_SHOW);
					ShowWindow(hDlg, SW_RESTORE);
				case WM_RBUTTONDOWN:
					SetForegroundWindow(hDlg);
					break;
				case WM_RBUTTONUP:
				{
					unsigned long i = 60001;
					Modulo *ex;
					MENUITEMINFO mitem;
					GetCursorPos(&p);
					DestroyMenu(hConfig);
					DestroyMenu(hHelp);
					hConfig = CreatePopupMenu();
					AppendMenu(hConfig, MF_STRING, IDM_CONF, CPATH);
					AppendMenu(hConfig, MF_SEPARATOR, 0, NULL);
					for (ex = modulos; ex; ex = ex->sig)
					{
						if (ex->cargado) /* está en conf */
							AppendMenu(hConfig, MF_STRING, i++, ex->config);
					}
					mitem.cbSize = sizeof(MENUITEMINFO);
					mitem.fMask = MIIM_SUBMENU;
					mitem.hSubMenu = hConfig;
					SetMenuItemInfo(hTray, IDM_CONFIG, MF_BYCOMMAND, &mitem);
					hHelp = CreatePopupMenu();
					AppendMenu(hHelp, MF_STRING, IDM_AYUDA, "&Ayuda");
					AppendMenu(hHelp, MF_STRING, IDM_LOG, "Ver &log");
					AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
					AppendMenu(hHelp, MF_STRING, IDM_ACERCA, "Acerca &de Colossus...");
					mitem.hSubMenu = hHelp;
					SetMenuItemInfo(hTray, IDM_AYUDA_MENU, MF_BYCOMMAND, &mitem);
					TrackPopupMenu(hTray, TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x, p.y, 0, hDlg, NULL);
					SendMessage(hDlg, WM_NULL, 0, 0);
					break;
				}
			}
			return 0;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam) >= 60000 && HIWORD(wParam) == 0 && !lParam) 
			{
				unsigned char path[MAX_PATH];
				GetMenuString(hConfig, LOWORD(wParam), path, MAX_PATH, MF_BYCOMMAND);
				ShellExecute(NULL, "open", "notepad", path, NULL, SW_MAXIMIZE);
				break;
			}
			switch(LOWORD(wParam))
			{
				case BT_CON:
				{
					if (IsDlgButtonChecked(hDlg, BT_CON))
					{
						ChkBtCon(0, 1);
						AbreSockIrcd();
					}
					else
					{
						SockClose(SockIrcd, LOCAL);
						ChkBtCon(0, 0);
					}
					break;
				}
				case BT_CONF:
				{
					unsigned long i = 60001;
					Modulo *ex;
					GetCursorPos(&p);
					DestroyMenu(hConfig);
					hConfig = CreatePopupMenu();
					AppendMenu(hConfig, MF_STRING, IDM_CONF, CPATH);
					AppendMenu(hConfig, MF_SEPARATOR, 0, NULL);
					for (ex = modulos; ex; ex = ex->sig)
					{
						if (ex->cargado) /* está en conf */
							AppendMenu(hConfig, MF_STRING, i++, ex->config);
					}
					TrackPopupMenu(hConfig, TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x, p.y, 0, hDlg, NULL);
					SendMessage(hDlg, WM_NULL, 0, 0);
					break;
				}
				case BT_AYUDA:
				{
					GetCursorPos(&p);
					DestroyMenu(hHelp);
					hHelp = CreatePopupMenu();
					AppendMenu(hHelp, MF_STRING, IDM_AYUDA, "&Ayuda");
					AppendMenu(hHelp, MF_STRING, IDM_LOG, "Ver &log");
					AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
					AppendMenu(hHelp, MF_STRING, IDM_ACERCA, "Acerca &de Colossus...");
					TrackPopupMenu(hHelp, TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x, p.y, 0, hDlg, NULL);
					SendMessage(hDlg, WM_NULL, 0, 0);
					break;
				}
				case IDM_AYUDA:
					ShellExecute(NULL, "open", "colossusdoc.html", NULL, NULL, SW_MAXIMIZE);
					break;
				case IDM_LOG:
					ShellExecute(NULL, "open", "notepad", "colossus.log", NULL, SW_MAXIMIZE);
					break;
				case IDM_ACERCA:
					DialogBox(hInst, "Acerca", hDlg, (DLGPROC)AcercaDLG);
					break;
				case IDM_REHASH:
				case BT_REHASH:
					Refresca();
					break;
				case IDM_SHUTDOWN:
					if (MessageBox(hDlg, "¿Quieres cerrar Colossus?", "¿Estás seguro?", MB_YESNO|MB_ICONQUESTION) == IDNO)
						return 0;
					else 
						CierraColossus(0);
					break;
			}
			return 0;
		}
	}
	return FALSE;
}
LRESULT CALLBACK ConfErrorDLG(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) 
	{
		case WM_INITDIALOG:
			MessageBeep(MB_ICONEXCLAMATION);
			return (TRUE);
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
			{
				hwConfError = NULL;
				EndDialog(hDlg, TRUE);
			}
			break;
		case WM_CLOSE:
			EndDialog(hDlg, TRUE);
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}
LRESULT CALLBACK AcercaDLG(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
	static HFONT hFont;
	static HCURSOR hCursor;
	switch (message) 
	{
		case WM_INITDIALOG: 
		{
			char pth[8];
			ShowWindow(hDlg, SW_HIDE);
			SetWindowText(hDlg, "Acerca de Colossus...");
			SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, 
				(LPARAM)(HICON)LoadImage(NULL, "src/win32/icon1.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE));
			SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, 
				(LPARAM)(HICON)LoadImage(NULL, "src/win32/icon1.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE));
			hFont = CreateFont(8,0,0,0,0,0,1,0,ANSI_CHARSET,0,0,PROOF_QUALITY,0,"MS Sans Serif");
			SendMessage(GetDlgItem(hDlg, IDC_WEB), WM_SETFONT, (WPARAM)hFont,TRUE);
#ifdef UDB
			SetDlgItemText(hDlg, IDC_UDBV, UDB_VER);
#endif
#ifdef USA_SSL
			SetDlgItemText(hDlg, IDC_OPENV, strchr(OPENSSL_VERSION_TEXT, ' ')+1);
#endif
#ifdef USA_ZLIB
			SetDlgItemText(hDlg, IDC_ZLIBV, zlibVersion());
#endif
			if (sql->clientinfo[0])
				SetDlgItemText(hDlg, IDC_SQLCV, sql->clientinfo);
			if (sql->servinfo[0])
				SetDlgItemText(hDlg, IDC_SQLSV, sql->servinfo);
			ircsprintf(pth, "%i.%i.%i", PTW32_VERSION);
			SetDlgItemText(hDlg, IDC_PTHV, pth);
			return TRUE;
		}
		case WM_DRAWITEM: 
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			unsigned char text[500];
			COLORREF oldtext;
			RECT focus;
			GetWindowText(lpdis->hwndItem, text, 500);
			if (wParam == IDC_WEB) 
			{
				FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_3DFACE));
				oldtext = SetTextColor(lpdis->hDC, RGB(0,0,255));
				DrawText(lpdis->hDC, text, strlen(text), &lpdis->rcItem, DT_LEFT);
				SetTextColor(lpdis->hDC, oldtext);
				if (lpdis->itemState & ODS_FOCUS) 
				{
					CopyRect(&focus, &lpdis->rcItem);
					focus.left += 2;
					focus.right -= 2;
					focus.top += 1;
					focus.bottom -= 1;
					DrawFocusRect(lpdis->hDC, &focus);
				}
				return TRUE;
			}
		}
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_DBLCLK) 
			{
				if (LOWORD(wParam) == IDC_WEB) 
					ShellExecute(NULL, "open", "http://www.rallados.net", NULL, NULL, SW_MAXIMIZE);
				EndDialog(hDlg, TRUE);
				return 0;
			}
			break;
		case WM_CLOSE:
			EndDialog(hDlg, TRUE);
			break;
	}
	return FALSE;
}
void Error(char *formato, ...)
{
	char buf[BUFSIZE], *texto, actual[BUFSIZE];
	int len;
	va_list vl;
	va_start(vl, formato);
	ircvsprintf(buf, formato, vl);
	va_end(vl);
	strcat(buf, "\r\n");
	if (!hwConfError)
	{
		hwConfError = CreateDialog(hInst, "CONFERROR", hwMain, (DLGPROC)ConfErrorDLG);
		ShowWindow(hwConfError, SW_SHOW);
		texto = (char *)Malloc(sizeof(char) * (strlen(buf) + 1));
		strcpy(texto, buf);
	}
	else
	{
		len = GetDlgItemText(hwConfError, EDT_ERR, actual, BUFSIZE);
		texto = (char *)Malloc(sizeof(char) * (len + strlen(buf) + 1));
		strcpy(texto, actual);
		strcat(texto, buf);
	}
	SetDlgItemText(hwConfError, EDT_ERR, texto);
	Free(texto);
}
void ChkBtCon(int val, int block)
{
	SetDlgItemText(hwMain, BT_CON, val ? "Desconectar" : "Conectar");
	CheckDlgButton(hwMain, BT_CON, val && !block ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwMain, BT_CON), block ? FALSE : TRUE);
}
int Info(char *formato, ...)
{
	static char info[2048];
	char texto[BUFSIZE], txt[BUFSIZE];
	static int len = 0;
	struct tm *timeptr;
	time_t ts;
	va_list vl;
	ts = time(0);
	timeptr = localtime(&ts);
	va_start(vl, formato);
	ircsprintf(texto, "(%.2i:%.2i:%.2i) %s\r\n", timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, formato);
	ircvsprintf(txt, texto, vl);
	va_end(vl);
	len += strlen(txt);
	if (len > sizeof(info))
	{
		strcpy(info, txt);
		len = 0;
	}
	else
		strcat(info, txt);
	SetDlgItemText(hwMain, EDT_INFO, info);
	return -1;
}
typedef struct {
	char *pregunta;
	char *def;
	char resp[512];
	char *titu;
}CampoST;
CampoST preg;
LRESULT CampoDLG(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message) 
	{
		case WM_INITDIALOG:
			SetWindowText(hDlg, preg.titu);
			SetDlgItemText(hDlg, IDC_OPENV, preg.pregunta);
			if (preg.def)
				SetDlgItemText(hDlg, IDC_PASS, preg.def);
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL) 
			{
				preg.resp[0] = '\0';
				EndDialog(hDlg, TRUE);
			}
			else if (LOWORD(wParam) == IDOK) 
			{
				GetDlgItemText(hDlg, IDC_PASS, preg.resp, sizeof(preg.resp));
				EndDialog(hDlg, TRUE);
			}
			return FALSE;
		case WM_CLOSE:
			preg.resp[0] = '\0';
			EndDialog(hDlg, TRUE);
		default:
			return FALSE;
	}
}
char *PreguntaCampo(char *titu, char *texto, char *def)
{
	preg.titu = titu;
	preg.pregunta = texto;
	preg.def = def;
	DialogBoxParam(hInst, "PreguntaCampo", hwMain, (DLGPROC)CampoDLG, (LPARAM)NULL); 
	if (!preg.resp[0])
		return NULL;
	return preg.resp;
}
