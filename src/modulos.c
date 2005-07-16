/*
 * $Id: modulos.c,v 1.11 2005-07-16 15:25:29 Trocotronic Exp $ 
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

Modulo *modulos = NULL;
Modulo *CreaModulo(char *archivo)
{
	char tmppath[128];
#ifdef _WIN32
	HMODULE modulo;
	char tmppdb[128], pdb[128];
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
Mod_Func BuscaFuncion(Modulo *mod, char *nombre, int *nivel)
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
