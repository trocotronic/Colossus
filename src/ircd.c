#include "struct.h"
#include "ircd.h"
#include "comandos.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif

Comando *comandos = NULL;
char buf[BUFSIZE];
char backupbuf[BUFSIZE];
Cliente me;
Modulo *modulos;
double tburst;
Cliente *link = NULL;
int intentos = 0;
Cliente *clientes = NULL;
Canal *canales;
char **margv;
void renick_bot(char *);
void conecta_bots(void);
Sock *SockIrcd = NULL, *EscuchaIrcd = NULL;
char reset = 0;
time_t inicio;
SOCKFUNC(escucha_abre);
/* Comandos soportados por colossus */

IRCFUNC(m_msg);
IRCFUNC(m_whois);
IRCFUNC(m_nick);
IRCFUNC(m_topic);
IRCFUNC(m_quit);
IRCFUNC(m_kill);
IRCFUNC(m_ping);
IRCFUNC(m_pass);
IRCFUNC(m_join);
IRCFUNC(m_part);
IRCFUNC(m_mode);
IRCFUNC(m_kick);
IRCFUNC(m_chghost);
IRCFUNC(m_chgident);
IRCFUNC(m_chgname);
IRCFUNC(m_tkl);
#ifdef UDB
IRCFUNC(m_db);
IRCFUNC(m_dbq);
#endif
IRCFUNC(m_sethost);
IRCFUNC(m_version);
IRCFUNC(m_stats);
IRCFUNC(m_eos);
IRCFUNC(m_server);
IRCFUNC(m_squit);
IRCFUNC(m_netinfo);
IRCFUNC(m_umode);
IRCFUNC(m_error);
IRCFUNC(m_away);
IRCFUNC(m_105);
IRCFUNC(m_sajoin);
IRCFUNC(m_sapart);
IRCFUNC(m_rehash);
IRCFUNC(m_module);

