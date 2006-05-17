/*
 * $Id: socks.c,v 1.19 2006-05-17 14:27:45 Trocotronic Exp $ 
 */

#include "struct.h"
#ifdef USA_ZLIB
#include "zip.h"
#endif
#include "ircd.h"
#ifndef _WIN32
#include <errno.h>
#endif
int listens[MAX_LISTN];
int listenS = 0;

void Encola(DBuf *, char *, int);
int Desencola(DBuf *, char *, int);
void EnviaCola(Sock *);
char *lee_cola(Sock *);
int CompletaConexion(Sock *);
//#define DEBUG

/*
 * resolv
 * Resvuelve el host dado
 * NULL si no se pudo resolver
 */
struct in_addr *Resuelve(char *host)
{
	struct hostent *he;
	if (BadPtr(host))
		return NULL;
	if (!(he = gethostbyname(host))) 
		return NULL;
	else 
		return (struct in_addr *)he->h_addr;
}
int SockNoBlock(int pres)
{
#ifndef _WIN32
	int res;
#endif
	int nonb = 0;
#ifdef	NBLOCK_POSIX
	nonb |= O_NONBLOCK;
#endif
#ifdef	NBLOCK_BSD
	nonb |= O_NDELAY;
#endif
#ifdef	NBLOCK_SYSV
	res = 1;
	if (ioctl(pres, FIONBIO, &res) < 0)
		return -1;
#else
  #if !defined(_WIN32)
	if ((res = fcntl(pres, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl(pres, F_SETFL, res | nonb) == -1)
		return -2;
  #else
	nonb = 1;
	if (ioctlsocket(pres, FIONBIO, &nonb) < 0)
		return -1;
  #endif
#endif
	return 0;
}
void InsertaSock(Sock *sck)
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
void BorraSock(Sock *sck)
{
	ListaSocks.socket[sck->slot] = NULL;
	sck->slot = -1;
	while (!ListaSocks.socket[ListaSocks.tope])
		ListaSocks.tope--;
	ListaSocks.abiertos--;
}
/*!
 * @desc: Abre una conexión.
 Las funciones SOCKFUNC reciben dos parámetros:
 	- Sock *sck: Conexión a la que pertenece.
 	- char *data: Datos. En algunos casos puede ser NULL.
 * @params: $host [in] Host o ip a conectar.
 	    $puerto [in] Puerto remoto. Si es negativo, indica que se usa bajo SSL.
 	    $openfunc [in] Función a ejecutar cuando se complete la conexión. Si no se precisa, usar NULL.
 	    El parámetro <i>data</i> apunta a NULL.
 	    $readfunc [in] Función a ejecutar cuando se reciban datos a través de la conexión. Si no se precisa, usar NULL.
 	    El parámetro <i>data</i> apunta a la cadena de datos. Esta función se ejecuta cada vez que se recibe una cadena que termina con \n.
 	    $writefunc [in] Función a ejecutar cuando se escriben datos.
 	    El parámetro <i>data</i> apunta a los datos que se escriben.
 	    $closefunc [in] Función a ejecutar cuando se cierra la conexión.
 	    El parámetro <i>data</i> sólo vale NULL si la desconexión es remota.
 	    $add [in] Si vale 1 se añade a la lista de proceso interna de sockets. Se recomienda extremadamente usar 1.
 * @ret: Devuelve la estructura de conexión en caso que haya podido establecerse con éxito. Devuelve NULL si no ha podido conectar.
 * @ex:
 Sock *sck;
 SOCKFUNC(Abrir)
 {
 	printf("Se ha abierto una conexión con %s.", sck->host);
 	return 0;
}
 SOCKFUNC(Cerrar)
 {
 	if (data)
 		printf("Han desconectado!");
 	else
 		printf("Has desconectado voluntariamente");
 	return 0;
}
 SOCKFUNC(Leer)
 {
 	printf("He recibido estos datos: %s", data);
 	return 0;
}
...
if (!(sck = SockOpen("1.2.3.4", 123, Abrir, Leer, NULL, Cerrar)))
{
	printf("Ha sido imposible conectar");
	return;
}
...
 * @ver: SockClose SockListen
 * @cat: Conexiones
 !*/
Sock *SockOpen(char *host, int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc))
{
	return SockOpenEx(host, puerto, openfunc, readfunc, writefunc, closefunc, 30, 0, 0);
}

Sock *SockOpenEx(char *host, int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc), u_int contout, u_int recvtout, u_int opts)
{
	Sock *sck;
	struct in_addr *res;
	if (!(opts & OPT_NADD) && ListaSocks.abiertos == MAXSOCKS)
		return NULL;
	BMalloc(sck, Sock);
	SockDesc(sck);
#ifdef USA_SSL
	if (puerto < 0) /* es un puerto ssl */
	{
		puerto = puerto * -1;
		SetSSL(sck);
	}
#endif
	if ((sck->pres = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return NULL;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = htons((u_short)puerto);
	if ((res = Resuelve(host)))
		sck->server.sin_addr = *res;
	sck->host = strdup(host);
	SockConn(sck);
	if (SockNoBlock(sck->pres) == -1)
		return NULL;
#ifdef _WIN32
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) == -1 &&
		WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK)
#else
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) < 0 && errno != EINPROGRESS)
#endif			
			return NULL;
	if (opts & OPT_NORECVQ)
		SetNoRecvQ(sck);
	else
		BMalloc(sck->recvQ, DBuf);
	BMalloc(sck->sendQ, DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	sck->puerto = puerto;
	sck->inicio = time(NULL);
	sck->contout = contout;
	sck->recvtout = recvtout;
#ifdef DEBUG
	Debug("Abriendo conexion con %s:%i (%i) %s", host, puerto, sck->pres, EsSSL(sck) ? "[SSL]" : "");
#endif
	if (!(opts & OPT_NADD))
		InsertaSock(sck);
	return sck;
}

