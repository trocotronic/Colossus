/*
 * $Id: main.c,v 1.2 2004-09-11 16:08:03 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include <fcntl.h>
#include <sys/stat.h>
#undef USA_CONSOLA
#ifndef _WIN32
#include <errno.h>
#include <utime.h>
#include <sys/resource.h>
#endif
#define BadPtr(x) (!(x) || (*(x) == '\0'))

static char buf[BUFSIZE];
static char bufr[BUFSIZE+1];

struct Sockets
{
	Sock *socket[MAXSOCKS];
	int abiertos;
}ListaSocks;

Sock *SockActual;
MYSQL *mysql = NULL;
Signal *signals[MAXSIGS];
Timer *timers = NULL;
Proc *procs = NULL;
char tokbuf[BUFSIZE];
#ifdef USA_CONSOLA
char conbuf[128];
HANDLE hStdin;
#endif
int listens[MAX_LISTN];
int listenS = 0;
char reth = 0;

void encola(DBuf *, char *, int);
int desencola(DBuf *, char *, int);
void envia_cola(Sock *);
char *lee_cola(Sock *);
int carga_cache(void);

#ifdef USA_CONSOLA
void *consola_loop_principal(void *);
void parsea_comando(char *);
#endif
void programa_loop_principal(void *);

const char NTL_tolower_tab[] = {
       /* x00-x07 */ '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
       /* x08-x0f */ '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
       /* x10-x17 */ '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
       /* x18-x1f */ '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
       /* ' '-x27 */    ' ',    '!',    '"',    '#',    '$',    '%',    '&', '\x27',
       /* '('-'/' */    '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
       /* '0'-'7' */    '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
       /* '8'-'?' */    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
       /* '@'-'G' */    '@',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
       /* 'H'-'O' */    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
       /* 'P'-'W' */    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
       /* 'X'-'_' */    'x',    'y',    'z',    '{',    '|',    '}',    '~',    '_',
       /* '`'-'g' */    '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
       /* 'h'-'o' */    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
       /* 'p'-'w' */    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
       /* 'x'-x7f */    'x',    'y',    'z',    '{',    '|',    '}',    '~', '\x7f'
};
const char NTL_toupper_tab[] = {
       /* x00-x07 */ '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
       /* x08-x0f */ '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
       /* x10-x17 */ '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
       /* x18-x1f */ '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
       /* ' '-x27 */    ' ',    '!',    '"',    '#',    '$',    '%',    '&', '\x27',
       /* '('-'/' */    '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
       /* '0'-'7' */    '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
       /* '8'-'?' */    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
       /* '@'-'G' */    '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
       /* 'H'-'O' */    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
       /* 'P'-'W' */    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
       /* 'X'-'_' */    'X',    'Y',    'Z',    '[', '\x5c',    ']',    '^',    '_',
       /* '`'-'g' */    '`',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
       /* 'h'-'o' */    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
       /* 'p'-'w' */    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
       /* 'x'-x7f */    'X',    'Y',    'Z',    '[', '\x5c',    ']',    '^', '\x7f'
};
#ifdef USA_CONSOLA
const char logo[] = {
	32 , 32 , 95 , 95 , 95 , 95 , 95 , 32 , 32 , 32 , 32 , 32 , 32 , 95 , 10 ,
	32 , 47 , 95 , 95 , 95 , 95 , 95 , 124 , 32 , 32 , 32 , 32 , 124 , 32 , 124 , 10 ,
	124 , 32 , 124 , 32 , 32 , 32 , 32 , 32 , 95 , 95 , 95 , 32 , 124 , 32 , 124 , 32 , 95 , 95 , 95 , 32 , 32 , 95 , 95 , 95 , 32 , 95 , 95 , 95 , 32 , 95 , 32 , 32 , 32 , 95 , 32 , 95 , 95 , 95 , 10 ,
	124 , 32 , 124 , 32 , 32 , 32 , 32 , 47 , 32 , 95 , 32 , 92 , 124 , 32 , 124 , 47 , 32 , 95 , 32 , 92 , 47 , 32 , 95 , 95 , 47 , 32 , 95 , 95 , 124 , 32 , 124 , 32 , 124 , 32 , 47 , 32 , 95 , 95 , 124 , 10 ,
	124 , 32 , 124 , 95 , 95 , 95 , 124 , 32 , 40 , 95 , 41 , 32 , 124 , 32 , 124 , 32 , 40 , 95 , 41 , 32 , 92 , 95 , 95 , 32 , 92 , 95 , 95 , 32 , 92 , 32 , 124 , 95 , 124 , 32 , 92 , 95 , 95 , 32 , 92 , 10 ,
	32 , 92 , 95 , 95 , 95 , 95 , 95 , 92 , 95 , 95 , 95 , 47 , 124 , 95 , 124 , 92 , 95 , 95 , 95 , 47 , 124 , 95 , 95 , 95 , 47 , 95 , 95 , 95 , 47 , 92 , 95 , 95 , 44 , 95 , 124 , 95 , 95 , 95 , 47 , 10 ,
	0 
};
#endif

int strcasecmp(const char *a, const char *b)
{
	const char* ra = a;
	const char* rb = b;
	while (ToLower(*ra) == ToLower(*rb)) 
	{
		if (!*ra++)
			return 0;
		else
			++rb;
	}
	return (*ra - *rb);
}
void ircstrdup(char **dest, const char *orig)
{
	ircfree(*dest);
	if (!orig)
		*dest = NULL;
	else
		*dest = strdup(orig);
}
/* 
 * print_r
 * Imprime una estructura de forma legible
 */
