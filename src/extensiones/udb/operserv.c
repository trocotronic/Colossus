#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "bdd.h"
#include "modulos/nickserv.h"
#include "modulos/operserv.h"

OperServ *operserv = NULL;

BOTFUNC(OSModos);
BOTFUNCHELP(OSHModos);
BOTFUNC(OSSnomask);
BOTFUNCHELP(OSHSnomask);
BOTFUNC(OSOpt);
BOTFUNCHELP(OSHOptimiza);
EXTFUNC(OSOpers);

#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)
#define NS_OPT_UDB 0x1000

bCom operserv_coms[] = {
	{ "modos" , OSModos , N4 , "Fija los modos de operador que se obtiene autom�ticamente (BDD)." , OSHModos } ,
	{ "snomask" , OSSnomask , N4 , "Fija las m�scaras de operador que se obtiene autom�ticamente (BDD)." , OSHSnomask } ,
	{ "optimiza" , OSOpt , N4 , "Optimiza la base de datos (BDD)." , OSHOptimiza } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void CargaOperServ(Extension *ext)
{
	int i;
	if (ext->config)
	{
		Conf *cf;
		if ((cf = BuscaEntrada(ext->config, "OperServ")))
		{
			for (i = 0; i < cf->secciones; i++)
			{
				if (!strcmp(cf->seccion[i]->item, "funciones"))
					ProcesaComsMod(cf->seccion[i], operserv->hmod, operserv_coms);
				else if (!strcmp(cf->seccion[i]->item, "funcion"))
					SetComMod(cf->seccion[i], operserv->hmod, operserv_coms);
			}
		}
		else
			ProcesaComsMod(NULL, operserv->hmod, operserv_coms);
	}
	else
		ProcesaComsMod(NULL, operserv->hmod, operserv_coms);
	InsertaSenyalExt(5, OSOpers, ext);
}
void DescargaOperServ(Extension *ext)
{
	BorraSenyalExt(5, OSOpers, ext);
}
BOTFUNCHELP(OSHModos)
{
	Responde(cl, CLI(operserv), "Fija los modos de operador que recibir� un operador de red a�adido v�a BDD (no por .conf).");
	Responde(cl, CLI(operserv), "Estos modos son: \00312ohAOkNCWqHX");
	Responde(cl, CLI(operserv), "Ser�n otorgados de forma autom�tica cada vez que el operador use su nick v�a /nick nick:pass");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312MODOS operador [modos]");
	Responde(cl, CLI(operserv), "Si no se especifican modos, se borrar�n los que pueda haber.");
	return 0;
}
BOTFUNCHELP(OSHSnomask)
{
	Responde(cl, CLI(operserv), "Fija las m�scaras de operador que recibir� un operador de red a�adido v�a BDD (no por .conf).");
	Responde(cl, CLI(operserv), "Ser�n otorgadas de forma autom�tica cada vez que el operador use su nick v�a /nick nick:pass");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SNOMASK operador [snomasks]");
	Responde(cl, CLI(operserv), "Si no se especifican m�scaras, se borrar�n las que pueda haber.");
	return 0;
}
BOTFUNCHELP(OSHOptimiza)
{
	Responde(cl, CLI(operserv), "Optimiza las bases de datos.");
	Responde(cl, CLI(operserv), "Este comando elimina los registros duplicados o sup�rfluos que pueda haber en un archivo de la bdd.");
	Responde(cl, CLI(operserv), "As� se consigue reducir el tama�o del archivo.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312OPTIMIZA");
	return 0;
}
BOTFUNC(OSModos)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick [modos]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no est� migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("+ohaAOkNCWqHX", *modos))
			{
				ircsprintf(buf, "El modo %c est� prohibido. S�lo se permiten los modos ohaAOkNCWqHX.", *modos);
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
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick [m�scaras]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no est� migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("cefFGjknNoqsSv", *modos))
			{
				ircsprintf(buf, "La m�scara %c est� prohibido. S�lo se permiten las m�scaras cefFGjknNoqsSv.", *modos);
				Responde(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		PropagaRegistro("N::%s::K %s", param[1], param[2]);
		Responde(cl, CLI(operserv), "Las m�scaras de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("N::%s::K", param[1]);
		Responde(cl, CLI(operserv), "Se han eliminado las m�scaras de operador para \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(OSOpt)
{
	u_int i;
	time_t hora = time(0);
	Udb *aux;
	for (i = 0; i < BDD_TOTAL; i++)
	{
		aux = IdAUdb(i);
		EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, bloques[i], hora);
		Optimiza(aux);
		ActualizaGMT(aux, hora);
	}
	Responde(cl, CLI(operserv), "Se han optimizado todos los bloques");
	return 0;
}
EXTFUNC(OSOpers)
{
	int nivel;
	if (mod != operserv->hmod)
		return 1;
	if (params >= 3)
	{
		if ((nivel = BuscaOpt(param[2], NivelesBDD)))
			PropagaRegistro("N::%s::O %c%i", param[1], CHAR_NUM, nivel);
	}
	else
	{
		if ((nivel = NivelOperBdd(param[1])))
			PropagaRegistro("N::%s::O", param[1]);
	}
	return 0;
}
