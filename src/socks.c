/*
 * $Id: socks.c,v 1.42 2008/02/13 16:16:09 Trocotronic Exp $
 */

#ifdef _WIN32
#include <winsock.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#endif
#include "struct.h"
#ifdef USA_ZLIB
#include "zip.h"
#endif
#include "ircd.h"

int listens[MAX_LISTN];
int listenS = 0;

void Encola(DBuf *, char *, int);
void EnviaCola(Sock *);
char *lee_cola(Sock *);
int CompletaConexion(Sock *);
void LiberaSock(Sock *);
#if defined(NOCORE)
#define DEBUG
#endif

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
//	Debug("Insertando sock %X %i en %i ", sck, sck->pres, ListaSocks.abiertos);
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
 * @desc: Abre una conexi�n.
 Las funciones SOCKFUNC reciben tres par�metros:
 	- Sock *sck: Conexi�n a la que pertenece.
 	- char *data: Datos. En algunos casos puede ser NULL.
 	- u_int len: Longitud v�lida de los datos.
 * @params: $host [in] Host o ip a conectar.
 	    $puerto [in] Puerto remoto.
 	    $openfunc [in] Funci�n a ejecutar cuando se complete la conexi�n. Si no se precisa, usar NULL.
 	    El par�metro <i>data</i> apunta a NULL.
 	    $readfunc [in] Funci�n a ejecutar cuando se reciban datos a trav�s de la conexi�n. Si no se precisa, usar NULL.
 	    El par�metro <i>data</i> apunta a la cadena de datos. Esta funci�n se ejecuta cada vez que se recibe una cadena que termina con \n.
 	    $writefunc [in] Funci�n a ejecutar cuando se escriben datos.
 	    El par�metro <i>data</i> apunta a los datos que se escriben.
 	    $closefunc [in] Funci�n a ejecutar cuando se cierra la conexi�n.
 	    El par�metro <i>data</i> s�lo vale NULL si la desconexi�n es remota.
 * @ret: Devuelve la estructura de conexi�n en caso que haya podido establecerse con �xito. Devuelve NULL si no ha podido conectar.
 * @ex:
 Sock *sck;
 SOCKFUNC(Abrir)
 {
 	printf("Se ha abierto una conexi�n con %s.", sck->host);
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
 * @ver: SockClose SockListen SockOpenEx
 * @cat: Conexiones
 !*/
Sock *SockOpen(char *host, int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc))
{
	return SockOpenEx(host, puerto, openfunc, readfunc, writefunc, closefunc, 30, 0, 0);
}
/*!
 * @desc: Abre una conexi�n avanzada.
 Las funciones SOCKFUNC reciben tres par�metros:
 	- Sock *sck: Conexi�n a la que pertenece.
 	- char *data: Datos. En algunos casos puede ser NULL.
 	- u_int len: Longitud v�lida de los datos.
 * @params: $host [in] Host o ip a conectar.
 	    $puerto [in] Puerto remoto.
 	    $openfunc [in] Funci�n a ejecutar cuando se complete la conexi�n. Si no se precisa, usar NULL.
 	    El par�metro <i>data</i> apunta a NULL.
 	    $readfunc [in] Funci�n a ejecutar cuando se reciban datos a trav�s de la conexi�n. Si no se precisa, usar NULL.
 	    El par�metro <i>data</i> apunta a la cadena de datos. Esta funci�n se ejecuta cada vez que se recibe una cadena que termina con \n.
 	    $writefunc [in] Funci�n a ejecutar cuando se escriben datos.
 	    El par�metro <i>data</i> apunta a los datos que se escriben.
 	    $closefunc [in] Funci�n a ejecutar cuando se cierra la conexi�n.
 	    El par�metro <i>data</i> s�lo vale NULL si la desconexi�n es remota.
 	    $contout [in] N�mero de segundos de timeout de espera a conectar. Si en este tiempo no conecta, se aborta la conexi�n (se ejecuta SockClose local).
 	    $recvout [in] N�mero de segundos de timeout para recibir datos. Si en este tiempo no se reciben datos, se cierra la conexi�n (se ejecuta SockClose remoto).
 	    $opts [in] Opciones:
 	    	- OPT_SSL: La conexi�n es SSL.
 	    	- OPT_NORECVQ: Los datos se env�an por bloques a medida que se leen. No se env�an l�neas que terminen por \r\n sino que se env�an por bloques de 16KB.
 	    	- OPT_NADD: No a�ade el socket en la lista interna de sockets. Deber� gestionar el socket por su cuenta. Si no se a�ade el socket a la lista, no se ejecutan las funciones SockRead.
 	    	- OPT_ZLIB: La conexi�n comprime los datos con ZLIB.
 	    	- OPT_BIN: La conexi�n recibir� datos binarios. No a�adir� \0 al final.
 * @ret: Devuelve la estructura de conexi�n en caso que haya podido establecerse con �xito. Devuelve NULL si no ha podido conectar.
 * @ver: SockClose SockListen SockOpen
 * @cat: Conexiones
 !*/

