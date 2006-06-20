
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/probserv.h"

ProbServ *probserv = NULL;

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
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuraci�n para %s", mod->archivo, MOD_INFO(ProbServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(ProbServ).nombre);
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
					Error("[%s] La configuraci�n de %s no ha pasado el test", mod->archivo, MOD_INFO(ProbServ).nombre);
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
	return 0;
}
void PBSSet(Conf *config, Modulo *mod)
{
	int i;
	if (!probserv)
		BMalloc(probserv, ProbServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
			ProcesaComsMod(config->seccion[i], mod, probserv_coms);
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
		Responde(cl, CLI(probserv), PBS_ERR_EMPT, "Opci�n desconocida.");
	return 0;
}
BOTFUNCHELP(PBSHSql)
{
	Responde(cl, CLI(probserv), "Inserta un registro de prueba");
	Responde(cl, CLI(probserv), " ");
	Responde(cl, CLI(probserv), "Sintaxis: \00312SQL registro [valor]");
	Responde(cl, CLI(probserv), "Si no hay valor, el registro se borrar�");
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
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"campo1 varchar(255) default NULL, "
			");", PREFIJO, PBS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, PBS_SQL);
	}
	SQLCargaTablas();
	return 0;
}