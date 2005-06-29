/*
 * $Id: ircd.c,v 1.26 2005-06-29 21:13:51 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifdef UDB
#include "bdd.h"
#endif

time_t inicio;
char buf[BUFSIZE];
char backupbuf[BUFSIZE];
Cliente me;
Modulo *modulos;
Cliente *linkado = NULL;
int intentos = 0;
Cliente *clientes = NULL;
Canal *canales;
char **margv;
Sock *SockIrcd = NULL, *IrcdEscucha = NULL;
char reset = 0;
SOCKFUNC(EscuchaAbre);
Canal *canal_debug = NULL;
Protocolo *protocolo = NULL;
Comando *comandos = NULL;
char modebuf[BUFSIZE], parabuf[BUFSIZE];

Comando *BuscaComando(char *accion)
{
	Comando *aux;
	for (aux = comandos; aux; aux = aux->sig)
	{
		if (!strcmp(aux->cmd, accion) || !strcmp(aux->tok, accion))
			return aux;
	}
	return NULL;
}
char *Token(char *msg)
{
	Comando *com;
	if ((com = BuscaComando(msg)))
		return com->tok;
	return msg;
}
char *Cmd(char *tok)
{
	Comando *com;
	if ((com = BuscaComando(tok)))
		return com->cmd;
	return tok;
}
void InsertaComando(char *cmd, char *tok, IRCFUNC(*func), int cuando, u_char params)
{
	Comando *com;
	if (!cmd)
		return;
	if ((com = BuscaComando(cmd)))
	{
		int i;
		for (i = 0; i < com->funciones; i++)
		{
			if (com->funcion[i] == func)
				return;
		}
		com->funcion[com->funciones] = func;
		com->funciones++;
		return;
	}
	com = (Comando *)Malloc(sizeof(Comando));
	com->cmd = cmd;
	com->tok = tok;
	com->funciones = 1;
	com->funcion[0] = func;
	com->params = params;
	com->cuando = cuando;
	AddItem(com, comandos);
}
int BorraComando(char *cmd, IRCFUNC(*func))
{
	Comando *com;
	if ((com = BuscaComando(cmd)))
	{
		int i;
		for (i = 0; i < com->funciones; i++)
		{
			if (com->funcion[i] == func)
			{
				while (i < com->funciones)
				{
					com->funcion[i] = com->funcion[i+1];
					i++;
				}
				if (!--com->funciones)
					LiberaItem(com, comandos);
				return 1;
			}
		}
	}
	return 0;
}
int AbreSockIrcd()
{
#ifdef USA_SSL
	if (!(SockIrcd = SockOpen(conf_server->addr, (conf_ssl ? -1 : 1) * conf_server->puerto, IniciaIrcd, ProcesaIrcd, NULL, CierraIrcd, ADD)))
#else
	if (!(SockIrcd = SockOpen(conf_server->addr, conf_server->puerto, IniciaIrcd, ProcesaIrcd, NULL, CierraIrcd, ADD)))
#endif
	{
		Info("No puede conectar");
		CierraIrcd(NULL, NULL);
		return 0;
	}
	if (IrcdEscucha)
	{
		SockClose(IrcdEscucha, LOCAL);
		IrcdEscucha = NULL;
	}
	return 1;
}
void EscuchaIrcd()
{
	SockIrcd = NULL;
	if (!IrcdEscucha)
	{
		IrcdEscucha = SockListen(conf_server->escucha, EscuchaAbre, NULL, NULL, NULL);
#ifdef USA_SSL
		if (IrcdEscucha && conf_ssl)
			IrcdEscucha->opts |= OPT_SSL;
#endif
	}
}
SOCKFUNC(IniciaIrcd)
{
	inicio = time(0);
#ifdef _WIN32
	ChkBtCon(1, 1);
#endif
	ApagaCrono("rircd", NULL);
	canales = NULL;
	protocolo->inicia();
	return 0;
}
SOCKFUNC(ProcesaIrcd)
{
#ifdef DEBUG
	if (data)
		Debug("* %s", data);
#endif
	strcpy(backupbuf, data);
	if (!canal_debug)
		canal_debug = BuscaCanal(conf_set->debug, NULL);
	if (conf_set->debug && canal_debug && canal_debug->miembros)
		port_func(P_MSG_VL)((Cliente *)canal_debug, &me, 1, data, NULL);
	return protocolo->parsea(sck, data);
}
SOCKFUNC(CierraIrcd)
{
	Cliente *al, *aux;
	Canal *cn, *caux;
	for (al = clientes; al; al = aux)
	{
		aux = al->sig;
		if (!aux) /* ultimo slot */
			break;
		LiberaMemoriaCliente(al);
	}
	for (cn = canales; cn; cn = caux)
	{
		caux = cn->sig;
		LiberaMemoriaCanal(cn);
	}
	canales = NULL;
	linkado = NULL;
	if (reset)
	{
		(void)execv(margv[0], margv);
		CierraColossus(0);
	}
	if (!data) /* es remoto */
	{
#ifdef _WIN32		
		ChkBtCon(0, 1);
#endif		
		intentos++;
		if (intentos > conf_set->reconectar.intentos)
		{
			Info("Se han realizado %i intentos y ha sido imposible conectar", intentos - 1);
#ifdef _WIN32			
			ChkBtCon(0, 0);
#endif
			intentos = 0;
			EscuchaIrcd();
			return 1;
		}
		//Alerta(FOK, "Intento %i. Reconectando en %i segundos...", intentos, conf_set->reconectar->intervalo);
		IniciaCrono("rircd", NULL, 1, conf_set->reconectar.intervalo, AbreSockIrcd, NULL, 0);
	}
