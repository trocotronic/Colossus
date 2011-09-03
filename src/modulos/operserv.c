/*
 * $Id: operserv.c,v 1.45 2008/06/02 10:29:08 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/operserv.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#include "modulos/memoserv.h"

OperServ *operserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, operserv->hmod, NULL)
Noticia *gnoticia[MAXNOT];
int gnoticias = 0;

BOTFUNC(OSHelp);
BOTFUNC(OSRaw);
BOTFUNCHELP(OSHRaw);
BOTFUNC(OSRestart);
BOTFUNCHELP(OSHRestart);
BOTFUNC(OSRehash);
BOTFUNCHELP(OSHRehash);
BOTFUNC(OSGline);
BOTFUNCHELP(OSHGline);
BOTFUNC(OSSpam);
BOTFUNCHELP(OSHSpam);
BOTFUNC(OSOpers);
BOTFUNCHELP(OSHOpers);
BOTFUNC(OSSajoin);
BOTFUNCHELP(OSHSajoin);
BOTFUNC(OSSapart);
BOTFUNCHELP(OSHSapart);
BOTFUNC(OSRejoin);
BOTFUNCHELP(OSHRejoin);
BOTFUNC(OSKill);
BOTFUNCHELP(OSHKill);
BOTFUNC(OSGlobal);
BOTFUNCHELP(OSHGlobal);
BOTFUNC(OSNoticias);
BOTFUNCHELP(OSHNoticias);
BOTFUNC(OSClose);
BOTFUNCHELP(OSHClose);
BOTFUNC(OSVaciar);
BOTFUNCHELP(OSHVaciar);
BOTFUNC(OSAkill);
BOTFUNCHELP(OSHAkill);
BOTFUNC(OSCache);
BOTFUNCHELP(OSHCache);
BOTFUNC(OSExportar);
BOTFUNCHELP(OSHExportar);
BOTFUNC(OSImportar);
BOTFUNCHELP(OSHImportar);

static bCom operserv_coms[] = {
	{ "help" , OSHelp , N3 , "Muestra esta ayuda." , NULL } ,
	{ "raw" , OSRaw , N3 , "Manda un raw al servidor." , OSHRaw } ,
	{ "restart" , OSRestart , N5 , "Reinicia los servicios." , OSHRestart } ,
	{ "rehash" , OSRehash , N5 , "Refresca los servicios." , OSHRehash} ,
	{ "close" , OSClose , N5 , "Cierra el programa." , OSHClose } ,
	{ "gline" , OSGline , N3 , "Gestiona las glines de la red." , OSHGline } ,
	{ "spam" , OSSpam , N3 , "Gestiona las palabras o expresiones consideradas Spam." , OSHSpam} ,
	{ "opers" , OSOpers , N4 , "Administra los operadores de la red." , OSHOpers } ,
	{ "sajoin" , OSSajoin , N2 , "Ejecuta SAJOIN sobre un usuario." , OSHSajoin } ,
	{ "sapart" , OSSapart , N2 , "Ejecuta SAPART sobre un usuario." , OSHSapart } ,
	{ "rejoin" , OSRejoin , N2 , "Obliga a un usuario a reentrar a un canal." , OSHRejoin } ,
	{ "kill" , OSKill , N2 , "Desconecta a un usuario." , OSHKill } ,
	{ "global" , OSGlobal , N2 , "Manda un mensaje global." , OSHGlobal } ,
	{ "noticias" , OSNoticias , N4 , "Administra las noticias de la red." , OSHNoticias } ,
	{ "vaciar" , OSVaciar , N5 , "Elimina todos los registros de la base de datos." , OSHVaciar } ,
	{ "akill" , OSAkill , N3 , "Prohibe la conexión de una ip/host o usuario permanentemente." , OSHAkill } ,
	{ "cache" , OSCache , N5 , "Elimina toda la cache interna de los servicios." , OSHCache } ,
	{ "exportar" , OSExportar , N5 , "Crea una copia de seguridad de la base de datos." , OSHExportar } ,
	{ "importar" , OSImportar , N5 , "Restaura una copia de seguridad de la base de datos." , OSHImportar } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

int OSSigIdOk	(Cliente *);
int OSSigSQL	();
int OSSigSynch	();
int OSSigEOS	();
int OSSigSockClose	();
void OSCargaNoticias();
void OSDescargaNoticias();

int OSCmdJoin(Cliente *, Canal *);
int OSCmdNick(Cliente *, int);
void OSSet(Conf *, Modulo *);
int OSTest(Conf *, int *);

ModInfo MOD_INFO(OperServ) = {
	"OperServ" ,
	1.0 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};

int MOD_CARGA(OperServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(OperServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(OperServ).nombre))
			{
				if (!OSTest(modulo.seccion[0], &errores))
					OSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el OSTest", mod->archivo, MOD_INFO(OperServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(OperServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		OSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(OperServ)(Modulo *mod)
{
	BorraSenyal(SIGN_JOIN, OSCmdJoin);
	BorraSenyal(SIGN_POST_NICK, OSCmdNick);
	BorraSenyal(NS_SIGN_IDOK, OSSigIdOk);
	BorraSenyal(SIGN_SQL, OSSigSQL);
	BorraSenyal(SIGN_SYNCH, OSSigSynch);
	BorraSenyal(SIGN_EOS, OSSigEOS);
	//BorraSenyal(SIGN_SOCKCLOSE, OSSigSockClose);
	OSDescargaNoticias();
	BotUnset(operserv);
	return 0;
}
int OSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], operserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void OSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!operserv)
		operserv = BMalloc(OperServ);
	operserv->maxlist = 30;
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, operserv_coms);
			else if (!strcmp(config->seccion[i]->item, "autoop"))
				operserv->opts |= OS_OPTS_AOP;
			else if (!strcmp(config->seccion[i]->item, "maxlist"))
				operserv->maxlist = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, operserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, operserv_coms);
	InsertaSenyal(SIGN_JOIN, OSCmdJoin);
	InsertaSenyal(SIGN_POST_NICK, OSCmdNick);
	InsertaSenyal(NS_SIGN_IDOK, OSSigIdOk);
	InsertaSenyal(SIGN_SQL, OSSigSQL);
	InsertaSenyal(SIGN_SYNCH, OSSigSynch);
	InsertaSenyal(SIGN_EOS, OSSigEOS);
	//InsertaSenyal(SIGN_SOCKCLOSE, OSSigSockClose);
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
	SQLRes res;
	SQLRow row;
	time_t aux;
	for (i = 0; i < gnoticias; i++)
	{
		if (gnoticia[i]->id == id)
		{
			not = gnoticia[i];
			goto sigue;
		}
	}
	not = BMalloc(Noticia);
	if ((res = SQLQuery("SELECT bot,noticia,fecha FROM %s%s WHERE n=%i", PREFIJO, OS_NOTICIAS, id)))
	{
		row = SQLFetchRow(res);
		not->botname = strdup(row[0]);
		not->noticia = strdup(row[1]);
		aux = atoul(row[2]);
		strftime(not->fecha, sizeof(not->fecha), "%d/%m/%Y", localtime(&aux));
		not->id = id;
	}
	sigue:
	if (!refrescando)
	{
		if (!(gn = OSEsBotNoticia(not->botname)))
			not->cl = CreaBot(not->botname, operserv->hmod->ident, operserv->hmod->host, "kdBq", operserv->hmod->realname);
		else
			not->cl = gn->cl;
	}
	gnoticia[gnoticias++] = not;
}
int OSDescargaNoticia(int id)
{
	int i;
	Noticia *not = NULL;
	Cliente *gl = NULL;
	for (i = 0; i < gnoticias; i++)
	{
		if (gnoticia[i]->id == id)
		{
			not = gnoticia[i];
			gl = not->cl;
			gnoticias--;
			while (i < gnoticias)
			{
				gnoticia[i] = gnoticia[i+1];
				i++;
			}
			break;
		}
	}
	if (!not)
		return 0;
	if (!OSEsBotNoticia(not->botname) && gl)
	{
		ircsprintf(buf, "Mensajería global %s", conf_set->red);
		DesconectaBot(gl, buf);
	}
	ircfree(not->botname);
	ircfree(not->noticia);
	ircfree(not);
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
	SQLRes res;
	SQLRow row;
	Cliente *cl = NULL;
	Noticia *gn = NULL;
	time_t aux;
	char *m_c, *bot = NULL;
	if (gnoticias == MAXNOT)
		return 0;
	if (!id && !(gn = OSEsBotNoticia(botname)))
		cl = CreaBot(botname, operserv->hmod->ident, operserv->hmod->host, "kdBq", operserv->hmod->realname);
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
		bot = strdup(botname);
		m_c = SQLEscapa(noticia);
		SQLQuery("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, botname, m_c, aux);
		res = SQLQuery("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, m_c, bot);
		Free(m_c);
		row = SQLFetchRow(res);
		not->id = atoi(row[0]);
		SQLFreeRes(res);
		Free(bot);
	}
	else
		not->id = id;
	gnoticia[gnoticias++] = not;
	return 1;
}
int OSBorraNoticia(int id)
{
	SQLQuery("DELETE from %s%s where n=%i", PREFIJO, OS_NOTICIAS, id);
	return OSDescargaNoticia(id);
}
void OSCargaNoticias()
{
	SQLRes res;
	SQLRow row;
	if ((res = SQLQuery("SELECT * from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = SQLFetchRow(res)))
			OSCargaNoticia(atoi(row[0]));
		SQLFreeRes(res);
	}
}
int OSReinicia()
{
	Reinicia();
	return 0;
}
BOTFUNCHELP(OSHRaw)
{
	Responde(cl, CLI(operserv), "Manda un raw al servidor.");
	Responde(cl, CLI(operserv), "Este comando actúa directamente al servidor.");
	Responde(cl, CLI(operserv), "No es procesado por los servicios, con lo que no tiene un registro sobre él.");
	Responde(cl, CLI(operserv), "Sólo debe usarse para fines administrativos cuya justificación sea de peso.");
	Responde(cl, CLI(operserv), "No usárase si no se tuviera conocimiento explícito de los raw's del servidor.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312RAW linea");
	Responde(cl, CLI(operserv), "Ejemplo: \00312RAW %s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
	Responde(cl, CLI(operserv), "Ejecutaría el comando \00312:%s SWHOIS %s :Esto es un swhois", me.nombre, cl->nombre);
	return 0;
}
BOTFUNCHELP(OSHGline)
{
	Responde(cl, CLI(operserv), "Administra los bloqueos de la red.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312GLINE {nick|user@host} [[tiempo] motivo]");
	Responde(cl, CLI(operserv), "Para añadir una gline basta con especificar un motivo.");
	Responde(cl, CLI(operserv), "Si no se especifica ningún motivo, se quitará esa gline.");
	Responde(cl, CLI(operserv), "Se puede especificar un nick o un user@host indistintamente. Si se especifica un nick, se usará su user@host sin necesidad de buscarlo previamente.");
	return 0;
}
BOTFUNCHELP(OSHSpam)
{
	Responde(cl, CLI(operserv), "Gestiona aqui tu lista de palabras o expresiones considerada SPAM.");
	Responde(cl, CLI(operserv), "Para añadir una palabra deberas usar el simbolo +, para eliminar -");
	Responde(cl, CLI(operserv), "Podras listar las palabras o expresiones usando un patrón.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Añadir SPAM:");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SPAM +spam [flags] [accion] [motivo]");
	Responde(cl, CLI(operserv), "Eliminar SPAM:");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SPAM -spam");
	Responde(cl, CLI(operserv), "Listar SPAM:");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SPAM patrón");
	return 0;
}
BOTFUNCHELP(OSHSajoin)
{
	Responde(cl, CLI(operserv), "Fuerza a un usuario a entrar en un canal.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SAJOIN usuario #canal");
	return 0;
}
BOTFUNCHELP(OSHSapart)
{
	Responde(cl, CLI(operserv), "Fuerza a un usuario a salir de un canal.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SAPART usuario #canal");
	return 0;
}
BOTFUNCHELP(OSHRejoin)
{
	Responde(cl, CLI(operserv), "Fuerza a un usuario a reentrar en un canal.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312REJOIN usuario #canal");
	return 0;
}
BOTFUNCHELP(OSHKill)
{
	Responde(cl, CLI(operserv), "Desconecta a un usuario de la red.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312KILL usuario [motivo]");
	return 0;
}
BOTFUNCHELP(OSHGlobal)
{
	Responde(cl, CLI(operserv), "Envía un mensaje global.");
	Responde(cl, CLI(operserv), "Dispone de varios parámetros para especificar sus destinatarios y el modo de envío.");
	Responde(cl, CLI(operserv), "Estos son:");
	Responde(cl, CLI(operserv), "\00312o\003 Exclusivo a operadores.");
	Responde(cl, CLI(operserv), "\00312p\003 Para preopers.");
	Responde(cl, CLI(operserv), "\00312b\003 Utiliza un bot para mandar el mensaje.");
	Responde(cl, CLI(operserv), "\00312m\003 Vía memo.");
	Responde(cl, CLI(operserv), "\00312n\003 Vía notice.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312GLOBAL [-parámetros [bot] mensaje");
	Responde(cl, CLI(operserv), "Si no se especifican modos, se envía a todos los usuarios vía privmsg.");
	return 0;
}
BOTFUNCHELP(OSHNoticias)
{
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
	return 0;
}
BOTFUNCHELP(OSHAkill)
{
	Responde(cl, CLI(operserv), "La principal ventaja es que estas prohibiciones nunca se pierden, aunque se reinicie el servidor.");
	Responde(cl, CLI(operserv), "Su entrada siempre será prohibida, aunque los servicios no estén conectados.");
	Responde(cl, CLI(operserv), "Si no se especifica ninguna acción (+ o -) se listarán los host que coincidan con el patrón.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis:");
	Responde(cl, CLI(operserv), "AKILL [[user@]{host|ip} [motivo]]");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Ejemplos:");
	Responde(cl, CLI(operserv), "Añadir akills.");
	Responde(cl, CLI(operserv), "AKILL +pepito@palotes.com largo");
	Responde(cl, CLI(operserv), "AKILL +127.0.0.1 largo");
	Responde(cl, CLI(operserv), "Eliminar akills");
	Responde(cl, CLI(operserv), "AKILL -google.com");
	Responde(cl, CLI(operserv), "AKILL -*@*.aol.com");
	Responde(cl, CLI(operserv), "Listar akills");
	Responde(cl, CLI(operserv), "AKILL *@.com");
	return 0;
}
BOTFUNCHELP(OSHOpers)
{
	Responde(cl, CLI(operserv), "Administra los operadores de la red.");
	Responde(cl, CLI(operserv), "Cuando un usuario es identificador como propietario de su nick, y figura como operador, se le da el modo +h.");
	Responde(cl, CLI(operserv), "Así queda reconocido como operador de red.");
	Responde(cl, CLI(operserv), "Rangos:");
	Responde(cl, CLI(operserv), "\00312PREO");
	Responde(cl, CLI(operserv), "\00312OPER (+h)");
	Responde(cl, CLI(operserv), "\00312ADMIN (+N)");
	Responde(cl, CLI(operserv), "\00312ROOT");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312OPERS usuario [rango]");
	return 0;
}
BOTFUNCHELP(OSHRestart)
{
	Responde(cl, CLI(operserv), "Reinicia los servicios.");
	Responde(cl, CLI(operserv), "Cierra y vuelve a abrir el programa.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312RESTART");
	return 0;
}
BOTFUNCHELP(OSHRehash)
{
	Responde(cl, CLI(operserv), "Refresca los servicios.");
	Responde(cl, CLI(operserv), "Este comando resulta útil cuando se han modificado los archivos de configuración o hay alguna desincronización.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312REHASH");
	return 0;
}
BOTFUNCHELP(OSHClose)
{
	Responde(cl, CLI(operserv), "Cierra el programa.");
	Responde(cl, CLI(operserv), "Una vez cerrado no se podrá volver ejecutar si no es manualmente.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312CLOSE");
	return 0;
}
BOTFUNCHELP(OSHVaciar)
{
	Responde(cl, CLI(operserv), "Elimina todos los registros de la base de datos.");
	Responde(cl, CLI(operserv), "Requiere confirmación.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312VACIAR");
	return 0;
}
BOTFUNCHELP(OSHCache)
{
	Responde(cl, CLI(operserv), "Elimina la cache interna de los servicios.");
	Responde(cl, CLI(operserv), "Este comando es muy delicado y sólo debe de usarse en caso expreso, ya que elimina comprobaciones y puede saturar el proceso.");
	Responde(cl, CLI(operserv), "La cache intera es una tabla de datos que se usa para no tener que procesar lo mismo una y otra vez. Los resultados de búsquedas, consultas, etc. se escriben ahí.");
	Responde(cl, CLI(operserv), "Así, no es necesario hacer la misma consulta. Se lee el resultado en la tabla y se ahorra mucho tiempo.");
	Responde(cl, CLI(operserv), "En esa tabla se guardan valores como últimos registros, comprobaciones de proxys, etc. Si se elimina, todos estas consultas volverán a procesarse.");
	Responde(cl, CLI(operserv), "Requiere confirmación.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312CACHE");
	return 0;
}
BOTFUNCHELP(OSHExportar)
{
	Responde(cl, CLI(operserv), "Crea una copia de seguridad de la base de datos.");
	Responde(cl, CLI(operserv), "NOTA: tenga en cuenta de que sólo se hará la copia de aquellas tablas que estén cargadas. Si existe alguna tabla de un módulo descargado, no se hará su copia.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312EXPORTAR [etiqueta]");
	return 0;
}
BOTFUNCHELP(OSHImportar)
{
	Responde(cl, CLI(operserv), "Restaura una copia de seguridad de la base de datos.");
	Responde(cl, CLI(operserv), "Esta copia debe haberse realizado con el comando EXPORTAR.");
	Responde(cl, CLI(operserv), "Para listar las copias existentes, no utilice ningún parámetro.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "NOTA: no cargue una base de datos con datos que pertenezcan a módulos no cargados. Debe tener los mismos módulos cargados que cuando hizo la copia de seguridad.");
	Responde(cl, CLI(operserv), "NOTA: no realice importaciones de bases de datos que pertenezcan a otras versiones de Colossus.");
	Responde(cl, CLI(operserv), "Sintaxis: \00312IMPORTAR [etiqueta]");
	return 0;
}
BOTFUNC(OSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), "\00312%s\003 se encarga de gestionar la red de forma global y reservada al personal cualificado para realizar esta tarea.", operserv->hmod->nick);
		Responde(cl, CLI(operserv), "Esta gestión permite tener un control exhaustivo sobre las actividades de la red para su buen funcionamiento.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Comandos disponibles:");
		ListaDescrips(operserv->hmod, cl);
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Para más información, \00312/msg %s %s comando", operserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], operserv->hmod, param, params))
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Opción desconocida.");
	EOI(operserv, 0);
	return 0;
}
BOTFUNC(OSRaw)
{
	char *raw;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "mensaje");
		return 1;
	}
	raw = Unifica(param, params, 1, -1);
	EnviaAServidor("%s", raw);
	Responde(cl, CLI(operserv), "Comando ejecutado \00312%s\003.", raw);
	EOI(operserv, 1);
	return 0;
}
BOTFUNC(OSRestart)
{
	Responde(cl, CLI(operserv), "Los servicios van a reiniciarse.");
	IniciaCrono(1, 1, OSReinicia, NULL);
	//EOI(operserv, 2);
	return 0;
}
BOTFUNC(OSRehash)
{
	Refresca();
	//EOI(operserv, 3); *** PROHIBIDO ***
	return -1; /* stop stop stop. paramos todo lo que venga a continuación */
}
BOTFUNC(OSGline)
{
	char *user, *host, *userhost; 
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "{nick|user@host} [tiempo motivo]");
		return 1;
	}
	user = host = NULL;
	if (!strchr(param[1], '@'))
	{
		Cliente *al;
		if ((al = BuscaCliente(param[1])))
		{
			user = al->ident;
			host = al->host;
		}
		else
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado. Especifique una máscara.");
			return 1;
		}
	}
	else
	{
		strlcpy(tokbuf, param[1], sizeof(tokbuf));
		user = strtok(tokbuf, "@");
		host = strtok(NULL, "@");
	}
	if (params < 3)
	{
		Tkl *tkl = NULL;
		ircsprintf(buf, "%s@%s", user, host);
		if (!(tkl = BuscaTKL(buf, tklines[TKL_GLINE])))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Esta gline no existe.");
			return 1;
		}
		ProtFunc(P_GLINE)(cl, DEL, user, host, 0, NULL);		
		Responde(cl, CLI(operserv), "Se ha quitado la GLine a \00312%s@%s\003.", user, host);
		userhost = MascaraTKL(user,host);	
		if (SQLCogeRegistro(OS_LINES, userhost, "motivo")) //Si es un akill
			SQLBorra(OS_LINES, userhost);
	}
	else if (params < 4)
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "GLINE {nick|user@host} [tiempo] motivo");
		return 1;
	}
	else
	{
		if (IsDigit(*param[2]) &&  *param[2] > 48)
		{
			ProtFunc(P_GLINE)(cl, ADD, user, host, atoi(param[2]), Unifica(param, params, 3, -1));
			Responde(cl, CLI(operserv), "Se ha añadido una GLine a \00312%s@%s\003 durante \00312%s\003 segundos.", user, host, param[2]);
		}
		else
		{	
			int motivo = 0;
			if (*param[2] == 48) //Controlamos si los segundo son 0 para coger todo el motivo o saltarnos el 0
				motivo = 3;
			else
				motivo = 2;
				
			ProtFunc(P_GLINE)(cl, ADD, user, host, 0, Unifica(param, params, motivo, -1));
			userhost = MascaraTKL(user,host); 			
			SQLInserta(OS_LINES, userhost, "motivo",Unifica(param, params, motivo, -1));
			Responde(cl, CLI(operserv), "Se ha añadido una GLine a \00312%s\003 permanentemente.", userhost);
		}
	}
	EOI(operserv, 4);
	return 0;
}
BOTFUNC(OSSpam)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "[+|-]{palabra|patrón} [flags] [accion] [motivo]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *motivo, *c;
		if (params < 5)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "+palabra [flags] [accion] [motivo]");
			return 1;
		}
		param[1]++;
		if (SQLCogeRegistro(OS_LINES, param[1], "motivo"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "La palabra ya se encuentra en la lista de SPAM");
			return 1;
		}
		for (c = param[2]; !BadPtr(c); c++)
		{
			if (!strchr("cpnNPqduat", *c))
			{
				Responde(cl, CLI(operserv), OS_ERR_EMPT, "Sólo se aceptan las flags c|p|n|N|P|q|d|u|a|t");
				return 1;
			}
		}
		for (c = param[3]; !BadPtr(c); c++)
		{
			if (!strchr("bdgkKsSvzZ", *c))
			{
				Responde(cl, CLI(operserv), OS_ERR_EMPT, "Sólo se aceptan las acciones b|d|g|k|K|s|S|v|z|Z");
				return 1;
			}
		}
		motivo = Unifica(param, params, 4, -1);		
		SQLQuery("INSERT INTO %s%s (item,tipo,flags,accion,motivo) VALUES ('%s','%s','%s','%s','%s')",PREFIJO, OS_LINES,
				param[1], "F", param[2], param[3], motivo);
		motivo = str_replace(motivo, ' ', '_'); //Normalizamos para que lo muestre bien
		ProtFunc(P_SPAM)(CLI(operserv), ADD, param[1], param[2], param[3], motivo);
		Responde(cl, CLI(operserv), "Se ha añadido la palabra \00312%s\003 a la lista de spam.", param[1]);
	}
	else if (*param[1] == '-')
	{
		SQLRes res;
		SQLRow row;
		param[1]++;
		if (!SQLCogeRegistro(OS_LINES, param[1], "motivo"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "La palabra no se encuentra en la lista de SPAM");
			return 1;
		}		
		res = SQLQuery("SELECT flags,accion from %s%s where item LIKE '%s'", PREFIJO, OS_LINES, param[1]);
		row = SQLFetchRow(res);
		Responde(cl, CLI(operserv), "Se ha eliminado la palabra \00312%s\003 a la lista de spam.", param[1]);
		ProtFunc(P_SPAM)(CLI(operserv), DEL, param[1], row[0], row[1], NULL);
		SQLBorra(OS_LINES, param[1]);
		SQLFreeRes(res);
	}
	else
	{
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,tipo,flags,accion,motivo from %s%s where item LIKE '%s' and tipo='F'", PREFIJO, OS_LINES, rep)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "*** SPAM que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < operserv->maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(operserv), "Spam: \00312%s\003 Flags: \00312%s\003 Accion: \00312%s\003 Motivo: \00312%s\003", row[0], row[2], row[3], row[4]);
		Responde(cl, CLI(operserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}	
	return 0;
}
BOTFUNC(OSOpers)
{
	Nivel *niv = NULL;
	Cliente *al = NULL;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "[+|-]{nick|patrón} [nivel]");
		return 1;
	}
	if (*param[1] == '+')
	{
		param[1]++;
		if (!IsReg(param[1]))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Debes definir un nivel para el operador.");
			return 1;
		}
		if (!(niv = BuscaNivel(param[2])))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nivel no existe.");
			return 1;
		}
		SQLInserta(OS_SQL, param[1], "nivel", "%i", niv->nivel);
		Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido añadido como \00312%s\003.", param[1], param[2]);
		if ((al = BuscaCliente(param[1])))
			al->nivel |= niv->nivel;
		
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		if (!SQLCogeRegistro(OS_SQL, param[1], "nivel"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no es representante de la red.");
			return 1;
		}
		SQLBorra(OS_SQL, param[1]);
		Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido borrado.", param[1]);
		if ((al = BuscaCliente(param[1])))
		{
			if ((niv = BuscaNivel("USER")))
				al->nivel &= ~niv->nivel;
			else
				al->nivel = 0;
		}

	}
	else
	{	
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,nivel from %s%s where item LIKE '%s'", PREFIJO, OS_SQL, rep)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "*** Listado de representantes de la RED ***");

		for (i = 0; i < operserv->maxlist && (row = SQLFetchRow(res));)
		{		
				Responde(cl, CLI(operserv), "%s (%s)", row[0], row[1]);
				i++;
		}

		Responde(cl, CLI(operserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
		return 1;
	}			
	EOI(operserv, 5);
	return 0;
}
BOTFUNC(OSSajoin)
{
	Cliente *al;
	if (!ProtFunc(P_JOIN_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	ProtFunc(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a entrar en \00312%s\003.", param[1], param[2]);
	EOI(operserv, 6);
	return 0;
}
BOTFUNC(OSSapart)
{
	Cliente *al;
	if (!ProtFunc(P_PART_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	ProtFunc(P_PART_USUARIO_REMOTO)(al, BuscaCanal(param[2]), NULL);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir de \00312%s\003.", param[1], param[2]);
	EOI(operserv, 7);
	return 0;
}
BOTFUNC(OSRejoin)
{
	Cliente *al;
	if (!ProtFunc(P_JOIN_USUARIO_REMOTO) || !ProtFunc(P_PART_USUARIO_REMOTO))
	{
		Responde(cl, CLI(operserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick #canal");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	ProtFunc(P_PART_USUARIO_REMOTO)(al, BuscaCanal(param[2]), NULL);
	ProtFunc(P_JOIN_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido forzado a salir y a entrar en \00312%s\003.", param[1], param[2]);
	EOI(operserv, 8);
	return 0;
}
BOTFUNC(OSKill)
{
	Cliente *al;
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick motivo");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (IsOper(al) && !IsAdmin(cl))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "No puedes ejecutar este comando sobre Operadores.");
		return 1;
	}
	ProtFunc(P_QUIT_USUARIO_REMOTO)(al, CLI(operserv), Unifica(param, params, 2, -1));
	Responde(cl, CLI(operserv), "El usuario \00312%s\003 ha sido desconectado.", param[1]);
	EOI(operserv, 9);
	return 0;
}
BOTFUNC(OSGlobal)
{
	int opts = 0;
	char *msg, t = 0; /* privmsg */
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "[-modos] mensaje");
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
		SQLRes res;
		SQLRow row;
		Cliente *al; 
		if ((res = SQLQuery("SELECT item from %s%s where estado!='F'", PREFIJO, NS_SQL)))
		{
			while ((row = SQLFetchRow(res)))
			{
				al = BuscaCliente(row[0]);
				if ((opts & 0x1) && !IsOper(al))
					continue;
				if ((opts & 0x2) && !IsPreo(al))
					continue;
				if (opts & 0x4)
					MSSend(row[0], param[2], Unifica(param, params, 3, -1));
				else
					MSSend(row[0], operserv->hmod->nick, Unifica(param, params, 2, -1));
			}
			SQLFreeRes(res);
		}
		Responde(cl, CLI(operserv), "Mensaje global enviado vía memo.");
	}
	else
	{
		Cliente *al, *bl = CLI(operserv);
		if (opts & 0x4)
		{
			bl = CreaBot(param[2], operserv->hmod->ident, operserv->hmod->host, operserv->hmod->modos, operserv->hmod->realname);
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
			if (EsBot(al) || EsServidor(al))
				continue;
			if (!t)
				Responde(al, bl, msg);
			else
				ProtFunc(P_NOTICE)(al, bl, msg);
		}
		if (opts & 0x4)
			ProtFunc(P_QUIT_USUARIO_LOCAL)(bl, conf_set->red);
		if (!t)
			Responde(cl, CLI(operserv), "Mensaje global enviado.");
		else
			Responde(cl, CLI(operserv), "Notice global enviado.");
	}
	EOI(operserv, 10);
	return 0;
}
BOTFUNC(OSNoticias)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "ADD|DEL|LIST parámetros");
		return 1;
	}
	if (!strcasecmp(param[1], "ADD"))
	{
		SQLRes res;
		SQLRow row;
		char *noticia, *bot, *m_c;
		if (params < 4)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "ADD botname noticia");
			return 1;
		}

		noticia = Unifica(param, params, 3, -1);
		bot = strdup(param[2]);
		m_c = SQLEscapa(noticia);
		SQLQuery("INSERT into %s%s (bot,noticia,fecha) values ('%s','%s','%lu')", PREFIJO, OS_NOTICIAS, param[2], m_c, time(0));
		res = SQLQuery("SELECT n from %s%s where noticia='%s' AND bot='%s'", PREFIJO, OS_NOTICIAS, m_c, bot);
		Free(m_c);
		row = SQLFetchRow(res);
		OSCargaNoticia(atoi(row[0]));
		SQLFreeRes(res);
		Free(bot);
		Responde(cl, CLI(operserv), "Noticia añadida.");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "DEL nº");
			return 1;
		}
		OSBorraNoticia(atoi(param[2]));
		Responde(cl, CLI(operserv), "Noticia borrada.");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		SQLRes res;
		SQLRow row;
		time_t ti;
		if (!(res = SQLQuery("SELECT n,bot,noticia,fecha from %s%s", PREFIJO, OS_NOTICIAS)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No hay noticias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "Noticias de la red:");
		while ((row = SQLFetchRow(res)))
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
		SQLFreeRes(res);
	}
	else
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, "Opción desconcida.");
		return 1;
	}
	EOI(operserv, 11);
	return 0;
}
BOTFUNC(OSAkill)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "[+|-]{mascara|patrón} [motivo]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *user, *host, *motivo;
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "+mascara motivo");
			return 1;
		}
		param[1]++;
		if (strchr(param[1], '!'))
		{
			Responde(cl, CLI(operserv), OS_ERR_SNTX, "La máscara no debe contener el nick ('!').");
			return 1;
		}
		if (strchr(param[1], '@'))
			strlcpy(buf, param[1], sizeof(buf));
		else
			ircsprintf(buf, "*@%s", param[1]);
		motivo = Unifica(param, params, 2, -1);
		SQLInserta(OS_LINES, buf, "motivo", motivo);
		strlcpy(tokbuf, buf, sizeof(tokbuf));
		user = strtok(tokbuf, "@");
		host = strtok(NULL, "@");
		ProtFunc(P_GLINE)(CLI(operserv), ADD, user, host, 0, motivo);
		Responde(cl, CLI(operserv), "Se ha insertado un AKILL para \00312%s\003 indefinido.", param[1]);
	}
	else if (*param[1] == '-')
	{
		char *user, *host;
		param[1]++;
		if (strchr(param[1], '@'))
			strlcpy(buf, param[1], sizeof(buf));
		else
			ircsprintf(buf, "*@%s", param[1]);
		if (!SQLCogeRegistro(OS_LINES, buf, "motivo"))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Esta máscara no tiene akill.");
			return 1;
		}
		SQLBorra(OS_LINES, buf);
		strlcpy(tokbuf, buf, sizeof(tokbuf));
                user = strtok(tokbuf, "@");
                host = strtok(NULL, "@");
		ProtFunc(P_GLINE)(CLI(operserv), DEL, user, host, 0, NULL);
		Responde(cl, CLI(operserv), "Se ha retirado el AKILL a \00312%s", param[1]);
	}
	else
	{
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,tipo,motivo from %s%s where item LIKE '%s' and tipo='G'", PREFIJO, OS_LINES, rep)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(operserv), "*** AKILLS que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < operserv->maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(operserv), "\00312%s\003: %s", row[0], row[2]);
		Responde(cl, CLI(operserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	EOI(operserv, 12);
	return 0;
}
BOTFUNC(OSClose)
{
	Responde(cl, CLI(operserv), "Los servicios van a cerrarse.");
	CierraColossus(0);
	EOI(operserv, 13);
	return 0;
}
BOTFUNC(OSVaciar)
{
	static char *confirm = NULL;
	if (params < 2)
	{
		confirm = AleatorioEx("********");
		Responde(cl, CLI(operserv), "Para mayor seguridad, ejecute el comando \00312%s %s", fc->com, confirm);
	}
	else
	{
		int i = 0;
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
		for (i = 0; i < sql->tablas; i++)
		{
			if (sql->tabla[i].cargada)
				SQLQuery("TRUNCATE %s", sql->tabla[i].tabla);
		}
		Responde(cl, CLI(operserv), "Se ha vaciado toda la base de datos. Se procederá a reiniciar el programa en 5 segundos...");
		IniciaCrono(1, 5, OSReinicia, NULL);
	}
	EOI(operserv, 14);
	return 0;
}
BOTFUNC(OSCache)
{
	static char *confirm = NULL;
	if (params < 2)
	{
		confirm = AleatorioEx("********");
		Responde(cl, CLI(operserv), "Para mayor seguridad, ejecute el comando \00312%s %s", fc->com, confirm);
	}
	else
	{
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
		SQLQuery("DELETE FROM %s%s", PREFIJO, SQL_CACHE);
		Responde(cl, CLI(operserv), "Se ha vaciado toda la cache interna.");
	}
	EOI(operserv, 15);
	return 0;
}
BOTFUNC(OSExportar)
{
	if (SQLDump(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Se ha producido un error y no es posible hacer la copia.");
		return 1;
	}
	Responde(cl, CLI(operserv), "Copia realizada con éxito.");
	return 0;
}
BOTFUNC(OSImportar)
{
	if (params < 2)
	{
		Directorio dir;
		if ((dir = AbreDirectorio(SQL_BCK_DIR)))
		{
			char *f, *c;
			Responde(cl, CLI(operserv), "Etiquetas existentes:");
			Responde(cl, CLI(operserv), " ");
			while ((f = LeeDirectorio(dir)))
			{
				if ((c = strrchr(f, '.')) && !strcmp(c, ".sql"))
				{
					*c = 0;
					if ((c = strchr(f, '-')))
						Responde(cl, CLI(operserv), "Tag: \00312 %s", c+1);
				}
			}
		}
		else
			Responde(cl, CLI(operserv), "No existen copias de seguridad.");
	}
	else if (params == 2)
	{
		if (SQLImportar(param[1]) == 1)
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "Esta etiqueta no existe.");
		else
			Responde(cl, CLI(operserv), "Copia restaurada con éxito. Se recomienda que realice un restart de Colossus.");
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
	if (!IsOper(cl) || !(operserv->opts & OS_OPTS_AOP))
		return 0;
	if (!IsChanReg(cn->nombre))
		ProtFunc(P_MODO_CANAL)(RedOverride ? &me : CLI(operserv), cn, "+o %s", TRIO(cl));
	return 0;
}
int OSSigIdOk(Cliente *cl)
{
	int nivel;
	char *modos;
	if ((modos = SQLCogeRegistro(OS_SQL, cl->nombre, "nivel")))
	{
		if ((nivel = atoi(modos)))
		{
			int i;
			for (i = 0; i < nivs; i++)
			{
				if ((nivel & niveles[i]->nivel) && niveles[i]->flags)
					ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(operserv), niveles[i]->flags, 1);
			}
		}
		cl->nivel = nivel+1; //Para identificarlo, aparte del nivel le añadimos 1
	}
	else //Si no tiene nivel es que esta identificado
		cl->nivel = 1;
	return 0;
}
int OSSigSQL()
{
	SQLNuevaTabla(OS_NOTICIAS, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"n SERIAL, "
  		"bot text, "
  		"noticia text, "
  		"fecha int4 default '0' "
		");", PREFIJO, OS_NOTICIAS);
	SQLNuevaTabla(OS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
		"item varchar(255) default NULL, "
		"nivel varchar(255) default NULL, "
		"KEY item (item) "
		");", PREFIJO, OS_SQL);
	SQLNuevaTabla(OS_LINES, "CREATE TABLE IF NOT EXISTS %s%s ( "
		"item varchar(255) default NULL, "
		"tipo ENUM( 'G', 'S', 'F' ) default 'G', "
		"flags varchar(255) default NULL, "
		"motivo varchar(255) default NULL, "
		"accion varchar(255) default NULL, "
		"KEY item (item) "
		");", PREFIJO, OS_LINES);
	OSCargaNoticias();
	return 0;
}
int OSSigSynch()
{
	Cliente *bl;
	int i = 0;
	SQLRes res;
	SQLRow row;
	if ((res = SQLQuery("SELECT DISTINCT bot from %s%s", PREFIJO, OS_NOTICIAS)))
	{
		while ((row = SQLFetchRow(res)))
		{
			bl = CreaBot(row[0], operserv->hmod->ident, operserv->hmod->host, "+oqkBd", "Servicio de noticias");
			for (i = 0; i < gnoticias; i++)
			{
				if (!strcmp(row[0], gnoticia[i]->botname))
					gnoticia[i]->cl = bl;
			}
		}
		SQLFreeRes(res);
	}
	return 0;
}
int OSSigEOS()
{
	SQLRes res;
	SQLRow row;
	char *c;
	if ((res = SQLQuery("SELECT item,tipo,motivo,flags,accion from %s%s ", PREFIJO, OS_LINES)))
	{
		while ((row = SQLFetchRow(res)))
		{
			if (strchr(row[1], 'G')) { //Es Gline
				if ((c = strchr(row[0], '@')))
					*c++ = '\0';
				ProtFunc(P_GLINE)(CLI(operserv), ADD, row[0], c, 0, row[2]);
			}

			if (strchr(row[1], 'F')) //Es Spamfilter
				ProtFunc(P_SPAM)(CLI(operserv), ADD, row[0], row[3], row[4], row[2]);

		}
		SQLFreeRes(res);
	}
	return 0;
}
int OSSigSockClose()
{
	OSDescargaNoticias();
	return 0;
}