/*!
 * @desc: Pone en escucha un puerto para aceptar conexiones.
 * @params: $puerto [in] Puerto a escuchar.
 	    $openfunc [in] Función a ejecutar cuando se acepte la conexión.
 	    $readfunc [in] Función a ejecutar cuando se reciban datos a través de la nueva conexión aceptada.
 	    $writefunc [in] Función a ejecutar cuando se escriban datos en ella.
 	    $closefunc [in] Función a ejecutar cuando se cierre la nueva conexión aceptada.
 * @ret: Devuelve el recurso de conexión en escucha. Este recurso NO es el mismo que se le pasa cuando se acepta una nueva conexión.
 Sólo tiene función puramente de escucha. No puede escribirse o recibir datos a través de ella.
 En caso de error, devuelve NULL. Posiblemente si ocurre un error se deba a que el puerto ya esté en escucha por otra aplicación.
 * @ex:
 Sock *escucha;
 SOCKFUNC(Abre)
 {
 	printf("Se ha conectado un nuevo cliente con dirección %s:%i", sck->host, sck->puerto);
 	return 0;
}
if (!(escucha = SockListen(123, Abre, NULL, NULL, NULL)))
{
	printf("No se puede escuchar este puerto. Posiblemente esté siendo escuchado por otra aplicacion.");
	return 0;
}
...
 * @ver: SockOpen SockClose
 * @cat: Conexiones
 !*/
 
