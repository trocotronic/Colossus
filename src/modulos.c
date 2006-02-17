/*
 * $Id: modulos.c,v 1.17 2006-02-17 19:47:34 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
const char *ErrorDl(void);
#endif
int id = 1;

Nivel *niveles[MAX_NIVS];
int nivs = 0;

Modulo *modulos = NULL;
Modulo *CreaModulo(char *modulo)
{
	Recurso hmod;
	char archivo[128], tmppath[PMAX];
	int (*mod_carga)(Modulo *), (*mod_descarga)(Modulo *);
	ModInfo *inf;
	if ((hmod = CopiaDll(modulo, archivo, tmppath)))
	{
		Modulo *mod;
		irc_dlsym(hmod, "Mod_Carga", mod_carga);
		if (!mod_carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Carga)", archivo);
			irc_dlclose(hmod);
			return NULL;
		}
		irc_dlsym(hmod, "Mod_Descarga", mod_descarga);
		if (!mod_descarga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Descarga)", archivo);
			irc_dlclose(hmod);
			return NULL;
		}
		irc_dlsym(hmod, "Mod_Info", inf);
		if (!inf || !inf->nombre)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Info)", archivo);
			irc_dlclose(hmod);
			return NULL;
		}
		BMalloc(mod, Modulo);
		mod->archivo = strdup(modulo);
		mod->tmparchivo = strdup(tmppath);
		mod->hmod = hmod;
		mod->id = id;
		mod->carga = mod_carga;
		mod->descarga = mod_descarga;
		mod->info = inf;
		id <<= 1;
		if (!modulos)
			modulos = mod;
		else
		{
			Modulo *aux;
			for (aux = modulos; aux; aux = aux->sig)
			{
				if (!aux->sig)
				{
					aux->sig = mod;
					break;
				}
			}
		}
		return mod;
	}
	return NULL;
}
void DescargaModulo(Modulo *ex)
{
	DesconectaBot(ex->nick, "Refrescando");
	if (ex->descarga)
		(*ex->descarga)(ex);
	ircfree(ex->archivo);
	ircfree(ex->tmparchivo);
	BorraItem(ex, modulos);
	ex->comandos = 0; /* hay que vaciarlo! */
	irc_dlclose(ex->hmod);
	Free(ex);
}
void DescargaModulos()
{
	Modulo *ex, *sig;
	for (ex = modulos; ex; ex = sig)
	{
		sig = ex->sig;
		DescargaModulo(ex);
	}
	id = 1;
}
Modulo *BuscaModulo(char *nick, Modulo *donde)
{
	Modulo *ex;
	if (!donde)
		return NULL;
	for (ex = donde; ex; ex = ex->sig)
	{
		if (!strcasecmp(nick, ex->nick))
			return ex;
	}
	return NULL;
}
#ifdef _WIN32
const char *ErrorDl(void)
{
	static char errbuf[513];
	DWORD err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, errbuf, 512, NULL);
	return errbuf;
}
#endif
/*!
 * @desc: Busca si una función está cargada mediante su comando.
 * @params: $com [in] Comando a buscar.
 	    $mod [in] Módulo al que buscar.
 * @ret: Devuelve la estructura del comando.
 * @cat: Modulos
 * @ver: BuscaFuncion, ParseaConfiguracion, BuscaComMod, TieneNivel
 !*/
Funcion *BuscaFuncion(char *com, Modulo *mod)
{
	int i;
	for (i = 0; i < mod->comandos; i++)
	{
		if (!strcasecmp(mod->comando[i]->com, com))
			return mod->comando[i];
	}
	return NULL;
}
/*!
 * @desc: Busca el nombre de una función dentro de una lista estática de comandos.
 * @params: $coms [in] Lista de comandos.
 	    $func [in] Nombre de la función.
 * @cat: Modulos
 * @ret: Devuelve el comando de la lista estática asociado a esa función. NULL si no existe en la lista.
 * @ver: BuscaFuncion ParseaConfiguracion TieneNivel
 !*/
bCom *BuscaComMod(bCom *coms, char *func)
{
	bCom *cm;
	cm = coms;
	while (cm && cm->com != 0x0)
	{
		if (!strcasecmp(cm->com, func))
			return cm;
		cm++;
	}
	return NULL;
}
/*!
 * @desc: Busca la ayuda asociada a un comando. Si existe, se la muestra al usuario.
 * @params: $cl [in] Cliente a informar.
		$com [in] Comando a busca.
		$mod [in] Módulo al que hace referencia.
		$param [in] Listado de parámetros enviados al privado.
		$params [in] Número de elementos de la lista de parámetros.
 * @cat: Modulos
 * @ret: Devuelve 1 si existe una función de ayuda asociada a ese comando. 0, si no.
 * @ver: ListaDescrips
 !*/	
