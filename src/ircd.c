/*
 * $Id: ircd.c,v 1.36 2006-04-30 18:08:31 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"

/*!
 * @desc: Contiene la fecha de inicio de la conexión con el servidor IRCd.
 * @cat: IRCd
 !*/

time_t inicio;

/*!
 * @desc: Variable de uso general para cualquier trato con cadenas.
 * @cat: Programa
 !*/
 
char buf[BUFSIZE];
char backupbuf[BUFSIZE];
/*!
 * @desc: Cliente local del servidor de servicios.
 * @cat: IRCd
 !*/
 
Cliente me;
Modulo *modulos;
Cliente *linkado = NULL;
int intentos = 0;
Cliente *clientes = NULL;
Canal *canales = NULL;
char **margv;
Sock *SockIrcd = NULL, *IrcdEscucha = NULL;
char reset = 0;
SOCKFUNC(EscuchaAbre);
Canal *canal_debug = NULL;
Protocolo *protocolo = NULL;
Comando *comandos = NULL;
char modebuf[BUFSIZE], parabuf[BUFSIZE];
Tkl *tklines[TKL_MAX];

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
		com->funcion[com->funciones++] = func;
		return;
	}
	BMalloc(com, Comando);
	com->cmd = strdup(cmd);
	com->tok = strdup(tok);
	com->funcion[com->funciones++] = func;
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
				{
					BorraItem(com, comandos);
					Free(com->cmd);
					Free(com->tok);
					Free(com);
				}
				return 1;
			}
		}
	}
	return 0;
}
int AbreSockIrcd()
{
#ifdef USA_SSL
	if (!(SockIrcd = SockOpen(conf_server->addr, (conf_ssl ? -1 : 1) * conf_server->puerto, IniciaIrcd, ProcesaIrcd, NULL, CierraIrcd)))
#else
	if (!(SockIrcd = SockOpen(conf_server->addr, conf_server->puerto, IniciaIrcd, ProcesaIrcd, NULL, CierraIrcd)))
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
			SetSSL(IrcdEscucha);
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
		ProtFunc(P_MSG_VL)((Cliente *)canal_debug, &me, 1, data, NULL);
	return protocolo->parsea(sck, data);
}
SOCKFUNC(CierraIrcd)
{
	Cliente *al, *aux;
	Canal *cn, *caux;
	Modulo *mod;
	for (al = clientes; al; al = aux)
	{
		aux = al->sig;
		if (al == &me)
			continue;
		LiberaMemoriaCliente(al);
	}
	for (cn = canales; cn; cn = caux)
	{
		caux = cn->sig;
		LiberaMemoriaCanal(cn);
	}
	canales = NULL;
	linkado = NULL;
	for (mod = modulos; mod; mod = mod->sig)
		mod->cl = NULL;
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
		if (++intentos > conf_set->reconectar.intentos)
		{
			Info("Se han realizado %i intentos y ha sido imposible conectar", intentos - 1);
#ifdef _WIN32			
			ChkBtCon(0, 0);
#endif
			intentos = 0;
		}
		else
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

/*!
 * @desc: Envía una cadena al servidor, talmente como un RAW.
 * @params: $formato [in] Cadena con formato a enviar al servidor.
 	    $... [in] Argumentos variables según cadena con formato.
 * @cat: IRCd
 !*/
 
void EnviaAServidor(char *formato, ...)
{
	va_list vl;
	va_start(vl, formato);
	if (SockIrcd)
		SockWriteVL(SockIrcd, formato, vl);
	va_end(vl);
}

/*!
 * @desc: Busca un cliente por su nick.
 * @params: $nick [in] Nickname del cliente.
 	    $lugar [in] En desuso. Usar NULL.
 * @ret: Devuelve la estructura del cliente en caso de encontrarlo. Devuelve NULL si el cliente no existe.
 * @cat: IRCd
 !*/
 
Cliente *BuscaCliente(char *nick, Cliente *lugar)
{
	if (nick)
		lugar = BuscaClienteEnHash(nick, lugar, uTab);
	return lugar;
}

/*!
 * @desc: Busca un canal por su nombre.
 * @params: $canal [in] Nombre del canal.
 	    $lugar [in] En desuso. Usar NULL.
 * @ret: Devuelve la estructura del canal en caso de encontrarlo. Devuelve NULL si el canal no existe.
 * @cat: IRCd
 !*/
 
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
	if (umodos)
		ProcesaModosCliente(cl, umodos);
	if (info)
		cl->info = strdup(info);
	cl->sck = SockActual;
	cl->tipo = TCLIENTE;
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
MallaCliente *CreaMallaCliente(char flag)
{
	MallaCliente *mcl;
	BMalloc(mcl, MallaCliente);
	mcl->flag = flag;
	return mcl;
}
MallaCliente *BuscaMallaCliente(Canal *cn, char flag)
{
	MallaCliente *mcl;
	for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
	{
		if (mcl->flag == flag)
			return mcl;
	}
	return NULL;
}
MallaMascara *CreaMallaMascara(char flag)
{
	MallaMascara *mmk;
	BMalloc(mmk, MallaMascara);
	mmk->flag = flag;
	return mmk;
}
MallaMascara *BuscaMallaMascara(Canal *cn, char flag)
{
	MallaMascara *mmk;
	for (mmk = cn->mallamk; mmk; mmk = mmk->sig)
	{
		if (mmk->flag == flag)
			return mmk;
	}
	return NULL;
}
MallaParam *CreaMallaParam(char flag)
{
	MallaParam *mpm;
	BMalloc(mpm, MallaParam);
	mpm->flag = flag;
	return mpm;
}
MallaParam *BuscaMallaParam(Canal *cn, char flag)
{
	MallaParam *mpm;
	for (mpm = cn->mallapm; mpm; mpm = mpm->sig)
	{
		if (mpm->flag == flag)
			return mpm;
	}
	return NULL;
}
Canal *InfoCanal(char *canal, int crea)
{
	Canal *cn;
	if ((cn = BuscaCanal(canal, NULL)))
		return cn;
	if (crea)
	{
		char *c;
		BMalloc(cn, Canal);
		cn->nombre = strdup(canal);
		if (canales)
			canales->prev = cn;
		for (c = protocolo->modcl; !BadPtr(c); c++)
			AddItem(CreaMallaCliente(*c), cn->mallacl);
		for (c = protocolo->modmk; !BadPtr(c); c++)
			AddItem(CreaMallaMascara(*c), cn->mallamk);
		for (c = protocolo->modpm1; !BadPtr(c); c++)
			AddItem(CreaMallaParam(*c), cn->mallapm);
		for (c = protocolo->modpm2; !BadPtr(c); c++)
			AddItem(CreaMallaParam(*c), cn->mallapm);
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
	MallaCliente *maux;
	if (!cl || !cn)
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
	for (maux = cn->mallacl; maux; maux = maux->sig)
		BorraModoCliente(&maux->malla, cl);
	if (!cn->miembros)
		LiberaMemoriaCanal(cn);
	return 0;
}
void InsertaMascara(Cliente *cl, Mascara **mk, char *mascara)
{
	Mascara *maskid;
	if (!mascara)
		return;
	maskid = (Mascara *)Malloc(sizeof(Mascara));
	maskid->quien = cl;
	maskid->mascara = strdup(mascara);
	AddItem(maskid, *mk);
}
int BorraMascaraDeCanal(Mascara **mk, char *mascara)
{
	Mascara *aux, *prev = NULL;
	if (!mascara)
		return 0;
	for (aux = *mk; aux; aux = aux->sig)
	{
		if (!strcmp(aux->mascara, mascara))
		{
			if (prev)
				prev->sig = aux->sig;
			else
				*mk = aux->sig;
			Free(aux->mascara);
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
	me->tipo = TSERVIDOR;
	me->numeric = conf_server->numeric;
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

/*!
 * @desc: Genera una máscara válida para ban.
 * @params: $mascara [in] Máscara a corregir. Puede ser sólo el nickname, el username, el host, o cualquier combinación de ambas.
 * @ret: Devuelve esa máscara corregida.
 * @cat: IRCd
 !*/
 
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

/*!
 * @desc: Entra un bot a un canal.
 * @params: $bl [in] Estructura Cliente del bot a entrar.
 	    $canal [in] Nombre del canal.
 * @ver: SacaBot
 * @cat: Modulos
 !*/
 
void EntraBot(Cliente *bl, char *canal)
{
	Canal *cn;
	cn = InfoCanal(canal, !0);
	InsertaCanalEnCliente(bl, cn);
	InsertaClienteEnCanal(cn, bl);
	ProtFunc(P_JOIN_USUARIO_LOCAL)(bl, cn);
	if (conf_set->opts & AUTOBOP)
		ProtFunc(P_MODO_CANAL)(&me, cn, "+o %s", TRIO(bl));
}

/*!
 * @desc: Saca un bot de un canal.
 * @params: $bl [in] Estructura Cliente del bot a sacar.
 	    $canal [in] Nombre del canal.
 	    $motivo [in] Motivo de la salida. Si no se precisa ninguno, usar NULL.
 * @cat: Modulos
 !*/
 
void SacaBot(Cliente *bl, char *canal, char *motivo)
{
	Canal *cn;
	cn = InfoCanal(canal, 0);
	BorraCanalDeCliente(bl, cn);
	BorraClienteDeCanal(cn, bl);
	ProtFunc(P_PART_USUARIO_LOCAL)(bl, cn, motivo);
}

/*!
 * @desc: Genera una máscara según los 9 tipos de máscara irc.
 * @params: $mask [in] Máscara completa con nickname, username y host.
 	    $tipo [in] Tipo de máscara. Número del 1 al 9.
 * @cat: IRCd
 !*/
 
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
	Free(cl->nombre);
	Free(cl->ident);
	if (cl->rvhost != cl->host) /* hay que liberarlo */
		Free(cl->rvhost);
	if (cl->ip != cl->host)
		Free(cl->ip);
	Free(cl->host);
	Free(cl->vhost);
	Free(cl->mask);
	Free(cl->vmask);
	Free(cl->trio);
	Free(cl->info);
	for (aux = cl->canal; aux; aux = tmp)
	{
		tmp = aux->sig;
		Free(aux);
	}
	ircfree(cl);
}
void LiberaMemoriaCanal(Canal *cn)
{
	MallaMascara *maux, *mtmp;
	MallaCliente *caux, *ctmp;
	MallaParam *paux, *ptmp;
	LinkCliente *aux, *tmp;
	Mascara *kaux, *ktmp;
	BorraCanalDeHash(cn, cn->nombre, cTab);
	if (cn->prev)
		cn->prev->sig = cn->sig;
	else
		canales = cn->sig;
	if (cn->sig)
		cn->sig->prev = cn->prev;
	ircfree(cn->topic);
	for (maux = cn->mallamk; maux; maux = mtmp)
	{
		mtmp = maux->sig;
		for (kaux = maux->malla; kaux; kaux = ktmp)
		{
			ktmp = kaux->sig;
			Free(kaux->mascara);
			Free(kaux);
		}
		Free(maux);
	}
	for (caux = cn->mallacl; caux; caux = ctmp)
	{
		ctmp = caux->sig;
		for (aux = caux->malla; aux; aux = tmp)
		{
			tmp = aux->sig;
			Free(aux);
		}
		Free(caux);
	}
	for (paux = cn->mallapm; paux; paux = ptmp)
	{
		ptmp = paux->sig;
		ircfree(paux->param);
		Free(paux);
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

/*!
 * @desc: Crea una estructura Cliente para un bot.
 Esta función se llama durante la creación de los módulos y NO debe ser llamada por los propios módulos.
 Sólo debe utilizarse cuando se quieran introducir otros bots adicionales.
 * @params: $nick [in] Nickname del bot.
 	    $ident [in] Username del bot.
 	    $host [in] Host del bot.
 	    $modos [in] Modos de usuario del bot.
 	    $realname [in] Realname del bot.
 * @ret: Devuelve la estructura Cliente creada a partir de estos datos. 
 * @ver: DesconectaBot
 * @cat: Modulos
 !*/
 
Cliente *CreaBot(char *nick, char *ident, char *host, char *modos, char *realname)
{
	Cliente *al;
	static int num = 0;
	if (!EsOk(SockIrcd))
		return NULL;
	if ((al = BuscaCliente(nick, NULL)) && !EsBot(al))
		ProtFunc(P_QUIT_USUARIO_REMOTO)(al, &me, "Nick protegido.");
#ifdef DEBUG
	Debug("Creando %s", nick);
#endif
	al = NuevoCliente(nick, ident, host, NULL, me.nombre, host, modos, realname);
	al->tipo = TBOT;
	al->numeric = num;
	ProtFunc(P_NUEVO_USUARIO)(al);
	num++;
	return al;
}

/*!
 * @desc: Desconecta un bot llamado previamente por CreaBot.
 * @params: $nick [in] Nickname del bot.
 	    $motivo [in] Motivo de la desconexión. Si no se precisa, usar NULL.
 * @ver: CreaBot
 * @cat: Modulos
 !*/
 
void DesconectaBot(char *nick, char *motivo)
{
	Cliente *bl;
	Modulo *ex;
	if ((bl = BuscaCliente(nick, NULL)) && EsBot(bl))
		ProtFunc(P_QUIT_USUARIO_LOCAL)(bl, motivo);
	if ((ex = BuscaModulo(nick, modulos)))
		ex->cl = NULL;
}
void ReconectaBot(char *nick)
{
	Modulo *ex;
	char *canal;
	if (!EsOk(SockIrcd) || BuscaCliente(nick, NULL) || !(ex = BuscaModulo(nick, modulos)) || ex->activo)
		return;
	ex->cl = CreaBot(ex->nick, ex->ident, ex->host, ex->modos, ex->realname);
	if (ex->residente)
	{
		strcpy(tokbuf, ex->residente);
		for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
			EntraBot(ex->cl, canal);
	}
}
u_long FlagAModo(char flag, mTab tabla[])
{
	mTab *aux = &tabla[0];
	while (aux->flag)
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
	while (aux->flag)
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
	while (aux->flag)
	{
		if (modes & aux->mode)
			*flags++ = aux->flag;
		aux++;
	}
	*flags = '\0';
	if (cn)
	{
		MallaParam *mpm;
		for (mpm = cn->mallapm; mpm; mpm = mpm->sig)
		{
			strcat(flags, " ");
			strcat(flags, mpm->param);
		}
	}
	return buf;
}
void ProcesaModosCliente(Cliente *cl, char *modos)
{
	int i;
	if (!cl)
		return;
	FlagsAModos(&(cl->modos), modos, protocolo->umodos);
	for (i = 0; i < nivs; i++)
	{
		if (niveles[i]->flags)
		{
			char *c;
			for (c = niveles[i]->flags; !BadPtr(c); c++)
			{
				if (!(cl->modos & FlagAModo(*c, protocolo->umodos)))
					break;
			}
			if (BadPtr(c))
				cl->nivel |= niveles[i]->nivel;
			else
				cl->nivel &= ~(niveles[i]->nivel);
		}
	}
}

/*!
 * @desc: Envía un mensaje privado a un cliente desde un bot.
 * @params: $cl [in] Cliente receptor del mensaje.
 	    $bot [in] Cliente bot origen del mensaje.
 	    $formato [in] Cadena con formato que se desea enviar.
 	    $... [in] Argumentos variables según cadena con formato.
 * @cat: IRCd
 !*/
 
void Responde(Cliente *cl, Cliente *bot, char *formato, ...)
{
	va_list vl;
	if (!cl) /* algo pasa */
		return;
	//if (EsBot(cl))
	//	return;
	va_start(vl, formato);
	if (conf_set->opts & RESP_PRIVMSG)
		ProtFunc(P_MSG_VL)(cl, bot, 1, formato, &vl);
	else
		ProtFunc(P_MSG_VL)(cl, bot, 0, formato, &vl);
	va_end(vl);
}
int EntraResidentes()
{
	char *canal;
	Modulo *aux;
	for (aux = modulos; aux; aux = aux->sig)
	{
		if (aux->residente && !aux->activo)
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
		if (!aux->activo && !BuscaCliente(aux->nick, NULL))
			aux->cl = CreaBot(aux->nick, aux->ident, aux->host, aux->modos, aux->realname);
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
Tkl *BuscaTKL(int tipo, char *mask, Tkl *lugar)
{
	Tkl *aux;
	for (aux = lugar; aux; aux = aux->sig)
	{
		if (!strcasecmp(aux->mascara, mask))
			return aux;
	}
	return NULL;
}
