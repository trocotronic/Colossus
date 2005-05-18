/*
 * $Id: modulos.c,v 1.8 2005-05-18 18:51:04 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
const char *our_dlerror(void);
#endif
int id = 1;

Modulo *modulos = NULL;
Modulo *crea_modulo(char *archivo)
{
	char tmppath[128];
#ifdef _WIN32
	HMODULE modulo;
#else
	void *modulo;
#endif
	int (*mod_carga)(Modulo *), (*mod_descarga)(Modulo *);
	ModInfo *inf;
	char *amod;
	Modulo *ex;
	amod = strrchr(archivo, '/');
	if (!amod)
	{
		fecho(FADV, "Ha sido imposible cargar %s (falla ruta)", archivo);
		return NULL;
	}
	amod++;
	sprintf_irc(tmppath, "./tmp/%s", amod);
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
		fecho(FADV, "Ha sido imposible cargar %s (no puede copiar)", archivo);
		return NULL;
	}
	if ((modulo = irc_dlopen(tmppath, RTLD_NOW)))
	{
		Modulo *mod;
		irc_dlsym(modulo, "carga", mod_carga);
		if (!mod_carga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (carga)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		irc_dlsym(modulo, "descarga", mod_descarga);
		if (!mod_descarga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (descarga)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		irc_dlsym(modulo, "info", inf);
		if (!inf || !inf->nombre)
		{
			fecho(FADV, "Ha sido imposible cargar %s (info)", amod);
			irc_dlclose(modulo);
			return NULL;
		}
		da_Malloc(mod, Modulo);
		mod->archivo = strdup(archivo);
		mod->tmparchivo = strdup(tmppath);
		mod->hmod = modulo;
		mod->id = id;
		mod->carga = mod_carga;
		mod->descarga = mod_descarga;
		mod->cargado = 1; /* está en conf */
		mod->info = inf;
		id <<= 1;
		AddItem(mod, modulos);
		return mod;
	}
	else
		fecho(FADV, "Ha sido imposible cargar %s (dlopen): %s", amod, irc_dlerror());
	return NULL;
}
void descarga_modulo(Modulo *ex)
{
	killbot(ex->nick, "Refrescando");
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
void carga_modulos()
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
				descarga_modulo(ex);
				fecho(FADV, "No se ha podido cargar el modulo %s", ex->archivo);
				continue;
			}
			ex->cargado = 2;
		}
	}
}
void descarga_modulos()
{
	Modulo *ex, *sig;
	for (ex = modulos; ex; ex = sig)
	{
		sig = ex->sig;
		if (ex->cargado)
			descarga_modulo(ex);
	}
	id = 1;
}
Modulo *busca_modulo(char *nick, Modulo *donde)
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
const char *our_dlerror(void)
{
	static char errbuf[513];
	DWORD err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, errbuf, 512, NULL);
	return errbuf;
}
#endif
Mod_Func busca_funcion(Modulo *mod, char *nombre, int *nivel)
{
	int i;
	for (i = 0; i < mod->comandos; i++)
	{
		if (!strcasecmp(mod->comando[i]->com, nombre))
		{
			if (nivel)
				*nivel = mod->comando[i]->nivel;
			return mod->comando[i]->func;
		}
	}
	if (nivel)
		*nivel = 0;
	return NULL;
}
