/*
 * $Id: socks.c,v 1.1 2005-02-18 22:14:12 Trocotronic Exp $ 
 */

#include "struct.h"
#ifdef USA_ZLIB
#include "zlib.h"
#endif
#include "ircd.h"
int listens[MAX_LISTN];
int listenS = 0;

void encola(DBuf *, char *, int);
int desencola(DBuf *, char *, int);
void envia_cola(Sock *);
char *lee_cola(Sock *);
int completa_conexion(Sock *);

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
	int len;
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
	len = strlen(buf);
#ifdef ZLIB
	if (EsZlib(sck))
		msg = comprime(sck, msg, &len);
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
	while (len > 0)
	{
		copiados = MIN(len, (int)(sizeof(bufc->wslot->data) - (bufc->wchar - bufc->wslot->data)));
		if (copiados <= 0) /* nunca debería ser menor que 0 */
		{
			/* el slot está lleno, preparamos otro */
			DbufData *aux;
			if (copiados < 0)
			{
				fecho(FERR, "copiados es menor que 0. Póngase en contacto con el autor.");
				cierra_colossus(-1);
			}
			da_Malloc(aux, DbufData);
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
void elimina_slot(DBuf *bufc, int bytes)
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
			if (!aux) /* final del trayecto */
				break;
			bufc->rslot = aux;
			bufc->rchar = &aux->data[0];
		}
	}
}
/* copia el nº de bytes indicados y los borra una vez colocados */
int copia_slot(DBuf *bufc, char *dest, int bytes)
{
	int copiados = 0;
	DbufData *aux;
	char *b = dest;
	if (!bufc->rslot)
		return 0;
	while (bytes > 0)
	{
		aux = bufc->rslot->sig;
		copiados = MIN((int)bufc->rslot->len, bytes);
		memcpy(b, bufc->rchar, copiados);
		b += copiados;
		bytes -= copiados;
		elimina_slot(bufc, copiados);
	}
	return b - dest;
}
/* mete en buf una línea completa o hasta que se quede lleno */	
int desencola(DBuf *bufc, char *buf, int bytes)
{
	/* bytes contiene el máximo que podemos copiar */
	DbufData *aux;
	char *s, tmp[4296];
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
				elimina_slot(bufc, 1);
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
	bzero(tmp, sizeof(tmp));
	strncpy(tmp, bufc->rchar, MIN(copiados, bytes));
	i = copia_slot(bufc, buf, MIN(copiados, bytes)+1);
	if (copiados > i)
		elimina_slot(bufc, copiados - i);
	if (i >= 0)
		*(buf + i) = '\0';
	return i;
}
void envia_cola(Sock *sck)
{
	char buf[BUFSIZE], *msg;
	int len = 0;
	if (sck->sendQ && sck->sendQ->len > 0)
	{
		while ((len = desencola(sck->sendQ, &buf[0], sizeof(buf))) > 0) /* hemos copiado una línea entera */
		{
			msg = &buf[0];
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
			if (sck->writefunc)
				sck->writefunc(sck, msg);
		}
	}
}
int lee_mensaje(Sock *sck)
{
	char lee[BUF_SOCK];
	int len = 0;
	if (EsCerr(sck))
		return -3;
	SET_ERRNO(0);
#ifdef USA_SSL
	if (EsSSL(sck))
		len = ircd_SSL_read(sck, lee, sizeof(lee));
	else
#endif
	len = READ_SOCK(sck->pres, lee, sizeof(lee));
	if (len < 0 && ERRNO == P_EWOULDBLOCK)
		return 1;
	if (len > 0)
		encola(sck->recvQ, lee, len);
	return len;
}
void crea_mensaje(Sock *sck, char *msg, int len)
{
	char *p = msg, *c;
#ifdef ZIP_LINKS
	if (EsZip(sck))
	{
		zip = len;
		sck->zlib->incount = 0;
		sck->zlib->inbuf[0] = '\0';
		p = descomprime(sck, msg, &len);
	}
#endif
	if ((c = strchr(p, '\r')))
		*c = '\0';
	if ((c = strchr(p, '\n')))
		*c = '\0';
#ifndef DEBUG
	Debug("[Parseando: %s]", p);
#endif
	if (sck->readfunc)
		sck->readfunc(sck, p);
}
int completa_conexion(Sock *sck)
{
	SockOk(sck);
#ifdef DEBUG
	Debug("Conexion establecida con %s:%i (%i)", sck->host, sck->puerto, sck->pres);
#endif
	if (sck->openfunc)
		sck->openfunc(sck, NULL);
	ircd_log(LOG_CONN, "Conexión establecida con %s:%i", sck->host, sck->puerto);
	return 0;
}
int lee_socks() /* devuelve los bytes leídos */
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
			while (SockActual->recvQ->len > 0)
			{
				char read[BUFSIZE];
				int cop = 0;
				cop = desencola(SockActual->recvQ, read, sizeof(read));
				if (cop > 0)
					crea_mensaje(SockActual, read, cop);
				else /* stop, por lo que sea */
					break;
				if (EsCerr(SockActual))
					break;
			}
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
