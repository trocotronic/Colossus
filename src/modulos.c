/*
 * $Id: modulos.c,v 1.14 2005-12-04 14:09:22 Trocotronic Exp $ 
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
Modulo *CreaModulo(char *archivo)
{
	char *amod, tmppath[128];
#ifdef _WIN32
	HMODULE modulo;
	char tmppdb[128], pdb[128];
#else
	void *modulo;
#endif
	int (*mod_carga)(Modulo *), (*mod_descarga)(Modulo *);
	ModInfo *inf;
	Modulo *ex;
	amod = strrchr(archivo, '/');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta)", archivo);
		return NULL;
	}
	amod++;
	ircsprintf(tmppath, "./tmp/%s", amod);
#ifdef _WIN32
	strcpy(pdb, archivo);
	amod = strrchr(pdb, '.');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla pdb)", archivo);
		return NULL;
	}
	strcpy(amod, ".pdb");
	amod = strrchr(pdb, '/');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta pdb)", pdb);
		return NULL;
	}
	amod++;
	ircsprintf(tmppdb, "./tmp/%s", amod);
#endif
	for (ex = modulos; ex; ex = ex->sig)
	{
		if (!strcasecmp(ex->archivo, archivo))
		{
			ex->cargado = 1; /* está en conf */
			return ex; /* no creamos lo que está creado */
		}
	}
	if (!copyfile(archivo, tmppath))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar)", archivo);
		return NULL;
	}
#ifdef _WIN32
	if (!copyfile(pdb, tmppdb))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar pdb)", pdb);
		return NULL;
	}
