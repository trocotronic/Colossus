/*
 * $Id: operserv.c,v 1.19 2005-06-29 21:14:03 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "modulos/operserv.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#include "modulos/memoserv.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

OperServ operserv;
#define ExFunc(x) BuscaFuncion(operserv.hmod, x, NULL)
Noticia *gnoticia[MAXNOT];
int gnoticias = 0;
char *confirm = NULL;

BOTFUNC(OSHelp);
BOTFUNC(OSRaw);
BOTFUNC(OSRestart);
BOTFUNC(OSRehash);
BOTFUNC(OSBackup);
BOTFUNC(OSRestaura);
BOTFUNC(OSGline);
BOTFUNC(OSOpers);
BOTFUNC(OSSajoin);
BOTFUNC(OSSapart);
BOTFUNC(OSRejoin);
BOTFUNC(OSKill);
BOTFUNC(OSGlobal);
BOTFUNC(OSNoticias);
BOTFUNC(OSClose);
BOTFUNC(OSVaciar);
#ifdef UDB
BOTFUNC(OSModos);
BOTFUNC(OSSnomask);
#endif
BOTFUNC(OSAkill);

static bCom operserv_coms[] = {
	{ "help" , OSHelp , OPERS } ,
	{ "raw" , OSRaw , OPERS } ,
	{ "restart" , OSRestart , ROOTS } ,
	{ "rehash" , OSRehash , ROOTS } ,
	{ "backup" , OSBackup , ROOTS } ,
	{ "close" , OSClose , ROOTS } ,
	{ "restaura" , OSRestaura , ROOTS } ,
	{ "gline" , OSGline , OPERS } ,
	{ "opers" , OSOpers , ADMINS } ,
	{ "sajoin" , OSSajoin , OPERS } ,
	{ "sapart" , OSSapart , OPERS } ,
	{ "rejoin" , OSRejoin , OPERS } ,
	{ "kill" , OSKill , OPERS } ,
	{ "global" , OSGlobal , OPERS } ,
	{ "noticias" , OSNoticias , OPERS } ,
	{ "vaciar" , OSVaciar , ROOTS } ,
#ifdef UDB
	{ "modos" , OSModos , ADMINS } ,
	{ "snomask" , OSSnomask , ADMINS } ,
#endif
	{ "akill" , OSAkill , OPERS } ,
	{ 0x0 , 0x0 , 0x0 }
};

#ifndef UDB
int OSSigIdOk	(Cliente *);
#endif
int OSSigMySQL	();
int OSSigSynch	();
void OSCargaNoticias();
void OSDescargaNoticias();

int OSCmdJoin(Cliente *, Canal *);
int OSCmdNick(Cliente *, int);
#ifndef _WIN32
int (*ChanReg_dl)(char *);
int (*memoserv_send_dl)(char *, char *, char *);
#else
#define ChanReg_dl ChanReg
#define memoserv_send_dl MSSend
#endif
#define IsChanReg_dl(x) (ChanReg_dl(x) == 1)
#define IsChanPReg_dl(x) (ChanReg_dl(x) == 2)
void OSSet(Conf *, Modulo *);
int OSTest(Conf *, int *);

ModInfo info = {
	"OperServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, info.nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, info.nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
			{
				if (!OSTest(modulo.seccion[0], &errores))
					OSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el OSTest", mod->archivo, info.nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
				irc_dlsym(ex->hmod, "ChanReg", ChanReg_dl);
			else if (!strcasecmp(ex->info->nombre, "MemoServ"))
				irc_dlsym(ex->hmod, "MSSend", memoserv_send_dl);
		}
	}
#endif	
	if (mysql)
		OSCargaNoticias();
	return errores;
}
int descarga()
{
	BorraSenyal(SIGN_JOIN, OSCmdJoin);
	BorraSenyal(SIGN_POST_NICK, OSCmdNick);
#ifndef UDB
	BorraSenyal(NS_SIGN_IDOK, OSSigIdOk);
#endif
	BorraSenyal(SIGN_MYSQL, OSSigMySQL);
	BorraSenyal(SIGN_SYNCH, OSSigSynch);
	OSDescargaNoticias();
	BotUnset(operserv);
	return 0;
}	
int OSTest(Conf *config, int *errores)
{
	return 0;
}
void OSSet(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *os;
	operserv.maxlist = 30;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				os = &operserv_coms[0];
				while (os->com != 0x0)
				{
					if (!strcasecmp(os->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = os;
						break;
					}
					os++;
				}
				if (os->com == 0x0)
					Error("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
		if (!strcmp(config->seccion[i]->item, "autoop"))
			operserv.opts |= OS_OPTS_AOP;
		if (!strcmp(config->seccion[i]->item, "maxlist"))
			operserv.maxlist = atoi(config->seccion[i]->data);
	}
	InsertaSenyal(SIGN_JOIN, OSCmdJoin);
	InsertaSenyal(SIGN_POST_NICK, OSCmdNick);
#ifndef UDB
	InsertaSenyal(NS_SIGN_IDOK, OSSigIdOk);
#endif
	InsertaSenyal(SIGN_MYSQL, OSSigMySQL);
	InsertaSenyal(SIGN_SYNCH, OSSigSynch);
	BotSet(operserv);
}
Noticia *OSEsBotNoticia(char *botname)
{
	int i;
	for (i = 0; i < gnoticias; i++)
	{
		if (!strcasecmp(gnoticia[i]->botname, botname))
			return gnoticia[i];
	}
	return NULL;
}
void OSCargaNoticia(int id)
{
	int i;
	Noticia *not, *gn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	time_t aux;
	for (i = 0; i < gnoticias; i++)
	{
		if (gnoticia[i]->id == id)
		{
			not = gnoticia[i];
			goto sigue;
		}
	}
	BMalloc(not, Noticia);
	if ((res = MySQLQuery("SELECT bot,noticia,fecha FROM %s%s WHERE n=%i", PREFIJO, OS_NOTICIAS, id)))
	{
		row = mysql_fetch_row(res);
		not->botname = strdup(row[0]);
		not->noticia = strdup(row[1]);
		aux = atoul(row[2]);
		strftime(not->fecha, sizeof(not->fecha), "%d/%m/%Y", localtime(&aux));
		not->id = id;
		gnoticia[gnoticias++] = not;
	}
	sigue:
	if (!(gn = OSEsBotNoticia(not->botname)))
		not->cl = CreaBot(not->botname, operserv.hmod->ident, operserv.hmod->host, me.nombre, "kdBq", operserv.hmod->realname);
	else
		not->cl = gn->cl;
}
int OSDescargaNoticia(int id)
{
	int i;
	Noticia *not = NULL;
	for (i = 0; i < gnoticias; i++)
	{
		if (gnoticia[i]->id == id)
		{
			not = gnoticia[i];
			i++;
			while (i < gnoticias)
				gnoticia[i] = gnoticia[i+1];
			gnoticias--;
			break;
		}
	}
	if (!not)
		return 0;
	if (!OSEsBotNoticia(not->botname))
	{
		Cliente *bl = BuscaCliente(not->botname, NULL);
		if (bl)
			port_func(P_QUIT_USUARIO_LOCAL)(bl, "Mensajería global %s", conf_set->red);
	}
	ircfree(not->botname);
	ircfree(not->noticia);
	Free(not);
	return 1;
}
void OSDescargaNoticias()
{
	while (gnoticias)
		OSDescargaNoticia(gnoticia[0]->id);
}
int OSInsertaNoticia(char *botname, char *noticia, time_t fecha, int id)
{
	Noticia *not;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Cliente *cl = NULL;
	Noticia *gn = NULL;
	time_t aux;
	if (gnoticias == MAXNOT)
		return 0;
	if (!id && !(gn = OSEsBotNoticia(botname)))
		cl = CreaBot(botname, operserv.hmod->ident, operserv.hmod->host, me.nombre, "kdBq", operserv.hmod->realname);
	if (gn)
		cl = gn->cl;
	not = (Noticia *)Malloc(sizeof(Noticia));
	not->botname = strdup(botname);
	not->noticia = strdup(noticia);
	aux = fecha ? fecha : time(0);
	strftime(not->fecha, sizeof(not->fecha), "%d/%m/%Y", localtime(&aux));
	not->cl = cl ? cl : NULL;
	if (!id)
	{
		MySQLQuery("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, botname, noticia, aux);
		res = MySQLQuery("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, noticia, botname);
		row = mysql_fetch_row(res);
		not->id = atoi(row[0]);
		mysql_free_result(res);
	}
	else
		not->id = id;
	gnoticia[gnoticias++] = not;
	return 1;
}
int OSBorraNoticia(int id)
{
	MySQLQuery("DELETE from %s%s where n='%i'", PREFIJO, OS_NOTICIAS, id);
	return OSDescargaNoticia(id);
}
void OSCargaNoticias()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if ((res = MySQLQuery("SELECT * from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = mysql_fetch_row(res)))
			OSCargaNoticia(atoi(row[0]));
		mysql_free_result(res);
	}
}
int OSReinicia()
{
	Reinicia();
	return 0;
}
BOTFUNC(OSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), "\00312%s\003 se encarga de gestionar la red de forma global y reservada al personal cualificado para realizar esta tarea.", operserv.hmod->nick);
		Responde(cl, CLI(operserv), "Esta gestión permite tener un control exhaustivo sobre las actividades de la red para su buen funcionamiento.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Comandos disponibles:");
		FuncResp(operserv, "RAW", "Manda un raw al servidor.");
		FuncResp(operserv, "GLINE", "Gestiona las glines de la red.");
		FuncResp(operserv, "SAJOIN", "Ejecuta SAJOIN sobre un usuario.");
		FuncResp(operserv, "SAPART", "Ejecuta SAPART sobre un usuario.");
		FuncResp(operserv, "REJOIN", "Obliga a un usuario a reentrar a un canal.");
		FuncResp(operserv, "KILL", "Desconecta a un usuario.");
		FuncResp(operserv, "GLOBAL", "Manda un mensaje global.");
		FuncResp(operserv, "NOTICIAS", "Administra las noticias de la red.");
		FuncResp(operserv, "AKILL", "Prohibe la conexión de una ip/host o usuario permanentemente.");
		if (IsAdmin(cl))
		{
			FuncResp(operserv, "OPERS", "Administra los operadores de la red.");
#ifdef UDB
			FuncResp(operserv, "MODOS", "Fija los modos de operador que se obtiene automáticamente (BDD).");
			FuncResp(operserv, "SNOMASK", "Fija las máscaras de operador que se obtiene automáticamente (BDD).");
#endif
		}
		if (IsRoot(cl))
		{
			FuncResp(operserv, "RESTART", "Reinicia los servicios.");
			FuncResp(operserv, "REHASH", "Refresca los servicios.");
			FuncResp(operserv, "BACKUP", "Crea una copia de seguridad de la base de datos.");
			FuncResp(operserv, "RESTAURA", "Restaura la base de datos.");
			FuncResp(operserv, "CLOSE", "Cierra el programa.");
			FuncResp(operserv, "VACIAR", "Elimina todos los registros de la base de datos.");
		}
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Para más información, \00312/msg %s %s comando", operserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "RAW") && ExFunc("RAW"))
	{
		Responde(cl, CLI(operserv), "\00312RAW");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Manda un raw al servidor.");
		Responde(cl, CLI(operserv), "Este comando actúa directamente al servidor.");
		Responde(cl, CLI(operserv), "No es procesado por los servicios, con lo que no tiene un registro sobre él.");
		Responde(cl, CLI(operserv), "Sólo debe usarse para fines administrativos cuya justificación sea de peso.");
		Responde(cl, CLI(operserv), "No usárase si no se tuviera conocimiento explícito de los raw's del servidor.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312RAW linea");
		Responde(cl, CLI(operserv), "Ejemplo: \00312RAW %s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
		Responde(cl, CLI(operserv), "Ejecutaría el comando \00312:%s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
	}
	else if (!strcasecmp(param[1], "GLINE") && ExFunc("GLINE"))
	{
		Responde(cl, CLI(operserv), "\00312GLINES");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Administra los bloqueos de la red.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312GLINE {+|-}{nick|user@host} [tiempo motivo]");
		Responde(cl, CLI(operserv), "Para añadir una gline se antepone '+' antes de la gline. Para quitarla, '-'.");
		Responde(cl, CLI(operserv), "Se puede especificar un nick o un user@host indistintamente. Si se especifica un nick, se usará su user@host sin necesidad de buscarlo previamente.");
	}
	else if (!strcasecmp(param[1], "SAJOIN") && ExFunc("SAJOIN"))
	{
		Responde(cl, CLI(operserv), "\00312SAJOIN");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Fuerza a un usuario a entrar en un canal.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312SAJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "SAPART") && ExFunc("SAPART"))
	{
		Responde(cl, CLI(operserv), "\00312SAPART");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Fuerza a un usuario a salir de un canal.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312SAPART usuario #canal");
	}
	else if (!strcasecmp(param[1], "REJOIN") && ExFunc("REJOIN"))
	{
		Responde(cl, CLI(operserv), "\00312REJOIN");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Fuerza a un usuario a reentrar en un canal.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312REJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "KILL") && ExFunc("KILL"))
	{
		Responde(cl, CLI(operserv), "\00312KILL");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Desconecta a un usuario de la red.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312KILL usuario [motivo]");
	}
	else if (!strcasecmp(param[1], "GLOBAL") && ExFunc("GLOBAL"))
	{
		Responde(cl, CLI(operserv), "\00312GLOBAL");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Envía un mensaje global.");
		Responde(cl, CLI(operserv), "Dispone de varios parámetros para especificar sus destinatarios y el modo de envío.");
		Responde(cl, CLI(operserv), "Estos son:");
		Responde(cl, CLI(operserv), "\00312o\003 Exclusivo a operadores.");
		Responde(cl, CLI(operserv), "\00312p\003 Para preopers.");
		Responde(cl, CLI(operserv), "\00312b\003 Utiliza un bot para mandar el mensaje.");
		Responde(cl, CLI(operserv), "\00312m\003 Vía memo.");
		Responde(cl, CLI(operserv), "\00312n\003 Vía notice.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312GLOBAL [-parámetros [[bot]] mensaje");
		Responde(cl, CLI(operserv), "Si no se especifican modos, se envía a todos los usuarios vía privmsg.");
	}
	else if (!strcasecmp(param[1], "NOTICIAS") && ExFunc("NOTICIAS"))
	{
		Responde(cl, CLI(operserv), "\00312NOTICIAS");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Gestiona las noticias de la red.");
		Responde(cl, CLI(operserv), "Estas noticias son mostradas cuando se conecta un usuario en forma de privado.");
		Responde(cl, CLI(operserv), "Se muestra la fecha y el cuerpo de la noticia y son mandadas por el bot especificado.");
		Responde(cl, CLI(operserv), "Las noticias son acumulativas si no se borran.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Comandos disponibles:");
		Responde(cl, CLI(operserv), "\00312ADD\003 Añade una noticia.");
		Responde(cl, CLI(operserv), "\00312DEL\003 Bora una noticia.");
		Responde(cl, CLI(operserv), "\00312LIST\003 Lista las noticias actuales.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis:");
		Responde(cl, CLI(operserv), "\00312ADD bot noticia");
		Responde(cl, CLI(operserv), "\00312DEL nº");
		Responde(cl, CLI(operserv), "\00312LIST");
	}
	else if (!strcasecmp(param[1], "AKILL") && ExFunc("AKILL"))
	{
		Responde(cl, CLI(operserv), "\00312AKILL");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Prohibe la conexión de una ip/host o un usuario de forma permanente.");
		Responde(cl, CLI(operserv), "La principal ventaja es que estas prohibiciones nunca se pierden, aunque se reinicie el servidor.");
		Responde(cl, CLI(operserv), "Su entrada siempre será prohibida, aunque los servicios no estén conectados.");
		Responde(cl, CLI(operserv), "Si no se especifica ninguna acción (+ o -) se listarán los host que coincidan con el patrón.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis:");
		Responde(cl, CLI(operserv), "AKILL {+|-}[user@]{host|ip} [motivo]");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Ejemplos:");
		Responde(cl, CLI(operserv), "AKILL +pepito@palotes.com largo");
		Responde(cl, CLI(operserv), "AKILL +127.0.0.1 largo");
		Responde(cl, CLI(operserv), "AKILL -google.com");
		Responde(cl, CLI(operserv), "AKILL *@*.aol.com");
	}
	else if (!strcasecmp(param[1], "OPERS") && IsAdmin(cl) && ExFunc("OPERS"))
	{
		Responde(cl, CLI(operserv), "\00312OPERS");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Administra los operadores de la red.");
		Responde(cl, CLI(operserv), "Cuando un usuario es identificador como propietario de su nick, y figura como operador, se le da el modo +h.");
		Responde(cl, CLI(operserv), "Así queda reconocido como operador de red.");
		Responde(cl, CLI(operserv), "Rangos:");
		Responde(cl, CLI(operserv), "\00312PREO");
		Responde(cl, CLI(operserv), "\00312OPER (+h)");
		Responde(cl, CLI(operserv), "\00312DEVEL");
		Responde(cl, CLI(operserv), "\00312ADMIN (+N)");
		Responde(cl, CLI(operserv), "\00312ROOT");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312OPERS {+|-}usuario rango");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MODOS") && IsAdmin(cl) && ExFunc("MODOS"))
	{
		Responde(cl, CLI(operserv), "\00312MODOS");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Fija los modos de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
		Responde(cl, CLI(operserv), "Estos modos son: \00312ohAOkNCWqHX");
		Responde(cl, CLI(operserv), "Serán otorgados de forma automática cada vez que el operador use su nick vía /nick nick:pass");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312MODOS operador [modos]");
		Responde(cl, CLI(operserv), "Si no se especifican modos, se borrarán los que pueda haber.");
	}
	else if (!strcasecmp(param[1], "SNOMASK") && IsAdmin(cl) && ExFunc("SNOMASK"))
	{
		Responde(cl, CLI(operserv), "\00312SNOMASK");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Fija las máscaras de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
		Responde(cl, CLI(operserv), "Serán otorgadas de forma automática cada vez que el operador use su nick vía /nick nick:pass");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312SNOMASK operador [snomasks]");
		Responde(cl, CLI(operserv), "Si no se especifican máscaras, se borrarán las que pueda haber.");
	}
#endif
	else if (!strcasecmp(param[1], "RESTART") && IsRoot(cl) && ExFunc("RESTART"))
	{
		Responde(cl, CLI(operserv), "\00312RESTART");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Reinicia los servicios.");
		Responde(cl, CLI(operserv), "Cierra y vuelve a abrir el programa.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312RESTART");
	}
	else if (!strcasecmp(param[1], "REHASH") && IsRoot(cl) && ExFunc("REHASH"))
	{
		Responde(cl, CLI(operserv), "\00312REHASH");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Refresca los servicios.");
		Responde(cl, CLI(operserv), "Este comando resulta útil cuando se han modificado los archivos de configuración o hay alguna desincronización.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312REHASH");
	}
	else if (!strcasecmp(param[1], "BACKUP") && IsRoot(cl) && ExFunc("BACKUP"))
	{
		Responde(cl, CLI(operserv), "\00312BACKUP");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Crea una copia de seguridad de la base de datos y se guarda en el archivo backup.sql");
		Responde(cl, CLI(operserv), "Cada vez que se inicie el programa y detecte este archivo pedirá su restauración.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312BACKUPP");
	}
	else if (!strcasecmp(param[1], "RESTAURAR") && IsRoot(cl) && ExFunc("RESTAURAR"))
	{
		Responde(cl, CLI(operserv), "\00312RESTAURAR");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Restaura una copia de base de datos realizada con el comando BACKUP");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312RESTAURAR");
	}
	else if (!strcasecmp(param[1], "CLOSE") && IsRoot(cl) && ExFunc("CLOSE"))
	{
		Responde(cl, CLI(operserv), "\00312CLOSE");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Cierra el programa.");
		Responde(cl, CLI(operserv), "Una vez cerrado no se podrá volver ejecutar si no es manualmente.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312CLOSE");
	}
	else if (!strcasecmp(param[1], "VACIAR") && IsRoot(cl) && ExFunc("VACIAR"))
	{
		Responde(cl, CLI(operserv), "\00312VACIAR");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Elimina todos los registros de la base de datos.");
		Responde(cl, CLI(operserv), "Requiere confirmación.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312VACIAR");
	}
	else
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(OSRaw)
{
	char *raw;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "RAW mensaje");
		return 1;
	}
	raw = Unifica(param, params, 1, -1);
	EnviaAServidor("%s", raw);
	Responde(cl, CLI(operserv), "Comando ejecutado \00312%s\003.", raw);
	return 0;
}
BOTFUNC(OSRestart)
{
	Responde(cl, CLI(operserv), "Los servicios van a reiniciarse.");
	IniciaCrono("reinicia", NULL, 1, 1, OSReinicia, NULL, 0);
	return 0;
}
BOTFUNC(OSRehash)
{
	Refresca();
	return 0;
}
BOTFUNC(OSBackup)
{
	if (MySQLBackup())
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error al intentar hacer una copia de seguridad de la base de datos.");
	else
		Responde(cl, CLI(operserv), "Base de datos copiada con éxito.");
	return 0;
}
BOTFUNC(OSRestaura)
{
	if (MySQLRestaura())
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error al intentar restaurar la base de datos.");
	else
		Responde(cl, CLI(operserv), "Base de datos restaurada con éxito.");
	return 0;
}
BOTFUNC(OSGline)
{
	char *user, *host;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "GLINE {+|-}{nick|user@host} [tiempo motivo]");
		return 1;
	}
	if (*param[1] == '+' && params < 4)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "GLINE +{nick|user@host} [tiempo motivo]");
		return 1;
	}
	else if (*param[1] != '+' && *param[1] != '-')
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, "GLINE {+|-}{nick|user@host} [tiempo motivo]");
		return 1;
	}
	user = host = NULL;
	if (!strchr(param[1] + 1, '@'))
	{
		Cliente *al;
		if ((al = BuscaCliente(param[1] + 1, NULL)))
		{
			user = al->ident;
			host = al->host;
		}
		else
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
			return 1;
		}
	}
	else
	{
		strcpy(tokbuf, param[1] + 1);
		user = strtok(tokbuf, "@");
		host = strtok(NULL, "@");
	}
	if (*param[1] == '+')
	{
		port_func(P_GLINE)(cl, ADD, user, host, atoi(param[2]), Unifica(param, params, 3, -1));
		Responde(cl, CLI(operserv), "Se ha añadido una GLine a \00312%s@%s\003 durante \00312%s\003 segundos.", user, host, param[2]);
	}
	else if (*param[1] == '-')
	{
		port_func(P_GLINE)(cl, DEL, user, host, 0, NULL);
		Responde(cl, CLI(operserv), "Se ha quitado la GLine a \00312%s@%s\003.", user, host);
	}
	return 0;
}
BOTFUNC(OSOpers)
{
	char oper[128];
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "OPERS {+|-}nick [rango]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, "OPERS {+|-}nick rango");
			return 1;
		}
		if (!IsReg(param[1] + 1))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		if (!strcasecmp(param[2], "OPER"))
			ircsprintf(oper, "%i", BDD_OPER);
		else if (!strcasecmp(param[2], "DEVEL"))
			ircsprintf(oper, "%i", BDD_DEVEL);
		else if (!strcasecmp(param[2], "ADMIN"))
		{
			if (!IsRoot(cl))
			{
				Responde(cl, CLI(operserv), OS_ERR_FORB, "");
				return 1;
			}
			ircsprintf(oper, "%i", BDD_ADMIN);
		}
		else if (!strcasecmp(param[2], "ROOT"))
		{
			if (strcasecmp(conf_set->root, cl->nombre))
			{
				Responde(cl, CLI(operserv), OS_ERR_FORB, "");
				return 1;
			}
			ircsprintf(oper, "%i", BDD_ROOT);
		}
		else if (!strcasecmp(param[2], "PREO"))
			ircsprintf(oper, "%i", BDD_PREO);
		else
		{
			Responde(cl, CLI(operserv), OS_ERR_SNTX, "OPERS +nick OPER|DEVEL|ADMIN|ROOT|PREO");
			return 1;
		}
#ifdef UDB
		PropagaRegistro("N::%s::O %c%s", param[1] + 1, CHAR_NUM, oper);
#else
		MySQLInserta(OS_MYSQL, param[1] + 1, "nivel", oper);
#endif		
		Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido añadido como \00312%s\003.", param[1] + 1, param[2]);
	}
	else if (*param[1] == '-')
	{
#ifdef UDB
		PropagaRegistro("N::%s::O", param[1] + 1);
#else
		if (!MySQLCogeRegistro(OS_MYSQL, param[1] + 1, "nivel"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no es oper.");
			return 1;
		}
		MySQLBorra(OS_MYSQL, param[1]);
#endif
		Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido borrado.", param[1] + 1);
	}
	else
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, "OPERS {+|-}nick [rango]");
		return 1;
	}
	return 0;
}
BOTFUNC(OSSajoin)
{
	Cliente *al;
	if (!port_existe(P_JOIN_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "SAJOIN nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(OSSapart)
{
	Cliente *al;
	if (!port_existe(P_PART_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "SAPART nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_PART_USUARIO_REMOTO)(al, BuscaCanal(param[2], NULL), NULL);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir de \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(OSRejoin)
{
	Cliente *al;
	if (!port_existe(P_JOIN_USUARIO_REMOTO) || !port_existe(P_PART_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "REJOIN nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_PART_USUARIO_REMOTO)(al, BuscaCanal(param[2], NULL), NULL);
	port_func(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir y a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(OSKill)
{
	Cliente *al;
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "KILL nick motivo");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_QUIT_USUARIO_REMOTO)(al, CLI(operserv), Unifica(param, params, 2, -1));
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido desconectado.", param[1]);
	return 0;
}
BOTFUNC(OSGlobal)
{
	int opts = 0;
	char *msg, t = 0; /* privmsg */
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "GLOBAL [-modos] mensaje");
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
			t = 1; /* notice */
	}
	if (opts & 0x8)
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		if ((res = MySQLQuery("SELECT item from %s%s", PREFIJO, NS_MYSQL)))
		{
			int level;
			while ((row = mysql_fetch_row(res)))
			{
#ifdef UDB
				level = NivelOperBdd(row[0]);
#else
				level = atoi(MySQLCogeRegistro(OS_MYSQL, row[0], "nivel"));
#endif
				if ((opts & 0x1) && !(level & BDD_OPER))
					continue;
				if ((opts & 0x2) && !(level & BDD_PREO))
					continue;
				if (opts & 0x4)
					memoserv_send_dl(row[0], param[2], Unifica(param, params, 3, -1));
				else
					memoserv_send_dl(row[0], operserv.hmod->nick, Unifica(param, params, 2, -1));
			}
			mysql_free_result(res);
		}
		Responde(cl, CLI(operserv), "Mensaje global enviado vía memo.");
	}
	else
	{
		Cliente *al, *bl = CLI(operserv);
		if (opts & 0x4)
		{
			bl = CreaBot(param[2], operserv.hmod->ident, operserv.hmod->host, me.nombre, operserv.hmod->modos, operserv.hmod->realname);
			msg = Unifica(param, params, 3, -1);
		}
		else
		{
			if (*param[1] == '-')
				msg = Unifica(param, params, 2, -1);
			else
				msg = Unifica(param, params, 1, -1);
		}
		for (al = clientes; al; al = al->sig)
		{
			if ((opts & 0x1) && !IsOper(al))
				continue;
			if ((opts & 0x2) && !IsPreo(al))
				continue;
			if (EsBot(al) || EsServer(al))
				continue;
			if (!t)
				Responde(al, bl, msg);
		}
		if (opts & 0x4)
			port_func(P_QUIT_USUARIO_LOCAL)(bl, conf_set->red);
		Responde(cl, CLI(operserv), "Mensaje global enviado.");
	}
	return 0;
}
BOTFUNC(OSNoticias)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	if (!strcasecmp(param[1], "ADD"))
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		char *noticia;
		if (params < 4)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS ADD botname noticia");
			return 1;
		}
		noticia = Unifica(param, params, 3, -1);
		MySQLQuery("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, param[2], noticia, time(0));
		res = MySQLQuery("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, noticia, param[2]);
		row = mysql_fetch_row(res);
		OSCargaNoticia(atoi(row[0]));
		mysql_free_result(res);
		Responde(cl, CLI(operserv), "Noticia añadida.");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS DEL nº");
			return 1;
		}
		OSBorraNoticia(atoi(param[2]));
		Responde(cl, CLI(operserv), "Noticia borrada.");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		time_t ti;
		if (!(res = MySQLQuery("SELECT n,bot,noticia,fecha from %s%s", PREFIJO, OS_NOTICIAS)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No hay noticias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "Noticias de la red");
		while ((row = mysql_fetch_row(res)))
		{
			if (strlen(row[2]) > 35)
			{
				row[2][30] = row[2][31] = row[2][32] = '.';
				row[2][33] = '\0';
			}
			buf[0] = '\0';
			ti = atol(row[3]);
			strftime(buf, sizeof(buf), "%d/%m/%Y", localtime(&ti));
			Responde(cl, CLI(operserv), "\00312%s\003 - %s - \00312%s\003 por \00312%s\003.", row[0], buf, row[2], row[1]);
		}
		mysql_free_result(res);
	}
	else
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	return 0;
}
BOTFUNC(OSAkill)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "AKILL [+|-]{mascara|patron} [motivo]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *c, *motivo;
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, "AKILL +mascara motivo");
			return 1;
		}
		param[1]++;
		if (strchr(param[1], '!'))
		{
			Responde(cl, CLI(operserv), OS_ERR_SNTX, "La máscara no debe contener el nick ('!').");
			return 1;
		}
		if (strchr(param[1], '@'))
			strcpy(buf, param[1]);
		else
			ircsprintf(buf, "*@%s", param[1]);
		motivo = Unifica(param, params, 2, -1);
		MySQLInserta(OS_AKILL, buf, "motivo", motivo);
		c = strchr(buf, '@');
		*c++ = '\0';
		port_func(P_GLINE)(CLI(operserv), ADD, buf, c, 0, motivo);
		Responde(cl, CLI(operserv), "Se ha insertado un AKILL para \00312%s\003 indefinido.", param[1]);
	}
	else if (*param[1] == '-')
	{
		char *c;
		param[1]++;
		if (strchr(param[1], '@'))
			strcpy(buf, param[1]);
		else
			ircsprintf(buf, "*@%s", param[1]);
		if (!MySQLCogeRegistro(OS_AKILL, buf, "motivo"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Esta máscara no tiene akill.");
			return 1;
		}
		MySQLBorra(OS_AKILL, buf);
		c = strchr(buf, '@');
		*c++ = '\0';
		port_func(P_GLINE)(CLI(operserv), DEL, buf, c, 0, NULL);
		Responde(cl, CLI(operserv), "Se ha retirado el AKILL a \00312%s", param[1]);
	}
	else
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = MySQLQuery("SELECT item from %s%s where item LIKE '%s'", PREFIJO, OS_AKILL, rep)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "*** AKILLS que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < operserv.maxlist && (row = mysql_fetch_row(res)); i++)
			Responde(cl, CLI(operserv), "\00312%s", row[0]);
		Responde(cl, CLI(operserv), "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
		mysql_free_result(res);
	}
	return 0;
}
BOTFUNC(OSClose)
{
	Responde(cl, CLI(operserv), "Los servicios van a cerrarse.");
	CierraColossus(0);
	return 0;
}
#ifdef UDB
BOTFUNC(OSModos)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "MODOS nick [modos]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("+ohaAOkNCWqHX", *modos))
			{
				ircsprintf(buf, "El modo %c está prohibido. Sólo se permiten los modos ohaAOkNCWqHX.", *modos);
				Responde(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		PropagaRegistro("N::%s::M %s", param[1], param[2]);
		Responde(cl, CLI(operserv), "Los modos de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("N::%s::M", param[1]);
		Responde(cl, CLI(operserv), "Se han eliminado los modos de operador para \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(OSSnomask)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, "SNOMASK nick [máscaras]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("cefFGjknNoqsSv", *modos))
			{
				ircsprintf(buf, "La máscara %c está prohibido. Sólo se permiten las máscaras cefFGjknNoqsSv.", *modos);
				Responde(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		PropagaRegistro("N::%s::K %s", param[1], param[2]);
		Responde(cl, CLI(operserv), "Las máscaras de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("N::%s::K", param[1]);
		Responde(cl, CLI(operserv), "Se han eliminado las máscaras de operador para \00312%s", param[1]);
	}
	return 0;
}
#endif
BOTFUNC(OSVaciar)
{
	if (params < 2)
	{
		confirm = AleatorioEx("********");
		Responde(cl, CLI(operserv), "Para mayor seguridad, ejecute el comando \00312VACIAR %s", confirm);
	}
	else
	{
		int i;
		if (!confirm)
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No ha solicitado confirmación. Ejecute el comando nuevamente sin parámetros.");
			return 1;
		}
		if (strcmp(confirm, param[1]))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "La confirmación no se corresponde con la dada. Ejecut el comando nuevamente sin parámetros.");
			return 1;
		}
		confirm = NULL;
		for (i = 0; i < mysql_tablas.tablas; i++)
		{
			if (!strncmp(PREFIJO, mysql_tablas.tabla[i], strlen(PREFIJO)))
				MySQLQuery("TRUNCATE %s", mysql_tablas.tabla[i]);
		}
		Responde(cl, CLI(operserv), "Se ha vaciado toda la base de datos. Se procederá a reiniciar el programa en 5 segundos...");
		IniciaCrono("reinicia", NULL, 1, 5, OSReinicia, NULL, 0);
	}
	return 0;
}
int OSCmdNick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		if (gnoticias)
		{
			int i;
			Responde(cl, gnoticia[0]->cl, "Noticias de la Red:");
			for (i = 0; i < gnoticias; i++)
				Responde(cl, gnoticia[i]->cl, "%s - %s", gnoticia[i]->fecha, gnoticia[i]->noticia);
		}
	}
	return 0;
}
int OSCmdJoin(Cliente *cl, Canal *cn)
{
	if (!IsOper(cl) || !(operserv.opts & OS_OPTS_AOP))
		return 0;
	if (!IsChanReg_dl(cn->nombre))
		port_func(P_MODO_CANAL)(RedOverride ? &me : CLI(operserv), cn, "+o %s", TRIO(cl));
	return 0;
}