int tokeniza_irc(char *cadena, char *parv[], char **com, char *sender)
{
	char *ini;
	int i = 0;
	*sender = 0;
	if (*cadena == 0x3A)
	{
		*sender = 1;
		cadena++;
	}
	ini = cadena;
	do
	{
		if (*cadena == 0x20)
		{
			*cadena = 0x0;
			parv[i] = ini;
			ini = cadena + 1;
			if (!(*com) && ((i == 0 && !(*sender)) || (i == 1 && *sender)))
				*com = parv[i];
			else
				i++;
			if (*(cadena + 1) == 0x3A)
			{
				ini++;
				break;
			}
		}
		cadena++;
	} while(*cadena != 0x0);
	parv[i] = ini;
	if (!(*com) && ((i == 0 && !(*sender)) || (i == 1 && *sender)))
		*com = parv[i];
	else
		i++;
	parv[i] = NULL;
	return i;
}
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
void inserta_comando(char *cmd, char *tok, IRCFUNC(*func), int cuando)
{
	Comando *com;
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
				com->funciones--;
				return 1;
			}
		}
	}
	return 0;
}
int abre_sock_ircd()
{
	if (!(SockIrcd = sockopen(conf_server->addr, conf_server->puerto, inicia_ircd, procesa_ircd, NULL, cierra_ircd, ADD)))
	{
		fecho(FERR, "No puede conectar");
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
void carga_comandos()
{
	inserta_comando(MSG_PRIVATE, TOK_PRIVATE, m_msg, INI);
	inserta_comando(MSG_WHOIS, TOK_WHOIS, m_whois, INI);
	inserta_comando(MSG_NICK, TOK_NICK, m_nick, INI);
	inserta_comando(MSG_TOPIC, TOK_TOPIC, m_topic, INI);
	inserta_comando(MSG_QUIT, TOK_QUIT, m_quit, FIN);
	inserta_comando(MSG_KILL, TOK_KILL, m_kill, INI);
	inserta_comando(MSG_PING, TOK_PING, m_ping, INI);
	inserta_comando(MSG_PASS, TOK_PASS, m_pass, INI);
	inserta_comando(MSG_NOTICE, TOK_NOTICE, m_msg, INI);
	inserta_comando(MSG_JOIN, TOK_JOIN, m_join, INI);
	inserta_comando(MSG_PART, TOK_PART, m_part, FIN);
	inserta_comando(MSG_MODE, TOK_MODE, m_mode, INI);
	inserta_comando(MSG_KICK, TOK_KICK, m_kick, INI);
	inserta_comando(MSG_CHGHOST, TOK_CHGHOST, m_chghost, INI);
	inserta_comando(MSG_CHGIDENT, TOK_CHGIDENT, m_chgident, INI);
	inserta_comando(MSG_CHGNAME, TOK_CHGNAME, m_chgname, INI);
	inserta_comando(MSG_TKL, TOK_TKL, m_tkl, INI);
#ifdef UDB
	inserta_comando(MSG_DB, TOK_DB, m_db, INI);
	inserta_comando(MSG_DBQ, TOK_DBQ, m_dbq, INI);
#endif
	inserta_comando(MSG_SETHOST, TOK_SETHOST, m_sethost, INI);
	inserta_comando(MSG_VERSION, TOK_VERSION, m_version, INI);
	inserta_comando(MSG_STATS, TOK_STATS, m_stats, INI);
	inserta_comando(MSG_EOS, TOK_EOS, m_eos, INI);
	inserta_comando(MSG_SERVER, TOK_SERVER, m_server, INI);
	inserta_comando(MSG_SQUIT, TOK_SQUIT, m_squit, FIN);
	inserta_comando(MSG_NETINFO, TOK_NETINFO, m_netinfo, INI);
	inserta_comando(MSG_UMODE2, TOK_UMODE2, m_umode, INI);
	inserta_comando(MSG_ERROR, TOK_ERROR, m_error, INI);
	inserta_comando(MSG_AWAY, TOK_AWAY, m_away, INI);
	inserta_comando("105", "105", m_105, INI);
	inserta_comando(MSG_SAJOIN, TOK_SAJOIN, m_sajoin, INI);
	inserta_comando(MSG_SAPART, TOK_SAPART, m_sapart, INI);
	inserta_comando(MSG_REHASH, TOK_REHASH, m_rehash, INI);
	inserta_comando(MSG_MODULE, TOK_MODULE, m_module, INI);
}
SOCKFUNC(inicia_ircd)
{
	inicio = time(0);
	timer_off("rircd", NULL);
	canales = NULL;
#ifdef ZIP_LINKS
	if (conf_server->compresion)
		sendto_serv("PROTOCTL UDB3 NICKv2 VHP VL TOKEN UMODE2 ZIP");
	else
#endif
	sendto_serv("PROTOCTL UDB3 NICKv2 VHP VL TOKEN UMODE2");
	sendto_serv("PASS :%s", conf_server->password->local);
	sendto_serv("SERVER %s 1 :U%i-%s-%i %s", me.nombre, me.protocol, me.opts, me.numeric, me.info);
	return 0;
}
SOCKFUNC(procesa_ircd)
{
	Comando *comd;
	char sender;
	char *parv[256];
	char *com = NULL;
	int parc, i;
#ifdef DEBUG
	if (data)
		Debug("* %s", data);
#endif
	strcpy(backupbuf, data);
	if (conf_set->debug && busca_canal(conf_set->debug, NULL))
		sendto_serv(":%s %s %s :%s", me.nombre, TOK_PRIVATE, conf_set->debug, data);
	parc = tokeniza_irc(data, parv, &com, &sender);
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
	}
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
	if (reset)
	{
		execv(margv[0], margv);
		cierra_colossus(0);
	}
	if (!data) /* es remoto */
	{
#ifdef _WIN32		
		ChkBtCon(0, 1);
#endif		
		intentos++;
		if (intentos > conf_set->reconectar->intentos)
		{
			fecho(FERR, "Se han realizado %i intentos y ha sido imposible conectar", intentos - 1);
#ifdef _WIN32			
			ChkBtCon(0, 0);
#endif
			return 1;
		}
		fecho(FOK, "Intento %i. Reconectando en %i segundos...", intentos, conf_set->reconectar->intervalo);
		timer("rircd", NULL, 1, conf_set->reconectar->intervalo, abre_sock_ircd, NULL, 0);
	}
	else /* es local */
		ChkBtCon(0, 0);
	SockIrcd = NULL;
	if (!EscuchaIrcd)
		EscuchaIrcd = socklisten(conf_server->escucha, escucha_abre, NULL, NULL, NULL);
	return 0;
}
SOCKFUNC(escucha_abre) /* nos aceptan el sock, lo renombramos */
{
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
			return 1;
		}
		inicia_ircd(sck, NULL);
	}
	if (EscuchaIrcd)
	{
		sockclose(EscuchaIrcd, LOCAL);
		EscuchaIrcd = NULL;
	}
	return 0;
}
IRCFUNC(m_msg)
{
	char *param[256], *mcom = NULL, par[BUFSIZE], p;
	bCom *bcom;
	int params, i;
	Modulo *mod;
	strtok(parv[1], "@");
	parv[parc] = strtok(NULL, "@");
	parv[parc+1] = NULL;
	while (*parv[2] == ':')
		parv[2]++;
	strcpy(par, parv[2]);
	params = tokeniza_irc(par, param, &mcom, &p);
	for (i = params++; i; i--)
		param[i] = param[i-1];
	param[0] = mcom;
	if (!strcasecmp(mcom, "\1PING"))
	{
		sendto_serv(":%s %s %s :%s", parv[1], TOK_NOTICE, parv[0], parv[2]);
		return 0;
	}
	else if (!strcasecmp(mcom, "\1VERSION\1"))
	{
		sendto_serv(":%s %s %s :\1VERSION Colossus v%s .%s.\1", parv[1], TOK_NOTICE, parv[0], COLOSSUS_VERSION, conf_set->red);
		return 0;
	}
	else if (!strcasecmp(mcom, "CREDITOS"))
	{
		response(cl, parv[1], "\00312Colossus v%s - Trocotronic ©2004", COLOSSUS_VERSION);
		response(cl, parv[1], " ");
		response(cl, parv[1], "Este programa ha salido tras horas y horas de dedicación y entusiasmo.");
		response(cl, parv[1], "Si no es perfecto, ¿qué más da? Mi intención es que disfrutes tanto como yo lo he hecho programándolos.");
		response(cl, parv[1], "Sé feliz. Paz.");
		response(cl, parv[1], " ");
		response(cl, parv[1], "Puedes descargar este programa de forma gratuita en %c\00312http://www.rallados.net", 31);
		//response(cl, parv[1], "Quiero dedicar este programa a MaD por el soporte prestado y a todos los rallados por usarlo :) (estáis locos)");
		response(cl, parv[1], "Gracias a locaxecdl y a MaD por los mareos que les he causado y por el soporte prestado.");
		response(cl, parv[1], "Gracias a todos por usar este programa :) (estáis locos)");
		return 0;
	}
	if ((mod = busca_modulo(parv[1], modulos)))
	{
		for (i = 0; mod->comandos[i]; i++)
		{
			bcom = mod->comandos[i];
			if (!strcasecmp(mcom, bcom->com))
			{
				if ((bcom->nivel & USER) && !(cl->nivel & USER))
				{
					response(cl, parv[1], ERR_NOID, "");
					return 0;
				}
				if (bcom->nivel & cl->nivel || bcom->nivel == TODOS)
					bcom->func(cl, parv, parc, param, params);
				else
				{
					if (cl->nivel & USER)
						response(cl, parv[1], ERR_FORB, "");
					else
						response(cl, parv[1], ERR_NOID, "");
				}
				return 0;
			}
		}
		if (!EsServer(cl))
			response(cl, parv[1], ERR_DESC, "");
	}
	return 0;
}
IRCFUNC(m_whois)
{
	Modulo *mod;
	mod = busca_modulo(parv[1], modulos);
	if (mod)
	{
		sendto_serv(":%s 311 %s %s %s %s * :%s", me.nombre, parv[0], mod->nick, mod->ident, mod->host, mod->realname);
		sendto_serv(":%s 312 %s %s %s :%s", me.nombre, parv[0], mod->nick, me.nombre, me.info);
		sendto_serv(":%s 307 %s %s :Tiene el nick Registrado y Protegido", me.nombre, parv[0], mod->nick);
		sendto_serv(":%s 310 %s %s :Es un OPERador de los servicios de red", me.nombre, parv[0], mod->nick);
		sendto_serv(":%s 316 %s %s :es un roBOT oficial de la Red %s", me.nombre, parv[0], mod->nick, conf_set->red);
		sendto_serv(":%s 313 %s %s :es un IRCop", me.nombre, parv[0], mod->nick);
		sendto_serv(":%s 318 %s %s :Fin de /WHOIS", me.nombre, parv[0], mod->nick);
	}
	return 0;
}
IRCFUNC(m_nick)
{
	if (parc > 3)
	{
		Cliente *cl;
		if (parc == 7) /* NICKv1 */
			cl = nuevo_cliente(parv[0], parv[3], parv[4], parv[5], parv[4], NULL, parv[6]);
		else if (parc == 8) /* nickv1 también (¿?) */
			cl = nuevo_cliente(parv[0], parv[3], parv[4], parv[5], parv[4], NULL, parv[7]);
		else if (parc == 10) /* NICKv2 */
			cl = nuevo_cliente(parv[0], parv[3], parv[4], parv[5], parv[8], parv[7], parv[9]);
		if (conf_set->modos && conf_set->modos->usuarios)
			envia_umodos(cl, me.nombre, conf_set->modos->usuarios);
		if (conf_set->autojoin && conf_set->autojoin->usuarios)
			sendto_serv_us(&me, MSG_SVSJOIN, TOK_SVSJOIN, "%s %s", parv[0], conf_set->autojoin->usuarios);
		if (parc == 10)
		{
			if (conf_set->autojoin && conf_set->autojoin->operadores && (strchr(parv[7], 'h') || strchr(parv[7], 'o')))
				sendto_serv_us(&me, MSG_SVSJOIN, TOK_SVSJOIN, "%s %s", parv[0], conf_set->autojoin->operadores);
		}
		if (busca_modulo(parv[0], modulos))
		{
			irckill(cl, me.nombre, "Nick protegido.");
			renick_bot(parv[0]);
		}
	}
	else
		cambia_nick(cl, parv[1]);
	return 0;
}
IRCFUNC(m_topic)
{
	if (parc == 5)
	{
		Canal *cn;
		cn = info_canal(parv[1], !0);
		ircstrdup(&cn->topic, parv[4]);
		cn->ntopic = cl;
	}
	else if (parc == 4)
	{
		Canal *cn;
		cn = info_canal(parv[0], !0);
		ircstrdup(&cn->topic, parv[3]);
		cn->ntopic = cl;
	}	
	return 0;
}
IRCFUNC(m_quit)
{
	LinkCanal *lk;
	for (lk = cl->canal; lk; lk = lk->sig)
		borra_cliente_de_canal(lk->chan, cl);
	libera_cliente_de_memoria(cl);
	return 0;
}
IRCFUNC(m_kill)
{
	Cliente *al;
	if (EsServer(cl))
		return 0;
	if ((al = busca_cliente(parv[1], NULL)))
	{
		LinkCanal *lk;
		for (lk = al->canal; lk; lk = lk->sig)
			borra_cliente_de_canal(lk->chan, al);
		libera_cliente_de_memoria(al);
	}
	if (conf_set->opts & REKILL)
	{
		if (busca_modulo(parv[1], modulos))
			renick_bot(parv[1]);
	}
	return 0;
}
IRCFUNC(m_ping)
{
	sendto_serv("PONG :%s", parv[0]);
	return 0;
}
IRCFUNC(m_pass)
{
	if (strcmp(parv[0], conf_server->password->remoto))
	{
		fecho(FERR, "Contraseñas incorrectas");
		sockclose(sck, LOCAL);
	}
	return 0;
}
IRCFUNC(m_join)
{
	char *canal;
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
	{
		Canal *cn;
		if (conf_set->debug && cn->miembros == 1 && !IsAdmin(cl) && !strcmp(canal, conf_set->debug))
		{
			sendto_serv(":%s SVSPART %s %s", me.nombre, cl->nombre, canal);
			continue;
		}
		cn = info_canal(canal, !0);
		inserta_canal_en_usuario(cl, cn);
		inserta_usuario_en_canal(cn, cl);
		if (conf_set->debug && IsAdmin(cl) && cn->miembros == 1 && !strcmp(canal, conf_set->debug))
			envia_cmodos(cn, me.nombre, "+sAm");
	}
	return 0;
}
IRCFUNC(m_part)
{
	Canal *cn;
	cn = busca_canal(parv[1], NULL);
	borra_canal_de_usuario(cl, cn);
	borra_cliente_de_canal(cn, cl);
	return 0;
}
IRCFUNC(m_mode)
{
	Canal *cn;
	cn = info_canal(parv[1], !0);
	procesa_modo(cl, cn, parv + 2, parc - 2);
	if (EsServer(cl) && (conf_set->opts & NOSERVDEOP))
	{
		int i = 3, j = 1, f = ADD;
		char *modos = parv[2], modebuf[BUFSIZE], parabuf[BUFSIZE];
		Canal *cn;
		if (!(cn = busca_canal(parv[1], NULL)))
			return 1;
		modebuf[0] = '+';
		parabuf[0] = '\0';
		while (!BadPtr(modos))
		{
			switch (*modos)
			{
				case '+':
					f = ADD;
					break;
				case '-':
					f = DEL;
					break;
				case 'q':
				case 'a':
				case 'o':
				case 'h':
				case 'v':
					if (f == DEL)
					{
						modebuf[j++] = *modos;
						strcat(parabuf, parv[i]);
						strcat(parabuf, " ");
					}
				case 'b':
				case 'e':
				case 'k':
				case 'L':
				case 'l':
					i++;
					break;
			}
			modos++;
		}
		modebuf[j] = '\0';
		if (modebuf[1] != '\0')
			envia_cmodos(cn, me.nombre, "%s %s", modebuf, parabuf);
	}
	return 0;
}
IRCFUNC(m_kick)
{
	Canal *cn;
	Cliente *al;
	if (!(al = busca_cliente(parv[2], NULL)))
		return 1;
	cn = info_canal(parv[1], 0);
	borra_canal_de_usuario(al, cn);
	borra_cliente_de_canal(cn, al);
	if (EsBot(al))
	{
		if (conf_set->opts & REJOIN)
			mete_bot_en_canal(al, parv[1]);
	}
	return 0;
}
IRCFUNC(m_chghost)
{
	ircstrdup(&cl->vhost, parv[2]);
	genera_mask(cl);
	return 0;
}
IRCFUNC(m_chgident)
{
	ircstrdup(&cl->ident, parv[2]);
	genera_mask(cl);
	return 0;
}
IRCFUNC(m_chgname)
{
	ircstrdup(&cl->info, parv[2]);
	return 0;
}
IRCFUNC(m_tkl)
{
	return 0;
}
#ifdef UDB
IRCFUNC(m_db)
{
	if (parc < 5)
	{
		sendto_serv(":%s DB %s ERR 0 %i", me.nombre, cl->nombre, E_UDB_PARAMS);
		return 1;
	}
	if (match(parv[1], me.nombre))
		return 0;
	if (!strcasecmp(parv[2], "INF"))
	{
		Udb *bloq;
		bloq = coge_de_id(*parv[3]);
		if (strcmp(parv[4], bloq->data_char))
			sendto_serv(":%s DB %s RES %c %lu", me.nombre, parv[0], *parv[3], bloq->data_long);
	}
	else if (!strcasecmp(parv[2], "RES"))
	{
		u_long bytes;
		Udb *bloq;
		bloq = coge_de_id(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes < bloq->data_long) /* tiene menos, se los mandamos */
		{
			FILE *fp;
			if ((fp = fopen(bloq->item, "rb")))
			{
				char linea[512], *d;
				fseek(fp, bytes, SEEK_SET);
				while (!feof(fp))
				{
					bzero(linea, 512);
					if (!fgets(linea, 512, fp))
						break;
					if ((d = strchr(linea, '\r')))
						*d = '\0';
					if ((d = strchr(linea, '\n')))
						*d = '\0';
					if (strchr(linea, ' '))
						sendto_serv(":%s DB * INS %lu %c::%s", me.nombre, bytes, *parv[3], linea);
					else
						sendto_serv(":%s DB * DEL %lu %c::%s", me.nombre, bytes, *parv[3], linea);
					bytes = ftell(fp);
				}
			}
		}
	}
	else if (!strcasecmp(parv[2], "INS"))
	{
		char buf[1024], tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr("NCIS", tipo))
		{
			sendto_serv(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = coge_de_id(tipo);
		if (bytes != bloq->data_long)
		{
			sendto_serv(":%s DB %s ERR INS %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		sprintf_irc(buf, "%s %s", r, implode(parv, parc, 5, -1));
		parsea_linea(tipo, buf, 1);
	}
	else if (!strcasecmp(parv[2], "DEL"))
	{
		char tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr("NCIS", tipo))
		{
			sendto_serv(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = coge_de_id(tipo);
		if (bytes != bloq->data_long)
		{
			sendto_serv(":%s DB %s ERR DEL %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		parsea_linea(tipo, r, 1);
	}
	else if (!strcasecmp(parv[2], "DRP"))
	{
		FILE *fp;
		Udb *bloq;
		char *contenido = NULL;
		u_long bytes;
		if (!strchr("NCIS", *parv[3]))
		{
			sendto_serv(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		bloq = coge_de_id(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes > bloq->data_long)
		{
			sendto_serv(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_LEN, *parv[3]);
			return 1;
		}
		if ((fp = fopen(bloq->item, "rb")))
		{
			contenido = Malloc(bytes);
			if (fread(contenido, 1, bytes, fp) != bytes)
			{
				fclose(fp);
				sendto_serv(":%s DB %s ERR DRP %i %c fread", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
				return 1;
			}
			fclose(fp);
			if ((fp = fopen(bloq->item, "wb")))
			{
				int id;
				id = coge_de_char(*parv[3]);
				if (fwrite(contenido, 1, bytes, fp) != bytes)
				{
					fclose(fp);
					sendto_serv(":%s DB %s ERR DRP %i %c fwrite", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
					return 1;
				}
				fclose(fp);
				actualiza_hash(bloq);
				descarga_bloque(id);
				carga_bloque(id);
			}
			else
			{
				sendto_serv(":%s DB %s ERR DRP %i %c fopen(wb)", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
				return 1;
			}
			if (contenido)
				Free(contenido);
		}
		else
		{
			sendto_serv(":%s DB %s ERR DRP %i %c fopen(rb)", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
			return 1;
		}
	}
	return 0;
}
IRCFUNC(m_dbq)
{
	return 0;
}
#endif
IRCFUNC(m_sethost)
{
	return 0;
}
IRCFUNC(m_version)
{
	sendto_serv(":%s 351 %s :%s .%s. iniciado el %s", me.nombre, parv[0], COLOSSUS_VERSION, conf_set->red, _asctime(&inicio));
	return 0;
}
IRCFUNC(m_stats)
{
	switch(*parv[1])
	{
		case 'u':
		{
			time_t dura;
			dura = time(0) - inicio;
			sendto_serv(":%s 242 %s :Servidor online %i días, %02i:%02i:%02i", me.nombre, parv[0], dura / 86400, (dura / 3600) % 24, (dura / 60) % 60, dura % 60);
			break;
		}
	}
	sendto_serv(":%s 219 %s %s :Fin de /STATS", me.nombre, parv[0], parv[1]);
	return 0;
}
IRCFUNC(m_eos)
{
	if (cl == link)
	{
		sendto_serv(":%s VERSION", me.nombre);
		sendto_serv(":%s %s :Sincronización realizada en %.3f segs", me.nombre, TOK_WALLOPS, abs(microtime() - tburst));
		intentos = 0;
#ifdef _WIN32		
		ChkBtCon(1, 0);
#endif		
	}
	return 0;
}
IRCFUNC(m_server)
{
	Cliente *al;
	char *protocol, *opts, *numeric, *inf;
	protocol = opts = numeric = inf = NULL;
	strcpy(tokbuf, parv[parc - 1]);
	protocol = strtok(tokbuf, "-");
	if (protocol) /* esto nunca dara null */
		opts = strtok(NULL, "-");
	if (opts) /* hay parámetros */
		numeric = strtok(NULL, " ");
	else
	{
		protocol = NULL;
		inf = parv[parc - 1];
	}
	if (numeric)
		inf = strtok(NULL, "");
	else
		numeric = parv[parc - 2];
	if (!cl && atoi(protocol + 1) < 2302)
	{
		fecho(FERR, "Version UnrealIRCd incorrecta. Solo funciona con v3.2.x y v3.1.x");
		sockclose(sck, LOCAL);
		return 1;
	}
	al = nuevo_cliente(cl ? parv[1] : parv[0], NULL, NULL, NULL, NULL, NULL, inf);
	al->protocol = protocol ? atoi(protocol + 1) : 0;
	al->numeric = numeric ? atoi(numeric) : 0;
	al->tipo = ES_SERVER;
	al->opts = strdup(opts);
	if (!cl) /* primer link */
	{
#ifdef ZIP_LINKS
		if (conf_server->compresion)
		{
			if (zip_init(sck, conf_server->compresion) == -1)
			{
				fecho(FERR, "No puede hacer zip_init");
				zip_free(sck);
				cierra_ircd(NULL, NULL);
				return 0;
			}
			sck->opts |= OPT_ZIP;
			sck->zip->first = 1;
		}
#endif
		link = al;
		sincroniza(sck, al, parv, parc);
	}
	else
		ircd_log(LOG_SERVER, "Servidor %s (%s)", al->nombre, al->info);
	return 0;
}
IRCFUNC(m_squit)
{
	Cliente *al;
	al = busca_cliente(parv[0], NULL);
	libera_cliente_de_memoria(al);
	return 0;
}
IRCFUNC(m_netinfo)
{
	if (*parv[3] != '*' && strcmp(parv[3], conf_set->cloak->crc))
		fecho(FADV, "Las cloak keys no se corresponden (%s != %s)", conf_set->cloak->crc, parv[3]);
	//if (strcasecmp(parv[7], conf_set->red))
	//	fecho(FADV, "El nombre de la red es distinto");
	return 0;
}
IRCFUNC(m_umode)
{
	procesa_umodos(cl, parv[1]);
	return 0;
}
IRCFUNC(m_error)
{
	ircd_log(LOG_ERROR, "ERROR: %s", parv[1]);
	return 0;
}
IRCFUNC(m_away)
{
	if (parc == 2)
		cl->nivel |= AWAY;
	else if (parc == 1)
		cl->nivel &= ~AWAY;
	return 0;
}
IRCFUNC(m_rehash)
{
	if (!strcasecmp(parv[1], me.nombre))
		refresca();
	return 0;
}
IRCFUNC(m_module)
{
	Modulo *ex;
	char send[512], minbuf[512];
	for (ex = modulos; ex; ex = ex->sig)
	{
		sprintf_irc(send, "Módulo %s.", ex->info->nombre);
		if (ex->info->version)
		{
			sprintf_irc(minbuf, " Versión %.1f.", ex->info->version);
			strcat(send, minbuf);
		}
		if (ex->info->autor)
		{
			sprintf_irc(minbuf, " Autor: %s.", ex->info->autor); 
			strcat(send, minbuf);
		}
		if (ex->info->email)
		{
			sprintf_irc(minbuf, " Email: %s.", ex->info->email);
			strcat(send, minbuf);
		}
		sendto_serv(":%s %s %s :%s", me.nombre, TOK_NOTICE, cl->nombre, send);
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
		lugar = busca_cliente_en_hash(nick, lugar);
	return lugar;
}
Canal *busca_canal(char *canal, Canal *lugar)
{
	if (canal)
		lugar = busca_canal_en_hash(canal, lugar);
	return lugar;
}
Cliente *nuevo_cliente(char *nombre, char *ident, char *host, char *server, char *vhost, char *umodos, char *info)
{
	Cliente *cl;
	char *oct;
	cl = (Cliente *)Malloc(sizeof(Cliente));
	cl->nombre = nombre ? strdup(nombre) : NULL;
	cl->ident = ident ? strdup(ident) : NULL;
	cl->host = host ? strdup(host) : NULL;
	if (EsIp(host)) /* es ip, la resolvemos */
		cl->rvhost = dominum(host);
	else /* ya es host, apuntamos a host */
		cl->rvhost = cl->host;
	cl->server = server ? busca_cliente(server, NULL) : NULL;
	cl->vhost = vhost ? strdup(vhost) : NULL;
	cl->mask = NULL;
	cl->vmask = NULL;
	cl->nivel = NOTID;
	cl->modos = 0L;
	if (umodos)
		procesa_umodos(cl, umodos);
	cl->info = info ? strdup(info) : NULL;
	cl->canales = 0;
	cl->sck = SockActual;
	cl->canal = NULL;
	cl->opts = NULL;
	cl->tipo = ES_CLIENTE;
	cl->fundadores = 0;
	cl->ultimo_reg = 0L;
	genera_mask(cl);
	cl->sig = clientes;
	cl->prev = NULL;
	if (clientes)
		clientes->prev = cl;
	clientes = cl;
	inserta_cliente_en_hash(cl, nombre);
	return cl;
}
void cambia_nick(Cliente *cl, char *nuevonick)
{
	if (!cl)
		return;
	borra_cliente_de_hash(cl, cl->nombre);
	ircstrdup(&cl->nombre, nuevonick);
	genera_mask(cl);
	inserta_cliente_en_hash(cl, nuevonick);
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
		inserta_canal_en_hash(cn, canal);
		signal1(SIGN_CREATE_CHAN, cn);
	}
	return cn;
}
void inserta_canal_en_usuario(Cliente *cl, Canal *cn)
{
	LinkCanal *lk;
	if (!cl)
		return;
	lk = (LinkCanal *)Malloc(sizeof(LinkCanal)+1);
	lk->chan = cn;
	lk->sig = cl->canal;
	cl->canal = lk;
	cl->canales++;
}
void inserta_usuario_en_canal(Canal *cn, Cliente *cl)
{
	LinkCliente *lk;
	if (!cl)
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
void procesa_umodos(Cliente *cl, char *modos)
{
	if (!cl)
		return;
	flags2modes(&(cl->modos), modos, uModes);
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
void procesa_modo(Cliente *cl, Canal *cn, char *parv[], int parc)
{
	int modo = ADD, param = 1;
	char *mods = parv[0];
	while (*mods)
	{
		switch(*mods)
		{
			case '+':
				modo = ADD;
				break;
			case '-':
				modo = DEL;
				break;
			case 'b':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_ban_en_canal(cl, cn, parv[param]);
				else
					borra_ban_de_canal(cn, parv[param]);
				param++;
				break;
			case 'e':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_exc_en_canal(cl, cn, parv[param]);
				else
					borra_exc_de_canal(cn, parv[param]);
				param++;
				break;
			case 'k':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(&cn->clave, parv[param]);
					cn->modos |= flag2mode('k', chModes);
				}
				else
				{
					if (cn->clave)
						Free(cn->clave);
					cn->clave = NULL;
					cn->modos &= ~flag2mode('k', chModes);
				}
				param++;
				break;
			case 'f':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(&cn->flood, parv[param]);
					cn->modos |= flag2mode('f', chModes);
				}
				else
				{
					if (cn->flood)
						Free(cn->flood);
					cn->flood = NULL;
					cn->modos &= ~flag2mode('f', chModes);
				}
				param++;
				break;
			case 'l':
				if (modo == ADD)
				{
					if (!parv[param])
						break;
					cn->limite = atoi(parv[param]);
					cn->modos |= flag2mode('l', chModes);
					param++;
				}
				else
				{
					cn->limite = 0;
					cn->modos &= ~flag2mode('l', chModes);
				}
				break;
			case 'L':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(&cn->link, parv[param]);
					cn->modos |= flag2mode('L', chModes);
				}
				else
				{
					if (cn->link)
						Free(cn->link);
					cn->link = NULL;
					cn->modos &= ~flag2mode('L', chModes);
				}
				param++;
				break;
			case 'q':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->owner, busca_cliente(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->owner, busca_cliente(parv[param], NULL));
				param++;
				break;
			case 'a':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->admin, busca_cliente(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->admin, busca_cliente(parv[param], NULL));
				param++;
				break;
			case 'o':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->op, busca_cliente(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->op, busca_cliente(parv[param], NULL));
				param++;
				break;
			case 'h':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->half, busca_cliente(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->half, busca_cliente(parv[param], NULL));
				param++;
				break;
			case 'v':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->voz, busca_cliente(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->voz, busca_cliente(parv[param], NULL));
				param++;
				break;
			default:
				if (modo == ADD)
					cn->modos |= flag2mode(*mods, chModes);
				else
					cn->modos &= ~flag2mode(*mods, chModes);
		}
		mods++;
	}
}
void procesa_modos(Canal *cn, char *modos)
{
	char *arv[256];
	int arc = 0;
	bzero(arv, 256);
	arv[arc++] = modos;
	do
	{
		if (*modos == 0x20)
		{
			do
			{
				*modos++ = 0x0;
			} while(*modos == 0x20);
			arv[arc++] = modos;
		}
		modos++;
	} while(*modos != 0x0);
	procesa_modo(&me, cn, arv, arc);
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
	if (!cl)
		return;
	lk = (LinkCliente *)Malloc(sizeof(LinkCliente));
	lk->user = cl;
	lk->sig = *link;
	*link = lk;
}
int borra_modo_cliente_de_canal(LinkCliente **link, Cliente *cl)
{
	LinkCliente *aux, *prev = NULL;
	if (!cl)
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
IRCFUNC(sincroniza)
{
	Modulo *ex;
	tburst = microtime();
#ifdef UDB
	sendto_serv(":%s DB %s INF N %s", me.nombre, cl->nombre, nicks->data_char);
	sendto_serv(":%s DB %s INF C %s", me.nombre, cl->nombre, chans->data_char);
	sendto_serv(":%s DB %s INF I %s", me.nombre, cl->nombre, ips->data_char);
	sendto_serv(":%s DB %s INF S %s", me.nombre, cl->nombre, sets->data_char);
#endif	
	conecta_bots();
	//modos de canal
	//topics
	//tkls
	//sqlines
	sendto_serv_us(&me, MSG_EOS, TOK_EOS, "");
	signal(SIGN_EOS);
	return 0;
}
IRCFUNC(m_105)
{
	int i;
	for (i = 2; i < parc; i++)
	{
		if (!match("NICKLEN*", parv[i]))
		{
			conf_set->nicklen = atoi(strchr(parv[i], '=') + 1);
			break;
		}
		else if (!match("NETWORK*", parv[i]))
		{
			char *p = strchr(parv[i], '=') + 1;
			ircstrdup(&conf_set->red, p);
			break;
		}
	}
	return 0;
}
IRCFUNC(m_sajoin)
{
	Cliente *bl;
	if ((bl = busca_cliente(parv[1], NULL)))
		mete_bot_en_canal(bl, parv[2]);
	return 0;
}
IRCFUNC(m_sapart)
{
	Cliente *bl;
	if (busca_cliente(parv[1], NULL))
		sendto_serv(":%s %s %s", parv[1], TOK_PART, parv[2]);
	return 0;
}
void distribuye_me(Cliente *me, Sock **sck)
{
	me->nombre = strdup(conf_server->host);
	me->tipo = ES_SERVER;
	me->numeric = conf_server->numeric;
	me->protocol = PROTOCOL;
	me->opts = strdup("ihWE");
	me->info = strdup(conf_server->info);
	me->sck = *sck;
	me->sig = clientes;
	me->prev = NULL;
	me->host = strdup("0.0.0.0");
	if (clientes)
		clientes->prev = me;
	clientes = me;
	inserta_cliente_en_hash(me, conf_server->host);
}
void envia_umodos(Cliente *cl, char *emisor, char *modos)
{
	if (!cl)
		return;
	procesa_umodos(cl, modos);
#ifdef UDB
	sendto_serv(":%s %s %s %s %lu", emisor, TOK_SVS3MODE, cl->nombre, modos, time(0));
#else
	sendto_serv(":%s %s %s %s %lu", emisor, TOK_SVS2MODE, cl->nombre, modos, time(0));
#endif
	signal2(SIGN_UMODE, cl, modos);
}
void envia_cmodos(Canal *cn, char *emisor, char *modos, ...)
{
	char buf[BUFSIZE], *copy;
	va_list vl;
	if (!cn)
		return;
	va_start(vl, modos);
	vsprintf_irc(buf, modos, vl);
	va_end(vl);
	copy = strdup(buf);
	procesa_modos(cn, buf);
	sendto_serv(":%s %s %s %s", emisor, TOK_MODE, cn->nombre, copy);
	Free(copy);
}
void irckill(Cliente *cl, char *emisor, char *msg, ...)
{
	char buf[BUFSIZE];
	LinkCanal *lk;
	va_list vl;
	if (!cl)
		return;
	va_start(vl, msg);
	vsprintf_irc(buf, msg, vl);
	va_end(vl);
	sendto_serv(":%s %s %s :%s", emisor, TOK_KILL, cl->nombre, buf);
	signal2(SIGN_QUIT, cl, buf);
	for (lk = cl->canal; lk; lk = lk->sig)
		borra_cliente_de_canal(lk->chan, cl);
	libera_cliente_de_memoria(cl);
}
char *ircdmask(char *mascara)
{
	buf[0] = '\0';
	if (!strchr(mascara, '.') && !strchr(mascara, '@'))
		sprintf_irc(buf, "%s!*@*", mascara);
	else if (strchr(mascara, '.') && !strchr(mascara, '@'))
		sprintf_irc(buf, "*!*@%s", mascara);
	else if (strchr(mascara, '@') && strchr(mascara, '!'))
		sprintf_irc(buf, "%s", mascara);
	else if (strchr(mascara, '@') && !strchr(mascara, '!'))
		sprintf_irc(buf, "*!%s", mascara);
	else 
		sprintf_irc(buf, "%s", mascara);
	return buf;
}
void irckick(char *emisor, Cliente *al, Canal *cn, char *motivo, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, motivo);
	vsprintf_irc(buf, motivo, vl);
	va_end(vl);
	if (!al || !cn)
		return;
	if (EsServer(al) || EsBot(al))
		return;
	sendto_serv(":%s %s %s %s :%s", emisor, TOK_KICK, cn->nombre, al->nombre, buf);
	borra_canal_de_usuario(al, cn);
	borra_cliente_de_canal(cn, al);
}
void irctkl(char modo, char tkl, char *ident, char *host, char *mascara, int tiempo, char *motivo)
{
	if (modo == TKL_ADD)
		sendto_serv(":%s %s + %c %s %s %s %lu %lu :%s", 
			me.nombre, 
			TOK_TKL, 
			tkl, 
			ident, 
			host, 
			mascara, 
			tiempo == 0 ? 0 : time(NULL) + tiempo,
			time(NULL),
			motivo ? motivo : "");
	else if (modo == TKL_DEL)
		sendto_serv(":%s %s - %c %s %s %s", 
			me.nombre, 
			TOK_TKL, 
			tkl, 
			ident, 
			host, 
			mascara);
}
void irctopic(char *nick, Canal *cn, char *topic)
{
	if (!cn || !nick)
		return;
	if (!cn->topic || strcmp(cn->topic, topic))
	{
		sendto_serv(":%s %s %s :%s", nick, TOK_TOPIC, cn->nombre, topic);
		ircstrdup(&cn->topic, topic);
	}
}
void mete_bot_en_canal(Cliente *bl, char *canal)
{
	Canal *cn;
	cn = info_canal(canal, !0);
	inserta_canal_en_usuario(bl, cn);
	inserta_usuario_en_canal(cn, bl);
	sendto_serv(":%s %s %s", bl->nombre, TOK_JOIN, canal);
	if (conf_set->opts & AUTOBOP)
		sendto_serv(":%s %s %s +o %s", me.nombre, TOK_MODE, canal, bl->nombre);
}
void saca_bot_de_canal(Cliente *bl, char *canal, char *motivo)
{
	Canal *cn;
	cn = info_canal(canal, 0);
	borra_canal_de_usuario(bl, cn);
	borra_cliente_de_canal(cn, bl);
	if (motivo)
		sendto_serv(":%s %s %s :%s", bl->nombre, TOK_PART, canal, motivo);
	else
		sendto_serv(":%s %s %s", bl->nombre, TOK_PART, canal);
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
#ifdef UDB
DLLFUNC void envia_registro_bdd(char *item, ...)
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
	int i;
	LinkCanal *aux, *tmp;
	if (!cl)
		return;
	borra_cliente_de_hash(cl, cl->nombre);
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
	ircfree(cl->vhost);
	ircfree(cl->mask);
	ircfree(cl->vmask);
	ircfree(cl->opts);
	ircfree(cl->info);
	for (i = 0; i < cl->fundadores; i++)
		ircfree(cl->fundador[i]);
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
	borra_canal_de_hash(cn, cn->nombre);
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
	signal1(SIGN_DESTROY_CHAN, nombre);
	Free(nombre);
}
int reinicia()
{
	sendto_serv(":%s %s :Los servicios se están reiniciando...", me.nombre, TOK_WALLOPS);
	sendto_serv("SQUIT %s :Reiniciando", me.nombre);
	reset = 1;
	return 0;
}
int refresca()
{
	Conf config;
	descarga_modulos();
	parseconf("colossus.conf", &config, 1);
	distribuye_conf(&config);
	carga_modulos();
#ifdef UDB
	carga_bloques();
#endif
	if (SockIrcd)
		conecta_bots();
	return 0;
}
Cliente *botnick(char *nick, char *ident, char *host, char *server, char *modos, char *realname)
{
	Cliente *al;
	if ((al = busca_cliente(nick, NULL)) && !EsBot(al))
		irckill(al, me.nombre, "Nick protegido.");
	al = nuevo_cliente(nick, ident, host, server, host, modos, realname);
	al->tipo = ES_BOT;
	sendto_serv("%s %s 1 %lu %s %s %s 0 +%s %s :%s", TOK_NICK, nick, time(0), ident, host, server, modos, host, realname);
	return al;
}
void killbot(char *bot, char *motivo)
{
	Cliente *bl;
	if ((bl = busca_cliente(bot, NULL)) && EsBot(bl))
	{
		LinkCanal *lk;
		sendto_serv(":%s %s :%s", bot, TOK_QUIT, motivo ? motivo : "Cerrando servicio...");
		for (lk = bl->canal; lk; lk = lk->sig)
			borra_cliente_de_canal(lk->chan, bl);
		libera_cliente_de_memoria(bl);
	}
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
void conecta_bots()
{
	Modulo *ex;
	char *canal;
	for (ex = modulos; ex; ex = ex->sig)
	{
		if (ex->cargado)
		{
			Cliente *bl;
			bl = botnick(ex->nick, ex->ident, ex->host, me.nombre, ex->modos, ex->realname);
			if (ex->residente)
			{
				strcpy(tokbuf, ex->residente);
				for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
					mete_bot_en_canal(bl, canal);
			}
		}
	}
}