Sock *SockOpenEx(char *host, int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc), u_int contout, u_int recvtout, u_int opts)
{
	Sock *sck;
	struct in_addr *res;
	if (!(opts & OPT_NADD) && ListaSocks.abiertos == MAXSOCKS)
		return NULL;
	sck = BMalloc(Sock);
	SockDesc(sck);
	if ((sck->pres = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		Free(sck);
		return NULL;
	}
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = htons((u_short)puerto);
	if ((res = Resuelve(host)))
		sck->server.sin_addr = *res;
	SockConn(sck);
	if (SockNoBlock(sck->pres) == -1)
	{
		Free(sck);
		return NULL;
	}
#ifdef _WIN32
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) == -1 &&	WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK)
#else
	if (connect(sck->pres, (struct sockaddr *)&sck->server, sizeof(struct sockaddr)) < 0 && errno != EINPROGRESS)
#endif
	{
		Free(sck);
		return NULL;
	}
	sck->host = strdup(host);
	if (!(opts & OPT_NORECVQ))
		sck->recvQ = BMalloc(DBuf);
	sck->sendQ = BMalloc(DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	sck->puerto = puerto;
	sck->inicio = time(NULL);
	sck->contout = contout;
	sck->recvtout = recvtout;
	sck->opts |= opts;
#ifdef DEBUG
#ifdef USA_SSL
	Debug("Abriendo conexion con %s:%i (%i) %s", host, puerto, sck->pres, EsSSL(sck) ? "[SSL]" : "");
#else
	Debug("Abriendo conexion con %s:%i (%i)", host, puerto, sck->pres);
#endif
#endif
	if (!(opts & OPT_NADD))
		InsertaSock(sck);
	return sck;
}

/*!
 * @desc: Pone en escucha un puerto para aceptar conexiones.
 * @params: $puerto [in] Puerto a escuchar.
 	    $openfunc [in] Funci�n a ejecutar cuando se acepte la conexi�n.
 	    $readfunc [in] Funci�n a ejecutar cuando se reciban datos a trav�s de la nueva conexi�n aceptada.
 	    $writefunc [in] Funci�n a ejecutar cuando se escriban datos en ella.
 	    $closefunc [in] Funci�n a ejecutar cuando se cierre la nueva conexi�n aceptada.
 * @ret: Devuelve el recurso de conexi�n en escucha. Este recurso NO es el mismo que se le pasa cuando se acepta una nueva conexi�n.
 S�lo tiene funci�n puramente de escucha. No puede escribirse o recibir datos a trav�s de ella.
 En caso de error, devuelve NULL. Posiblemente si ocurre un error se deba a que el puerto ya est� en escucha por otra aplicaci�n.
 * @ex:
 Sock *escucha;
 SOCKFUNC(Abre)
 {
 	printf("Se ha conectado un nuevo cliente con direcci�n %s:%i", sck->host, sck->puerto);
 	return 0;
}
if (!(escucha = SockListen(123, Abre, NULL, NULL, NULL)))
{
	printf("No se puede escuchar este puerto. Posiblemente est� siendo escuchado por otra aplicacion.");
	return 0;
}
...
 * @ver: SockOpen SockClose
 * @cat: Conexiones
 !*/

