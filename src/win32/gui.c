/*
 * $Id: gui.c,v 1.2 2004-10-02 22:48:45 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "resource.h"
#include <commctrl.h>
#include <windows.h>

extern void InitDebug(void);
extern int carga_programa(int, char **);

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
	carga_programa(__argc, __argv);
	if (WSAStartup(MAKEWORD(1, 1), &wsaData))
	{
		MessageBox(hWnd, "No se ha podido inicializar winsock.", "Error", MB_OK|MB_ICONERROR);
		exit(-1);
	}
	ShowWindow(hWnd, SW_SHOW);
	if ((hThreadPrincipal = (HANDLE)_beginthread(programa_loop_principal, 0, NULL)) < 0)
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
	static HMENU hTray, hConfig;
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
			if (MessageBox(hDlg, "�Quieres cerrar Colossus?", "�Est�s seguro?", MB_YESNO|MB_ICONQUESTION) == IDNO)
				return 0;
			else 
				cierra_colossus(0);
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
					GetCursorPos(&p);
					DestroyMenu(hConfig);
					hConfig = CreatePopupMenu();
					AppendMenu(hConfig, MF_STRING, IDM_CONF, "colossus.conf");
					AppendMenu(hConfig, MF_SEPARATOR, 0, NULL);
					for (ex = modulos; ex; ex = ex->sig)
					{
						if (ex->cargado) /* est� en conf */
							AppendMenu(hConfig, MF_STRING, i++, ex->config);
					}
					ModifyMenu(hTray, IDM_CONFIG, MF_BYCOMMAND|MF_POPUP|MF_STRING, (UINT)hConfig, "&Configuraci�n");
					TrackPopupMenu(hTray, TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x, p.y, 0, hDlg, NULL);
					/* Kludge for a win bug */
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
				ShellExecute(NULL, "open", path, NULL, NULL, SW_MAXIMIZE);
				break;
			}
			switch(LOWORD(wParam))
			{
				case BT_CON:
				{
					if (IsDlgButtonChecked(hDlg, BT_CON))
					{
						ChkBtCon(1, 1);
						abre_sock_ircd();
					}
					else
					{
						ChkBtCon(0, 1);
						sockclose(SockIrcd, LOCAL);
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
					AppendMenu(hConfig, MF_STRING, IDM_CONF, "colossus.conf");
					AppendMenu(hConfig, MF_SEPARATOR, 0, NULL);
					for (ex = modulos; ex; ex = ex->sig)
					{
						if (ex->cargado) /* est� en conf */
							AppendMenu(hConfig, MF_STRING, i++, ex->config);
					}
					TrackPopupMenu(hConfig, TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x, p.y, 0, hDlg, NULL);
					/* Kludge for a win bug */
					SendMessage(hDlg, WM_NULL, 0, 0);
					break;
				}
				case IDM_AYUDA:
				case BT_AYUDA:
					ShellExecute(NULL, "open", "colossusdoc.html", NULL, NULL, SW_MAXIMIZE);
					break;
				case IDM_REHASH:
				case BT_REHASH:
					fecho(FOK, "Refrescando servicios");
					refresca();
					break;
				case IDM_SHUTDOWN:
					if (MessageBox(hDlg, "�Quieres cerrar Colossus?", "�Est�s seguro?", MB_YESNO|MB_ICONQUESTION) == IDNO)
						return 0;
					else 
						cierra_colossus(0);
					break;
			}
			return 0;
		}
	}
	return FALSE;
}
void ChkBtCon(int val, int block)
{
	SetDlgItemText(hwMain, BT_CON, val ? "Desconectar" : "Conectar");
	CheckDlgButton(hwMain, BT_CON, val && !block ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwMain, BT_CON), block ? FALSE : TRUE);
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
void conferror(char *formato, ...)
{
	char buf[BUFSIZE], *texto, actual[BUFSIZE];
	int len;
	va_list vl;
	va_start(vl, formato);
	vsprintf_irc(buf, formato, vl);
	va_end(vl);
	strcat(buf, "\r\n");
	if (!hwConfError)
	{
		hwConfError = CreateDialog(hInst, "CONFERROR", 0, (DLGPROC)ConfErrorDLG);
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
