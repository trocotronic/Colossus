/*
 * $Id: helpserv.c,v 1.6 2007-03-19 19:16:36 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/helpserv.h"
#include "modulos/chanserv.h"
#include "httpd.h"

HelpServ *helpserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, helpserv->hmod, NULL)
HDir *hdtest = NULL;
Canal *helpchan = NULL;

BOTFUNC(HSTeste);
BOTFUNCHELP(HSHTeste);

static bCom helpserv_coms[] = {
	{ "test" , HSTeste , N0 , "Obtiene +v habiendo aprobado el test" , HSHTeste } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void HSSet(Conf *, Modulo *);
int HSTest(Conf *, int *);
HDIRFUNC(LeeHDir);
int HSCmdJoin(Cliente *, Canal *);
int HSSigCDestroy(Canal *);

ModInfo MOD_INFO(HelpServ) = {
	"HelpServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(HelpServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(HelpServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(HelpServ).nombre))
			{
				if (!HSTest(modulo.seccion[0], &errores))
					HSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(HelpServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(HelpServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		HSSet(NULL, mod);
	if (conf_httpd)
	{
		InsertaSenyal(SIGN_JOIN, HSCmdJoin);
		InsertaSenyal(SIGN_CDESTROY, HSSigCDestroy);
		if (!(hdtest = CreaHDir("C:/Colossus", "/html", LeeHDir)))
			errores++;
	}
	helpchan = BuscaCanal(helpserv->canal);
	return errores;
}
int MOD_DESCARGA(HelpServ)()
{
	if (conf_httpd)
	{
		BorraSenyal(SIGN_JOIN, HSCmdJoin);
		BorraSenyal(SIGN_CDESTROY, HSSigCDestroy);
		BorraHDir(hdtest);
	}
	BotUnset(helpserv);
	return 0;
}
int HSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, helpserv_coms, 1);
	if (!(eval = BuscaEntrada(config, "canal_soporte")))
	{
		Error("[%s:%s] No se encuentra la directriz canal_soporte.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (BadPtr(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz canal_soporte esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void HSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!helpserv)
		helpserv = BMalloc(HelpServ);
	ircfree(helpserv->canal);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, helpserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, helpserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
			else if (!strcmp(config->seccion[i]->item, "canal_soporte"))
				ircstrdup(helpserv->canal, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "url_test"))
				ircstrdup(helpserv->url_test, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "castigo"))
			{
				if (config->seccion[i]->secciones > 0)
				{
					helpserv->castigo = BMalloc(struct _castigo);
					helpserv->castigo->bantype = 3;
				}
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "bantype"))
						helpserv->castigo->bantype = atoi(config->seccion[i]->seccion[p]->data);
					else if (!strcmp(config->seccion[i]->seccion[p]->item, "minutos"))
						helpserv->castigo->minutos = atoi(config->seccion[i]->seccion[p]->data);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, helpserv_coms);
	BotSet(helpserv);
}
int HSQuitaBan(char *b)
{
	if (!b)
		return 1;
	if (!helpchan)
		helpchan = BuscaCanal(helpserv->canal);
	ProtFunc(P_MODO_CANAL)(CLI(helpserv), helpchan, b);
	Free(b);
	return 0;
}
HDIRFUNC(LeeHDir)
{
	if (!helpchan)
		helpchan = BuscaCanal(helpserv->canal);
	if (helpchan && hh->param_get)
	{
		char *user = NULL, *tok = NULL, *c, *d, a;
		Cliente *al;
		u_long crc32;
		strlcpy(tokbuf, hh->param_get, sizeof(tokbuf));
		for (c = strtok(tokbuf, "&"); c; c = strtok(NULL, "&"))
		{
			if (!(d = strchr(c, '=')))
			{
				*errmsg = "Faltan parámetros";
				return 400;
			}
			if (*c == 'k' || *c == 'v' || *c == 'u')
			{
				a = *c;
				user = d+1;
			}
			else if (*c == 'c')
				tok = d+1;
		}
		if ((a == 'v' || a == 'k') && (!tok || !user))
		{
			*errmsg = "Faltan parámetros";
			return 400;
		}
		if (!(al = BuscaCliente(user)))
		{
			*errmsg = "Este usuario no existe";
			return 400;
		}
		if (!EsLink(helpchan->miembro, al))
		{
			*errmsg = "Este usuario no está en el canal";
			return 400;
		}
		if (tok)
		{
			sscanf(tok, "%X", &crc32);
			ircsprintf(buf, "%s?%s?%s", al->nombre, al->ip, "aquellos ojos verdes");
			if (crc32 != Crc32(buf, strlen(buf)))
			{
				*errmsg = "Acción denegada";
				return 400;
			}
		}
		switch(a)
		{
			case 'v':
				ProtFunc(P_MODO_CANAL)(CLI(helpserv), helpchan, "+v %s", user);
				break;
			case 'k':
			{
				if (helpserv->castigo)
				{
					char tmp[128];
					ircsprintf(tmp, "+b %s", TipoMascara(al->mask, 9));
					ProtFunc(P_MODO_CANAL)(CLI(helpserv), helpchan, tmp);
					if (helpserv->castigo->minutos)
					{
						ProtFunc(P_KICK)(al, CLI(helpserv), helpchan, "No has superado el test (ban temporal %i minutos)", helpserv->castigo->minutos);
						tmp[0] = '-';
						IniciaCrono(1, helpserv->castigo->minutos*60, HSQuitaBan, strdup(tmp));
					}
					else
						ProtFunc(P_KICK)(al, CLI(helpserv), helpchan, "No has superado el test (ban permanente)", helpserv->castigo->minutos);
				}
				break;
			}
		}
		return 200;
	}
	else if (!helpchan)
	{
		*errmsg = "Servicio no disponible";
		return 400;
	}
	return 200;
}
int HSCmdJoin(Cliente *cl, Canal *cn)
{
	if ((helpchan && helpchan == cn) || (!strcmp(cn->nombre, helpserv->canal)))
	{
		char *c_c, *n_c;
		if (!helpchan)
			helpchan = BuscaCanal(helpserv->canal);
		c_c = SQLEscapa(strtolower(cn->nombre));
		n_c = SQLEscapa(strtolower(cl->nombre));
		if (!SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, c_c, n_c) &&
			!SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s' AND LOWER(founder)='%s'", PREFIJO, CS_SQL, c_c, n_c))
		{
			Responde(cl, CLI(helpserv), "Para poder hablar en este canal deberás hacer un test y responderlo correctamente.");
			Responde(cl, CLI(helpserv), "Si lo superas tendrás +v en este canal automáticamente.");
			ircsprintf(buf, ":%i", conf_httpd->puerto);
			ircsprintf(tokbuf, "%s?%s?%s", cl->nombre, cl->ip, "aquellos ojos verdes");
			Responde(cl, CLI(helpserv), "La dirección es \00312http://%s%s/%s?u=%s&c=%X", conf_httpd->url, conf_httpd->puerto != 80 ? buf : "", helpserv->url_test, cl->nombre, Crc32(tokbuf, strlen(tokbuf)));
		}
		Free(c_c);
		Free(n_c);
	}
	return 0;
}
int HSSigCDestroy(Canal *cn)
{
	if (helpchan && helpchan == cn)
		helpchan = NULL;
	return 0;
}
BOTFUNC(HSTeste)
{
	u_long crc32;
	if (params < 2)
	{
		Responde(cl, CLI(helpserv), CS_ERR_PARA, fc->com, "clave");
		return 1;
	}
	sscanf(param[1], "%X", &crc32);
	ircsprintf(buf, "%s!%s", cl->nombre, "aquellos ojos verdes");
	if (crc32 != Crc32(buf, strlen(buf)))
	{
		Responde(cl, CLI(helpserv), CS_ERR_EMPT, "Clave incorrecta");
		return 1;
	}
	if (!helpchan)
		helpchan = BuscaCanal(helpserv->canal);
	Responde(cl, CLI(helpserv), "Clave correcta");
	ProtFunc(P_MODO_CANAL)(CLI(helpserv), helpchan, "+v %s", cl->nombre);
	return 0;
}
BOTFUNCHELP(HSHTeste)
{
	Responde(cl, CLI(helpserv), "Otorga +v si se ha aprobado el test en algún momento.");
	Responde(cl, CLI(helpserv), "Una vez se ha aprobado el test, no es necesario hacerlo cada vez que se entre al canal, puesto que se considera persona con conocimientos suficientes.");
	Responde(cl, CLI(helpserv), "Si has aprobado el test alguna vez se te habrá dado una clave para obtener +v y saltarte este paso.");
	Responde(cl, CLI(helpserv), " ");
	Responde(cl, CLI(helpserv), "Sintaxis: \00312TEST clave");
	return 0;
}
