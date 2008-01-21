/*
 * $Id: misc.c,v 1.18 2008-01-21 19:46:46 Trocotronic Exp $ 
 */

#include "struct.h"
#include <pthread.h>

#ifdef _WIN32
#include <io.h>
WIN32_FIND_DATA FindFileData;
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#endif
#include <sys/stat.h>
extern pthread_mutex_t mutex;

typedef struct _ecmd
{
	char *cmd;
	char *params;
	u_long *len;
	char **res;
	ECmdFunc func;
	void *v;
}ECmd;
#ifdef _WIN32
HANDLE CreateChildProcess(char *cmd, HANDLE hChildStdinRd, HANDLE hChildStdoutWr) 
{ 
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bFuncRetn = FALSE;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = hChildStdoutWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
   	if (!(bFuncRetn = CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo)))
   		return 0;
   	WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	return piProcInfo.hProcess;
}
int EjecutaCmd(ECmd *ecmd)
{
	/*char chBuf[BUFSIZE], *res;
   	int fp;
   	struct stat inode;
   	ircsprintf(chBuf, "@%s %s > tmp/output.tmp 2>NUL", ecmd->cmd, ecmd->params);
   	system(chBuf);
   	if ((fp = open("tmp/output.tmp", O_RDONLY|O_BINARY)) != -1)
	{
		if (fstat(fp, &inode) != -1)
		{
			res = (char *)Malloc(inode.st_size + 1);
			res[inode.st_size] = '\0';
			if (read(fp, res, inode.st_size) != inode.st_size)
				ircfree(res);
			if (ecmd->func)
				ecmd->func(inode.st_size, res, ecmd->v);
			else
			{
				if (ecmd->len)
					*ecmd->len = inode.st_size;
				if (ecmd->res)
					*ecmd->res = res;
			}
		}
		close(fp);
	}
	_unlink("tmp/output.tmp");
	Free(ecmd);
   	return 0;*/
	HANDLE hChildStdoutRd, hProc;
	SECURITY_ATTRIBUTES saAttr; 
   	DWORD dwRead;
   	char *res, tmp[BUFSIZE], name[BUFSIZE];
   	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL;
	do {
		ircsprintf(name, "tmp/output%lu.tmp", Aleatorio(0, 99999));
		hChildStdoutRd = CreateFile(name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, &saAttr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	}while (hChildStdoutRd == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS);
	ircsprintf(tmp, "%s %s", ecmd->cmd, ecmd->params);
	if ((hProc = CreateChildProcess(tmp, NULL, hChildStdoutRd)))
	{
		SetFilePointer(hChildStdoutRd, 0, NULL, FILE_BEGIN);
		dwRead = GetFileSize(hChildStdoutRd, NULL);
		res = (char *)Malloc(sizeof(char) * dwRead);
		ReadFile(hChildStdoutRd, res, sizeof(char) * dwRead, &dwRead, NULL);
	}
	CloseHandle(hChildStdoutRd);
	do {
		DeleteFile(name);
	}while (GetLastError() == ERROR_SHARING_VIOLATION);
     if (ecmd->func)
     	ecmd->func(dwRead, res, ecmd->v);
     else
     {
     	if (ecmd->len)
     		*ecmd->len = dwRead;
     	if (ecmd->res)
     		*ecmd->res = res;
     }
     Free(ecmd);
     return 0;
}
#else
#define TBLOQ 4092
int EjecutaCmd(ECmd *ecmd)
{
	FILE *fp;
	char chBuf[TBLOQ], *res;
	int dwRead, i = 1, libres = TBLOQ;
   	u_long len = 0L;
   	size_t t;
	ircsprintf(chBuf, "%s %s", ecmd->cmd, ecmd->params);
	t = sizeof(char) * TBLOQ;
	res = (char *)Malloc(t);
	*res = '\0';
	if ((fp = popen(chBuf, "r")))
	{
		while (fgets(chBuf, sizeof(chBuf)-1, fp))
		{
			dwRead = strlen(chBuf);
			libres -= dwRead;
			len += dwRead;
			if (libres <= 1)
			{
				i++;
				libres = TBLOQ;
				t = sizeof(char) * TBLOQ * i;
				res = (char *)realloc(res, t);
			}
			strlcat(res, chBuf, t);
		}
		pclose(fp);
		if (ecmd->func)
     		ecmd->func(len, res, ecmd->v);
     	else
     	{
     		if (ecmd->len)
     			*ecmd->len = len;
     		if (ecmd->res)
     			*ecmd->res = res;
     	}
	}
	else
		return 1;
	Free(ecmd);
	return 0;
}
#endif
int EjecutaComandoSinc(char *cmd, char *params, u_long *len, char **res)
{
	ECmd *ecmd;
	ecmd = BMalloc(ECmd);
	ecmd->cmd = cmd;
	ecmd->params = params;
	ecmd->len = len;
	ecmd->res = res;
	return EjecutaCmd(ecmd);
}
int EjecutaComandoASinc(char *cmd, char *params, ECmdFunc func, void *v)
{
	ECmd *ecmd;
	pthread_t id;
	int val;
	ecmd = BMalloc(ECmd);
	ecmd->cmd = cmd;
	ecmd->params = params;
	ecmd->func = func;
	ecmd->v = v;
	do {
		val = pthread_create(&id, NULL, (void *)EjecutaCmd, (void *)ecmd);
	}while (val == EAGAIN);
	return 0;
}
/*!
 * @desc: Indica si un archivo existe o no
 * @params: $archivo [in] Ruta completa del archivo
 * @ret: Devuelve 1 si existe; 0, si no.
 * @cat: Programa
 !*/

int EsArchivo(char *archivo)
{
	FILE *fp;
	if (!(fp = fopen(archivo, "r")))
		return 0;
	fclose(fp);
	return 1;
}

/*!
 * @desc: Indica si se trata de una ip en notación por puntos (X.X.X.X)
 * @params: $ip [in] Ip a analizar
 * @ret: Devuelve 1 si se trata de una ip en notación por puntos; 0, si no.
 * @cat: Internet
 !*/

int EsIp(char *ip)
{
	u_int oct[4];
	if (!ip)
		return 0;
	if (sscanf(ip, "%u.%u.%u.%u", &oct[0], &oct[1], &oct[2], &oct[3]) == 4)
		return 1;
	return 0;
}
typedef struct
{
	char **destino;
	char *ip;
}Host;
void Dominio(Host *aux)
{
	struct hostent *he;
	int addr;
	addr = inet_addr(aux->ip);
	if ((he = gethostbyaddr((char *)&addr, 4, AF_INET)))
	{
		*aux->destino = strdup(he->h_name);
		InsertaCache(CACHE_HOST, aux->ip, 86400, 0, he->h_name);
	}
	else
		ircfree(*aux->destino);
	Free(aux->ip);
	Free(aux);
}

/*!
 * @desc: Resuelve una ip asíncronamente
 * @params: $destino [out] Dirección del puntero que apunta a la zona de memoria donde se guardará el resultado
 	    $ip [in] Dirección IP en notación de puntos a resolver
 * @cat: Internet
 !*/

void ResuelveHost(char **destino, char *ip)
{
	char *cache;
	if (EsIp(ip))
	{
		if ((cache = CogeCache(CACHE_HOST, ip, 0)))
			*destino = strdup(cache);
		else
		{
			pthread_t id;
			Host *aux;
			aux = BMalloc(Host);
			aux->destino = destino;
			aux->ip = strdup(ip);
			pthread_create(&id, NULL, (void *)Dominio, (void *)aux);
		}
	}
	else
		*destino = strdup(ip);
}

/*!
 * @desc: Abre un recurso de tipo Directorio para obtener los nombres de los archivos que hay dentro.
 * @params: $dirname [in] Nombre del directorio a abrir.
 * @ret: Devuelve un recurso de tipo Directorio. NULL si no se ha abierto.
 * @cat: Programa
 * @ver: LeeDirectorio CierraDirectorio
 !*/
Directorio AbreDirectorio(char *dirname)
{
	Directorio dir;
#ifdef _WIN32
	char DirSpec[MAX_PATH + 1], *c, *d; 
	for (c = dirname, d = DirSpec; !BadPtr(c); c++)
	{
		if (*c == '/')
		{
			*d++ = '\\';
			*d++ = '\\';
		}
		else
			*d++ = *c;
	}
	*d++ = '\\';
	*d++ = '\\';
	*d++ = '*';
	*d++ = '\0';
	if ((dir = FindFirstFile(DirSpec, &FindFileData)) == INVALID_HANDLE_VALUE)
		return NULL;
	return dir;
#else
	if ((dir = opendir(dirname)) == NULL)
		return NULL;
	return dir;
#endif
}
/*!
 * @desc: Obtiene los nombres de los archivos que se haya dentro del directorio 'dir', previamente abierto con <b>AbreDirectorio</b>. Recuerde que una vez haya usado este recurso, <b>debe</b> cerrarlo.
 * @param: $dir [in] Recurso de tipo Directorio devuelto por <b>AbreDirectorio</b>.
 * @ret: Durante cada llamada a esta función irá devolviendo los nombres de los archivos. NULL cuando no hay más archivos por listar.
 * @cat: Programa
 * @ver: AbreDirectorio CierraDirectorio
 * @ex: 	Directorio dir;
 	if ((dir = AbreDirectorio("database")))
 	{
 		char *nombre;
 		while ((nombre = LeeDirectorio(dir)))
 		{
 			// nombre irá tomando los nombres de los archivos en "database" 
 		}
 		CierraDirectorio(dir);
 	}
 	// no se ha podido abrir el directorio, posiblemente no exista
 !*/
char *LeeDirectorio(Directorio dir)
{
#ifdef _WIN32
	static char archivo[256];
	char *c;
	if (BadPtr(FindFileData.cFileName))
		return NULL;
	while (!strcmp(FindFileData.cFileName, ".") || !strcmp(FindFileData.cFileName, ".."))
	{
		if (!FindNextFile(dir, &FindFileData))
		{
			*FindFileData.cFileName = '\0';
			return NULL;
		}
	}
	if ((c = strrchr(FindFileData.cFileName, '\\')))
		strlcpy(archivo, c+1, sizeof(archivo));
	else
		strlcpy(archivo, FindFileData.cFileName, sizeof(archivo));
	if (!FindNextFile(dir, &FindFileData))
		*FindFileData.cFileName = '\0';
	return archivo;
#else
	struct dirent *dir_entry;
	struct stat stat_info;
	while ((dir_entry = readdir(dir)) != NULL)
	{
		lstat(dir_entry->d_name, &stat_info);
		if (S_ISDIR(stat_info.st_mode)) 
			continue;
		return dir_entry->d_name;
	}
	return NULL;
#endif
}
/*!
 * @desc: Cierra un recurso de tipo directorio. Recuerde que una vez haya usado este recurso, <b>debe</b> cerrarlo.
 * @param: $dir [in] Recurso de tipo directorio a cerrar.
 * @ver: AbreDirectorio LeeDirectorio
 * @cat: Programa
 !*/
void CierraDirectorio(Directorio dir)
{
#ifdef _WIN32
	FindClose(dir);
#else
	closedir(dir);
#endif
}

/*!
 * @desc: Genera una ventana de información.
 * @params: $err [in] Tipo de información.
 		- FERR: Se trata de un error. Mostrará una aspa roja.
 		- FSQL: Se trata de una advertencia en el motor SQL.
 		- FADV: Se trata de una advertencia. Mostrará una admiración amarilla.
 		- FOK: Se trata de un información. Mostrará una "i" azúl.
 	    $error [in] Cadena con formato a mostrar.
 	    $... [in] Argumentos variables según la cadena de formato.
 * @cat: Programa
 !*/
 
void Alerta(char err, char *error, ...)
{
	char buf[BUFSIZE];
#ifdef _WIN32
	int opts = MB_OK;
#endif
	va_list vl;
	va_start(vl, error);
	ircvsprintf(buf, error, vl);
	va_end(vl);
#ifdef _WIN32
	switch (err)
	{
		case FERR:
			opts |= MB_ICONERROR;
			break;
		case FSQL:
		case FADV:
			opts |= MB_ICONWARNING;
			break;
		case FOK:
			opts |= MB_ICONINFORMATION;
			break;
	}
	MessageBox(hwMain, buf, "Colossus", opts);
#else
	switch (err)
	{
		case FERR:
			fprintf(stderr, "[ERROR: %s]\n", buf);
			break;
		case FSQL:
			fprintf(stderr, "[SQL: %s]\n", buf);
			break;
		case FADV:
			fprintf(stderr, "[Advertencia: %s]\n", buf);
			break;
		case FOK:
			fprintf(stderr, "[Ok: %s]\n", buf);
			break;
	}
#endif
}
#ifdef NOCORE
#define DEBUG
#endif
void Debug(char *formato, ...)
{
#ifndef DEBUG
	return;
#else
	char debugbuf[8000];
	va_list vl;
#ifdef _WIN32
	static HANDLE conh = NULL;
	DWORD len = 0;
#endif
	va_start(vl, formato);
	vsnprintf(debugbuf, sizeof(debugbuf)-3, formato, vl);
	va_end(vl);
	strncat(debugbuf, "\r\n", sizeof(debugbuf));
#ifdef _WIN32
	if (!conh)
	{
		if (AllocConsole())
			conh = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	if (conh)
		WriteFile(conh, debugbuf, strlen(debugbuf), &len, NULL);
#else
	fprintf(stderr, debugbuf);
#endif
#endif
}
#ifndef _WIN32
int Pregunta(char *preg)
{
	fprintf(stderr, "%s (s/n)\n", preg);
	if (getc(stdin) == 's')
		return 1;
	return 0;
}
#endif

/*!
 * @desc: Guarda en el archivo .log los datos especificados, según se haya especificado en la configuración del programa.
 * @params: $opt [in] Tipo de información.
 		- LOG_CONN: Relacionado con conexiones.
 		- LOG_SERVER: Relacionado con servidores.
 		- LOG_ERROR: Relacionado con errores.
 		- LOG_MSN: Relacionado con el complemento MSN.
 	    $formato [in] Cadena con formato a loguear.
 	    $... [in] Argumentos variables según cadena de formato.
 * @cat: Programa
 !*/
 
void Loguea(int opt, char *formato, ...)
{
	char buf[256], auxbuf[512], *campo;
	int fp;
	va_list vl;
	time_t tm;
	struct stat inode;
	if (!conf_log || !(conf_log->opts & opt))
		return;
	tm = time(0);
	va_start(vl, formato);
	ircvsprintf(buf, formato, vl);
	va_end(vl);
	if (opt == LOG_CONN)
		campo = "CON";
	else if (opt == LOG_SERVER)
		campo = "SERVER";
	else if (opt == LOG_ERROR)
		campo = "ERR";
	else if (opt == LOG_MSN)
		campo = "MSN";
	ircsprintf(auxbuf, "[%s] (%s) > %s\n", Fecha(&tm), buf);
	if (stat(conf_log->archivo, &inode) == -1)
		return;
	if (conf_log->size && inode.st_size > conf_log->size)
		fp = open(conf_log->archivo, O_CREAT|O_WRONLY|O_TRUNC, S_IREAD|S_IWRITE);
	else
		fp = open(conf_log->archivo, O_CREAT|O_APPEND|O_WRONLY, S_IREAD|S_IWRITE);
	if (fp == -1)
		return;
	write(fp, auxbuf, strlen(auxbuf));
	close(fp);
}

/*!
 * @desc: Codifica la cadena para mandarse vía HTTP. Esta función usa Malloc, por lo que debe liberarse con Free una vez utilizada.
 * @params: $str [in] Cadena a codificar.
 * @ret: Devuelve un puntero a una nueva cadena codificada. No olvidar a liberar con Free una vez utilizado.
 * @cat: Internet
 !*/
 
char *URLEncode(char *str)
{
	char *coded, *c, *d;
	if (BadPtr(str))
		return NULL;
	coded = (char *)Malloc(sizeof(char) * (strlen(str) * 3 + 1)); /* como máximo, se codificará todo */
	d = coded;
	for (c = str; !BadPtr(c); c++)
	{
		if (IsAlnum(*c) || *c == '-' || *c == '_' || *c == '.')
			*d++ = *c;
		else
		{
			char tmp[3];
			ircsprintf(tmp, "%X", *c);
			*d++ = '%';
			*d++ = tmp[0];
			*d++ = tmp[1];
		}
	}
	*d++ = '\0';
	return coded;
}
