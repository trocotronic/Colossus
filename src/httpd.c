/*
 * $Id: httpd.c,v 1.29 2008-02-13 16:16:08 Trocotronic Exp $ 
 */
 
#ifdef _WIN32
#include <io.h>
#else
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#endif
#include "struct.h"
#include "httpd.h"

Sock *listen_httpd;
HHead *httpcons[MAX_CON];
HDir *hdirs = NULL;
char *fecha_fmt = "%a, %d %b %Y %H:%M:%S GMT";
char *fecha_sfmt = "%s %u %s %u %u:%u:%u GMT";
char lpath[PMAX];

Opts herrores[] = {
	{ 100 , "Continue" } ,
	{ 101 , "Switching Protocols" } ,
	{ 200 , "OK" } ,
	{ 201 , "Created" } ,
	{ 202 , "Accepted" } ,
	{ 203 , "Non-Authoritative Information" } ,
	{ 204 , "No Content" } ,
	{ 205 , "Reset Content" } ,
	{ 206 , "Partial Content" } ,
	{ 300 , "Multiple Choices" } ,
	{ 301 , "Moved Permanently" } ,
	{ 302 , "Found" } ,
	{ 303 , "See Other" } ,
	{ 304 , "Not Modified" } ,
	{ 305 , "Use Proxy" } ,
	{ 307 , "Temporary Redirect" } ,
	{ 400 , "Bad Request" } ,
	{ 401 , "Unauthorized" } ,
	{ 402 , "Payment Required" } ,
	{ 403 , "Forbidden" } ,
	{ 404 , "Not Found" } ,
	{ 405 , "Method Not Allowed" } ,
	{ 406 , "Not Acceptable" } ,
	{ 407 , "Proxy Authentication Required" } ,
	{ 408 , "Request Timeout" } ,
	{ 409 , "Conflict" } ,
	{ 410 , "Gone" } ,
	{ 411 , "Length Required" } ,
	{ 412 , "Precondition Failed" } ,
	{ 413 , "Request Entity Too Large" } ,
	{ 414 , "Request-URI Too Large" } ,
	{ 415 , "Unsupported Media Type" } ,
	{ 416 , "Request Range Not Satisfiable" } ,
	{ 417 , "Expectation Failed" } ,
	{ 500 , "Internal Server Error" } ,
	{ 501 , "Not Implemented" } ,
	{ 502 , "Bad Gateway" } ,
	{ 503 , "Service Unavailable" } ,
	{ 504 , "Gateway Timeout" } ,
	{ 505 , "HTTP Version Not Supported" } ,
	{ 0x0 , 0x0 }
};
char *meses[] = {
	"Jan" , "Feb" , "Mar" , "Apr" , "May" , "Jun" ,
	"Jul" , "Aug" , "Sep" , "Oct" , "Nov" , "Dec" , 
	NULL
};
int BuscaMes(char *mes)
{
	int i;
	if (BadPtr(mes))
		return -1;
	for (i = 0; i < 12; i++)
	{
		if (!strcmp(mes, meses[i]))
			return i;
	}
	return -1;
}
char *BuscaTipo(char *ext)
{
#ifdef _WIN32
	HKEY hk, hks;
	char valor[128], item[128];
	static char tmp[128];
	DWORD i, j, k = 0, svalor = sizeof(valor), stmp = sizeof(tmp), sitem = sizeof(item);
	if (!ext)
		return "text/plain";
	if (*ext == '.')
		ext++;
	if (!strncmp(ext, "php", 3))
		return "text/html";
	ircsprintf(buf, "SOFTWARE\\Classes\\.%s", ext);
	if (RegOpenKey(HKEY_LOCAL_MACHINE, buf, &hk) == ERROR_SUCCESS)
	{
		for (j = 0; RegEnumValue(hk, j, item, &sitem, NULL, NULL, tmp, &stmp) != ERROR_NO_MORE_ITEMS; j++)
		{
			sitem = sizeof(item);
			stmp = sizeof(valor);
			if (!strcmp(item, "Content Type"))
			{
				k = 1;
				break;
			}
		}
		RegCloseKey(hk);
		if (k)
			return tmp;
	}
	return "text/plain";
	/*{
		for (i = 0; RegEnumKey(hk, i, tmp, stmp) != ERROR_NO_MORE_ITEMS; i++)
		{
			if (RegOpenKeyEx(hk, tmp, 0, KEY_READ, &hks) == ERROR_SUCCESS)
			{
				for (j = 0; RegEnumValue(hks, j, item, &sitem, NULL, NULL, valor, &svalor) != ERROR_NO_MORE_ITEMS; j++)
				{
					sitem = sizeof(item);
					svalor = sizeof(valor);
					if (!strcmp(item, "Extension") && !strcmp(&valor[1], ext))
					{
						k = 1;
						break;
					}
				}
				//for (j = 0; RegEnumKey(hks, j, valor, svalor) != ERROR_NO_MORE_ITEMS; j++)
			}
			RegCloseKey(hks);
			if (k)
				break;
		}
	}
	RegCloseKey(hk);
	if (!k)
		return "text/plain";
	return tmp;*/
#else
	FILE *fp;
	char tmp[BUFSIZE], *tp, *e, *c;
	static char tipo[128];
	if (!ext)
		return "text/plain";
	if (!strncmp(ext, "php", 3))
		return "text/html";
	if ((fp = fopen("/etc/mime.types", "r")))
	{
		while (fgets(tmp, sizeof(tmp), fp))
		{
			if ((c = strchr(tmp, '\n')))
				*c = '\0';
			tp = strtok(tmp, "\t");
			while ((e = strtok(NULL, " ")))
			{
				while (*e == '\t')
					e++;
				if (!strcmp(e, ext))
				{
					strlcpy(tipo, tp, sizeof(tipo));
					fclose(fp);
					return tipo;
				}
			}
		}
	}
	return "text/plain";
#endif
}
void LiberaHHead(HHead *hh)
{
	httpcons[hh->slot] = NULL;
	ircfree(hh->host);
	ircfree(hh->user_agent);
	ircfree(hh->referer);
	ircfree(hh->ruta);
	ircfree(hh->archivo);
	ircfree(hh->param_get);
	ircfree(hh->param_post);
	ircfree(hh->ext);
	ircfree(hh);
}
void EnviaRespuesta(HHead *hh, u_int num, time_t lmod, char *errmsg, u_long bytes, char *data)
{
	char tbuf[2048], timebuf[128];
	char *msg;
	time_t ahora;
	if (!hh->sck)
	{
		LiberaHHead(hh);
		return;
	}
	if (!bytes && data)
		bytes = strlen(data);
	if (!errmsg && !(errmsg = BuscaOptItem(num, herrores)))
		errmsg = "Desconocido";
	ahora = time(0);
	strftime(timebuf, sizeof(timebuf), fecha_fmt, gmtime(&ahora));
	msg = "HTTP/1.1 %u %s\r\n"
		"Server: %s\r\n"
		"Date: %s\r\n";
	ircsprintf(tbuf, msg, num, errmsg, COLOSSUS_VERSION, timebuf);
	if (num != 304)
	{
		if (hh->ext && !strncasecmp(hh->ext, "php", 3))
		{
			msg = "Content-Length: %lu\r\n";
			sprintf(buf, msg, bytes);
		}
		else
		{
			msg = "Content-Length: %lu\r\n"
				"Content-Type: %s\r\n";
			sprintf(buf, msg, bytes, BuscaTipo(hh->ext));
		}
		strlcat(tbuf, buf, sizeof(tbuf));
		if (conf_httpd->max_age < 0 || (num != 200 && num != 300) || (hh->ext && strncasecmp(hh->ext, "htm", 3)))
			strlcat(tbuf, "Cache-Control: no-cache,no-store\r\n", sizeof(tbuf));
		else
		{
			time_t expira = ahora + conf_httpd->max_age;
			strftime(timebuf, sizeof(timebuf), fecha_fmt, gmtime(&expira));
			msg = "Cache-Control: max-age=%i\r\n"
				"Expires: %s\r\n";
			ircsprintf(buf, msg, conf_httpd->max_age, timebuf);
			strlcat(tbuf, buf, sizeof(tbuf));
			if (lmod >= 0)
			{
				strftime(timebuf, sizeof(timebuf), fecha_fmt, gmtime(&lmod));
				ircsprintf(buf, "Last-Modified: %s\r\n", timebuf);
				strlcat(tbuf, buf, sizeof(tbuf));
			}
		}
	}
	//strlcat(tbuf, "\r\n", sizeof(tbuf));
	SockWrite(hh->sck, tbuf);
	if (data && bytes)
		SockWriteBin(hh->sck, bytes, data);
	SockClose(hh->sck);
}
void EnviaError(HHead *hh, u_int num, char *texto)
{
	char tbuf[2048];
	char *msg = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
			"<html><head>\n"
			"<title>%u %s</title>\n"
			"</head><body>\n"
			"<h1>%s</h1>\n"
			"<p>%s</p>\n"
			"<hr>\n"
			"<address>Servidor %s en %s Puerto %i</address>\n"
			"</body></html>\n", *errmsg;
	if (!(errmsg = BuscaOptItem(num, herrores)))
		errmsg = "Desconocido";
	ircsprintf(tbuf, msg, num, errmsg, errmsg, texto, COLOSSUS_VERSION, conf_httpd->url, conf_httpd->puerto);
	EnviaRespuesta(hh, num, -1, errmsg, 0, tbuf);
}
HDir *CreaHDir(char *ruta, HDIRFUNC(*func))
{
	HDir *hd;
	if (!ruta || !func)
		return NULL;
	hd = BMalloc(HDir);
	hd->ruta = strdup(ruta);
	hd->func = func;
	AddItem(hd, hdirs);
	return hd;
}
int BorraHDir(HDir *hd)
{
	ircfree(hd->ruta);
	if (LiberaItem(hd, hdirs))
		return 1;
	return 0;
}
HHead *BuscaHHead(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_CON; i++)
	{
		if (httpcons[i] && httpcons[i]->sck == sck)
			return httpcons[i];
	}
	return NULL;
}
SOCKFUNC(AbreHTTPD)
{
	int i;
	for (i = 0; i < MAX_CON; i++)
	{
		if (!httpcons[i])
		{
			HHead *hh;
			hh = BMalloc(HHead);
			hh->sck = sck;
			hh->lmod = 0;
			hh->slot = i;
			httpcons[i] = hh;
			return 0;
		}
	}
	SockClose(sck);
	return 1;
}
int EPhp(u_long len, char *res, HHead *hh)
{
	hh->noclosesock = 0;
	EnviaRespuesta(hh, 200, -1, NULL, len, res);
	Free(res);
	//SockClose(hh->sck);
	return 0;
}
void ProcesaHHead(HHead *hh, Sock *sck)
{
	HDir *hd;
	int num = -1;
	char *errmsg = NULL, *data = NULL;
	for (hd = hdirs; hd; hd = hd->sig)
	{
		if (hh->ruta && !strncmp(hh->ruta, hd->ruta, strlen(hd->ruta)))
		{
			num = hd->func(hh, hd, &errmsg, &data);
			break;
		}
	}
	switch (num)
	{
		case -1:
			EnviaError(hh, 404, "La página que solicita no existe.");
			break;
		case 200:
			if (!data)
			{
				u_long len = 0;
				char *p = NULL, f[PMAX], servars[BUFSIZE];
				ircsprintf(f, "%s%s/%s", lpath, hh->ruta, hh->archivo);
				if (!EsArchivo(f))
				{
					EnviaError(hh, 404, "La página que solicita no existe.");
					break;
				}
				if (conf_httpd->php && !strncasecmp(hh->ext, "php", 3))
				{
					ircsprintf(servars, "REMOTE_ADDR=%s", sck->host);
					ircsprintf(buf, "-f \"%s\" \"%s\" \"%s\" \"%s\"", f, hh->param_get ? hh->param_get : "NULL", hh->param_post ? hh->param_post : "NULL", servars);
					if (hh->asynch)
					{
						hh->noclosesock = 1; /* no cerramos el sock hasta que no recibamos respuesta */
						EjecutaComandoASinc(conf_httpd->php, buf, (ECmdFunc) EPhp, hh);
					}
					else
					{
						EjecutaComandoSinc(conf_httpd->php, buf, &len, &p);
						EnviaRespuesta(hh, 200, time(0), NULL, len, p);
						Free(p);
					}
				}
				else
				{
#ifdef _WIN32
					HANDLE fd, mp;
					ULARGE_INTEGER fullTime;
					FILETIME cTime;
					if ((fd = CreateFile(f, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
#else
					int fd;
					struct stat sb;
					if ((fd = open(f, O_RDONLY)) == -1)
#endif
					{
						EnviaError(hh, 500, "Ha ocurrido un error inesperado (nofd).");
						break;
					}
#ifdef _WIN32
					GetFileTime(fd, NULL, NULL, &cTime);
					fullTime.LowPart = cTime.dwLowDateTime;
					fullTime.HighPart = cTime.dwHighDateTime;
					fullTime.QuadPart -= 116444736000000000;
					fullTime.QuadPart /= 10000000;
					if (fullTime.LowPart <= (u_long)hh->lmod)
					{
						CloseHandle(fd);
						EnviaRespuesta(hh, 304, -1, NULL, 0, NULL);
						break;
					}
					len = GetFileSize(fd, NULL);
					if (!(mp = CreateFileMapping(fd, NULL, PAGE_READONLY|SEC_COMMIT, 0, len, NULL)))
					{
						EnviaError(hh, 500, "Ha ocurrido un error inesperado (cfm).");
						CloseHandle(fd);
						break;
					}
					if (!(p = MapViewOfFile(mp, FILE_MAP_READ, 0, 0, 0)))
					{
						EnviaError(hh, 500, "Ha ocurrido un error inesperado (mvof).");
						CloseHandle(mp);
						CloseHandle(fd);
						break;
					}
					EnviaRespuesta(hh, 200, fullTime.LowPart, NULL, len, p);
					UnmapViewOfFile(p);
					CloseHandle(mp);
					CloseHandle(fd);
#else	
					if (fstat(fd, &sb) == -1)
					{
						close(fd);
						EnviaError(hh, 500, "Ha ocurrido un error inesperado (fs).");
						break;
					}
					if (sb.st_mtime <= hh->lmod)
					{
						close(fd);
						EnviaRespuesta(hh, 304, -1, NULL, 0, NULL);
						break;
					}
					len = sb.st_size;
					if (!(p = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0)))
					{
						close(fd);
						EnviaError(hh, 500, "Ha ocurrido un error inesperado (mm).");
						break;
					}
					EnviaRespuesta(hh, 200, sb.st_mtime, NULL, len, p);
					munmap(p, len);
					close(fd);
#endif	
				}
			}
			break;
		default:
			EnviaError(hh, num, errmsg);
	}
}
SOCKFUNC(LeeHTTPD)
{
	HHead *hh;
	char *c = data, *d, *e;
	if (!(hh = BuscaHHead(sck)))
		return 1;
#ifndef DEBUG
	Debug("[Web: %s]", c);
#endif
	if (!hh->viene_post)
	{
		while (!BadPtr(c))
		{
			if ((e = strchr(c, '\n')))
			{
				if (e == c || e == c+1)
					goto fin;
				*e = '\0';
				if (*(e-1) == '\r')
					*(e-1) = '\0';
			}
			if (!e)
				break;
			if (!hh->metodo)
			{
				if (!strncmp(c, "GET", 3))
					hh->metodo = HTTP_GET;
				else if (!strncmp(c, "POST", 4))
					hh->metodo = HTTP_POST;
				else if (!strncmp(c, "HEAD", 4))
					hh->metodo = HTTP_HEAD;
				else
					hh->metodo = HTTP_DESC;
				if (!(c = strchr(c, ' ')))
					break;
				c++;
				if ((d = strchr(c, ' ')))
					*d = '\0';
				if ((d = strchr(c, '?')))
				{
					*d = '\0';
					ircstrdup(hh->param_get, d+1);
				}
				if ((d = strrchr(c, '/')))
				{
					*d = '\0';
					ircstrdup(hh->archivo, d+1);
					if ((d = strrchr(d+1, '.')))
					{
						d++;
						hh->ext = strdup(d);
					}
				}
				ircstrdup(hh->ruta, c);
			}
			else if (!strncmp(c, "Host:", 5))
				ircstrdup(hh->host, c+6);
			else if (!strncmp(c, "Referer:", 8))
				ircstrdup(hh->referer, c+9);
			else if (!strncmp(c, "If-Modified-Since:", 18))
			{
				u_int dia, anyo, hora, min, seg;
				char mes[4], wdia[4];
				struct tm ttm;
				sscanf(c+19, fecha_sfmt, wdia, &dia, mes, &anyo, &hora, &min, &seg);
				wdia[3] = '\0';
				ttm.tm_sec = seg;
				ttm.tm_min = min;
#ifdef _WIN32
				ttm.tm_hour = hora-_timezone/3600;
#else
				ttm.tm_hour = hora;
#endif
				ttm.tm_mday = dia;
				ttm.tm_mon = BuscaMes(mes);
				ttm.tm_year = anyo-1900;
				ttm.tm_isdst = 0;
#ifdef _WIN32
				hh->lmod = mktime(&ttm);
#else
				hh->lmod = timegm(&ttm);
#endif
				strftime(buf, sizeof(buf), fecha_fmt, &ttm);
			}
			c = e+1;
			fin:
			if (*c == '\n' || *c == '\r')
			{
				if (*c == '\r')
					c++;
				if (*c == '\n')
					c++;
				hh->viene_post = 1;
				break;
			}
		}
	}
	if (!BadPtr(c))
		ircstrdup(hh->param_post, c);
	if ((hh->viene_post && hh->metodo != HTTP_POST) || (hh->metodo == HTTP_POST && hh->param_post))
	{
		if ((hh->param_post && strstr(hh->param_post, "__ASYNCH__=1")) || (hh->param_get && strstr(hh->param_get, "__ASYNCH__=1")))
			hh->asynch = 1;
		ProcesaHHead(hh, sck); 
	}
	return 0;
}
SOCKFUNC(CierraHTTPD)
{
	HHead *hh;
	if ((hh = BuscaHHead(sck)))
	{
		hh->sck = NULL;
		if (!hh->noclosesock)
			LiberaHHead(hh);
	}
	return 0;
}
int IniciaHTTPD()
{
	int i;
	char *c, *d = lpath;
	for (i = 0; i < MAX_CON; i++)
		httpcons[i] = NULL;
	if (!(listen_httpd = SockListenEx(conf_httpd->puerto, AbreHTTPD, LeeHTTPD, NULL, CierraHTTPD, OPT_NORECVQ)))
		return 1;
	for (c = SPATH; !BadPtr(c); c++)
	{
		if (*c == '\\')
			*d++ = '/';
		else
			*d++ = *c;
	}
	*d = '\0';
	strlcat(d, "/html", sizeof(lpath));
	return 0;
}
int DetieneHTTPD()
{
	int i;
	for (i = 0; i < MAX_CON; i++)
	{
		if (httpcons[i])
			SockClose(httpcons[i]->sck);
	}
	if (listen_httpd)
		SockClose(listen_httpd);
	return 0;
}
