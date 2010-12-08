/*
 * $Id: nickserv.c,v 1.59 2008/01/21 19:46:45 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#include <netdb.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"

NickServ *nickserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, nickserv->hmod, NULL)

BOTFUNC(NSHelp);
BOTFUNC(NSRegister);
BOTFUNCHELP(NSHRegister);
BOTFUNC(NSIdentify);
BOTFUNCHELP(NSHIdentify);
BOTFUNC(NSOpts);
BOTFUNCHELP(NSHSet);
BOTFUNC(NSDrop);
BOTFUNCHELP(NSHDrop);
BOTFUNC(NSSendpass);
BOTFUNCHELP(NSHSendpass);
BOTFUNC(NSInfo);
BOTFUNCHELP(NSHInfo);
BOTFUNC(NSList);
BOTFUNCHELP(NSHList);
BOTFUNC(NSGhost);
BOTFUNCHELP(NSHGhost);
BOTFUNC(NSSuspender);
BOTFUNCHELP(NSHSuspender);
BOTFUNC(NSLiberar);
BOTFUNCHELP(NSHLiberar);
BOTFUNC(NSSwhois);
BOTFUNCHELP(NSHSwhois);
BOTFUNC(NSRename);
BOTFUNCHELP(NSHRename);
BOTFUNC(NSForbid);
BOTFUNCHELP(NSHForbid);
BOTFUNC(NSUnForbid);
BOTFUNCHELP(NSHUnForbid);
BOTFUNC(NSMarcas);
BOTFUNCHELP(NSHMarcas);
BOTFUNC(NSOptsNick);
BOTFUNCHELP(NSHOptsNick);

int NSSigSQL		();
int NSCmdPreNick 	(Cliente *, char *);
int NSCmdPostNick 	(Cliente *, int);
int NSCmdUmode		(Cliente *, char *);
int NSSigQuit 		(Cliente *, char *);
int NSSigIdOk 		(Cliente *);
int NSSigSockClose	();
int NSSigSynch		();
int NSKillea		(char *);

int NSDropanicks	(Proc *);

int NSBaja(char *, int);
int NickOpts(Cliente *, char *, char **, int, Funcion *);
DLLFUNC void NSCambiaInv(Cliente *);

void NSSet(Conf *, Modulo *);
int NSTest(Conf *, int *);
extern MODVAR mTab cFlags[];
mTab *umodreg = NULL;

typedef struct _killuser KillUser;
struct _killuser
{
	struct _killuser *sig;
	Cliente *cl;
	Timer *timer;
};
KillUser *killusers = NULL;

static bCom nickserv_coms[] = {
	{ "help" , NSHelp , N0 , "Muestra esta ayuda." , NULL } ,
	{ "register" , NSRegister , N0 , "Registra el nick que lleves puesto." , NSHRegister } ,
	{ "identify" , NSIdentify , N0 , "Te identifica como propietario del nick." , NSHIdentify } ,
	{ "set" , NSOpts , N1 , "Permite fijar distintas opciones para tu nick." , NSHSet } ,
	{ "drop" , NSDrop , N2 , "Desregistra un usuario." , NSHDrop } ,
	{ "sendpass" , NSSendpass , N2 , "Genera un password para el usuario y se lo envía a su correo." , NSHSendpass } ,
	{ "info" , NSInfo , N0 , "Muestra información de un nick." , NSHInfo } ,
	{ "list" , NSList, N0 , "Lista todos los nicks que coinciden con un patrón." , NSHList } ,
	{ "ghost" , NSGhost , N0 , "Desconecta a un usuario que utilice su nick." , NSHGhost } ,
	{ "suspender" , NSSuspender , N3 , "Suspende los privilegios de un nick." , NSHSuspender } ,
	{ "liberar" , NSLiberar , N3 , "Libera un nick de su suspenso." , NSHLiberar } ,
	{ "swhois" , NSSwhois , N2 , "Añade o borra un whois especial al nick especificado." , NSHSwhois } ,
	{ "rename" , NSRename , N3 , "Cambia el nick a un usuario." , NSHRename } ,
	{ "forbid" , NSForbid , N4 , "Prohibe un determinado nick." , NSHForbid } ,
	{ "unforbid" , NSUnForbid , N4 , "Permite un determinado nick prohibido." , NSHUnForbid } ,
	{ "marca" , NSMarcas , N3 , "Lista o inserta una entrada en el historial de un nick." , NSHMarcas } ,
	{ "setnick" , NSOptsNick , N3 , "Cambia las opciones de un nick." , NSHOptsNick } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};
ModInfo MOD_INFO(NickServ) = {
	"NickServ" ,
	1.0 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};

int MOD_CARGA(NickServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(NickServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(NickServ).nombre))
			{
				if (!NSTest(modulo.seccion[0], &errores))
					NSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el NSTest", mod->archivo, MOD_INFO(NickServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(NickServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		NSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(NickServ)()
{
	DetieneProceso(NSDropanicks);
	BorraSenyal(SIGN_PRE_NICK, NSCmdPreNick);
	BorraSenyal(SIGN_POST_NICK, NSCmdPostNick);
	BorraSenyal(SIGN_UMODE, NSCmdUmode);
	BorraSenyal(SIGN_SQL, NSSigSQL);
	BorraSenyal(SIGN_QUIT, NSSigQuit);
	BorraSenyal(NS_SIGN_IDOK, NSSigIdOk);
	BorraSenyal(SIGN_SOCKCLOSE, NSSigSockClose);
	BorraSenyal(SIGN_SYNCH, NSSigSynch);
	BotUnset(nickserv);
	return 0;
}
int NSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], nickserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void NSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!nickserv)
		nickserv = BMalloc(NickServ);
	nickserv->opts = NS_PROT_KILL;
	ircstrdup(nickserv->recovernick, "inv-%s-????");
	ircstrdup(nickserv->securepass, "******");
	nickserv->min_reg = 24;
	nickserv->autodrop = 15;
	nickserv->nicks = 3;
	nickserv->intentos = 3;
	nickserv->maxlist = 30;
	nickserv->forbmails = 0;
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "sid"))
				nickserv->opts |= NS_SID;
			else if (!strcmp(config->seccion[i]->item, "secure"))
				nickserv->opts |= NS_SMAIL;
			else if (!strcmp(config->seccion[i]->item, "regcode"))
				nickserv->opts |= NS_REGCODE;
			else if (!strcmp(config->seccion[i]->item, "prot"))
			{
				nickserv->opts &= ~(NS_PROT_KILL | NS_PROT_CHG);
				if (!strcasecmp(config->seccion[i]->data, "KILL"))
					nickserv->opts |= NS_PROT_KILL;
				else
					nickserv->opts |= NS_PROT_CHG;
			}
			else if (!strcmp(config->seccion[i]->item, "recovernick"))
				ircstrdup(nickserv->recovernick, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "securepass"))
				ircstrdup(nickserv->securepass, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "min_reg"))
				nickserv->min_reg = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "autodrop"))
				nickserv->autodrop = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "nicks"))
				nickserv->nicks = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "intentos"))
				nickserv->intentos = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "maxlist"))
				nickserv->maxlist = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "forbmails"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
					ircstrdup(nickserv->forbmail[nickserv->forbmails++], config->seccion[i]->seccion[p]->item);
			}
			else if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, nickserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, nickserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
		}
	}
	else
	{
		nickserv->opts |= NS_SID;
		nickserv->opts |= NS_SMAIL;
		nickserv->opts |= NS_PROT_KILL;
		ProcesaComsMod(NULL, mod, nickserv_coms);
	}
	InsertaSenyal(SIGN_PRE_NICK, NSCmdPreNick);
	InsertaSenyal(SIGN_POST_NICK, NSCmdPostNick);
	InsertaSenyal(SIGN_UMODE, NSCmdUmode);
	InsertaSenyal(SIGN_SQL, NSSigSQL);
	InsertaSenyal(SIGN_QUIT, NSSigQuit);
	InsertaSenyal(NS_SIGN_IDOK, NSSigIdOk);
	InsertaSenyal(SIGN_SOCKCLOSE, NSSigSockClose);
	InsertaSenyal(SIGN_SYNCH, NSSigSynch);
	BotSet(nickserv);
}
KillUser *InsertaKillUser(Cliente *cl, int kill)
{
	KillUser *ku;
	ku = BMalloc(KillUser);
	ku->cl = cl;
	ku->timer = IniciaCrono(1, kill, NSKillea, strdup(cl->nombre));
	AddItem(ku, killusers);
	return ku;
}
KillUser *BuscaKillUser(Cliente *cl)
{
	KillUser *aux;
	for (aux = killusers; aux; aux = aux->sig)
	{
		if (aux->cl == cl)
			return aux;
	}
	return NULL;
}
int BorraKillUser(Cliente *cl, int apaga)
{
	KillUser *ku;
	if ((ku = BuscaKillUser(cl)))
	{
		Free(ku->timer->args);
		if (apaga)
			ApagaCrono(ku->timer);
		LiberaItem(ku, killusers);
	}
	return 0;
}
void NSCambiaInv(Cliente *cl)
{
	char buf[BUFSIZE];
	if (!ProtFunc(P_CAMBIO_USUARIO_REMOTO))
		return;
	ircsprintf(buf, nickserv->recovernick, cl->nombre);
	ProtFunc(P_CAMBIO_USUARIO_REMOTO)(cl, AleatorioEx(buf));
}
void NSMarca(Cliente *cl, char *nombre, char *marca)
{
	char *marcas;
	if (IsReg(nombre))
	{
		time_t fecha = time(NULL);
		if ((marcas = SQLCogeRegistro(NS_SQL, nombre, "marcas")))
			SQLInserta(NS_SQL, nombre, "marcas", "%s\t\00312%s\003 - \00312%s\003 - %s", marcas, Fecha(&fecha), cl->nombre, marca);
		else
			SQLInserta(NS_SQL, nombre, "marcas", "\00312%s\003 - \00312%s\003 - %s", Fecha(&fecha), cl->nombre, marca);
	}
}
BOTFUNCHELP(NSHIdentify)
{
	Responde(cl, CLI(nickserv), "Te identifica como propietario del nick.");
	Responde(cl, CLI(nickserv), "Para ello debes proporcionar una contraseña de autentificación, la misma que utilizaste en el registro de tu nick.");
	Responde(cl, CLI(nickserv), "Una vez identificado como propietario podrás disponer de cuantiosas oportunidades que te ofrece el reigstro de nicks.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312IDENTIFY tupass");
	return 0;
}
BOTFUNCHELP(NSHInfo)
{
	Responde(cl, CLI(nickserv), "Muestra distinta información de un nick, como su fecha de registro o su último quit, entre otros.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312INFO nick");
	return 0;
}
BOTFUNCHELP(NSHList)
{
	Responde(cl, CLI(nickserv), "Lista todos los nicks registrados que coincidan con un patrón especificado.");
	Responde(cl, CLI(nickserv), "Para evitar abusos de flood, este comando emite como máximo \00312%i\003 entradas.", nickserv->maxlist);
	Responde(cl, CLI(nickserv), "El patrón puede ser exacto, o puede contener comodines (*) para cerrar el campo de búsqueda.");
	Responde(cl, CLI(nickserv), "Además, puedes especificar un filtro para listar los nicks.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Nicks activos: \00312LIST -a patrón");
	Responde(cl, CLI(nickserv), "Nicks suspendidos (Solo Opers): \00312LIST -s patrón");
	Responde(cl, CLI(nickserv), "Nicks prohibidos (Solo Opers): \00312LIST -f patrón");
	Responde(cl, CLI(nickserv), "Email (Solo Opers): \00312LIST -e patrón");
	Responde(cl, CLI(nickserv), "Ipvirtual (Solo Opers): \00312LIST -v patrón");
	Responde(cl, CLI(nickserv), "Host (Solo Opers): \00312LIST -h patrón");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312LIST [-filtro] patrón");
	return 0;
}
BOTFUNCHELP(NSHGhost)
{
	Responde(cl, CLI(nickserv), "Este comando desconecta a un usuario que lleve puesto tu nick.");
	Responde(cl, CLI(nickserv), "Para ello deberás especificar la contraseña de propietario del nick.");
	Responde(cl, CLI(nickserv), "Si la contraseña es válida, %s.", nickserv->opts & NS_PROT_KILL ? "se desconectará el usuario" : "se le cambiará el nick al usuario");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312GHOST nick contraseña");
	return 0;
}
BOTFUNCHELP(NSHSet)
{
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), "Permite fijar diferentes opciones para tu nick.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Opciones disponibles:");
		Responde(cl, CLI(nickserv), "\00312PASS\003 Cambia tu contraseña de usuario.");
		Responde(cl, CLI(nickserv), "\00312EMAIL\003 Cambia tu dirección de correo.");
		Responde(cl, CLI(nickserv), "\00312URL\003 Cambia tu página web.");
		Responde(cl, CLI(nickserv), "\00312KILL\003 Activa/Desactiva tu protección de KILL.");
		if (IsOper(cl))
			Responde(cl, CLI(nickserv), "\00312NODROP\003 Activa/Desactiva tu protección de DROP.");
		Responde(cl, CLI(nickserv), "\00312HIDE\003 Opciones para mostrar en tu INFO.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Para más información, \00312/msg %s %s SET opción", nickserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		Responde(cl, CLI(nickserv), "\00312SET PASS");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu contraseña de usuario.");
		Responde(cl, CLI(nickserv), "A partir de este cambio, ésta será la contraseña con la que debes identificarte.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET PASS nuevopass");
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET EMAIL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu dirección de correo.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET email direccion@correo");
	}
	else if (!strcasecmp(param[2], "URL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET URL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu página web personal.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET url http://pagina.web");
	}
	else if (!strcasecmp(param[2], "KILL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET KILL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Especifica tu protección KILL.");
		Responde(cl, CLI(nickserv), "Con esta protección, si no te identificas antes de los segundos que especifiques %s.", nickserv->opts & NS_PROT_KILL ? "serás desconectado" : "se te cambiará el nick");
		Responde(cl, CLI(nickserv), "Si especificas el 0 como valor, esta protección se desactiva.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312KILL segundos");
	}
	else if (!strcasecmp(param[2], "NODROP") && IsOper(cl))
	{
		Responde(cl, CLI(nickserv), "\00312SET NODROP");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Evita que tu nick pueda ser dropeado, a excepción de los Administradores.");
		Responde(cl, CLI(nickserv), "Si está en ON, tu nick no podrá dropearse.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET NODROP on|off");
	}
	else if (!strcasecmp(param[2], "HIDE"))
	{
		Responde(cl, CLI(nickserv), "\00312SET HIDE");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Permite mostrar u ocultar distintas opciones de tu INFO.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Opciones para mostrar:");
		Responde(cl, CLI(nickserv), "\00312EMAIL\003 Muestra tu email.");
		Responde(cl, CLI(nickserv), "\00312URL\003 Muestra tu página personal.");
		Responde(cl, CLI(nickserv), "\00312MASK\003 Muestra tu user@host.");
		Responde(cl, CLI(nickserv), "\00312TIME\003 Muestra la última hora que fuiste visto en la red.");
		Responde(cl, CLI(nickserv), "\00312QUIT\003 Muestra tu último quit.");
		Responde(cl, CLI(nickserv), "\00312LIST\003 Se te mostrará cuando alguien haga un LIST.");
		Responde(cl, CLI(nickserv), "\00312LOC\003 Muestra la posición geográfica desde dónde estás conectado");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET HIDE opcion on|off");
	}
	else
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNCHELP(NSHRegister)
{
	Responde(cl, CLI(nickserv), "Permite registrar tu nick en la red para que nadie sin autorización pueda utilizarlo.");
	Responde(cl, CLI(nickserv), "Para utilizar tu nick registrado deberás identificarte como propietario mediante el comando \00312IDENTIFY\003.");
	Responde(cl, CLI(nickserv), "El registro del nick requiere una cuenta de email válida. Recuerda que si no lo es, no podrás recibir tu nueva contraseña en caso de que la perdieras.");
	if (nickserv->opts & NS_SMAIL)
		Responde(cl, CLI(nickserv), "Una vez hayas registrado tu nick, te llegará en tu correo email una clave temporal, que será la que deberás usar para identificarte.");
	Responde(cl, CLI(nickserv), " ");
	if (nickserv->opts & NS_SMAIL)
		Responde(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tu@email");
	else
		Responde(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tupass tu@email");
	return 0;
}
BOTFUNCHELP(NSHSwhois)
{
	Responde(cl, CLI(nickserv), "Añade o borra un whois especial a un usuario.");
	Responde(cl, CLI(nickserv), "Este whois se le añadirá cuando se identifique.");
	Responde(cl, CLI(nickserv), "Se mostrará con el comando WHOIS.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SWHOIS nick [swhois]");
	Responde(cl, CLI(nickserv), "Si no se especifica swhois, se borrará el que pudiera haber.");
	Responde(cl, CLI(nickserv), "NOTA: Es posible que la red no soporte este comando (sólo unreal).");
	return 0;
}
BOTFUNCHELP(NSHSuspender)
{
	Responde(cl, CLI(nickserv), "Suspende el uso de un determinado nick.");
	Responde(cl, CLI(nickserv), "Este nick podrá ponerselo pero no tendrá privilegios algunos.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SUSPENDER nick motivo");
	return 0;
}
BOTFUNCHELP(NSHLiberar)
{
	Responde(cl, CLI(nickserv), "Libera un nick de su suspenso.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312LIBERAR nick");
	return 0;
}
BOTFUNCHELP(NSHRename)
{
	Responde(cl, CLI(nickserv), "Cambia el nick a un usuario conectado.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312RENAME nick nuevonick");
	return 0;
}
BOTFUNCHELP(NSHForbid)
{
	Responde(cl, CLI(nickserv), "Prohibe el uso de un nick o apodo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312FORBID nick [motivo]");
	Responde(cl, CLI(nickserv), "Si no se especifica swhois, se borrará el que pudiera haber.");
	return 0;
}
BOTFUNCHELP(NSHUnForbid)
{
	Responde(cl, CLI(nickserv), "Permite el uso de un nick o apodo prohibido.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312UNFORBID nick");
	return 0;
}
BOTFUNCHELP(NSHDrop)
{
	Responde(cl, CLI(nickserv), "Borra un nick de la base de datos.");
	Responde(cl, CLI(nickserv), "Este desregistro anula completamente la propiedad de un usuario sobre un nick.");
	Responde(cl, CLI(nickserv), "Una vez borrado, no se puede volver a recuperar si no es registrándolo de nuevo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312DROP nick");
	return 0;
}
BOTFUNCHELP(NSHSendpass)
{
	Responde(cl, CLI(nickserv), "Regenera una contraseña de usuario y se la envía a su correo.");
	Responde(cl, CLI(nickserv), "Este comando sólo debe en caso de pérdida de la contraseña del usuario y a petición del mismo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SENDPASS nick");
	return 0;
}
BOTFUNCHELP(NSHMarcas)
{
	Responde(cl, CLI(nickserv), "Inserta una marca en el historial de un nick.");
	Responde(cl, CLI(nickserv), "Las marcas son anotaciones que no pueden eliminarse.");
	Responde(cl, CLI(nickserv), "En ellas figura la fecha, su autor y un comentario.");
	Responde(cl, CLI(nickserv), "Si no se especifica ninguna marca, se listarán todas las que hubiera.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312MARCA [nick]");
	return 0;
}
BOTFUNCHELP(NSHOptsNick)
{
	Responde(cl, CLI(nickserv), "Cambia las opciones de un nick.");
	Responde(cl, CLI(nickserv), "Son las mismas opciones que puedes cambiarte tú pero lo haces a otro nick.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SETNICK nick opcion valor");
	return 0;
}
BOTFUNC(NSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), "\00312%s\003 se encarga de gestionar los nicks de la red para evitar robos de identidad.", nickserv->hmod->nick);
		Responde(cl, CLI(nickserv), "El registro de tu nick permite que otros no lo utilicen, mostrarte como propietario y aventajarte de cuantiosas oportunidades.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Comandos disponibles:");
		ListaDescrips(nickserv->hmod, cl);
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Para más información, \00312/msg %s %s comando", nickserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], nickserv->hmod, param, params))
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opción desconocida.");
	EOI(nickserv, 0);
	return 0;
}
BOTFUNC(NSRegister)
{
	u_int i, opts;
	char *dominio, *mail, *pass, *usermask, *m_c, *ll, *hh, *regcode;
	SQLRes res = NULL;
	usermask = strchr(cl->mask, '!') + 1;
	if (strlen(cl->nombre) > (u_int)(protocolo->nicklen - 7)) /* 6 minimo de contraseña +1 de los ':' */
	{
		ircsprintf(buf, "Tu nick es demasiado largo. Sólo puede tener %i caracteres como máximo.", (protocolo->nicklen - 7));
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if (params < ((nickserv->opts & NS_SMAIL) ? 2 : 3))
	{
		if (nickserv->opts & NS_SMAIL)
			Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "tu@email");
		else
			Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "tupass tu@email");
		return 1;
	}
	if (CogeCache(CACHE_ULTIMO_REG, usermask, nickserv->hmod->id))
	{
		char buf[512];
		ircsprintf(buf, "No puedes efectuar otro registro hasta que no pasen %i horas.", nickserv->min_reg);
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	/* comprobamos que no esté registrado */
	if (IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick ya está registrado.");
		return 1;
	}
	if (nickserv->opts & NS_SMAIL)
	{
		mail = param[1];
		pass = NULL;
		regcode = param[2];
	}
	else
	{
		mail = param[2];
		pass = param[1];
		regcode = param[3];
	}
	/* comprobamos su email */
	if (!(dominio = strchr(mail, '@')) || !Mx(dominio+1))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No parece ser una cuenta de email válida.");
		return 1;
	}
	dominio++;
	for (i = 0; i < nickserv->forbmails; i++)
	{
		if (!strcasecmp(nickserv->forbmail[i], dominio))
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Esta cuenta de correo está prohibida. Usa otra.");
			return 1;
		}
	}
	if (nickserv->nicks && ((res = SQLQuery("SELECT * from %s%s where email='%s'", PREFIJO, NS_SQL, mail)) && SQLNumRows(res) == nickserv->nicks))
	{
		SQLFreeRes(res);
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes registrar más nicks en esta cuenta.");
		return 1;
	}
	SQLFreeRes(res);
	if (pass && strchr(pass, '!'))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "La contraseña no puede contener el signo !");
		return 1;
	}
	if (nickserv->opts & NS_REGCODE)
	{
		char *r = CogeCache(CACHE_REGCODE, cl->nombre, nickserv->hmod->id);
		if (!regcode)
		{
			int c = Aleatorio(0,3), colores[] = { 12, 3, 4, 8, 0, 1, 2, 5, 6, 7, 9, 10, 11, 13, 14, 15 };
			char *d, tmp[8], *l = NULL;
			if (!r)
			{
				InsertaCache(CACHE_REGCODE, cl->nombre, 86400, nickserv->hmod->id, AleatorioEx("######"));
				r = CogeCache(CACHE_REGCODE, cl->nombre, nickserv->hmod->id);
			}
			d = AleatorioEx("######");
			buf[0] = 0;
			ircsprintf(tokbuf, "%i", colores[c]);
			while (*r && *d)
			{
				strcat(buf, "\003");
				if (Aleatorio(0,1))
				{
					strcat(buf, tokbuf);
					chrcat(buf, *r++);
				}
				else
				{
					ircsprintf(tmp, "%i", colores[Aleatorio(4, 15)]);
					strcat(buf, tmp);
					chrcat(buf, *d++);
				}
			}
			if (*r)
			{
				strcat(buf, "\003");
				strcat(buf, tokbuf);
				strcat(buf, r);
			}
			else
			{
				while (*d)
				{
					strcat(buf, "\003");
					ircsprintf(tmp, "%i", colores[Aleatorio(4, 15)]);
					strcat(buf, tmp);
					chrcat(buf, *d++);
				}
			}
			if (c == 0)
				l = "azul";
			else if (c == 1)
				l = "verde";
			else if (c == 2)
				l = "rojo";
			else if (c == 3)
				l = "amarillo";
			Responde(cl, CLI(nickserv), "Para continuar debes introducir las letras que sean de color \002\003%i%s\xF del siguiente código: \002%s", colores[Aleatorio(4, 15)], l, buf);
			Responde(cl, CLI(nickserv), "Ejecuta /msg %s %s %s %s codigo", CLI(nickserv)->nombre, fc->com, param[1], (pass ? param[2] : ""));
			return 1;
		}
		else
		{
			if (!r || strcmp(r, regcode))
			{
				Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Código incorrecto.");
				return 1;
			}
		}
	}
	opts = NS_OPT_MASK;
	if (nickserv->opts & NS_HIDEFMAIL)
		opts |= NS_OPT_MAIL;
	m_c = SQLEscapa(cl->info);
	ll = SQLEscapa(cl->nombre);
	hh = SQLEscapa(cl->ident);
	if (!pass)
		pass = AleatorioEx(nickserv->securepass);
	SQLQuery("INSERT INTO %s%s (item,pass,email,gecos,host,opts,id,reg,last) VALUES ('%s','%s','%s','%s','%s@%s',%lu,%lu,%lu,%lu)",
			PREFIJO, NS_SQL,
			ll, pass ? MDString(pass, 0) : "null",
			mail, m_c,
			hh, cl->host,
			opts, nickserv->opts & NS_SMAIL ? 0 : time(0),
			time(0), time(0));
	Free(m_c);
	Free(ll);
	Free(hh);
	if (sql->_errno)
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Ha sido imposible insertar tu nick en la base de datos. Vuelve a probarlo.");
		return 1;
	}
	if (nickserv->opts & NS_SMAIL)
		Email(mail, "Nueva contraseña", "Debido al registro del nick %s, se ha generado una contraseña totalmente segura.\r\n"
		"A partir de ahora, la clave de %s es:\r\n\r\n%s\r\n\r\nPuede cambiarla mediante el comando /msg %s SET pass.\r\n\r\nGracias por utilizar los servicios de %s.", cl->nombre, cl->nombre, pass, nickserv->hmod->nick, conf_set->red);
	else if (umodreg)
		ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", umodreg->flag);
	if (!IsOper(cl))
		InsertaCache(CACHE_ULTIMO_REG, usermask, 3600 * nickserv->min_reg, nickserv->hmod->id, "%lu", time(0));
	Responde(cl, CLI(nickserv), "Tu nick ha sido registrado bajo la cuenta \00312%s\003.", mail);
	LlamaSenyal(NS_SIGN_REG, 1, cl->nombre);
	EOI(nickserv, 1);
	return 0;
}
BOTFUNC(NSIdentify)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No tienes el nick registrado.");
		return 1;
	}
	if (IsSusp(cl->nombre))
	{
		ircsprintf(buf, "Tienes el nick suspendido: %s", SQLCogeRegistro(NS_SQL, cl->nombre, "suspend"));
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if ((nickserv->opts & NS_SID) && !parv[parc])
	{
		ircsprintf(buf, "Identificación incorrecta. /msg %s@%s IDENTIFY pass", nickserv->hmod->nick, me.nombre);
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if (!strcmp(MDString(param[1], 0), SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
	{
		if (umodreg)
			ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", umodreg->flag);
		Responde(cl, CLI(nickserv), "Ok \00312%s\003, bienvenid@ a casa :)", cl->nombre);
		LlamaSenyal(NS_SIGN_IDOK, 1, cl);
	}
	else
	{
		int intentos = 0;
		char *cache;
		if ((cache = CogeCache(CACHE_INTENTOS_ID, cl->nombre, nickserv->hmod->id)))
			intentos = atoi(cache);
		if (++intentos == nickserv->intentos)
		{
			BorraCache(CACHE_INTENTOS_ID, cl->nombre, nickserv->hmod->id);
			ProtFunc(P_QUIT_USUARIO_REMOTO)(cl, CLI(nickserv), "Demasiadas contraseñas inválidas.");
		}
		else
		{
			ircsprintf(buf, "Contraseña incorrecta. Tienes %i intentos más.", nickserv->intentos - intentos);
			Responde(cl, CLI(nickserv), buf);
			InsertaCache(CACHE_INTENTOS_ID, cl->nombre, 86400, nickserv->hmod->id, "%i", intentos);
		}
	}
	EOI(nickserv, 2);
	return 0;
}
BOTFUNC(NSOpts)
{
	int ret;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "opción valor");
		return 1;
	}
	ret = NickOpts(cl, cl->nombre, param, params, fc);
	EOI(nickserv, 3);
	return ret;
}
BOTFUNC(NSDrop)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (IsForbid(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NFORB);
		return 1;
	}
	if (!IsAdmin(cl) && (atoi(SQLCogeRegistro(NS_SQL, param[1], "opts")) & NS_OPT_NODROP))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no se puede dropear.");
		return 1;
	}
	if (!NSBaja(param[1], 1))
		Responde(cl, CLI(nickserv), "Se ha dropeado el nick \00312%s\003.", param[1]);
	else
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se ha podido dropear. Comuníquelo a la administración.");
	EOI(nickserv, 4);
	return 0;
}
int NSBaja(char *nick, int opt)
{
	Cliente *al;
	int opts = atoi(SQLCogeRegistro(NS_SQL, nick, "opts"));
	if (!opt && (opts & NS_OPT_NODROP))
		return 1;
	LlamaSenyal(NS_SIGN_DROP, 1, nick);
	if ((al = BuscaCliente(nick)))
		ProtFunc(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "-r");
	SQLBorra(NS_SQL, nick);
	return 0;
}
BOTFUNC(NSSendpass)
{
	char *pass;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	pass = AleatorioEx(nickserv->securepass);
	SQLInserta(NS_SQL, param[1], "pass", MDString(pass, 0));
	Email(SQLCogeRegistro(NS_SQL, param[1], "email"), "Reenvío de la contraseña", "Debido a la pérdida de tu contraseña, se te ha generado otra clave totalmente segura.\r\n"
		"A partir de ahora, la clave de tu nick es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, nickserv->hmod->nick, conf_set->red);
	Responde(cl, CLI(nickserv), "Se ha generado y enviado otra contraseña al email de \00312%s\003.", param[1]);
	EOI(nickserv, 5);
	return 0;
}
BOTFUNC(NSInfo)
{
	int comp;
	SQLRes res;
	SQLRow row;
	time_t reg;
	int opts;
	char *ll;
	Cliente *al;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick");
		return 1;
	}
	al = BuscaCliente(param[1]);
	if (IsForbid(param[1])) //Si esta prohibido, damos la informacion corta
        {
		Responde(cl, CLI(nickserv), "Información de \00312%s", param[1]);
	        Responde(cl, CLI(nickserv), "Estado: \0034PROHIBIDO");
	        Responde(cl, CLI(nickserv), "Motivo: \00312%s", SQLCogeRegistro(NS_SQL, param[1], "motivo"));
	        return 1;      
        }
	if (IsReg(param[1]))
	{
		comp = strcasecmp(param[1], cl->nombre);
		ll = SQLEscapa(param[1]);
		res = SQLQuery("SELECT opts,gecos,reg,motivo,host,quit,last,email,url,killtime,item,ipvirtual,ipcaduca,swhois from %s%s where item='%s'", PREFIJO, NS_SQL, ll);
		row = SQLFetchRow(res);
		opts = atoi(row[0]);
		Responde(cl, CLI(nickserv), "Información de \00312%s", row[10]);
		if (IsActivo(param[1]))
			Responde(cl, CLI(nickserv), "Estado: \00312ACTIVO");
		if (IsSusp(param[1])) {
			Responde(cl, CLI(nickserv), "Estado: \0038SUSPENDIDO");
			Responde(cl, CLI(nickserv), "Motivo del suspenso: \00312%s", row[3]);
		}
		Responde(cl, CLI(nickserv), "Realname: \00312%s", row[1]);
		reg = (time_t)atol(row[2]);
		Responde(cl, CLI(nickserv), "Registrado el \00312%s", Fecha(&reg));
		if (!(opts & NS_OPT_MASK) || (!comp && IsId(cl)) || IsOper(cl))
			Responde(cl, CLI(nickserv), "Último mask: \00312%s", row[4]);
		if (!(opts & NS_OPT_TIME) || (!comp && IsId(cl)) || IsOper(cl))
		{
			if (al && IsId(al))
			{
				Responde(cl, CLI(nickserv), "Usuario conectado. Utiliza /WHOIS %s para más información.", param[1]);
				if (al->loc && (!(opts & NS_OPT_LOC) || (!comp && IsId(cl)) || IsOper(cl)))
					Responde(cl, CLI(nickserv), "Conectado desde: \00312%s, %s, %s", al->loc->city, al->loc->state, al->loc->country);
			}
			else
			{
				reg = (time_t)atol(row[6]);
				Responde(cl, CLI(nickserv), "Visto por última vez el \00312%s", Fecha(&reg));
			}
		}
		if (!BadPtr(row[5]) && (!(opts & NS_OPT_QUIT) || (!comp && IsId(cl)) || IsOper(cl)))
			Responde(cl, CLI(nickserv), "Último quit: \00312%s", row[5]);
		if (!(opts & NS_OPT_MAIL) || (!comp && IsId(cl)) || IsOper(cl))
			Responde(cl, CLI(nickserv), "Email: \00312%s", row[7]);
		if (!BadPtr(row[8]) && (!(opts & NS_OPT_WEB) || (!comp && IsId(cl)) || IsOper(cl)))
			Responde(cl, CLI(nickserv), "Url: \00312%s", row[8]);		
		if (!BadPtr(row[11]))
			Responde(cl, CLI(nickserv), "IP Virtual: \00312%s", row[11]);
		if (!BadPtr(row[11]) && !BadPtr(row[12]) && ((!comp && IsId(cl)) || IsOper(cl))) 
		{
			reg = (time_t)atol(row[12]);
			Responde(cl, CLI(nickserv), "Caducidad IP Virtual: \00312%s", Fecha(&reg));
		}
		if (!BadPtr(row[13]))
			Responde(cl, CLI(nickserv), "Swhois: \00312%s", row[13]);
		if (atoi(row[9]))
			Responde(cl, CLI(nickserv), "Protección kill a \00312%i\003 segundos.", atoi(row[9]));
		if (opts & NS_OPT_NODROP)
			Responde(cl, CLI(nickserv), "Este nick no caduca.");
		EOI(nickserv, 6);
		SQLFreeRes(res);
		if ((!strcasecmp(cl->nombre, param[1]) && IsId(cl)) || IsOper(cl))
		{
			Responde(cl, CLI(nickserv), "*** Niveles de acceso ***");
			if ((res = SQLQuery("SELECT * FROM %s%s WHERE nick='%s'", PREFIJO, CS_ACCESS, ll)))
			{
				while ((row = SQLFetchRow(res)))
				{
					if (!BadPtr(row[2]) && !BadPtr(row[3]) && *row[2] != '0')
						Responde(cl, CLI(nickserv), "Canal: \00312%s\003 flags: \00312%s\003 [\00312%s\003] (\00312%s\003)", row[0], ModosAFlags(atoul(row[2]), cFlags, NULL), row[3], row[2]);
					else if (!BadPtr(row[3]))
						Responde(cl, CLI(nickserv), "Canal: \00312%s\003 flags: [\00312%s\003]", row[0], row[3]);
					else if (!BadPtr(row[2]) && *row[2] != '0')
						Responde(cl, CLI(nickserv), "Canal: \00312%s\003 flags: \00312%s\003 (\00312%s\003)", row[0], ModosAFlags(atoul(row[2]), cFlags, NULL), row[2]);
				}
				SQLFreeRes(res);
			}
			if ((res = SQLQuery("SELECT item from %s%s where founder='%s'", PREFIJO, CS_SQL, ll)))
			{
				while ((row = SQLFetchRow(res)))
				{
					ircsprintf(tokbuf, " \003[\00312%s\003]", protocolo->modcl);
					Responde(cl, CLI(nickserv), "Canal: \00312%s\003 flags: \00312+%s%s\003 (\00312FUNDADOR\003)", row[0],
						ModosAFlags(CS_LEV_ALL, cFlags, NULL), tokbuf);
				}
				SQLFreeRes(res);
			}
		}
		Free(ll);
	}
	else if (al && IsOper(cl))
	{
		Responde(cl, CLI(nickserv), "Información de \00312%s", al->nombre);
		Responde(cl, CLI(nickserv), "Estado: \2NO REGISTRADO");
		Responde(cl, CLI(nickserv), "Realname: \00312%s", al->info);
		if (al->loc)
			Responde(cl, CLI(nickserv), "Conectado desde: \00312%s, %s, %s", al->loc->city, al->loc->state, al->loc->country);
	}
	else
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	return 0;
}
BOTFUNC(NSList)
{
	char *rep;
	SQLRes res;
	SQLRow row;
	int i;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "[-filtro] patrón");
		return 1;
	}

	if (params == 3)
	{
		if (*param[1] == '-')
		{
			if (params < 3)
			{
				Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "[-filtro] patrón");
				return 1;
			}
			switch(*(param[1] + 1))
			{
				case 'a':
					rep = SQLEscapa(str_replace(param[2], '*', '%'));
					if (!(res = SQLQuery("SELECT item from %s%s where item LIKE '%s' AND estado='A'", PREFIJO, NS_SQL, rep)))
					{
						Free(rep);
						Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
						return 1;
					}
					Free(rep);
					Responde(cl, CLI(nickserv), "*** Nicks \00312activos\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
					for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
					{
						if (IsOper(cl) || !(atoi(SQLCogeRegistro(NS_SQL, row[0], "opts")) & NS_OPT_LIST))
						{
							Responde(cl, CLI(nickserv), "%s", row[0]);
							i++;
						}
					}
					Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
					SQLFreeRes(res);
					break;
				case 's':
					if (IsOper(cl))
					{
						rep = SQLEscapa(str_replace(param[2], '*', '%'));
						if (!(res = SQLQuery("SELECT item from %s%s where item LIKE '%s' AND estado='S'", PREFIJO, NS_SQL, rep)))
						{
							Free(rep);
							Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
							return 1;
						}
						Free(rep);
						Responde(cl, CLI(nickserv), "*** Nicks \00312suspendidos\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
						for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
						{
							if (IsOper(cl))
							{
								Responde(cl, CLI(nickserv), "%s", row[0]);
								i++;
							}
						}
						Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
						SQLFreeRes(res);
					}
					else
					Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No esta autorizado a ver este listado.");
					break;
				case 'f':
					if (IsOper(cl))
					{
						rep = SQLEscapa(str_replace(param[2], '*', '%'));
						if (!(res = SQLQuery("SELECT item from %s%s where item LIKE '%s' AND estado='F'", PREFIJO, NS_SQL, rep)))
						{
							Free(rep);
							Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
							return 1;
						}
						Free(rep);
						Responde(cl, CLI(nickserv), "*** Nicks \00312prohibidos\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
						for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
						{
							Responde(cl, CLI(nickserv), "%s", row[0]);
							i++;
						}
						Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
						SQLFreeRes(res);
					}
					else
					Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No esta autorizado a ver este listado.");
					break;
				case 'e':
					if (IsOper(cl))
					{
						rep = SQLEscapa(str_replace(param[2], '*', '%'));
						if (!(res = SQLQuery("SELECT item,email from %s%s where email LIKE '%s'", PREFIJO, NS_SQL, rep)))
						{
							Free(rep);
							Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
							return 1;
						}
						Free(rep);
						Responde(cl, CLI(nickserv), "*** Nicks con \00312email\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
						for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
						{
							Responde(cl, CLI(nickserv), "Nick: \00312%s\003 Email: \00312%s\003", row[0],row[1]);
							i++;
						}
						Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
						SQLFreeRes(res);
					}
					else
					Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No esta autorizado a ver este listado.");
					break;
				case 'v':
					if (IsOper(cl))
					{
						rep = SQLEscapa(str_replace(param[2], '*', '%'));
						if (!(res = SQLQuery("SELECT item,ipvirtual from %s%s where ipvirtual LIKE '%s' AND ipvirtual!=''", PREFIJO, NS_SQL, rep)))
						{
							Free(rep);
							Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
							return 1;
						}
						Free(rep);
						Responde(cl, CLI(nickserv), "*** Nicks con \00312ipvirtual\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
						for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
						{
							Responde(cl, CLI(nickserv), "Nick: \00312%s\003 Ipvirtual: \00312%s\003", row[0],row[1]);
							i++;
						}
						Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
						SQLFreeRes(res);
					}
					else
					Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No esta autorizado a ver este listado.");
					break;
				case 'h':
					if (IsOper(cl))
					{
						rep = SQLEscapa(str_replace(param[2], '*', '%'));
						if (!(res = SQLQuery("SELECT item,host from %s%s where host LIKE '%s'", PREFIJO, NS_SQL, rep)))
						{
							Free(rep);
							Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
							return 1;
						}
						Free(rep);
						Responde(cl, CLI(nickserv), "*** Nicks con \00312host\003 que coinciden con el patrón \00312%s\003 ***", param[2]);
						for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
						{
							Responde(cl, CLI(nickserv), "Nick: \00312%s\003 Host: \00312%s\003", row[0],row[1]);
							i++;
						}
						Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
						SQLFreeRes(res);
					}
					else
					Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No esta autorizado a ver este listado.");
					break;
				default:
					Responde(cl, CLI(nickserv), NS_ERR_SNTX, "Parámetro no válido.");
					return 1;
			}
		}
	}
	else
	{
		rep = SQLEscapa(str_replace(param[1], '*', '%'));
		if (!(res = SQLQuery("SELECT item from %s%s where item LIKE '%s'", PREFIJO, NS_SQL, rep)))
		{
			Free(rep);
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}

		Free(rep);
		Responde(cl, CLI(nickserv), "*** Nicks que coinciden con el patrón \00312%s\003 ***", param[1]);

		for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res));)
		{
			if (IsOper(cl) || !(atoi(SQLCogeRegistro(NS_SQL, row[0], "opts")) & NS_OPT_LIST))
			{
				char *estado = "\00312ACTIVO\003";
				if (IsSusp(row[0]))
					estado = "\0038SUSPENDIDO\003";
				if (IsForbid(row[0]))
					estado = "\0034PROHIBIDO\003";

				Responde(cl, CLI(nickserv), "%s (%s)", row[0], estado);
				i++;
			}
		}

		Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	EOI(nickserv, 7);
	return 0;
}
BOTFUNC(NSGhost)
{
	Cliente *al;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick pass");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (!strcasecmp(param[1], cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes ejecutar este comando sobre ti mismo.");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, param[1], "pass"), MDString(param[2], 0)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	if (nickserv->opts & NS_PROT_KILL)
		ProtFunc(P_QUIT_USUARIO_REMOTO)(al, CLI(nickserv), "Comando GHOST utilizado por %s.", cl->nombre);
	else
		NSCambiaInv(al);
	Responde(cl, CLI(nickserv), "Nick \00312%s\003 liberado.", param[1]);
	EOI(nickserv, 8);
	return 0;
}
BOTFUNC(NSSuspender)
{
	char *motivo = NULL;
	Cliente *al = NULL;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick motivo");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
	SQLInserta(NS_SQL, param[1], "motivo", motivo);
	SQLInserta(NS_SQL, param[1], "estado", "S");
	ircsprintf(buf, "Suspendido: %s", motivo);
	NSMarca(cl, param[1], buf);
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido suspendido.", param[1]);
	if ((al = BuscaCliente(param[1])))
	{
		Responde(al, CLI(nickserv), "Tu nick ha sido suspendido por \00312%s\003: %s", cl->nombre, motivo);
		NSCambiaInv(al);
	}
	EOI(nickserv, 9);
	return 0;
}
BOTFUNC(NSLiberar)
{
	Cliente *al = NULL;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!IsSusp(param[1]))	
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no está suspendido.");
		return 1;
	}
	SQLInserta(NS_SQL, param[1], "motivo", "");
	SQLInserta(NS_SQL, param[1], "estado", "A");
	NSMarca(cl, param[1], "Suspenso levantado.");
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido liberado de su suspenso.", param[1]);
	if ((al = BuscaCliente(param[1])))
		NSCambiaInv(al);
	EOI(nickserv, 10);
	return 0;
}
BOTFUNC(NSSwhois)
{
	if (!ProtFunc(P_WHOIS_ESPECIAL))
	{
		Responde(cl, CLI(nickserv), ERR_NSUP);
		return 1;
	}
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick [swhois]");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (params >= 3)
	{
		char *swhois;
		Cliente *al;
		swhois = Unifica(param, params, 2, -1);
		SQLInserta(NS_SQL, param[1], "swhois", swhois);
		if ((al = (BuscaCliente(param[1]))))
			ProtFunc(P_WHOIS_ESPECIAL)(al, swhois);
		Responde(cl, CLI(nickserv), "El swhois de \00312%s\003 ha sido cambiado.", param[1]);
	}
	else
	{
		SQLInserta(NS_SQL, param[1], "swhois", NULL);
		Responde(cl, CLI(nickserv), "Se ha eliminado el swhois de \00312%s", param[1]);
	}
	EOI(nickserv, 11);
	return 0;
}
BOTFUNC(NSRename)
{
	Cliente *al;
	if (!ProtFunc(P_CAMBIO_USUARIO_REMOTO))
	{
		Responde(cl, CLI(nickserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick nuevonick");
		return 1;
	}
	if ((al = BuscaCliente(param[2])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "El nuevo nick ya está en uso.");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no está conectado.");
		return 1;
	}
	ProtFunc(P_CAMBIO_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha cambiado su nick a \00312%s\003.", param[1], param[2]);
	EOI(nickserv, 12);
	return 0;
}
BOTFUNC(NSForbid)
{
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick [motivo]");
		return 1;
	}

	if (IsForbid(param[1])) {
		Responde(cl, CLI(nickserv), NS_ERR_NFORB);
		return 1;	
	}
		
	if (IsReg(param[1]))
		NSBaja(param[1], 1);

	SQLQuery("INSERT INTO %s%s (item,opts) VALUES ('%s',%s)",
		PREFIJO, NS_SQL,
		param[1], "200"); //Hacemos el nick no dropable

	if (sql->_errno)
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Ha sido imposible prohibir el nick. Vuelve a probarlo.");
		return 1;
	}

	motivo = Unifica(param, params, 2, -1);
	SQLInserta(NS_SQL, param[1], "estado", "F");
	SQLInserta(NS_SQL, param[1], "motivo", motivo);
	if (ProtFunc(P_FORB_NICK))
		ProtFunc(P_FORB_NICK)(param[1], ADD,  motivo);
	ircsprintf(buf, "Prohibido: %s", motivo);
	NSMarca(cl, param[1], buf);
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido prohibido.", param[1]);
	EOI(nickserv, 13);
	return 0;
}
BOTFUNC(NSUnForbid)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick");
		return 1;
	}

	if (!IsForbid(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NUFORB);
		return 1;	
	}	

	SQLBorra(NS_SQL, param[1]);
	if (ProtFunc(P_FORB_NICK))
		ProtFunc(P_FORB_NICK)(param[1], DEL, NULL);
	NSMarca(cl, param[1], "Prohibición levantada.");
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido permitido.", param[1]);
	EOI(nickserv, 14);
	return 0;
}
BOTFUNC(NSMarcas)
{
	char *marcas;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick [comentario]");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (params < 3)
	{
		if ((marcas = SQLCogeRegistro(NS_SQL, param[1], "marcas")))
		{
			char *tok;
			Responde(cl, CLI(nickserv), "Historial de \00312%s", param[1]);
			for (tok = strtok(marcas, "\t"); tok; tok = strtok(NULL, "\t"))
				Responde(cl, CLI(nickserv), tok);
		}
		else
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "El historial está vacío.");
			return 1;
		}
	}
	else
	{
		NSMarca(cl, param[1], Unifica(param, params, 2, -1));
		Responde(cl, CLI(nickserv), "Marca añadida al historial de \00312%s", param[1]);
	}
	EOI(nickserv, 15);
	return 0;
}
BOTFUNC(NSOptsNick)
{
	int ret;
	if (params < 4)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "nick opción valor");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	ret = NickOpts(cl, param[1], &param[1], params-1, fc);
	EOI(nickserv, 16);
	return ret;
}
int NSCmdUmode(Cliente *cl, char *modos) //Hacia que los nicks que se encontraban en la UDB se identificase dos veces - posible funcion obsoleta
{
	//if (conf_set->modos && conf_set->modos->usuarios)
	//	ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), conf_set->modos->usuarios);
	//if (umodreg && strchr(modos, umodreg->flag) && (cl->modos & UMODE_REGNICK))
	//	LlamaSenyal(NS_SIGN_IDOK, 1, cl);
	return 0;
}
/* si nuevo es NULL, es un cliente nuevo. si no, es un cambio de nick */
int NSCmdPreNick(Cliente *cl, char *nuevo)
{
	if (nuevo)
	{
		if (IsReg(cl->nombre))
		{
			if (IsId(cl))
				SQLInserta(NS_SQL, cl->nombre, "last", "%i", time(0));
			else
				BorraKillUser(cl, 1);
		}
	}
	return 0;
}
int NSCmdPostNick(Cliente *cl, int nuevo)
{	
	if (IsForbid(cl->nombre))
	{
		Responde(cl, CLI(nickserv), "Este nick está prohibido: %s", SQLCogeRegistro(NS_SQL, cl->nombre, "motivo"));
		NSCambiaInv(cl);
		return 0;
	}
	if (IsReg(cl->nombre))
	{		
		if (!IsId(cl) && IsActivo(cl->nombre))
		{
			char *kill;
			if (nickserv->opts & NS_SID)
				ircsprintf(buf, "%s@%s", nickserv->hmod->nick, me.nombre);
			else
				ircsprintf(buf, "%s", nickserv->hmod->nick);
			Responde(cl, CLI(nickserv), "Este nick está protegido. Si es tu nick escribe \00312/msg %s IDENTIFY tupass\003. Si no lo es, escoge otro.", buf);
			kill = SQLCogeRegistro(NS_SQL, cl->nombre, "killtime");
			if (kill && atoi(kill))
			{
				Responde(cl, CLI(nickserv), "De no hacerlo, serás desconectado en \00312%s\003 segundos.", kill);
				InsertaKillUser(cl, atoi(kill));
			}
		}
		else {	
			LlamaSenyal(NS_SIGN_IDOK, 1, cl);
		}
	}
	else //Eliminamos el nivel si no esta registrado
		cl->nivel = 0;
		
	return 0;
}
int NSSigSQL()
{
	SQLNuevaTabla(NS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
		"n SERIAL, "
		"item varchar(255), "
		"estado enum('A','S','F') default 'A', " 
		"pass varchar(255), "
		"email varchar(255), "
		"url varchar(255), "
		"gecos text, "
		"host varchar(255), "
		"opts int2 default '0', "
		"id int4 default '0', "
		"reg int4 default '0', "
		"last int4 default '0', "
		"quit text, "
		"motivo text, "
		"killtime int2 default '0', "
		"swhois text, "
		"marcas text, "
		"ipvirtual varchar(255), "
		"ipcaduca int4 default '0', "
		"KEY item (item) "
		");", PREFIJO, NS_SQL);
	/*else
	{
		if (!SQLEsCampo(NS_SQL, "killtime"))
		{
			SQLRes res;
			SQLRow row;
			SQLQuery("ALTER TABLE %s%s ADD COLUMN killtime int2 DEFAULT '0'", PREFIJO, NS_SQL);
			res = SQLQuery("SELECT * FROM %s%s", PREFIJO, NS_SQL);
			while ((row = SQLFetchRow(res)))
				SQLQuery("UPDATE %s%s SET killtime=%s WHERE item='%s'", PREFIJO, NS_SQL, row[13], row[1]);
		}
		if (!SQLEsCampo(NS_SQL, "marcas"))
			SQLQuery("ALTER TABLE %s%s ADD COLUMN marcas text", PREFIJO, NS_SQL);
		SQLQuery("ALTER TABLE %s%s CHANGE item item VARCHAR( 255 )", PREFIJO, NS_SQL);
	}
	//SQLQuery("ALTER TABLE %s%s ADD PRIMARY KEY(n)", PREFIJO, NS_SQL);
	SQLQuery("ALTER TABLE %s%s DROP INDEX item", PREFIJO, NS_SQL);
	SQLQuery("ALTER TABLE %s%s ADD INDEX ( item ) ", PREFIJO, NS_SQL);*/
	/*SQLNuevaTabla(NS_FORBIDS, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"motivo varchar(255) default NULL, "
  		"KEY item (item) "
		");", PREFIJO, NS_FORBIDS);*/
	umodreg = BuscaModoProtocolo(UMODE_REGNICK, protocolo->umodos);
	return 0;
}
int NSSigQuit(Cliente *cl, char *msg)
{
	if (IsReg(cl->nombre))
	{
		if (IsId(cl))
		{
			SQLInserta(NS_SQL, cl->nombre, "quit", msg);
			SQLInserta(NS_SQL, cl->nombre, "last", "%i", time(0));
		}
		else
			BorraKillUser(cl, 1);
	}
	return 0;
}
int NSSigIdOk(Cliente *cl)
{
	if (IsReg(cl->nombre))
	{
		char *swhois;
		SQLInserta(NS_SQL, cl->nombre, "id", "%i", time(0));
		SQLInserta(NS_SQL, cl->nombre, "host", "%s@%s", cl->ident, cl->host);
		SQLInserta(NS_SQL, cl->nombre, "gecos", cl->info);
		BorraKillUser(cl, 1);
		if ((swhois = SQLCogeRegistro(NS_SQL, cl->nombre, "swhois")))
			ProtFunc(P_WHOIS_ESPECIAL)(cl, swhois);
		cl->nivel |= N1;
	}
	return 0;
}
int NSKillea(char *quien)
{
	Cliente *al;
	if ((al = BuscaCliente(quien)))
	{
		if (!IsId(al))
		{
			if (nickserv->opts & NS_PROT_KILL)
				ProtFunc(P_QUIT_USUARIO_REMOTO)(al, CLI(nickserv), "Protección de NICK.");
			else
				NSCambiaInv(al);
		}
		BorraKillUser(al, 0);
	}
	//Free(quien); //se libera en borrakilluser
	return 0;
}
int NSDropanicks(Proc *proc)
{
	SQLRes res;
	SQLRow row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = SQLQuery("SELECT item,reg,id from %s%s LIMIT 30 OFFSET %i ", PREFIJO, NS_SQL, proc->proc)) || !SQLNumRows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			if (atoi(row[2]) == 0) /* primer registro */
			{
				if (atoi(row[1]) + 86400 < time(0)) /* 24 horas */
					NSBaja(row[0], 0);
			}
			else if (nickserv->autodrop && (atoi(row[2]) + 86400 * nickserv->autodrop < time(0)))
				NSBaja(row[0], 0);
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
int NickOpts(Cliente *cl, char *nick, char **param, int params, Funcion *fc)
{
	int dif = strcasecmp(nick, cl->nombre);
	Cliente *al;
	if (!strcasecmp(param[1], "EMAIL"))
	{
		char *dominio;
		if (!(dominio = strchr(param[2], '@')) || !Mx(dominio+1))
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No parece ser una cuenta de email válida.");
			return 1;
		}
		SQLInserta(NS_SQL, nick, "email", param[2]);
		ircsprintf(buf, "Nuevo email: %s", param[2]);
		NSMarca(cl, nick, buf);
		if (dif)
			Responde(cl, CLI(nickserv), "El email de %s se ha cambiado a \00312%s\003.", nick, param[2]);
		else
			Responde(cl, CLI(nickserv), "Tu email se ha cambiado a \00312%s\003.", param[2]);
	}
	else if (!strcasecmp(param[1], "URL"))
	{
		SQLInserta(NS_SQL, nick, "url", param[2]);
		if (dif)
			Responde(cl, CLI(nickserv), "La url de %s se ha cambiado a \00312%s\003.", nick, param[2]);
		else
			Responde(cl, CLI(nickserv), "Tu url se ha cambiado a \00312%s\003.", param[2]);
	}
	else if (!strcasecmp(param[1], "KILL"))
	{
		if (atoi(param[2]) < 10 && strcasecmp(param[2], "OFF"))
		{
			Responde(cl, CLI(nickserv), NS_ERR_SNTX, "El valor kill debe ser un número mayor que 10.");
			return 1;
		}
		SQLInserta(NS_SQL, nick, "killtime", param[2]);
		if (dif)
			Responde(cl, CLI(nickserv), "El kill de %s se ha cambiado a \00312%s\003.", nick, param[2]);
		else
			Responde(cl, CLI(nickserv), "Tu kill se ha cambiado a \00312%s\003.", param[2]);
	}
	else if (!strcasecmp(param[1], "NODROP"))
	{
		if (IsOper(cl))
		{
			int opts = atoi(SQLCogeRegistro(NS_SQL, nick, "opts"));
			if (!strcasecmp(param[2], "ON"))
				opts |= NS_OPT_NODROP;
			else if (!strcasecmp(param[2], "OFF"))
				opts &= ~NS_OPT_NODROP;
			else
			{
				Responde(cl, CLI(nickserv), NS_ERR_SNTX, "ON|OFF");
				return 1;
			}
			SQLInserta(NS_SQL, nick, "opts", "%i", opts);
			if (dif)
				Responde(cl, CLI(nickserv), "El DROP de %s se ha cambiado a \00312%s\003.", nick, param[2]);
			else
				Responde(cl, CLI(nickserv), "El DROP de tu nick se ha cambiado a \00312%s\003.", param[2]);
		}
		else
		{
			Responde(cl, CLI(nickserv), NS_ERR_FORB);
			return 1;
		}
	}
	else if (!strcasecmp(param[1], "PASS"))
	{
		char *passmd5 = MDString(param[2], 0);
		SQLInserta(NS_SQL, nick, "pass", passmd5);
		NSMarca(cl, nick, "Contraseña cambiada.");
		if (nick)
			Responde(cl, CLI(nickserv), "La contraseña de %s se ha cambiado con éxito.", nick);
		else
			Responde(cl, CLI(nickserv), "Tu contraseña se ha cambiado con éxito.");
	}
	else if (!strcasecmp(param[1], "HIDE"))
	{
		int opts = atoi(SQLCogeRegistro(NS_SQL, nick, "opts"));
		if (params < 4)
		{
			Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "HIDE valor on|off");
			return 1;
		}
		if (strcasecmp(param[3], "ON") && strcasecmp(param[3], "OFF"))
		{
			Responde(cl, CLI(nickserv), NS_ERR_SNTX, "HIDE valor on|off");
			return 1;
		}
		if (!strcasecmp(param[2], "EMAIL"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_MAIL;
			else
				opts &= ~NS_OPT_MAIL;
		}
		else if (!strcasecmp(param[2], "URL"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_WEB;
			else
				opts &= ~NS_OPT_WEB;
		}
		else if (!strcasecmp(param[2], "MASK"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_MASK;
			else
				opts &= ~NS_OPT_MASK;
		}
		else if (!strcasecmp(param[2], "TIME"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_TIME;
			else
				opts &= ~NS_OPT_TIME;
		}
		else if (!strcasecmp(param[2], "QUIT"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_QUIT;
			else
				opts &= ~NS_OPT_QUIT;
		}
		else if (!strcasecmp(param[2], "LIST"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_LIST;
			else
				opts &= ~NS_OPT_LIST;
		}
		else if (!strcasecmp(param[2], "LOC"))
		{
			if (!strcasecmp(param[3], "ON"))
				opts |= NS_OPT_LOC;
			else
				opts &= ~NS_OPT_LOC;
		}
		else

		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opción incorrecta.");
			return 1;
		}
		SQLInserta(NS_SQL, nick, "opts", "%i", opts);
		Responde(cl, CLI(nickserv), "Hide %s cambiado a \00312%s\003.", param[2], param[3]);
	}
	else
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opción incorrecta.");
		return 1;
	}
	if ((al = BuscaCliente(nick)) != cl)
		Responde(al, CLI(nickserv), "%s ha ejecutado \"\00312%s\003\" sobre ti.", cl->nombre, strtoupper(Unifica(param, params, 0, -1)));
	return 0;
}
int NSSigSynch()
{
	IniciaProceso(NSDropanicks);
	return 0;
}
int NSSigSockClose()
{
	KillUser *ku, *sig;
	for (ku = killusers; ku; ku = sig)
	{
		sig = ku->sig;
		Free(ku->timer->args);
		ApagaCrono(ku->timer);
		Free(ku);
	}
	killusers = NULL;
	DetieneProceso(NSDropanicks);
	return 0;
}
