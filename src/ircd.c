/*
 * $Id: ircd.c,v 1.22 2005-03-14 14:18:09 Trocotronic Exp $ 
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
Sock *SockIrcd = NULL, *EscuchaIrcd = NULL;
char reset = 0;
SOCKFUNC(escucha_abre);
Canal *canal_debug = NULL;
Protocolo *protocolo = NULL;
Comando *comandos = NULL;
char modebuf[BUFSIZE], parabuf[BUFSIZE];

Comando *busca_comando(char *accion)
{
	Comando *aux;
	for (aux = comandos; aux; aux = aux->sig)
	{
		if (!strcmp(aux->cmd, accion) || !strcmp(aux->tok, accion))
			return aux;
	}
	return NULL;
}
char *token(char *msg)
{
	Comando *com;
	if ((com = busca_comando(msg)))
		return com->tok;
	return msg;
}
char *cmd(char *tok)
{
	Comando *com;
	if ((com = busca_comando(tok)))
		return com->cmd;
	return tok;
}
void inserta_comando(char *cmd, char *tok, IRCFUNC(*func), int cuando, u_char params)
{
	Comando *com;
	if (!cmd)
		return;
	if ((com = busca_comando(cmd)))
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
	com->sig = comandos;
	comandos = com;
}
int borra_comando(char *cmd, IRCFUNC(*func))
{
	Comando *com;
	if ((com = busca_comando(cmd)))
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
				{
					Comando *prev = NULL, *sig;
					for (sig = comandos; sig; sig = sig->sig)
					{
						if (sig == com) /* lo tenemos */
						{
							if (prev)
								prev->sig = sig->sig;
							else
								comandos = sig->sig;
							Free(sig);
							break;
						}
						prev = sig;
					}
				}
				return 1;
			}
		}
	}
	return 0;
}
int abre_sock_ircd()
{
#ifdef USA_SSL
	if (!(SockIrcd = sockopen(conf_server->addr, (conf_ssl ? -1 : 1) * conf_server->puerto, inicia_ircd, procesa_ircd, NULL, cierra_ircd, ADD)))
#else
	if (!(SockIrcd = sockopen(conf_server->addr, conf_server->puerto, inicia_ircd, procesa_ircd, NULL, cierra_ircd, ADD)))
#endif
	{
		Info("No puede conectar");
		cierra_ircd(NULL, NULL);
		return 0;
	}
	if (EscuchaIrcd)
	{
		sockclose(EscuchaIrcd, LOCAL);
		EscuchaIrcd = NULL;
	}
	return 1;
}
void escucha_ircd()
{
	SockIrcd = NULL;
	if (!EscuchaIrcd)
	{
		EscuchaIrcd = socklisten(conf_server->escucha, escucha_abre, NULL, NULL, NULL);
#ifdef USA_SSL
		if (EscuchaIrcd && conf_ssl)
			EscuchaIrcd->opts |= OPT_SSL;
#endif
	}
}
SOCKFUNC(inicia_ircd)
{
	inicio = time(0);
	ChkBtCon(1, 1);
	timer_off("rircd", NULL);
	canales = NULL;
	protocolo->inicia();
	return 0;
}
SOCKFUNC(procesa_ircd)
{
#ifdef DEBUG
	if (data)
		Debug("* %s", data);
#endif
	strcpy(backupbuf, data);
	if (!canal_debug)
		canal_debug = busca_canal(conf_set->debug, NULL);
	if (conf_set->debug && canal_debug && canal_debug->miembros)
		port_func(P_MSG_VL)((Cliente *)canal_debug, &me, 1, data, NULL);
	protocolo->parsea(sck, data);
	/*parc = tokeniza_irc(data, parv, &com, &sender);
	//for (i = 0; i < parc; i++)
	//	printf("%s (%i)\n",parv[i], strlen(parv[i]));
	if ((comd = busca_comando(com)))
	{
		Cliente *cl = NULL;
		if (comd->cuando == INI)
		{
			for (i = 0; i < comd->funciones; i++)
			{
				if (!cl)
					cl = busca_cliente(parv[0], NULL);
				if (comd->funcion[i])
					comd->funcion[i](sck, cl, parv, parc);
			}
		}
		else if (comd->cuando == FIN)
		{
			for (i = comd->funciones - 1; i >= 0; i--)
			{
				if (!cl)
					cl = busca_cliente(parv[0], NULL);
				if (comd->funcion[i])
					comd->funcion[i](sck, cl, parv, parc);
			}
		}
	}*/
	return 0;
}
SOCKFUNC(cierra_ircd)
{
	Cliente *al, *aux;
	Canal *cn, *caux;
	for (al = clientes; al; al = aux)
	{
		aux = al->sig;
		if (!aux) /* ultimo slot */
			break;
		libera_cliente_de_memoria(al);
	}
	for (cn = canales; cn; cn = caux)
	{
		caux = cn->sig;
		libera_canal_de_memoria(cn);
	}
	canales = NULL;
	linkado = NULL;
	if (reset)
	{
		(void)execv(margv[0], margv);
		cierra_colossus(0);
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
			escucha_ircd();
			return 1;
		}
		//fecho(FOK, "Intento %i. Reconectando en %i segundos...", intentos, conf_set->reconectar->intervalo);
		timer("rircd", NULL, 1, conf_set->reconectar.intervalo, abre_sock_ircd, NULL, 0);
	}