Sock *SockListen(int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc))
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
	(void)ircsprintf(ipname, "%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
	BMalloc(sck, Sock);
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
	if (SockNoBlock(sck->pres) == -1)
		return NULL;
#ifdef DEBUG
	Debug("Escuchando puerto %i (%i)", puerto, sck->pres);
#endif
	BMalloc(sck->recvQ, DBuf);
	BMalloc(sck->sendQ, DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	listen(sck->pres, LISTEN_SIZE);
	listens[listenS++] = puerto;
	InsertaSock(sck);
	return sck;
}
Sock *SockAccept(Sock *list, int pres)
{
	Sock *sck;
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr);
	if (getpeername(pres, (struct sockaddr *)&addr, &len) == -1)
	{
		CLOSE_SOCK(pres);
		return NULL;
	}
	BMalloc(sck, Sock);
	sck->pres = pres;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = list->server.sin_port;
	memcpy(&sck->server.sin_addr, &addr.sin_addr, sizeof(struct sockaddr_in));
	sck->host = strdup(inet_ntoa(addr.sin_addr));
	sck->puerto = list->puerto;
	if (SockNoBlock(sck->pres) == -1)
	{
		Free(sck);
		return NULL;
	}
	BMalloc(sck->recvQ, DBuf);
	BMalloc(sck->sendQ, DBuf);
	sck->openfunc = list->openfunc;
	sck->readfunc = list->readfunc;
	sck->writefunc = list->writefunc;
	sck->closefunc = list->closefunc;
	InsertaSock(sck);
#ifdef USA_SSL
	if (EsSSL(list))
	{
		SetSSLAcceptHandshake(sck);
		if (!(sck->ssl = SSL_new(ctx_server)))
		{
			SockClose(sck, LOCAL);
			return NULL;
		}
		SetSSL(sck);
		SSL_set_fd(sck->ssl, pres);
		SSLSockNoBlock(sck->ssl);
		if (!SSLAccept(sck, pres)) 
		{
			SSL_set_shutdown(sck->ssl, SSL_RECEIVED_SHUTDOWN);
			SSLShutDown(sck->ssl);
			SSL_free(sck->ssl);
			SockClose(sck, LOCAL);
			return NULL;
		}
	}
	else
#endif
	CompletaConexion(sck);
	return sck;
}

void SockWriteExVL(Sock *sck, int opts, char *formato, va_list vl)
{
	char buf[SOCKBUF], *msg;
	int len;
	if (!sck)
		return;
	msg = buf;
	ircvsprintf(buf, formato, vl);
#ifdef DEBUG
	Debug("[Enviando: %s]", buf);
#endif
	if (sck->writefunc && buf[0])
		sck->writefunc(sck, buf);
	if (opts & OPT_CR)
		strcat(buf, "\r");
	if (opts & OPT_LF)
		strcat(buf, "\n");
	len = strlen(buf);
#ifdef USA_ZLIB
	if (EsZlib(sck))
		msg = ZLibComprime(sck, msg, &len, 0);
	if (len)
#endif
	Encola(sck->sendQ, msg, len); /* de momento siempre son cadenas */
}

void SockWriteEx(Sock *sck, int opts, char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	SockWriteExVL(sck, opts, formato, vl);
	va_end(vl);
}

/*!
 * @desc: Escribe datos en una conexión a partir de una lista de argumentos.
 * @params: $sck [in] Conexión a la que se desean enviar datos.
 	    $formato [in] Cadena con formato que se desea enviar.
 	    $vl [in] Lista de argumentos.
 * @ver: SockWrite
 * @cat: Conexiones
 !*/
 
void SockWriteVL(Sock *sck, char *formato, va_list vl)
{
	SockWriteExVL(sck, OPT_CRLF, formato, vl);
}

/*!
 * @desc: Escribe datos en una conexión a partir de una cadena y argumentos variables.
 * @params: $sck [in] Conexión a la que se desean enviar datos.
 	    $formato [in] Cadena con formato que se desea enviar.
 	    $... [in] Argumentos variables según cadena con formato.
 * @ver: SockWriteVL
 * @cat: Conexiones
 !*/
 
void SockWrite(Sock *sck, char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	SockWriteExVL(sck, OPT_CRLF, formato, vl);
	va_end(vl);
}

