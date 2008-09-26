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

int ISSigEOS_U	();
int ISSigVDrop	(char *);
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
	InsertaSenyalExt(1, ISSetipv_U, ext);
	InsertaSenyalExt(3, ISClones_U, ext);
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
	BorraSenyalExt(1, ISSetipv_U, ext);
	BorraSenyalExt(3, ISClones_U, ext);
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
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "ip [host]");
		return 1;
	}
	if (!EsIp(param[1]))
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "No parece ser una ip");
		return 1;
	}
	if (params == 3)
	{
		PropagaRegistro("I::%s::H %s", param[1], param[2]);
		Responde(cl, CLI(ipserv), "Se ha cambiado la resolución de la ip \00312%s\003 a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("I::%s::H", param[1]);
		Responde(cl, CLI(ipserv), "Se ha eliminado al resolución de la ip \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(ISNolines)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "ip [G|Z|Q|S|T]");
		return 1;
	}
	if (params == 3)
	{
		char *c;
		for (c = param[2]; !BadPtr(c); c++)
		{
			if (!strchr("GZQST", *c))
			{
				Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Sólo se aceptan las nolines G|Z|Q|S|T");
				return 1;
			}
		}
		PropagaRegistro("I::%s::E %s", param[1], param[2]);
		Responde(cl, CLI(ipserv), "Se han cambiado las excepciones para la ip \00312%s\003 a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("I::%s::E", param[1]);
		Responde(cl, CLI(ipserv), "Se han eliminado las excepciones para la ip \00312%s", param[1]);
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