int OSSigIdOk(Cliente *cl)
{
	int nivel;
#ifdef UDB
	if ((nivel = NivelOperBdd(cl->nombre)))
	{	
#else
	char *modos;
	if ((modos = MySQLCogeRegistro(OS_MYSQL, cl->nombre, "nivel")))
	{
		char tmp[5];
		if ((nivel = atoi(modos)))
		{
			if (nivel & BDD_ADMIN)
			{
				ircsprintf(tmp, "+%c%c%c", (protocolo->umodos+1)->flag, (protocolo->umodos+2)->flag, (protocolo->umodos+3)->flag);
				port_func(P_MODO_USUARIO_REMOTO)(cl, operserv.hmod->nick, tmp, 1);
			}
			else
			{
				ircsprintf(tmp, "+%c", (protocolo->umodos+3)->flag);
				port_func(P_MODO_USUARIO_REMOTO)(cl, operserv.hmod->nick, tmp, 1);
			}
#endif
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
#ifndef UDB
	}
#endif
	return 0;
}
int OSSigMySQL()
{
	if (!MySQLEsTabla(OS_NOTICIAS))
	{
		if (MySQLQuery("CREATE TABLE `%s%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`bot` text NOT NULL, "
  			"`noticia` text NOT NULL, "
  			"`fecha` bigint(20) NOT NULL default '0', "
  			"KEY `n` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de noticias';", PREFIJO, OS_NOTICIAS))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, OS_NOTICIAS);
	}
#ifndef UDB
	if (!MySQLEsTabla(OS_MYSQL))
	{
		if (MySQLQuery("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`nivel` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de operadores';", PREFIJO, OS_MYSQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, OS_MYSQL);
	}
#endif
	if (!MySQLEsTabla(OS_AKILL))
	{
		if (MySQLQuery("CREATE TABLE `%s%s` ( "
			"`item` varchar(255) default NULL, "
			"`motivo` varchar(255) default NULL, "
			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de akills';", PREFIJO, OS_AKILL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, OS_AKILL);
	}
	MySQLCargaTablas();
	OSCargaNoticias();
	return 1;
}
int OSSigSynch()
{
	Cliente *bl;
	int i = 0;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *c;
	if ((res = MySQLQuery("SELECT DISTINCT bot from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = mysql_fetch_row(res)))
		{
			bl = CreaBot(row[0], operserv.hmod->ident, operserv.hmod->host, me.nombre, "+oqkBd", "Servicio de noticias");
			for (i = 0; i < gnoticias; i++)
			{
				if (!strcmp(row[0], gnoticia[i]->botname))
					gnoticia[i]->cl = bl;
			}
		}
		mysql_free_result(res);
	}
	if ((res = MySQLQuery("SELECT item,motivo from %s%s ", PREFIJO, OS_AKILL)))
	{
		while ((row = mysql_fetch_row(res)))
		{
			if ((c = strchr(row[0], '@')))
				*c++ = '\0';
			port_func(P_GLINE)(CLI(operserv), ADD, row[0], c, 0, row[1]);
		}
		mysql_free_result(res);
	}
	return 0;
}