#endif
	if ((modulo = irc_dlopen(tmppath, RTLD_LAZY)))
	{
		Modulo *mod;
		irc_dlsym(modulo, "Mod_Carga", mod_carga);
		if (!mod_carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Carga)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		irc_dlsym(modulo, "Mod_Descarga", mod_descarga);
		if (!mod_descarga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Descarga)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		irc_dlsym(modulo, "Mod_Info", inf);
		if (!inf || !inf->nombre)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Mod_Info)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		BMalloc(mod, Modulo);
		mod->archivo = strdup(archivo);
		mod->tmparchivo = strdup(tmppath);
		mod->hmod = modulo;
		mod->id = id;
		mod->carga = mod_carga;
		mod->descarga = mod_descarga;
		mod->cargado = 1; /* está en conf */
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
	else
		Alerta(FADV, "Ha sido imposible cargar %s (dlopen): %s", amod, irc_dlerror());
	return NULL;
}
void DescargaModulo(Modulo *ex)
{
	DesconectaBot(ex->nick, "Refrescando");
	if (ex->cargado == 2) /* está cargado */
	{
		if (ex->descarga)
			(*ex->descarga)(ex);
		ircfree(ex->archivo);
		ircfree(ex->tmparchivo);
		BorraItem(ex, modulos);
		ex->cargado = 0;
		ex->comandos = 0; /* hay que vaciarlo! */
		irc_dlclose(ex->hmod);
		Free(ex);
	}
}
void CargaModulos()
{
	Modulo *ex;
	for (ex = modulos; ex; ex = ex->sig)
	{
		if (ex->cargado) /* está en conf */
		{
			int val;
			val = (*ex->carga)(ex);
			if (val)
			{
				DescargaModulo(ex);
				Alerta(FADV, "No se ha podido cargar el modulo %s", ex->archivo);
				continue;
			}
			ex->cargado = 2;
		}
	}
}
void DescargaModulos()
{
	Modulo *ex, *sig;
	for (ex = modulos; ex; ex = sig)
	{
		sig = ex->sig;
		if (ex->cargado)
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
		if (ex->cargado && !strcasecmp(nick, ex->nick))
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
bCom *CargadoComMod(char *com, Modulo *mod)
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
 * @desc: Busca si una función está cargada mediante su comando y su nivel.
 * @params: $mod [in] Módulo donde buscar.
 	    $nombre [in] Nombre del comando.
 	    $nivel [out] Contendrá el nivel de la función necesario para ejecutarse. Si es 0, la función no existe.
 * @ret: Devuelve la dirección de la función que ejecutará ese comando. NULL si no existe.
 * @cat: Modulos
 * @ver: CargadoComMod ParseaConfiguracion BuscaComMod TieneNivel
 !*/
Mod_Func BuscaFuncion(Modulo *mod, char *nombre, u_int *nivel)
{
	bCom *bcom;
	if ((bcom = CargadoComMod(nombre, mod)))
	{
		if (nivel)
			*nivel = bcom->nivel;
		return bcom->func;
	}
	if (nivel)
		*nivel = 0;
	return NULL;
}
/*!
 * @desc: Busca el nombre de una función dentro de una lista estática de comandos.
 * @params: $coms [in] Lista de comandos.
 	    $func [in] Nombre de la función.
 * @cat: Modulos
 * @ret: Devuelve el comando asociado a esa función. NULL si no existe en la lista.
 * @ver: BuscaFuncion ParseaConfiguracion TieneNivel CargadoComMod
 !*/
bCom *BuscaComMod(bCom *coms, char *func)
{
	bCom *cm;
	cm = &coms[0];
	while (cm->com != 0x0)
	{
		if (!strcasecmp(cm->com, func))
			return cm;
		cm++;
	}
	return NULL;
}
/*!
 * @desc: Comprueba si un cliente tiene nivel para ejecutar una función.
 * @params: $cl [in] Cliente.
 	    $func [in] Nombre del comando que ha usado el cliente.
 	    $mod [in] Módulo que ha llamado el cliente.
 	    $ex [out] Vale 1 si esa función existe. 0, si no existe.
 * @cat: Modulos
 * @ver: BuscaComMod ParseaConfiguracion CargadoComMod BuscaFuncion
 * @ret: Devuelve la dirección de la función que se ejecutará. NULL si no existe o si no tiene nivel.
 !*/
Mod_Func TieneNivel(Cliente *cl, char *func, Modulo *mod, char *ex)
{
	Mod_Func modfunc;
	u_int niv;
	if (ex)
		*ex = 0;
	if ((modfunc = BuscaFuncion(mod, func, &niv)))
	{
		if (ex)
			*ex = 1;
		if (cl->nivel >= niv)
			return modfunc;
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
/*!
 * @desc: Parsea un bloque de configuración correspondiente a funciones cargadas del módulo.
 * @params: $config [in] Rama o bloque de funciones del archivo de configuración. Llamado por ParseaConfiguracion.
 	    $mod [in] Recurso del módulo a cargar estos comandos.
 	    $coms [in] Lista completa de los comandos soportados por el módulo.
 * @cat: Modulos
 * @ver: ParseaConfiguracion TieneNivel CargadoComMod BuscaFuncion BuscaComMod
 !*/
 
int ProcesaComsMod(Conf *config, Modulo *mod, bCom *coms)
{
	int p, errores = 0;
	bCom *cm = NULL;
	for (p = 0; p < config->secciones; p++)
	{
		if ((cm = BuscaComMod(coms, config->seccion[p]->item)))
		{
			if (!CargadoComMod(cm->com, mod))
				mod->comando[mod->comandos++] = cm;
		}
		else
		{
			Error("[%s:%i] No se ha encontrado la funcion %s", config->archivo, config->seccion[p]->linea, config->seccion[p]->item);
			errores++;
		}
	}
	return errores;
}
bCom *CreaCom(bCom *padre)
{
	bCom *cm = NULL;
	BMalloc(cm, bCom);
	if (padre)
	{
		cm->func = padre->func;
		cm->com = strdup(padre->com);
		cm->nivel = padre->nivel;
	}
	return cm;
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
	bCom *cm = NULL;
	if (!config->data)
	{
		if (avisa)
			Error("[%s:%i] Falta nombre de función", config->archivo, config->linea);
		return 1;
	}
	if ((cm = BuscaComMod(coms, config->data)))
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
	}
	else
	{
		if (avisa)
			Error("[%s:%i] No se encuentra la función %s", config->archivo, config->linea, config->data);
		errores++;
	}
	return errores;
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
	if ((cm = BuscaComMod(coms, config->data)))
	{
		int p;
		bCom *com;
		com = CreaCom(cm);
		for (p = 0; p < config->secciones; p++)
		{
			if (!strcmp(config->seccion[p]->item, "nivel"))
				com->nivel = BuscaOptNiv(config->seccion[p]->data);
			else if (!strcmp(config->seccion[p]->item, "comando"))
				ircstrdup(com->com, config->seccion[p]->data);
		}
		if (!CargadoComMod(com->com, mod)) /* con uno basta */
			mod->comando[mod->comandos++] = com;
	}
}
void LiberaComs(Modulo *mod)
{
	int i;
	for (i = 0; i < mod->comandos; i++)
	{
		ircfree(mod->comando[i]->com);
		ircfree(mod->comando[i]);
	}
	mod->comandos = 0;
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