void print_r(Conf *conf, int escapes)
{
	int i;
	char tabs[32];
	tabs[0] = '\0';
	for (i = 0; i < escapes; i++)
		strcat(tabs, "\t");
	conferror("%s%s%s%s%s%s", tabs, conf->item, conf->data ? " \"" : "", conf->data ? conf->data : "", conf->data ? "\"" : "", conf->secciones ? " {" : ";");
	if (conf->secciones)
		escapes++;
	for (i = 0; i < conf->secciones; i++)
		print_r(conf->seccion[i], escapes);
	if (conf->secciones)
	{
		escapes--;
		tabs[0] = '\0';
		for (i = 0; i < escapes; i++)
			strcat(tabs, "\t");
		conferror("%s};", tabs);
	}
}
/*
 * resolv
 * Resvuelve el host dado
 * NULL si no se pudo resolver
 */
struct in_addr *resolv(char *host)
{
	struct hostent *he;
	if (!host || !host[0]) 
		return NULL;
	if (!(he = gethostbyname(host))) 
		return NULL;
	else 
		return (struct in_addr *)he->h_addr;
}
int sockblock(int pres)
{
#ifdef _WIN32
	int bl = 1;
	return ioctlsocket(pres, FIONBIO, &bl);
#else
	int res, nonb = 0;
	if ((res = fcntl(pres, F_GETFL, 0)) >= 0)
		return fcntl(pres, F_SETFL, res | nonb);
	return -1;
#endif
}
void inserta_sock(Sock *sck)
{
	ListaSocks.socket[ListaSocks.abiertos] = sck;
#ifdef DEBUG
	Debug("Insertando sock %X %i en %i ", sck, sck->pres, ListaSocks.abiertos);
#endif
	ListaSocks.abiertos++;
}	
/*
 * sockopen
 * Abre una conexión
 * Devuelve el puntero al socket abierto
 */
