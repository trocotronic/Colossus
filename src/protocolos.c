/*
 * $Id: protocolos.c,v 1.1 2004-12-31 12:27:59 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

void libera_protocolo(Protocolo *prot)
{
	protocolo->descarga();
	irc_dlclose(prot->hprot);
	Free(prot->archivo);
	Free(prot->tmparchivo);
	Free(prot);
}
int carga_protocolo(Conf *config)
{
	char tmppath[128];
#ifdef _WIN32
	HMODULE prot;
#else
	void *prot;
#endif
	int (*mod_carga)(), (*mod_descarga)();
	void (*ini)();
	SOCKFUNC(*parsea);
	ProtInfo *inf;
	char *amod;
	Com *comandos_especiales;
	mTab *um, *cm;
	amod = strrchr(config->data, '/');
	if (!amod)
	{
		fecho(FADV, "Ha sido imposible cargar %s (falla ruta)", config->data);
		return 1;
	}
	amod++;
	sprintf_irc(tmppath, "./tmp/%s", amod);
	if (!copyfile(config->data, tmppath))
		return 1;
	if ((prot = irc_dlopen(tmppath, RTLD_NOW)))
	{
		irc_dlsym(prot, "carga", mod_carga);
		if (!mod_carga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (carga)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "descarga", mod_descarga);
		if (!mod_descarga)
		{
			fecho(FADV, "Ha sido imposible cargar %s (descarga)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "info", inf);
		if (!inf || !inf->nombre)
		{
			fecho(FADV, "Ha sido imposible cargar %s (info)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "inicia", ini);
		if (!ini)
		{
			fecho(FADV, "Ha sido imposible cargar %s (inicia)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "parsea", parsea);
		if (!parsea)
		{
			fecho(FADV, "Ha sido imposible cargar %s (parsea)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "comandos_especiales", comandos_especiales);
		if (!comandos_especiales)
		{
			fecho(FADV, "Ha sido imposible cargar %s (comandos_especiales)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "umodos", um);
		if (!um)
		{
			fecho(FADV, "Ha sido imposible cargar %s (umodos)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "cmodos", cm);
		if (!cm)
		{
			fecho(FADV, "Ha sido imposible cargar %s (cmodos)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "comandos_especiales", comandos_especiales);
		/* una vez está todo correcto, liberamos el anterior si hubiera uno */
		if (protocolo)
			libera_protocolo(protocolo);
		da_Malloc(protocolo, Protocolo);
		protocolo->archivo = strdup(config->data);
		protocolo->tmparchivo = strdup(tmppath);
		protocolo->hprot = prot;
		protocolo->carga = mod_carga;
		protocolo->descarga = mod_descarga;
		protocolo->info = inf;
		//protocolo->sincroniza = sincro;
		protocolo->inicia = ini;
		protocolo->parsea = parsea;
		protocolo->especiales = comandos_especiales;
		protocolo->umodos = um;
		protocolo->cmodos = cm;
		if (mod_carga(config))
		{
			fecho(FADV, "Ha sido imposible cargar %s (mod_carga())", amod);
			libera_protocolo(protocolo);
			return 1;
		}
	}
	else
	{
		fecho(FADV, "Ha sido imposible cargar %s (dlopen): %s", amod, irc_dlerror());
		return 2;
	}
	return 0;
}
