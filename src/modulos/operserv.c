/*
 * $Id: operserv.c,v 1.2 2004-09-11 16:08:05 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "operserv.h"
#include "nickserv.h"
#include "chanserv.h"

OperServ *operserv = NULL;
Noticia *gnoticia[MAXNOT];
int gnoticias = 0;

static int operserv_help	(Cliente *, char *[], int, char *[], int);
static int operserv_raw		(Cliente *, char *[], int, char *[], int);
static int operserv_restart	(Cliente *, char *[], int, char *[], int);
static int operserv_rehash	(Cliente *, char *[], int, char *[], int);
static int operserv_backup	(Cliente *, char *[], int, char *[], int);
static int operserv_restaura	(Cliente *, char *[], int, char *[], int);
static int operserv_blocks	(Cliente *, char *[], int, char *[], int);
static int operserv_opers	(Cliente *, char *[], int, char *[], int);
static int operserv_sajoin	(Cliente *, char *[], int, char *[], int);
static int operserv_sapart	(Cliente *, char *[], int, char *[], int);
static int operserv_rejoin	(Cliente *, char *[], int, char *[], int);
static int operserv_kill	(Cliente *, char *[], int, char *[], int);
static int operserv_global	(Cliente *, char *[], int, char *[], int);
static int operserv_noticias	(Cliente *, char *[], int, char *[], int);
static int operserv_clones	(Cliente *, char *[], int, char *[], int);
static int operserv_close	(Cliente *, char *[], int, char *[], int);
#ifdef UDB
static int operserv_modos	(Cliente *, char *[], int, char *[], int);
#endif

static bCom operserv_coms[] = {
	{ "help" , operserv_help , OPERS } ,
	{ "raw" , operserv_raw , OPERS } ,
	{ "restart" , operserv_restart , ROOTS } ,
	{ "rehash" , operserv_rehash , ROOTS } ,
	{ "backup" , operserv_backup , ROOTS } ,
	{ "close" , operserv_close , ROOTS } ,
	{ "restaura" , operserv_restaura , ROOTS } ,
	{ "blocks" , operserv_blocks , OPERS } ,
	{ "opers" , operserv_opers , ADMINS } ,
	{ "sajoin" , operserv_sajoin , OPERS } ,
	{ "sapart" , operserv_sapart , OPERS } ,
	{ "rejoin" , operserv_rejoin , OPERS } ,
	{ "kill" , operserv_kill , OPERS } ,
	{ "global" , operserv_global , OPERS } ,
	{ "noticias" , operserv_noticias , OPERS } ,
	{ "clones" , operserv_clones , ADMINS } ,
#ifdef UDB
	{ "modos" , operserv_modos , ADMINS } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};

#ifndef UDB
DLLFUNC int operserv_idok	(Cliente *);
#endif
DLLFUNC int operserv_sig_mysql	();
DLLFUNC int operserv_sig_eos	();

DLLFUNC IRCFUNC(operserv_umode);
DLLFUNC IRCFUNC(operserv_join);
DLLFUNC IRCFUNC(operserv_nick);
#ifndef _WIN32
int (*ChanReg_dl)(char *);
int (*memoserv_send_dl)(char *, char *, char *);
#else
#define ChanReg_dl ChanReg
#define memoserv_send_dl memoserv_send
#endif
#define IsChanReg_dl(x) (ChanReg_dl(x) == 1)
#define IsChanPReg_dl(x) (ChanReg_dl(x) == 2)
void set(Conf *, Modulo *);
DLLFUNC ModInfo info = {
	"OperServ" ,
	0.5 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
};
	
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	char *file, *k;
	file = strdup(mod->archivo);
	k = strrchr(file, '.') + 1;
	*k = '\0';
	strcat(file, "inc");
	if (parseconf(file, &modulo, 1))
		return 1;
	Free(file);
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
				irc_dlsym(ex->hmod, "ChanReg", (int) ChanReg_dl);
			else if (!strcasecmp(ex->info->nombre, "MemoServ"))
				irc_dlsym(ex->hmod, "memoserv_send", (int) memoserv_send_dl);
		}
	}
#endif	
	return errores;
}
DLLFUNC int descarga()
{
	borra_comando(MSG_JOIN, operserv_join);
	borra_comando(MSG_NICK, operserv_nick);
#ifndef UDB
	signal_del(NS_SIGN_IDOK, operserv_idok);
#endif
	signal_del(SIGN_MYSQL, operserv_sig_mysql);
	signal_del(SIGN_EOS, operserv_sig_eos);
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
	bCom *os;
	if (!operserv)
		da_Malloc(operserv, OperServ);
	ircstrdup(&operserv->bline, "Bot-Lined: %s");
#ifndef UDB
	operserv->clones = 2;
#endif
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&operserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&operserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&operserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&operserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&operserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				os = &operserv_coms[0];
				while (os->com != 0x0)
				{
					if (!strcasecmp(os->com, config->seccion[i]->seccion[p]->item))
					{
						operserv->comando[operserv->comandos++] = os;
						break;
					}
					os++;
				}
				if (os->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&operserv->residente, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "bline"))
			ircstrdup(&operserv->bline, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "autoop"))
			operserv->opts |= OS_OPTS_AOP;
#ifndef UDB
		if (!strcmp(config->seccion[i]->item, "clones"))
			operserv->clones = atoi(config->seccion[i]->data);
#endif
	}
	if (operserv->mascara)
		Free(operserv->mascara);
	operserv->mascara = (char *)Malloc(sizeof(char) * (strlen(operserv->nick) + 1 + strlen(operserv->ident) + 1 + strlen(operserv->host) + 1));
	sprintf_irc(operserv->mascara, "%s!%s@%s", operserv->nick, operserv->ident, operserv->host);
	inserta_comando(MSG_JOIN, TOK_JOIN, operserv_join, INI);
	inserta_comando(MSG_NICK, TOK_NICK, operserv_nick, INI);
#ifndef UDB
	signal_add(NS_SIGN_IDOK, operserv_idok);
#endif
	signal_add(SIGN_MYSQL, operserv_sig_mysql);
	signal_add(SIGN_EOS, operserv_sig_eos);
	mod->nick = operserv->nick;
	mod->ident = operserv->ident;
	mod->host = operserv->host;
	mod->realname = operserv->realname;
	mod->residente = operserv->residente;
	mod->modos = operserv->modos;
	mod->comandos = operserv->comando;
}
int es_bot_noticia(char *botname)
{
	int i;
	for (i = 0; i < gnoticias; i++)
	{
		if (!strcasecmp(gnoticia[i]->botname, botname))
			return 1;
	}
	return 0;
}
int inserta_noticia(char *botname, char *noticia, time_t fecha, int n)
{
	Noticia *not;
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (gnoticias == MAXNOT)
		return 0;
	if (!n && !es_bot_noticia(botname))
		botnick(botname, operserv->ident, operserv->host, me.nombre, "kdBq", operserv->realname);
	not = (Noticia *)Malloc(sizeof(Noticia));
	not->botname = strdup(botname);
	not->noticia = strdup(noticia);
	not->fecha = fecha ? fecha : time(0);
	if (!n)
	{
		_mysql_query("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, botname, noticia, not->fecha);
		res = _mysql_query("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, noticia, botname);
		row = mysql_fetch_row(res);
		not->id = atoi(row[0]);
		mysql_free_result(res);
	}
	else
		not->id = n;
	gnoticia[gnoticias++] = not;
	return 1;
}
int borra_noticia(int id)
{
	int i;
	char *botname;
	for (i = 0; i < gnoticias; i++)
	{
		if (gnoticia[i]->id == id)
		{
			botname = strdup(gnoticia[i]->botname);
			Free(gnoticia[i]->botname);
			Free(gnoticia[i]->noticia);
			Free(gnoticia[i]);
			_mysql_query("DELETE from %s%s where n='%i'", PREFIJO, OS_NOTICIAS, id);
			for (; i < gnoticias; i++)
				gnoticia[i] = gnoticia[i+1];
			gnoticias--;
			if (!es_bot_noticia(botname))
				sendto_serv(":%s %s :Mensajería global %s", botname, TOK_QUIT, conf_set->red);
			Free(botname);
			return 1;
		}
	}
	return 0;
}
void operserv_carga_noticias()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if ((res = _mysql_query("SELECT * from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = mysql_fetch_row(res)))
			inserta_noticia(row[1], row[2], atol(row[3]), atoi(row[0]));
	}
}
BOTFUNC(operserv_help)
{
	if (params < 2)
	{
		response(cl, operserv->nick, "\00312%s\003 se encarga de gestionar la red de forma global y reservada al personal cualificado para realizar esta tarea.", operserv->nick);
		response(cl, operserv->nick, "Esta gestión permite tener un control exhaustivo sobre las actividades de la red para su buen funcionamiento.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Comandos disponibles:");
		response(cl, operserv->nick, "\00312RAW\003 Manda un raw al servidor.");
		response(cl, operserv->nick, "\00312BLOCKS\003 Gestiona las blocks de la red.");
		response(cl, operserv->nick, "\00312SAJOIN\003 Ejecuta SAJOIN sobre un usuario.");
		response(cl, operserv->nick, "\00312SAPART\003 Ejecuta SAPART sobre un usuario.");
		response(cl, operserv->nick, "\00312REJOIN\003 Obliga a un usuario a reentrar a un canal.");
		response(cl, operserv->nick, "\00312KILL\003 Desconecta a un usuario.");
		response(cl, operserv->nick, "\00312GLOBAL\003 Manda un mensaje global.");
		response(cl, operserv->nick, "\00312NOTICIAS\003 Administra las noticias de la red.");
		if (IsAdmin(cl))
		{
			response(cl, operserv->nick, "\00312OPERS\003 Administra los operadores de la red.");
			response(cl, operserv->nick, "\00312CLONES\003 Administra la lista de ips con más clones.");
		}
		if (IsRoot(cl))
		{
			response(cl, operserv->nick, "\00312RESTART\003 Reinicia los servicios.");
			response(cl, operserv->nick, "\00312REHASH\003 Refresca los servicios.");
			response(cl, operserv->nick, "\00312BACKUP\003 Crea una copia de seguridad de la base de datos.");
			response(cl, operserv->nick, "\00312RESTAURA\003 Restaura la base de datos.");
			response(cl, operserv->nick, "\00312CLOSE\003 Cierra el programa.");
		}
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Para más información, \00312/msg %s %s comando", operserv->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "RAW"))
	{
		response(cl, operserv->nick, "\00312RAW");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Manda un raw al servidor.");
		response(cl, operserv->nick, "Este comando actúa directamente al servidor.");
		response(cl, operserv->nick, "No es procesado por los servicios, con lo que no tiene un registro sobre él.");
		response(cl, operserv->nick, "Sólo debe usarse para fines administrativos cuya justificación sea de peso.");
		response(cl, operserv->nick, "No usárase si no se tuviera conocimiento explícito de los raw's del servidor.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312RAW linea");
		response(cl, operserv->nick, "Ejemplo: \00312RAW %s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
		response(cl, operserv->nick, "Ejecutaría el comando \00312:%s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
	}
	else if (!strcasecmp(param[1], "BLOCKS"))
	{
		response(cl, operserv->nick, "\00312BLOCKS");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Administra los bloqueos de la red.");
		response(cl, operserv->nick, "Existen varios tipos de bloqueo que se detallan a continuación:");
		response(cl, operserv->nick, "\00312G\003 - Global Line. Pone un bloqueo global a un user@host.");
		response(cl, operserv->nick, "\00312Q\003 - Q Line. Bloquea el uso de un nick.");
		response(cl, operserv->nick, "\00312s\003 - Shun. Paraliza a un usuario.");
		response(cl, operserv->nick, "\00312Z\003 - Pone un bloqueo a una ip.");
		response(cl, operserv->nick, "Nótese el uso correcto de mayúsculas/minúsculas.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312BLOCKS {+|-}|Q|Z|s|G nick|user@host [parámetros]");
		response(cl, operserv->nick, "Para añadir una block se antepone '+' antes de la block. Para quitarla, '-'.");
		response(cl, operserv->nick, "Se puede especificar un nick o un user@host indistintamente. Si se especifica un nick, se usará su user@host sin necesidad de buscarlo previamente.");
	}
	else if (!strcasecmp(param[1], "SAJOIN"))
	{
		response(cl, operserv->nick, "\00312SAJOIN");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Fuerza a un usuario a entrar en un canal.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312SAJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "SAPART"))
	{
		response(cl, operserv->nick, "\00312SAPART");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Fuerza a un usuario a salir de un canal.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312SAPART usuario #canal");
	}
	else if (!strcasecmp(param[1], "REJOIN"))
	{
		response(cl, operserv->nick, "\00312REJOIN");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Fuerza a un usuario a reentrar en un canal.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312REJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "KILL"))
	{
		response(cl, operserv->nick, "\00312KILL");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Desconecta a un usuario de la red.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312KILL usuario [motivo]");
	}
	else if (!strcasecmp(param[1], "GLOBAL"))
	{
		response(cl, operserv->nick, "\00312GLOBAL");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Envía un mensaje global.");
		response(cl, operserv->nick, "Dispone de varios parámetros para especificar sus destinatarios y el modo de envío.");
		response(cl, operserv->nick, "Estos son:");
		response(cl, operserv->nick, "\00312o\003 Exclusivo a operadores.");
		response(cl, operserv->nick, "\00312p\003 Para preopers.");
		response(cl, operserv->nick, "\00312b\003 Utiliza un bot para mandar el mensaje.");
		response(cl, operserv->nick, "\00312m\003 Vía memo.");
		response(cl, operserv->nick, "\00312n\003 Vía notice.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312GLOBAL [-parámetros [[bot]] mensaje");
		response(cl, operserv->nick, "Si no se especifican modos, se envía a todos los usuarios vía privmsg.");
	}
	else if (!strcasecmp(param[1], "NOTICIAS"))
	{
		response(cl, operserv->nick, "\00312NOTICIAS");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Gestiona las noticias de la red.");
		response(cl, operserv->nick, "Estas noticias son mostradas cuando se conecta un usuario en forma de privado.");
		response(cl, operserv->nick, "Se muestra la fecha y el cuerpo de la noticia y son mandadas por el bot especificado.");
		response(cl, operserv->nick, "Las noticias son acumulativas si no se borran.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Comandos disponibles:");
		response(cl, operserv->nick, "\00312ADD\003 Añade una noticia.");
		response(cl, operserv->nick, "\00312DEL\003 Bora una noticia.");
		response(cl, operserv->nick, "\00312LIST\003 Lista las noticias actuales.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis:");
		response(cl, operserv->nick, "\00312ADD bot noticia");
		response(cl, operserv->nick, "\00312DEL nº");
		response(cl, operserv->nick, "\00312LIST");
	}
	else if (!strcasecmp(param[1], "OPERS") && IsAdmin(cl))
	{
		response(cl, operserv->nick, "\00312OPERS");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Administra los operadores de la red.");
		response(cl, operserv->nick, "Cuando un usuario es identificador como propietario de su nick, y figura como operador, se le da el modo +h.");
		response(cl, operserv->nick, "Así queda reconocido como operador de red.");
		response(cl, operserv->nick, "Rangos:");
		response(cl, operserv->nick, "\00312PREO");
		response(cl, operserv->nick, "\00312OPER (+h)");
		response(cl, operserv->nick, "\00312DEVEL");
		response(cl, operserv->nick, "\00312ADMIN (+N)");
		response(cl, operserv->nick, "\00312ROOT");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312OPERS {+|-}usuario rango");
	}
	else if (!strcasecmp(param[1], "CLONES") && IsAdmin(cl))
	{
		response(cl, operserv->nick, "\00312CLONES");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Gestiona la lista de ips con más clones.");
		response(cl, operserv->nick, "Estas ips o hosts tienen un número concreto de clones permitidos.");
		response(cl, operserv->nick, "Si se sobrepasa el límite, los usuarios son desconectados.");
		response(cl, operserv->nick, "Es importante distinguir entre ip y host. Si el usuario resuelve a host, deberá introducirse el host.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312CLONES +{ip|host} número");
		response(cl, operserv->nick, "Añade una ip|host con un número propio de clones.");
		response(cl, operserv->nick, "Sintaxis: \00312CLONES -{ip|host}");
		response(cl, operserv->nick, "Borra una ip|host.");
	}
	else if (!strcasecmp(param[1], "RESTART") && IsRoot(cl))
	{
		response(cl, operserv->nick, "\00312RESTART");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Reinicia los servicios.");
		response(cl, operserv->nick, "Cierra y vuelve a abrir el programa.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312RESTART");
	}
	else if (!strcasecmp(param[1], "REHASH") && IsRoot(cl))
	{
		response(cl, operserv->nick, "\00312REHASH");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Refresca los servicios.");
		response(cl, operserv->nick, "Este comando resulta útil cuando se han modificado los archivos de configuración o hay alguna desincronización.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312REHASH");
	}
	else if (!strcasecmp(param[1], "BACKUP") && IsRoot(cl))
	{
		response(cl, operserv->nick, "\00312BACKUP");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Crea una copia de seguridad de la base de datos y se guarda en el archivo backup.sql");
		response(cl, operserv->nick, "Cada vez que se inicie el programa y detecte este archivo pedirá su restauración.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312BACKUPP");
	}
	else if (!strcasecmp(param[1], "RESTAURAR") && IsRoot(cl))
	{
		response(cl, operserv->nick, "\00312RESTAURAR");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Restaura una copia de base de datos realizada con el comando BACKUP");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312RESTAURAR");
	}
	else if (!strcasecmp(param[1], "CLOSE") && IsRoot(cl))
	{
		response(cl, operserv->nick, "\00312CLOSE");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Cierra el programa.");
		response(cl, operserv->nick, "Una vez cerrado no se podrá volver ejecutar si no es manualmente.");
		response(cl, operserv->nick, " ");
		response(cl, operserv->nick, "Sintaxis: \00312CLOSE");
	}
	else
		response(cl, operserv->nick, OS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(operserv_raw)
{
	char *raw;
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "RAW mensaje");
		return 1;
	}
	raw = implode(param, params, 1, -1);
	sendto_serv(":%s", raw);
	response(cl, operserv->nick, "Comando ejecutado \00312%s\003.", raw);
	return 0;
}
BOTFUNC(operserv_restart)
{
	response(cl, operserv->nick, "Los servicios van a reiniciarse.");
	timer("reinicia", NULL, 1, 1, reinicia, NULL, 0);
	return 0;
}
BOTFUNC(operserv_rehash)
{
	refresca();
	return 0;
}
BOTFUNC(operserv_backup)
{
	if ( _mysql_backup())
		response(cl, operserv->nick, OS_ERR_EMPT, "Ha ocurrido un error al intentar hacer una copia de seguridad de la base de datos.");
	else
		response(cl, operserv->nick, "Base de datos copiada con éxito.");
	return 0;
}
BOTFUNC(operserv_restaura)
{
	if (_mysql_restaura())
		response(cl, operserv->nick, OS_ERR_EMPT, "Ha ocurrido un error al intentar restaurar la base de datos.");
	else
		response(cl, operserv->nick, "Base de datos restaurada con éxito.");
	return 0;
}
BOTFUNC(operserv_blocks)
{
	if (params < 3)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "BLOCKS {+|-}|Q|Z|s|G nick|user@host [parámetros]");
		return 1;
	}
	switch(*(param[1] + 1))
	{
		case TKL_QLINE:
			if (*param[1] == '+')
			{
				buf[0] = '\0';
				sprintf_irc(buf, operserv->bline, implode(param, params, 3, -1));
#ifdef UDB
				envia_registro_bdd("N::%s::forbid %s", param[2], buf);
#else
				_mysql_add(NS_FORBIDS, param[2], "motivo", buf);
				sendto_serv(":%s %s %s :%s", me.nombre, TOK_SQLINE, param[2], buf);
#endif
			}
			else if (*param[1] == '-')
			{
#ifdef UDB
				envia_registro_bdd("N::%s::forbid", param[2]);
#else
				_mysql_del(NS_FORBIDS, param[2]);
				sendto_serv(":%s %s %s", me.nombre, TOK_UNSQLINE, param[2]);
#endif
			}
			else
			{
				response(cl, operserv->nick, OS_ERR_SNTX, "BLOCKS {+|-}|Q|Z|s|G nick|user@host [parámetros]");
				return 1;
			}
			break;
		case TKL_SHUN:
		case TKL_ZAP:
		case TKL_GLINE:
		{
			char *user, *host;
			user = host = NULL;
			if (!strchr(param[2], '@'))
			{
				Cliente *al;
				if ((al = busca_cliente(param[2], NULL)))
				{
					user = al->ident;
					host = al->host;
				}
				else
				{
					response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no está conectado.");
					return 1;
				}
			}
			else
			{
				user = strtok(param[2], "@");
				host = strtok(NULL, "@");
			}
			if (*param[1] == '+')
			{
				if (params < 5)
				{
					response(cl, operserv->nick, OS_ERR_PARA, "BLOCKS {+|-}|Q|Z|s|G nick|user@host tiempo motivo");
					return 1;
				}
				irctkl(TKL_ADD, *(param[1] + 1), user, host, operserv->mascara, atoi(param[3]), implode(param, params, 4, -1));
			}
			else if (*param[1] == '-')
				irctkl(TKL_DEL, *(param[1] + 1), user, host, operserv->mascara, 0, NULL);
			else
			{
			 	response(cl, operserv->nick, OS_ERR_SNTX, "BLOCKS {+|-}|Q|Z|s|G nick|user@host [parámetros]");
				return 1;
			}
			break;
		}
		default:
			response(cl, operserv->nick, OS_ERR_SNTX, "BLOCKS {+|-}|Q|Z|s|G nick|user@host [parámetros]");
			return 1;
	}
	response(cl, operserv->nick, "Block \00312%c\003 %s con éxito.", *(param[1] + 1), *param[1] == '+' ? "añadida" : "eliminada");
	return 0;
}
BOTFUNC(operserv_opers)
{
	char oper[128];
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "OPERS {+|-}nick [rango]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, operserv->nick, OS_ERR_PARA, "OPERS {+|-}nick rango");
			return 1;
		}
		if (!IsReg(param[1] + 1))
		{
			response(cl, operserv->nick, OS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		if (!strcasecmp(param[2], "OPER"))
			sprintf_irc(oper, "%i", BDD_OPER);
		else if (!strcasecmp(param[2], "DEVEL"))
			sprintf_irc(oper, "%i", BDD_DEVEL);
		else if (!strcasecmp(param[2], "ADMIN"))
		{
			if (!IsRoot(cl))
			{
				response(cl, operserv->nick, OS_ERR_FORB, "");
				return 1;
			}
			sprintf_irc(oper, "%i", BDD_ADMIN);
		}
		else if (!strcasecmp(param[2], "ROOT"))
		{
			if (strcasecmp(conf_set->root, cl->nombre))
			{
				response(cl, operserv->nick, OS_ERR_FORB, "");
				return 1;
			}
			sprintf_irc(oper, "%i", BDD_ROOT);
		}
		else if (!strcasecmp(param[2], "PREO"))
			sprintf_irc(oper, "%i", BDD_PREO);
		else
		{
			response(cl, operserv->nick, OS_ERR_SNTX, "OPERS +nick OPER|DEVEL|ADMIN|ROOT|PREO");
			return 1;
		}
#ifdef UDB
		envia_registro_bdd("N::%s::oper %c%s", param[1] + 1, CHAR_NUM, oper);
#else
		_mysql_add(OS_MYSQL, param[1] + 1, "nivel", oper);
#endif		
		response(cl, operserv->nick, "El usuario \00312%s\003 ha sido añadido como \00312%s\003.", param[1] + 1, param[2]);
	}
	else if (*param[1] == '-')
	{
#ifdef UDB
		envia_registro_bdd("N::%s::oper", param[1] + 1);
#else
		if (!_mysql_get_registro(OS_MYSQL, param[1] + 1, "nivel"))
		{
			response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no es oper.");
			return 1;
		}
		_mysql_del(OS_MYSQL, param[1]);
#endif
		response(cl, operserv->nick, "El usuario \00312%s\003 ha sido borrado.", param[1] + 1);
	}
	else
	{
		response(cl, operserv->nick, OS_ERR_SNTX, "OPERS {+|-}nick [rango]");
		return 1;
	}
	return 0;
}
BOTFUNC(operserv_sajoin)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "SAJOIN nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	sendto_serv_us(&me, MSG_SVSJOIN, TOK_SVSJOIN, "%s %s", param[1], param[2]);
	response(cl, operserv->nick, "El usuario \00312%s\003 ha sido forzado a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_sapart)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "SAPART nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	sendto_serv_us(&me, MSG_SVSPART, TOK_SVSPART, "%s %s", param[1], param[2]);
	response(cl, operserv->nick, "El usuario \00312%s\003 ha sido forzado a salir de \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_rejoin)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "REJOIN nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	sendto_serv_us(&me, MSG_SVSPART, TOK_SVSPART, "%s %s", param[1], param[2]);
	sendto_serv_us(&me, MSG_SVSJOIN, TOK_SVSJOIN, "%s %s", param[1], param[2]);
	response(cl, operserv->nick, "El usuario \00312%s\003 ha sido forzado a salir y a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_kill)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "KILL nick motivo");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	irckill(al, operserv->nick, implode(param, params, 2, -1));
	response(cl, operserv->nick, "El usuario \00312%s\003 ha sido desconectado.", param[1]);
	return 0;
}
BOTFUNC(operserv_global)
{
	int opts = 0;
	char *msg, *t = TOK_PRIVATE;
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "GLOBAL [-modos] mensaje");
		return 1;
	}
	if (*param[1] == '-')
	{
		if (strchr(param[1], 'o'))
			opts |= 0x1;
		if (strchr(param[1], 'p'))
			opts |= 0x2;
		if (strchr(param[1], 'b'))
			opts |= 0x4;
		if (strchr(param[1], 'm'))
			opts |= 0x8;
		if (strchr(param[1], 'n'))
			t = TOK_NOTICE;
	}
	if (opts & 0x8)
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		if ((res = _mysql_query("SELECT item from %s%s", PREFIJO, NS_MYSQL)))
		{
			int level;
			while ((row = mysql_fetch_row(res)))
			{
#ifdef UDB
				level = level_oper_bdd(row[0]);
#else
				level = atoi(_mysql_get_registro(OS_MYSQL, row[0], "nivel"));
#endif
				if ((opts & 0x1) && !(level & BDD_OPER))
					continue;
				if ((opts & 0x2) && !(level & BDD_PREO))
					continue;
				if (opts & 0x4)
					memoserv_send_dl(row[0], param[2], implode(param, params, 3, -1));
				else
					memoserv_send_dl(row[0], operserv->nick, implode(param, params, 2, -1));
			}
			mysql_free_result(res);
		}
		response(cl, operserv->nick, "Mensaje global enviado vía memo.");
	}
	else
	{
		char *bot;
		Cliente *al;
		if (opts & 0x4)
		{
			sendto_serv("%s %s 1 %lu %s %s %s 0 +%s %s :%s", TOK_NICK, param[2], time(0), operserv->ident, operserv->host, me.nombre, operserv->modos, operserv->host, operserv->realname);
			msg = implode(param, params, 3, -1);
			bot = param[2];
		}
		else
		{
			if (*param[1] == '-')
				msg = implode(param, params, 2, -1);
			else
				msg = implode(param, params, 1, -1);
			bot = operserv->nick;
		}
		for (al = clientes; al; al = al->sig)
		{
			if ((opts & 0x1) && !IsOper(al))
				continue;
			if ((opts & 0x2) && !IsPreo(al))
				continue;
			if (EsBot(al) || EsServer(al))
				continue;
			sendto_serv(":%s %s %s :%s", bot, t, al->nombre, msg);
		}
		if (opts & 0x4)
			sendto_serv(":%s %s :Mensajería global %s", bot, TOK_QUIT, conf_set->red);
		response(cl, operserv->nick, "Mensaje global enviado.");
	}
	return 0;
}
BOTFUNC(operserv_noticias)
{
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	if (!strcasecmp(param[1], "ADD"))
	{
		if (params < 4)
		{
			response(cl, operserv->nick, OS_ERR_PARA, "NOTICIAS ADD botname noticia");
			return 1;
		}
		inserta_noticia(param[2], implode(param, params, 3, -1), 0, 0);
		response(cl, operserv->nick, "Noticia añadida.");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		if (params < 3)
		{
			response(cl, operserv->nick, OS_ERR_PARA, "NOTICIAS DEL nº");
			return 1;
		}
		borra_noticia(atoi(param[2]));
		response(cl, operserv->nick, "Noticia borrada.");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		time_t ti;
		if (!(res = _mysql_query("SELECT n,bot,noticia,fecha from %s%s", PREFIJO, OS_NOTICIAS)))
		{
			response(cl, operserv->nick, OS_ERR_EMPT, "No hay noticias.");
			return 1;
		}
		response(cl, operserv->nick, "Noticias de la red");
		while ((row = mysql_fetch_row(res)))
		{
			if (strlen(row[2]) > 35)
			{
				row[2][30] = row[2][31] = row[2][32] = '.';
				row[2][33] = '\0';
			}
			buf[0] = '\0';
			ti = atol(row[3]);
			strftime(buf, BUFSIZE, "%d/%m/%Y", localtime(&ti));
			response(cl, operserv->nick, "\00312%s\003 - %s - \00312%s\003 por \00312%s\003.", row[0], buf, row[2], row[1]);
		}
		mysql_free_result(res);
	}
	else
	{
		response(cl, operserv->nick, OS_ERR_SNTX, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	return 0;
}
BOTFUNC(operserv_clones)
{
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, operserv->nick, OS_ERR_PARA, "CLONES +{ip|host} nº");
			return 1;
		}
		if (atoi(param[2]) < 1)
		{
			response(cl, operserv->nick, OS_ERR_SNTX, "CLONES +{ip|host} número");
			return 1;
		}
		param[1]++;
#ifdef UDB
		envia_registro_bdd("I::%s %c%s", param[1], CHAR_NUM, param[2]);
#else
		_mysql_add(OS_CLONS, param[1], "clones", param[2]);
#endif
		response(cl, operserv->nick, "La ip/host \00312%s\003 se ha añadido con \00312%s\003 clones.", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
#ifdef UDB
		envia_registro_bdd("I::%s", param[1]);
#else
		_mysql_del(OS_CLONS, param[1]);
#endif
		response(cl, operserv->nick, "La ip/host \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		response(cl, operserv->nick, OS_ERR_SNTX, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	return 0;
}		
BOTFUNC(operserv_close)
{
	response(cl, operserv->nick, "Los servicios van a cerrarse.");
	cierra_colossus(0);
	return 0;
}
#ifdef UDB
BOTFUNC(operserv_modos)
{
	if (params < 2)
	{
		response(cl, operserv->nick, OS_ERR_PARA, "MODOS nick [modos]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		response(cl, operserv->nick, OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("ohaAOkNCWqHX", *modos))
			{
				sprintf_irc(buf, "El modo %c está prohibido. Sólo se permiten los modos ohaAOkNCWqHX.", *modos);
				response(cl, operserv->nick, OS_ERR_EMPT, buf);
				return 1;
			}
		}
		envia_registro_bdd("N::%s::modos %s", param[1], param[2]);
		response(cl, operserv->nick, OS_ERR_EMPT, "Los modos de operador para \00312%s\003 se han puesto a \00312", param[1], param[2]);
	}
	else
	{
		envia_registro_bdd("N::%s::modos", param[1]);
		response(cl, operserv->nick, OS_ERR_EMPT, "Se han eliminado los modos de operador para \00312%s", param[1]);
	}
	return 0;
}
#endif
IRCFUNC(operserv_nick)
{
#ifndef UDB
	Cliente *aux;
	int i = 0, clons = operserv->clones;
	char *cc;
	for (aux = clientes; aux; aux = aux->sig)
	{
		if (EsServer(aux) || EsBot(aux))
			continue;
		if (!strcmp(cl->host, aux->host))
			i++;
	}
	if ((cc = _mysql_get_registro(OS_CLONS, cl->host, "clones")))
		clons = atoi(cc);
	if (i > clons)
	{
		irckill(cl, operserv->nick, "Demasiados clones.");
		return 0;
	}
#endif
	if (gnoticias && parc > 4)
	{
		int i;
		time_t ti;
		response(cl, gnoticia[0]->botname, "Noticias de la Red:");
		for (i = 0; i < gnoticias; i++)
		{
			ti = gnoticia[i]->fecha;
			strftime(buf, BUFSIZE, "%d/%m/%Y", localtime(&ti));
			response(cl, gnoticia[i]->botname, "%s - %s", buf, gnoticia[i]->noticia);
		}
	}
	return 0;
}
IRCFUNC(operserv_join)
{
	char *canal;
	if (!IsOper(cl) || !(operserv->opts & OS_OPTS_AOP))
		return 0;
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
	{
		if (!IsChanReg_dl(canal))
			envia_cmodos(busca_canal(canal, NULL), operserv->nick, "+o %s", cl->nombre);
	}
	return 0;
}

int operserv_idok(Cliente *cl)
{
#ifdef UDB
	int nivel;
	if ((nivel = level_oper_bdd(cl->nombre)))
	{
		if (nivel & BDD_PREO)
			cl->nivel |= PREO;
		if (nivel & BDD_OPER)
			cl->nivel |= OPER;
		if (nivel & BDD_DEVEL)
			cl->nivel |= DEVEL;
		if (nivel & BDD_ADMIN)
			cl->nivel |= ADMIN;
		if (nivel & BDD_ROOT)
			cl->nivel |= ROOT;
	}
#else
	int opts = 0;
	char *modos;
	if ((modos = _mysql_get_registro(OS_MYSQL, cl->nombre, "nivel")))
	{
		opts = atoi(modos);
		if (opts & BDD_ADMIN)
			envia_umodos(cl, operserv->nick, "+ohN");
		else
			envia_umodos(cl, operserv->nick, "+h");
		if (opts & BDD_PREO)
			cl->nivel |= PREO;
		if (opts & BDD_OPER)
			cl->nivel |= OPER;
		if (opts & BDD_DEVEL)
			cl->nivel |= DEVEL;
		if (opts & BDD_ADMIN)
			cl->nivel |= ADMIN;
		if (opts & BDD_ROOT)
			cl->nivel |= ROOT;
	}
#endif
	return 0;
}
int operserv_sig_mysql()
{
	int i;
#ifdef UDB
	char buf[1][BUFSIZE], tabla[1];
#else
	char buf[3][BUFSIZE], tabla[3];
#endif
	tabla[0] = 0;
#ifndef UDB
	tabla[1] = tabla[2] = 0;
#endif
	sprintf_irc(buf[0], "%s%s", PREFIJO, OS_NOTICIAS);
#ifndef UDB
	sprintf_irc(buf[1], "%s%s", PREFIJO, OS_MYSQL);
	sprintf_irc(buf[2], "%s%s", PREFIJO, OS_CLONS);
#endif
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
#ifndef UDB			
		else if (!strcasecmp(mysql_tablas.tabla[i], buf[1]))
			tabla[1] = 1;
		else if (!strcasecmp(mysql_tablas.tabla[i], buf[2]))
			tabla[2] = 1;
#endif
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`bot` text NOT NULL, "
  			"`noticia` text NOT NULL, "
  			"`fecha` bigint(20) NOT NULL default '0', "
  			"KEY `n` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de noticias';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
#ifndef UDB
	if (!tabla[1])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`nivel` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de operadores';", buf[1]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[1]);
	}
	if (!tabla[2])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`clones` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de clones';", buf[2]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[2]);
	}
#endif
	_mysql_carga_tablas();
	operserv_carga_noticias();
	return 1;
}
int operserv_sig_eos()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if ((res = _mysql_query("SELECT DISTINCT bot from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = mysql_fetch_row(res)))
			botnick(row[0], operserv->ident, operserv->host, me.nombre, "+qkBd", "Servicio de noticias");
	}
	return 0;
}
