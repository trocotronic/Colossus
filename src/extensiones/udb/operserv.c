/*
 * $Id: operserv.c,v 1.9 2007/02/18 18:58:53 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#endif
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
BOTFUNC(OSOptimizar);
BOTFUNCHELP(OSHOptimizar);
BOTFUNC(OSBackupUDB);
BOTFUNCHELP(OSHBackupUDB);
BOTFUNC(OSRestaurarUDB);
BOTFUNCHELP(OSHRestaurarUDB);
BOTFUNC(OSSetUDB);
BOTFUNCHELP(OSHSetUDB);
BOTFUNC(OSLines);
BOTFUNCHELP(OSHLines);
BOTFUNC(OSPropaga);
BOTFUNCHELP(OSHPropaga);
EXTFUNC(OSOpers);

#define NS_OPT_UDB 0x80
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)

bCom operserv_coms[] = {
	{ "modos" , OSModos , N4 , "Fija los modos de operador que se obtiene autom�ticamente (BDD)." , OSHModos } ,
	{ "snomask" , OSSnomask , N4 , "Fija las m�scaras de operador que se obtiene autom�ticamente (BDD)." , OSHSnomask } ,
	{ "optimizar" , OSOptimizar , N4 , "Optimiza la base de datos (BDD)." , OSHOptimizar } ,
	{ "backupudb" , OSBackupUDB , N4 , "Realiza una copia de seguridad de los bloques UDB." , OSHBackupUDB } ,
	{ "restaurarudb" , OSRestaurarUDB , N4 , "Restaura una copia de seguridad realizada con el comando \00312backupudb\003." , OSHRestaurarUDB } ,
	{ "setudb" , OSSetUDB , N4 , "Fija distintos par�metros UDB de red." , OSHSetUDB } ,
	{ "lines" , OSLines , N4 , "Propaga por la red *lines (spamfilters, glines, zlines, shuns y qlines)" , OSHLines } ,
	{ "propaga" , OSPropaga , N4 , "Inserta una l�nea UDB en la red" , OSHPropaga } ,
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
BOTFUNCHELP(OSHOptimizar)
{
	Responde(cl, CLI(operserv), "Optimiza las bases de datos.");
	Responde(cl, CLI(operserv), "Este comando elimina los registros duplicados o sup�rfluos que pueda haber en un archivo de la bdd.");
	Responde(cl, CLI(operserv), "As� se consigue reducir el tama�o del archivo.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312OPTIMIZA");
	return 0;
}
BOTFUNCHELP(OSHBackupUDB)
{
	Responde(cl, CLI(operserv), "Realiza una copia de seguridad de todos los bloques UDB.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312BACKUPUDB");
	return 0;
}
BOTFUNCHELP(OSHRestaurarUDB)
{
	Responde(cl, CLI(operserv), "Restaura una copia de seguridad realizada con \00312BACKUPUDB\003.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Si no se especifican par�metros, listar� todas las copias de seguridad que est�n disponibles.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312RESTAURARUDB [punto_restauraci�n]");
	return 0;
}
BOTFUNCHELP(OSHSetUDB)
{
	if (params < 3)
	{
		Responde(cl, CLI(operserv), "Fija distintos par�metros UDB de red.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Opciones disponibles:");
		Responde(cl, CLI(operserv), "\00312PASS-FLOOD\003 Establece el n�mero de intentos para intentar una contrase�a v�a /nick nick:pass");
		Responde(cl, CLI(operserv), "\00312SERVER-DEBUG\003 Fija un servidor como servidor debug.");
		Responde(cl, CLI(operserv), "\00312CLIENTES\003 Permite o no la conexi�n de clientes en un servidor no-UDB, leaf y uline.");
		Responde(cl, CLI(operserv), "\00312GLOBALSERV\003 Establece el nick!user@host para el env�o de globales v�a /msg $*.");
		Responde(cl, CLI(operserv), "\00312PREFIJOS\003 Fija los prefijos para los modos +qaohv.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312SETUDB <opci�n> [par�metros]");
		Responde(cl, CLI(operserv), "Para m�s informaci�n, \00312/msg %s %s SETUDB <opci�n>", operserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[2], "PASS-FLOOD"))
	{
		Responde(cl, CLI(operserv), "Establece el n�mero de intentos para intentar una contrase�a v�a /nick nick:pass");
		Responde(cl, CLI(operserv), "El formato que se usa es <v>:<s>, donde <v> es el n�mero de intentos y <s> el n�mero de segundos.");
		Responde(cl, CLI(operserv), "Por ejemplo, un valor de 2:60 significar�a que el usuario puede usar /nick nick:pass 2 veces cada 60 segundos");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312PASS-FLOOD <v>:<s>");
	}
	else if (!strcasecmp(param[2], "SERVER-DEBUG"))
	{
		Responde(cl, CLI(operserv), "Fija un servidor como servidor debug.");
		Responde(cl, CLI(operserv), "Este servidor recibir� todos los cambios de modos realizados por UDB sobre un usuario.");
		Responde(cl, CLI(operserv), "En general no necesitar� esta opci�n, salvo que decida conectar un servidor no-UDB y quiera procesar este tipo de datos.");
		Responde(cl, CLI(operserv), "Por ejemplo, si quiere unir a la red un servidor de estad�sticas, puede serle �til esta opci�n.");
		Responde(cl, CLI(operserv), "NOTA: NO UTILICE ESTA OPCI�N CON UN SERVIDOR UNREALIRCD O CON COLOSSUS.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312SERVER-DEBUG nombre.del.servidor ON|OFF");
	}
	else if (!strcasecmp(param[2], "CLIENTES"))
	{
		Responde(cl, CLI(operserv), "Los servidores que no sean UDB s�lo pueden conectarse a la red si son leafs (es decir, no son hubs).");
		Responde(cl, CLI(operserv), "No obstante, estos servidores no-UDB leafs no pueden introducir clientes por motivos de desincronizaci�n.");
		Responde(cl, CLI(operserv), "A�n as�, es posible que sea necesario que conecten clientes virtuales. Esto se produce cuando se conectan servidores como servicios o estad�sticas.");
		Responde(cl, CLI(operserv), "Es muy importante que este valor s�lo se active si realmente se sabe que el servidor no-UDB no causar� desincronizaci�n.");
		Responde(cl, CLI(operserv), "\0034TENGA CUIDADO CON ESTA OPCI�N");
		Responde(cl, CLI(operserv), "Con esta opci�n activa, un servidor de estad�sticas podr�a introducir clientes, siempre y cuando est� configurado como uline.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312CLIENTES nombre.del.servidor ON|OFF");
	}
	else if (!strcasecmp(param[2], "GLOBALSERV"))
	{
		Responde(cl, CLI(operserv), "Fija el par�metro GlobalServ de UDB.");
		Responde(cl, CLI(operserv), "Determina la m�scara con la que se mandan los globales v�a /msg $*");
		Responde(cl, CLI(operserv), "Debe ser de la forma nick!user@host");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312GLOBALSERV nick!user@host");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Ejemplo: \00312GLOBALSERV GlobalServ!colossus@servicios.colossus");
	}
	else if (!strcasecmp(param[2], "PREFIJOS"))
	{
		Responde(cl, CLI(operserv), "Fija los prefijos para los modos +qaohv");
		Responde(cl, CLI(operserv), "Deben fijarse con orden qaohv.");
		Responde(cl, CLI(operserv), "Por defecto, se utiliza el valor '~&@%+'");
		Responde(cl, CLI(operserv), "Si no se especifica valor, se utilizan por defecto.");
		Responde(cl, CLI(operserv), " ");
		Responde(cl, CLI(operserv), "Sintaxis: \00312PREFIJOS qaohv");
	}
	else
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Opci�n desconocida.");
	return 0;
}
BOTFUNCHELP(OSHPassFlood)
{
	Responde(cl, CLI(operserv), "Establece el n�mero de intentos para intentar una contrase�a v�a /nick nick:pass");
	Responde(cl, CLI(operserv), "El formato que se usa es <v>:<s>, donde <v> es el n�mero de intentos y <s> el n�mero de segundos.");
	Responde(cl, CLI(operserv), "Por ejemplo, un valor de 2:60 significar�a que el usuario puede usar /nick nick:pass 2 veces cada 60 segundos");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312PASS-FLOOD <v>:<s>");
	return 0;
}
BOTFUNCHELP(OSHServerDebug)
{
	Responde(cl, CLI(operserv), "Fija un servidor como servidor debug.");
	Responde(cl, CLI(operserv), "Este servidor recibir� todos los cambios de modos realizados por UDB sobre un usuario.");
	Responde(cl, CLI(operserv), "En general no necesitar� esta opci�n, salvo que decida conectar un servidor no-UDB y quiera procesar este tipo de datos.");
	Responde(cl, CLI(operserv), "Por ejemplo, si quiere unir a la red un servidor de estad�sticas, puede serle �til esta opci�n.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Si utiliza este comando sobre un servidor que ya es debug, se le quitar� esta opci�n. Si no es debug, se le a�adir�.");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SERVER-DEBUG nombre.del.servidor");
	return 0;
}
BOTFUNCHELP(OSHLines)
{
	Responde(cl, CLI(operserv), "Propaga *lines por la red mediante el bloque K.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis:");
	Responde(cl, CLI(operserv), "\00312GLINE ip|host motivo");
	Responde(cl, CLI(operserv), "A�ade una gline permanente por UDB.");
	Responde(cl, CLI(operserv), "\00312GLINE ip|host");
	Responde(cl, CLI(operserv), "Elimina esa gline");
	return 0;
}
BOTFUNCHELP(OSHPropaga)
{
	Responde(cl, CLI(operserv), "Inserta una l�nea UDB cualquiera por la red.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312PROPAGA bloque::campo::campo contenido");
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
			if (!strchr("ohaAOkNCWqHX", *modos))
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
BOTFUNC(OSOptimizar)
{
	u_int i;
	time_t hora = time(0);
	UDBloq *aux;
	for (i = 0; i < BDD_TOTAL; i++)
	{
		if ((aux = CogeDeId(i)))
		{
			EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, aux->letra, hora);
			OptimizaBloque(aux);
			ActualizaGMT(aux, hora);
		}
	}
	Responde(cl, CLI(operserv), "Se han optimizado todos los bloques");
	return 0;
}
BOTFUNC(OSBackupUDB)
{
	u_int i;
	time_t hora = time(0);
	UDBloq *aux;
	strftime(buf, sizeof(buf), "%d%m%y-%H%M", gmtime(&hora));
	for (i = 0; i < BDD_TOTAL; i++)
	{
		if ((aux = CogeDeId(i)))
		{
			EnviaAServidor(":%s DB * BCK %c %s", me.nombre, aux->letra, buf);
			switch (CopiaSeguridad(aux, buf))
			{
				case -1:
				case -2:
					Responde(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error grave (fopen)");
					return 1;
				case -3:
					Responde(cl, CLI(operserv), OS_ERR_EMPT,"Ha ocurrido un error grave (zDeflate)");
					return 1;
			}
		}
	}
	Responde(cl, CLI(operserv), "Se ha realizado una copia de seguridad de todos los bloques");
	return 0;
}
BOTFUNC(OSRestaurarUDB)
{
	u_int i;
	time_t tshora = time(0);
	UDBloq *aux;
	char tmp[64];
	u_int dia, mes, ano, hora, min;
	if (params < 3)
	{
		Directorio dir;
		char *nombre;
		if (!(dir = AbreDirectorio(DB_DIR_BCK)))
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No hay puntos de restauraci�n");
			return 1;
		}
		Responde(cl, CLI(operserv), "*** Listando copias de seguridad ***");
		while ((nombre = LeeDirectorio(dir)))
		{
			if ((sscanf(nombre, "C%02u%02u%02u-%02u%02u", &dia, &mes, &ano, &hora, &min)) == 5)
			{
				ircsprintf(buf, "\00312%02u\003/\00312%02u\003/\00312%02u %02u\003:\00312%02u", dia, mes, ano, hora, min);
				Responde(cl, CLI(operserv), buf);
			}
		}
		CierraDirectorio(dir);
		return 0;
	}
	if (sscanf(param[1], "%02u/%02u/%02u", &dia, &mes, &ano) != 3 || sscanf(param[2], "%02u:%02u", &hora, &min) != 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, "El punto de restauraci�n debe tener el formato de fecha dd/mm/yy hh:mm");
		return 1;
	}
	tshora = time(0);
	ircsprintf(tmp, "%02u%02u%02u-%02u%02u", dia, mes, ano, hora, min);
	for (i = 0; i < BDD_TOTAL; i++)
	{
		if ((aux = CogeDeId(i)))
		{
			EnviaAServidor(":%s DB * RST %c %s %lu", me.nombre, aux->letra, tmp, tshora);
			switch (RestauraSeguridad(aux, tmp))
			{
				case -1:
				case -2:
					Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este punto de restauraci�n no existe");
					return 1;
				case -3:
					Responde(cl, CLI(operserv), OS_ERR_EMPT, "Ha ocurrido un error grave (zInflate)");
					return 1;
			}
		}
	}
	Responde(cl, CLI(operserv), "El punto de restauraci�n \00312%s %s\003 se ha restaurado con �xito", param[1], param[2]);
	return 0;
}
BOTFUNC(OSSetUDB)
{
	Udb *reg, *bloq;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "opci�n [par�metros]");
		return 1;
	}
	if (!strcasecmp(param[1], "PASS-FLOOD"))
	{
		int v, s;
		if (params < 3)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "PASS-FLOOD <v>:<s>");
			return 1;
		}
		if (sscanf(param[2], "%i:%i", &v, &s) != 2)
		{
			Responde(cl, CLI(operserv), OS_ERR_SNTX, "PASS-FLOOD <v>:<s>. <v> y <s> deben ser segundos");
			return 1;
		}
		if (v < 1 || v > 60)
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "El n�mero de intentos <v> debe estar entre 1-60 intentos");
			return 1;
		}
		if (s < 2 || s > 120)
		{
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "El n�mero de segundos <s> debe estar entre 2-120 segundos");
			return 1;
		}
		PropagaRegistro("S::F %i:%i", v, s);
		Responde(cl, CLI(operserv), "Se ha fijado el pass-flood en \00312%s", param[2]);
	}
	else if (!strcasecmp(param[1], "SERVER-DEBUG") || !strcasecmp(param[1], "CLIENTES"))
	{
		u_long val = 0L;
		int opt;
		char tmp[BUFSIZE];
		if (ToLower(*param[1]) == 's')
			opt = L_OPT_DEBG;
		else
			opt = L_OPT_CLNT;
		ircsprintf(tmp, "%s nombre.del.servidor ON|OFF", strtoupper(param[1]));
		if (params < 4)
		{
			Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, tmp);
			return 1;
		}
		if ((reg = BuscaBloque(param[2], UDB_LINKS)))
		{
			if ((bloq = BuscaBloque(L_OPT, reg)))
				val = bloq->data_long;
		}
		if (!strcasecmp(param[3], "ON"))
			PropagaRegistro("L::%s::O %c%lu", param[2], CHAR_NUM, val | opt);
		else if (!strcasecmp(param[3], "OFF"))
		{
			if (val & ~opt)
				PropagaRegistro("L::%s::O %c%lu", param[2], CHAR_NUM, val & ~opt);
			else
				PropagaRegistro("L::%s::O", param[2]);
		}
		else
		{
			Responde(cl, CLI(operserv), OS_ERR_SNTX, tmp);
			return 1;
		}
		Responde(cl, CLI(operserv), "El par�metro \00312%s\003 para el servidor \00312%s\003 est� en \00312%s", strtoupper(param[1]), param[2], param[3]);
	}
	else if (!strcasecmp(param[1], "GLOBALSERV"))
	{
		if (params < 3)
		{
			PropagaRegistro("S::G");
			Responde(cl, CLI(operserv), "Se ha eliminado el par�metro GlobalServ de UDB.");
		}
		else
		{
			PropagaRegistro("S::G %s", param[2]);
			Responde(cl, CLI(operserv), "Se ha fijado el par�metro GlobalServ de UDB a \00312%s", param[2]);
		}
	}
	else if (!strcasecmp(param[1], "PREFIJOS"))
	{
		if (params < 3)
		{
			PropagaRegistro("S::P");
			Responde(cl, CLI(operserv), "Se han eliminado los prefijos.");
		}
		else
		{
			PropagaRegistro("S::P %s", param[2]);
			Responde(cl, CLI(operserv), "Se han fijado los prefijos a \00312%s", param[2]);
		}
	}
	return 0;
}
BOTFUNC(OSLines)
{
	if (params < 3)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "*line par�metros");
		return 1;
	}
	if (!strcasecmp(param[1], "GLINE"))
	{
		if (params > 3) /* a�ade */
			PropagaRegistro("K::G::%s::R %s", param[2], Unifica(param, params, 3, -1));
		else
			PropagaRegistro("K::G::%s", param[2]);
	}
	return 0;
}
BOTFUNC(OSPropaga)
{
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "l�nea");
		return 1;
	}
	PropagaRegistro(Unifica(param, params, 1, -1));
	Responde(cl, CLI(operserv), "L�nea insertada con �xito.");
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
		if ((nivel = LevelOperUdb(param[1])))
			PropagaRegistro("N::%s::O", param[1]);
	}
	return 0;
}
