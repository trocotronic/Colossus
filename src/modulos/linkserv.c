/*
 * $Id: linkserv.c,v 1.17 2008-02-13 16:16:10 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "modulos/linkserv.h"
#include "modulos/chanserv.h"

LinkServ *linkserv = NULL;
Links **links = NULL;
int linkS = 0;

static int linkserv_help			(Cliente *, char *[], int, char *[], int);
static int linkserv_red			(Cliente *, char *[], int, char *[], int);
static int linkserv_link			(Cliente *, char *[], int, char *[], int);

static bCom linkserv_coms[] = {
	{ "help" , linkserv_help , TODOS } ,
	{ "red" , linkserv_red , USERS } ,
	{ "link" , linkserv_link , USERS } ,
	{ 0x0 , 0x0 , 0x0 }
};

int linkserv_sig_mysql();
int linkserv_sig_eos();
int linkserv_sig_bot();
SOCKFUNC(linkserv_sockop);
SOCKFUNC(linkserv_sockre);
SOCKFUNC(linkserv_sockcl);
#ifndef _WIN32
int (*es_fundador_dl)(Cliente *, char *);
#else
#define es_fundador_dl es_fundador
#endif

void set(Conf *, Modulo *);
int test(Conf *, int *);

ModInfo info = {
	"LinkServ" ,
	0.2 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"linkserv.inc"
};
int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (ParseaConfiguracion(mod->config, &modulo, 1))
		return 1;
	if (!strcasecmp(modulo.seccion[0]->item, Mod_Info.nombre))
	{
		if (!test(modulo.seccion[0], &errores))
			set(modulo.seccion[0], mod);
	}
	else
	{
		Error("[%s] La configuracion de %s es erronea", mod->archivo, Mod_Info.nombre);
		errores++;
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
			{
				irc_dlsym(ex->hmod, "es_fundador", es_fundador_dl);
				break;
			}
		}
	}
#endif
	return errores;
}
int descarga()
{
	BorraSenyal(SIGN_SQL, linkserv_sig_mysql);
	BorraSenyal(SIGN_EOS, linkserv_sig_eos);
	BorraSenyal(SIGN_BOT, linkserv_sig_bot);
	return 0;
}
int test(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "nick")))
	{
		Error("[%s:%s] No se encuentra la directriz nick.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "ident")))
	{
		Error("[%s:%s] No se encuentra la directriz ident.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "host")))
	{
		Error("[%s:%s] No se encuentra la directriz host.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "realname")))
	{
		Error("[%s:%s] No se encuentra la directriz realname.]\n", config->archivo, config->item);
		error_parcial++;
	}
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *ls;
	if (!linkserv)
		linkserv = BMalloc(LinkServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(linkserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(linkserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(linkserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(linkserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(linkserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ls = &linkserv_coms[0];
				while (ls->com != 0x0)
				{
					if (!strcasecmp(ls->com, config->seccion[i]->seccion[p]->item))
					{
						linkserv->comando[linkserv->comandos++] = ls;
						break;
					}
					ls++;
				}
				if (ls->com == 0x0)
					Error("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(linkserv->residente, config->seccion[i]->data);
	}
	if (linkserv->mascara)
		Free(linkserv->mascara);
	linkserv->mascara = (char *)Malloc(sizeof(char) * (strlen(linkserv->nick) + 1 + strlen(linkserv->ident) + 1 + strlen(linkserv->host) + 1));
	ircsprintf(linkserv->mascara, "%s!%s@%s", linkserv->nick, linkserv->ident, linkserv->host);
	InsertaSenyal(SIGN_SQL, linkserv_sig_mysql);
	InsertaSenyal(SIGN_EOS, linkserv_sig_eos);
	InsertaSenyal(SIGN_BOT, linkserv_sig_bot);
	bot_mod(linkserv);
}
/* devuelve el puntero al link a partir del sock */
Links *GetLinkSock(Sock *sck)
{
	int i;
	for (i = 0; i < linkS; i++)
	{
		if (links[i]->sck == sck)
			return links[i];
	}
	return NULL; /* no debería pasar nunca */
}
/* devuelve el puntero al link a partir de la red */
Links *GetLinkRed(char *red)
{
	int i;
	for (i = 0; i < linkS; i++)
	{
		if (!strcasecmp(links[i]->red, red))
			return links[i];
	}
	return NULL; /* no debería pasar nunca */
}
int linkserv_conecta_red(char *red)
{
	char *serv, *puerto;
	if (!links)
		links = (Links **)Malloc(sizeof(Links *));
	links[linkS] = (Links *)Malloc(sizeof(Links));
	serv = strdup(SQLCogeRegistro(LS_SQL, red, "servidor"));
	puerto = SQLCogeRegistro(LS_SQL, red, "puerto");
	if (!(links[linkS]->sck = SockOpen(serv, atoi(puerto), linkserv_sockop, linkserv_sockre, NULL, linkserv_sockcl)))
	{
		Free(links[linkS]);
		Free(serv);
		EnviaAServidor(":%s %s :Es imposible establecer la conexión con %s:%s (%s)", me.nombre, TOK_WALLOPS, serv, puerto, linkserv->nick);
		return 0;
	}
	links[linkS]->red = strdup(red);
	Free(serv);
	linkS++;
	return 1;
}
int linkserv_cierra_red(char *red)
{
	int i;
	for (i = 0; i < linkS; i++)
	{
		if (!strcasecmp(links[i]->red, red))
		{
			if (links[i]->sck)
				SockClose(links[i]->sck);
			Free(links[i]->red);
			Free(links[i]);
			links[i] = NULL;
			for (; i < linkS; i++)
				links[i] = links[i+1];
			return 1;
		}
	}
	return 0;
}
BOTFUNC(linkserv_help)
{
	return 0;
}
BOTFUNC(linkserv_red)
{
	if (params < 2)
	{
		SQLRes res;
		SQLRow row;
		if (!(res = SQLQuery("SELECT * from %s%s", PREFIJO, LS_SQL)))
		{
			Responde(cl, linkserv->cl, LS_ERR_EMPT, "No existen redes para listar.");
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			if (!BadPtr(row[3]))
				Responde(cl, linkserv->cl, "Red: \00312%s\003 Servidor: \00312%s\003:\00312%s\003 con el nick \00312%s", row[0], row[1], row[2], row[3]);
			else
				Responde(cl, linkserv->cl, "Red: \00312%s\003 Servidor: \00312%s\003:\00312%s", row[0], row[1], row[2]);
		}
		SQLFreeRes(res);
	}
	else
	{
		if (!IsAdmin(cl))
		{
			Responde(cl, linkserv->cl, LS_ERR_FORB, "");
			return 1;
		}
		if (*param[1] == '+')
		{
			char *port = NULL;
			if (params < 3)
			{
				Responde(cl, linkserv->cl, LS_ERR_PARA, "RED +red servidor[:puerto] [nick]");
				return 1;
			}
			param[1]++;
			if (SQLCogeRegistro(LS_SQL, param[1], NULL))
			{
				Responde(cl, linkserv->cl, LS_ERR_EMPT, "Esta red ya está en la lista.");
				return 1;
			}
			if ((port = strchr(param[2], ':')))
				*port++ = '\0';
			SQLInserta(LS_SQL, param[1], "servidor", param[2]);
			if (params > 3)
				SQLInserta(LS_SQL, param[1], "nick", param[3]);
			else
				SQLInserta(LS_SQL, param[1], "nick", AleatorioEx("u??????"));
			if (port)
				SQLInserta(LS_SQL, param[1], "puerto", port);
			linkserv_conecta_red(param[1]);
			Responde(cl, linkserv->cl, "Se ha añadido la red \00312%s", param[1]);
		}
		else if (*param[1] == '-')
		{
			param[1]++;
			if (!SQLCogeRegistro(LS_SQL, param[1], NULL))
			{
				Responde(cl, linkserv->cl, LS_ERR_EMPT, "Esta red no se encuentra.");
				return 1;
			}
			SQLBorra(LS_SQL, param[1]);
			linkserv_cierra_red(param[1]);
			Responde(cl, linkserv->cl, "Se ha borrado la red \00312%s", param[1]);
		}
		else
		{
			Responde(cl, linkserv->cl, LS_ERR_SNTX, "RED {+|-}red servidor[:puerto] [nick]");
			return 1;
		}
	}
	return 0;
}
BOTFUNC(linkserv_link)
{
	Sock *sck;
	if (params < 3)
	{
		Responde(cl, linkserv->cl, LS_ERR_PARA, "LINK red {+|-}#canal");
		return 1;
	}
	if (!IsAdmin(cl) && !es_fundador_dl(cl, param[2] + 1))
	{
		Responde(cl, linkserv->cl, LS_ERR_FORB, "");
		return 1;
	}
	if (!GetLinkRed(param[1]) || !(sck = GetLinkRed(param[1])->sck))
	{
		Responde(cl, linkserv->cl, LS_ERR_EMPT, "Esta red no está disponible.");
		return 1;
	}
	if (*param[2] == '+')
	{
		SockWrite(sck, OPT_CRLF, "JOIN %s", param[2] + 1);
		EntraBot(linkserv->cl, param[2] + 1);
	}
	else if (*param[2] == '-')
	{
		SockWrite(sck, OPT_CRLF, "PART %s", param[2] + 1);
		SacaBot(linkserv->cl, param[2] + 1, "Separación recibida");
	}
	else
	{
		Responde(cl, linkserv->cl, LS_ERR_SNTX, "LINK red {+|-}#canal");
		return 1;
	}
	Responde(cl, linkserv->cl, "%s con el canal \00312%s\003 de \00312%s\003 realizado.", *param[2] == '+' ? "Unión" : "Separación", param[2] + 1, param[1]);
	return 0;
}
int linkserv_sig_mysql()
{
	char buf[1][BUFSIZE], tabla[1];
	int i;
	tabla[0] = 0;
	ircsprintf(buf[0], "%s%s", PREFIJO, LS_SQL);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
	}
	if (!SQLEsTabla(LS_SQL))
	{
		if (SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"servidor varchar(255) default NULL, "
  			"puerto varchar(255) NOT NULL default '6667', "
  			"nick varchar(255) default NULL "
			");", PREFIJO, LS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, LS_SQL);
	}
	return 0;
}
int linkserv_sig_eos()
{
	SQLRes res;
	SQLRow row;
	char *canal;
	if (linkserv->residente)
	{
		strcpy(tokbuf, linkserv->residente);
		for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
			EntraBot(linkserv->cl, canal);
	}
	if (!links) /* primer arranque */
	{
		if ((res = SQLQuery("SELECT item from %s%s", PREFIJO, LS_SQL)))
		{
			while ((row = SQLFetchRow(res)))
				linkserv_conecta_red(row[0]);
			SQLFreeRes(res);
		}
	}
	return 0;
}
int linkserv_sig_bot()
{
	Cliente *bl;
	bl = CreaBot(linkserv->nick, linkserv->ident, linkserv->host, me.nombre, linkserv->modos, linkserv->realname);
	linkserv->cl = bl;
	return 0;
}

