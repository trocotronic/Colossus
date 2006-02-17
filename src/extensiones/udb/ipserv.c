#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/ipserv.h"
#include "modulos/nickserv.h"
#include "bdd.h"

IpServ *ipserv = NULL;

BOTFUNC(ISOpts);
BOTFUNCHELP(ISHSet);
BOTFUNC(ISDns);
BOTFUNCHELP(ISHDns);
BOTFUNC(ISNolines);
BOTFUNCHELP(ISHNolines);
EXTFUNC(ISSetipv);
EXTFUNC(ISClones);

int ISSigEOS	();
int ISSigVDrop	(char *);
int ISCompruebaCifrado();
extern int ISSigDrop	(char *);
extern int ISSigUmode		(Cliente *, char *);
extern int ISSigNick		(Cliente *, int);
extern int ISSigIdOk 		(Cliente *);
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)
#define NS_OPT_UDB 0x1000

bCom ipserv_coms[] = {
	{ "set" , ISOpts , N4 , "Fija algunos parámetros de la red." } ,
	{ "dns" , ISDns , N4 , "Establece una resolución inversa de una ip." } ,
	{ "nolines" , ISNolines , N3 , "Controla las excepciones para una ip." } ,
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
	BorraSenyal(SIGN_POST_NICK, ISSigNick);
	BorraSenyal(SIGN_UMODE, ISSigUmode);
	BorraSenyal(NS_SIGN_IDOK, ISSigIdOk);
	InsertaSenyal(SIGN_EOS, ISSigEOS);
	InsertaSenyal(IS_SIGN_DROP, ISSigVDrop);
	InsertaSenyalExt(1, ISSetipv, ext);
	InsertaSenyalExt(3, ISClones, ext);
	/*InsertaSenyalExt(16, CSLiberar, ext);
	InsertaSenyalExt(17, CSForbid, ext);
	InsertaSenyalExt(18, CSUnforbid, ext);
	InsertaSenyalExt(20, CSRegister, ext);*/
}
void DescargaIpServ(Extension *ext)
{
	InsertaSenyal(SIGN_POST_NICK, ISSigNick);
	InsertaSenyal(SIGN_UMODE, ISSigUmode);
	InsertaSenyal(NS_SIGN_IDOK, ISSigIdOk);
	BorraSenyal(SIGN_EOS, ISSigEOS);
	BorraSenyal(IS_SIGN_DROP, ISSigVDrop);
	BorraSenyalExt(1, ISSetipv, ext);
	BorraSenyalExt(3, ISClones, ext);
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
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "SET quit_ips|quit_clones|clones valor");
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
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "DNS ip [host]");
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
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "NOLINES ip [G|Z|Q|S|T]");
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
EXTFUNC(ISSetipv)
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
EXTFUNC(ISClones)
{
	if (mod != ipserv->hmod)
		return 1;
	if (params >= 3)
		PropagaRegistro("I::%s::S %c%s", param[1], CHAR_NUM, param[2]);
	else
		PropagaRegistro("I::%s::S", param[1]);
	return 0;
}
int ISSigEOS()
{
	PropagaRegistro("S::I %s", ipserv->hmod->mascara);
	PropagaRegistro("S::J %s", ipserv->sufijo);
	ISCompruebaCifrado();
	return 0;
}
int ISCompruebaCifrado()
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