#ifdef _WIN32
	else /* es local */
		ChkBtCon(0, 0);
#endif
	escucha_ircd();
	return 0;
}
SOCKFUNC(escucha_abre) /* nos aceptan el sock, lo renombramos */
{
	if (EscuchaIrcd) /* primero cerramos la escucha para no aceptar más conexiones */
	{
		sockclose(EscuchaIrcd, LOCAL);
		EscuchaIrcd = NULL;
	}
	if (SockIrcd) /* ups, ya teníamos la conexión. cerramos la de escucha */
		sockclose(sck, LOCAL); /* de momento no tiene asignadas las funciones propias del irc, vía libre */
	else
	{
		/* asignamos funciones */
		sck->readfunc = procesa_ircd;
		sck->closefunc = cierra_ircd;
		SockIrcd = sck;
		if (strcmp(sck->host, conf_server->addr))
		{
			sockclose(sck, LOCAL);
			ircd_log(LOG_CONN, "Se ha rechazado una conexión remota %s:%i", sck->host, sck->puerto);
			escucha_ircd();
			return 1;
		}
		inicia_ircd(sck, NULL);
	}
	return 0;
}

void sendto_serv(char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	if (SockIrcd)
		sockwritev(SockIrcd, OPT_CRLF, formato, vl);
	va_end(vl);
}
void sendto_serv_us(Cliente *cl, char *cmd, char *tok, char *formato, ...)
{
	if (cl)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, formato);
		vsprintf_irc(buf, formato, vl);
		va_end(vl);
		sockwrite(SockActual, OPT_CRLF, ":%s %s %s", cl->nombre, tok, buf);
	}
}

