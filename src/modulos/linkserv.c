/*
 * $Id: linkserv.c,v 1.6 2004-09-23 17:01:50 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "linkserv.h"
#include "chanserv.h"

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

DLLFUNC int linkserv_sig_mysql();
DLLFUNC int linkserv_sig_eos();
DLLFUNC SOCKFUNC(linkserv_sockop);
DLLFUNC SOCKFUNC(linkserv_sockre);
DLLFUNC SOCKFUNC(linkserv_sockcl);
#ifndef _WIN32
int (*es_fundador_dl)(Cliente *, char *);
#else
#define es_fundador_dl es_fundador
#endif

void set(Conf *, Modulo *);
int test(Conf *, int *);

DLLFUNC ModInfo info = {
	"LinkServ" ,
	0.2 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
	"linkserv.inc"
};
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (parseconf(mod->config, &modulo, 1))
		return 1;
	if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
	{
		if (!test(modulo.seccion[0], &errores))
			set(modulo.seccion[0], mod);
	}
	else
	{
		conferror("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
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
DLLFUNC int descarga()
{
	borra_senyal(SIGN_MYSQL, linkserv_sig_mysql);
	borra_senyal(SIGN_EOS, linkserv_sig_eos);
	return 0;
}
int test(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = busca_entrada(config, "nick")))
	{
		conferror("[%s:%s] No se encuentra la directriz nick.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "ident")))
	{
		conferror("[%s:%s] No se encuentra la directriz ident.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "realname")))
	{
		conferror("[%s:%s] No se encuentra la directriz realname.]\n", config->archivo, config->item);
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
		da_Malloc(linkserv, LinkServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&linkserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&linkserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&linkserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&linkserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&linkserv->modos, config->seccion[i]->data);
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
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&linkserv->residente, config->seccion[i]->data);
	}
	if (linkserv->mascara)
		Free(linkserv->mascara);
	linkserv->mascara = (char *)Malloc(sizeof(char) * (strlen(linkserv->nick) + 1 + strlen(linkserv->ident) + 1 + strlen(linkserv->host) + 1));
	sprintf_irc(linkserv->mascara, "%s!%s@%s", linkserv->nick, linkserv->ident, linkserv->host);
	inserta_senyal(SIGN_MYSQL, linkserv_sig_mysql);
	inserta_senyal(SIGN_EOS, linkserv_sig_eos);
	mod->nick = linkserv->nick;
	mod->ident = linkserv->ident;
	mod->host = linkserv->host;
	mod->realname = linkserv->realname;
	mod->residente = linkserv->residente;
	mod->modos = linkserv->modos;
	mod->comandos = linkserv->comando;
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
	serv = strdup(_mysql_get_registro(LS_MYSQL, red, "servidor"));
	puerto = _mysql_get_registro(LS_MYSQL, red, "puerto");
	if (!(links[linkS]->sck = sockopen(serv, atoi(puerto), linkserv_sockop, linkserv_sockre, NULL, linkserv_sockcl, ADD)))
	{
		Free(links[linkS]);
		Free(serv);
		sendto_serv(":%s %s :Es imposible establecer la conexión con %s:%s (%s)", me.nombre, TOK_WALLOPS, serv, puerto, linkserv->nick);
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
				sockclose(links[i]->sck, LOCAL);
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
		MYSQL_RES *res;
		MYSQL_ROW row;
		if (!(res = _mysql_query("SELECT * from %s%s", PREFIJO, LS_MYSQL)))
		{
			response(cl, linkserv->nick, LS_ERR_EMPT, "No existen redes para listar.");
			return 1;
		}
		while ((row = mysql_fetch_row(res)))
		{
			if (!BadPtr(row[3]))
				response(cl, linkserv->nick, "Red: \00312%s\003 Servidor: \00312%s\003:\00312%s\003 con el nick \00312%s", row[0], row[1], row[2], row[3]);
			else
				response(cl, linkserv->nick, "Red: \00312%s\003 Servidor: \00312%s\003:\00312%s", row[0], row[1], row[2]);
		}
		mysql_free_result(res);
	}
	else
	{
		if (!IsAdmin(cl))
		{
			response(cl, linkserv->nick, LS_ERR_FORB, "");
			return 1;
		}
		if (*param[1] == '+')
		{
			char *port = NULL;
			if (params < 3)
			{
				response(cl, linkserv->nick, LS_ERR_PARA, "RED +red servidor[:puerto] [nick]");
				return 1;
			}
			param[1]++;
			if (_mysql_get_registro(LS_MYSQL, param[1], NULL))
			{
				response(cl, linkserv->nick, LS_ERR_EMPT, "Esta red ya está en la lista.");
				return 1;
			}
			if ((port = strchr(param[2], ':')))
				*port++ = '\0';
			_mysql_add(LS_MYSQL, param[1], "servidor", param[2]);
			if (params > 3)
				_mysql_add(LS_MYSQL, param[1], "nick", param[3]);
			else
				_mysql_add(LS_MYSQL, param[1], "nick", random_ex("u??????"));
			if (port)
				_mysql_add(LS_MYSQL, param[1], "puerto", port);
			linkserv_conecta_red(param[1]);
			response(cl, linkserv->nick, "Se ha añadido la red \00312%s", param[1]);
		}
		else if (*param[1] == '-')
		{
			param[1]++;
			if (!_mysql_get_registro(LS_MYSQL, param[1], NULL))
			{
				response(cl, linkserv->nick, LS_ERR_EMPT, "Esta red no se encuentra.");
				return 1;
			}
			_mysql_del(LS_MYSQL, param[1]);
			linkserv_cierra_red(param[1]);
			response(cl, linkserv->nick, "Se ha borrado la red \00312%s", param[1]);
		}
		else
		{
			response(cl, linkserv->nick, LS_ERR_SNTX, "RED {+|-}red servidor[:puerto] [nick]");
			return 1;
		}
	}
	return 0;
}
BOTFUNC(linkserv_link)
{
	Cliente *bl;
	Sock *sck;
	if (params < 3)
	{
		response(cl, linkserv->nick, LS_ERR_PARA, "LINK red {+|-}#canal");
		return 1;
	}
	if (!IsAdmin(cl) && !es_fundador_dl(cl, param[2] + 1))
	{
		response(cl, linkserv->nick, LS_ERR_FORB, "");
		return 1;
	}
	if (!GetLinkRed(param[1]) || !(sck = GetLinkRed(param[1])->sck))
	{
		response(cl, linkserv->nick, LS_ERR_EMPT, "Esta red no está disponible.");
		return 1;
	}
	bl = busca_cliente(linkserv->nick, NULL);
	if (*param[2] == '+')
	{
		sockwrite(sck, OPT_CRLF, "JOIN %s", param[2] + 1);
		mete_bot_en_canal(bl, param[2] + 1);
	}
	else if (*param[2] == '-')
	{
		sockwrite(sck, OPT_CRLF, "PART %s", param[2] + 1);
		saca_bot_de_canal(bl, param[2] + 1, "Separación recibida");
	}
	else
	{
		response(cl, linkserv->nick, LS_ERR_SNTX, "LINK red {+|-}#canal");
		return 1;
	}
	response(cl, linkserv->nick, "%s con el canal \00312%s\003 de \00312%s\003 realizado.", *param[2] == '+' ? "Unión" : "Separación", param[2] + 1, param[1]);
	return 0;
}
int linkserv_sig_mysql()
{
	char buf[1][BUFSIZE], tabla[1];
	int i;
	tabla[0] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, LS_MYSQL);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`servidor` varchar(255) default NULL, "
  			"`puerto` varchar(255) NOT NULL default '6667', "
  			"`nick` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de excepción de hosts';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
	_mysql_carga_tablas();
	return 0;
}
int linkserv_sig_eos()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (!links) /* primer arranque */
	{
		if ((res = _mysql_query("SELECT item from %s%s", PREFIJO, LS_MYSQL)))
		{
			while ((row = mysql_fetch_row(res)))
				linkserv_conecta_red(row[0]);
			mysql_free_result(res);
		}
	}
	return 0;
}
SOCKFUNC(linkserv_sockop)
{
	char *nick, *lnick;
	nick = _mysql_get_registro(LS_MYSQL, GetLinkSock(sck)->red, "nick");
	lnick = strtolower(nick);
	sockwrite(sck, OPT_CRLF, "NICK %s", nick);
	sockwrite(sck, OPT_CRLF, "USER %s %s 0 :Linkaje a la red %s", lnick, lnick, conf_set->red ? conf_set->red : "NULL");
	return 0;
}
SOCKFUNC(linkserv_sockre)
{
	char *lparv[256], sender, *com = NULL;
	int lparc = 0;
#ifdef DEBUG
	Debug("& %s", data);
#endif
	lparc = tokeniza_irc(data, lparv, &com, &sender);
	if (com)
	{
		if (!strcmp(com, "PING"))
			sockwrite(sck, OPT_CRLF, "PONG :%s", lparv[0]);
		else if (!strcmp(com, "PRIVMSG"))
		{
			if (*lparv[1] == '#' && *lparv[2] != '!')
				sendto_serv(":%s %s %s :! (%s) %s", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparv[2]);
		}
		else if (!strcmp(com, "JOIN"))
			sendto_serv(":%s %s %s :! \2JOIN\2 (%s)", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparv[1]);
		else if (!strcmp(com, "PART"))
			sendto_serv(":%s %s %s :! \2PART\2 (%s) %s", linkserv->nick, TOK_PRIVATE, lparv[1], strtok(lparv[0], "!"), lparc > 2 ? lparv[2] : "");
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
