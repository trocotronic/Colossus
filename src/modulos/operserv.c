/*
 * $Id: operserv.c,v 1.16 2005-03-19 12:48:52 Trocotronic Exp $ 
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
#define exfunc(x) busca_funcion(operserv.hmod, x, NULL)
Noticia *gnoticia[MAXNOT];
int gnoticias = 0;
char *confirm = NULL;

static int operserv_help		(Cliente *, char *[], int, char *[], int);
static int operserv_raw			(Cliente *, char *[], int, char *[], int);
static int operserv_restart		(Cliente *, char *[], int, char *[], int);
static int operserv_rehash		(Cliente *, char *[], int, char *[], int);
static int operserv_backup		(Cliente *, char *[], int, char *[], int);
static int operserv_restaura		(Cliente *, char *[], int, char *[], int);
static int operserv_gline		(Cliente *, char *[], int, char *[], int);
static int operserv_opers		(Cliente *, char *[], int, char *[], int);
static int operserv_sajoin		(Cliente *, char *[], int, char *[], int);
static int operserv_sapart		(Cliente *, char *[], int, char *[], int);
static int operserv_rejoin		(Cliente *, char *[], int, char *[], int);
static int operserv_kill		(Cliente *, char *[], int, char *[], int);
static int operserv_global		(Cliente *, char *[], int, char *[], int);
static int operserv_noticias		(Cliente *, char *[], int, char *[], int);
static int operserv_close		(Cliente *, char *[], int, char *[], int);
static int operserv_vaciar		(Cliente *, char *[], int, char *[], int);
#ifdef UDB
static int operserv_modos		(Cliente *, char *[], int, char *[], int);
static int operserv_snomask		(Cliente *, char *[], int, char *[], int);
#endif

static bCom operserv_coms[] = {
	{ "help" , operserv_help , OPERS } ,
	{ "raw" , operserv_raw , OPERS } ,
	{ "restart" , operserv_restart , ROOTS } ,
	{ "rehash" , operserv_rehash , ROOTS } ,
	{ "backup" , operserv_backup , ROOTS } ,
	{ "close" , operserv_close , ROOTS } ,
	{ "restaura" , operserv_restaura , ROOTS } ,
	{ "gline" , operserv_gline , OPERS } ,
	{ "opers" , operserv_opers , ADMINS } ,
	{ "sajoin" , operserv_sajoin , OPERS } ,
	{ "sapart" , operserv_sapart , OPERS } ,
	{ "rejoin" , operserv_rejoin , OPERS } ,
	{ "kill" , operserv_kill , OPERS } ,
	{ "global" , operserv_global , OPERS } ,
	{ "noticias" , operserv_noticias , OPERS } ,
	{ "vaciar" , operserv_vaciar , ROOTS } ,
#ifdef UDB
	{ "modos" , operserv_modos , ADMINS } ,
	{ "snomask" , operserv_snomask , ADMINS } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};

#ifndef UDB
int operserv_idok	(Cliente *);
#endif
int operserv_sig_mysql	();
int operserv_sig_synch	();

int operserv_join(Cliente *, Canal *);
int operserv_nick(Cliente *, int);
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
int test(Conf *, int *);

ModInfo info = {
	"OperServ" ,
	0.9 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		conferror("[%s] Falta especificar archivo de configuración para %s", mod->archivo, info.nombre);
		errores++;
	}
	else
	{
		if (parseconf(mod->config, &modulo, 1))
		{
			conferror("[%s] Hay errores en la configuración de %s", mod->archivo, info.nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
			{
				if (!test(modulo.seccion[0], &errores))
					set(modulo.seccion[0], mod);
				else
				{
					conferror("[%s] La configuración de %s no ha pasado el test", mod->archivo, info.nombre);
					errores++;
				}
			}
			else
			{
				conferror("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
				errores++;
			}
		}
		libera_conf(&modulo);
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
				irc_dlsym(ex->hmod, "ChanReg", ChanReg_dl);
			else if (!strcasecmp(ex->info->nombre, "MemoServ"))
				irc_dlsym(ex->hmod, "memoserv_send", memoserv_send_dl);
		}
	}
#endif	
	return errores;
}
int descarga()
{
	borra_senyal(SIGN_JOIN, operserv_join);
	borra_senyal(SIGN_POST_NICK, operserv_nick);
#ifndef UDB
	borra_senyal(NS_SIGN_IDOK, operserv_idok);
#endif
	borra_senyal(SIGN_MYSQL, operserv_sig_mysql);
	borra_senyal(SIGN_SYNCH, operserv_sig_synch);
	return 0;
}	
int test(Conf *config, int *errores)
{
	return 0;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *os;
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
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
		if (!strcmp(config->seccion[i]->item, "autoop"))
			operserv.opts |= OS_OPTS_AOP;
	}
	inserta_senyal(SIGN_JOIN, operserv_join);
	inserta_senyal(SIGN_POST_NICK, operserv_nick);
#ifndef UDB
	inserta_senyal(NS_SIGN_IDOK, operserv_idok);
#endif
	inserta_senyal(SIGN_MYSQL, operserv_sig_mysql);
	inserta_senyal(SIGN_SYNCH, operserv_sig_synch);
	bot_set(operserv);
}
Noticia *es_bot_noticia(char *botname)
{
	int i;
	for (i = 0; i < gnoticias; i++)
	{
		if (!strcasecmp(gnoticia[i]->botname, botname))
			return gnoticia[i];
	}
	return NULL;
}
int inserta_noticia(char *botname, char *noticia, time_t fecha, int id)
{
	Noticia *not;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Cliente *cl = NULL;
	Noticia *gn = NULL;
	time_t aux;
	if (gnoticias == MAXNOT)
		return 0;
	if (!id && !(gn = es_bot_noticia(botname)))
		cl = botnick(botname, operserv.hmod->ident, operserv.hmod->host, me.nombre, "kdBq", operserv.hmod->realname);
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
		_mysql_query("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, botname, noticia, aux);
		res = _mysql_query("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, noticia, botname);
		row = mysql_fetch_row(res);
		not->id = atoi(row[0]);
		mysql_free_result(res);
	}
	else
		not->id = id;
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
			{
				Cliente *bl = busca_cliente(botname, NULL);
				port_func(P_QUIT_USUARIO_LOCAL)(bl, "Mensajería global %s", conf_set->red);
			}
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
			inserta_noticia(row[1], row[2], atol(row[3]), atoul(row[0]));
		mysql_free_result(res);
	}
}
int os_reinicia()
{
	reinicia();
	return 0;
}
BOTFUNC(operserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(operserv), "\00312%s\003 se encarga de gestionar la red de forma global y reservada al personal cualificado para realizar esta tarea.", operserv.hmod->nick);
		response(cl, CLI(operserv), "Esta gestión permite tener un control exhaustivo sobre las actividades de la red para su buen funcionamiento.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Comandos disponibles:");
		func_resp(operserv, "RAW", "Manda un raw al servidor.");
		func_resp(operserv, "GLINE", "Gestiona las glines de la red.");
		func_resp(operserv, "SAJOIN", "Ejecuta SAJOIN sobre un usuario.");
		func_resp(operserv, "SAPART", "Ejecuta SAPART sobre un usuario.");
		func_resp(operserv, "REJOIN", "Obliga a un usuario a reentrar a un canal.");
		func_resp(operserv, "KILL", "Desconecta a un usuario.");
		func_resp(operserv, "GLOBAL", "Manda un mensaje global.");
		func_resp(operserv, "NOTICIAS", "Administra las noticias de la red.");
		if (IsAdmin(cl))
		{
			func_resp(operserv, "OPERS", "Administra los operadores de la red.");
#ifdef UDB
			func_resp(operserv, "MODOS", "Fija los modos de operador que se obtiene automáticamente (BDD).");
			func_resp(operserv, "SNOMASK", "Fija las máscaras de operador que se obtiene automáticamente (BDD).");
#endif
		}
		if (IsRoot(cl))
		{
			func_resp(operserv, "RESTART", "Reinicia los servicios.");
			func_resp(operserv, "REHASH", "Refresca los servicios.");
			func_resp(operserv, "BACKUP", "Crea una copia de seguridad de la base de datos.");
			func_resp(operserv, "RESTAURA", "Restaura la base de datos.");
			func_resp(operserv, "CLOSE", "Cierra el programa.");
			func_resp(operserv, "VACIAR", "Elimina todos los registros de la base de datos.");
		}
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Para más información, \00312/msg %s %s comando", operserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "RAW") && exfunc("RAW"))
	{
		response(cl, CLI(operserv), "\00312RAW");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Manda un raw al servidor.");
		response(cl, CLI(operserv), "Este comando actúa directamente al servidor.");
		response(cl, CLI(operserv), "No es procesado por los servicios, con lo que no tiene un registro sobre él.");
		response(cl, CLI(operserv), "Sólo debe usarse para fines administrativos cuya justificación sea de peso.");
		response(cl, CLI(operserv), "No usárase si no se tuviera conocimiento explícito de los raw's del servidor.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312RAW linea");
		response(cl, CLI(operserv), "Ejemplo: \00312RAW %s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
		response(cl, CLI(operserv), "Ejecutaría el comando \00312:%s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
	}
	else if (!strcasecmp(param[1], "GLINE") && exfunc("GLINE"))
	{
		response(cl, CLI(operserv), "\00312GLINES");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Administra los bloqueos de la red.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312GLINE {+|-}{nick|user@host} [tiempo motivo]");
		response(cl, CLI(operserv), "Para añadir una gline se antepone '+' antes de la gline. Para quitarla, '-'.");
		response(cl, CLI(operserv), "Se puede especificar un nick o un user@host indistintamente. Si se especifica un nick, se usará su user@host sin necesidad de buscarlo previamente.");
	}
	else if (!strcasecmp(param[1], "SAJOIN") && exfunc("SAJOIN"))
	{
		response(cl, CLI(operserv), "\00312SAJOIN");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Fuerza a un usuario a entrar en un canal.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312SAJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "SAPART") && exfunc("SAPART"))
	{
		response(cl, CLI(operserv), "\00312SAPART");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Fuerza a un usuario a salir de un canal.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312SAPART usuario #canal");
	}
	else if (!strcasecmp(param[1], "REJOIN") && exfunc("REJOIN"))
	{
		response(cl, CLI(operserv), "\00312REJOIN");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Fuerza a un usuario a reentrar en un canal.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312REJOIN usuario #canal");
	}
	else if (!strcasecmp(param[1], "KILL") && exfunc("KILL"))
	{
		response(cl, CLI(operserv), "\00312KILL");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Desconecta a un usuario de la red.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312KILL usuario [motivo]");
	}
	else if (!strcasecmp(param[1], "GLOBAL") && exfunc("GLOBAL"))
	{
		response(cl, CLI(operserv), "\00312GLOBAL");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Envía un mensaje global.");
		response(cl, CLI(operserv), "Dispone de varios parámetros para especificar sus destinatarios y el modo de envío.");
		response(cl, CLI(operserv), "Estos son:");
		response(cl, CLI(operserv), "\00312o\003 Exclusivo a operadores.");
		response(cl, CLI(operserv), "\00312p\003 Para preopers.");
		response(cl, CLI(operserv), "\00312b\003 Utiliza un bot para mandar el mensaje.");
		response(cl, CLI(operserv), "\00312m\003 Vía memo.");
		response(cl, CLI(operserv), "\00312n\003 Vía notice.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312GLOBAL [-parámetros [[bot]] mensaje");
		response(cl, CLI(operserv), "Si no se especifican modos, se envía a todos los usuarios vía privmsg.");
	}
	else if (!strcasecmp(param[1], "NOTICIAS") && exfunc("NOTICIAS"))
	{
		response(cl, CLI(operserv), "\00312NOTICIAS");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Gestiona las noticias de la red.");
		response(cl, CLI(operserv), "Estas noticias son mostradas cuando se conecta un usuario en forma de privado.");
		response(cl, CLI(operserv), "Se muestra la fecha y el cuerpo de la noticia y son mandadas por el bot especificado.");
		response(cl, CLI(operserv), "Las noticias son acumulativas si no se borran.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Comandos disponibles:");
		response(cl, CLI(operserv), "\00312ADD\003 Añade una noticia.");
		response(cl, CLI(operserv), "\00312DEL\003 Bora una noticia.");
		response(cl, CLI(operserv), "\00312LIST\003 Lista las noticias actuales.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis:");
		response(cl, CLI(operserv), "\00312ADD bot noticia");
		response(cl, CLI(operserv), "\00312DEL nº");
		response(cl, CLI(operserv), "\00312LIST");
	}
	else if (!strcasecmp(param[1], "OPERS") && IsAdmin(cl) && exfunc("OPERS"))
	{
		response(cl, CLI(operserv), "\00312OPERS");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Administra los operadores de la red.");
		response(cl, CLI(operserv), "Cuando un usuario es identificador como propietario de su nick, y figura como operador, se le da el modo +h.");
		response(cl, CLI(operserv), "Así queda reconocido como operador de red.");
		response(cl, CLI(operserv), "Rangos:");
		response(cl, CLI(operserv), "\00312PREO");
		response(cl, CLI(operserv), "\00312OPER (+h)");
		response(cl, CLI(operserv), "\00312DEVEL");
		response(cl, CLI(operserv), "\00312ADMIN (+N)");
		response(cl, CLI(operserv), "\00312ROOT");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312OPERS {+|-}usuario rango");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MODOS") && IsAdmin(cl) && exfunc("MODOS"))
	{
		response(cl, CLI(operserv), "\00312MODOS");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Fija los modos de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
		response(cl, CLI(operserv), "Estos modos son: \00312ohAOkNCWqHX");
		response(cl, CLI(operserv), "Serán otorgados de forma automática cada vez que el operador use su nick vía /nick nick:pass");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312MODOS operador [modos]");
		response(cl, CLI(operserv), "Si no se especifican modos, se borrarán los que pueda haber.");
	}
	else if (!strcasecmp(param[1], "SNOMASK") && IsAdmin(cl) && exfunc("SNOMASK"))
	{
		response(cl, CLI(operserv), "\00312SNOMASK");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Fija las máscaras de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
		response(cl, CLI(operserv), "Serán otorgadas de forma automática cada vez que el operador use su nick vía /nick nick:pass");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312SNOMASK operador [snomasks]");
		response(cl, CLI(operserv), "Si no se especifican máscaras, se borrarán las que pueda haber.");
	}
#endif
	else if (!strcasecmp(param[1], "RESTART") && IsRoot(cl) && exfunc("RESTART"))
	{
		response(cl, CLI(operserv), "\00312RESTART");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Reinicia los servicios.");
		response(cl, CLI(operserv), "Cierra y vuelve a abrir el programa.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312RESTART");
	}
	else if (!strcasecmp(param[1], "REHASH") && IsRoot(cl) && exfunc("REHASH"))
	{
		response(cl, CLI(operserv), "\00312REHASH");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Refresca los servicios.");
		response(cl, CLI(operserv), "Este comando resulta útil cuando se han modificado los archivos de configuración o hay alguna desincronización.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312REHASH");
	}
	else if (!strcasecmp(param[1], "BACKUP") && IsRoot(cl) && exfunc("BACKUP"))
	{
		response(cl, CLI(operserv), "\00312BACKUP");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Crea una copia de seguridad de la base de datos y se guarda en el archivo backup.sql");
		response(cl, CLI(operserv), "Cada vez que se inicie el programa y detecte este archivo pedirá su restauración.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312BACKUPP");
	}
	else if (!strcasecmp(param[1], "RESTAURAR") && IsRoot(cl) && exfunc("RESTAURAR"))
	{
		response(cl, CLI(operserv), "\00312RESTAURAR");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Restaura una copia de base de datos realizada con el comando BACKUP");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312RESTAURAR");
	}
	else if (!strcasecmp(param[1], "CLOSE") && IsRoot(cl) && exfunc("CLOSE"))
	{
		response(cl, CLI(operserv), "\00312CLOSE");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Cierra el programa.");
		response(cl, CLI(operserv), "Una vez cerrado no se podrá volver ejecutar si no es manualmente.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312CLOSE");
	}
	else if (!strcasecmp(param[1], "VACIAR") && IsRoot(cl) && exfunc("VACIAR"))
	{
		response(cl, CLI(operserv), "\00312VACIAR");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Elimina todos los registros de la base de datos.");
		response(cl, CLI(operserv), "Requiere confirmación.");
		response(cl, CLI(operserv), " ");
		response(cl, CLI(operserv), "Sintaxis: \00312VACIAR");
	}
	else
		response(cl, CLI(operserv), OS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(operserv_raw)
{
	char *raw;
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "RAW mensaje");
		return 1;
	}
	raw = implode(param, params, 1, -1);
	sendto_serv("%s", raw);
	response(cl, CLI(operserv), "Comando ejecutado \00312%s\003.", raw);
	return 0;
}
BOTFUNC(operserv_restart)
{
	response(cl, CLI(operserv), "Los servicios van a reiniciarse.");
	timer("reinicia", NULL, 1, 1, os_reinicia, NULL, 0);
	return 0;
}
BOTFUNC(operserv_rehash)
{
	refresca();
	return 0;
}
BOTFUNC(operserv_backup)
{
	if (_mysql_backup())
		response(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error al intentar hacer una copia de seguridad de la base de datos.");
	else
		response(cl, CLI(operserv), "Base de datos copiada con éxito.");
	return 0;
}
BOTFUNC(operserv_restaura)
{
	if (_mysql_restaura())
		response(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error al intentar restaurar la base de datos.");
	else
		response(cl, CLI(operserv), "Base de datos restaurada con éxito.");
	return 0;
}
BOTFUNC(operserv_gline)
{
	char *user, *host;
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "GLINE {+|-}{nick|user@host} [tiempo motivo]");
		return 1;
	}
	if (*param[1] == '+' && params < 4)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "GLINE +{nick|user@host} [tiempo motivo]");
		return 1;
	}
	else if (*param[1] != '+' && *param[1] != '-')
	{
		response(cl, CLI(operserv), OS_ERR_SNTX, "GLINE {+|-}{nick|user@host} [tiempo motivo]");
		return 1;
	}
	user = host = NULL;
	if (!strchr(param[1] + 1, '@'))
	{
		Cliente *al;
		if ((al = busca_cliente(param[1] + 1, NULL)))
		{
			user = al->ident;
			host = al->host;
		}
		else
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
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
		port_func(P_GLINE)(CLI(operserv), ADD, user, host, atoi(param[2]), implode(param, params, 3, -1));
		response(cl, CLI(operserv), "Se ha añadido una GLine a \00312%s@%s\003 durante \00312%s\003 segundos.", user, host, param[2]);
	}
	else if (*param[1] == '-')
	{
		port_func(P_GLINE)(CLI(operserv), ADD, user, host, 0, NULL);
		response(cl, CLI(operserv), "Se ha quitado la GLine a \00312%s@%s\003.", user, host);
	}
	return 0;
}
BOTFUNC(operserv_opers)
{
	char oper[128];
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "OPERS {+|-}nick [rango]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, CLI(operserv), OS_ERR_PARA, "OPERS {+|-}nick rango");
			return 1;
		}
		if (!IsReg(param[1] + 1))
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está registrado.");
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
				response(cl, CLI(operserv), OS_ERR_FORB, "");
				return 1;
			}
			sprintf_irc(oper, "%i", BDD_ADMIN);
		}
		else if (!strcasecmp(param[2], "ROOT"))
		{
			if (strcasecmp(conf_set->root, cl->nombre))
			{
				response(cl, CLI(operserv), OS_ERR_FORB, "");
				return 1;
			}
			sprintf_irc(oper, "%i", BDD_ROOT);
		}
		else if (!strcasecmp(param[2], "PREO"))
			sprintf_irc(oper, "%i", BDD_PREO);
		else
		{
			response(cl, CLI(operserv), OS_ERR_SNTX, "OPERS +nick OPER|DEVEL|ADMIN|ROOT|PREO");
			return 1;
		}
#ifdef UDB
		envia_registro_bdd("N::%s::oper %c%s", param[1] + 1, CHAR_NUM, oper);
#else
		_mysql_add(OS_MYSQL, param[1] + 1, "nivel", oper);
#endif		
		response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido añadido como \00312%s\003.", param[1] + 1, param[2]);
	}
	else if (*param[1] == '-')
	{
#ifdef UDB
		envia_registro_bdd("N::%s::oper", param[1] + 1);
#else
		if (!_mysql_get_registro(OS_MYSQL, param[1] + 1, "nivel"))
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no es oper.");
			return 1;
		}
		_mysql_del(OS_MYSQL, param[1]);
#endif
		response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido borrado.", param[1] + 1);
	}
	else
	{
		response(cl, CLI(operserv), OS_ERR_SNTX, "OPERS {+|-}nick [rango]");
		return 1;
	}
	return 0;
}
BOTFUNC(operserv_sajoin)
{
	Cliente *al;
	if (!port_existe(P_JOIN_USUARIO_REMOTO))
	{
		response(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "SAJOIN nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_sapart)
{
	Cliente *al;
	if (!port_existe(P_PART_USUARIO_REMOTO))
	{
		response(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "SAPART nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_PART_USUARIO_REMOTO)(al, busca_canal(param[2], NULL), NULL);
	response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir de \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_rejoin)
{
	Cliente *al;
	if (!port_existe(P_JOIN_USUARIO_REMOTO) || !port_existe(P_PART_USUARIO_REMOTO))
	{
		response(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "REJOIN nick #canal");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_PART_USUARIO_REMOTO)(al, busca_canal(param[2], NULL), NULL);
	port_func(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir y a entrar en \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(operserv_kill)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "KILL nick motivo");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	port_func(P_QUIT_USUARIO_REMOTO)(al, CLI(operserv), implode(param, params, 2, -1));
	response(cl, CLI(operserv), "El usuario \00312%s\003 ha sido desconectado.", param[1]);
	return 0;
}
BOTFUNC(operserv_global)
{
	int opts = 0;
	char *msg, t = 0; /* privmsg */
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "GLOBAL [-modos] mensaje");
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
					memoserv_send_dl(row[0], operserv.hmod->nick, implode(param, params, 2, -1));
			}
			mysql_free_result(res);
		}
		response(cl, CLI(operserv), "Mensaje global enviado vía memo.");
	}
	else
	{
		Cliente *al, *bl = CLI(operserv);
		if (opts & 0x4)
		{
			bl = botnick(param[2], operserv.hmod->ident, operserv.hmod->host, me.nombre, operserv.hmod->modos, operserv.hmod->realname);
			msg = implode(param, params, 3, -1);
		}
		else
		{
			if (*param[1] == '-')
				msg = implode(param, params, 2, -1);
			else
				msg = implode(param, params, 1, -1);
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
				response(al, bl, msg);
		}
		if (opts & 0x4)
			port_func(P_QUIT_USUARIO_LOCAL)(bl, conf_set->red);
		response(cl, CLI(operserv), "Mensaje global enviado.");
	}
	return 0;
}
BOTFUNC(operserv_noticias)
{
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	if (!strcasecmp(param[1], "ADD"))
	{
		if (params < 4)
		{
			response(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS ADD botname noticia");
			return 1;
		}
		inserta_noticia(param[2], implode(param, params, 3, -1), 0, 0);
		response(cl, CLI(operserv), "Noticia añadida.");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		if (params < 3)
		{
			response(cl, CLI(operserv), OS_ERR_PARA, "NOTICIAS DEL nº");
			return 1;
		}
		borra_noticia(atoi(param[2]));
		response(cl, CLI(operserv), "Noticia borrada.");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		time_t ti;
		if (!(res = _mysql_query("SELECT n,bot,noticia,fecha from %s%s", PREFIJO, OS_NOTICIAS)))
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "No hay noticias.");
			return 1;
		}
		response(cl, CLI(operserv), "Noticias de la red");
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
			response(cl, CLI(operserv), "\00312%s\003 - %s - \00312%s\003 por \00312%s\003.", row[0], buf, row[2], row[1]);
		}
		mysql_free_result(res);
	}
	else
	{
		response(cl, CLI(operserv), OS_ERR_SNTX, "NOTICIAS ADD|DEL|LIST parámetros");
		return 1;
	}
	return 0;
}
BOTFUNC(operserv_close)
{
	response(cl, CLI(operserv), "Los servicios van a cerrarse.");
	cierra_colossus(0);
	return 0;
}
#ifdef UDB
BOTFUNC(operserv_modos)
{
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "MODOS nick [modos]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("+ohaAOkNCWqHX", *modos))
			{
				sprintf_irc(buf, "El modo %c está prohibido. Sólo se permiten los modos ohaAOkNCWqHX.", *modos);
				response(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		envia_registro_bdd("N::%s::modos %s", param[1], param[2]);
		response(cl, CLI(operserv), "Los modos de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		envia_registro_bdd("N::%s::modos", param[1]);
		response(cl, CLI(operserv), "Se han eliminado los modos de operador para \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(operserv_snomask)
{
	if (params < 2)
	{
		response(cl, CLI(operserv), OS_ERR_PARA, "SNOMASK nick [máscaras]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		response(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("cefFGjknNoqsSv", *modos))
			{
				sprintf_irc(buf, "La máscara %c está prohibido. Sólo se permiten las máscaras cefFGjknNoqsSv.", *modos);
				response(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		envia_registro_bdd("N::%s::snomasks %s", param[1], param[2]);
		response(cl, CLI(operserv), "Las máscaras de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		envia_registro_bdd("N::%s::snomasks", param[1]);
		response(cl, CLI(operserv), "Se han eliminado las máscaras de operador para \00312%s", param[1]);
	}
	return 0;
}
#endif
BOTFUNC(operserv_vaciar)
{
	if (params < 2)
	{
		confirm = random_ex("********");
		response(cl, CLI(operserv), "Para mayor seguridad, ejecute el comando \00312VACIAR %s", confirm);
	}
	else
	{
		int i;
		if (!confirm)
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "No ha solicitado confirmación. Ejecute el comando nuevamente sin parámetros.");
			return 1;
		}
		if (strcmp(confirm, param[1]))
		{
			response(cl, CLI(operserv), OS_ERR_EMPT, "La confirmación no se corresponde con la dada. Ejecut el comando nuevamente sin parámetros.");
			return 1;
		}
		confirm = NULL;
		for (i = 0; i < mysql_tablas.tablas; i++)
		{
			if (!strncmp(PREFIJO, mysql_tablas.tabla[i], strlen(PREFIJO)))
				_mysql_query("TRUNCATE %s", mysql_tablas.tabla[i]);
		}
		response(cl, CLI(operserv), "Se ha vaciado toda la base de datos. Se procederá a reiniciar el programa en 5 segundos...");
		timer("reinicia", NULL, 1, 5, os_reinicia, NULL, 0);
	}
	return 0;
}
int operserv_nick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		if (gnoticias)
		{
			int i;
			response(cl, gnoticia[0]->cl, "Noticias de la Red:");
			for (i = 0; i < gnoticias; i++)
				response(cl, gnoticia[i]->cl, "%s - %s", gnoticia[i]->fecha, gnoticia[i]->noticia);
		}
	}
	return 0;
}
int operserv_join(Cliente *cl, Canal *cn)
{
	if (!IsOper(cl) || !(operserv.opts & OS_OPTS_AOP))
		return 0;
	if (!IsChanReg_dl(cn->nombre))
		port_func(P_MODO_CANAL)(RedOverride ? &me : CLI(operserv), cn, "+o %s", TRIO(cl));
	return 0;
}

int operserv_idok(Cliente *cl)
{
	int nivel;
#ifdef UDB
	if ((nivel = level_oper_bdd(cl->nombre)))
	{	
#else
	char *modos;
	if ((modos = _mysql_get_registro(OS_MYSQL, cl->nombre, "nivel")))
	{
		char tmp[5];
		if ((nivel = atoi(modos)))
		{
			if (nivel & BDD_ADMIN)
			{
				sprintf_irc(tmp, "+%c%c%c", (protocolo->umodos+1)->flag, (protocolo->umodos+2)->flag, (protocolo->umodos+3)->flag);
				port_func(P_MODO_USUARIO_REMOTO)(cl, operserv.hmod->nick, tmp, 1);
			}
			else
			{
				sprintf_irc(tmp, "+%c", (protocolo->umodos+3)->flag);
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
int operserv_sig_mysql()
{
	if (!_mysql_existe_tabla(OS_NOTICIAS))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`bot` text NOT NULL, "
  			"`noticia` text NOT NULL, "
  			"`fecha` bigint(20) NOT NULL default '0', "
  			"KEY `n` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de noticias';", PREFIJO, OS_NOTICIAS))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, OS_NOTICIAS);
	}
#ifndef UDB
	if (!_mysql_existe_tabla(OS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`nivel` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de operadores';", PREFIJO, OS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, OS_MYSQL);
	}
#endif
	_mysql_carga_tablas();
	operserv_carga_noticias();
	return 1;
}
int operserv_sig_synch()
{
	Cliente *bl;
	int i = 0;
	MYSQL_RES *res;
	MYSQL_ROW row;
	if ((res = _mysql_query("SELECT DISTINCT bot from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = mysql_fetch_row(res)))
		{
			bl = botnick(row[0], operserv.hmod->ident, operserv.hmod->host, me.nombre, "+oqkBd", "Servicio de noticias");
			for (i = 0; i < gnoticias; i++)
			{
				if (!strcmp(row[0], gnoticia[i]->botname))
					gnoticia[i]->cl = bl;
			}
		}
		mysql_free_result(res);
	}
	return 0;
}