Cliente *busca_cliente(char *nick, Cliente *lugar)
{
	if (nick)
		lugar = busca_cliente_en_hash(nick, lugar, uTab);
	return lugar;
}
Canal *busca_canal(char *canal, Canal *lugar)
{
	if (canal)
		lugar = busca_canal_en_hash(canal, lugar, cTab);
	return lugar;
}
Cliente *nuevo_cliente(char *nombre, char *ident, char *host, char *ip, char *server, char *vhost, char *umodos, char *info)
{
	Cliente *cl;
	da_Malloc(cl, Cliente);
	if (nombre)
		cl->nombre = strdup(nombre);
	if (ident)
		cl->ident = strdup(ident);
	if (host)
		cl->host = strdup(host);
	cl->rvhost = cl->host; /* suponemos que ya es host, si no lo fuera, lo obtendríamos más tarde */
	if (EsIp(host)) /* es ip, la resolvemos */
		resuelve_host(&cl->rvhost, cl->host);
	if (ip)
		cl->ip = strdup(ip);	
	if (server)
		cl->server = busca_cliente(server, NULL);
	if (vhost)
		cl->vhost = strdup(vhost);
	cl->nivel = NOTID;
	if (umodos)
		procesa_umodos(cl, umodos);
	if (info)
		cl->info = strdup(info);
	cl->sck = SockActual;
	cl->tipo = ES_CLIENTE;
	genera_mask(cl);
	cl->sig = clientes;
	if (clientes)
		clientes->prev = cl;
	clientes = cl;
	inserta_cliente_en_hash(cl, nombre, uTab);
	return cl;
}
void cambia_nick(Cliente *cl, char *nuevonick)
{
	if (!cl)
		return;
	borra_cliente_de_hash(cl, cl->nombre, uTab);
	ircstrdup(&cl->nombre, nuevonick);
	genera_mask(cl);
	inserta_cliente_en_hash(cl, nuevonick, uTab);
}
Canal *info_canal(char *canal, int crea)
{
	Canal *cn;
	if ((cn = busca_canal(canal, NULL)))
		return cn;
	if (crea)
	{
		da_Malloc(cn, Canal);
		cn->nombre = strdup(canal);
		cn->sig = canales;
		if (canales)
			canales->prev = cn;
		canales = cn;
		inserta_canal_en_hash(cn, canal, cTab);
	}
	return cn;
}
void inserta_canal_en_usuario(Cliente *cl, Canal *cn)
{
	LinkCanal *lk;
	if (!cl || es_linkcanal(cl->canal, cn))
		return;
	lk = (LinkCanal *)Malloc(sizeof(LinkCanal));
	lk->chan = cn;
	lk->sig = cl->canal;
	cl->canal = lk;
	cl->canales++;
}
void inserta_usuario_en_canal(Canal *cn, Cliente *cl)
{
	LinkCliente *lk;
	if (!cl || es_link(cn->miembro, cl))
		return;
	lk = (LinkCliente *)Malloc(sizeof(LinkCliente));
	lk->user = cl;
	lk->sig = cn->miembro;
	cn->miembro = lk;
	cn->miembros++;
}
int borra_canal_de_usuario(Cliente *cl, Canal *cn)
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
int borra_cliente_de_canal(Canal *cn, Cliente *cl)
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
	borra_modo_cliente_de_canal(&cn->owner, cl);
	borra_modo_cliente_de_canal(&cn->admin, cl);
	borra_modo_cliente_de_canal(&cn->op, cl);
	borra_modo_cliente_de_canal(&cn->half, cl);
	borra_modo_cliente_de_canal(&cn->voz, cl);
	if (!cn->miembros 
#ifdef UDB
		&& !(cn->modos & MODE_RGSTR)
#endif
	)
		libera_canal_de_memoria(cn);
	return 0;
}
void inserta_ban_en_canal(Cliente *cl, Canal *cn, char *ban)
{
	Ban *banid;
	if (!ban)
		return;
	banid = (Ban *)Malloc(sizeof(Ban));
	banid->quien = cl;
	banid->ban = strdup(ban);
	banid->sig = cn->ban;
	cn->ban = banid;
}
int borra_ban_de_canal(Canal *cn, char *ban)
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
	excid->sig = cn->exc;
	cn->exc = excid;
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
void inserta_modo_cliente_en_canal(LinkCliente **link, Cliente *cl)
{
	LinkCliente *lk;
	if (!cl || es_link(*link, cl))
		return;
	lk = (LinkCliente *)Malloc(sizeof(LinkCliente));
	lk->user = cl;
	lk->sig = *link;
	*link = lk;
}
int borra_modo_cliente_de_canal(LinkCliente **link, Cliente *cl)
{
	LinkCliente *aux, *prev = NULL;
	if (!cl || !es_link(*link, cl))
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
void genera_mask(Cliente *cl)
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
		sprintf_irc(cl->mask, "%s!%s@%s", cl->nombre, cl->ident, cl->host);
	}
	if (cl->vhost)
	{
		cl->vmask = (char *)Malloc(sizeof(char) * (strlen(cl->nombre) + 1 + strlen(cl->ident) + 1 + strlen(cl->vhost) + 1));
		sprintf_irc(cl->vmask, "%s!%s@%s", cl->nombre, cl->ident, cl->vhost);
	}
}
void distribuye_me(Cliente *me, Sock **sck)
{
	ircstrdup(&me->nombre, conf_server->host);
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
	ircstrdup(&me->info, conf_server->info);
	ircstrdup(&me->host, "0.0.0.0");
	me->sck = *sck;
	me->sig = me->prev = NULL;
	if (!clientes)
	{
		clientes = me;
		inserta_cliente_en_hash(me, conf_server->host, uTab);
	}
}
char *ircdmask(char *mascara)
{
	buf[0] = '\0';
	if (!strchr(mascara, '@'))
	{
		if (!strchr(mascara, '.'))
			sprintf_irc(buf, "%s!*@*", mascara);
		else 
			sprintf_irc(buf, "*!*@%s", mascara);
	}
	else
	{
		if (strchr(mascara, '!'))
			sprintf_irc(buf, "%s", mascara);
		else
			sprintf_irc(buf, "*!%s", mascara);
	}
	return buf;
}
void mete_bot_en_canal(Cliente *bl, char *canal)
{
	Canal *cn;
	cn = info_canal(canal, !0);
	//inserta_canal_en_usuario(bl, cn);
	//inserta_usuario_en_canal(cn, bl);
	port_func(P_JOIN_USUARIO_LOCAL)(bl, cn);
	if (conf_set->opts & AUTOBOP)
		port_func(P_MODO_CANAL)(&me, cn, "+o %s", TRIO(bl));
}
void saca_bot_de_canal(Cliente *bl, char *canal, char *motivo)
{
	Canal *cn;
	cn = info_canal(canal, 0);
	//borra_canal_de_usuario(bl, cn);
	//borra_cliente_de_canal(cn, bl);
	port_func(P_PART_USUARIO_LOCAL)(bl, cn, motivo);
}
char *mascara(char *mask, int tipo)
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
			sprintf_irc(buf, "*!*%s@%s", user, host);
			break;
		case 2:
			sprintf_irc(buf, "*!*@%s", host);
			break;
		case 3:
			sprintf_irc(buf, "*!*%s@*.%s", user, host2);
			break;
		case 4:
			sprintf_irc(buf, "*!*@*.%s", host2);
			break;
		case 5:
			sprintf_irc(buf, "%s", mask);
			break;
		case 6:
			sprintf_irc(buf, "%s!*%s@%s", nick, user, host);
			break;
		case 7:
			sprintf_irc(buf, "%s!*@%s", nick, host);
			break;
		case 8:
			sprintf_irc(buf, "%s!*%s@*.%s", nick, user, host2);
			break;
		case 9:
			sprintf_irc(buf, "%s!*@*.%s", nick, host2);
			break;
		default:
			strcpy(buf, mask);
	}
	return buf;
}
int es_link(LinkCliente *link, Cliente *al)
{
	LinkCliente *aux;
	for (aux = link; aux; aux = aux->sig)
	{
		if (aux->user == al)
			return 1;
	}
	return 0;
}
int es_linkcanal(LinkCanal *link, Canal *cn)
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
void envia_registro_bdd(char *item, ...)
{
	va_list vl;
	char r, buf[1024];
	int ins;
	Udb *bloq;
	u_long pos;
	va_start(vl, item);
	vsprintf_irc(buf, item, vl);
	va_end(vl);
	r = buf[0];
	bloq = coge_de_id(r);
	pos = bloq->data_long;
	ins = parsea_linea(r, &buf[3], 1);
	if (strchr(buf, ' '))
	{
		if (ins == 1)
			sendto_serv(":%s DB * INS %lu %s", me.nombre, pos, buf);
	}
	else
	{
		if (ins == -1)
			sendto_serv(":%s DB * DEL %lu %s", me.nombre, pos, buf);
	}
}
#endif
void libera_cliente_de_memoria(Cliente *cl)
{
	LinkCanal *aux, *tmp;
	if (!cl)
		return;
	borra_cliente_de_hash(cl, cl->nombre, uTab);
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
	ircfree(cl->host);
	ircfree(cl->ip);
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
void libera_canal_de_memoria(Canal *cn)
{
	Ban *ban, *tmpb;
	LinkCliente *aux, *tmp;
	char *nombre = strdup(cn->nombre);
	borra_canal_de_hash(cn, cn->nombre, cTab);
	if (cn->prev)
		cn->prev->sig = cn->sig;
	else
		canales = cn->sig;
	if (cn->sig)
		cn->sig->prev = cn->prev;
	ircfree(cn->nombre);
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
	Free(cn);
	Free(nombre);
}
Cliente *botnick(char *nick, char *ident, char *host, char *server, char *modos, char *realname)
{
	Cliente *al;
	static int num = 0;
	if ((al = busca_cliente(nick, NULL)) && !EsBot(al))
		port_func(P_QUIT_USUARIO_REMOTO)(al, &me, "Nick protegido.");
	al = nuevo_cliente(nick, ident, host, NULL, server, host, modos, realname);
	al->tipo = ES_BOT;
	al->numeric = num;
	port_func(P_NUEVO_USUARIO)(al);
	num++;
	return al;
}
void killbot(char *bot, char *motivo)
{
	Cliente *bl;
	if ((bl = busca_cliente(bot, NULL)) && EsBot(bl))
		port_func(P_QUIT_USUARIO_LOCAL)(bl, motivo);
}
void renick_bot(char *nick)
{
	Modulo *ex;
	Cliente *bl;
	char *canal;
	if (!(ex = busca_modulo(nick, modulos)) || ex->cargado == 0)
		return;
	bl = botnick(ex->nick, ex->ident, ex->host, me.nombre, ex->modos, ex->realname);
	if (ex->residente)
	{
		strcpy(tokbuf, ex->residente);
		for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
			mete_bot_en_canal(bl, canal);
	}
}
u_long flag2mode(char flag, mTab tabla[])
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
char mode2flag(u_long mode, mTab tabla[])
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
u_long flags2modes(u_long *modos, char *flags, mTab tabla[])
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
					*modos |= flag2mode(*flags, tabla);
				else
					*modos &= ~flag2mode(*flags, tabla);
		}
		flags++;
	}
	return *modos;
}
char *modes2flags(u_long modes, mTab tabla[], Canal *cn)
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
void procesa_umodos(Cliente *cl, char *modos)
{
	if (!cl)
		return;
	flags2modes(&(cl->modos), modos, protocolo->umodos);
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
void response(Cliente *cl, Cliente *bot, char *formato, ...)
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
int mete_residentes()
{
	char *canal;
	Modulo *aux;
	for (aux = modulos; aux; aux = aux->sig)
	{
		if (aux->cargado == 2 && aux->residente)
		{
			strcpy(tokbuf, aux->residente);
			for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
				mete_bot_en_canal(aux->cl, canal);
		}
	}
	return 0;
}
int mete_bots()
{
	Modulo *aux;
	for (aux = modulos; aux; aux = aux->sig)
	{
		if (aux->cargado == 2)
			aux->cl = botnick(aux->nick, aux->ident, aux->host, me.nombre, aux->modos, aux->realname);
	}
	return 0;
}
