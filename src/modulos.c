#include "struct.h"
#include "ircd.h"
#include "comandos.h"
#include "modulos.h"
#ifndef _WIN32
#include <dlfcn.h>
#else
const char *our_dlerror(void);
#endif

Modulo *modulos = NULL;
void response(Cliente *cl, char *nick, char *formato, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	if (EsBot(cl))
		return;
	va_start(vl, formato);
	vsprintf_irc(buf, formato, vl);
	va_end(vl);
	sockwrite(SockActual, OPT_CRLF, ":%s %s %s :%s", nick, conf_set->opts & RESP_PRIVMSG ? TOK_PRIVATE : TOK_NOTICE, cl->nombre, buf);
}
int crea_modulo(char *archivo)
{
	char tmppath[128];
	static id = 0;
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
		return 1;
	}
	amod++;
	sprintf_irc(tmppath, "./tmp/%s", amod);
	for (ex = modulos; ex; ex = ex->sig)
	{
		if (!strcasecmp(ex->archivo, archivo))
		{
			ex->cargado = 1; /* está en conf */
			return 0; /* no creamos lo que está creado */
		}
	}
	if (!copyfile(archivo, tmppath))
		return 1;
	if ((modulo = irc_dlopen(tmppath, RTLD_NOW)))
	{
		Modulo *mod;
		irc_dlsym(modulo, "carga", mod_carga);
		if (!mod_carga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (carga)", amod);
			irc_dlclose(modulo);
			return 1;
		}
		irc_dlsym(modulo, "descarga", mod_descarga);
		if (!mod_carga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (descarga)", amod);
			irc_dlclose(modulo);
			return 1;
		}
		irc_dlsym(modulo, "info", inf);
		if (!inf || !inf->nombre)
		{
			fecho(FADV, "Ha sido imposible cargar %s (info)", amod);
			irc_dlclose(modulo);
			return 1;
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
		mod->sig = modulos;
		modulos = mod;
		id *= 2;
	}
	else
	{
		fecho(FADV, "Ha sido imposible cargar %s (dlopen): %s", amod, irc_dlerror());
		return 2;
	}
	return 0;
}
void descarga_modulo(Modulo *ex)
{
	killbot(ex->nick, "Refrescando");
	if (ex->cargado == 2) /* está cargado */
	{
		if (ex->descarga)
			(*ex->descarga)(ex);
	}
	ex->cargado = 0;
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
	Modulo *ex;
	for (ex = modulos; ex; ex = ex->sig)
	{
		if (ex->cargado)
			descarga_modulo(ex);
	}
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
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
		0, errbuf, 512, NULL);
	return errbuf;
}
#endif