/*!
 * @desc: Cierra una conexión.
 * @params: $sck [in] Conexión a cerrar.
 	    $closef [in] Forma de cierre:
 	    	- LOCAL: Desconexión local. Recomendado.
 	    	- REMOTO: Desconexión remota.
 * @ver: SockOpen SockListen
 * @cat: Conexiones
 !*/
 
void SockClose(Sock *sck, char closef)
{
	if (!sck || EsCerr(sck))
		return;
	if (closef == LOCAL)
		EnviaCola(sck); /* enviamos toda su cola */
	else if (sck->pos)
	{
#ifdef DEBUG
		Debug("[Parseando: %s]", sck->buffer);
#endif
		if (sck->readfunc) /* nos ha quedado algo en el buffer, lo enviamos; pero solo si es remoto */
			sck->readfunc(sck, sck->buffer);
	}
	SockCerr(sck);
#ifdef DEBUG
	Debug("Cerrando conexion con %s:%i (%i) [%s]", sck->host, sck->puerto, sck->pres, closef == LOCAL ? "LOCAL" : "REMOTO");
#endif
	if (sck->closefunc)
		sck->closefunc(sck, closef ? NULL : "LOCAL");
}
void LiberaSock(Sock *sck)
{
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
		CLOSE_SOCK(sck->pres);
#ifdef USA_SSL
	if (EsSSL(sck) && sck->ssl) 
	{
		SSL_set_shutdown((SSL *)sck->ssl, SSL_RECEIVED_SHUTDOWN);
		SSLShutDown((SSL *)sck->ssl);
		SSL_free((SSL *)sck->ssl);
		sck->ssl = NULL;
	}
#endif
	BorraSock(sck);
	if (!EsNoRecvQ(sck))
		ircfree(sck->recvQ);
	ircfree(sck->sendQ);
	ircfree(sck->host);
	ircfree(sck);
}
void CierraSocks()
{
	int i;
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (ListaSocks.socket[i] && ListaSocks.socket[i]->pres >= 0)
		{
			CLOSE_SOCK(ListaSocks.socket[i]->pres);
			ListaSocks.socket[i]->pres = -2;
		}
	}
	ListaSocks.tope = -1;
#ifdef _WIN32
	WSACleanup();