Sock *SockListen(int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc))
{
	return SockListenEx(puerto, openfunc, readfunc, writefunc, closefunc, 0);
}

Sock *SockListenEx(int puerto, SOCKFUNC(*openfunc), SOCKFUNC(*readfunc), SOCKFUNC(*writefunc), SOCKFUNC(*closefunc), u_int opts)
{
	Sock *sck;
	int i;
	int ad[4];
	char ipname[20], *name = conf_server->bind_ip;
	for (i = 0; i < listenS; i++)
	{
		if (listens[i] == puerto)
			return NULL;
	}
	if (ListaSocks.abiertos == MAXSOCKS)
		return NULL;
	if (!name)
		name = "*";
	ad[0] = ad[1] = ad[2] = ad[3] = 0;
	(void)sscanf(name, "%d.%d.%d.%d", &ad[0], &ad[1], &ad[2], &ad[3]);
	(void)ircsprintf(ipname, "%d.%d.%d.%d", ad[0], ad[1], ad[2], ad[3]);
	sck = BMalloc(Sock);
	if ((sck->pres = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		Free(sck);
		return NULL;
	}
	sck->server.sin_family = AF_INET;
	sck->server.sin_addr.s_addr = inet_addr(ipname);
	sck->server.sin_port = htons((u_short)puerto);
	sck->puerto = puerto;
	if (bind(sck->pres, (struct sockaddr *)&sck->server, sizeof(sck->server)) == -1)
	{
		CLOSE_SOCK(sck->pres);
		Free(sck);
		return NULL;
	}
	SockList(sck);
	if (SockNoBlock(sck->pres) == -1)
	{
		CLOSE_SOCK(sck->pres);
		Free(sck);
		return NULL;
	}
	sck->host = strdup("127.0.0.1");
#ifdef DEBUG
	Debug("Escuchando puerto %i (%i)", puerto, sck->pres);
#endif
	if (!(opts & OPT_NORECVQ))
		sck->recvQ = BMalloc(DBuf);
	sck->sendQ = BMalloc(DBuf);
	sck->openfunc = openfunc;
	sck->readfunc = readfunc;
	sck->writefunc = writefunc;
	sck->closefunc = closefunc;
	sck->opts |= opts;
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
	sck = BMalloc(Sock);
	sck->pres = pres;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = list->server.sin_port;
	memcpy(&sck->server.sin_addr, &addr.sin_addr, sizeof(struct sockaddr_in));
	sck->puerto = list->puerto;
	if (SockNoBlock(sck->pres) == -1)
	{
		CLOSE_SOCK(sck->pres);
		Free(sck);
		return NULL;
	}
	sck->host = strdup(inet_ntoa(addr.sin_addr));
	if (!(list->opts & OPT_NORECVQ))
		sck->recvQ = BMalloc(DBuf);
	sck->sendQ = BMalloc(DBuf);
	sck->openfunc = list->openfunc;
	sck->readfunc = list->readfunc;
	sck->writefunc = list->writefunc;
	sck->closefunc = list->closefunc;
	sck->opts |= list->opts;
	InsertaSock(sck);
#ifdef USA_SSL
	if (EsSSL(list))
	{
		SetSSLAcceptHandshake(sck);
		if (!(sck->ssl = SSL_new(ctx_server)))
		{
			SockClose(sck);
			return NULL;
		}
		SSL_set_fd(sck->ssl, pres);
		SSLSockNoBlock(sck->ssl);
		if (!SSLAccept(sck, pres))
		{
			SSL_set_shutdown(sck->ssl, SSL_RECEIVED_SHUTDOWN);
			SSLShutDown(sck->ssl);
			SSL_free(sck->ssl);
			SockClose(sck);
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
		sck->writefunc(sck, buf, strlen(buf));
	if (opts & OPT_CR)
		strlcat(buf, "\r", sizeof(buf));
	if (opts & OPT_LF)
		strlcat(buf, "\n", sizeof(buf));
	len = strlen(buf);
#ifdef USA_ZLIB
	if (EsZlib(sck))
		msg = ZLibComprime(sck, msg, &len, 0);
	if (len)
#endif
	Encola(sck->sendQ, msg, len);
}

void SockWriteEx(Sock *sck, int opts, char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	SockWriteExVL(sck, opts, formato, vl);
	va_end(vl);
}
void SockWriteBin(Sock *sck, u_long len, char *data)
{
	Encola(sck->sendQ, data, len);
#ifdef DEBUG
	Debug("[Enviando: %s]", data);
#endif
}

/*!
 * @desc: Escribe datos en una conexi�n a partir de una lista de argumentos. Al final se a�ade \r\n
 * @params: $sck [in] Conexi�n a la que se desean enviar datos.
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
 * @desc: Escribe datos en una conexi�n a partir de una cadena y argumentos variables. Al final se a�ade \r\n.
 * @params: $sck [in] Conexi�n a la que se desean enviar datos.
 	    $formato [in] Cadena con formato que se desea enviar.
 	    $... [in] Argumentos variables seg�n cadena con formato.
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
 * @desc: Cierra una conexi�n.
 * @params: $sck [in] Conexi�n a cerrar.
 * @ver: SockOpen SockListen
 * @cat: Conexiones
 !*/

void SockClose(Sock *sck)
{
	SockCloseEx(sck, LOCAL);
}

/*!
 * @desc: Cierra una conexi�n con par�metros extra.
 * @params: $sck [in] Conexi�n a cerrar.
 	    $closef [in] Forma de cierre:
 	    	- LOCAL: Desconexi�n local. Recomendado.
 	    	- REMOTO: Desconexi�n remota.
 * @ver: SockOpen SockListen
 * @cat: Conexiones
 !*/

void SockCloseEx(Sock *sck, int closef)
{
	if (!sck || EsCerr(sck))
		return;
	if (EsList(sck))
	{
#ifdef DEBUG
		Debug("Cerrando conexion de escucha en puerto %i (%i)", sck->puerto, sck->pres);
#endif
		if (sck->closefunc)
			sck->closefunc(sck, closef ? NULL : "LOCAL", 0);
		LiberaSock(sck);
	}
	else
	{
		if (closef == LOCAL)
			EnviaCola(sck);
		else if (sck->pos)
		{
#ifdef DEBUG
			if (!(sck->opts & OPT_BIN))
				Debug("[Parseando: %s]", sck->buffer);
#endif
			if (sck->readfunc)
				sck->readfunc(sck, sck->buffer, sck->pos);
			sck->pos = 0;
			sck->buffer[0] = '\0';
		}
		if (sck->closefunc)
			sck->closefunc(sck, closef ? NULL : "LOCAL", 0);
		SockCerr(sck);
#ifdef DEBUG
		Debug("Cerrando conexion con %s:%i (%i) [%s]", sck->host, sck->puerto, sck->pres, closef == LOCAL ? "LOCAL" : "REMOTO");
#endif
	}
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
	for (i = ListaSocks.tope; i > -1; i--)
	{
		if (ListaSocks.socket[i])
			LiberaSock(ListaSocks.socket[i]);
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
		bufc->wslot = BMalloc(DbufData);
		bufc->rslot = bufc->wslot;
		bufc->rchar = &bufc->rslot->data[0];
		bufc->wchar = &bufc->wslot->data[0];
		//Debug("Abriendo slot 0");
		bufc->slots++;
	}
	while (len > 0)
	{
		copiados = MIN(len, (int)(sizeof(bufc->wslot->data) - (bufc->wchar - bufc->wslot->data)));
		if (copiados <= 0) /* nunca deber�a ser menor que 0 */
		{
			/* el slot est� lleno, preparamos otro */
			DbufData *aux;
			if (copiados < 0)
			{
				Alerta(FERR, "copiados es menor que 0. P�ngase en contacto con el autor.");
				CierraColossus(-1);
			}
			aux = BMalloc(DbufData);
			bufc->wslot->sig = aux;
			bufc->wslot = aux;
			bufc->wchar = &bufc->wslot->data[0];
			//Debug("Abriendo slot %i %i", bufc->slots,len);
			bufc->slots++;
			/* se copiar� todo en la siguiente iteraci�n */
			continue;
		}
		//Debug("%% %X %X",((Cliente *)uTab[280].item) ? ((Cliente *)uTab[280].item)->nombre: "nul",bufc->wchar);
		memcpy(bufc->wchar, str, copiados);
		//Debug("%i %s", copiados, str);
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
/* copia el n� de bytes indicados y los borra una vez colocados */
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
void EnviaCola(Sock *sck)
{
	char buf[SOCKBUF], *msg;
	int len = 0, ret;
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
			do
			{
				msg = &buf[0];
#ifdef USA_SSL
				if (EsSSL(sck))
					ret = SSLSockWrite(sck, msg, len);
				else
#endif
				ret = WRITE_SOCK(sck->pres, msg, len);
#ifdef _WIN32
			}while(ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK);
#else
			}while(ret == -1);
#endif
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
		len = SSLSockRead(sck, lee, sizeof(lee)-1);
	else
#endif
	len = READ_SOCK(sck->pres, lee, sizeof(lee)-1);
	if (len < 0 && ERRNO == P_EWOULDBLOCK)
		return 1;
	if (len > 0)
	{
		if (EsNoRecvQ(sck))
		{
			sck->pos = MIN(len, sizeof(sck->buffer)-1);
			memcpy(sck->buffer, lee, sck->pos);
			if (!(sck->opts & OPT_BIN))
				sck->buffer[sck->pos] = '\0';
			if (sck->readfunc)
				sck->readfunc(sck, sck->buffer, sck->pos);
			sck->pos = 0;
			sck->buffer[0] = '\0';
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
				if (!(sck->opts & OPT_BIN))
				{
					*b = '\0';
#ifdef DEBUG
					Debug("[Parseando: %s]", sck->buffer);
#endif
				}
				if (sck->readfunc)
					sck->readfunc(sck, sck->buffer, sck->pos);
				sck->pos = 0;
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
	if (!(sck->opts & OPT_BIN))
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
		sck->openfunc(sck, NULL, 0);
	Loguea(LOG_CONN, "Conexi�n establecida con %s:%i", sck->host, sck->puerto);
	return 0;
}
int LeeSocks() /* devuelve los bytes le�dos */
{
	int i, len, sels, lee = 0;
	fd_set read_set, write_set, excpt_set;
	struct timeval wait;
	Sock *sck;
	time_t ahora;
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&excpt_set);
	ahora = time(0);
	for (i = ListaSocks.tope; i >= 0; i--)
	{
		if (!(sck = ListaSocks.socket[i]))
			continue;
		if (sck->pres >= 0)
		{
			if ((EsConn(sck) && ((time_t)(sck->inicio + sck->contout) < ahora)) || (EsOk(sck) && sck->recvtout && (time_t)(sck->recibido + sck->recvtout) < ahora))
				SockClose(sck);
			if (EsCerr(sck))
				LiberaSock(sck);
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
				SockCloseEx(SockActual, REMOTO);
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
				/*if (!SSL_is_init_finished(SockActual->ssl))
				{
					if (EsCerr(SockActual) || EsSSLAcceptHandshake(SockActual) ? !SSLAccept(SockActual, SockActual->pres) : SSLConnect(SockActual) < 0)
						len = -2;
				}
				else*/
					CompletaConexion(SockActual);
			}
#endif
			if (len <= 0)
			{
				sels--;
				FD_CLR(SockActual->pres, &read_set);
				SockCloseEx(SockActual, REMOTO);
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
			SockCloseEx(SockActual, REMOTO);
			continue;
		}
	}
	return lee;
}
