/*
 * $Id: gameserv.c,v 1.1 2006-11-01 11:38:26 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/gameserv.h"
#include "modulos/gameserv/kyrhos.h"

GameServ *gameserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, gameserv->hmod, NULL)

extern Kyrhos *kyrhos;


static bCom gameserv_coms[] = {
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void GSSet(Conf *, Modulo *);
int GSTest(Conf *, int *);
int GSSigEOS();
int GSSigSQL();
int GSSigPMsg(Cliente *, Cliente *, char *, int);
extern ProcesaKyrhos(Cliente *, char *);

ModInfo MOD_INFO(LogServ) = {
	"GameServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};
int MOD_CARGA(GameServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
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
	BorraSenyal(SIGN_EOS, GSSigEOS);
	BorraSenyal(SIGN_SQL, GSSigSQL);
	BorraSenyal(SIGN_PMSG, GSSigPMsg);
	BotUnset(gameserv);
	if (kyrhos)
	{
		ircfree(kyrhos->nick);
		DesconectaBot(kyrhos->cl, "La sombra de Kyrhos");
	}
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
			else if (!strcmp(config->seccion[i]->item, "Kyrhos"))
			{
				kyrhos = BMalloc(Kyrhos);
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "nick"))
						ircstrdup(kyrhos->nick, config->seccion[i]->seccion[p]->data);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, gameserv_coms);
	InsertaSenyal(SIGN_EOS, GSSigEOS);
	InsertaSenyal(SIGN_EOS, GSSigSQL);
	InsertaSenyal(SIGN_PMSG, GSSigPMsg);
	BotSet(gameserv);
}
int GSSigEOS()
{
	if (kyrhos)
		kyrhos->cl = CreaBot(kyrhos->nick ? kyrhos->nick : "Kyrhos", gameserv->hmod->ident, gameserv->hmod->host, "kdBq", "La sombra de Kyrhos");
	return 0;
}
int GSSigSQL()
{
	if (kyrhos && !SQLEsTabla(GS_KYRHOS))
	{
		if (SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"item varchar(255), "
			"pos int, "
			"KEY `item` (`item`) "
			");", PREFIJO, GS_KYRHOS))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, GS_KYRHOS);
	}
	SQLCargaTablas();
	return 0;
}
int GSSigPMsg(Cliente *cl, Cliente *bl, char *msg, int resp)
{
	if (bl == kyrhos->cl)
		ProcesaKyrhos(cl, msg);
	return 0;
}