int MuestraAyudaComando(Cliente *cl, char *com, Modulo *mod, char **param, int params)
{
	Funcion *func;
	if ((func = BuscaFuncion(com, mod)))
	{
		if (func->func_help && TieneNivel(cl, func->com, mod, NULL))
		{
			if (params < 3)
			{
				Responde(cl, mod->cl, "\00312%s", func->com);
				Responde(cl, mod->cl, " ");
			}
			func->func_help(cl, param, params);
			return 1;
		}
	}
	return 0;
}
/*!
 * @desc: Comprueba si un cliente tiene nivel para ejecutar una función.
 * @params: $cl [in] Cliente.
 	    $func [in] Nombre del comando que ha usado el cliente.
 	    $mod [in] Módulo que ha llamado el cliente.
 	    $ex [out] Vale 1 si esa función existe. 0, si no existe.
 * @cat: Modulos
 * @ver: BuscaComMod ParseaConfiguracion BuscaFuncion
 * @ret: Devuelve la dirección de la función que se ejecutará. NULL si no existe o si no tiene nivel.
 !*/
Funcion *TieneNivel(Cliente *cl, char *func, Modulo *mod, char *ex)
{
	Funcion *cm;
	if (ex)
		*ex = 0;
	if ((cm = BuscaFuncion(func, mod)))
	{
		if (ex)
			*ex = 1;
		if (cl->nivel >= cm->nivel)
			return cm;
	}
	return NULL;
}
int BuscaOptNiv(char *nombre)
{
	Nivel *niv;
	if ((niv = BuscaNivel(nombre)))
		return niv->nivel;
	return 0;
}
Funcion *CreaCom(bCom *padre)
{
	Funcion *cm = NULL;
	BMalloc(cm, Funcion);
	if (padre)
	{
		cm->func = padre->func;
		cm->com = strdup(strtoupper(padre->com));
		cm->nivel = padre->nivel;
		cm->descrip = padre->descrip;
		cm->func_help = padre->func_help;
	}
	return cm;
}
/*!
 * @desc: Parsea el bloque funciones { } con las funciones soportadas por el módulo.
 * @params: $config [in] Rama o bloque de funciones del archivo de configuración. Llamado por ParseaConfiguracion.
		Si es NULL, cargará todos los comandos que se pasen por el tercer parámetro.
 	    $mod [in] Recurso del módulo a cargar estos comandos.
 	    $coms [in] Lista completa de los comandos soportados por el módulo.
 * @cat: Modulos
 * @ver: ParseaConfiguracion TieneNivel BuscaFuncion BuscaComMod
 !*/
 
int ProcesaComsMod(Conf *config, Modulo *mod, bCom *coms)
{
	int p, errores = 0;
	bCom *cm = NULL;
	if (!mod)
		return 1;
	if (config)
	{
		for (p = 0; p < config->secciones; p++)
		{
			if ((cm = BuscaComMod(coms, config->seccion[p]->item)))
			{
				if (!BuscaFuncion(cm->com, mod))
					mod->comando[mod->comandos++] = CreaCom(cm);
			}
			else
			{
				Error("[%s:%i] No se ha encontrado la funcion %s", config->archivo, config->seccion[p]->linea, config->seccion[p]->item);
				errores++;
			}
		}
	}
	else
	{
		for (cm = coms; cm && cm->com; cm++)
			mod->comando[mod->comandos++] = CreaCom(cm);
	}
	return errores;
}

/*!
 * @desc: Testea un bloque de configuración correspondiente a una funcion { }
 * @params: $config [in] Bloque de configuración.
 	    $coms [in] Lista estática que contiene todos los comandos soportados.
 	    $avisa [in] Si es 1, emitirá una ventana de aviso en caso de que haya un error o advertencia.
 * @ret: Devuelve el número de errores.
 * @cat: Modulos
 * @ver: ParseaConfiguracion SetComMod
 !*/
int TestComMod(Conf *config, bCom *coms, char avisa)
{
	int errores = 0;
	FILE *fp;
	if (!config->data)
	{
		if (avisa)
			Error("[%s:%i] Falta nombre de función", config->archivo, config->linea);
		return 1;
	}
	if (BuscaComMod(coms, config->data) || (fp = fopen(config->data, "r")))
	{
		int p;
		for (p = 0; p < config->secciones; p++)
		{
			if (!strcmp(config->seccion[p]->item, "nivel"))
			{
				if (BuscaOptNiv(config->seccion[p]->data) < 0)
				{
					if (avisa)
						Error("[%s:%i] No se encuentra el nivel %s", config->archivo, config->linea, config->seccion[p]->data);
					errores++;
				}
			}
			else if (!strcmp(config->seccion[p]->item, "comando"))
			{
				if (!config->seccion[p]->data)
				{
					if (avisa)
						Error("[%s:%i] Falta nombre de comando", config->archivo, config->linea, config->seccion[p]->data);
					errores++;
				}
			}
		}
		fclose(fp);
	}
	else
	{
		if (avisa)
			Error("[%s:%i] No se encuentra la función %s", config->archivo, config->linea, config->data);
		errores++;
	}
	return errores;
}
bCom *BuscaComModRemoto(char *remoto, Recurso *hmod)
{
	Recurso mod;
	char archivo[128], tmppath[PMAX];
	bCom *cm;
	if ((mod = CopiaDll(remoto, archivo, tmppath)))
	{
		irc_dlsym(mod, "comando", cm);
		*hmod = mod;
		return cm;
	}
	return NULL;
}
	
