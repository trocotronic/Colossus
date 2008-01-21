	
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/probserv.h"

ProbServ *probserv = NULL;
char *tabla = NULL;

BOTFUNC(PBSHelp);
BOTFUNC(PBSSql);
BOTFUNCHELP(PBSHSql);

int PBSSigSQL();

/* tabla de comandos */
static bCom probserv_coms[] = {
	{ "help", PBSHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "sql" , PBSSql , N3 , "Inserta o borra un registro." , PBSHSql } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void PBSSet(Conf *, Modulo *);
int PBSTest(Conf *, int *);

ModInfo MOD_INFO(ProbServ) = {
	"ProbServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(ProbServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, MOD_INFO(ProbServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(ProbServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(ProbServ).nombre))
			{
				if (!PBSTest(modulo.seccion[0], &errores))
					PBSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(ProbServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(ProbServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	return errores;
}
int MOD_DESCARGA(ProbServ)()
{
	BorraSenyal(SIGN_SQL, PBSSigSQL);
	BotUnset(probserv);
	return 0;
}
int PBSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], probserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
	return 0;
}
void PBSSet(Conf *config, Modulo *mod)
{
	int i;
	if (!probserv)
		probserv = BMalloc(ProbServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
			ProcesaComsMod(config->seccion[i], mod, probserv_coms);
		if (!strcmp(config->seccion[i]->item, "tabla"))
			ircstrdup(tabla, config->seccion[i]->valor);
	}
	InsertaSenyal(SIGN_SQL, PBSSigSQL);
	BotSet(probserv);
}
BOTFUNC(PBSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(probserv), "\00312%s\003 es un bot ejemplo.", probserv->hmod->nick);
		Responde(cl, CLI(probserv), " ");
		Responde(cl, CLI(probserv), "Comandos:");
		ListaDescrips(probserv->hmod, cl);
	}
	else if (!MuestraAyudaComando(cl, param[1], probserv->hmod, param, params))
		Responde(cl, CLI(probserv), PBS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNCHELP(PBSHSql)
{
	Responde(cl, CLI(probserv), "Inserta un registro de prueba");
	Responde(cl, CLI(probserv), " ");
	Responde(cl, CLI(probserv), "Sintaxis: \00312SQL registro [valor]");
	Responde(cl, CLI(probserv), "Si no hay valor, el registro se borrará");
	return 0;
}
BOTFUNC(PBSSql)
{
	if (params < 2)
	{
		Responde(cl, CLI(probserv), PBS_ERR_PARA, fc->com, "registro [valor]");
		return 1;
	}
	if (param[2])
	{
		SQLInserta(PBS_SQL, param[1], "campo1", param[2]);
		Responde(cl, CLI(probserv), "Registro insertado");
	}
	else
	{
		SQLBorra(PBS_SQL, param[1]);
		Responde(cl, CLI(probserv), "Registro borrado");
	}
	return 0;
}
int PBSSigSQL()
{
	if (!SQLEsTabla(PBS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"campo1 varchar(255) default NULL, "
  			"KEY item (item) "
			");", PREFIJO, PBS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, PBS_SQL);
	}
	return 0;
}
