/*
 * $Id: memoserv.c,v 1.35 2008/01/21 19:46:46 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#include "modulos/memoserv.h"

MemoServ *memoserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, memoserv->hmod, NULL)

BOTFUNC(MSHelp);
BOTFUNC(MSMemo);
BOTFUNCHELP(MSHSend);
BOTFUNC(MSRead);
BOTFUNCHELP(MSHRead);
BOTFUNC(MSDel);
BOTFUNCHELP(MSHDel);
BOTFUNC(MSList);
BOTFUNCHELP(MSHList);
BOTFUNC(MSOpts);
BOTFUNCHELP(MSHSet);
BOTFUNC(MSInfo);
BOTFUNCHELP(MSHInfo);
BOTFUNC(MSCancelar);
BOTFUNCHELP(MSHCancelar);

static bCom memoserv_coms[] = {
	{ "help" , MSHelp , N0 , "Muestra esta ayuda." , NULL } ,
	{ "send" , MSMemo , N1 , "Envía un mensaje." , MSHSend } ,
	{ "read" , MSRead , N1 , "Lee un mensaje." , MSHRead } ,
	{ "del" , MSDel , N1 , "Borra un mensaje." , MSHDel } ,
	{ "list" , MSList , N1 , "Lista los mensajes." , MSHList } ,
	{ "set" , MSOpts , N1 , "Fija tus opciones." , MSHSet } ,
	{ "info" , MSInfo , N1 , "Muestra información de tu configuración." , MSHInfo } ,
	{ "cancelar" , MSCancelar , N1 , "Cancela el último mensaje que hayas enviado." , MSHCancelar } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

int MSCmdAway(Cliente *, char *);
int MSCmdJoin(Cliente *, Canal *);

int MSSigIdOk	(Cliente *);
int MSSigSQL	();
void MSNotifica	(Cliente *);
int MSSigDrop	(char *);
int MSSigRegistra	(char *);
void MSSet(Conf *, Modulo *);
int MSTest(Conf *, int *);

ModInfo MOD_INFO(MemoServ) = {
	"MemoServ" ,
	0.8 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};

int MOD_CARGA(MemoServ)(Modulo *mod)
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
	else
		MSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(MemoServ)()
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
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], memoserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void MSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!memoserv)
		memoserv = BMalloc(MemoServ);
	memoserv->def = 5;
	memoserv->cada = 30;
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "defecto"))
				memoserv->def = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "cada"))
				memoserv->cada = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, memoserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, memoserv_coms);
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
		ProcesaComsMod(NULL, mod, memoserv_coms);
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
BOTFUNCHELP(MSHSend)
{
	Responde(cl, CLI(memoserv), "Envía un mensaje a un usuario o canal.");
	Responde(cl, CLI(memoserv), "El mensaje es guardado hasta que no se borra y puede leerse tantas veces se quiera.");
	Responde(cl, CLI(memoserv), "Cuando se deja un mensaje a un usuario, se le notificará de que tiene mensajes pendientes para leer.");
	Responde(cl, CLI(memoserv), "Si es a un canal, cada vez que entre un usuario y tenga nivel suficiente, se le notificará de que el canal tiene mensajes.");
	Responde(cl, CLI(memoserv), "Tanto el nick o canal destinatario deben estar registrados.");
	Responde(cl, CLI(memoserv), "Sólo pueden enviarse mensajes cada \00312%i\003 segundos.", memoserv->cada);
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintaxis: \00312SEND nick|#canal mensaje");
	return 0;
}
BOTFUNCHELP(MSHRead)
{
	Responde(cl, CLI(memoserv), "Lee un mensaje.");
	Responde(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer un LIST.");
	Responde(cl, CLI(memoserv), "Adicionalmente, puedes especificar NEW para leer todos los mensajes nuevos o LAST para leer el último.");
	Responde(cl, CLI(memoserv), "Un mensaje lo podrás leer siempre que quieras hasta que sea borrado.");
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintaxis: \00312READ [#canal] nº|NEW|LAST");
	return 0;
}
BOTFUNCHELP(MSHDel)
{
	Responde(cl, CLI(memoserv), "Borra un mensaje.");
	Responde(cl, CLI(memoserv), "El número de mensaje es el mismo que aparece antepuesto al hacer LIST.");
	Responde(cl, CLI(memoserv), "Adicionalmente, puedes especificar ALL para borrarlos todos a la vez.");
	Responde(cl, CLI(memoserv), "Recuerda que una vez borrado, no lo podrás recuperar.");
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintxis: \00312DEL [#canal] nº|ALL");
	return 0;
}
BOTFUNCHELP(MSHList)
{
	Responde(cl, CLI(memoserv), "Lista todos los mensajes.");
	Responde(cl, CLI(memoserv), "Se detalla la fecha de emisión, el emisor y el número que ocupa.");
	Responde(cl, CLI(memoserv), "Además, si va precedido por un asterisco, marca que el mensaje es nuevo y todavía no se ha leído.");
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintaxis: \00312LIST [#canal]");
	return 0;
}
BOTFUNCHELP(MSHInfo)
{
	Responde(cl, CLI(memoserv), "Muestra distinta información procedente de tus opciones.");
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintaxis: \00312INFO [#canal]");
	return 0;
}
BOTFUNCHELP(MSHCancelar)
{
	Responde(cl, CLI(memoserv), "Cancela el último mensaje que has enviado.");
	Responde(cl, CLI(memoserv), "Esto puede servir para cerciorarse de enviar un mensaje.");
	Responde(cl, CLI(memoserv), "Recuerda que si has enviado uno después, tendrás que cancelar ambos ya que se elimina primero el último.");
	Responde(cl, CLI(memoserv), " ");
	Responde(cl, CLI(memoserv), "Sintaxis: \00312CANCELAR nick|#canal");
	return 0;
}
BOTFUNCHELP(MSHSet)
{
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
	Responde(cl, CLI(memoserv), "-\00312w\003 Te notifica de mensajes nuevos cuando vuelves del estado de away.");
	Responde(cl, CLI(memoserv), "NOTA: El tener el modo +w deshabilita los demás. Esto es así porque sólo se quiere ser informado cuando no se está away.");
	return 0;
}
BOTFUNC(MSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), "\00312%s\003 se encarga de gestionar el servicio de mensajería.", memoserv->hmod->nick);
		Responde(cl, CLI(memoserv), "Este servicio permite enviar y recibir mensajes entre usuarios aunque no estén conectados simultáneamente.");
		Responde(cl, CLI(memoserv), "Además, permite mantener un historial de mensajes recibidos y enviar de nuevos a usuarios que se encuentren desconectados.");
		Responde(cl, CLI(memoserv), "Cuando se conecten, y según su configuración, recibirán un aviso de que tienen nuevos mensajes para leer.");
		Responde(cl, CLI(memoserv), "Los mensajes también pueden enviarse a un canal. Así, los usuarios que tengan el nivel suficiente podrán enviar y recibir mensajes para un determinado canal.");
		Responde(cl, CLI(memoserv), "Este servicio sólo es para usuarios y canales registrados.");
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Comandos disponibles:");
		ListaDescrips(memoserv->hmod, cl);
		Responde(cl, CLI(memoserv), " ");
		Responde(cl, CLI(memoserv), "Para más información, \00312/msg %s %s comando", memoserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], memoserv->hmod, param, params))
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
		Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "[#canal] nº|LAST|NEW");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "#canal nº|LAST|NEW");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
		para = SQLEscapa(strtolower(param[1]));
		no = param[2];
	}
	else
	{
		para = SQLEscapa(strtolower(cl->nombre));
		no = param[1];
	}
	if (!atoi(no) && strcasecmp(no, "LAST") && strcasecmp(no, "NEW"))
	{
		Responde(cl, CLI(memoserv), MS_ERR_SNTX, "Opción desconocida. Sólo nº|LAST|NEW");
		return 1;
	}
	if (!strcasecmp(no, "NEW"))
	{
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, para)))
		{
			char *c1, *c2;
			while ((row = SQLFetchRow(res)))
			{
				tim = atol(row[2]);
				Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
				Responde(cl, CLI(memoserv), "%s", row[1]);
				c1 = SQLEscapa(row[1]);
				c2 = SQLEscapa(row[0]);
				SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, para, c1, c2, row[2]);
				Free(c1);
				Free(c2);
			}
			SQLFreeRes(res);
		}
		else
		{
			Free(para);
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes nuevos.");
			return 1;
		}
	}
	else if (!strcasecmp(no, "LAST"))
	{
		char *fech = NULL;
		if ((res = SQLQuery("SELECT MAX(fecha) FROM %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, para)))
		{
			row = SQLFetchRow(res);
			if (BadPtr(row[0]))
				goto non;
			fech = strdup(row[0]);
			SQLFreeRes(res);
		}
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s' AND fecha=%s", PREFIJO, MS_SQL, para, fech)))
		{
			char *c1, *c2;
			row = SQLFetchRow(res);
			tim = atol(row[2]);
			Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
			Responde(cl, CLI(memoserv), "%s", row[1]);
			c1 = SQLEscapa(row[1]);
			c2 = SQLEscapa(row[0]);
			SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, para, c1, c2, row[2]);
			SQLFreeRes(res);
			Free(fech);
			Free(c1);
			Free(c2);
		}
		else
		{
			non:
			Free(para);
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
			return 1;
		}
	}
	else
	{
		int max = atoi(no);
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, para)))
		{
			for (i = 0; (row = SQLFetchRow(res)); i++)
			{
				if ((i + 1) == max) /* hay memo */
				{
					char *c1, *c2;
					tim = atol(row[2]);
					Responde(cl, CLI(memoserv), "Mensaje de \00312%s\003, enviado el \00312%s\003", row[0], Fecha(&tim));
					Responde(cl, CLI(memoserv), "%s", row[1]);
					c1 = SQLEscapa(row[1]);
					c2 = SQLEscapa(row[0]);
					SQLQuery("UPDATE %s%s SET leido=1 where LOWER(para)='%s' AND mensaje='%s' AND de='%s' AND fecha=%s", PREFIJO, MS_SQL, para, c1, c2, row[2]);
					SQLFreeRes(res);
					Free(c1);
					Free(c2);
					Free(para);
					return 0;
				}
			}
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "Este mensaje no existe.");
			SQLFreeRes(res);
			Free(para);
			return 1;
		}
		else
		{
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No tienes mensajes.");
			Free(para);
			return 1;
		}
	}
	return 0;
}
BOTFUNC(MSMemo)
{
	SQLRes res;
	SQLRow row;
	int memos = 0;
	char *limite;
	if (params < 3)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "nick|#canal mensaje");
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
		if (!CSTieneNivel(cl, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
	}
	if (!IsOper(cl))
	{
		char *ll = SQLEscapa(strtolower(cl->nombre));
		res = SQLQuery("SELECT MAX(fecha) from %s%s where LOWER(de)='%s'", PREFIJO, MS_SQL, ll);
		Free(ll);
		if (res)
		{
			row = SQLFetchRow(res);
			if (row[0] && (atol(row[0]) + memoserv->cada) > time(0))
			{
				ircsprintf(buf, "Sólo puedes enviar mensajes cada %i segundos.", memoserv->cada);
				Responde(cl, CLI(memoserv), MS_ERR_EMPT, buf);
				return 1;
			}
			SQLFreeRes(res);
		}
		ll = SQLEscapa(strtolower(param[1]));
		res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(para)='%s'", PREFIJO, MS_SQL, ll);
		Free(ll);
		if (res)
		{
			memos = SQLNumRows(res);
			SQLFreeRes(res);
		}
		if ((limite = SQLCogeRegistro(MS_SET, param[1], "limite")))
		{
			if (memos >= atoi(limite))
			{
				ircsprintf(buf, "No puedes mandar más emails a \00312%s\003. Tiene la cuenta llena.", param[1]);
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
		Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "[#canal] nº|ALL");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 3)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "#canal nº|ALL");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
		para = SQLEscapa(strtolower(param[1]));
		no = param[2];
	}
	else
	{
		para = SQLEscapa(strtolower(cl->nombre));
		no = param[1];
	}
	if (!atoi(no) && strcasecmp(no, "ALL"))
	{
		Responde(cl, CLI(memoserv), MS_ERR_SNTX, "Opción desconocida. Sólo: nº|ALL");
		Free(para);
		return 1;
	}
	if (!strcasecmp(no, "ALL"))
	{
		SQLQuery("DELETE from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, para);
		Responde(cl, CLI(memoserv), "Todos los mensajes han sido borrados.");
	}
	else
	{
		int max = atoi(no);
		char *m_m, *c1;
		if ((res = SQLQuery("SELECT de,mensaje,fecha from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, para)))
		{
			for(i = 0; (row = SQLFetchRow(res)); i++)
			{
				if (max == (i + 1))
				{
					m_m = SQLEscapa(row[1]);
					c1 = SQLEscapa(row[0]);
					SQLQuery("DELETE from %s%s where LOWER(para)='%s' AND de='%s' AND mensaje='%s' AND fecha=%s", PREFIJO, MS_SQL, para, c1, m_m, row[2]);
					Responde(cl, CLI(memoserv), "El mensaje \00312%s\003 ha sido borrado.", no);
					SQLFreeRes(res);
					Free(m_m);
					Free(c1);
					Free(para);
					return 0;
				}
			}
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No se encuentra el mensaje.");
			SQLFreeRes(res);
		}
		else
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "No hay mensajes.");
	}
	Free(para);
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
		if (!CSTieneNivel(cl, param[1], CS_LEV_MEM))
		{
			Responde(cl, CLI(memoserv), CS_ERR_FORB, "");
			return 1;
		}
		tar = SQLEscapa(strtolower(param[1]));
	}
	else
		tar = SQLEscapa(strtolower(cl->nombre));
	if ((res = SQLQuery("SELECT de,fecha,leido from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, tar)))
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
	Free(tar);
	return 0;
}
BOTFUNC(MSOpts)
{
	char *para, *opt, *val;
	if (params < 3)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "[#canal] opción valor");
		return 1;
	}
	if (*param[1] == '#')
	{
		if (params < 4)
		{
			Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "#canal opción valor");
			return 1;
		}
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		if (!CSTieneNivel(cl, param[1], CS_LEV_MEM))
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
		u_int lim;
		if (!IsDigit(*val))
		{
			Responde(cl, CLI(memoserv), MS_ERR_SNTX, "El límite debe ser un número entero");
			return 1;
		}
		if ((lim = abs(atoi(val))) > 30)
		{
			Responde(cl, CLI(memoserv), MS_ERR_EMPT, "El límite no puede superar los 30 mensajes.");
			return 1;
		}
		SQLInserta(MS_SET, para, "limite", "%u", lim);
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
				Responde(cl, CLI(memoserv), MS_ERR_SNTX, "Opción desconocida. Sólo: ON|OFF");
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
				Responde(cl, CLI(memoserv), MS_ERR_SNTX, "Los modos deben empezar por + o -");
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
	char *regopts, *ll;
	if (params > 1 && *param[1] == '#')
	{
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(memoserv), MS_ERR_NOTR, "");
			return 1;
		}
		ll = SQLEscapa(strtolower(param[1]));
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, ll)))
		{
			memos = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		Free(ll);
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
		ll = SQLEscapa(strtolower(cl->nombre));
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, ll)))
		{
			memos = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		if ((res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, ll)))
		{
			noleid = (int)SQLNumRows(res);
			SQLFreeRes(res);
		}
		Free(ll);
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
BOTFUNC(MSCancelar)
{
	SQLRes res;
	SQLRow row;
	char *de, *ll;
	if (params < 2)
	{
		Responde(cl, CLI(memoserv), MS_ERR_PARA, fc->com, "nick|#canal");
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
	de = SQLEscapa(strtolower(cl->nombre));
	ll = SQLEscapa(strtolower(param[1]));
	if ((res = SQLQuery("SELECT MAX(fecha) from %s%s where LOWER(de)='%s' AND LOWER(para)='%s'", PREFIJO, MS_SQL, de, ll)))
	{
		row = SQLFetchRow(res);
		if (BadPtr(row[0]))
			goto non;
		SQLQuery("DELETE from %s%s where LOWER(para)='%s' AND LOWER(de)='%s' AND fecha=%s", PREFIJO, MS_SQL, ll, de, row[0]);
		SQLFreeRes(res);
		Responde(cl, CLI(memoserv), "El último mensaje de \00312%s\003 ha sido eliminado.", param[1]);
	}
	else
	{
		non:
		Free(de);
		Free(ll);
		buf[0] = '\0';
		ircsprintf(buf, "No has enviado ningún mensaje a %s.", param[1]);
		Responde(cl, CLI(memoserv), MS_ERR_EMPT, buf);
		return 1;
	}
	Free(de);
	Free(ll);
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
	char *regopts, *cc;
	if (!IsChanReg(cn->nombre))
		return 1;
	cc = SQLEscapa(strtolower(cn->nombre));
	res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s'", PREFIJO, MS_SQL, cc);
	Free(cc);
	if (!res)
		return 1;
	SQLFreeRes(res);
	if (!(regopts = SQLCogeRegistro(MS_SET, cn->nombre, "opts")))
		return 1;
	opts = atoi(regopts);
	if (opts & MS_OPT_CNO)
	{
		if (CSTieneNivel(cl, cn->nombre, CS_LEV_MEM))
			ProtFunc(P_NOTICE)(cl, CLI(memoserv), "Hay mensajes en el canal");
	}
	return 0;
}
int MSSend(char *para, char *de, char *mensaje)
{
	int opts = 0;
	Cliente *al;
	char *regopts, *m_c, *ll, *pp;
	if (para && de && mensaje)
	{
		if (!IsChanReg(para) && !IsReg(para))
			return 1;
		m_c = SQLEscapa(mensaje);
		pp = SQLEscapa(para);
		ll = SQLEscapa(de);
		SQLQuery("INSERT into %s%s (para,de,fecha,mensaje) values ('%s','%s','%lu','%s')", PREFIJO, MS_SQL, pp, ll, time(0), m_c);
		Free(m_c);
		Free(pp);
		Free(ll);
		if (!(regopts = SQLCogeRegistro(MS_SET, para, "opts")))
			return 1;
		opts = atoi(regopts);
		if (*para == '#')
		{
			if (opts & MS_OPT_CNO)
				ProtFunc(P_NOTICE)((Cliente *)BuscaCanal(para), CLI(memoserv), "Hay un mensaje nuevo en el canal");
		}
		else
		{
			if (opts & MS_OPT_NEW)
			{
				if ((al = BuscaCliente(para)) && IsId(al))
				{
					if (!al->away || !(opts & MS_OPT_AWY))
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
		if ((opts & MS_OPT_LOG) && (!al->away || !(opts & MS_OPT_AWY)))
			MSNotifica(al);
	}
	return 0;
}
int MSSigSQL()
{
	SQLNuevaTabla(MS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"mensaje text, "
  		"para varchar(255), "
  		"de varchar(255), "
  		"fecha int4 default '0', "
  		"leido int4 default '0', "
  		"KEY para (para) "
		");", PREFIJO, MS_SQL);
	if (!SQLNuevaTabla(MS_SET, "CREATE TABLE IF NOT EXISTS %s%s ( "
		"item varchar(255) default NULL, "
		"opts varchar(255) default NULL, "
		"limite int4 default '%i', "
		"KEY item (item) "
		");", PREFIJO, MS_SET, memoserv->def))
	{
		SQLRes res;
		SQLRow row;
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
	return 0;
}
void MSNotifica(Cliente *al)
{
	SQLRes res;
	char *ll = SQLEscapa(strtolower(al->nombre));
	res = SQLQuery("SELECT * from %s%s where LOWER(para)='%s' AND leido='0'", PREFIJO, MS_SQL, ll);
	Free(ll);
	if (res)
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
