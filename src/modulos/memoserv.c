/*
 * $Id: memoserv.c,v 1.10 2004-12-31 12:28:01 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#include "modulos/memoserv.h"

MemoServ memoserv;

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

int memoserv_away(Cliente *, char *);
int memoserv_join(Cliente *, Canal *);

int memoserv_idok	(Cliente *);
int memoserv_sig_mysql	();
static void memoserv_notifica	(Cliente *);
int memoserv_sig_drop	(char *);
int memoserv_sig_reg	(char *);
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
int test(Conf *, int *);

ModInfo info = {
	"MemoServ" ,
	0.7 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net"
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
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
			{
				irc_dlsym(ex->hmod, "tiene_nivel", tiene_nivel_dl);
				irc_dlsym(ex->hmod, "ChanReg", ChanReg_dl);
				break;
			}
		}
	}
#endif
	return errores;
}	
int descarga()
{
	borra_senyal(SIGN_AWAY, memoserv_away);
	borra_senyal(SIGN_JOIN, memoserv_join);
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
	return 0;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *ms;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "defecto"))
			memoserv.def = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "cada"))
			memoserv.cada = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ms = &memoserv_coms[0];
				while (ms->com != 0x0)
				{
					if (!strcasecmp(ms->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = ms;
						break;
					}
					ms++;
				}
				if (ms->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
	}
	inserta_senyal(SIGN_AWAY, memoserv_away);
	inserta_senyal(SIGN_JOIN, memoserv_join);
	inserta_senyal(NS_SIGN_IDOK, memoserv_idok);
	inserta_senyal(SIGN_MYSQL, memoserv_sig_mysql);
	inserta_senyal(NS_SIGN_DROP, memoserv_sig_drop);
	inserta_senyal(NS_SIGN_REG, memoserv_sig_reg);
	inserta_senyal(CS_SIGN_DROP, memoserv_sig_drop);
	inserta_senyal(CS_SIGN_REG, memoserv_sig_reg);
	bot_set(memoserv);
}
BOTFUNC(memoserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(memoserv), "\00312%s\003 se encarga de gestionar el servicio de mensajería.", memoserv.hmod->nick);
		response(cl, CLI(memoserv), "Este servicio permite enviar y recibir mensajes entre usuarios aunque no estén conectados simultáneamente.");
		response(cl, CLI(memoserv), "Además, permite mantener un historial de mensajes recibidos y enviar de nuevos a usuarios que se encuentren desconectados.");
		response(cl, CLI(memoserv), "Cuando se conecten, y según su configuración, recibirán un aviso de que tienen nuevos mensajes para leer.");
		response(cl, CLI(memoserv), "Los mensajes también pueden enviarse a un canal. Así, los usuarios que tengan el nivel suficiente podrán enviar y recibir mensajes para un determinado canal.");
		response(cl, CLI(memoserv), "Este servicio sólo es para usuarios y canales registrados.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Comandos disponibles:");
		response(cl, CLI(memoserv), "\00312SEND\003 Envía un mensaje.");
		response(cl, CLI(memoserv), "\00312READ\003 Lee un mensaje.");
		response(cl, CLI(memoserv), "\00312DEL\003 Borra un mensaje.");
		response(cl, CLI(memoserv), "\00312LIST\003 Lista los mensajes.");
		response(cl, CLI(memoserv), "\00312SET\003 Fija tus opciones.");
		response(cl, CLI(memoserv), "\00312INFO\003 Muestra información de tu configuración.");
		response(cl, CLI(memoserv), "\00312CANCELAR\003 Cancela el último mensaje.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Para más información, \00312/msg %s %s comando", memoserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SEND"))
	{
		response(cl, CLI(memoserv), "\00312SEND");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Envía un mensaje a un usuario o canal.");
		response(cl, CLI(memoserv), "El mensaje es guardado hasta que no se borra y puede leerse tantas veces se quiera.");
		response(cl, CLI(memoserv), "Cuando se deja un mensaje a un usuario, se le notificará de que tiene mensajes pendientes para leer.");
		response(cl, CLI(memoserv), "Si es a un canal, cada vez que entre un usuario y tenga nivel suficiente, se le notificará de que el canal tiene mensajes.");
		response(cl, CLI(memoserv), "Tanto el nick o canal destinatario deben estar registrados.");
		response(cl, CLI(memoserv), "Sólo pueden enviarse mensajes cada \00312%i\003 segundos.", memoserv.cada);
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312SEND nick|#canal mensaje");
	}
	else if (!strcasecmp(param[1], "READ"))
	{
		response(cl, CLI(memoserv), "\00312READ");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Lee un mensaje.");
		response(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer un LIST.");
		response(cl, CLI(memoserv), "Adicionalmente, puedes especificar NEW para leer todos los mensajes nuevos o LAST para leer el último.");
		response(cl, CLI(memoserv), "Un mensaje lo podrás leer siempre que quieras hasta que sea borrado.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312READ [#canal] nº|NEW|LAST");
	}
	else if (!strcasecmp(param[1], "DEL"))
	{
		response(cl, CLI(memoserv), "\00312DEL");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Borra un mensaje.");
		response(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer LIST.");
		response(cl, CLI(memoserv), "Adicionalmente, puedes especificar ALL para borrarlos todos a la vez.");
		response(cl, CLI(memoserv), "Recuerda que una vez borrado, no lo podrás recuperar.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintxis: \00312DEL [#canal] nº|ALL");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		response(cl, CLI(memoserv), "\00312LIST");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Lista todos los mensajes.");
		response(cl, CLI(memoserv), "Se detalla la fecha de emisión, el emisor y el número que ocupa.");
		response(cl, CLI(memoserv), "Además, si va precedido por un asterisco, marca que el mensaje es nuevo y todavía no se ha leído.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312LIST [#canal]");
	}
	else if (!strcasecmp(param[1], "INFO"))
	{
		response(cl, CLI(memoserv), "\00312INFO");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Muestra distinta información procedente de tus opciones.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312INFO [#canal]");
	}
	else if (!strcasecmp(param[1], "CANCELAR"))
	{
		response(cl, CLI(memoserv), "\00312CANCELAR");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Cancela el último mensaje que has enviado.");
		response(cl, CLI(memoserv), "Esto puede servir para cerciorarse de enviar un mensaje.");
		response(cl, CLI(memoserv), "Recuerda que si has enviado uno después, tendrás que cancelar ambos ya que se elimina primero el último.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312CANCELAR nick|#canal");
	}
	else if (!strcasecmp(param[1], "SET"))
	{
		response(cl, CLI(memoserv), "\00312SET");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Fija las distintas opciones.");
		response(cl, CLI(memoserv), " ");
		response(cl, CLI(memoserv), "Sintaxis: \00312SET [#canal] LIMITE nº");
		response(cl, CLI(memoserv), "Fija el nº máximo de mensajes que puede recibir.");
		response(cl, CLI(memoserv), "Sintaxis: \00312SET #canal NOTIFY ON|OFF");
		response(cl, CLI(memoserv), "Avisa si hay mensajes nuevos en el canal.");
		response(cl, CLI(memoserv), "Sintaxis: \00312SET NOTIFY {+|-}modos");
		response(cl, CLI(memoserv), "Fija distintos métodos de notificación de tus memos:");
		response(cl, CLI(memoserv), "-\00312l\003 Te notifica de mensajes nuevos al identificarte como propietario de tu nick.");
		response(cl, CLI(memoserv), "-\00312n\003 Te notifica de mensajes nuevos cuando te son enviados al instante.");
		response(cl, CLI(memoserv), "-\00312w\003 Te notifica de mensajes nuevos cuando vuevles del estado de away.");
		response(cl, CLI(memoserv), "NOTA: El tener el modo +w deshabilita los demás. Esto es así porque sólo se quiere ser informado cuando no se está away.");
	}
	else
		response(cl, CLI(memoserv), NS_ERR_EMPT, "Opción desconocida.");
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
		response(cl, CLI(memoserv), MS_ERR_PARA, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			response(cl, CLI(memoserv), MS_ERR_PARA, "READ #canal nº|LAST|NEW");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, CLI(memoserv), CS_ERR_FORB, "");
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
		response(cl, CLI(memoserv), MS_ERR_SNTX, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (!strcasecmp(no, "NEW"))
	{
		if ((res = _mysql_query("SELECT de,mensaje,fecha from %s%s where para='%s' AND leido='0'", PREFIJO, MS_MYSQL, para)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				tim = atol(row[2]);
				response(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
				response(cl, CLI(memoserv), "%s", row[1]);
				_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
			}
			mysql_free_result(res);
		}
		else
		{
			response(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes nuevos.");
			return 1;
		}
	}
	else if (!strcasecmp(no, "LAST"))
	{
		char *fech = NULL;
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
			response(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
			response(cl, CLI(memoserv), "%s", row[1]);
			_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
			mysql_free_result(res);
			Free(fech);
		}
		else
		{
			non:
			response(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
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
					response(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], _asctime(&tim));
					response(cl, CLI(memoserv), "%s", row[1]);
					_mysql_query("UPDATE %s%s SET leido=1 where para='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, para, row[1], row[0], row[2]);
					mysql_free_result(res);
					return 0;
				}
			}
			response(cl, CLI(memoserv), MS_ERR_EMPT, "Este mensaje no existe.");
			mysql_free_result(res);
			return 1;
		}
		else
		{
			response(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
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
		response(cl, CLI(memoserv), MS_ERR_PARA, "SEND nick|#canal mensaje");
		return 1;
	}
	if (*param[1] != '#' && !IsReg(param[1]))
	{
		response(cl, CLI(memoserv), MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	else if (*param[1] == '#')
	{
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
	}
	if (!IsOper(cl))
	{
		if ((res = _mysql_query("SELECT MAX(fecha) from %s%s where de='%s'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			row = mysql_fetch_row(res);
			if (row[0] && (atol(row[0]) + memoserv.cada) > time(0))
			{
				sprintf_irc(buf, "Sólo puedes enviar mensajes cada %i segundos.", memoserv.cada);
				response(cl, CLI(memoserv), MS_ERR_EMPT, buf);
				return 1;
			}
		}
	}
	memoserv_send(param[1], cl->nombre, implode(param, params, 2, -1));
	response(cl, CLI(memoserv), "Mensaje enviado a \00312%s\003.", param[1]);
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
		response(cl, CLI(memoserv), MS_ERR_PARA, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			response(cl, CLI(memoserv), MS_ERR_PARA, "DEL #canal nº|ALL");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, CLI(memoserv), CS_ERR_FORB, "");
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
		response(cl, CLI(memoserv), MS_ERR_SNTX, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (!strcasecmp(no, "ALL"))
	{
		_mysql_query("DELETE from %s%s where para='%s'", PREFIJO, MS_MYSQL, para);
		response(cl, CLI(memoserv), "Todos los mensajes han sido borrados.");
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
					response(cl, CLI(memoserv), "El mensaje \00312%s\003 ha sido borrado.", no);
					mysql_free_result(res);
					return 0;
				}
			}
			response(cl, CLI(memoserv), MS_ERR_EMPT, "No se encuentra el mensaje.");
			mysql_free_result(res);
		}
		else
			response(cl, CLI(memoserv), MS_ERR_EMPT, "No hay mensajes.");
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
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, CLI(memoserv), CS_ERR_FORB, "");
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
			response(cl, CLI(memoserv), "%s \00312%i\003 - \00312%s\003 - \00312%s\003", atoi(row[2]) ? "" : "*", i + 1, row[0], _asctime(&tim));
		}
		mysql_free_result(res);
	}
	else
		response(cl, CLI(memoserv), MS_ERR_EMPT, "No hay mensajes para listar.");
	return 0;
}
BOTFUNC(memoserv_set)
{
	char *para, *opt, *val;
	if (params < 3)
	{
		response(cl, CLI(memoserv), MS_ERR_PARA, "SET [#canal] opción valor");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 4)
		{
			response(cl, CLI(memoserv), MS_ERR_PARA, "SET #canal opción valor");
			return 1;
		}
		if (!IsChanReg_dl(param[1]))
		{
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!tiene_nivel_dl(cl->nombre, param[1], CS_LEV_MEM))
		{
			response(cl, CLI(memoserv), CS_ERR_FORB, "");
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
			response(cl, CLI(memoserv), MS_ERR_SNTX, "SET [#canal] limite nº");
			return 1;
		}
		if (abs(atoi(val)) > 30)
		{
			response(cl, CLI(memoserv), MS_ERR_EMPT, "El límite no puede superar los 30 mensajes.");
			return 1;
		}
		_mysql_add(MS_SET, para, "limite", val);
		response(cl, CLI(memoserv), "Límite cambiado a \00312%s\003.", val);
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
				response(cl, CLI(memoserv), MS_ERR_SNTX, "SET #canal NOTIFY ON|OFF");
				return 1;
			}
		}
		else
		{
			int opts = atoi(_mysql_get_registro(MS_SET, para, "opts"));
			char f = ADD, *modos = val;
			if (*val != '+' && *val != '-')
			{
				response(cl, CLI(memoserv), MS_ERR_SNTX, "SET NOTIFY +-modos");
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
			response(cl, CLI(memoserv), "Notify cambiado a \00312%s\003.", val);
		}
	}
	else
	{
		response(cl, CLI(memoserv), MS_ERR_EMPT, "Opción desconocida.");
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
			response(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if ((res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, param[1])))
		{
			memos = (int)mysql_num_rows(res);
			mysql_free_result(res);
		}
		opts = atoi(_mysql_get_registro(MS_SET, param[1], "opts"));
		response(cl, CLI(memoserv), "El canal \00312%s\003 tiene \00312%i\003 mensajes.", param[1], memos);
		response(cl, CLI(memoserv), "La notificación de memos está \002%s\002.", opts ? "ON" : "OFF");
		response(cl, CLI(memoserv), "El límite de mensajes está fijado a \00312%s\003.", _mysql_get_registro(MS_SET, param[1], "limite"));
	}
	else
	{
		int noleid = 0;
		if ((res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			memos = (int)mysql_num_rows(res);
			mysql_free_result(res);
		}
		if ((res = _mysql_query("SELECT * from %s%s where para='%s' AND leido='0'", PREFIJO, MS_MYSQL, cl->nombre)))
		{
			noleid = (int)mysql_num_rows(res);
			mysql_free_result(res);
		}
		opts = atoi(_mysql_get_registro(MS_SET, cl->nombre, "opts"));
		response(cl, CLI(memoserv), "Tienes \00312%i\003 mensajes sin leer de un total de \00312%i\003.", noleid, memos);
		response(cl, CLI(memoserv), "El límite de mensajes está fijado a \00312%s\003.", _mysql_get_registro(MS_SET, cl->nombre, "limite"));
		response(cl, CLI(memoserv), "La notificación de memos está a:");
		if (opts & MS_OPT_LOG)
			response(cl, CLI(memoserv), "-Al identificarte como propietario de tu nick.");
		if (opts & MS_OPT_AWY)
			response(cl, CLI(memoserv), "-Cuando regreses del away.");
		if (opts & MS_OPT_NEW)
			response(cl, CLI(memoserv), "-Cuando recibas un mensaje nuevo.");
	}	
	return 0;
}
BOTFUNC(memoserv_cancelar)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (params < 2)
	{
		response(cl, CLI(memoserv), MS_ERR_PARA, "CANCELAR nick|#canal");
		return 1;
	}
	if (*param[1] == '#' && !IsChanReg_dl(param[1]))
	{
		response(cl, CLI(memoserv), MS_ERR_EMPT, "Este canal no está registrado.");
		return 1;
	}
	else if (*param[1] != '#' && !IsReg(param[1]))
	{
		response(cl, CLI(memoserv), MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	if ((res = _mysql_query("SELECT MAX(fecha) from %s%s where de='%s' AND para='%s'", PREFIJO, MS_MYSQL, cl->nombre, param[1])))
	{
		row = mysql_fetch_row(res);
		if (!row[0])
			goto non;
		_mysql_query("DELETE from %s%s where para='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_MYSQL, param[1], cl->nombre, row[0]);
		mysql_free_result(res);
		response(cl, CLI(memoserv), "El último mensaje de \00312%s\003 ha sido eliminado.", param[1]);
	}
	else
	{
		non:
		buf[0] = '\0';
		sprintf_irc(buf, "No has enviado ningún mensaje a %s.", param[1]);
		response(cl, CLI(memoserv), MS_ERR_EMPT, buf);
		return 1;
	}
	return 0;
}
int memoserv_away(Cliente *cl, char *away)
{
	int opts;
	if (!away)
	{
		if (!IsId(cl))
			return 1;
		opts = atoi(_mysql_get_registro(MS_SET, cl->nombre, "opts"));
		if (opts & MS_OPT_AWY)
			memoserv_notifica(cl);
	}
	return 0;
}
int memoserv_join(Cliente *cl, Canal *cn)
{
	int opts;
	MYSQL_RES *res;
	if (!IsChanReg_dl(cn->nombre))
		return 1;
	if (!(res = _mysql_query("SELECT * from %s%s where para='%s'", PREFIJO, MS_MYSQL, cn->nombre)))
		return 1;
	mysql_free_result(res);
	opts = atoi(_mysql_get_registro(MS_SET, cn->nombre, "opts"));
	if (opts & MS_OPT_CNO)
	{
		if (tiene_nivel_dl(cl->nombre, cn->nombre, CS_LEV_MEM))
			port_func(P_NOTICE)(cl, CLI(memoserv), "Hay mensajes en el canal");
	}
	return 0;
}
int memoserv_send(char *para, char *de, char *mensaje)
{
	int opts = 0;
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
				port_func(P_NOTICE)((Cliente *)busca_canal(para, NULL), CLI(memoserv), "Hay un mensaje nuevo en el canal");
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
	if (!_mysql_existe_tabla(MS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`mensaje` text NOT NULL, "
  			"`para` text NOT NULL, "
  			"`de` text NOT NULL, "
  			"`fecha` bigint(20) NOT NULL default '0', "
  			"`leido` int(11) NOT NULL default '0', "
  			"KEY `n` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de mensajes';", PREFIJO, MS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, MS_MYSQL);
	}
	if (!_mysql_existe_tabla(MS_SET))
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`opts` varchar(255) default NULL, "
  			"`limite` int(11) NOT NULL default '%i', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de opciones de mensajes';", PREFIJO, MS_SET, memoserv.def))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, MS_SET);
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
		response(al, CLI(memoserv), "Tienes \00312%i\003 mensaje(s) nuevo(s).", mysql_num_rows(res));
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
