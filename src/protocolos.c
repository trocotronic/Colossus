/*
 * $Id: protocolos.c,v 1.3 2005-07-16 15:25:29 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

void LiberaMemoriaProtocolo(Protocolo *prot)
{
	protocolo->descarga();
	irc_dlclose(prot->hprot);
	Free(prot->archivo);
	Free(prot->tmparchivo);
	Free(prot);
}
int CargaProtocolo(Conf *config)
{
	char tmppath[128];
#ifdef _WIN32
	HMODULE prot;
	char tmppdb[128], pdb[128];
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
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta)", config->data);
		return 1;
	}
	amod++;
	ircsprintf(tmppath, "./tmp/%s", amod);
#ifdef _WIN32
	strcpy(pdb, config->data);
	amod = strrchr(pdb, '.');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla pdb)", config->data);
		return 1;
	}
	strcpy(amod, ".pdb");
	amod = strrchr(pdb, '/');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta pdb)", pdb);
		return 1;
	}
	amod++;
	ircsprintf(tmppdb, "./tmp/%s", amod);
#endif
	if (!copyfile(config->data, tmppath))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar)", config->data);
		return 1;
	}
#ifdef _WIN32
	if (!copyfile(pdb, tmppdb))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar pdb)", pdb);
		return 1;
	}
#endif
	if ((prot = irc_dlopen(tmppath, RTLD_LAZY)))
	{
		irc_dlsym(prot, "Prot_Carga", mod_carga);
		if (!mod_carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Carga)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Descarga", mod_descarga);
		if (!mod_descarga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Descarga)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Info", inf);
		if (!inf || !inf->nombre)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Info)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Inicia", ini);
		if (!ini)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Inicia)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Parsea", parsea);
		if (!parsea)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Parsea)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Comandos", comandos_especiales);
		if (!comandos_especiales)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Comandos)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Umodos", um);
		if (!um)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Umodos)", amod);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Cmodos", cm);
		if (!cm)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Cmodos)", amod);
			irc_dlclose(prot);
			return 1;
		}
		/* una vez está todo correcto, liberamos el anterior si hubiera uno */
		if (protocolo)
			LiberaMemoriaProtocolo(protocolo);
		BMalloc(protocolo, Protocolo);
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
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Carga())", amod);
			LiberaMemoriaProtocolo(protocolo);
			return 1;
		}
	}
	else
	{
		Alerta(FADV, "Ha sido imposible cargar %s (dlopen): %s", amod, irc_dlerror());
		return 2;
	}
	return 0;
}
