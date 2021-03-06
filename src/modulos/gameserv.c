/*
 * $Id: gameserv.c,v 1.10 2007/08/26 11:36:13 Trocotronic Exp $
 */

#include <time.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/gameserv.h"
#ifdef BIDLE
#include "modulos/gameserv/bidle.h"
#endif
#ifdef KYRHOS
#include "modulos/gameserv/kyrhos.h"
#endif

GameServ *gameserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, gameserv->hmod, NULL)

static bCom gameserv_coms[] = {
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void GSSet(Conf *, Modulo *);
int GSTest(Conf *, int *);
int GSSigEOS();
int GSSigSQL();
int GSSigPMsg(Cliente *, Cliente *, char *, int);
int GSSigSockClose();

ModInfo MOD_INFO(LogServ) = {
	"GameServ" ,
	1.0 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};
int MOD_CARGA(GameServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(GameServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(GameServ).nombre))
			{
				if (!GSTest(modulo.seccion[0], &errores))
					GSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(GameServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(GameServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		GSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(GameServ)()
{
	BotUnset(gameserv);
#ifdef KYRHOS
	if (kyrhos)
		KyrhosDescarga();
#endif
#ifdef BIDLE
	if (bidle)
		BidleDescarga();
#endif
	return 0;
}
int GSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, gameserv_coms, 1);
	*errores += error_parcial;
	return error_parcial;
}
void GSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!gameserv)
		gameserv = BMalloc(GameServ);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, gameserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, gameserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
#ifdef KYRHOS
			else if (!strcmp(config->seccion[i]->item, "Kyrhos"))
				KyrhosParseaConf(config->seccion[i]);
#endif
#ifdef BIDLE
			else if (!strcasecmp(config->seccion[i]->item, "Bidle"))
				BidleParseaConf(config->seccion[i]);
#endif
		}
	}
	else
		ProcesaComsMod(NULL, mod, gameserv_coms);
	BotSet(gameserv);
}
