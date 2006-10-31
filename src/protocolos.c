/*
 * $Id: protocolos.c,v 1.8 2006-10-31 23:49:11 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

u_long UMODE_REGNICK = 0L;
u_long CHMODE_RGSTR = 0L;
u_long UMODE_HIDE = 0L;

void LiberaMemoriaProtocolo(Protocolo *prot)
{
	protocolo->descarga();
	irc_dlclose(prot->hprot);
	Free(prot->archivo);
	Free(prot->tmparchivo);
	Free(prot);
}
void DescargaProtocolo()
{
	if (protocolo) 
	{ 
		LiberaMemoriaProtocolo(protocolo); 
		protocolo = NULL;
	}
}
int CargaProtocolo(Conf *config)
{
	Recurso prot;
	char archivo[MAX_FNAME], tmppath[PMAX];
	int (*mod_carga)(), (*mod_descarga)();
	void (*ini)();
	SOCKFUNC(*parsea);
	ProtInfo *inf;
	if ((prot = CopiaDll(config->data, archivo, tmppath)))
	{
		irc_dlsym(prot, "Prot_Carga", mod_carga);
		if (!mod_carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Carga)", archivo);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Descarga", mod_descarga);
		if (!mod_descarga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Descarga)", archivo);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Info", inf);
		if (!inf || !inf->nombre)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Info)", archivo);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Inicia", ini);
		if (!ini)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Inicia)", archivo);
			irc_dlclose(prot);
			return 1;
		}
		irc_dlsym(prot, "Prot_Parsea", parsea);
		if (!parsea)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Parsea)", archivo);
			irc_dlclose(prot);
			return 1;
		}
		/* una vez está todo correcto, liberamos el anterior si hubiera uno */
		if (protocolo)
			LiberaMemoriaProtocolo(protocolo);
		protocolo = BMalloc(Protocolo);
		protocolo->archivo = strdup(config->data);
		protocolo->tmparchivo = strdup(tmppath);
		protocolo->hprot = prot;
		protocolo->carga = mod_carga;
		protocolo->descarga = mod_descarga;
		protocolo->info = inf;
		//protocolo->sincroniza = sincro;
		protocolo->inicia = ini;
		protocolo->parsea = parsea;
		//protocolo->especiales = comandos_especiales;z
		if (mod_carga(config))
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Prot_Carga())", archivo);
			DescargaExtensiones(protocolo);
			LiberaMemoriaProtocolo(protocolo);
			return 1;
		}
		CargaExtensiones(protocolo);
	}
	else
		return 2;
	return 0;
}
int DescargaExtension(Extension *ext, Protocolo *mod)
{
	if (ext->descarga)
		ext->descarga(ext, mod);
	BorraItem(ext, mod->extensiones);
	Free(ext->archivo);
	Free(ext->tmparchivo);
	irc_dlclose(ext->hmod);
	Free(ext);
	return 0;
}
Extension *CreaExtension(Conf *config, Protocolo *mod)
{
	Recurso hmod;
	char archivo[MAX_FNAME], tmppath[PMAX];
	int (*mod_carga)(Extension *, Protocolo *), (*mod_descarga)(Extension *, Protocolo *);
	ModInfo *inf;
	if ((hmod = CopiaDll(config->data, archivo, tmppath)))
	{
		Extension *ext;
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
		ext = BMalloc(Extension);
		ext->archivo = strdup(config->data);
		ext->tmparchivo = strdup(tmppath);
		ext->hmod = hmod;
		ext->carga = mod_carga;
		ext->descarga = mod_descarga;
		ext->info = inf;
		ext->config = config;
		AddItem(ext, mod->extensiones);
		return ext;
	}
	return NULL;
}
int DescargaExtensiones(Protocolo *mod)
{
	Extension *ext, *sig;
	for (ext = mod->extensiones; ext; ext = sig)
	{
		sig = ext->sig;
		DescargaExtension(ext, mod);
	}
	return 0;
}
int CargaExtensiones(Protocolo *mod)
{
	Extension *ext, *sig;
	for (ext = mod->extensiones; ext; ext = sig)
	{
		sig = ext->sig;
		if (ext->carga && ext->carga(ext, mod))
			DescargaExtension(ext, mod);
	}
	return 0;
}
/*ExtFunc *BuscaExtFunc(Extension *ext, Modulo *mod)
{
	ExtFunc *extp;
	for (extp = ext->extfunc; extp; extp = extp->sig)
	{
		if (extp->mod == mod)
			return extp;
	}
	return NULL;
}			
void InsertaExtFuncion(Extension *ext, Modulo *mod, int pos, Mod_Func func)
{
	ExtFunc *extp;
	if (!(extp = BuscaExtFunc(ext, mod)))
	{
		extp = BMalloc(ExtFunc);
		extp->mod = mod;
		AddItem(extp, ext->extfunc);
	}
	extp->funcs[pos] = func;
}
int BorraExtFuncion(Extension *ext, Modulo *mod, int pos)
{
	ExtFunc *extp;
	if ((extp = BuscaExtFunc(ext, mod)))
	{
		extp->funcs[pos] = NULL;
		return 1;
	}
	return 0;
}
*/
void InsertaSenyalExt(int senyal, Ext_Func func, Extension *ext)
{
	SenyalExt *sign, *aux;
	for (aux = ext->senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
			return;
	}
	sign = BMalloc(SenyalExt);
	sign->senyal = senyal;
	sign->func = func;
	if (!ext->senyals[senyal])
		ext->senyals[senyal] = sign;
	else
	{
		for (aux = ext->senyals[senyal]; aux; aux = aux->sig)
		{
			if (!aux->sig)
			{
				aux->sig = sign;
				break;
			}
		}
	}
}
int BorraSenyalExt(int senyal, Ext_Func func, Extension *ext)
{
	SenyalExt *aux, *prev = NULL;
	for (aux = ext->senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				ext->senyals[senyal] = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void LlamaSenyalExt(Modulo *mod, int senyal, Cliente *cl, char *parv[], int parc, char *param[], int params)
{
	Extension *ext;
	SenyalExt *aux;
	for (ext = protocolo->extensiones; ext; ext = ext->sig)
	{
		for (aux = ext->senyals[senyal]; aux; aux = aux->sig)
		{
			if (aux->func)
				aux->func(mod, cl, parv, parc, param, params);
		}
	}
}
mTab *InsertaModoProtocolo(char flag, u_long *modo, mTab *dest)
{
	int i;
	for (i = 0; i < MAXMOD && dest[i].flag && dest[i].mode; i++);
	if (i == MAXMOD)
		return NULL;
	*modo = (1 << i);
	dest[i].mode = *modo;
	dest[i].flag = flag;
	return &dest[i];
}
mTab *BuscaModoProtocolo(u_long modo, mTab *orig)
{
	int i;
	for (i = 0; i < MAXMOD && orig[i].flag && orig[i].mode; i++)
	{
		if (orig[i].mode == modo)
			return &orig[i];
	}
	return NULL;
}
