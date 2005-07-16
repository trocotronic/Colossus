/*
 * $Id: memoserv.c,v 1.17 2005-07-16 15:25:31 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#include "modulos/memoserv.h"

MemoServ memoserv;
#define ExFunc(x) BuscaFuncion(memoserv.hmod, x, NULL)

BOTFUNC(MSHelp);
BOTFUNC(MSMemo);
BOTFUNC(MSRead);
BOTFUNC(MSDel);
BOTFUNC(MSList);
BOTFUNC(MSOpts);
BOTFUNC(MSInfo);
BOTFUNC(MSCancela);

static bCom memoserv_coms[] = {
	{ "help" , MSHelp , TODOS } ,
	{ "send" , MSMemo , USERS } ,
	{ "read" , MSRead , USERS } ,
	{ "del" , MSDel , USERS } ,
	{ "list" , MSList , USERS } ,
	{ "set" , MSOpts , USERS } ,
	{ "info" , MSInfo , USERS } ,
	{ "cancelar" , MSCancela , USERS } ,
	{ 0x0 , 0x0 , 0x0 }
};

int MSCmdAway(Cliente *, char *);
int MSCmdJoin(Cliente *, Canal *);

int MSSigIdOk	(Cliente *);
int MSSigSQL	();
static void MSNotifica	(Cliente *);
int MSSigDrop	(char *);
int MSSigRegistra	(char *);
void MSSet(Conf *, Modulo *);
int MSTest(Conf *, int *);

ModInfo MOD_INFO(MemoServ) = {
	"MemoServ" ,
	0.8 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int MOD_CARGA(MemoServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, MOD_INFO(MemoServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(MemoServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(MemoServ).nombre))
			{
				if (!MSTest(modulo.seccion[0], &errores))
					MSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(MemoServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(MemoServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	return errores;
}	
int descarga()
{
	BorraSenyal(SIGN_AWAY, MSCmdAway);
	BorraSenyal(SIGN_JOIN, MSCmdJoin);
	BorraSenyal(NS_SIGN_IDOK, MSSigIdOk);
	BorraSenyal(SIGN_SQL, MSSigSQL);
	BorraSenyal(NS_SIGN_DROP, MSSigDrop);
	BorraSenyal(NS_SIGN_REG, MSSigRegistra);
	BorraSenyal(CS_SIGN_DROP, MSSigDrop);
	BorraSenyal(CS_SIGN_REG, MSSigRegistra);
	BotUnset(memoserv);
	return 0;
}
int MSTest(Conf *config, int *errores)
{
	return 0;
}
void MSSet(Conf *config, Modulo *mod)
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
					Error("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
	}
	InsertaSenyal(SIGN_AWAY, MSCmdAway);
	InsertaSenyal(SIGN_JOIN, MSCmdJoin);
	InsertaSenyal(NS_SIGN_IDOK, MSSigIdOk);
	InsertaSenyal(SIGN_SQL, MSSigSQL);
	InsertaSenyal(NS_SIGN_DROP, MSSigDrop);
	InsertaSenyal(NS_SIGN_REG, MSSigRegistra);
	InsertaSenyal(CS_SIGN_DROP, MSSigDrop);
	InsertaSenyal(CS_SIGN_REG, MSSigRegistra);
	BotSet(memoserv);
}
BOTFUNC(MSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), "\00312%s\003 se encarga de gestionar el servicio de mensajería.", memoserv.hmod->nick);
		Responde(cl, CLI(memoserv), "Este servicio permite enviar y recibir mensajes entre usuarios aunque no estén conectados simultáneamente.");
		Responde(cl, CLI(memoserv), "Además, permite mantener un historial de mensajes recibidos y enviar de nuevos a usuarios que se encuentren desconectados.");
		Responde(cl, CLI(memoserv), "Cuando se conecten, y según su configuración, recibirán un aviso de que tienen nuevos mensajes para leer.");
		Responde(cl, CLI(memoserv), "Los mensajes también pueden enviarse a un canal. Así, los usuarios que tengan el nivel suficiente podrán enviar y recibir mensajes para un determinado canal.");
		Responde(cl, CLI(memoserv), "Este servicio sólo es para usuarios y canales registrados.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Comandos disponibles:");
		FuncResp(memoserv, "SEND", "Envía un mensaje.");
		FuncResp(memoserv, "READ", "Lee un mensaje.");
		FuncResp(memoserv, "DEL", "Borra un mensaje.");
		FuncResp(memoserv, "LIST", "Lista los mensajes.");
		FuncResp(memoserv, "SET", "Fija tus opciones.");
		FuncResp(memoserv, "INFO", "Muestra información de tu configuración.");
		FuncResp(memoserv, "CANCELAR", "Cancela el último mensaje.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Para más información, \00312/msg %s %s comando", memoserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SEND") && ExFunc("SEND"))
	{
		Responde(cl, CLI(memoserv), "\00312SEND");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Envía un mensaje a un usuario o canal.");
		Responde(cl, CLI(memoserv), "El mensaje es guardado hasta que no se borra y puede leerse tantas veces se quiera.");
		Responde(cl, CLI(memoserv), "Cuando se deja un mensaje a un usuario, se le notificará de que tiene mensajes pendientes para leer.");
		Responde(cl, CLI(memoserv), "Si es a un canal, cada vez que entre un usuario y tenga nivel suficiente, se le notificará de que el canal tiene mensajes.");
		Responde(cl, CLI(memoserv), "Tanto el nick o canal destinatario deben estar registrados.");
		Responde(cl, CLI(memoserv), "Sólo pueden enviarse mensajes cada \00312%i\003 segundos.", memoserv.cada);
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312SEND nick|#canal mensaje");
	}
	else if (!strcasecmp(param[1], "READ") && ExFunc("READ"))
	{
		Responde(cl, CLI(memoserv), "\00312READ");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Lee un mensaje.");
		Responde(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer un LIST.");
		Responde(cl, CLI(memoserv), "Adicionalmente, puedes especificar NEW para leer todos los mensajes nuevos o LAST para leer el último.");
		Responde(cl, CLI(memoserv), "Un mensaje lo podrás leer siempre que quieras hasta que sea borrado.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312READ [#canal] nº|NEW|LAST");
	}
	else if (!strcasecmp(param[1], "DEL") && ExFunc("DEL"))
	{
		Responde(cl, CLI(memoserv), "\00312DEL");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Borra un mensaje.");
		Responde(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer LIST.");
		Responde(cl, CLI(memoserv), "Adicionalmente, puedes especificar ALL para borrarlos todos a la vez.");
		Responde(cl, CLI(memoserv), "Recuerda que una vez borrado, no lo podrás recuperar.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintxis: \00312DEL [#canal] nº|ALL");
	}
	else if (!strcasecmp(param[1], "LIST") && ExFunc("LIST"))
	{
		Responde(cl, CLI(memoserv), "\00312LIST");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Lista todos los mensajes.");
		Responde(cl, CLI(memoserv), "Se detalla la fecha de emisión, el emisor y el número que ocupa.");
		Responde(cl, CLI(memoserv), "Además, si va precedido por un asterisco, marca que el mensaje es nuevo y todavía no se ha leído.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312LIST [#canal]");
	}
	else if (!strcasecmp(param[1], "INFO") && ExFunc("INFO"))
	{
		Responde(cl, CLI(memoserv), "\00312INFO");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Muestra distinta información procedente de tus opciones.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312INFO [#canal]");
	}
	else if (!strcasecmp(param[1], "CANCELAR") && ExFunc("CANCELAR"))
	{
		Responde(cl, CLI(memoserv), "\00312CANCELAR");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Cancela el último mensaje que has enviado.");
		Responde(cl, CLI(memoserv), "Esto puede servir para cerciorarse de enviar un mensaje.");
		Responde(cl, CLI(memoserv), "Recuerda que si has enviado uno después, tendrás que cancelar ambos ya que se elimina primero el último.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312CANCELAR nick|#canal");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "SET") && ExFunc("SET"))
	{
		Responde(cl, CLI(memoserv), "\00312SET");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Fija las distintas opciones.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312SET [#canal] LIMITE nº");
		Responde(cl, CLI(memoserv), "Fija el nº máximo de mensajes que puede recibir.");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312SET #canal NOTIFY ON|OFF");
		Responde(cl, CLI(memoserv), "Avisa si hay mensajes nuevos en el canal.");
		Responde(cl, CLI(memoserv), "Sintaxis: \00312SET NOTIFY {+|-}modos");
		Responde(cl, CLI(memoserv), "Fija distintos métodos de notificación de tus memos:");
		Responde(cl, CLI(memoserv), "-\00312l\003 Te notifica de mensajes nuevos al identificarte como propietario de tu nick.");
		Responde(cl, CLI(memoserv), "-\00312n\003 Te notifica de mensajes nuevos cuando te son enviados al instante.");
		Responde(cl, CLI(memoserv), "-\00312w\003 Te notifica de mensajes nuevos cuando vuevles del estado de away.");
		Responde(cl, CLI(memoserv), "NOTA: El tener el modo +w deshabilita los demás. Esto es así porque sólo se quiere ser informado cuando no se está away.");
	}
#endif
	else
		Responde(cl, CLI(memoserv), NS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(MSRead)
{
	int i;
	SQLRes res;
	SQLRow row;
	char *para, *no;
	time_t tim;
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, "READ #canal nº|LAST|NEW");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
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
		Responde(cl, CLI(memoserv), MS_ERR_SNTX, "READ [#canal] nº|LAST|NEW");
		return 1;
	}
	if (!strcasecmp(no, "NEW"))
	{
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, strtolower(para))))
		{
			while ((row = SQLFetchRow(res)))
			{
				tim = atol(row[2]);
				Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
				Responde(cl, CLI(memoserv), "%s", row[1]);
				SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, strtolower(para), row[1], row[0], row[2]);
			}
			SQLFreeRes(res);
		}
		else
		{
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes nuevos.");
			return 1;
		}
	}
	else if (!strcasecmp(no, "LAST"))
	{
		char *fech = NULL;
		if ((res = SQLQuery("SELECT MAX(fecha) FROM %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(para))))
		{
			row = SQLFetchRow(res);
			if (!row[0])
				goto non;
			fech = strdup(row[0]);
			SQLFreeRes(res);
		}
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s' AND fecha=%s", PREFIJO, MS_SQL, strtolower(para), fech)))
		{
			row = SQLFetchRow(res);
			tim = atol(row[2]);
			Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
			Responde(cl, CLI(memoserv), "%s", row[1]);
			SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, strtolower(para), row[1], row[0], row[2]);
			SQLFreeRes(res);
			Free(fech);
		}
		else
		{
			non:
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
			return 1;
		}
	}
	else
	{
		int max = atoi(no);
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(para))))
		{
			for (i = 0; (row = SQLFetchRow(res)); i++)
			{
				if ((i + 1) == max) /* hay memo */
				{
					tim = atol(row[2]);
					Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
					Responde(cl, CLI(memoserv), "%s", row[1]);
					SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, strtolower(para), row[1], row[0], row[2]);
					SQLFreeRes(res);
					return 0;
				}
			}
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Este mensaje no existe.");
			SQLFreeRes(res);
			return 1;
		}
		else
		{
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
			return 1;
		}
	}
	return 0;
}
BOTFUNC(MSMemo)
{
	SQLRes res;
	SQLRow row;
	if (params < 3)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, "SEND nick|#canal mensaje");
		return 1;
	}
	if (*param[1] != '#' && !IsReg(param[1]))
	{
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	else if (*param[1] == '#')
	{
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
	}
	if (!IsOper(cl))
	{
		if ((res = SQLQuery("SELECT MAX(fecha) from %s%s where LOWER(de)='%s'", PREFIJO, MS_SQL, strtolower(cl->nombre))))
		{
			row = SQLFetchRow(res);
			if (row[0] && (atol(row[0]) + memoserv.cada) > time(0))
			{
				ircsprintf(buf, "Sólo puedes enviar mensajes cada %i segundos.", memoserv.cada);
				Responde(cl, CLI(memoserv), MS_ERR_EMPT, buf);
				return 1;
			}
		}
	}
	MSSend(param[1], cl->nombre, Unifica(param, params, 2, -1));
	Responde(cl, CLI(memoserv), "Mensaje enviado a \00312%s\003.", param[1]);
	return 0;
}
BOTFUNC(MSDel)
{
	int i;
	SQLRes res;
	SQLRow row;
	char *para, *no;
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, "DEL #canal nº|ALL");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
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
		Responde(cl, CLI(memoserv), MS_ERR_SNTX, "DEL [#canal] nº|ALL");
		return 1;
	}
	if (!strcasecmp(no, "ALL"))
	{
		SQLQuery("DELETE from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(para));
		Responde(cl, CLI(memoserv), "Todos los mensajes han sido borrados.");
	}
	else
	{
		int max = atoi(no);
		char *paralow = strtolower(para);
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, paralow)))
		{
			for(i = 0; (row = SQLFetchRow(res)); i++)
			{
				if (max == (i + 1))
				{
					SQLQuery("DELETE from %s%s where LOWER(para)='%s' AND de='%s' AND mensaje='%s' AND fecha=%s", PREFIJO, MS_SQL, paralow, row[0], row[1], row[2]);
					Responde(cl, CLI(memoserv), "El mensaje \00312%s\003 ha sido borrado.", no);
					SQLFreeRes(res);
					return 0;
				}
			}
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No se encuentra el mensaje.");
			SQLFreeRes(res);
		}
		else
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No hay mensajes.");
	}
	return 0;
}
BOTFUNC(MSList)
{
	SQLRes res;
	SQLRow row;
	char *tar;
	int i;
	time_t tim;
	if (params > 1 && *param[1] == '#')
	{
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
		tar = param[1];
	}
	else
		tar = cl->nombre;
	if ((res = SQLQuery("SELECT de,fecha,leido from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(tar))))
	{
		for (i = 0; (row = SQLFetchRow(res)); i++)
		{
			tim = atol(row[1]);
			Responde(cl, CLI(memoserv), "%s \00312%i\003 - \00312%s\003 - \00312%s\003", atoi(row[2]) ? "" : "*", i + 1, row[0], Fecha(&tim));
		}
		SQLFreeRes(res);
	}
	else
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No hay mensajes para listar.");
	return 0;
}
BOTFUNC(MSOpts)
{
	char *para, *opt, *val;
	if (params < 3)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, "SET [#canal] opción valor");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 4)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, "SET #canal opción valor");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
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
			Responde(cl, CLI(memoserv), MS_ERR_SNTX, "SET [#canal] limite nº");
			return 1;
		}
		if (abs(atoi(val)) > 30)
		{
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "El límite no puede superar los 30 mensajes.");
			return 1;
		}
		SQLInserta(MS_SET, para, "limite", val);
		Responde(cl, CLI(memoserv), "Límite cambiado a \00312%s\003.", val);
	}
	else if (!strcasecmp(opt, "NOTIFY"))
	{
		if (*para == '#')
		{
			if (!strcasecmp(val, "ON"))
				SQLInserta(MS_SET, para, "opts", "%i", MS_OPT_CNO);
			else if (!strcasecmp(val, "OFF"))
				SQLInserta(MS_SET, para, "opts", "0");
			else
			{
				Responde(cl, CLI(memoserv), MS_ERR_SNTX, "SET #canal NOTIFY ON|OFF");
				return 1;
			}
		}
		else
		{
			int opts;
			char f = ADD, *modos = val, *regopts;
			if (!(regopts = SQLCogeRegistro(MS_SET, para, "opts")))
			{
				Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Ha ocurrido un error grave: no se encuentra opts (1)");
				return 1;
			}
			opts = atoi(regopts);
			if (*val != '+' && *val != '-')
			{
				Responde(cl, CLI(memoserv), MS_ERR_SNTX, "SET NOTIFY +-modos");
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
			SQLInserta(MS_SET, para, "opts", "%i", opts);
			Responde(cl, CLI(memoserv), "Notify cambiado a \00312%s\003.", val);
		}
	}
	else
	{
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Opción desconocida.");
		return 1;
	}
	return 0;
}
BOTFUNC(MSInfo)
{
	SQLRes res;
	int memos = 0, opts = 0;
	char *regopts;
	if (params > 1 && *param[1] == '#')
	{
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(param[1]))))
		{
			memos = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		if (!(regopts = SQLCogeRegistro(MS_SET, param[1], "opts")))
		{
			Responde(cl, CLI(memoserv), "Ha ocurrido un error grave: no se encuentra opts (2)");
			return 1;
		}
		opts = atoi(regopts);
		Responde(cl, CLI(memoserv), "El canal \00312%s\003 tiene \00312%i\003 mensajes.", param[1], memos);
		Responde(cl, CLI(memoserv), "La notificación de memos está \002%s\002.", opts ? "ON" : "OFF");
		Responde(cl, CLI(memoserv), "El límite de mensajes está fijado a \00312%s\003.", SQLCogeRegistro(MS_SET, param[1], "limite"));
	}
	else
	{
		int noleid = 0;
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(cl->nombre))))
		{
			memos = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, strtolower(cl->nombre))))
		{
			noleid = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		if (!(regopts = SQLCogeRegistro(MS_SET, cl->nombre, "opts")))
		{
			Responde(cl, CLI(memoserv), "Ha ocurrido un error grave: no se encuentra opts (3)");
			return 1;
		}
		opts = atoi(regopts);
		Responde(cl, CLI(memoserv), "Tienes \00312%i\003 mensajes sin leer de un total de \00312%i\003.", noleid, memos);
		Responde(cl, CLI(memoserv), "El límite de mensajes está fijado a \00312%s\003.", SQLCogeRegistro(MS_SET, cl->nombre, "limite"));
		Responde(cl, CLI(memoserv), "La notificación de memos está a:");
		if (opts & MS_OPT_LOG)
			Responde(cl, CLI(memoserv), "-Al identificarte como propietario de tu nick.");
		if (opts & MS_OPT_AWY)
			Responde(cl, CLI(memoserv), "-Cuando regreses del away.");
		if (opts & MS_OPT_NEW)
			Responde(cl, CLI(memoserv), "-Cuando recibas un mensaje nuevo.");
	}	
	return 0;
}
BOTFUNC(MSCancela)
{
	SQLRes res;
	SQLRow row;
	char *de;
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, "CANCELAR nick|#canal");
		return 1;
	}
	if (*param[1] == '#' && !IsChanReg(param[1]))
	{
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Este canal no está registrado.");
		return 1;
	}
	else if (*param[1] != '#' && !IsReg(param[1]))
	{
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	de = strdup(strtolower(cl->nombre));
	if ((res = SQLQuery("SELECT MAX(fecha) from %s%s where LOWER(de)='%s' AND LOWER(para)='%s'", PREFIJO, MS_SQL, de, strtolower(param[1]))))
	{
		row = SQLFetchRow(res);
		if (!row[0])
			goto non;
		SQLQuery("DELETE from %s%s where LOWER(para)='%s' AND LOWER(de)='%s' AND fecha=%s", PREFIJO, MS_SQL, strtolower(param[1]), de, row[0]);
		SQLFreeRes(res);
		Responde(cl, CLI(memoserv), "El último mensaje de \00312%s\003 ha sido eliminado.", param[1]);
	}
	else
	{
		non:
		Free(de);
		buf[0] = '\0';
		ircsprintf(buf, "No has enviado ningún mensaje a %s.", param[1]);
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, buf);
		return 1;
	}
	Free(de);
	return 0;
}
int MSCmdAway(Cliente *cl, char *away)
{
	int opts;
	if (!away)
	{
		char *regopts;
		if (!IsId(cl))
			return 1;
		if (!(regopts = SQLCogeRegistro(MS_SET, cl->nombre, "opts")))
			return 1; /* no debería pasar nunca! */
		opts = atoi(regopts);
		if (opts & MS_OPT_AWY)
			MSNotifica(cl);
	}
	return 0;
}
int MSCmdJoin(Cliente *cl, Canal *cn)
{
	int opts;
	SQLRes res;
	char *regopts;
	if (!IsChanReg(cn->nombre))
		return 1;
	if (!(res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, strtolower(cn->nombre))))
		return 1;
	SQLFreeRes(res);
	if (!(regopts = SQLCogeRegistro(MS_SET, cn->nombre, "opts")))
		return 1;
	opts = atoi(regopts);
	if (opts & MS_OPT_CNO)
	{
		if (CSTieneNivel(cl->nombre, cn->nombre, CS_LEV_MEM))
			ProtFunc(P_NOTICE)(cl, CLI(memoserv), "Hay mensajes en el canal");
	}
	return 0;
}
int MSSend(char *para, char *de, char *mensaje)
{
	int opts = 0;
	Cliente *al;
	char *regopts;
	if (para && de && mensaje)
	{
		if (!IsChanReg(para) && !IsReg(para))
			return 1;
		SQLQuery("INSERT into %s%s (para,de,fecha,mensaje) values ('%s','%s','%lu','%s')", PREFIJO, MS_SQL, para, de, time(0), mensaje);
		if (!(regopts = SQLCogeRegistro(MS_SET, para, "opts")))
			return 1;
		opts = atoi(regopts);
		if (*para == '#')
		{
			if (opts & MS_OPT_CNO)
				ProtFunc(P_NOTICE)((Cliente *)BuscaCanal(para, NULL), CLI(memoserv), "Hay un mensaje nuevo en el canal");
		}
		else
		{
			if (opts & MS_OPT_NEW)
			{
				al = BuscaCliente(para, NULL);
				if (al && IsId(al))
				{
					if (!(al->nivel & AWAY) || !(opts & MS_OPT_AWY))
							MSNotifica(al);
				}	
			}
		}
		return 0;
	}
	return -1;
}
int MSSigIdOk(Cliente *al)
{
	int opts;
	char *reg;
	if ((reg = SQLCogeRegistro(MS_SET, al->nombre, "opts")))
	{
		opts = atoi(reg);
		if ((opts & MS_OPT_LOG) && (!(al->nivel & AWAY) || !(opts & MS_OPT_AWY)))
			MSNotifica(al);
	}
	return 0;
}
int MSSigSQL()
{
	if (!SQLEsTabla(MS_SQL))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"n SERIAL, "
  			"mensaje text, "
  			"para text, "
  			"de text, "
  			"fecha int4 default '0', "
  			"leido int4 default '0' "
			");", PREFIJO, MS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, MS_SQL);
	}
	if (!SQLEsTabla(MS_SET))
	{
		SQLRes res;
		SQLRow row;
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"opts varchar(255) default NULL, "
  			"limite int4 default '%i' "
			");", PREFIJO, MS_SET, memoserv.def))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, MS_SET);
		if ((res = SQLQuery("SELECT item from %s%s", PREFIJO, NS_SQL)))
		{
			while ((row = SQLFetchRow(res)))
				SQLInserta(MS_SET, row[0], "opts", "%i", MS_OPT_ALL);
			SQLFreeRes(res);
		}
		if ((res = SQLQuery("SELECT item from %s%s", PREFIJO, CS_SQL)))
		{
			while ((row = SQLFetchRow(res)))
				SQLInserta(MS_SET, row[0], "opts", "%i", MS_OPT_ALL);
			SQLFreeRes(res);
		}
	}
	SQLCargaTablas();
	return 0;
}
void MSNotifica(Cliente *al)
{
	SQLRes res;
	if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, strtolower(al->nombre))))
	{
		Responde(al, CLI(memoserv), "Tienes \00312%i\003 mensaje(s) nuevo(s).", SQLNumRows(res));
		SQLFreeRes(res);
	}
	return;
}
int MSSigDrop(char *nick)
{
	SQLBorra(MS_SET, nick);
	return 0;
}
int MSSigRegistra(char *nick)
{
	SQLInserta(MS_SET, nick, "opts", "%i", MS_OPT_ALL);
	return 0;
}
