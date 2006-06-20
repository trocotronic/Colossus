/*
 * $Id: httpd.c,v 1.4 2006-06-20 13:46:10 Trocotronic Exp $ 
 */
#ifdef _WIN32
#include "struct.h"
#include "httpd.h"
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

Sock *listen_httpd;
HHead *httpcons[MAX_CON];
HDir *hdirs = NULL;
char *fecha_fmt = "%a, %d %b %Y %H:%M:%S GMT";
char *fecha_sfmt = "%s %u %s %u %u:%u:%u GMT";

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
char *htipos[] = {
	"htm" , "text/html" ,
	"php" , "text/html" ,
	"jpg" , "image/jpeg" ,
	"css" , "text/css" ,
	NULL
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
	int i;
	if (BadPtr(ext))
		return "text/plain";
	for (i = 0; htipos[i]; i+= 2)
	{
		if (!strncasecmp(htipos[i], ext, strlen(htipos[i])))
			return htipos[i+1];
	}
	return "text/plain";
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
		msg = "Content-Length: %lu\r\n"
			"Content-Type: %s\r\n";
		sprintf(buf, msg, bytes, BuscaTipo(hh->ext));
		strcat(tbuf, buf);
		if (conf_httpd->max_age < 0 || (num != 200 && num != 300) || (hh->ext && !strncasecmp(hh->ext, "php", 3)))
			strcat(tbuf, "Cache-Control: no-cache,no-store\r\n");
		else
		{
			time_t expira = ahora + conf_httpd->max_age;
			strftime(timebuf, sizeof(timebuf), fecha_fmt, gmtime(&expira));
			msg = "Cache-Control: max-age=%i\r\n"
				"Expires: %s\r\n";
			ircsprintf(buf, msg, conf_httpd->max_age, timebuf);
			strcat(tbuf, buf);
			if (lmod >= 0)
			{
				strftime(timebuf, sizeof(timebuf), fecha_fmt, gmtime(&lmod));
				ircsprintf(buf, "Last-Modified: %s\r\n", timebuf);
				strcat(tbuf, buf);
			}
		}
	}
	strcat(tbuf, "\r\n");
	SockWrite(hh->sck, tbuf);
	if (hh->sck)
	{
		if (data)
			SockWriteBin(hh->sck, bytes, data);
		SockClose(hh->sck, LOCAL);
	}
	else
		LiberaHHead(hh);
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
			"</body></html>", *errmsg;
	if (!(errmsg = BuscaOptItem(num, herrores)))
		errmsg = "Desconocido";
	ircsprintf(tbuf, msg, num, errmsg, errmsg, texto, COLOSSUS_VERSION, conf_httpd->url, conf_httpd->puerto);
	EnviaRespuesta(hh, num, -1, errmsg, 0, tbuf);
}
HDir *CreaHDir(char *carpeta, char *ruta, HDIRFUNC(*func))
{
	HDir *hd;
	if (!carpeta || !ruta || !func)
		return NULL;
	BMalloc(hd, HDir);
	hd->ruta = strdup(ruta);
	hd->carpeta = strdup(carpeta);
	hd->func = func;
	AddItem(hd, hdirs);
	return hd;
}
int BorraHDir(HDir *hd)
{
	ircfree(hd->ruta);
	ircfree(hd->carpeta);
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
			BMalloc(hh, HHead);
			hh->sck = sck;
			hh->lmod = 0;
			hh->slot = i;
			httpcons[i] = hh;
			return 0;
		}
	}
	return 1;
}
int EPhp(u_long len, char *res, HHead *hh)
{
	EnviaRespuesta(hh, 200, -1, NULL, len, res);
	return 0;
}
void ProcesaHHead(HHead *hh, Sock *sck)
{
	HDir *hd;
	int num = -1;
	char *errmsg = NULL, *data = NULL;
	for (hd = hdirs; hd; hd = hd->sig)
	{
		if (!strcmp(hh->ruta, hd->ruta))
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
				ircsprintf(f, "%s%s/%s", hd->carpeta, hd->ruta, hh->archivo);
				ircsprintf(servars, "REMOTE_ADDR=%s", sck->host);
				ircsprintf(buf, "-f %s %s %s %s", f, hh->param_get ? hh->param_get : "NULL", hh->param_post ? hh->param_post : "NULL", servars);
				if (!EsArchivo(f))
				{
					EnviaError(hh, 404, "La página que solicita no existe.");
					break;
				}
				if (!strncasecmp(hh->ext, "php", 3))
				{
					hh->noclosesock = 1; /* no cerramos el sock hasta que no recibamos respuesta */
					EjecutaComandoASinc("php", buf, EPhp, hh);
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
	Debug("[Web: %s", c);
	while (!BadPtr(c))
	{
		if ((e = strchr(c, '\r')))
			*e = '\0';
		else
		{
			if ((e = strchr(c, '\n')))
				*e = '\0';
		}
		if (!e)
			break;
		if (!hh->metodo)
		{
			if (!(c = strchr(c, ' ')))
				break;
			c++;
			if (!strncmp(data, "GET", 3))
				hh->metodo = HTTP_GET;
			else if (!strncmp(data, "POST", 4))
				hh->metodo = HTTP_POST;
			else if (!strncmp(data, "HEAD", 4))
				hh->metodo = HTTP_HEAD;
			else
				hh->metodo = HTTP_DESC;
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
			ircstrdup(hh->host, data+6);
		else if (!strncmp(c, "Referer:", 8))
			ircstrdup(hh->referer, data+9);
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
		if (*(c = e+1) == '\n')
			c++;
	}
	if (!BadPtr(c))
		ircstrdup(hh->param_post, c);
	ProcesaHHead(hh, sck); 
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
	for (i = 0; i < MAX_CON; i++)
		httpcons[i] = NULL;
	if (!(listen_httpd = SockListenEx(conf_httpd->puerto, AbreHTTPD, LeeHTTPD, NULL, CierraHTTPD, OPT_NORECVQ)))
		return 1;
	return 0;
}
int DetieneHTTPD()
{
	int i;
	for (i = 0; i < MAX_CON; i++)
	{
		if (httpcons[i])
			SockClose(httpcons[i]->sck, LOCAL);
	}
	if (listen_httpd)
		SockClose(listen_httpd, LOCAL);
	return 0;
}
#endif
