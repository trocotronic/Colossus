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
BOTFUNC(OSBackup);
BOTFUNCHELP(OSHBackup);
BOTFUNC(OSRestaurar);
BOTFUNCHELP(OSHRestaurar);
BOTFUNC(OSPassFlood);
BOTFUNCHELP(OSHPassFlood);
BOTFUNC(OSServerDebug);
BOTFUNCHELP(OSHServerDebug);
EXTFUNC(OSOpers);

#define NS_OPT_UDB 0x80
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)

bCom operserv_coms[] = {
	{ "modos" , OSModos , N4 , "Fija los modos de operador que se obtiene automáticamente (BDD)." , OSHModos } ,
	{ "snomask" , OSSnomask , N4 , "Fija las máscaras de operador que se obtiene automáticamente (BDD)." , OSHSnomask } ,
	{ "optimizar" , OSOptimizar , N4 , "Optimiza la base de datos (BDD)." , OSHOptimizar } ,
	{ "backup" , OSBackup , N4 , "Realiza una copia de seguridad de los bloques (BDD)." , OSHBackup } ,
	{ "restaurar" , OSRestaurar , N4 , "Restaura una copia de seguridad realizada con el comando \00312backup\003." , OSHRestaurar } ,
	{ "pass-flood" , OSPassFlood , N4 , "Establece el número de intentos para poner una contraseña vía /nick nick:pass" , OSHPassFlood } ,
	{ "server-debug" , OSServerDebug , N4 , "Establece un servidor como debug" , OSHServerDebug } ,
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
	Responde(cl, CLI(operserv), "Fija los modos de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
	Responde(cl, CLI(operserv), "Estos modos son: \00312ohAOkNCWqHX");
	Responde(cl, CLI(operserv), "Serán otorgados de forma automática cada vez que el operador use su nick vía /nick nick:pass");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312MODOS operador [modos]");
	Responde(cl, CLI(operserv), "Si no se especifican modos, se borrarán los que pueda haber.");
	return 0;
}
BOTFUNCHELP(OSHSnomask)
{
	Responde(cl, CLI(operserv), "Fija las máscaras de operador que recibirá un operador de red añadido vía BDD (no por .conf).");
	Responde(cl, CLI(operserv), "Serán otorgadas de forma automática cada vez que el operador use su nick vía /nick nick:pass");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SNOMASK operador [snomasks]");
	Responde(cl, CLI(operserv), "Si no se especifican máscaras, se borrarán las que pueda haber.");
	return 0;
}
BOTFUNCHELP(OSHOptimizar)
{
	Responde(cl, CLI(operserv), "Optimiza las bases de datos.");
	Responde(cl, CLI(operserv), "Este comando elimina los registros duplicados o supérfluos que pueda haber en un archivo de la bdd.");
	Responde(cl, CLI(operserv), "Así se consigue reducir el tamaño del archivo.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312OPTIMIZA");
	return 0;
}
BOTFUNCHELP(OSHBackup)
{
	Responde(cl, CLI(operserv), "Realiza una copia de seguridad de todos los bloques UDB.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312BACKUP");
	return 0;
}
BOTFUNCHELP(OSHRestaurar)
{
	Responde(cl, CLI(operserv), "Restaura una copia de seguridad realizada con \00312BACKUP\003.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Si no se especifican parámetros, listará todas las copias de seguridad que estén disponibles.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312RESTAURAR [punto_restauración]");
	return 0;
}
BOTFUNCHELP(OSHPassFlood)
{
	Responde(cl, CLI(operserv), "Establece el número de intentos para intentar una contraseña vía /nick nick:pass");
	Responde(cl, CLI(operserv), "El formato que se usa es <v>:<s>, donde <v> es el número de intentos y <s> el número de segundos.");
	Responde(cl, CLI(operserv), "Por ejemplo, un valor de 2:60 significaría que el usuario puede usar /nick nick:pass 2 veces cada 60 segundos");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Sintaxis: \00312PASS-FLOOD <v>:<s>");
	return 0;
}
BOTFUNCHELP(OSHServerDebug)
{
	Responde(cl, CLI(operserv), "Fija un servidor como servidor debug.");
	Responde(cl, CLI(operserv), "Este servidor recibirá todos los cambios de modos realizados por UDB sobre un usuario.");
	Responde(cl, CLI(operserv), "En general no necesitará esta opción, salvo que decida conectar un servidor no-UDB y quiera procesar este tipo de datos.");
	Responde(cl, CLI(operserv), "Por ejemplo, si quiere unir a la red un servidor de estadísticas, puede serle útil esta opción.");
	Responde(cl, CLI(operserv), " ");
	Responde(cl, CLI(operserv), "Si utiliza este comando sobre un servidor que ya es debug, se le quitará esta opción. Si no es debug, se le añadirá.");
	Responde(cl, CLI(operserv), "Sintaxis: \00312SERVER-DEBUG nombre.del.servidor");
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
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("ohaAOkNCWqHX", *modos))
			{
				ircsprintf(buf, "El modo %c está prohibido. Sólo se permiten los modos ohaAOkNCWqHX.", *modos);
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
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nick [máscaras]");
		return 1;
	}
	if (!IsNickUDB(param[1]))
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
	if (params == 3)
	{
		char *modos;
		for (modos = param[2]; !BadPtr(modos); modos++)
		{
			if (!strchr("cefFGjknNoqsSv", *modos))
			{
				ircsprintf(buf, "La máscara %c está prohibido. Sólo se permiten las máscaras cefFGjknNoqsSv.", *modos);
				Responde(cl, CLI(operserv), OS_ERR_EMPT, buf);
				return 1;
			}
		}
		PropagaRegistro("N::%s::K %s", param[1], param[2]);
		Responde(cl, CLI(operserv), "Las máscaras de operador para \00312%s\003 se han puesto a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("N::%s::K", param[1]);
		Responde(cl, CLI(operserv), "Se han eliminado las máscaras de operador para \00312%s", param[1]);
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
		aux = CogeDeId(i);
		EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, aux->letra, hora);
		OptimizaBloque(aux);
		ActualizaGMT(aux, hora);
	}
	Responde(cl, CLI(operserv), "Se han optimizado todos los bloques");
	return 0;
}
BOTFUNC(OSBackup)
{
	u_int i;
	time_t hora = time(0);
	UDBloq *aux;
	strftime(buf, sizeof(buf), "%d%m%y-%H%M", gmtime(&hora));
	for (i = 0; i < BDD_TOTAL; i++)
	{
		aux = CogeDeId(i);
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
	Responde(cl, CLI(operserv), "Se ha realizado una copia de seguridad de todos los bloques");
	return 0;
}
BOTFUNC(OSRestaurar)
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
			Responde(cl, CLI(operserv), OS_ERR_EMPT, "No hay puntos de restauración");
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
		Responde(cl, CLI(operserv), OS_ERR_SNTX, fc->com, "el punto de restauración debe tener el formato de fecha dd/mm/yy hh:mm");
		return 1;
	}
	tshora = time(0);
	ircsprintf(tmp, "%02u%02u%02u-%02u%02u", dia, mes, ano, hora, min);
	for (i = 0; i < BDD_TOTAL; i++)
	{
		aux = CogeDeId(i);
		EnviaAServidor(":%s DB * RST %c %s %lu", me.nombre, aux->letra, tmp, tshora);
		switch (RestauraSeguridad(aux, tmp))
		{
			case -1:
			case -2:
				Responde(cl, CLI(operserv), OS_ERR_EMPT, "Este punto de restauración no existe");
				return 1;
			case -3:
				Responde(cl, CLI(operserv), OS_ERR_EMPT,"Ha ocurrido un error grave (zInflate)");
				return 1;
		}
	}
	Responde(cl, CLI(operserv), "El punto de restauración \00312%s %s\003 se ha restaurado con éxito", param[1], param[2]);
	return 0;
}
BOTFUNC(OSPassFlood)
{
	int v, s;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "<v>:<s>");
		return 1;
	}
	if (sscanf(param[1], "%i:%i", &v, &s) != 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_SNTX, fc->com, "PASS-FLOOD <v>:<s>. <v> y <s> deben ser segundos");
		return 1;
	}
	if (v < 1 || v > 60)
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "El número de intentos <v> debe estar entre 1-60 intentos");
		return 1;
	}
	if (s < 2 || s > 120)
	{
		Responde(cl, CLI(operserv), OS_ERR_EMPT, "El número de segundos <s> debe estar entre 2-120 segundos");
		return 1;
	}
	PropagaRegistro("S::F %i:%i", v, s);
	Responde(cl, CLI(operserv), "Se ha fijado el pass-flood en \00312%s", param[1]);
	return 0;
}
BOTFUNC(OSServerDebug)
{
	Udb *reg, *bloq;
	u_long val = 0L;
	if (params < 2)
	{
		Responde(cl, CLI(operserv), OS_ERR_PARA, fc->com, "nombre.del.servidor");
		return 1;
	}
	if ((reg = BuscaBloque(param[1], UDB_LINKS)))
	{
		if ((bloq = BuscaBloque(L_OPT, reg)))
			val = bloq->data_long;
	}
	if (val & L_OPT_DEBG)
	{
		if (val & ~L_OPT_DEBG)
			PropagaRegistro("L::%s::O %c%lu", param[1], CHAR_NUM, val & ~L_OPT_DEBG);
		else
			PropagaRegistro("L::%s::O", param[1]);
		Responde(cl, CLI(operserv), "Se ha quitado el servidor \00312%s\003 como servidor debug", param[1]);
	}
	else
	{
		PropagaRegistro("L::%s::O %c%lu", param[1], CHAR_NUM, val | C_OPT_PBAN);
		Responde(cl, CLI(operserv), "Se ha añadido el servidor \00312%s\003 como servidor debug", param[1]);
	}
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
