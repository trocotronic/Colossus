/*
 * $Id: memoserv.c,v 1.4 2004-09-16 21:18:22 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#include "nickserv.h"
#include "chanserv.h"
#include "memoserv.h"

MemoServ *memoserv = NULL;

static int memoserv_help	(Cliente *, char *[], int, char *[], int);
static int memoserv_memo	(Cliente *, char *[], int, char *[], int);
static int memoserv_read	(Cliente *, char *[], int, char *[], int);
static int memoserv_del		(Cliente *, char *[], int, char *[], int);
static int memoserv_list	(Cliente *, char *[], int, char *[], int);
static int memoserv_set		(Cliente *, char *[], int, char *[], int);
static int memoserv_info	(Cliente *, char *[], int, char *[], int);
static int memoserv_cancelar	(Cliente *, char *[], int, char *[], int);

static bCom memoserv_coms[] = {
	{ "help" , memoserv_help , TODOS } ,
	{ "send" , memoserv_memo , USERS } ,
	{ "read" , memoserv_read , USERS } ,
	{ "del" , memoserv_del , USERS } ,
	{ "list" , memoserv_list , USERS } ,
	{ "set" , memoserv_set , USERS } ,
	{ "info" , memoserv_info , USERS } ,
	{ "cancelar" , memoserv_cancelar , USERS } ,
	{ 0x0 , 0x0 , 0x0 }
};

DLLFUNC IRCFUNC(memoserv_away);
DLLFUNC IRCFUNC(memoserv_join);

DLLFUNC int memoserv_idok	(Cliente *);
DLLFUNC int memoserv_sig_mysql	();
static void memoserv_notifica	(Cliente *);
DLLFUNC int memoserv_sig_drop	(char *);
DLLFUNC int memoserv_sig_reg	(char *);
#ifndef _WIN32
u_long (*tiene_nivel_dl)(char *, char *, u_long);
int (*ChanReg_dl)(char *);
#else
#define tiene_nivel_dl tiene_nivel
#define ChanReg_dl ChanReg
#endif
#define IsChanReg_dl(x) (ChanReg_dl(x) == 1)
#define IsChanPReg_dl(x) (ChanReg_dl(x) == 2)
void set(Conf *, Modulo *);
DLLFUNC ModInfo info = {
	"MemoServ" ,
	0.5 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
};
	
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	char *file, *k;
	file = (char *)Malloc(sizeof(char) * (strlen(mod->archivo) + 5));
	strcpy(file, mod->archivo);
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
			{
				irc_dlsym(ex->hmod, "tiene_nivel", (u_long) tiene_nivel_dl);
				irc_dlsym(ex->hmod, "ChanReg", (int) ChanReg_dl);
				break;
			}
		}
	}
#endif
	return errores;
}	
DLLFUNC int descarga()
{
	borra_comando(MSG_AWAY, memoserv_away);
	borra_comando(MSG_JOIN, memoserv_join);
	borra_senyal(NS_SIGN_IDOK, memoserv_idok);
	borra_senyal(SIGN_MYSQL, memoserv_sig_mysql);
	borra_senyal(NS_SIGN_DROP, memoserv_sig_drop);
	borra_senyal(NS_SIGN_REG, memoserv_sig_reg);
	borra_senyal(CS_SIGN_DROP, memoserv_sig_drop);
	borra_senyal(CS_SIGN_REG, memoserv_sig_reg);
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
	bCom *ms;
	if (!memoserv)
		da_Malloc(memoserv, MemoServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&memoserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&memoserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&memoserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&memoserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&memoserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "defecto"))
			memoserv->def = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "cada"))
			memoserv->cada = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ms = &memoserv_coms[0];
				while (ms->com != 0x0)
				{
					if (!strcasecmp(ms->com, config->seccion[i]->seccion[p]->item))
					{
						memoserv->comando[memoserv->comandos++] = ms;
						break;
					}
					ms++;
				}
				if (ms->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&memoserv->residente, config->seccion[i]->data);
	}
	if (memoserv->mascara)
		Free(memoserv->mascara);
	memoserv->mascara = (char *)Malloc(sizeof(char) * (strlen(memoserv->nick) + 1 + strlen(memoserv->ident) + 1 + strlen(memoserv->host) + 1));
	sprintf_irc(memoserv->mascara, "%s!%s@%s", memoserv->nick, memoserv->ident, memoserv->host);
	inserta_comando(MSG_AWAY, TOK_AWAY, memoserv_away, INI);
	inserta_comando(MSG_JOIN, TOK_JOIN, memoserv_join, INI);
	inserta_senyal(NS_SIGN_IDOK, memoserv_idok);
	inserta_senyal(SIGN_MYSQL, memoserv_sig_mysql);
	inserta_senyal(NS_SIGN_DROP, memoserv_sig_drop);
	inserta_senyal(NS_SIGN_REG, memoserv_sig_reg);
	inserta_senyal(CS_SIGN_DROP, memoserv_sig_drop);
	inserta_senyal(CS_SIGN_REG, memoserv_sig_reg);
	mod->nick = memoserv->nick;
	mod->ident = memoserv->ident;
	mod->host = memoserv->host;
	mod->realname = memoserv->realname;
	mod->residente = memoserv->residente;
	mod->modos = memoserv->modos;
	mod->comandos = memoserv->comando;
}
BOTFUNC(memoserv_help)
{
	if (params < 2)
	{
		response(cl, memoserv->nick, "\00312%s\003 se encarga de gestionar el servicio de mensajería.", memoserv->nick);
		response(cl, memoserv->nick, "Este servicio permite enviar y recibir mensajes entre usuarios aunque no estén conectados simultáneamente.");
		response(cl, memoserv->nick, "Además, permite mantener un historial de mensajes recibidos y enviar de nuevos a usuarios que se encuentren desconectados.");
		response(cl, memoserv->nick, "Cuando se conecten, y según su configuración, recibirán un aviso de que tienen nuevos mensajes para leer.");
		response(cl, memoserv->nick, "Los mensajes también pueden enviarse a un canal. Así, los usuarios que tengan el nivel suficiente podrán enviar y recibir mensajes para un determinado canal.");
		response(cl, memoserv->nick, "Este servicio sólo es para usuarios y canales registrados.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Comandos disponibles:");
		response(cl, memoserv->nick, "\00312SEND\003 Envía un mensaje.");
		response(cl, memoserv->nick, "\00312READ\003 Lee un mensaje.");
		response(cl, memoserv->nick, "\00312DEL\003 Borra un mensaje.");
		response(cl, memoserv->nick, "\00312LIST\003 Lista los mensajes.");
		response(cl, memoserv->nick, "\00312SET\003 Fija tus opciones.");
		response(cl, memoserv->nick, "\00312INFO\003 Muestra información de tu configuración.");
		response(cl, memoserv->nick, "\00312CANCELAR\003 Cancela el último mensaje.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Para más información, \00312/msg %s %s comando", memoserv->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SEND"))
	{
		response(cl, memoserv->nick, "\00312SEND");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Envía un mensaje a un usuario o canal.");
		response(cl, memoserv->nick, "El mensaje es guardado hasta que no se borra y puede leerse tantas veces se quiera.");
		response(cl, memoserv->nick, "Cuando se deja un mensaje a un usuario, se le notificará de que tiene mensajes pendientes para leer.");
		response(cl, memoserv->nick, "Si es a un canal, cada vez que entre un usuario y tenga nivel suficiente, se le notificará de que el canal tiene mensajes.");
		response(cl, memoserv->nick, "Tanto el nick o canal destinatario deben estar registrados.");
		response(cl, memoserv->nick, "Sólo pueden enviarse mensajes cada \00312%i\003 segundos.", memoserv->cada);
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312SEND nick|#canal mensaje");
	}
	else if (!strcasecmp(param[1], "READ"))
	{
		response(cl, memoserv->nick, "\00312READ");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Lee un mensaje.");
		response(cl, memoserv->nick, "El número de mensaje es el mismo que aparece antepuesto al hacer un LIST.");
		response(cl, memoserv->nick, "Adicionalmente, puedes especificar NEW para leer todos los mensajes nuevos o LAST para leer el último.");
		response(cl, memoserv->nick, "Un mensaje lo podrás leer siempre que quieras hasta que sea borrado.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312READ [#canal] nº|NEW|LAST");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		response(cl, memoserv->nick, "\00312DEL");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Borra un mensaje.");
		response(cl, memoserv->nick, "El número de mensaje es el mismo que aparece antepuesto al hacer LIST.");
		response(cl, memoserv->nick, "Adicionalmente, puedes especificar ALL para borrarlos todos a la vez.");
		response(cl, memoserv->nick, "Recuerda que una vez borrado, no lo podrás recuperar.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintxis: \00312DEL [#canal] nº|ALL");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		response(cl, memoserv->nick, "\00312LIST");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Lista todos los mensajes.");
		response(cl, memoserv->nick, "Se detalla la fecha de emisión, el emisor y el número que ocupa.");
		response(cl, memoserv->nick, "Además, si va precedido por un asterisco, marca que el mensaje es nuevo y todavía no se ha leído.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312LIST [#canal]");
	}
	else if (!strcasecmp(param[1], "INFO"))
	{
		response(cl, memoserv->nick, "\00312INFO");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Muestra distinta información procedente de tus opciones.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312INFO [#canal]");
	}
	else if (!strcasecmp(param[1], "CANCELAR"))
	{
		response(cl, memoserv->nick, "\00312CANCELAR");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Cancela el último mensaje que has enviado.");
		response(cl, memoserv->nick, "Esto puede servir para cerciorarse de enviar un mensaje.");
		response(cl, memoserv->nick, "Recuerda que si has enviado uno después, tendrás que cancelar ambos ya que se elimina primero el último.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312CANCELAR nick|#canal");
	}
	else if (!strcasecmp(param[1], "SET"))
	{
		response(cl, memoserv->nick, "\00312SET");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Fija las distintas opciones.");
		response(cl, memoserv->nick, " ");
		response(cl, memoserv->nick, "Sintaxis: \00312SET [#canal] LIMITE nº");
		response(cl, memoserv->nick, "Fija el nº máximo de mensajes que puede recibir.");
		response(cl, memoserv->nick, "Sintaxis: \00312SET #canal NOTIFY ON|OFF");
		response(cl, memoserv->nick, "Avisa si hay mensajes nuevos en el canal.");
		response(cl, memoserv->nick, "Sintaxis: \00312SET NOTIFY {+|-}modos");
		response(cl, memoserv->nick, "Fija distintos métodos de notificación de tus memos:");
		response(cl, memoserv->nick, "-\00312l\003 Te notifica de mensajes nuevos al identificarte como propietario de tu nick.");
		response(cl, memoserv->nick, "-\00312n\003 Te notifica de mensajes nuevos cuando te son enviados al instante.");
		response(cl, memoserv->nick, "-\00312w\003 Te notifica de mensajes nuevos cuando vuevles del estado de away.");
		response(cl, memoserv->nick, "NOTA: El tener el modo +w deshabilita los demás. Esto es así porque sólo se quiere ser informado cuando no se está away.");
	}
	else
		response(cl, memoserv->nick, NS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(memoserv_read)
{
	int i;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *para, *no;
	time_t tim;
	if (params < 2)
	{
		response(cl, memoserv->nick, MS_ERR_PARA, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			response(cl, memoserv->nick, MS_ERR_PARA, "READ #canal nº|LAST|NEW");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, memoserv->nick, CS_ERR_FORB, "");
			return 1;
		}
		para = param[1];
		no = param[2];
	}
	else
	{
		para = cl->nombre;
		no = param[1];
	}
	if (!atoi(no) && strcasecmp(no, "LAST") && strcasecmp(no, "NEW"))
	{
		response(cl, memoserv->nick, MS_ERR_SNTX, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (!strcasecmp(no, "NEW"))
	{
		if ((res = _mysql_query("SELECT de,mensaje,fecha from %s%s where para='%s' AND leido='0'", PREFIJO, MS_MYSQL, para)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				tim = atol(row[2]);
				response(cl, memoserv->nick, "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
				response(cl, memoserv->nick, "%s", row[1]);
				_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
			}
			mysql_free_result(res);
		}
		else
		{
			response(cl, memoserv->nick, MS_ERR_EMPT, "No tienes mensajes nuevos.");
			return 1;
		}
	}
	else if (!strcasecmp(no, "LAST"))
	{
		char *fech;
		if ((res = _mysql_query("SELECT MAX(fecha) FROM %s%s where para='%s'", PREFIJO, MS_MYSQL, para)))
		{
			row = mysql_fetch_row(res);
			if (!row[0])
				goto non;
			fech = strdup(row[0]);
			mysql_free_result(res);
		}
		if ((res = _mysql_query("SELECT de,mensaje,fecha from %s%s where para='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, fech)))
		{
			row = mysql_fetch_row(res);
			tim = atol(row[2]);
			response(cl, memoserv->nick, "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
			response(cl, memoserv->nick, "%s", row[1]);
			_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
			mysql_free_result(res);
			Free(fech);
		}
		else
		{
			non:
			response(cl, memoserv->nick, MS_ERR_EMPT, "No tienes mensajes.");
			return 1;
		}
	}
	else
	{
		int max = atoi(no);
		if ((res = _mysql_query("SELECT de,mensaje,fecha from %s%s where para='%s'", PREFIJO, MS_MYSQL, para)))
		{
			for (i = 0; (row = mysql_fetch_row(res)); i++)
			{
				if ((i + 1) == max) /* hay memo */
				{
					tim = atol(row[2]);
					response(cl, memoserv->nick, "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
					response(cl, memoserv->nick, "%s", row[1]);
					_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
					mysql_free_result(res);
					return 0;
				}
			}
			response(cl, memoserv->nick, MS_ERR_EMPT, "Este mensaje no existe.");
			mysql_free_result(res);
			return 1;
		}
		else
		{
			response(cl, memoserv->nick, MS_ERR_EMPT, "No tienes mensajes.");
			return 1;
		}
	}
	return 0;
}
BOTFUNC(memoserv_memo)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (params < 3)
	{
		response(cl, memoserv->nick, MS_ERR_PARA, "SEND nick|#canal mensaje");
		return 1;
	}
	if (*param[1] != '#' && !IsReg(param[1]))
	{
		response(cl, memoserv->nick, MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	else if (*param[1] == '#')
	{
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, memoserv->nick, CS_ERR_FORB, "");
			return 1;
		}
	}
	if (!IsOper(cl))
	{
		if ((res = _mysql_query("SELECT MAX(fecha) from %s%s where de='%s'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			row = mysql_fetch_row(res);
			if (!row[0])
				goto sig;
			if ((atol(row[0]) + memoserv->cada) > time(0))
			{
				buf[0] = '\0';
				sprintf_irc(buf, "Sólo puedes enviar mensajes cada %i segundos.", memoserv->cada);
				response(cl, memoserv->nick, MS_ERR_EMPT, buf);
				return 1;
			}
		}
	}
	sig:
	memoserv_send(param[1], cl->nombre, implode(param, params, 2, -1));
	response(cl, memoserv->nick, "Mensaje enviado a \00312%s\003.", param[1]);
	return 0;
}
BOTFUNC(memoserv_del)
{
	int i;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *para, *no;
	if (params < 2)
	{
		response(cl, memoserv->nick, MS_ERR_PARA, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			response(cl, memoserv->nick, MS_ERR_PARA, "DEL #canal nº|ALL");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, memoserv->nick, CS_ERR_FORB, "");
			return 1;
		}
		para = param[1];
		no = param[2];
	}
	else
	{
		para = cl->nombre;
		no = param[1];
	}
	if (!atoi(no) && strcasecmp(no, "ALL"))
	{
		response(cl, memoserv->nick, MS_ERR_SNTX, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (!strcasecmp(no, "ALL"))
	{
		_mysql_query("DELETE from %s%s where para='%s'", PREFIJO, MS_MYSQL, para);
		response(cl, memoserv->nick, "Todos los mensajes han sido borrados.");
	}
	else
	{
		int max = atoi(no);
		if ((res = _mysql_query("SELECT de,mensaje,fecha from %s%s where para='%s'", PREFIJO, MS_MYSQL, para)))
		{
			for(i = 0; (row = mysql_fetch_row(res)); i++)
			{
				if (max == (i + 1))
				{
					_mysql_query("DELETE from %s%s where para='%s' AND de='%s' AND mensaje='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[0], row[1], row[2]);
					response(cl, memoserv->nick, "El mensaje \00312%s\003 ha sido borrado.", no);
					mysql_free_result(res);
					return 0;
				}
			}
			response(cl, memoserv->nick, MS_ERR_EMPT, "No se encuentra el mensaje.");
			mysql_free_result(res);
		}
		else
			response(cl, memoserv->nick, MS_ERR_EMPT, "No hay mensajes.");
	}
	return 0;
}
BOTFUNC(memoserv_list)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *tar;
	int i;
	time_t tim;
	if (params > 1 && *param[1] == '#')
	{
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, memoserv->nick, CS_ERR_FORB, "");
			return 1;
		}
		tar = param[1];
	}
	else
		tar = cl->nombre;
	if ((res = _mysql_query("SELECT de,fecha,leido from %s%s where para='%s'", PREFIJO, MS_MYSQL, tar)))
	{
		for (i = 0; (row = mysql_fetch_row(res)); i++)
		{
			tim = atol(row[1]);
			response(cl, memoserv->nick, "%s \00312%i\003 - \00312%s\003 - \00312%s\003", atoi(row[2]) ? "" : "*", i + 1, row[0], _asctime(&tim));
		}
		mysql_free_result(res);
	}
	else
		response(cl, memoserv->nick, MS_ERR_EMPT, "No hay mensajes para listar.");
	return 0;
}
BOTFUNC(memoserv_set)
{
	char *para, *opt, *val;
	if (params < 3)
	{
		response(cl, memoserv->nick, MS_ERR_PARA, "SET [#canal] opción valor");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 4)
		{
			response(cl, memoserv->nick, MS_ERR_PARA, "SET #canal opción valor");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, memoserv->nick, CS_ERR_FORB, "");
			return 1;
		}
		para = param[1];
		opt = param[2];
		val = param[3];
	}
	else
	{
		para = cl->nombre;
		opt = param[1];
		val = param[2];
	}
	if (!strcasecmp(opt, "LIMITE"))
	{
		if (!atoi(val))
		{
			response(cl, memoserv->nick, MS_ERR_SNTX, "SET [#canal] limite nº");
			return 1;
		}
		if (abs(atoi(val)) > 30)
		{
			response(cl, memoserv->nick, MS_ERR_EMPT, "El límite no puede superar los 30 mensajes.");
			return 1;
		}
		_mysql_add(MS_SET, para, "limite", val);
		response(cl, memoserv->nick, "Límite cambiado a \00312%s\003.", val);
	}
	else if (!strcasecmp(opt, "NOTIFY"))
	{
		if (*para == '#')
		{
			if (!strcasecmp(val, "ON"))
				_mysql_add(MS_SET, para, "opts", "%i", MS_OPT_CNO);
			else if (!strcasecmp(val, "OFF"))
				_mysql_add(MS_SET, para, "opts", "0");
			else
			{
				response(cl, memoserv->nick, MS_ERR_SNTX, "SET #canal NOTIFY ON|OFF");
				return 1;
			}
		}
		else
		{
			int opts = atoi(_mysql_get_registro(MS_SET, para, "opts"));
			char f = ADD, *modos = val;
			if (*val != '+' && *val != '-')
			{
				response(cl, memoserv->nick, MS_ERR_SNTX, "SET NOTIFY +-modos");
				return 1;
			}
			while (!BadPtr(modos))
			{
				switch(*modos)
				{
					case '+':
						f = ADD;
						break;
					case '-':
						f = DEL;
						break;
					case 'l':
						if (f == ADD)
							opts |= MS_OPT_LOG;
						else
							opts &= ~MS_OPT_LOG;
						break;
					case 'w':
						if (f == ADD)
							opts |= MS_OPT_AWY;
						else
							opts &= ~MS_OPT_AWY;
						break;
					case 'n':
						if (f == ADD)
							opts |= MS_OPT_NEW;
						else
							opts &= ~MS_OPT_NEW;
						break;
				}
				modos++;
			}
			_mysql_add(MS_SET, para, "opts", "%i", opts);
			response(cl, memoserv->nick, "Notify cambiado a \00312%s\003.", val);
		}
	}
	else
	{
		response(cl, memoserv->nick, MS_ERR_EMPT, "Opción desconocida.");
		return 1;
	}
	return 0;
}
BOTFUNC(memoserv_info)
{
	MYSQL_RES *res;
	int memos = 0, opts = 0;
	if (params > 1 && *param[1] == '#')
	{
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, memoserv->nick, MS_ERR_NOTR, "");
			return 1;
		}
		if ((res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, param[1])))
		{
			memos = mysql_num_rows(res);
			mysql_free_result(res);
		}
		opts = atoi(_mysql_get_registro(MS_SET, param[1], "opts"));
		response(cl, memoserv->nick, "El canal \00312%s\003 tiene \00312%i\003 mensajes.", param[1], memos);
		response(cl, memoserv->nick, "La notificación de memos está \002%s\002.", opts ? "ON" : "OFF");
		response(cl, memoserv->nick, "El límite de mensajes está fijado a \00312%s\003.", _mysql_get_registro(MS_SET, param[1], "limite"));
	}
	else
	{
		int noleid = 0;
		if ((res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			memos = mysql_num_rows(res);
			mysql_free_result(res);
		}
		if ((res = _mysql_query("SELECT * from %s%s where para='%s' AND leido='0'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			noleid = mysql_num_rows(res);
			mysql_free_result(res);
		}
		opts = atoi(_mysql_get_registro(MS_SET, cl->nombre, "opts"));
		response(cl, memoserv->nick, "Tienes \00312%i\003 mensajes sin leer de un total de \00312%i\003.", noleid, memos);
		response(cl, memoserv->nick, "El límite de mensajes está fijado a \00312%s\003.", _mysql_get_registro(MS_SET, cl->nombre, "limite"));
		response(cl, memoserv->nick, "La notificación de memos está a:");
		if (opts & MS_OPT_LOG)
			response(cl, memoserv->nick, "-Al identificarte como propietario de tu nick.");
		if (opts & MS_OPT_AWY)
			response(cl, memoserv->nick, "-Cuando regreses del away.");
		if (opts & MS_OPT_NEW)
			response(cl, memoserv->nick, "-Cuando recibas un mensaje nuevo.");
	}	
	return 0;
}
BOTFUNC(memoserv_cancelar)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (params < 2)
	{
		response(cl, memoserv->nick, MS_ERR_PARA, "CANCELAR nick|#canal");
		return 1;
	}
	if (*param[1] == '#' && !IsChanReg_dl(param[1]))
	{
		response(cl, memoserv->nick, MS_ERR_EMPT, "Este canal no está registrado.");
		return 1;
	}
	else if (*param[1] != '#' && !IsReg(param[1]))
	{
		response(cl, memoserv->nick, MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	if ((res = _mysql_query("SELECT MAX(fecha) from %s%s where de='%s' AND para='%s'", PREFIJO, MS_MYSQL, cl->nombre, param[1])))
	{
		row = mysql_fetch_row(res);
		if (!row[0])
			goto non;
		_mysql_query("DELETE from %s%s where para='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, param[1], cl->nombre, row[0]);
		mysql_free_result(res);
		response(cl, memoserv->nick, "El último mensaje de \00312%s\003 ha sido eliminado.", param[1]);
	}
	else
	{
		non:
		buf[0] = '\0';
		sprintf_irc(buf, "No has enviado ningún mensaje a %s.", param[1]);
		response(cl, memoserv->nick, MS_ERR_EMPT, buf);
		return 1;
	}
	return 0;
}
IRCFUNC(memoserv_away)
{
	int opts;
	if (parc == 1)
	{
		if (!IsId(cl))
			return 1;
		opts = atoi(_mysql_get_registro(MS_SET, cl->nombre, "opts"));
		if (opts & MS_OPT_AWY)
			memoserv_notifica(cl);
	}
	return 0;
}
IRCFUNC(memoserv_join)
{
	char *canal;
	int opts;
	MYSQL_RES *res;
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
	{
		if (!IsChanReg_dl(canal))
			continue;
		if (!(res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, canal)))
			continue;
		mysql_free_result(res);
		opts = atoi(_mysql_get_registro(MS_SET, canal, "opts"));
		if (opts & MS_OPT_CNO)
		{
			
			if (tiene_nivel_dl(parv[0], canal, CS_LEV_MEM))
				sendto_serv(":%s %s %s :Hay mensajes en el canal", memoserv->nick, TOK_NOTICE, parv[0]);
		}
	}
	return 0;
}
DLLFUNC int memoserv_send(char *para, char *de, char *mensaje)
{
	int opts = 0;
	MYSQL_RES *res;
	Cliente *al;
	if (para && de && mensaje)
	{
		if (!IsChanReg_dl(para) && !IsReg(para))
			return 1;
		_mysql_query("INSERT into %s%s (para,de,fecha,mensaje) values ('%s','%s','%lu','%s')", PREFIJO, MS_MYSQL, para, de, time(0), mensaje);
		opts = atoi(_mysql_get_registro(MS_SET, para, "opts"));
		if (*para == '#')
		{
			if (opts & MS_OPT_CNO)
				sendto_serv(":%s %s @%s :Hay un mensaje nuevo en el canal", memoserv->nick, TOK_NOTICE, para);
		}
		else
		{
			if (opts & MS_OPT_NEW)
			{
				al = busca_cliente(para, NULL);
				if (al && IsId(al))
				{
					if (!(al->nivel & AWAY) || !(opts & MS_OPT_AWY))
							memoserv_notifica(al);
				}	
			}
		}
		return 0;
	}
	return -1;
}
int memoserv_idok(Cliente *al)
{
	int opts;
	char *reg;
	if ((reg = _mysql_get_registro(MS_SET, al->nombre, "opts")))
	{
		opts = atoi(reg);
		if ((opts & MS_OPT_LOG) && (!(al->nivel & AWAY) || !(opts & MS_OPT_AWY)))
			memoserv_notifica(al);
	}
	return 0;
}
int memoserv_sig_mysql()
{
	int i;
	char buf[2][BUFSIZE], tabla[2];
	tabla[0] = tabla[1] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, MS_MYSQL);
	sprintf_irc(buf[1], "%s%s", PREFIJO, MS_SET);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
		else if (!strcasecmp(mysql_tablas.tabla[i], buf[1]))
			tabla[1] = 1;
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`mensaje` text NOT NULL, "
  			"`para` text NOT NULL, "
  			"`de` text NOT NULL, "
  			"`fecha` bigint(20) NOT NULL default '0', "
  			"`leido` int(11) NOT NULL default '0', "
  			"KEY `n` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de mensajes';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
	if (!tabla[1])
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`opts` varchar(255) default NULL, "
  			"`limite` int(11) NOT NULL default '%i', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de opciones de mensajes';", buf[1], memoserv->def))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[1]);
		if ((res = _mysql_query("SELECT item from %s%s", PREFIJO, NS_MYSQL)))
		{
			while ((row = mysql_fetch_row(res)))
				_mysql_add(MS_SET, row[0], "opts", "%i", MS_OPT_ALL);
			mysql_free_result(res);
		}
		if ((res = _mysql_query("SELECT item from %s%s", PREFIJO, CS_MYSQL)))
		{
			while ((row = mysql_fetch_row(res)))
				_mysql_add(MS_SET, row[0], "opts", "%i", MS_OPT_ALL);
			mysql_free_result(res);
		}
	}
	_mysql_carga_tablas();
	return 0;
}
void memoserv_notifica(Cliente *al)
{
	MYSQL_RES *res;
	if ((res = _mysql_query("SELECT * from %s%s where para='%s' AND leido='0'", PREFIJO, MS_MYSQL, al->nombre)))
	{
		response(al, memoserv->nick, "Tienes \00312%i\003 mensaje(s) nuevo(s).", mysql_num_rows(res));
		mysql_free_result(res);
	}
	return;
}
int memoserv_sig_drop(char *nick)
{
	_mysql_del(MS_SET, nick);
	return 0;
}
int memoserv_sig_reg(char *nick)
{
	_mysql_add(MS_SET, nick, "opts", "%i", MS_OPT_ALL);
	return 0;
}