#endif
}
void Encola(DBuf *bufc, char *str, int bytes)
{
	int copiados, len = bytes ? bytes : strlen(str);
	//Debug("Len %i (%i %i %X)",len,bufc->len,bufc->wslot ? bufc->wslot->len : 0,bufc->wslot);
	if (!bufc->slots)
	{
		BMalloc(bufc->wslot, DbufData);
		bufc->rslot = bufc->wslot;
		bufc->rchar = &bufc->rslot->data[0];
		bufc->wchar = &bufc->wslot->data[0];
		//Debug("Abriendo slot 0");
		bufc->slots++;
	}
	while (len > 0)
	{
		copiados = MIN(len, (int)(sizeof(bufc->wslot->data) - (bufc->wchar - bufc->wslot->data)));
		if (copiados <= 0) /* nunca debería ser menor que 0 */
		{
			/* el slot está lleno, preparamos otro */
			DbufData *aux;
			if (copiados < 0)
			{
				Alerta(FERR, "copiados es menor que 0. Póngase en contacto con el autor.");
				CierraColossus(-1);
			}
			BMalloc(aux, DbufData);
			bufc->wslot->sig = aux;
			bufc->wslot = aux;
			bufc->wchar = &bufc->wslot->data[0];
			//Debug("Abriendo slot %i", bufc->slots);
			bufc->slots++;
			/* se copiará todo en la siguiente iteración */
			continue;
		}
		//Debug("%% %X %X",((Cliente *)uTab[280].item) ? ((Cliente *)uTab[280].item)->nombre: "nul",bufc->wchar);
		memcpy(bufc->wchar, str, copiados);
		//Debug("%% %X %X",((Cliente *)uTab[280].item) ? ((Cliente *)uTab[280].item)->nombre: "nul",bufc->wchar);
		str += copiados;
		bufc->wchar += copiados;
		len -= copiados;
		bufc->wslot->len += copiados;
		bufc->len += copiados;
	}
}
void EliminaSlot(DBuf *bufc, int bytes)
{
	DbufData *aux;
	int elim = 0;
	if (bytes > (int)bufc->len)
		bytes = bufc->len;
	while (bytes > 0)
	{
		elim = MIN((int)bufc->rslot->len, bytes);
		bufc->rchar += elim;
		bufc->rslot->len -= elim;
		bufc->len -= elim;
		bytes -= elim;
		if (bufc->rslot->len <= 0) /* se fini */
		{
			aux = bufc->rslot->sig;
			Free(bufc->rslot);
			bufc->slots--;
			bufc->rslot = aux;
			if (!aux) /* final del trayecto */
			{
				bufc->rchar = NULL;
				break;
			}
			bufc->rchar = &aux->data[0];
		}
	}
}
/* copia el nº de bytes indicados y los borra una vez colocados */
int CopiaSlot(DBuf *bufc, char *dest, int bytes)
{
	int copiados = 0;
	char *b = dest;
	if (!bufc->rslot)
		return 0;
	while (bytes > 0 && bufc->rslot)
	{
		copiados = MIN((int)bufc->rslot->len, bytes);
		memcpy(b, bufc->rchar, copiados);
		b += copiados;
		bytes -= copiados;
		EliminaSlot(bufc, copiados);
	}
	return b - dest;
}
/* mete en buf una línea completa o hasta que se quede lleno */	
int Desencola(DBuf *bufc, char *buf, int bytes)
{
	/* bytes contiene el máximo que podemos copiar */
	DbufData *aux;
	char *s;
	int len, copiados, i;
	inicio:
	if (!bufc)
		return 0;
	if (!bufc->slots || !bufc->rslot)
		return 0;
	aux = bufc->rslot;
	s = bufc->rchar;
	len = bufc->len;
	copiados = i = 0;
	if ((i = bufc->rslot->len) > len)
		i = len;
	*buf = 0;
	while (len > 0 && bytes > 0)
	{
		len--;
		bytes--;
		if (*s == '\n' || *s == '\r')
		{
			copiados = bufc->len - len;
			if (copiados == 1)
			{
				EliminaSlot(bufc, 1);
				goto inicio;
			}
			break;
		}
		if (!--i)
		{
			if (aux && (aux = aux->sig))
			{
				s = aux->data;
				i = MIN(DBUF, aux->len);
			}
			else if (!aux)
				break;
		}
		else
			s++;
	}
	if (copiados <= 0)
		return 0;
	i = CopiaSlot(bufc, buf, MIN(copiados, bytes)+1);
	if (copiados > i)
		EliminaSlot(bufc, copiados - i);
	if (i >= 0)
		*(buf + i) = '\0';
	return i;
}
void EnviaCola(Sock *sck)
{
	char buf[SOCKBUF], *msg;
	int len = 0;
#ifdef USA_ZLIB
	int mas = 0;
	if (EsZlib(sck) && sck->zlib->outcount)
	{
		if (sck->sendQ && sck->sendQ->len > 0)
			mas = 1;
		else
		{
			msg = ZLibComprime(sck, NULL, &len, 1);
			if (len == -1)
			{
				Info("No puede hacer ZLibComprime() (1)");
				return;
			}
			Encola(sck->sendQ, msg, len);
		}
	}
#endif
	if (sck->sendQ && sck->sendQ->len > 0)
	{
		while ((len = CopiaSlot(sck->sendQ, buf, sizeof(buf))) > 0)
		{
			msg = &buf[0];
#ifdef USA_SSL
			if (EsSSL(sck))
				SSLSockWrite(sck, msg, len);
			else
#endif
			WRITE_SOCK(sck->pres, msg, len);
		}
#ifdef USA_ZLIB
		if (!sck->sendQ->len && mas)
		{
			mas = 0;
			msg = ZLibComprime(sck, NULL, &len, 1);
			if (len == -1)
			{
				Info("No puede hacer ZLibComprime() (2)");
				return;
			}
			Encola(sck->sendQ, msg, len);
		}
#endif
	}
}
int LeeMensaje(Sock *sck)
{
	char lee[SOCKBUF];
	int len = 0;
	if (EsCerr(sck))
		return -3;
	SET_ERRNO(0);
#ifdef USA_SSL
	if (EsSSL(sck))
		len = SSLSockRead(sck, lee, sizeof(lee));
	else
#endif
	len = READ_SOCK(sck->pres, lee, sizeof(lee));
	if (len < 0 && ERRNO == P_EWOULDBLOCK)
		return 1;
	if (len > 0)
	{
		if (EsNoRecvQ(sck))
		{
			strncpy(sck->buffer, lee, MIN(len, sizeof(sck->buffer)));
			sck->buffer[len] = '\0';
			if (sck->readfunc)
				sck->readfunc(sck, sck->buffer);
		}
		else
			Encola(sck->recvQ, lee, len);
	}
	return len;
}
int CreaMensaje(Sock *sck, char *msg, int len)
{
#ifdef USA_ZLIB
	int zip = 0, hecho = 0;
#endif
	char *p, *b;
	p = msg;
	b = sck->buffer + sck->pos;
#ifdef USA_ZLIB
	if (EsZlib(sck) && sck->zlib->primero)
	{
		if (*p == '\n' || *p == '\r')
		{
			p++;
			len--;
		}
		sck->zlib->primero = 0;
	}
	else
		hecho = 1;
	if (EsZlib(sck))
	{
		zip = len;
		sck->zlib->inbuf[0] = '\0';
		sck->zlib->incount = 0;
		p = ZLibDescomprime(sck, p, &zip);
		len = zip;
		zip = 1;
		if (len == -1)
			return Info("Ha sido imposible hacer ZLibDescomprime() (1)");
	}
	do
	{
#endif
		while(--len >= 0)
		{
			char g = (*b = *p++);
			if (g == '\n' || g == '\r') /* asumo que terminan en \r\n o \r o \n, pero nunca \n\r */
			{
				if (b == sck->buffer) 
					continue;
				*b = '\0';
				sck->pos = 0;
#ifdef DEBUG
				Debug("[Parseando: %s]", sck->buffer);
#endif
				if (sck->readfunc)
					sck->readfunc(sck, sck->buffer);
				if (EsCerr(sck))
					return -1;
#ifdef USA_ZLIB
				if (EsZlib(sck) && zip == 0 && len > 0)
				{
					zip = len;
					if (zip > 0 && (*p == '\n' || *p == '\r'))
					{
						p++;
						zip--;
					}
					if (zip > 0)
					{
						sck->zlib->primero = 0;
						p = ZLibDescomprime(sck, p, &zip);
						len = zip;
						zip = 1;
						if (len == -1)
							return Info("Ha sido imposible hacer ZLibDescomprime() (2)");
					}
				}
#endif
				b = sck->buffer;
			}
			else if (b < sck->buffer + sizeof(sck->buffer) - 1)
				b++;
		}
#ifdef USA_ZLIB
		if (EsZlib(sck) && sck->zlib->incount)
		{
			p = ZLibDescomprime(sck, (char *)NULL, &zip);
			len = zip;
			zip = 1;
			if (len == -1)
				return Info("Ha sido imposible hacer ZLibDescomprime() (3)");
			b = p + len;
			hecho = 0;
		}
		else
			hecho = 1;
	} while(!hecho);
#endif
	sck->pos = b - sck->buffer;
	sck->buffer[sck->pos] = '\0';
	return 0;
}
int CompletaConexion(Sock *sck)
{
	SockOk(sck);
#ifdef DEBUG
	Debug("Conexion establecida con %s:%i (%i)", sck->host, sck->puerto, sck->pres);
#endif
	if (sck->openfunc)
		sck->openfunc(sck, NULL);
	Loguea(LOG_CONN, "Conexión establecida con %s:%i", sck->host, sck->puerto);
	return 0;
}
int LeeSocks() /* devuelve los bytes leídos */
{
	int i, len, sels, lee = 0;
	fd_set read_set, write_set, excpt_set;
	struct timeval wait;
	Sock *sck;
	time_t ahora;
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&excpt_set);
	time(&ahora);
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (!(sck = ListaSocks.socket[i]))
			continue;
		if (EsCerr(sck))
		{
			LiberaSock(sck);
			continue;
		}
		if (sck->pres >= 0)
		{
			if ((EsConn(sck) && ((time_t)(sck->inicio + sck->contout) < ahora)) || (EsOk(sck) && sck->recvtout && (time_t)(sck->recibido + sck->recvtout) < ahora))
				SockClose(sck, LOCAL);
			else
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
				SockAccept(SockActual, pres);
		}
	}
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (!sels)
			break;
		if (!(SockActual = ListaSocks.socket[i]))
			continue;
		if (EsCerr(SockActual))
		{
			LiberaSock(SockActual);
			continue;
		}
		if (FD_ISSET(SockActual->pres, &write_set))
		{
			int err = 0;
			if (EsConn(SockActual))
			{
#ifdef USA_SSL
				if (EsSSL(SockActual))
					err = SSLClienteConexion(SockActual);
				else
#endif
					err = CompletaConexion(SockActual);
				SockActual->recibido = ahora;
			}
			else
				EnviaCola(SockActual);
			if (EsCerr(SockActual) || err)
			{
				if (FD_ISSET(SockActual->pres, &read_set))
				{
					sels--;
					FD_CLR(SockActual->pres, &read_set);
				}
				SockClose(SockActual, REMOTO);
				continue;
			}
		}
		if (FD_ISSET(SockActual->pres, &read_set) && !EsCerr(SockActual))
		{
			len = 1;
#ifdef USA_SSL
			if (!(EsSSLAcceptHandshake(SockActual) || EsSSLConnectHandshake(SockActual)))
#endif
			len = LeeMensaje(SockActual); /* leemos el mensaje */
#ifdef USA_SSL
			if (SockActual->ssl && (EsSSLAcceptHandshake(SockActual) || EsSSLConnectHandshake(SockActual)))
			{
				if (!SSL_is_init_finished(SockActual->ssl))
				{
					if (EsCerr(SockActual) || EsSSLAcceptHandshake(SockActual) ? !SSLAccept(SockActual, SockActual->pres) : SSLConnect(SockActual) < 0)
						len = -2;
				}
				else
					CompletaConexion(SockActual);
			}
#endif
			if (len <= 0)
			{
				sels--;
				FD_CLR(SockActual->pres, &read_set);
				SockClose(SockActual, REMOTO);
				continue;
			}
			lee += len;
			SockActual->recibido = ahora;
			if (!EsNoRecvQ(SockActual))
			{
				while (SockActual->recvQ->len > 0)
				{
					char read[SOCKBUF];
					int cop = 0;
					cop = CopiaSlot(SockActual->recvQ, read, sizeof(read));
					//Debug("&&&& %i", cop);
					//Debug("Creando mensaje %X %s", SockActual, read);
					if (cop > 0)
					{
						if (CreaMensaje(SockActual, read, cop) < 0)
							break;
					}
					else /* stop, por lo que sea */
						break;
				}
			}
			sels--;
		}
		if (FD_ISSET(SockActual->pres, &excpt_set))
		{
			sels--;
			FD_CLR(SockActual->pres, &excpt_set);
			SockClose(SockActual, REMOTO);
			continue;
		}
	}
	return lee;
}
