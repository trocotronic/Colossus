/*
 * $Id: main.c,v 1.25 2004-12-31 12:27:57 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef UDB
#include "bdd.h"
#endif
#ifdef USA_ZLIB
#include "zlib.h"
#endif
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <utime.h>
#include <sys/resource.h>
#include <pthread.h>
#endif
#include <fcntl.h>
#undef USA_CONSOLA

static char buf[BUFSIZE];
static char bufr[BUFSIZE+1];

struct Sockets
{
	Sock *socket[MAXSOCKS];
	int abiertos;
	int tope;
}ListaSocks;

Sock *SockActual;
MYSQL *mysql = NULL;
Senyal *senyals[MAXSIGS];
Timer *timers = NULL;
Proc *procs = NULL;
char tokbuf[BUFSIZE];
#ifdef USA_CONSOLA
char conbuf[128];
HANDLE hStdin;
#endif
int listens[MAX_LISTN];
int listenS = 0;
MODVAR char **margv;
time_t iniciado;

void encola(DBuf *, char *, int);
int desencola(DBuf *, char *, int, int *);
void envia_cola(Sock *);
char *lee_cola(Sock *);
int carga_cache(void);
int completa_conexion(Sock *);

#ifdef USA_CONSOLA
void *consola_loop_principal(void *);
void parsea_comando(char *);
#endif
#ifdef _WIN32
void programa_loop_principal(void *);
#endif
#define MIN(x,y) (x < y ? x : y)
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
#ifndef _WIN32
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
char *StsMalloc(size_t size, char *file, long line)
{
	void *x;
	x = malloc(size);
#ifdef DEBUG
	Debug("Dando direccion %X", x);
#endif
	if (!x)
	{
		fecho(FERR,  "[%s:%i] Te has quedado sin memoria", file, line);
		exit(-1);
	}
	return x;
}
int strcasecmp(const char *a, const char *b)
{
	const char *ra = a;
	const char *rb = b;
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
char *my_itoa(int i)
{
	static char buf[128];
#ifndef _WIN32	
	sprintf_irc(buf, "%d", i);
#else
	_itoa(i, buf, 10);
#endif
	return (buf);
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
	int i;
	for (i = 0; ListaSocks.socket[i]; i++);
	sck->slot = i;
	ListaSocks.socket[i] = sck;
#ifdef DEBUG
	Debug("Insertando sock %X %i en %i ", sck, sck->pres, ListaSocks.abiertos);
#endif
	ListaSocks.abiertos++;
	if (i > ListaSocks.tope)
		ListaSocks.tope = i;
}
void borra_sock(Sock *sck)
{
	ListaSocks.socket[sck->slot] = NULL;
	sck->slot = -1;
	while (!ListaSocks.socket[ListaSocks.tope])
		ListaSocks.tope--;
	ListaSocks.abiertos--;
}
/*
 * sockopen
 * Abre una conexi�n
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
#ifdef USA_SSL
	if (puerto < 0) /* es un puerto ssl */
	{
		puerto = puerto * -1;
		sck->opts |= OPT_SSL;
	}
#endif
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
	Debug("Abriendo conexion con %s:%i (%i) %s", host, puerto, sck->pres, EsSSL(sck) ? "[SSL]" : "");
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
	Debug("Escuchando puerto %i (%i)", puerto, sck->pres);
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
		CLOSE_SOCK(pres);
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
	if (sockblock(sck->pres) == -1)
		return NULL;
	da_Malloc(sck->recvQ, DBuf);
	da_Malloc(sck->sendQ, DBuf);
	sck->openfunc = list->openfunc;
	sck->readfunc = list->readfunc;
	sck->writefunc = list->writefunc;
	sck->closefunc = list->closefunc;
	inserta_sock(sck);
#ifdef USA_SSL
	if (EsSSL(list))
	{
		SetSSLAcceptHandshake(sck);
		if (!(sck->ssl = SSL_new(ctx_server)))
		{
			sockclose(sck, LOCAL);
			return NULL;
		}
		sck->opts |= OPT_SSL;
		SSL_set_fd(sck->ssl, pres);
		SSL_set_nonblocking(sck->ssl);
		if (!ircd_SSL_accept(sck, pres)) 
		{
			SSL_set_shutdown(sck->ssl, SSL_RECEIVED_SHUTDOWN);
			SSL_smart_shutdown(sck->ssl);
			SSL_free(sck->ssl);
			sockclose(sck, LOCAL);
			return NULL;
		}
	}
	else