SOCKFUNC(linkserv_sockop)
{
	char *nick, *lnick;
	nick = SQLCogeRegistro(LS_SQL, GetLinkSock(sck)->red, "nick");
	lnick = strtolower(nick);
	SockWrite(sck, OPT_CRLF, "NICK %s", nick);
	SockWrite(sck, OPT_CRLF, "USER %s %s 0 :Linkaje a la red %s", lnick, lnick, conf_set->red ? conf_set->red : "NULL");
	return 0;
}
SOCKFUNC(linkserv_sockre)
{
	char *lparv[256], sender, *com = NULL;
	int lparc = 0;
#ifdef DEBUG
	Debug("& %s", data);
#endif
	//lparc = tokeniza_irc(data, lparv, &com, &sender);
	if (com)
	{
		if (!strcmp(com, "PING"))
			SockWrite(sck, OPT_CRLF, "PONG :%s", lparv[0]);
		else if (!strcmp(com, "PRIVMSG"))
		{
			if (*lparv[1] == '#' && *lparv[2] != '!')
				EnviaAServidor(":%s %s %s :! (%s) %s", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparv[2]);
		}
		else if (!strcmp(com, "JOIN"))
			EnviaAServidor(":%s %s %s :! \2JOIN\2 (%s)", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparv[1]);
		else if (!strcmp(com, "PART"))
			EnviaAServidor(":%s %s %s :! \2PART\2 (%s) %s", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparc > 2 ? lparv[2] : "");
	}
	return 0;
}
SOCKFUNC(linkserv_sockcl)
{
	Links *lnk;
	if ((lnk = GetLinkSock(sck)))
		lnk->sck = NULL;
	return 0;
}
