/*
 * $Id: ipserv.c,v 1.10 2007/05/27 19:14:36 Trocotronic Exp $
 */

#ifndef _WIN32
#include <dlfcn.h>
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/ipserv.h"
#include "modulos/nickserv.h"
#include "bdd.h"

#ifdef _WIN32
IpServ *ipserv = NULL;
#else
extern IpServ *ipserv;
#endif

BOTFUNC(ISOpts);
BOTFUNCHELP(ISHSet);
BOTFUNC(ISDns);
BOTFUNCHELP(ISHDns);
BOTFUNC(ISNolines);
BOTFUNCHELP(ISHNolines);
EXTFUNC(ISSetipv_U);
EXTFUNC(ISClones_U);
EXTFUNC(ISVHost_U);

int ISSigEOS_U	();
int ISSigVDrop	(char *);
int ISSigSQL2 ();
int ISCompruebaCifrado_U();
int ISSigSynch_U();
int ISSigSockClose_U();
int (*issigumode)();
int (*issigidok)();
int (*issignick)();
#define NS_OPT_UDB 0x80
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)

Timer *timercif = NULL;

bCom ipserv_coms[] = {
	{ "set" , ISOpts , N4 , "Fija algunos parámetros de la red." , ISHSet } ,
	{ "dns" , ISDns , N4 , "Establece una resolución inversa de una ip." , ISHDns } ,
	{ "nolines" , ISNolines , N3 , "Controla las excepciones para una ip." , ISHNolines } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void CargaIpServ(Extension *ext)
{
	int i;
	if (ext->config)
	{
		Conf *cf;
		if ((cf = BuscaEntrada(ext->config, "IpServ")))
		{
			for (i = 0; i < cf->secciones; i++)
			{
				if (!strcmp(cf->seccion[i]->item, "funciones"))
					ProcesaComsMod(cf->seccion[i], ipserv->hmod, ipserv_coms);
				else if (!strcmp(cf->seccion[i]->item, "funcion"))
					SetComMod(cf->seccion[i], ipserv->hmod, ipserv_coms);
			}
		}
		else
			ProcesaComsMod(NULL, ipserv->hmod, ipserv_coms);
	}
	else
		ProcesaComsMod(NULL, ipserv->hmod, ipserv_coms);
	irc_dlsym(ipserv->hmod->hmod, "ISSigNick", issignick);
	irc_dlsym(ipserv->hmod->hmod, "ISSigUmode", issigumode);
	irc_dlsym(ipserv->hmod->hmod, "ISSigIdOk", issigidok);
	BorraSenyal(SIGN_POST_NICK, issignick);
	BorraSenyal(SIGN_UMODE, issigumode);
	BorraSenyal(NS_SIGN_IDOK, issigidok);
	InsertaSenyal(SIGN_EOS, ISSigEOS_U);
	InsertaSenyal(IS_SIGN_DROP, ISSigVDrop);
	InsertaSenyal(SIGN_SQL2, ISSigSQL2);
	InsertaSenyalExt(1, ISSetipv_U, ext);
	InsertaSenyalExt(3, ISClones_U, ext);
	InsertaSenyalExt(4, ISVHost_U, ext);
	InsertaSenyal(SIGN_SYNCH, ISSigSynch_U);
	InsertaSenyal(SIGN_SOCKCLOSE, ISSigSockClose_U);
	/*InsertaSenyalExt(16, CSLiberar, ext);
	InsertaSenyalExt(17, CSForbid, ext);
	InsertaSenyalExt(18, CSUnforbid, ext);
	InsertaSenyalExt(20, CSRegister, ext);*/
}
void DescargaIpServ(Extension *ext)
{
	InsertaSenyal(SIGN_POST_NICK, issignick);
	InsertaSenyal(SIGN_UMODE, issigumode);
	InsertaSenyal(NS_SIGN_IDOK, issigidok);
	BorraSenyal(SIGN_EOS, ISSigEOS_U);
	BorraSenyal(IS_SIGN_DROP, ISSigVDrop);
	BorraSenyal(SIGN_SQL2, ISSigSQL2);
	BorraSenyalExt(1, ISSetipv_U, ext);
	BorraSenyalExt(3, ISClones_U, ext);
	BorraSenyalExt(4, ISVHost_U, ext);
	BorraSenyal(SIGN_SYNCH, ISSigSynch_U);
	BorraSenyal(SIGN_SOCKCLOSE, ISSigSockClose_U);
	if (timercif)
	{
		ApagaCrono(timercif);
		timercif = NULL;
	}
}
BOTFUNCHELP(ISHSet)
{
	Responde(cl, CLI(ipserv), "Establece varios parámetros de la red.");
	Responde(cl, CLI(ipserv), "\00312QUIT_IPS\003 Establece el mensaje de desconexión para ips con capacidad personal de clones.");
	Responde(cl, CLI(ipserv), "\00312QUIT_CLONES\003 Fija el mensaje de desconexión cuando se supera el número de clones permitidos en la red.");
	Responde(cl, CLI(ipserv), "\00312CLONES\003 Indica el número de clones permitidos en la red.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312SET quit_ips|quit_clones|clones valor");
	return 0;
}
BOTFUNCHELP(ISHDns)
{
	Responde(cl, CLI(ipserv), "Establece una resolución inversa de una ip.");
	Responde(cl, CLI(ipserv), "Este comando permite hacer una relación a partir de una ip hacia un host.");
	Responde(cl, CLI(ipserv), "Cuando conecte un usuario al servidor, si su ip está en DNS, se le asignará automáticamente este host relacionado, sin necesidad de resoluciones inversas.");
	Responde(cl, CLI(ipserv), "NOTA: Debe ser una ip");
	Responde(cl, CLI(ipserv), "Si no se especifica ningún host, la ip es eliminada de la lista.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312DNS ip [host]");
	return 0;
}
BOTFUNCHELP(ISHNolines)
{
	Responde(cl, CLI(ipserv), "Gestiona las excepciones para una ip.");
	Responde(cl, CLI(ipserv), "Con estas excepciones, la conexión es capaz de saltarse restricciones.");
	Responde(cl, CLI(ipserv), "Excepciones:");
	Responde(cl, CLI(ipserv), "\00312G\003 - GLines");
	Responde(cl, CLI(ipserv), "\00312Z\003 - ZLines");
	Responde(cl, CLI(ipserv), "\00312Q\003 - QLines");
	Responde(cl, CLI(ipserv), "\00312S\003 - Shuns");
	Responde(cl, CLI(ipserv), "\00312T\003 - Throttle");
	Responde(cl, CLI(ipserv), "La ip que tenga estas excepciones será inmune.");
	Responde(cl, CLI(ipserv), "Sin parámetros, se borran las excepciones para esa ip.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312NOLINES ip [nolines]");
	Responde(cl, CLI(ipserv), "Ejemplo: \00312NOLINES 127.0.01 GZQST");
	return 0;
}
BOTFUNC(ISOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "quit_ips|quit_clones|clones valor");
		return 1;
	}
	if (!strcasecmp(param[1], "QUIT_IPS"))
	{
		PropagaRegistro("S::T %s", Unifica(param, params, 2, -1));
		Responde(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión para ips con número personal de clones.");
	}
	else if (!strcasecmp(param[1], "QUIT_CLONES"))
	{
		PropagaRegistro("S::Q %s", Unifica(param, params, 2, -1));
		Responde(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión al rebasar el número de clones permitidos en la red.");
	}
	else if (!strcasecmp(param[1], "CLONES"))
	{
		int clons = atoi(param[2]);
		if (clons < 1 || clons > 1024)
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "El número de clones debe estar entre 1 y 1024.");
			return 1;
		}
		PropagaRegistro("S::S %c%s", CHAR_NUM, param[2]);
		Responde(cl, CLI(ipserv), "El número de clones permitidos en la red se ha cambiado a \00312%s", param[2]);
	}
	else
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "SET quit_ips|quit_clones|clones valor");
		return 1;
	}
	return 0;
}
BOTFUNC(ISDns)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "[+|-]{ip|patrón} [host] [nota]");
		return 1;
	}
	
	if (*param[1] == '+')
	{
		char *nota = NULL;
		time_t fecha = time(NULL);
		if (params < 3)
		{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "[+|-]{ip|patrón} [host] [nota]");
		return 1;
		}		
		param[1]++;
		if (!EsIp(param[1]))
		{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "No parece ser una ip");
		return 1;
		}

		SQLInserta(IS_DNS, param[1], "dns", param[2]);
		
		if (params > 3) { //Comprobamos nota o creamos una.
		  	nota = Unifica(param, params, 3, -1);
			SQLInserta(IS_DNS, param[1], "nota", "%s por %s a las %s\t", nota, cl->nombre, Fecha(&fecha));		  			
		}
		else
		 	SQLInserta(IS_DNS, param[1], "nota", "Añadido por %s a las %s\t", cl->nombre, Fecha(&fecha));
		
		PropagaRegistro("I::%s::H %s", param[1], param[2]); //Añadimos a UDB
		Responde(cl, CLI(ipserv), "Modificada la resolución de la ip \00312%s\003 a \00312%s", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		if (!SQLCogeRegistro(IS_DNS, param[1], "dns"))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "La ip no tiene asignada ninguna resolución personalizada.");
			return 1;
		}
		SQLBorra(IS_DNS, param[1]);
		PropagaRegistro("I::%s::H", param[1]);
		Responde(cl, CLI(ipserv), "La ip \00312%s\003 le ha sido eliminada su resolución personalizada.", param[1]);
	}
	else
	{
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,dns,nota from %s%s where item LIKE '%s'", PREFIJO, IS_DNS, rep)))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(ipserv), "*** Ip que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < ipserv->maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(ipserv), "Ip/Host: \00312%s\003 DNS: \00312%s\003 Nota: \00312%s\003", row[0], row[1], row[2]);
		Responde(cl, CLI(ipserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	return 0;
}
BOTFUNC(ISNolines)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "[+|-]{ip|host|patrón} [G|Z|Q|S|T] [nota]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *nota = NULL, *c;
		time_t fecha = time(NULL);
		if (params < 3)
		{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "[+|-]{ip|host|patrón} [G|Z|Q|S|T] [nota]");
		return 1;
		}		
		param[1]++;
		for (c = param[2]; !BadPtr(c); c++)
		{
			if (!strchr("GZQST", *c))
			{
				Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Sólo se aceptan las nolines G|Z|Q|S|T");
				return 1;

			}
		}

		SQLInserta(IS_NOLINE, param[1], "nolines", param[2]);
		
		if (params > 3) { //Comprobamos nota o creamos una.
		  	nota = Unifica(param, params, 3, -1);
			SQLInserta(IS_NOLINE, param[1], "nota", "%s por %s a las %s\t", nota, cl->nombre, Fecha(&fecha));		  			
		}
		else
		 	SQLInserta(IS_NOLINE, param[1], "nota", "Añadido por %s a las %s\t", cl->nombre, Fecha(&fecha));
		
		PropagaRegistro("I::%s::E %s", param[1], param[2]); //Añadimos a UDB
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 se le han agisnado las nolines: \00312%s", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		if (!SQLCogeRegistro(IS_NOLINE, param[1], "nolines"))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "La ip/host no tiene asignada ninguna excepción.");
			return 1;
		}
		SQLBorra(IS_NOLINE, param[1]);
		PropagaRegistro("I::%s::E", param[1]);
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 le han sido eliminados todas las excepciones.", param[1]);
	}
	else
	{
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,nolines,nota from %s%s where item LIKE '%s'", PREFIJO, IS_NOLINE, rep)))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(ipserv), "*** Ip/Host que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < ipserv->maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(ipserv), "Ip/Host: \00312%s\003 Nolines: \00312%s\003 Nota: \00312%s\003", row[0], row[1], row[2]);
		Responde(cl, CLI(ipserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	return 0;
}
EXTFUNC(ISSetipv_U)
{
	if (mod != ipserv->hmod || !IsNickUDB(param[1]))
		return 1;
	if (params >= 3)
	{
		char *ipv;
		if (!strcasecmp(param[0], "SETIPV2"))
		{
			buf[0] = '\0';
			ircsprintf(buf, "%s.%s", param[2], ipserv->sufijo);
			ipv = buf;
		}
		else
			ipv = param[2];
		PropagaRegistro("N::%s::V %s", param[1], ipv);
	}
	else
		PropagaRegistro("N::%s::V", param[1]);
	return 0;
}
EXTFUNC(ISClones_U)
{
	if (mod != ipserv->hmod)
		return 1;
	if (params >= 3)
		PropagaRegistro("I::%s::S %c%s", param[1], CHAR_NUM, param[2]);
	else
		PropagaRegistro("I::%s::S", param[1]);
	return 0;
}
EXTFUNC(ISVHost_U)
{
	if (mod != ipserv->hmod || !IsNickUDB(cl->nombre))
		return 1;
	if (params >= 2)
	{
		char *c;
		if ((c = SQLCogeRegistro(IS_SQL, cl->nombre, "ipvirtual")))
			PropagaRegistro("N::%s::V %s", cl->nombre, c);
	}
	else
		PropagaRegistro("N::%s::V", cl->nombre);
	return 0;
}
int ISSigEOS_U()
{
	PropagaRegistro("S::I %s", ipserv->hmod->mascara);
	PropagaRegistro("S::J %s", ipserv->sufijo);
	ISCompruebaCifrado_U();
	return 0;
}
int ISCompruebaCifrado_U()
{
	char clave[128];
	ircsprintf(clave, "a%lu", Crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv->cambio));
	PropagaRegistro("S::L %s", clave);
	return 0;
}
int ISSigVDrop (char *nick)
{
	PropagaRegistro("N::%s::V", nick);
	return 0;
}
int ISSigSQL2()
{
	SQLNuevaTabla(IS_NOLINE, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"nolines varchar(5) default NULL, "
		"nota text,"
  		"KEY item (item) "
		");", PREFIJO, IS_NOLINE);
	SQLNuevaTabla(IS_DNS, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"dns varchar(255) default NULL, "
		"nota text,"
  		"KEY item (item) "
		");", PREFIJO, IS_DNS);
	return 0;
}
int ISSigSynch_U()
{
	if (!timercif)
		timercif = IniciaCrono(0, 86400, ISCompruebaCifrado_U, NULL);
	return 0;
}
int ISSigSockClose_U()
{
	if (timercif)
	{
		ApagaCrono(timercif);
		timercif = NULL;
	}
	return 0;
}