#ifdef _WIN32
	else /* es local */
		ChkBtCon(0, 0);
#endif
	EscuchaIrcd();
	return 0;
}
SOCKFUNC(EscuchaAbre) /* nos aceptan el sock, lo renombramos */
{
	if (IrcdEscucha) /* primero cerramos la escucha para no aceptar más conexiones */
	{
		SockClose(IrcdEscucha, LOCAL);
		IrcdEscucha = NULL;
	}
	if (SockIrcd) /* ups, ya teníamos la conexión. cerramos la de escucha */
		SockClose(sck, LOCAL); /* de momento no tiene asignadas las funciones propias del irc, vía libre */
	else
	{
		/* asignamos funciones */
		sck->readfunc = ProcesaIrcd;
		sck->closefunc = CierraIrcd;
		SockIrcd = sck;
		if (strcmp(sck->host, conf_server->addr))
		{
			SockClose(sck, LOCAL);
			Loguea(LOG_CONN, "Se ha rechazado una conexión remota %s:%i", sck->host, sck->puerto);
			EscuchaIrcd();
			return 1;
		}
		IniciaIrcd(sck, NULL);
	}
	return 0;
}

void EnviaAServidor(char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	if (SockIrcd)
		SockWriteVL(SockIrcd, OPT_CRLF, formato, vl);
	va_end(vl);
}
Cliente *BuscaCliente(char *nick, Cliente *lugar)
{
	if (nick)
		lugar = BuscaClienteEnHash(nick, lugar, uTab);
	return lugar;
}
Canal *BuscaCanal(char *canal, Canal *lugar)
{
	if (canal)
		lugar = BuscaCanalEnHash(canal, lugar, cTab);
	return lugar;
}
Cliente *NuevoCliente(char *nombre, char *ident, char *host, char *ip, char *server, char *vhost, char *umodos, char *info)
{
	Cliente *cl;
	BMalloc(cl, Cliente);
	if (nombre)
		cl->nombre = strdup(nombre);
	if (ident)
		cl->ident = strdup(ident);
	if (host)
		cl->host = strdup(host);
	cl->rvhost = cl->host; /* suponemos que ya es host, si no lo fuera, lo obtendríamos más tarde */
	if (EsIp(host)) /* es ip, la resolvemos */
		ResuelveHost(&cl->rvhost, cl->host);
	if (ip)
		cl->ip = strdup(ip);	
	if (server)
		cl->server = BuscaCliente(server, NULL);
	if (vhost)
		cl->vhost = strdup(vhost);
	cl->nivel = NOTID;
	if (umodos)
		ProcesaModosCliente(cl, umodos);
	if (info)
		cl->info = strdup(info);
	cl->sck = SockActual;
	cl->tipo = ES_CLIENTE;
	GeneraMascara(cl);
	if (clientes)
		clientes->prev = cl;
	AddItem(cl, clientes);
	InsertaClienteEnHash(cl, nombre, uTab);
	return cl;
}
void CambiaNick(Cliente *cl, char *nuevonick)
{
	if (!cl)
		return;
	BorraClienteDeHash(cl, cl->nombre, uTab);
	ircstrdup(cl->nombre, nuevonick);
	GeneraMascara(cl);
	InsertaClienteEnHash(cl, nuevonick, uTab);
}
Canal *InfoCanal(char *canal, int crea)
{
	Canal *cn;
	if ((cn = BuscaCanal(canal, NULL)))
		return cn;
	if (crea)
	{
		BMalloc(cn, Canal);
		cn->nombre = strdup(canal);
		if (canales)
			canales->prev = cn;
		AddItem(cn, canales);
		InsertaCanalEnHash(cn, canal, cTab);
	}
	return cn;
}
void InsertaCanalEnCliente(Cliente *cl, Canal *cn)
{
	LinkCanal *lk;
	if (!cl || EsLinkCanal(cl->canal, cn))
		return;
	lk = (LinkCanal *)Malloc(sizeof(LinkCanal));
	lk->chan = cn;
	AddItem(lk, cl->canal);
	cl->canales++;
}
void InsertaClienteEnCanal(Canal *cn, Cliente *cl)
{
	LinkCliente *lk;
	if (!cl || EsLink(cn->miembro, cl))
		return;
	lk = (LinkCliente *)Malloc(sizeof(LinkCliente));
	lk->user = cl;
	AddItem(lk, cn->miembro);
	cn->miembros++;
}
int BorraCanalDeCliente(Cliente *cl, Canal *cn)
{
	LinkCanal *aux, *prev = NULL;
	if (!cl)
		return 0;
	for (aux = cl->canal; aux; aux = aux->sig)
	{
		if (aux->chan == cn)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				cl->canal = aux->sig;
			Free(aux);
			cl->canales--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
int BorraClienteDeCanal(Canal *cn, Cliente *cl)
{
	LinkCliente *aux, *prev = NULL;
	if (!cl)
		return 0;
	for (aux = cn->miembro; aux; aux = aux->sig)
	{
		if (aux->user == cl)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				cn->miembro = aux->sig;
			Free(aux);
			cn->miembros--;
			break;
		}
		prev = aux;
	}
	BorraModoCliente(&cn->owner, cl);
	BorraModoCliente(&cn->admin, cl);
	BorraModoCliente(&cn->op, cl);
	BorraModoCliente(&cn->half, cl);
	BorraModoCliente(&cn->voz, cl);
	if (!cn->miembros 
#ifdef UDB
		&& !(cn->modos & MODE_RGSTR)
#endif
	)
		LiberaMemoriaCanal(cn);
	return 0;
}
void InsertaBan(Cliente *cl, Canal *cn, char *ban)
{
	Ban *banid;
	if (!ban)
		return;
	banid = (Ban *)Malloc(sizeof(Ban));
	banid->quien = cl;
	banid->ban = strdup(ban);
	AddItem(banid, cn->ban);
}
int BorraBanDeCanal(Canal *cn, char *ban)
{
	Ban *aux, *prev = NULL;
	if (!ban)
		return 0;
	for (aux = cn->ban; aux; aux = aux->sig)
	{
		if (!strcmp(aux->ban, ban))
		{
			if (prev)
				prev->sig = aux->sig;
			else
				cn->ban = aux->sig;
			Free(aux->ban);
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void inserta_exc_en_canal(Cliente *cl, Canal *cn, char *exc)
{
	Ban *excid;
	if (!exc)
		return;
	excid = (Ban *)Malloc(sizeof(Ban));
	excid->quien = cl;
	excid->ban = strdup(exc);
	AddItem(excid, cn->exc);
}
int borra_exc_de_canal(Canal *cn, char *exc)
{
	Ban *aux, *prev = NULL;
	if (!exc)
		return 0;
	for (aux = cn->exc; aux; aux = aux->sig)
	{
		if (!strcmp(aux->ban, exc))
		{
			if (prev)
				prev->sig = aux->sig;
			else
				cn->exc = aux->sig;
			Free(aux->ban);
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void InsertaModoCliente(LinkCliente **link, Cliente *cl)
{
	LinkCliente *lk;
	if (!cl || EsLink(*link, cl))
		return;
	lk = (LinkCliente *)Malloc(sizeof(LinkCliente));
	lk->user = cl;
	AddItem(lk, *link);
}
int BorraModoCliente(LinkCliente **link, Cliente *cl)
{
	LinkCliente *aux, *prev = NULL;
	if (!cl || !EsLink(*link, cl))
		return 0;
	for (aux = *link; aux; aux = aux->sig)
	{
		if (cl == aux->user)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				*link = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void GeneraMascara(Cliente *cl)
{
	if (!cl)
		return;
	ircfree(cl->mask);
	ircfree(cl->vmask);
	cl->mask = cl->vmask = NULL;
	if (!cl->nombre || !cl->ident) /* no es usuario */
		return;
	if (cl->host)
	{
		cl->mask = (char *)Malloc(sizeof(char) * (strlen(cl->nombre) + 1 + strlen(cl->ident) + 1 + strlen(cl->host) + 1));
		ircsprintf(cl->mask, "%s!%s@%s", cl->nombre, cl->ident, cl->host);
	}
	if (cl->vhost)
	{
		cl->vmask = (char *)Malloc(sizeof(char) * (strlen(cl->nombre) + 1 + strlen(cl->ident) + 1 + strlen(cl->vhost) + 1));
		ircsprintf(cl->vmask, "%s!%s@%s", cl->nombre, cl->ident, cl->vhost);
	}
}
void DistribuyeMe(Cliente *me, Sock **sck)
{
	ircstrdup(me->nombre, conf_server->host);
	me->tipo = ES_SERVER;
	me->numeric = conf_server->numeric;
	me->protocol = PROTOCOL;
	ircfree(me->trio);
	me->trio = (char *)Malloc(sizeof(char) * 21); /* hay 20 flags posibles */
	*(me->trio) = '\0';
	strcat(me->trio, "hK");
#ifdef _WIN32
	strcat(me->trio, "W");
#endif
#ifdef USA_ZLIB
	strcat(me->trio, "Z");
#endif
#ifdef USA_SSL
	strcat(me->trio, "e");
#endif
	ircstrdup(me->info, conf_server->info);
	ircstrdup(me->host, "0.0.0.0");
	me->sck = *sck;
	me->sig = me->prev = NULL;
	if (!clientes)
	{
		clientes = me;
		InsertaClienteEnHash(me, conf_server->host, uTab);
	}
}
char *MascaraIrcd(char *mascara)
{
	buf[0] = '\0';
	if (!strchr(mascara, '@'))
	{
		if (!strchr(mascara, '.'))
			ircsprintf(buf, "%s!*@*", mascara);
		else 
			ircsprintf(buf, "*!*@%s", mascara);
	}
	else
	{
		if (strchr(mascara, '!'))
			ircsprintf(buf, "%s", mascara);
		else
			ircsprintf(buf, "*!%s", mascara);
	}
	return buf;
}
void EntraBot(Cliente *bl, char *canal)
{
	Canal *cn;
	cn = InfoCanal(canal, !0);
	//InsertaCanalEnCliente(bl, cn);
	//InsertaClienteEnCanal(cn, bl);
	port_func(P_JOIN_USUARIO_LOCAL)(bl, cn);
	if (conf_set->opts & AUTOBOP)
		port_func(P_MODO_CANAL)(&me, cn, "+o %s", TRIO(bl));
}
void SacaBot(Cliente *bl, char *canal, char *motivo)
{
	Canal *cn;
	cn = InfoCanal(canal, 0);
	//BorraCanalDeCliente(bl, cn);
	//BorraClienteDeCanal(cn, bl);
	port_func(P_PART_USUARIO_LOCAL)(bl, cn, motivo);
}
char *TipoMascara(char *mask, int tipo)
{
	char *nick, *user, *host, *host2;
	strcpy(tokbuf, mask);
	nick = strtok(tokbuf, "!");
	user = strtok(NULL, "@");
	host = strtok(NULL, "");
	if ((host2 = strchr(host, '.')))
		host2++;
	else
		host2 = host;
	switch(tipo)
	{
		case 1:
			ircsprintf(buf, "*!*%s@%s", user, host);
			break;
		case 2:
			ircsprintf(buf, "*!*@%s", host);
			break;
		case 3:
			ircsprintf(buf, "*!*%s@*.%s", user, host2);
			break;
		case 4:
			ircsprintf(buf, "*!*@*.%s", host2);
			break;
		case 5:
			ircsprintf(buf, "%s", mask);
			break;
		case 6:
			ircsprintf(buf, "%s!*%s@%s", nick, user, host);
			break;
		case 7:
			ircsprintf(buf, "%s!*@%s", nick, host);
			break;
		case 8:
			ircsprintf(buf, "%s!*%s@*.%s", nick, user, host2);
			break;
		case 9:
			ircsprintf(buf, "%s!*@*.%s", nick, host2);
			break;
		default:
			strcpy(buf, mask);
	}
	return buf;
}
int EsLink(LinkCliente *link, Cliente *al)
{
	LinkCliente *aux;
	for (aux = link; aux; aux = aux->sig)
	{
		if (aux->user == al)
			return 1;
	}
	return 0;
}
int EsLinkCanal(LinkCanal *link, Canal *cn)
{
	LinkCanal *aux;
	for (aux = link; aux; aux = aux->sig)
	{
		if (aux->chan == cn)
			return 1;
	}
	return 0;
}
#ifdef UDB
void PropagaRegistro(char *item, ...)
{
	va_list vl;
	char r, buf[1024];
	int ins;
	Udb *bloq;
	u_long pos;
	va_start(vl, item);
	ircvsprintf(buf, item, vl);
	va_end(vl);
	r = buf[0];
	bloq = IdAUdb(r);
	pos = bloq->data_long;
	ins = ParseaLinea(r, &buf[3], 1);
	if (strchr(buf, ' '))
	{
		if (ins == 1)
			EnviaAServidor(":%s DB * INS %lu %s", me.nombre, pos, buf);
	}
	else
	{
		if (ins == -1)
			EnviaAServidor(":%s DB * DEL %lu %s", me.nombre, pos, buf);
	}
}
#endif
void LiberaMemoriaCliente(Cliente *cl)
{
	LinkCanal *aux, *tmp;
	if (!cl)
		return;
	BorraClienteDeHash(cl, cl->nombre, uTab);
	if (cl->prev)
		cl->prev->sig = cl->sig;
	else
		clientes = cl->sig;
	if (cl->sig)
		cl->sig->prev = cl->prev;
	ircfree(cl->nombre);
	ircfree(cl->ident);
	if (cl->host != cl->rvhost) /* hay que liberarlo */
		ircfree(cl->rvhost);
	if (cl->host != cl->ip)
		ircfree(cl->ip);
	ircfree(cl->host);
	ircfree(cl->vhost);
	ircfree(cl->mask);
	ircfree(cl->vmask);
	ircfree(cl->trio);
	ircfree(cl->info);
	//for (i = 0; i < cl->fundadores; i++)
	//	ircfree(cl->fundador[i]);
	for (aux = cl->canal; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	ircfree(cl);
}
void LiberaMemoriaCanal(Canal *cn)
{
	Ban *ban, *tmpb;
	LinkCliente *aux, *tmp;
	BorraCanalDeHash(cn, cn->nombre, cTab);
	if (cn->prev)
		cn->prev->sig = cn->sig;
	else
		canales = cn->sig;
	if (cn->sig)
		cn->sig->prev = cn->prev;
	ircfree(cn->topic);
	ircfree(cn->clave);
	ircfree(cn->flood);
	ircfree(cn->link);
	for (ban = cn->ban; ban; ban = tmpb)
	{
		tmpb = ban->sig;
		ircfree(ban->ban);
		ircfree(ban);
	}
	for (ban = cn->exc; ban; ban = tmpb)
	{
		tmpb = ban->sig;
		ircfree(ban->ban);
		ircfree(ban);
	}
	for (aux = cn->owner; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	for (aux = cn->admin; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	for (aux = cn->op; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	for (aux = cn->half; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	for (aux = cn->voz; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	for (aux = cn->miembro; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	if (canal_debug && !strcasecmp(cn->nombre, canal_debug->nombre))
		canal_debug = NULL;
	ircfree(cn->nombre);
	Free(cn);
}
Cliente *CreaBot(char *nick, char *ident, char *host, char *server, char *modos, char *realname)
{
	Cliente *al;
	static int num = 0;
	if ((al = BuscaCliente(nick, NULL)) && !EsBot(al))
		port_func(P_QUIT_USUARIO_REMOTO)(al, &me, "Nick protegido.");
	al = NuevoCliente(nick, ident, host, NULL, server, host, modos, realname);
	al->tipo = ES_BOT;
	al->numeric = num;
	port_func(P_NUEVO_USUARIO)(al);
	num++;
	return al;
}
void DesconectaBot(char *bot, char *motivo)
{
	Cliente *bl;
	if ((bl = BuscaCliente(bot, NULL)) && EsBot(bl))
		port_func(P_QUIT_USUARIO_LOCAL)(bl, motivo);
}
void ReconectaBot(char *nick)
{
	Modulo *ex;
	Cliente *bl;
	char *canal;
	if (!(ex = BuscaModulo(nick, modulos)) || ex->cargado == 0)
		return;
	bl = CreaBot(ex->nick, ex->ident, ex->host, me.nombre, ex->modos, ex->realname);
	if (ex->residente)
	{
		strcpy(tokbuf, ex->residente);
		for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
			EntraBot(bl, canal);
	}
}
u_long FlagAModo(char flag, mTab tabla[])
{
	mTab *aux = &tabla[0];
	while (aux->flag != '0')
	{
		if (aux->flag == flag)
			return aux->mode;
		aux++;
	}
	return 0L;
}
char ModoAFlag(u_long mode, mTab tabla[])
{
	mTab *aux = &tabla[0];
	while (aux->flag != '0')
	{
		if (aux->mode == mode)
			return aux->flag;
		aux++;
	}
	return (char)NULL;
}
u_long FlagsAModos(u_long *modos, char *flags, mTab tabla[])
{
	char f = ADD;
	if (!*modos)
		*modos = 0L;
	while (*flags)
	{
		switch(*flags)
		{
			case '+':
				f = ADD;
				break;
			case '-':
				f = DEL;
				break;
			default:
				if (f == ADD)
					*modos |= FlagAModo(*flags, tabla);
				else
					*modos &= ~FlagAModo(*flags, tabla);
		}
		flags++;
	}
	return *modos;
}
char *ModosAFlags(u_long modes, mTab tabla[], Canal *cn)
{
	mTab *aux = &tabla[0];
	char *flags;
	flags = buf;
	while (aux->flag != '0')
	{
		if (modes & aux->mode)
			*flags++ = aux->flag;
		aux++;
	}
	*flags = '\0';
	if (cn)
	{
		if (cn->clave)
		{
			strcat(flags, " ");
			strcat(flags, cn->clave);
		}
		if (cn->link)
		{
			strcat(flags, " ");
			strcat(flags, cn->link);
		}
		if (cn->flood)
		{
			strcat(flags, " ");
			strcat(flags, cn->flood);
		}
	}
	return buf;
}
void ProcesaModosCliente(Cliente *cl, char *modos)
{
	if (!cl)
		return;
	FlagsAModos(&(cl->modos), modos, protocolo->umodos);
	if (cl->modos & UMODE_NETADMIN)
	{
		
		cl->nivel |= ADMIN;
		if (!strcasecmp(cl->nombre, conf_set->root))
			cl->nivel |= ROOT;
	}
	else
		cl->nivel &= ~(ADMIN | ROOT);
	if (cl->modos & UMODE_OPER)
		cl->nivel |= IRCOP;
	else
		cl->nivel &= ~IRCOP;
	if (cl->modos & UMODE_HELPOP)
		cl->nivel |= OPER;
	else
		cl->nivel &= ~OPER;
	if (cl->modos & UMODE_REGNICK)
		cl->nivel |= USER;
	else
		cl->nivel &= ~USER;		
}
void Responde(Cliente *cl, Cliente *bot, char *formato, ...)
{
	va_list vl;
	if (!cl) /* algo pasa */
		return;
	if (EsBot(cl))
		return;
	va_start(vl, formato);
	if (RESP_PRIVMSG)
		port_func(P_MSG_VL)(cl, bot, 1, formato, &vl);
	else
		port_func(P_MSG_VL)(cl, bot, 0, formato, &vl);
	va_end(vl);
}
int EntraResidentes()
{
	char *canal;
	Modulo *aux;
	for (aux = modulos; aux; aux = aux->sig)
	{
		if (aux->cargado == 2 && aux->residente)
		{
			strcpy(tokbuf, aux->residente);
			for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
				EntraBot(aux->cl, canal);
		}
	}
	return 0;
}
int EntraBots()
{
	Modulo *aux;
	for (aux = modulos; aux; aux = aux->sig)
	{
		if (aux->cargado == 2)
			aux->cl = CreaBot(aux->nick, aux->ident, aux->host, me.nombre, aux->modos, aux->realname);
	}
	return 0;
}
char *MascaraTKL(char *user, char *host)
{
	char *s = NULL;
	if (user)
	{
		s = (char *)Malloc(sizeof(char) * (strlen(user) + (host ? 1 + strlen(host) : 0) + 1));
		strcpy(s, user);
		if (host)
		{
			strcat(s, "@");
			strcat(s, host);
		}
	}
	return s;
}
Tkl *InsertaTKL(int tipo, char *user, char *host, char *autor, char *motivo, time_t inicio, time_t fin)
{
	Tkl *tkl;
	BMalloc(tkl, Tkl);
	tkl->mascara = MascaraTKL(user, host);
	tkl->autor = strdup(autor);
	tkl->motivo = strdup(motivo);
	tkl->tipo = tipo;
	tkl->inicio = inicio;
	tkl->fin = fin;
	return tkl;
}
int BorraTKL(Tkl **lugar, char *user, char *host)
{
	Tkl *aux, *prev = NULL;
	char *s;
	s = MascaraTKL(user, host);
	for (aux = *lugar; aux; aux = aux->sig)
	{
		if (!strcasecmp(s, aux->mascara))
		{
			if (prev)
				prev->sig = aux->sig;
			else
				*lugar = aux->sig;
			ircfree(aux->mascara);
			ircfree(aux->motivo);
			ircfree(aux->autor);
			ircfree(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