Sock *sockopen(char *host, int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc), int add)
{
	Sock *sck;
	struct in_addr *res;
	if (add && ListaSocks.abiertos == MAXSOCKS)
		return NULL;
	da_Malloc(sck, Sock);
	SockDesc(sck);
	if ((sck->pres = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return NULL;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = htons((u_short)puerto);
	if ((res = resolv(host)))
		sck->server.sin_addr = *res;
	sck->host = strdup(host);
	SockConn(sck);
	if (sockblock(sck->pres) == -1)
		return NULL;
#ifdef _WIN32
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) == -1 &&
		WSAGetLastError() != WSAEINPROGRESS &&
		WSAGetLastError() != WSAEWOULDBLOCK)
#else
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) < 0 && errno != EINPROGRESS)
#endif			
			return NULL;
	da_Malloc(sck->recvQ, DBuf);
	da_Malloc(sck->sendQ, DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	sck->puerto = puerto;
#ifdef DEBUG
	Debug("Abriendo conexion con %s:%i (%i)", host, puerto, sck->pres);
#endif
	if (add)
		inserta_sock(sck);
	return sck;
}
Sock *socklisten(int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc))
{
	Sock *sck;
	int i;
	int ad[4];
	char ipname[20], *name = "*";
	for (i = 0; i < listenS; i++)
	{
		if (listens[i] == puerto)
			return NULL;
	}
	if (ListaSocks.abiertos == MAXSOCKS)
		return NULL;
	ad[0] = ad[1] = ad[2] = ad[3] = 0;
	(void)sscanf(name, "%d.%d.%d.%d", &ad[0], &ad[1], &ad[2], &ad[3]);
	(void)sprintf_irc(ipname, "%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
	da_Malloc(sck, Sock);
	if ((sck->pres = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return NULL;
	sck->server.sin_family = AF_INET;
	sck->server.sin_addr.s_addr = inet_addr(ipname);
	sck->server.sin_port = htons((u_short)puerto);
	sck->puerto = puerto;
	sck->host = strdup("127.0.0.1");
	if (bind(sck->pres, (struct sockaddr *)&sck->server,
		    sizeof(sck->server)) == -1)
		return NULL;
	SockList(sck);
	if (sockblock(sck->pres) == -1)
		return NULL;
#ifdef DEBUG
	Debug("Escuchando puerto %i", puerto);
#endif
	da_Malloc(sck->recvQ, DBuf);
	da_Malloc(sck->sendQ, DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	listen(sck->pres, 5);
	listens[listenS++] = puerto;
	inserta_sock(sck);
	return sck;
}
Sock *sockaccept(Sock *list, int pres)
{
	Sock *sck;
	struct sockaddr_in addr;
	struct in_addr *res;
	char *host;
	int len = sizeof(struct sockaddr);
	if (getpeername(pres, (struct sockaddr *)&addr, &len) == -1)
	{
#ifdef _WIN32
		closesocket(pres);
#else
		close(pres);
#endif		
		return NULL;
	}
	da_Malloc(sck, Sock);
	sck->pres = pres;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = list->server.sin_port;
	host = inet_ntoa(addr.sin_addr);
	if ((res = resolv(host)))
		sck->server.sin_addr = *res;
	sck->host = strdup(host);
	sck->puerto = list->puerto;
	SockOk(sck);
	if (sockblock(sck->pres) == -1)
		return NULL;
	da_Malloc(sck->recvQ, DBuf);
	da_Malloc(sck->sendQ, DBuf);
	sck->openfunc = list->openfunc;
	sck->readfunc = list->readfunc;
	sck->writefunc = list->writefunc;
	sck->closefunc = list->closefunc;
	inserta_sock(sck);
	if (sck->openfunc)
		sck->openfunc(sck, NULL);
	ircd_log(LOG_CONN, "Conexión establecida con %s:%i", sck->host, sck->puerto);
	return sck;
}
void sockwritev(Sock *sck, int opts, char *formato, va_list vl)
{
	char buf[BUFSIZE], *msg;
#ifdef ZIP_LINKS	
	int len;
#endif
	if (!sck)
		return;
	msg = buf;
	vsprintf_irc(buf, formato, vl);
#ifdef DEBUG
	Debug("[Enviando: %s]", buf);
#endif
	if (opts & OPT_CR)
		strcat(buf, "\r");
	if (opts & OPT_LF)
		strcat(buf, "\n");
#ifdef ZIP_LINKS
	len = strlen(buf);
	if (IsZip(sck))
		msg = zip_buffer(sck, msg, &len, 0);
	if (len)
#endif
	encola(sck->sendQ, msg, 0); /* de momento siempre son cadenas */
}
void sockwrite(Sock *sck, int opts, char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	sockwritev(sck, opts, formato, vl);
	va_end(vl);
}
void sockclose(Sock *sck, char closef)
{
	int i;
	if (!sck)
		return;
	if (EsList(sck))
	{
		int i;
		for (i = 0; i < listenS; i++)
		{
			if (listens[i] == sck->puerto)
			{
				for (; i < listenS; i++)
					listens[i] = listens[i+1];
				listenS--;
				break;
			}
		}
	}
	SockCerr(sck);
#ifdef DEBUG
	Debug("Cerrando conexion con %s:%i (%i)", sck->host, sck->puerto, sck-pres);
#endif
	if (sck->closefunc)
		sck->closefunc(sck, closef ? NULL : "LOCAL");
#ifdef _WIN32
	closesocket(sck->pres);
#else
	close(sck->pres);
#endif
	for (i = 0; i < ListaSocks.abiertos; i++)
	{
		if (ListaSocks.socket[i] == sck)
		{
			for (; i < ListaSocks.abiertos; i++)
				ListaSocks.socket[i] = ListaSocks.socket[i+1];
			ListaSocks.abiertos--;
			break;
		}
	}
	bzero(bufr, BUFSIZE+1);
	ircfree(sck->recvQ);
	ircfree(sck->sendQ);
	ircfree(sck->host);
#ifdef ZIP_LINKS
	if (sck->zip)
		zip_free(sck);
#endif
	Free(sck);
}
void cierra_socks()
{
	while (ListaSocks.abiertos)
		sockclose(ListaSocks.socket[0], LOCAL);
}
void encola(DBuf *bufc, char *str, int bytes)
{
	int copiados, len = bytes ? bytes : strlen(str);
	//Debug("Len %i (%i %i %X)",len,bufc->len,bufc->wslot ? bufc->wslot->len : 0,bufc->wslot);
	if (!bufc->slots)
	{
		da_Malloc(bufc->wslot, DbufData);
		bufc->rslot = bufc->wslot;
		bufc->rchar = &bufc->rslot->data[0];
		bufc->wchar = &bufc->wslot->data[0];
		//Debug("Abriendo slot 0");
		bufc->slots++;
	}
	do
	{
		copiados = (len + bufc->wslot->len < sizeof(bufc->wslot->data) ? len : sizeof(bufc->wslot->data) - bufc->wslot->len);
		//Debug("Copiados %i",copiados);
		memcpy(bufc->wchar, str, copiados);
		str += copiados;
		bufc->wchar += copiados;
		len -= copiados;
		bufc->wslot->len += copiados;
		bufc->len += copiados;
		if (bufc->wslot->len == sizeof(bufc->wslot->data)) /* está lleno, abrimos otro */
		{
			DbufData *aux;
			da_Malloc(aux, DbufData);
			aux->prev = bufc->wslot;
			bufc->wslot->sig = aux;
			bufc->wslot = aux;
			bufc->wchar = &bufc->wslot->data[0];
			//Debug("Abriendo slot %i", bufc->slots);
			bufc->slots++;
		}
	}while (len > 0);
}	
int desencola(DBuf *bufc, char *buf, int bytes)
{
	/* bytes contiene el máximo que podemos copiar */
	DbufData *aux;
	int curpos = 0, copiados, resto;
	//Debug("Dlen %i",bytes);
	if (!bufc->slots)
		return 0;
	do
	{
		aux = bufc->rslot->sig;
		resto = bufc->rslot->len; /* lo que queda por copiar en este slot */
		copiados = resto < (bytes - curpos) ? resto : (bytes - curpos);
		//Debug("DCopiados %i %p %p",copiados, bufc->rchar,&bufc->rslot->data[0]);
		memcpy(buf + curpos, bufc->rchar, copiados);
		curpos += copiados;
		bufc->len -= copiados;
		bufc->rslot->len -= copiados;
		if (bufc->rslot->len <= 0) /* lo podemos copiar todo */
		{
			bufc->rchar = &aux->data[0]; /* al siguiente slot */
			Free(bufc->rslot);
			//Debug("Adios %X %X", bufc->rslot,aux);
			if (!(bufc->rslot = aux))
				break;
		}
		else
			bufc->rchar += copiados;
	}while(curpos < bytes);
	if (bufc->len <= 0) /* todo el bufer copiado */
	{
		bufc->len = 0;
		bufc->wslot = bufc->rslot = NULL;
		bufc->slots = 0;
	}
	//Debug("Liberando %i %i",bufc->len,curpos);
	return bufc->len;
}
void envia_cola(Sock *sck)
{
	char *msg;
	int len = 0;
#ifdef ZIP_LINKS
 	int mas = 0;
 	if (IsZip(sck) && sck->zip->outcount)
	{
		if (sck->sendQ->len > 0)
			mas = 1;
		else
		{
			msg = zip_buffer(sck, NULL, &len, 1);
			encola(sck->sendQ, msg, len);
		}
	}
#endif
	if (sck->sendQ->len > 0)
	{
		int quedan;
		msg = bufr;
		do 
		{
			quedan = desencola(sck->sendQ, msg, sizeof(bufr));
			if (BadPtr(msg))
				return;
			if (!len)
				len = strlen(msg);
			send(sck->pres, msg, len, 0);
			len = 0;
			if (sck->writefunc)
				sck->writefunc(sck, msg);
			bzero(msg, sizeof(bufr)); /* hay que vaciarlo!! */
		}while(quedan);
	}
#ifdef ZIP_LINKS
	if (sck->sendQ->len == 0 && mas)
	{
		mas = 0;
		msg = zip_buffer(sck, NULL, &len, 1);
		//Debug("Encolando 3 %s",msg ? msg : "(NULL)");
		encola(sck->sendQ, msg, len);
	}
#endif
}
int lee_mensaje(Sock *sck)
{
	char lee[BUFSIZE], *msg;
	int len;
	msg = lee;
	bzero(lee, BUFSIZE);
	len = recv(sck->pres, lee, BUFSIZE, 0);
	if (len > 0)
		encola(sck->recvQ, lee, len);
	return len;
}
void crea_mensaje(Sock *sck, int len)
{
	char *recv, *ini, *ul;
	int quedan, off;
#ifdef ZIP_LINKS
	int done_unzip = 0, zipped = 0;
#endif
	recv = bufr;
	off = 0;
	do
	{
		quedan = desencola(sck->recvQ, recv + off, BUFSIZE - off);
		recv[BUFSIZE] = '\0';
		//Debug("*** recv %i %i",quedan,strlen(recv));
#ifdef ZIP_LINKS
		if (IsZipStart(sck))
		{
			if (*recv == '\n' || *recv == '\r')
			{
				recv++;
		 		len--;
			}
			sck->zip->first = 0;
		}
		else
			done_unzip = 1;
		if (IsZip(sck))
		{
			zipped = len;
			sck->zip->inbuf[0] = '\0';    /* unnecessary but nicer for debugging */
			sck->zip->incount = 0;
			recv = unzip_packet(sck, recv, &zipped);
			len = zipped;
			zipped = 1;
			if (len <= 0)
				Debug("ARG");
		}
		do
		{
#endif		
		for (ini = ul = recv; !BadPtr(ul); ul++)
		{
			if (*ul == '\n')
			{
				*ul = '\0';
				if (*(ul - 1) == '\r')
					*(ul - 1) = '\0';
#ifdef ZIP_LINKS
				if (IsZip(sck) && (zipped == 0) && (len > 0))
				{
					zipped = len;
					if (zipped > 0 && (*recv == '\n' || *recv == '\r'))
					{
						recv++;
						zipped--;
					}
					sck->zip->first = 0;
					recv = unzip_packet(sck, recv, &zipped);
					len = zipped;
					zipped = 1;
					if (len == -1)
						Debug("ARG 2");
				}
#endif
#ifdef DEBUG
				Debug("[Parseando: %s]", ini);
#endif
				if (sck->readfunc)
					sck->readfunc(sck, ini);
				if (!sck || EsCerr(sck))
					return; /* hemos cerrado el socket */
				ini = ul + 1;
			}
		}
#ifdef ZIP_LINKS
		if (IsZip(sck) && sck->zip->incount)
		{
			recv = unzip_packet(sck, (char *)NULL, &zipped);
			len = zipped;
			zipped = 1;
			if (len == -1)
				Debug("ARG 3");
			done_unzip = 0;
		} 
		else 
			done_unzip = 1;
		} while(!done_unzip);
#endif
		off = 0;
		if (!BadPtr(ini))
		{
			if (quedan)
			{
				memcpy(recv, ini, ul - ini);
				off += ul - ini;
			}
			else
				encola(sck->recvQ, ini, 0);
			//Debug("Ini %s %i", ini, off);
		}
		bzero(recv + off, sizeof(bufr) - off);
	}while (quedan);
}
int lee_socks() /* devuelve los bytes leídos */
{
	int i, len, sels, lee = 0;
	fd_set read_set, write_set, excpt_set;
	struct timeval wait;
	Sock *sck;
	if (ListaSocks.abiertos <= 0)
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&excpt_set);
	for (i = 0; i < ListaSocks.abiertos; i++)
	{
		sck = ListaSocks.socket[i];
		if (sck->pres >= 0 && !EsCerr(sck))
		{
			FD_SET(sck->pres, &read_set);
			FD_SET(sck->pres, &excpt_set);
			if (sck->sendQ->len
#ifdef ZIP_LINKS
			|| (IsZip(sck) && sck->zip->outcount)
#endif
			|| EsConn(sck))
				FD_SET(sck->pres, &write_set);
		}
	}
	wait.tv_sec = 1;
	wait.tv_usec = 0;
	sels = select(MAXSOCKS + 1, &read_set, &write_set, &excpt_set, &wait);
	for (i = 0; i < ListaSocks.abiertos; i++)
	{
		if (!sels)
			break;
		SockActual = ListaSocks.socket[i];
		if (EsCerr(SockActual))
		{
			sels--;
			continue;
		}
		if (FD_ISSET(SockActual->pres, &write_set))
		{
			if (EsConn(SockActual))
			{
				SockOk(SockActual);
				if (SockActual->openfunc)
					SockActual->openfunc(SockActual, NULL);
				ircd_log(LOG_CONN, "Conexión establecida con %s:%i", SockActual->host, SockActual->puerto);
			}
			else
				envia_cola(SockActual);
			sels--;
		}
		if (FD_ISSET(SockActual->pres, &read_set))
		{
			if (EsConn(SockActual))
			{
				SockOk(SockActual);
				if (SockActual->openfunc)
					SockActual->openfunc(SockActual, NULL);
				ircd_log(LOG_CONN, "Conexión establecida con %s:%i", SockActual->host, SockActual->puerto);
			}
			else if (EsList(SockActual))
			{
				int pres;
				if ((pres = accept(SockActual->pres, NULL, NULL)) >= 0)
					sockaccept(SockActual, pres);
				sels--;
				continue;
			}
			len = lee_mensaje(SockActual); /* leemos el mensaje */
			if (len <= 0)
			{
				FD_CLR(SockActual->pres, &read_set);
				sockclose(SockActual, REMOTO);
				continue;
			}
			lee += len;
			crea_mensaje(SockActual, len);
			sels--;
		}
		if (FD_ISSET(SockActual->pres, &excpt_set))
		{
			sockclose(SockActual, REMOTO);
			continue;
		}
	}
	return lee;
}
void inicia_procesos()
{
	for(;;)
	{
#ifdef USA_CONSOLA
		if (!BadPtr(conbuf))
		{
			parsea_comando(conbuf);
			conbuf[0] = '\0';
		}
#endif
		lee_socks();
		comprueba_timers();
		procesos_auxiliares();
		if (reth)
			break;
	}
	/* si llegamos aquí estamos saliendo del programa */
	cierra_socks();
#ifdef _WIN32
	ChkBtCon(0, 0);
#endif
}
#ifdef _WIN32
void carga_programa()
#else
int main(char argv[], int argc)
#endif
{
	Conf config;
	int val, i;
#ifndef _WIN32
	struct rlimit corelim;
#endif
	ListaSocks.abiertos = 0;
	for (i = 0; i < MAXSOCKS; i++)
		ListaSocks.socket[i] = NULL;
	memset(signals, (int)NULL, sizeof(signals));
#ifdef _WIN32	
	mkdir("tmp");
#else
	mkdir("tmp", 0744);
#endif	
	if (parseconf("colossus.conf", &config, 1) < 0)
		return;
	distribuye_conf(&config);
	carga_comandos();
	carga_modulos();
	if ((val = carga_mysql()) <= 0)
	{
		if (!val) /* no ha emitido mensaje */
		{
			char pq[256];
			sprintf_irc(pq, "No se puede iniciar MySQL\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
#ifdef _WIN32			
			MessageBox(hwMain, pq, "MySQL", MB_OK|MB_ICONERROR);
#else
			fprintf(stderr, "[MySQL: %s]\n", pq);
#endif
		}
		cierra_colossus(-1);
	}
	if (is_file("backup.sql"))
	{
#ifdef _WIN32		
		if (MessageBox(hwMain, "Se ha encontrado una copia de la base de datos. ¿Quieres restaurarla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (pregunta("Se ha encontrado una copia de la base de datos. ¿Quieres restaurarla?") == 1)
#endif
		{
			if (!_mysql_restaura())
			{
#ifdef _WIN32				
				if (MessageBox(hwMain, "Se ha restaurado con éxito. ¿Quieres borrar la copia?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
				if (pregunta("Se ha restaurado con éxito. ¿Quieres borrar la copia?") == 1)
#endif
					remove("backup.sql");
			}
			else
			{
				fecho(FSQL, "Ha sido imposible restaurar la copia.");
				cierra_colossus(-1);
			}
		}
	}
	carga_cache();
#ifdef UDB
	bdd_init();
#endif
	distribuye_me(&me, &SockIrcd);
	for (i = 0; i < UMAX; i++)
	{
		uTab[i].item = NULL;
		uTab[i].items = 0;
	}
	for (i = 0; i < CHMAX; i++)
	{
		cTab[i].item = NULL;
		cTab[i].items = 0;
	}
#ifndef _WIN32
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
		printf("unlimit core size failed; errno = %d\n", errno);
	programa_loop_principal(NULL);
#else
	if (!EscuchaIrcd)
		EscuchaIrcd = socklisten(conf_server->escucha, escucha_abre, NULL, NULL, NULL);
#endif
}
void programa_loop_principal(void *args)
{
	intentos = 0;
	inicia_procesos();
}
int is_file(char *archivo)
{
	FILE *fp;
	if (!(fp = fopen(archivo, "r")))
		return 0;
	fclose(fp);
	return 1;
}
int EsIp(char *ip)
{
	char *oct;
	if (!ip)
		return 0;
	if ((oct = strrchr(ip, '.')))
	{
		oct++;
		if (*oct < 65) /* es ip */
			return 1;
	}
	return 0;
}
char *dominum(char *ip)
{
	struct hostent *he;
	int addr;
	if (EsIp(ip))
	{
		addr = inet_addr(ip);
		if ((he = gethostbyaddr((char *)&addr, 4, AF_INET)))
			return strdup(he->h_name);
	}
	else /* es host */
		return ip;
	return NULL;
}
int randomiza(int ini, int fin)
{
	int r, i;
#ifdef _WIN32
	struct _timeb aburst;
	_ftime(&aburst);
#else
	struct timeval aburst;
	gettimeofday(&aburst, NULL);
#endif
	i = (int)&aburst;
	srand(i ^ time(NULL) + rand());
	r = rand() % (fin + 1);
#ifdef _WIN32
	srand(i ^ (aburst.millitm + rand()));
#else
	srand(i ^ (aburst.tv_usec + rand()));
#endif
	if (r < ini)
		return fin - r;
	return r;
}	
char *random_ex(char *patron)
{
	char *ptr;
	int len;
	const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ", *lower = "abcdefghijklmnopqrstuvwxyz";
	len = strlen(patron);
	buf[0] = '\0';
	ptr = &buf[0];
	while (*patron)
	{
		switch(*patron)
		{
			case '?':
				*ptr++ = randomiza(0, 9) + 48;
				break;
			case 'º':
				*ptr++ = upper[randomiza(0, 25)];
				break;
			case '!':
				*ptr++ = lower[randomiza(0, 25)];
				break;
			case '#':
				*ptr++ = (randomiza(0, 1) == 1 ? upper[randomiza(0, 25)] : lower[randomiza(0, 25)]);
				break;
			case '*':
				*ptr++ = (randomiza(0, 1) == 1 ? randomiza(0, 9) + 48 : (randomiza(0, 1) == 1 ? upper[randomiza(0, 25)] : lower[randomiza(0, 25)]));
				break;
			default:
				*ptr++ = *patron;
		}
		patron++;
	}
	*ptr = '\0';
	return buf;
}
void signal_add(short signal, int (*func)())
{
	Signal *sign, *aux;
	for (aux = signals[signal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
			return;
	}
	sign = (Signal *)Malloc(sizeof(Signal));
	sign->signal = signal;
	sign->func = func;
	sign->sig = signals[signal];
	signals[signal] = sign;
}
int signal_del(short signal, int (*func)())
{
	Signal *aux, *prev = NULL;
	for (aux = signals[signal]; aux; aux = aux->sig)
	{
		if (aux->signal == signal)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				signals[signal] = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
double microtime()
{
	double segs, milisegs;
#ifdef _WIN32
	struct _timeb atime;
	_ftime(&atime);
	segs = atime.time;
	milisegs = atime.millitm;
#else
	struct timeval atime;
	gettimeofday(&atime, NULL);
	segs = atime.tv_sec;
	milisegs = atime.tv_usec;
#endif
	while (milisegs > 1) 
		milisegs /= 10;
	return (segs + milisegs);
}
void timer(char *nombre, Sock *sck, int veces, int cada, int (*func)(), void *args, size_t sizearg)
{
	Timer *aux;
	aux = (Timer *)Malloc(sizeof(Timer));
	aux->nombre = strdup(nombre);
	aux->sck = sck;
	aux->cuando = microtime() + cada;
	aux->veces = veces;
	aux->cada = cada;
	aux->lleva = 0;
	aux->func = func;
	aux->args = NULL;
	if (args)
	{
		aux->args = Malloc(sizearg);
		memcpy(aux->args, args, sizearg);
	}
	aux->sig = timers;
	timers = aux;
}
int timer_off(char *nombre, Sock *sck)
{
	Timer *aux, *prev = NULL;
	for (aux = timers; aux; aux = aux->sig)
	{
		if (!strcasecmp(nombre, aux->nombre) && aux->sck == sck)
		{
			Free(aux->nombre);
			Free(aux->args);
			if (prev)
				prev->sig = aux->sig;
			else
				timers = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void comprueba_timers()
{
	Timer *aux, *sig;
	for (aux = timers; aux; aux = sig)
	{
		sig = aux->sig;
		if (aux->cuando < microtime())
		{
			if (aux->args)
				aux->func(aux->args);
			else
				aux->func();
			aux->lleva++;
			aux->cuando = microtime() + aux->cada;
			if (aux->lleva == aux->veces)
				timer_off(aux->nombre, aux->sck);
		}
	}
}
char *str_replace(char *str, char orig, char rep)
{
	static char rem[BUFSIZE];
	char *remp;
	remp = &rem[0];
	strcpy(remp, str);
	while (*remp)
	{
		if (*remp == orig)
			*remp = rep;
		remp++;
	}
	return rem;
}
char *strtolower(char *str)
{
	static char tol[BUFSIZE];
	char *tolo;
	tolo = &tol[0];
	strcpy(tolo, str);
	while (*tolo)
	{
		*tolo = ToLower(*tolo);
		tolo++;
	}
	return tol;
}
char *strtoupper(char *str)
{
	static char tou[BUFSIZE];
	char *toup;
	toup = &tou[0];
	strcpy(toup, str);
	while (*toup)
	{
		*toup = ToUpper(*toup);
		toup++;
	}
	return tou;
}
char *implode(char *array[], int total, int parte, int hasta)
{
	static char imp[BUFSIZE];
	int i;
	imp[0] = '\0';
	for (i = parte; i < total; i++)
	{
		strcat(imp, array[i]);
		if (i != total - 1)
			strcat(imp, " ");
		if (i == hasta)
			break;
	}
	return imp;
}
char *gettok(char *str, int pos, char sep)
{
	int i;
	static char buftok[BUFSIZE];
	while (!BadPtr(str))
	{
		if (*str == sep || pos == 1)
		{
			if (pos > 1)
			{
				str++;
				pos--;
			}
			if (pos == 1)
			{
				while (*str == sep)
					str++;
				for (i = 0; !BadPtr(str); i++)
				{
					buftok[i] = *str++;
					if (*str == sep)
					{
						buftok[i+1] = '\0';
						return buftok;
					}
				}
				buftok[i] = '\0';
				return buftok;
			}
		}
		str++;
	}
	return NULL;
}
void procesos_auxiliares()
{
	Proc *aux;
	for (aux = procs; aux; aux = aux->sig)
		aux->func(aux);
}
/* Los procs no se apagan */
void proc(int (*func)())
{
	Proc *proc;
	for (proc = procs; proc; proc = proc->sig)
	{
		if (proc->func == func)
			return;
	}
	proc = (Proc *)Malloc(sizeof(Proc));
	proc->func = func;
	proc->proc = 0;
	proc->time = (time_t) 0;
	proc->sig = procs;
	procs = proc;
}
/*
#define MAX 32
typedef struct
{
	double valors[MAX][MAX];
	int ordre;
}mat;
double det(mat);
void print_mat(mat A)
{
	int i, j;
	for (i = 0; i < A.ordre; i++)
	{
		for (j = 0; j < A.ordre; j++)
			printf("%f ", A.valors[i][j]);
		printf("\n");
	}
	printf("------\n");
}
mat menor(mat A, int fila, int col)
{
	int i, j;
	for (i = fila; i < A.ordre; i++)
	{
		for (j = 0; j < A.ordre; j++)
			A.valors[i][j] = A.valors[i+1][j];
	}
	for (j = col; j < A.ordre; j++)
	{
		for (i = 0; i < A.ordre; i++)
			A.valors[i][j] = A.valors[i][j+1];
	}
	A.ordre--;
	print_mat(A);
	return A;
}
double adjunt(mat A, int fila, int col)
{
	return ((fila + col) % 2 ? -1 : 1)*A.valors[fila][col]*det(menor(A, fila, col));
}
double det(mat A)
{
	int i;
	double determ = 0;
	if (A.ordre == 2)
		return A.valors[0][0] * A.valors[1][1] - A.valors[0][1] * A.valors[1][0];
	for (i = 0; i < A.ordre; i++)
		determ += adjunt(A, i, 0);
	return determ;
}
int mainn(int argc, char *argv[])
{
	mat A;
	int i, j, ordre;
	ordre = atoi(argv[1]);
	srand(time(NULL));
	for (i = 0; i < ordre; i++)
	{
		for (j = 0; j < ordre; j++)
		{
			A.valors[i][j] = rand() % 3;
			//printf("%f ",A.valors[i][j]);
		}
	}
	//printf("\n");
	A.ordre = ordre;
	print_mat(A);
	printf("\nDeterminant: %f", det(A));
	return 1;
}
*/
char *_asctime(time_t *tim)
{
    static char *wday_name[] = {
        "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"
    };
    static char *mon_name[] = {
        "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
        "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
    };
    static char result[26];
    struct tm *timeptr;
    timeptr = localtime(tim);
    sprintf(result, "%.3s %.2d %.3s %d %.2d:%.2d:%.2d",
        wday_name[timeptr->tm_wday],
        timeptr->tm_mday,
        mon_name[timeptr->tm_mon],
        1900 + timeptr->tm_year,
        timeptr->tm_hour,
        timeptr->tm_min, timeptr->tm_sec);
    return result;
}
void fecho(char err, char *error, ...)
{
	char buf[BUFSIZE];
	int opts;
	va_list vl;
	va_start(vl, error);
	vsprintf_irc(buf, error, vl);
	va_end(vl);
#ifdef _WIN32
	switch (err)
	{
		case FERR:
			opts = MB_OK|MB_ICONERROR;
			break;
		case FSQL:
		case FADV:
			opts = MB_OK|MB_ICONWARNING;
			break;
		case FOK:
			opts = MB_OK|MB_ICONINFORMATION;
			break;
	}
	MessageBox(NULL, buf, "Colossus", opts);
#else
	switch (err)
	{
		case FERR:
			fprintf(stderr, "[ERROR: %s]\n", buf);
			break;
		case FSQL:
			fprintf(stderr, "[MySQL: %s]\n", buf);
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
int carga_cache()
{
	int i;
	char buf[1][BUFSIZE], tabla[1];
	tabla[0] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, MYSQL_CACHE);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`valor` varchar(255) default NULL, "
  			"`hora` int(11) NOT NULL default '0', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla caché';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
	_mysql_carga_tablas();
	return 1;
}
/* funciones extraidas del unreal */
time_t getfilemodtime(char *filename)
{
#ifdef _WIN32
	FILETIME cTime;
	SYSTEMTIME sTime, lTime;
	ULARGE_INTEGER fullTime;
	time_t result;
	HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	if (!GetFileTime(hFile, NULL, NULL, &cTime))
		return 0;
	CloseHandle(hFile);
	FileTimeToSystemTime(&cTime, &sTime);
	SystemTimeToTzSpecificLocalTime(NULL, &sTime, &lTime);
	SystemTimeToFileTime(&sTime, &cTime);
	fullTime.LowPart = cTime.dwLowDateTime;
	fullTime.HighPart = cTime.dwHighDateTime;
	fullTime.QuadPart -= 116444736000000000;
	fullTime.QuadPart /= 10000000;
	return fullTime.LowPart;
#else
	struct stat sb;
	if (stat(filename, &sb))
		return 0;
	return sb.st_mtime;
#endif
}
void setfilemodtime(char *filename, time_t mtime)
{
#ifdef _WIN32
	FILETIME mTime;
	LONGLONG llValue;
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	llValue = Int32x32To64(mtime, 10000000) + 116444736000000000;
	mTime.dwLowDateTime = (long)llValue;
	mTime.dwHighDateTime = llValue >> 32;
	
	SetFileTime(hFile, &mTime, &mTime, &mTime);
	CloseHandle(hFile);
#else
	struct utimbuf utb;
	utb.actime = utb.modtime = mtime;
	utime(filename, &utb);
#endif
}
int copyfile(char *src, char *dest)
{
	char buf[2048];
	time_t mtime = getfilemodtime(src);
#ifdef _WIN32
	int srcfd = open(src, _O_RDONLY|_O_BINARY);
#else
	int srcfd = open(src, O_RDONLY);
#endif
	int destfd;
	int len;
	if (srcfd < 0)
		return 0;
#ifdef _WIN32
	destfd = open(dest, _O_BINARY|_O_WRONLY|_O_CREAT, _S_IWRITE);
#else
	destfd  = open(dest, O_WRONLY|O_CREAT, 0644);
#endif
	if (destfd < 0)
	{
		fecho(FADV, "No se puede crear el archivo '%s'", dest);
		return 0;
	}

	while ((len = read(srcfd, buf, 1023)) > 0)
	{
		if (write(destfd, buf, len) != len)
		{
			fecho(FADV, "No se puede escribir en el archivo '%s'. Quizás por falta de espacio en su HD.", dest);
			close(srcfd);
			close(destfd);
			unlink(dest); /* make sure our corrupt file isn't used */
			return 0;
		}
	}
	close(srcfd);
	close(destfd);
	setfilemodtime(dest, mtime);
	return 1;	
}
#ifdef USA_CONSOLA
#define MyErrorExit(x) { fecho(FERR, x); exit(-1); }
void *consola_loop_principal(void *args)
{
	HANDLE hStdin;
	DWORD fdwMode, fdwSaveOldMode, cNumRead; 
	char irInBuf[128], cop[128]; 
	cop[0] = '\0';
	hStdin = GetStdHandle(STD_INPUT_HANDLE); 
	if (hStdin == INVALID_HANDLE_VALUE) 
		MyErrorExit("GetStdHandle");
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) 
		MyErrorExit("GetConsoleMode");
	fdwMode = ENABLE_PROCESSED_INPUT; 
	if (!SetConsoleMode(hStdin, fdwMode)) 
		MyErrorExit("SetConsoleMode");
	while (1)
	{
		char c;
		bzero(irInBuf, 128);
		if (!ReadConsole(hStdin, (LPVOID)irInBuf, 128, &cNumRead, NULL))
            			MyErrorExit("ReadConsoleInput");
            	textcolor(LIGHTGREEN);
		printf("%c", *irInBuf);
		textcolor(LIGHTGRAY);
            	if (*irInBuf == '\r')
            	{
            		printf("\n");
            		strcpy(conbuf, (char *) cop);
            		cop[0] = '\0';
            	}
            	else
            		strcat(cop, irInBuf);
        }
}
void parsea_comando(char *comando)
{
	if (!comando)
		return;
	if (!strcasecmp(comando, "REHASH"))
	{
		fecho(FOK, "Refrescando servicios");
		refresca();
	}
	/* else if (!strcasecmp(comando, "RESTART"))
	{
		fecho(FOK, "Reiniciando servicios");
		reinicia();
	} */
	else if (!strcasecmp(comando, "VERSION"))
	{
#ifdef UDB
		printf("\n\tv%s+UDB2.1\n\tServicios para IRC.\n\n\tTrocotronic - 2004\n\thttp://www.rallados.net\n\n", COLOSSUS_VERSION);
#else
		printf("\n\tv%s\n\tServicios para IRC.\n\n\tTrocotronic - 2004\n\thttp://www.rallados.net\n\n", COLOSSUS_VERSION);
#endif
	}
	else if (!strcasecmp(comando, "QUIT"))
	{
		fecho(FOK, "Saliendo de la aplicacion");
		Sleep(1000);
		exit(0);
	}
	else if (!strcasecmp(comando, "AYUDA"))
	{
		fecho(FOK, "Comandos disponibles:");
		fecho(FOK, "\t-REHASH Refresca la configuracion y recarga los modulos");
		fecho(FOK, "\t-VERSION Muestra la version actual");
		fecho(FOK, "\t-QUIT Cierra el programa");
	}
	else
		fecho(FERR, "Comando desconocido. Para mas informacion escriba AYUDA");
}
#endif
/*
int main(int argc, char *argv[])
{
	int i, j, k;
	int l0, l1, l2;
	u_long c1, c2, c3;
	c1 = our_crc32("80.58", strlen("80.58"));
	c2 = our_crc32("80.58.51", strlen("80.58.51"));
	c3 = our_crc32("80.58.51.44", strlen("80.58.51.44"));
	for (i = 10001; i < 2147483647; i++)
	{
		for (j = 10001; j < 2147483647; j++)
		{
			for (k = 10001; k < 2147483647; k++)
			{
				l0 = (((c1 + i) ^ j) + k) & 0x7FFFFFFF;
				l1 = (((c2 ^ j) + k) ^ i) & 0xFFFFFFFF;
				l2 = (((c3 + k) ^ i) + j) & 0x3FFFFFFF;
				if (0x184B845B == l2 && 0x64580349 == l1 && 0x5F27B5DF == l0)
					printf("%i %i %i\n", i, j, k);
			}
		}
	}
	return 0;
}
*/
void Debug(char *formato, ...)
{
	char debugbuf[BUFSIZE];
	va_list vl;
#ifdef _WIN32
	static HANDLE *conh = NULL;
	LPDWORD len = 0;
#endif
	va_start(vl, formato);
	vsprintf_irc(debugbuf, formato, vl);
	va_end(vl);
	strcat(debugbuf, "\r\n");
#ifdef _WIN32
	if (!conh)
	{
		if (AllocConsole())
		{
			conh = (HANDLE *)Malloc(sizeof(HANDLE));
			*conh = GetStdHandle(STD_OUTPUT_HANDLE);
		}
	}
	WriteFile(*conh, debugbuf, strlen(debugbuf), len, NULL);
#else
	fprintf(stderr, debugbuf);
#endif
}
void destruye_temporales()
{
	Modulo *mod;
	for (mod = modulos; mod; mod = mod->sig)
	{
		//fecho(FOK,"eliminando %s",mod->tmparchivo);
		unlink(mod->tmparchivo);
	}
}
void cierra_colossus(int excode)
{
	destruye_temporales();
#ifdef _WIN32
	DestroyWindow(hwMain);
	WSACleanup();
#endif
	exit(excode);
}
void ircd_log(int opt, char *formato, ...)
{
	char buf[256], auxbuf[512];
	int fp;
	va_list vl;
	time_t tm;
	struct stat inode;
	if (!conf_log || !(conf_log->opts & opt))
		return;
	tm = time(0);
	va_start(vl, formato);
	vsprintf_irc(buf, formato, vl);
	va_end(vl);
	sprintf_irc(auxbuf, "(%s) > %s\n", _asctime(&tm), buf);
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
#ifndef _WIN32
int pregunta(char *preg)
{
	fprintf(stderr, "%s (s/n)\n", preg);
	if (getc(stdin) == 's')
		return 1;
	return 0;
}
#endif