#endif
	completa_conexion(sck);
	return sck;
}
void sockwritev(Sock *sck, int opts, char *formato, va_list vl)
{
	char buf[BUFSIZE], *msg;
	if (!sck)
		return;
	msg = buf;
	vsprintf_irc(buf, formato, vl);
#ifndef DEBUG
	Debug("[Enviando: %s]", buf);
#endif
	if (opts & OPT_CR)
		strcat(buf, "\r");
	if (opts & OPT_LF)
		strcat(buf, "\n");
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
	if (!sck)
		return;
	if (closef == LOCAL)
		envia_cola(sck); /* enviamos toda su cola */
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
	Debug("Cerrando conexion con %s:%i (%i) [%s]", sck->host, sck->puerto, sck->pres, closef == LOCAL ? "LOCAL" : "REMOTO");
#endif
	CLOSE_SOCK(sck->pres);
#ifdef USA_SSL
	if (EsSSL(sck) && sck->ssl) 
	{
		SSL_set_shutdown((SSL *)sck->ssl, SSL_RECEIVED_SHUTDOWN);
		SSL_smart_shutdown((SSL *)sck->ssl);
		SSL_free((SSL *)sck->ssl);
		sck->ssl = NULL;
	}
#endif
	if (sck->closefunc)
		sck->closefunc(sck, closef ? NULL : "LOCAL");
	bzero(bufr, BUFSIZE+1);
	borra_sock(sck);
	ircfree(sck->recvQ);
	ircfree(sck->sendQ);
	ircfree(sck->host);
	Free(sck);
}
void cierra_socks()
{
	int i;
	for (i = 0; i < ListaSocks.tope; i++)
		sockclose(ListaSocks.socket[i], LOCAL);
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
		copiados = MIN(len, (int)(sizeof(bufc->wslot->data) - bufc->wslot->len));
		//Debug("Copiados %i",copiados);
		memcpy(bufc->wchar, str, copiados);
		str += copiados;
		bufc->wchar += copiados;
		len -= copiados;
		bufc->wslot->len += copiados;
		bufc->len += copiados;
		if (bufc->wslot->len == sizeof(bufc->wslot->data)) /* est� lleno, abrimos otro */
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
int desencola(DBuf *bufc, char *buf, int bytes, int *len)
{
	/* bytes contiene el m�ximo que podemos copiar */
	DbufData *aux;
	int curpos = 0, copiados;
	//Debug("Dlen %i",bytes);
	if (!bufc->slots)
		return 0;
	do
	{
		aux = bufc->rslot->sig;
		copiados = MIN((int)bufc->rslot->len, bytes - curpos);
		//Debug("DCopiados %i %p %p",copiados, bufc->rchar,&bufc->rslot->data[0]);
		memcpy(buf + curpos, bufc->rchar, copiados);
		curpos += copiados;
		bufc->len -= copiados;
		bufc->rslot->len -= copiados;
		if (bufc->rslot->len <= 0) /* lo podemos copiar todo */
		{
			Free(bufc->rslot);
			//Debug("Adios %X %X", bufc->rslot,aux);
			if (!(bufc->rslot = aux))
				break;
			bufc->rchar = &aux->data[0]; /* al siguiente slot */
			bufc->slots--;
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
	*len = MIN(bytes, curpos);
	return bufc->len;
}
void envia_cola(Sock *sck)
{
	char *msg;
	int len = 0;
	if (sck->sendQ && sck->sendQ->len > 0)
	{
		int quedan;
		msg = bufr;
		do 
		{
			quedan = desencola(sck->sendQ, msg, sizeof(bufr), &len);
			if (BadPtr(msg))
				return;
			if (!len)
				len = strlen(msg);
#ifdef USA_ZLIB
			if (EsZlib(sck))
				msg = comprime(sck, msg, &len);
#endif
#ifdef USA_SSL
			if (EsSSL(sck))
				ircd_SSL_write(sck, msg, len);
			else
#endif
			WRITE_SOCK(sck->pres, msg, len);
			len = 0;
			if (sck->writefunc)
				sck->writefunc(sck, msg);
			bzero(msg, sizeof(bufr)); /* hay que vaciarlo!! */
		}while(quedan);
	}
}
int lee_mensaje(Sock *sck)
{
	char lee[BUFSIZE], *msg;
	int len = 0;
	if (EsCerr(sck))
		return -3;
	SET_ERRNO(0);
	msg = lee;
	bzero(lee, BUFSIZE);
#ifdef USA_SSL
	if (EsSSL(sck))
		len = ircd_SSL_read(sck, lee, BUFSIZE);
	else
#endif
	len = READ_SOCK(sck->pres, lee, BUFSIZE);
	if (len < 0 && ERRNO == P_EWOULDBLOCK)
		return 1;
#ifdef USA_ZLIB
	if (EsZlib(sck))
		msg = descomprime(sck, msg, &len);
#endif
	if (len > 0)
		encola(sck->recvQ, msg, len);
	return len;
}
void crea_mensaje(Sock *sck, int leng)
{
	char *recv, *ini, *ul;
	int quedan, off, len = 0;
	recv = bufr;
	off = 0;
	do
	{
		quedan = desencola(sck->recvQ, recv + off, BUFSIZE - off, &len);
		recv[len] = '\0';
		//Debug("*** recv %i %i",quedan,len);
		for (ini = ul = recv; !BadPtr(ul); ul++)
		{
			if (*ul == '\n')
			{
				*ul = '\0';
				if (*(ul - 1) == '\r')
					*(ul - 1) = '\0';
#ifndef DEBUG
				Debug("[Parseando: %s]", ini);
#endif
				if (sck->readfunc)
					sck->readfunc(sck, ini);
				if (!sck || EsCerr(sck))
					return; /* hemos cerrado el socket */
				ini = ul + 1;
			}
		}
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
int completa_conexion(Sock *sck)
{
	SockOk(sck);
#ifdef DEBUG
	Debug("Conexion establecida con %s:%i (%i)", sck->host, sck->puerto, sck->pres);
#endif
	if (sck->openfunc)
		sck->openfunc(sck, NULL);
	ircd_log(LOG_CONN, "Conexi�n establecida con %s:%i", sck->host, sck->puerto);
	return 0;
}
int lee_socks() /* devuelve los bytes le�dos */
{
	int i, len, sels, lee = 0;
	fd_set read_set, write_set, excpt_set;
	struct timeval wait;
	Sock *sck;
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&excpt_set);
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (!(sck = ListaSocks.socket[i]))
			continue;
		if (sck->pres >= 0 && !EsCerr(sck))
		{
			FD_SET(sck->pres, &read_set);
			FD_SET(sck->pres, &excpt_set);
			
			if (sck->sendQ->len || EsConn(sck)
#ifdef USA_ZLIB
			|| (EsZlib(sck) && sck->zlib->outcount > 0)
#endif
			)
				FD_SET(sck->pres, &write_set);
		}
	}
	wait.tv_sec = 1;
	wait.tv_usec = 0;
	sels = select(MAXSOCKS + 1, &read_set, &write_set, &excpt_set, &wait);
	if (sels == -1)
	{
		if ((ERRNO == P_EINTR) || (ERRNO == P_ENOTSOCK))
			return -1;
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if ((SockActual = ListaSocks.socket[i]) && FD_ISSET(SockActual->pres, &read_set) && EsList(SockActual))
		{
			int pres;
			FD_CLR(SockActual->pres, &read_set);
			sels--;
			if ((pres = accept(SockActual->pres, NULL, NULL)) >= 0)
				sockaccept(SockActual, pres);
		}
	}
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (!sels)
			break;
		if (!(SockActual = ListaSocks.socket[i]))
			continue;
		if (FD_ISSET(SockActual->pres, &write_set))
		{
			int err = 0;
			if (EsConn(SockActual))
			{
#ifdef USA_SSL
				if (EsSSL(SockActual))
					err = ircd_SSL_client_handshake(SockActual);
				else
#endif
					err = completa_conexion(SockActual);

			}
			else
				envia_cola(SockActual);
			if (EsCerr(SockActual) || err)
			{
				if (FD_ISSET(SockActual->pres, &read_set))
				{
					sels--;
					FD_CLR(SockActual->pres, &read_set);
				}
				sockclose(SockActual, REMOTO);
				continue;
			}
		}
		if (FD_ISSET(SockActual->pres, &read_set))
		{
			len = 1;
#ifdef USA_SSL
			if (!(IsSSLAcceptHandshake(SockActual) || IsSSLConnectHandshake(SockActual)))
#endif
			len = lee_mensaje(SockActual); /* leemos el mensaje */
#ifdef USA_SSL
			if (SockActual->ssl && (IsSSLAcceptHandshake(SockActual) || IsSSLConnectHandshake(SockActual)))
			{
				if (!SSL_is_init_finished(SockActual->ssl))
				{
					if (EsCerr(SockActual) || IsSSLAcceptHandshake(SockActual) ? !ircd_SSL_accept(SockActual, SockActual->pres) : ircd_SSL_connect(SockActual) < 0)
						len = -2;
				}
				else
					completa_conexion(SockActual);
			}
#endif
			if (len <= 0)
			{
				sels--;
				FD_CLR(SockActual->pres, &read_set);
				if (!EsCerr(SockActual))
					sockclose(SockActual, REMOTO);
				continue;
			}
			lee += len;
			crea_mensaje(SockActual, len);
			sels--;
		}
		if (FD_ISSET(SockActual->pres, &excpt_set))
		{
			sels--;
			FD_CLR(SockActual->pres, &excpt_set);
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
	}
	/* si llegamos aqu� estamos saliendo del programa */
	cierra_socks();
#ifdef _WIN32
	ChkBtCon(0, 0);
#endif
}
void escribe_pid()
{
#ifdef PID
	int  fd;
	char buff[20];
	if ((fd = open(PID, O_CREAT | O_WRONLY, 0600)) >= 0)
	{
		bzero(buff, sizeof(buff));
		sprintf(buff, "%5i\n", (int)getpid());
		if (write(fd, buff, strlen(buff)) == -1)
			Debug("No se puede escribir el archivo pid %s", PID);
		close(fd);
		return;
	}
#ifdef	DEBUG
	else
		Debug("No se puede abrir el archivo pid %s", PID);
#endif
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
void cierra_todo()
{
	cierra_socks();
	destruye_temporales();
#ifdef _WIN32
	DestroyWindow(hwMain);
	WSACleanup();
#endif
}
void refresca()
{
	Conf config;
	descarga_modulos();
	if (protocolo)
	{
		libera_protocolo(protocolo);
		protocolo = NULL;
	}
	parseconf("colossus.conf", &config, 1);
	distribuye_conf(&config);
	carga_modulos();
#ifdef UDB
	carga_bloques();
#endif
	if (SockIrcd)
	{
		senyal(SIGN_SYNCH);
		senyal(SIGN_EOS);
	}
	return;
}
void reinicia()
{
	cierra_todo();
#ifdef _WIN32
	execv(margv[0], margv);
#else
	execv(".", margv);
#endif
	exit(-1);
}
void cierra_colossus(int excode)
{
	cierra_todo();
	exit(excode);
}
#ifdef _WIN32
int carga_programa(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	Conf config;
	int val, i;
#ifndef _WIN32
	struct rlimit corelim;
	for (i = 0; logo[i] != 0; i++)
		fprintf(stderr, "%c", logo[i]);
	fprintf(stderr, "\n\t\t" COLOSSUS_VERSION "\n");
#ifdef UDB
	fprintf(stderr, "\t\t+UDB 3.0\n");
#endif
#ifdef USA_ZLIB
	fprintf(stderr, "\t\t+ZLIB %s\n", zlibVersion());
#endif
#ifdef USA_SSL
	fprintf(stderr, "\t\t+%s\n", OPENSSL_VERSION_TEXT);
#endif
	fprintf(stderr, "\n\t\tTrocotronic - http://www.rallados.net\n");
	fprintf(stderr, "\t\t(c)2004\n");
	fprintf(stderr, "\n");
#endif
	iniciado = time(0);
	ListaSocks.abiertos = ListaSocks.tope = 0;
	for (i = 0; i < MAXSOCKS; i++)
		ListaSocks.socket[i] = NULL;
	memset(senyals, (int)NULL, sizeof(senyals));
#ifdef _WIN32	
	mkdir("tmp");
#else
	mkdir("tmp", 0744);
#endif	
#ifndef _WIN32
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
		printf("unlimit core size failed; errno = %d\n", errno);
#endif
	/* las primeras se�ales deben ser del n�cleo */
	inserta_senyal(SIGN_SYNCH, mete_bots);
	inserta_senyal(SIGN_EOS, mete_residentes);
	if (parseconf("colossus.conf", &config, 1) < 0)
		return 1;
	distribuye_conf(&config);
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
		if (MessageBox(hwMain, "Se ha encontrado una copia de la base de datos. �Quieres restaurarla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (pregunta("Se ha encontrado una copia de la base de datos. �Quieres restaurarla?") == 1)
#endif
		{
			if (!_mysql_restaura())
			{
#ifdef _WIN32				
				if (MessageBox(hwMain, "Se ha restaurado con �xito. �Quieres borrar la copia?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
				if (pregunta("Se ha restaurado con �xito. �Quieres borrar la copia?") == 1)
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
	margv = argv;
	carga_cache();
#ifdef UDB
	bdd_init();
#endif
#ifdef USA_SSL
	init_ssl();
#endif
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
	distribuye_me(&me, &SockIrcd);
#ifdef _WIN32
	signal(SIGSEGV, CleanUpSegv);
#else
	signal(SIGHUP, refresca);
	signal(SIGTERM, cierra_colossus);
	signal(SIGINT, reinicia);
#endif
#ifndef _WIN32
	if (fork())
		exit(0);
	escribe_pid();
	abre_sock_ircd();
#else
	escucha_ircd();
	return 0;
}
void programa_loop_principal(void *args)
{
#endif
	intentos = 0;
	inicia_procesos();
#ifndef _WIN32
	return 0;
#endif
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
		if (*(++oct) < 65) /* es ip */
			return 1;
	}
	return 0;
}
typedef struct
{
	char **destino;
	char *ip;
}Host;
void dominum(Host *aux)
{
	if (EsIp(aux->ip))
	{
		struct hostent *he;
		int addr;
		addr = inet_addr(aux->ip);
		if ((he = gethostbyaddr((char *)&addr, 4, AF_INET)))
			*(aux->destino) = strdup(he->h_name);
	}
	else /* es host */
		*(aux->destino) = aux->ip;
	Free(aux);
}
void resuelve_host(char **destino, char *ip)
{
	Host *aux;
#ifndef _WIN32
	pthread_t id;
#endif
	da_Malloc(aux, Host);
	aux->destino = destino;
	aux->ip = ip;
#ifdef _WIN32
	_beginthread(dominum, 0, aux);
#else
	pthread_create(&id, NULL, dominum, aux);
#endif
}
char *decode_ip(char *buf)
{
	int len = strlen(buf);
	char targ[25];
	b64_decode(buf, targ, 25);
	if (len == 8)
		return inet_ntoa(*(struct in_addr *)targ);
	return NULL;
}
char *encode_ip(char *ip)
{
	struct in_addr ia;
	static char buf[25];
	u_char *cp;
	ia.s_addr = inet_addr(ip);
	cp = (u_char *)ia.s_addr;
	b64_encode((char *)&cp, sizeof(struct in_addr), buf, 25);
	return buf;
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
	srand(i ^ (time(NULL) + rand()));
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
			case '�':
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
void inserta_senyal(short senyal, int (*func)())
{
	Senyal *sign, *aux;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
			return;
	}
	sign = (Senyal *)Malloc(sizeof(Senyal));
	sign->senyal = senyal;
	sign->func = func;
	sign->sig = senyals[senyal];
	senyals[senyal] = sign;
}
int borra_senyal(short senyal, int (*func)())
{
	Senyal *aux, *prev = NULL;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				senyals[senyal] = aux->sig;
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
        "Domingo", "Lunes", "Martes", "Mi�rcoles", "Jueves", "Viernes", "S�bado"
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
#ifdef _WIN32
	int opts;
#endif
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
	if (!_mysql_existe_tabla(MYSQL_CACHE))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`valor` varchar(255) default NULL, "
  			"`hora` int(11) NOT NULL default '0', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla cach�';", PREFIJO, MYSQL_CACHE))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, MYSQL_CACHE);
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
	mTime.dwHighDateTime = (DWORD)llValue >> 32;
	
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
			fecho(FADV, "No se puede escribir en el archivo '%s'. Quiz�s por falta de espacio en su HD.", dest);
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
#ifdef _WIN32
#include "win32/resource.h"
extern HWND hwConfError;
extern HINSTANCE hInst;
extern LRESULT CALLBACK ConfErrorDLG(HWND, UINT, WPARAM, LPARAM);
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
void ChkBtCon(int val, int block)
{
	SetDlgItemText(hwMain, BT_CON, val ? "Desconectar" : "Conectar");
	CheckDlgButton(hwMain, BT_CON, val && !block ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwMain, BT_CON), block ? FALSE : TRUE);
}
#endif

static u_long crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };
u_long our_crc32(const u_char *s, u_int len)
{
	u_int i;
	u_long crc32val = 0L;
	for (i = 0;  i < len;  i ++)
		crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
	return crc32val;
}
#define NUMNICKLOG 6
#define NUMNICKMAXCHAR 'z'
#define NUMNICKBASE 64
#define NUMNICKMASK 63
u_int base64toint(const char *s);
const char *inttobase64(char *buf, u_int v, u_int count);

void tea(u_int v[], u_int k[], u_int x[])
{
	u_int y = v[0] ^ x[0], z = v[1] ^ x[1], sum = 0, delta = 0x9E3779B9;
	u_int a = k[0], b = k[1], n = 32;
	u_int c = 0, d = 0;
	while (n-- > 0)
	{
		sum += delta;
		y += ((z << 4) + a) ^ ((z + sum) ^ ((z >> 5) + b));
		z += ((y << 4) + c) ^ ((y + sum) ^ ((y >> 5) + d));
	}
	x[0] = y;
	x[1] = z;
}
void untea(u_int v[], u_int k[], u_int x[])
{
	u_int y = v[0] ^ x[0], z = v[1] ^ x[1], sum = 0xC6EF3720, delta = 0x9E3779B9;
	u_int a = k[0], b = k[1], n = 32;
	u_int c = 0, d = 0;
	while (n-- > 0)
	{
		z -= ((y << 4) + c) ^ ((y + sum) ^ ((y >> 5) + d));
		y -= ((z << 4) + a) ^ ((z + sum) ^ ((z >> 5) + b));
		sum -= delta;
	}
	x[0] = y;
	x[1] = z;
}
static const char convert2y[] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3','4','5','6','7','8','9','[',']'
};
static const u_int convert2n[] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  52,53,54,55,56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 
   0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0,
   0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0,

   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
u_int base64toint(const char *s)
{
	u_int i = convert2n[(u_char)*s++];
	while (*s)
	{
		i <<= NUMNICKLOG;
		i += convert2n[(u_char)*s++];
	}
	return i;
}
const char *inttobase64(char *buf, u_int v, u_int count)
{
	buf[count] = '\0';
	while (count > 0)
	{
		buf[--count] = convert2y[(v & NUMNICKMASK)];
		v >>= NUMNICKLOG;
	}
	return buf;
}
char *cifranick(char *nickname, char *pass)
{
	u_int v[2], k[2], x[2], *p;
	int cont = (conf_set->nicklen + 8) / 8, i = 0;
	char *tmpnick, tmppass[12 + 1], *nick;
	static char clave[12 + 1];
	if (nickname == NULL || pass == NULL)
		return "\0";
	tmpnick = (char *)Malloc(sizeof(char) * ((8 * (conf_set->nicklen + 8) / 8) + 1));
	nick = (char *)Malloc(sizeof(char) * (conf_set->nicklen + 1));
	strcpy(nick, nickname);
	while (nick[i] != 0)
	{
		nick[i] = ToLower(nick[i]);
		i++;
	}  
	memset(tmpnick, 0, (8 * (conf_set->nicklen + 8) / 8) + 1);
	strncpy(tmpnick, nick , (8 * (conf_set->nicklen + 8) / 8) + 1 - 1);
	memset(tmppass, 0, sizeof(tmppass));
	strncpy(tmppass, pass, sizeof(tmppass) - 1);
	strncat(tmppass, "AAAAAAAAAAAA", sizeof(tmppass) - strlen(tmppass) -1);
	x[0] = x[1] = 0;
	k[1] = base64toint(tmppass + 6);
	tmppass[6] = '\0';
	k[0] = base64toint(tmppass);
	p = (u_int *)tmpnick;
	while(cont--)
	{
		v[0] = (u_int)ntohl(*p++);
		v[1] = (u_int)ntohl(*p++);
		tea(v, k, x);
	}
	inttobase64(clave, x[0], 6);
	inttobase64(clave + 6, x[1], 6);
	Free(nick);
	Free(tmpnick);
	return clave;
}