/*!
 * @desc: Setea en memoria una función a partir de su bloque funcion { }
 * @params: $config [in] Bloque función de configuración.
 	    $mod [in] Modulo al que pertenece esa función.
 	    $coms [in] Lista estática de comandos que soporta mod.
 * @cat: Modulos
 * @ver: ParseaConfiguracion TestComMod
 !*/
void SetComMod(Conf *config, Modulo *mod, bCom *coms)
{
	bCom *cm = NULL;
	char  *amod = NULL;
	Recurso hmod = NULL;
	if ((cm = BuscaComMod(coms, config->data)) || (cm = BuscaComModRemoto(config->data, &hmod)))
	{
		int p;
		Funcion *com;
		com = CreaCom(cm);
		com->hmod = hmod;
		for (p = 0; p < config->secciones; p++)
		{
			if (!strcmp(config->seccion[p]->item, "nivel"))
				com->nivel = BuscaOptNiv(config->seccion[p]->data);
			else if (!strcmp(config->seccion[p]->item, "comando"))
				ircstrdup(com->com, config->seccion[p]->data);
		}
		if (!BuscaFuncion(com->com, mod)) /* con uno basta */
			mod->comando[mod->comandos++] = com;
	}
}
void LiberaComs(Modulo *mod)
{
	int i;
	for (i = 0; i < mod->comandos; i++)
	{
		if (mod->comando[i]->hmod)
			irc_dlclose(mod->comando[i]->hmod);
		ircfree(mod->comando[i]->com);
		ircfree(mod->comando[i]);
	}
	mod->comandos = 0;
}
/*!
 * @desc: Lista todos los comandos y su descripción cargados en el módulo en cuyos cuales el usuario tenga suficiente nivel para ejecutarlos. Función para sistemas de AYUDA.
 * @params: $mod [in] Modulo al cual pertenecen las funciones que se pretenden listar.
 		  $cl [in] Cliente al que se desea hacer el listado.
 * @cat: Modulos
 * @ver: MuestraAyudaComando
 !*/
void ListaDescrips(Modulo *mod, Cliente *cl)
{
	int i;
	for (i = 0; i < mod->comandos; i++)
	{
		if (TieneNivel(cl, mod->comando[i]->com, mod, NULL))
			Responde(cl, mod->cl, "\00312%s\003 %s", mod->comando[i]->com, mod->comando[i]->descrip);
	}
}
u_int InsertaNivel(char *nombre, char *flags)
{
	Nivel *niv;
	if (nivs == 15)
		return 0;
	BMalloc(niv, Nivel);
	niv->nombre = strdup(nombre);
	if (flags)
		niv->flags = strdup(flags);
	niv->nivel = (1 << nivs);
	niveles[nivs++] = niv;
	return niv->nivel;
}
void DescargaNiveles()
{
	int i;
	Nivel *niv;
	for (i = 0; i < nivs; i++)
	{
		niv = niveles[i];
		ircfree(niv->nombre);
		ircfree(niv->flags);
		ircfree(niv);
	}
	nivs = 0;
}
Nivel *BuscaNivel(char *nombre)
{
	int i;
	for (i = 0; i < nivs; i++)
	{
		if (!strcasecmp(niveles[i]->nombre, nombre))
			return niveles[i];
	}
	return NULL;
}
Alias *CreaAlias(char *formato, char *sintaxis, Modulo *mod)
{
	Alias *alias;
	char *c;
	int i;
	BMalloc(alias, Alias);
	for (i = 0, alias->formato[i++] = c = strdup(formato); c = strchr(c, ' '); alias->formato[i++] = c)
		*c++ = '\0';
	alias->formato[i] = NULL;
	alias->crc = Crc32(alias->formato[0], strlen(alias->formato[0]));
	for (i = 0, alias->sintaxis[i++] = c = strdup(sintaxis); c = strchr(c, ' '); alias->sintaxis[i++] = c)
		*c++ = '\0';
	alias->sintaxis[i] = NULL;
	AddItem(alias, mod->aliases);
	return alias;
}
void DescargaAliases(Modulo *mod)
{
	Alias *aux, *tmp;
	for (aux = mod->aliases; aux; aux = tmp)
	{
		tmp = aux->sig;
		ircfree(aux->formato[0]);
		ircfree(aux->sintaxis[0]);
		ircfree(aux);
	}
	mod->aliases = NULL;
}
Alias *BuscaAlias(char **param, int params, Modulo *mod)
{
	int i;
	u_long crc;
	Alias *aux;
	if (!mod->aliases)
		return NULL;
	crc = Crc32(param[0], strlen(param[0]));
	for (aux = mod->aliases; aux; aux = aux->sig)
	{
		if (aux->crc == crc)
		{
			for (i = 1; i < params && aux->formato[i]; i++)
			{
				if (*aux->formato[i] != '%' && strcasecmp(param[i], aux->formato[i]))
					break;
			}
			if (i >= params || !aux->formato[i])
				return aux;
		}
	}
	return NULL;
}
