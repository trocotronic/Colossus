/*
 * $Id: nickserv.c,v 1.34 2006-02-18 14:34:32 Trocotronic Exp $ 
 */

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
BOTFUNC(NSMarcas);
BOTFUNCHELP(NSHMarcas);

int NSSigSQL		();
int NSCmdPreNick 	(Cliente *, char *);
int NSCmdPostNick 	(Cliente *, int);
int NSCmdUmode		(Cliente *, char *);
int NSSigQuit 		(Cliente *, char *);
int NSSigIdOk 		(Cliente *);
int NSSigStartUp 	();
int NSKillea		(char *);

int NSDropanicks	(Proc *);

int NSBaja(char *, char);
DLLFUNC void NSCambiaInv(Cliente *);

void NSSet(Conf *, Modulo *);
int NSTest(Conf *, int *);
extern MODVAR mTab cFlags[];
mTab *modreg;

static bCom nickserv_coms[] = {
	{ "help" , NSHelp , N0 , "Muestra esta ayuda." , NULL } ,
	{ "register" , NSRegister , N0 , "Registra el nick que lleves puesto." , NSHRegister } ,
	{ "identify" , NSIdentify , N0 , "Te identifica como propietario del nick." , NSHIdentify } ,
	{ "set" , NSOpts , N1 , "Permite fijar distintas opciones para tu nick." , NSHSet } ,
	{ "drop" , NSDrop , N2 , "Desregistra un usuario." , NSHDrop } , 
	{ "sendpass" , NSSendpass , N2 , "Genera un password para el usuario y se lo env�a a su correo." , NSHSendpass } ,
	{ "info" , NSInfo , N0 , "Muestra informaci�n de un nick." , NSHInfo } ,
	{ "list" , NSList, N0 , "Lista todos los nicks que coinciden con un patr�n." , NSHList } ,
	{ "ghost" , NSGhost , N0 , "Desconecta a un usuario que utilice su nick." , NSHGhost } ,
	{ "suspender" , NSSuspender , N3 , "Suspende los privilegios de un nick." , NSHSuspender } ,
	{ "liberar" , NSLiberar , N3 , "Libera un nick de su suspenso." , NSHLiberar } ,
	{ "swhois" , NSSwhois , N2 , "A�ade o borra un whois especial al nick especificado." , NSHSwhois } ,
	{ "rename" , NSRename , N3 , "Cambia el nick a un usuario." , NSHRename } ,
	{ "forbid" , NSForbid , N4 , "Prohibe o permite un determinado nick." , NSHForbid } ,
	{ "marca" , NSMarcas , N3 , "Lista o inserta una entrada en el historial de un nick." , NSHMarcas } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};
ModInfo MOD_INFO(NickServ) = {
	"NickServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int MOD_CARGA(NickServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(NickServ).nombre);
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
					Error("[%s] La configuraci�n de %s no ha pasado el NSTest", mod->archivo, MOD_INFO(NickServ).nombre);
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
	BorraSenyal(SIGN_PRE_NICK, NSCmdPreNick);
	BorraSenyal(SIGN_POST_NICK, NSCmdPostNick);
	BorraSenyal(SIGN_UMODE, NSCmdUmode);
	BorraSenyal(SIGN_SQL, NSSigSQL);
	BorraSenyal(SIGN_QUIT, NSSigQuit);
	BorraSenyal(NS_SIGN_IDOK, NSSigIdOk);
	BorraSenyal(SIGN_STARTUP, NSSigStartUp);
	DetieneProceso(NSDropanicks);
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
		BMalloc(nickserv, NickServ);
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
				for (p = 0; p < config->seccion[i]->secciones; p++, nickserv->forbmails++)
					ircstrdup(nickserv->forbmail[nickserv->forbmails], config->seccion[i]->seccion[p]->item);
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
	InsertaSenyal(SIGN_STARTUP, NSSigStartUp);
	IniciaProceso(NSDropanicks);
	BotSet(nickserv);
}
/* establece una nueva clave para el nick en cuesti�n */
char *NSRegeneraClave(char *nick)
{
	char *pass, *passmd5;
	pass = AleatorioEx(nickserv->securepass);
	passmd5 = MDString(pass);
	SQLInserta(NS_SQL, nick, "pass", passmd5);
	return pass;
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
	time_t fecha = time(NULL);
	char *marcas;
	if ((marcas = SQLCogeRegistro(NS_SQL, nombre, "marcas")))
		SQLInserta(NS_SQL, nombre, "marcas", "%s\t\00312%s\003 - \00312%s\003 - %s", marcas, Fecha(&fecha), cl->nombre, marca);
	else
		SQLInserta(NS_SQL, nombre, "marcas", "\00312%s\003 - \00312%s\003 - %s", Fecha(&fecha), cl->nombre, marca);
}
BOTFUNCHELP(NSHIdentify)
{
	Responde(cl, CLI(nickserv), "Te identifica como propietario del nick.");
	Responde(cl, CLI(nickserv), "Para ello debes proporcionar una contrase�a de autentificaci�n, la misma que utilizaste en el registro de tu nick.");
	Responde(cl, CLI(nickserv), "Una vez identificado como propietario podr�s disponer de cuantiosas oportunidades que te ofrece el reigstro de nicks.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312IDENTIFY tupass");
	return 0;
}
BOTFUNCHELP(NSHInfo)
{
	Responde(cl, CLI(nickserv), "Muestra distinta informaci�n de un nick, como su fecha de registro o su �ltimo quit, entre otros.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312INFO nick");
	return 0;
}
BOTFUNCHELP(NSHList)
{
	Responde(cl, CLI(nickserv), "Lista todos los nicks registrados que coincidan con un patr�n especificado.");
	Responde(cl, CLI(nickserv), "Para evitar abusos de flood, este comando emite como m�ximo \00312%i\003 entradas.", nickserv->maxlist);
	Responde(cl, CLI(nickserv), "El patr�n puede ser exacto, o puede contener comodines (*) para cerrar el campo de b�squeda.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312LIST patron");
	return 0;
}
BOTFUNCHELP(NSHGhost)
{
	Responde(cl, CLI(nickserv), "Este comando desconecta a un usuario que lleve puesto tu nick.");
	Responde(cl, CLI(nickserv), "Para ello deber�s especificar la contrase�a de propietario del nick.");
	Responde(cl, CLI(nickserv), "Si la contrase�a es v�lida, %s.", nickserv->opts & NS_PROT_KILL ? "se desconectar� el usuario" : "se le cambiar� el nick al usuario");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312GHOST nick contrase�a");
	return 0;
}
BOTFUNCHELP(NSHSet)
{
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), "Permite fijar diferentes opciones para tu nick.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Opciones disponibles:");
		Responde(cl, CLI(nickserv), "\00312PASS\003 Cambia tu contrase�a de usuario.");
		Responde(cl, CLI(nickserv), "\00312EMAIL\003 Cambia tu direcci�n de correo.");
		Responde(cl, CLI(nickserv), "\00312URL\003 Cambia tu p�gina web.");
		Responde(cl, CLI(nickserv), "\00312KILL\003 Activa/Desactiva tu protecci�n de KILL.");
		if (IsOper(cl))
			Responde(cl, CLI(nickserv), "\00312NODROP\003 Activa/Desactiva tu protecci�n de DROP.");
		Responde(cl, CLI(nickserv), "\00312HIDE\003 Opciones para mostrar en tu INFO.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Para m�s informaci�n, \00312/msg %s %s SET opci�n", nickserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		Responde(cl, CLI(nickserv), "\00312SET PASS");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu contrase�a de usuario.");
		Responde(cl, CLI(nickserv), "A partir de este cambio, �sta ser� la contrase�a con la que debes identificarte.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET PASS nuevopass");
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET EMAIL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu direcci�n de correo.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET email direccion@correo");
	}
	else if (!strcasecmp(param[2], "URL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET URL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Cambia tu p�gina web personal.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET url http://pagina.web");
	}
	else if (!strcasecmp(param[2], "KILL"))
	{
		Responde(cl, CLI(nickserv), "\00312SET KILL");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Especifica tu protecci�n KILL.");
		Responde(cl, CLI(nickserv), "Con esta protecci�n, si no te identificas antes de los segundos que especifiques %s.", nickserv->opts & NS_PROT_KILL ? "ser�s desconectado" : "se te cambiar� el nick");
		Responde(cl, CLI(nickserv), "Si especificas el 0 como valor, esta protecci�n se desactiva.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312KILL segundos");
	}
	else if (!strcasecmp(param[2], "NODROP") && IsOper(cl))
	{
		Responde(cl, CLI(nickserv), "\00312SET NODROP");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Evita que tu nick pueda ser dropeado, a excepci�n de los Administradores.");
		Responde(cl, CLI(nickserv), "Si est� en ON, tu nick no podr� dropearse.");
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
		Responde(cl, CLI(nickserv), "\00312URL\003 Muestra tu p�gina personal.");
		Responde(cl, CLI(nickserv), "\00312MASK\003 Muestra tu user@host.");
		Responde(cl, CLI(nickserv), "\00312TIME\003 Muestra la �ltima hora que fuiste visto en la red.");
		Responde(cl, CLI(nickserv), "\00312QUIT\003 Muestra tu �ltimo quit.");
		Responde(cl, CLI(nickserv), "\00312LIST\003 Se te mostrar� cuando alguien haga un LIST.");
		Responde(cl, CLI(nickserv), " ");
		Responde(cl, CLI(nickserv), "Sintaxis: \00312SET HIDE opcion on|off");
	}
	else
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opci�n desconocida.");
	return 0;
}
BOTFUNCHELP(NSHRegister)
{
	Responde(cl, CLI(nickserv), "Permite registrar tu nick en la red para que nadie sin autorizaci�n pueda utilizarlo.");
	Responde(cl, CLI(nickserv), "Para utilizar tu nick registrado deber�s identificarte como propietario mediante el comando \00312IDENTIFY\003.");
	Responde(cl, CLI(nickserv), "El registro del nick requiere una cuenta de email v�lida. Recuerda que si no lo es, no podr�s recibir tu nueva contrase�a en caso de que la perdieras.");
	if (nickserv->opts & NS_SMAIL)
		Responde(cl, CLI(nickserv), "Una vez hayas registrado tu nick, te llegar� en tu correo email una clave temporal, que ser� la que deber�s usar para identificarte.");
	Responde(cl, CLI(nickserv), " ");
	if (nickserv->opts & NS_SMAIL)
		Responde(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tu@email");
	else
		Responde(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tupass tu@email");
	return 0;
}
BOTFUNCHELP(NSHSwhois)
{
	Responde(cl, CLI(nickserv), "A�ade o borra un whois especial a un usuario.");
	Responde(cl, CLI(nickserv), "Este whois se le a�adir� cuando se identifique.");
	Responde(cl, CLI(nickserv), "Se mostrar� con el comando WHOIS.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SWHOIS nick [swhois]");
	Responde(cl, CLI(nickserv), "Si no se especifica swhois, se borrar� el que pudiera haber.");
	Responde(cl, CLI(nickserv), "NOTA: Es posible que la red no soporte este comando (s�lo unreal).");
	return 0;
}
BOTFUNCHELP(NSHSuspender)
{
	Responde(cl, CLI(nickserv), "Suspende el uso de un determinado nick.");
	Responde(cl, CLI(nickserv), "Este nick podr� ponerselo pero no tendr� privilegios algunos.");
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
	Responde(cl, CLI(nickserv), "Prohibe o permite el uso de un nick o apodo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312FORBID nick [motivo]");
	Responde(cl, CLI(nickserv), "Si no se especifica swhois, se borrar� el que pudiera haber.");
	return 0;
}
BOTFUNCHELP(NSHDrop)
{
	Responde(cl, CLI(nickserv), "Borra un nick de la base de datos.");
	Responde(cl, CLI(nickserv), "Este desregistro anula completamente la propiedad de un usuario sobre un nick.");
	Responde(cl, CLI(nickserv), "Una vez borrado, no se puede volver a recuperar si no es registr�ndolo de nuevo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312DROP nick");
	return 0;
}
BOTFUNCHELP(NSHSendpass)
{
	Responde(cl, CLI(nickserv), "Regenera una contrase�a de usuario y se la env�a a su correo.");
	Responde(cl, CLI(nickserv), "Este comando s�lo debe en caso de p�rdida de la contrase�a del usuario y a petici�n del mismo.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312SENDPASS nick");
	return 0;
}
BOTFUNCHELP(NSHMarcas)
{
	Responde(cl, CLI(nickserv), "Inserta una marca en el historial de un nick.");
	Responde(cl, CLI(nickserv), "Las marcas son anotaciones que no pueden eliminarse.");
	Responde(cl, CLI(nickserv), "En ellas figura la fecha, su autor y un comentario.");
	Responde(cl, CLI(nickserv), "Si no se especifica ninguna marca, se listar�n todas las que hubiera.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312MARCA [nick]");
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
		Responde(cl, CLI(nickserv), "Para m�s informaci�n, \00312/msg %s %s comando", nickserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], nickserv->hmod, param, params))
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opci�n desconocida.");
	EOI(nickserv, 0);
	return 0;
}
BOTFUNC(NSRegister)
{
	u_int i, opts;
	u_long ultimo;
	char *dominio;
	char *mail, *pass, *usermask, *cache;
	SQLRes res = NULL;
	usermask = strchr(cl->mask, '!') + 1;
	if (strlen(cl->nombre) > (u_int)(conf_set->nicklen - 7)) /* 6 minimo de contrase�a +1 de los ':' */
	{
		ircsprintf(buf, "Tu nick es demasiado largo. S�lo puede tener %i caracteres como m�ximo.", (conf_set->nicklen - 7));
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if (params < ((nickserv->opts & NS_SMAIL) ? 2 : 3))
	{
		if (nickserv->opts & NS_SMAIL)
			Responde(cl, CLI(nickserv), NS_ERR_PARA, "REGISTER tu@email");
		else
			Responde(cl, CLI(nickserv), NS_ERR_PARA, "REGISTER tupass tu@email");
		return 1;
	}
	if ((cache = CogeCache(CACHE_ULTIMO_REG, usermask, nickserv->hmod->id)))
	{
		ultimo = atoul(cache);
		if ((ultimo + (3600 * nickserv->min_reg)) > (u_long)time(0))
		{
			char buf[512];
			ircsprintf(buf, "No puedes efectuar otro registro hasta que no pasen %i horas.", nickserv->min_reg);
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
			return 1;
		}
		else
			BorraCache(CACHE_ULTIMO_REG, usermask, nickserv->hmod->id);
	}
	/* comprobamos que no est� registrado */
	if (IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick ya est� registrado.");
		return 1;
	}
	if (nickserv->opts & NS_SMAIL)
	{
		mail = param[1];
		pass = NULL;
	}
	else
	{
		mail = param[2];
		pass = param[1];
	}
	/* comprobamos su email */
	dominio = strchr(mail, '@');
	if (!dominio || !gethostbyname(dominio+1))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No parece ser una cuenta de email v�lida.");
		return 1;
	}
	for (i = 0; i < nickserv->forbmails; i++)
	{
		if (!strcasecmp(nickserv->forbmail[i], dominio))
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes utilizar una cuenta de correo gratu�ta.");
			return 1;
		}
	}
	if (nickserv->nicks && ((res = SQLQuery("SELECT * from %s%s where LOWER(email)='%s'", PREFIJO, NS_SQL, strtolower(mail))) && SQLNumRows(res) == nickserv->nicks))
	{
		SQLFreeRes(res);
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes registrar m�s nicks en esta cuenta.");
		return 1;
	}
	SQLFreeRes(res);
	opts = NS_OPT_MASK;
	if (pass)
		SQLInserta(NS_SQL, cl->nombre, "pass", MDString(pass));
	SQLInserta(NS_SQL, cl->nombre, "email", mail);
	SQLInserta(NS_SQL, cl->nombre, "gecos", cl->info);
	SQLInserta(NS_SQL, cl->nombre, "host", "%s@%s", cl->ident, cl->host);
	SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts);
	SQLInserta(NS_SQL, cl->nombre, "id", "%i", nickserv->opts & NS_SMAIL ? 0 : time(0));
	SQLInserta(NS_SQL, cl->nombre, "reg", "%i", time(0));
	SQLInserta(NS_SQL, cl->nombre, "last", "%i", time(0));
	if (nickserv->opts & NS_SMAIL)
		Email(mail, "Nueva contrase�a", "Debido al registro de tu nick, se ha generado una contrase�a totalmente segura.\r\n"
		"A partir de ahora, la clave de tu nick es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", NSRegeneraClave(cl->nombre), nickserv->hmod->nick, conf_set->red);
	else if (modreg)
		ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", modreg->flag);
	if (!IsOper(cl))
		InsertaCache(CACHE_ULTIMO_REG, usermask, 3600 * nickserv->min_reg, nickserv->hmod->id, "%lu", time(0));
	Responde(cl, CLI(nickserv), "Tu nick ha sido registrado bajo la cuenta \00312%s\003.", mail);
	Senyal1(NS_SIGN_REG, cl->nombre);
	EOI(nickserv, 1);
	return 0;
}
BOTFUNC(NSIdentify)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "IDENTIFY tupass");
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
		ircsprintf(buf, "Identificaci�n incorrecta. /msg %s@%s IDENTIFY pass", nickserv->hmod->nick, me.nombre);
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if (!strcmp(MDString(param[1]), SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
	{
		if (modreg)
			ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", modreg->flag);
		Responde(cl, CLI(nickserv), "Ok \00312%s\003, bienvenid@ a casa :)", cl->nombre);
		Senyal1(NS_SIGN_IDOK, cl);
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
			ProtFunc(P_QUIT_USUARIO_REMOTO)(cl, CLI(nickserv), "Demasiadas contrase�as inv�lidas.");
		}
		else
		{
			ircsprintf(buf, "Contrase�a incorrecta. Tienes %i intentos m�s.", nickserv->intentos - intentos);
			Responde(cl, CLI(nickserv), buf);
			InsertaCache(CACHE_INTENTOS_ID, cl->nombre, 86400, nickserv->hmod->id, "%i", intentos);
		}
	}
	EOI(nickserv, 2);
	return 0;
}
BOTFUNC(NSOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "SET opci�n valor");
		return 1;
	}
	if (!strcasecmp(param[1], "EMAIL"))
	{
		SQLInserta(NS_SQL, cl->nombre, "email", param[2]);
		ircsprintf(buf, "Nuevo email: %s", param[2]);
		NSMarca(cl, param[1], buf);
		Responde(cl, CLI(nickserv), "Tu email se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "URL"))
	{
		SQLInserta(NS_SQL, cl->nombre, "url", param[2]);
		Responde(cl, CLI(nickserv), "Tu url se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "KILL"))
	{
		if (atoi(param[2]) < 10 && strcasecmp(param[2], "OFF"))
		{
			Responde(cl, CLI(nickserv), NS_ERR_SNTX, "El valor kill debe ser un n�mero mayor que 10.");
			return 1;
		}
		SQLInserta(NS_SQL, cl->nombre, "killtime", param[2]);
		Responde(cl, CLI(nickserv), "Tu kill se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "NODROP"))
	{
		if (IsOper(cl))
		{
			int opts = atoi(SQLCogeRegistro(NS_SQL, cl->nombre, "opts"));
			if (!strcasecmp(param[2], "ON"))
				opts |= NS_OPT_NODROP;
			else
				opts &= ~NS_OPT_NODROP;
			SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts);
			Responde(cl, CLI(nickserv), "El DROP de tu nick se ha cambiado a \00312%s\003.", param[2]);
		}
		else
		{
			Responde(cl, CLI(nickserv), NS_ERR_FORB);
			return 1;
		}
		return 0;
	}
	else if (!strcasecmp(param[1], "PASS"))
	{
		char *passmd5 = MDString(param[2]);
		SQLInserta(NS_SQL, cl->nombre, "pass", passmd5);
		NSMarca(cl, param[1], "Contrase�a cambiada.");
		Responde(cl, CLI(nickserv), "Tu contrase�a se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "HIDE"))
	{
		int opts = atoi(SQLCogeRegistro(NS_SQL, cl->nombre, "opts"));
		if (params < 4)
		{
			Responde(cl, CLI(nickserv), NS_ERR_PARA, "SET HIDE valor on|off");
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
		else
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opci�n incorrecta.");
			return 1;
		}
		SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts);
		Responde(cl, CLI(nickserv), "Hide %s cambiado a \00312%s\003.", param[2], param[3]);
		return 0;
	}
	else
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Opci�n incorrecta.");
		return 1;
	}
	EOI(nickserv, 3);
	return 0;
}
BOTFUNC(NSDrop)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "DROP nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
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
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se ha podido dropear. Comun�quelo a la administraci�n.");
	return 0;
	EOI(nickserv, 4);
}
int NSBaja(char *nick, char opt)
{
	Cliente *al;
	int opts = atoi(SQLCogeRegistro(NS_SQL, nick, "opts"));
	if (!opt && (opts & NS_OPT_NODROP))
		return 1;
	al = BuscaCliente(nick, NULL);
	Senyal1(NS_SIGN_DROP, nick);
	if (al)
		ProtFunc(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "-r");
	SQLBorra(NS_SQL, nick);
	return 0;
}
BOTFUNC(NSSendpass)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "SENDPASS nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	Email(SQLCogeRegistro(NS_SQL, param[1], "email"), "Reenv�o de la contrase�a", "Debido a la p�rdida de tu contrase�a, se te ha generado otra clave totalmente segura.\r\n"
		"A partir de ahora, la clave de tu nick es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", NSRegeneraClave(param[1]), nickserv->hmod->nick, conf_set->red);
	Responde(cl, CLI(nickserv), "Se ha generado y enviado otra contrase�a al email de \00312%s\003.", param[1]);
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
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "INFO nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	comp = strcasecmp(param[1], cl->nombre);
	res = SQLQuery("SELECT opts,gecos,reg,suspend,host,quit,last,email,url,killtime,item from %s%s where LOWER(item)='%s'", PREFIJO, NS_SQL, strtolower(param[1]));
	row = SQLFetchRow(res);
	opts = atoi(row[0]);
	Responde(cl, CLI(nickserv), "Informaci�n de \00312%s", row[10]);
	Responde(cl, CLI(nickserv), "Realname: \00312%s", row[1]);
	reg = (time_t)atol(row[2]);
	Responde(cl, CLI(nickserv), "Registrado el \00312%s", Fecha(&reg));
	if (!BadPtr(row[3]))
		Responde(cl, CLI(nickserv), "Est� suspendido: \00312%s", row[3]);
	if (!(opts & NS_OPT_MASK) || (!comp && IsId(cl)) || IsOper(cl))
		Responde(cl, CLI(nickserv), "�ltimo mask: \00312%s", row[4]);
	if (!(opts & NS_OPT_TIME) || (!comp && IsId(cl)) || IsOper(cl))
	{
		Cliente *al;
		al = BuscaCliente(param[1], NULL);
		if (al && IsId(al))
			Responde(cl, CLI(nickserv), "Este usuario est� conectado. Utiliza /WHOIS %s para m�s informaci�n.", param[1]);
		else
		{
			reg = (time_t)atol(row[6]);
			Responde(cl, CLI(nickserv), "Visto por �ltima vez el \00312%s", Fecha(&reg));
		}
	}
	if (!BadPtr(row[5]) && (!(opts & NS_OPT_QUIT) || (!comp && IsId(cl)) || IsOper(cl)))
		Responde(cl, CLI(nickserv), "�ltimo quit: \00312%s", row[5]);
	if (!(opts & NS_OPT_MAIL) || (!comp && IsId(cl)) || IsOper(cl))
		Responde(cl, CLI(nickserv), "Email: \00312%s", row[7]);
	if (!BadPtr(row[8]) && (!(opts & NS_OPT_WEB) || (!comp && IsId(cl)) || IsOper(cl)))
		Responde(cl, CLI(nickserv), "Url: \00312%s", row[8]);
	if (atoi(row[9]))
		Responde(cl, CLI(nickserv), "Protecci�n kill a \00312%i\003 segundos.", atoi(row[9]));
	EOI(nickserv, 6);
	SQLFreeRes(res);
	if ((!strcasecmp(cl->nombre, param[1]) && IsId(cl)) || IsOper(cl))
	{
		CsRegistros *regs;
		int i;
		Responde(cl, CLI(nickserv), "*** Niveles de acceso ***");
		if ((regs = busca_cregistro(param[1])))
		{
			for (i = 0; i < regs->subs; i++)
			{
				tokbuf[0] = '\0';
				if (regs->sub[i].autos)
					ircsprintf(tokbuf, " \003[\00312%s\003]", regs->sub[i].autos);
				Responde(cl, CLI(nickserv), "Canal: \00312%s\003 flags: \00312%s%s%s\003 (\00312%lu\003)", regs->sub[i].canal, 
					(regs->sub[i].flags ? "+" : ""), ModosAFlags(regs->sub[i].flags, cFlags, NULL), tokbuf, regs->sub[i].flags);
			}
		}
		if ((res = SQLQuery("SELECT item from %s%s where LOWER(founder)='%s'", PREFIJO, CS_SQL, strtolower(param[1]))))
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
	return 0;
}
BOTFUNC(NSList)
{
	char *rep;
	SQLRes res;
	SQLRow row;
	u_int i;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "LIST patr�n");
		return 1;
	}
	rep = str_replace(param[1], '*', '%');
	if (!(res = SQLQuery("SELECT item from %s%s where LOWER(item) LIKE '%s'", PREFIJO, NS_SQL, strtolower(rep))))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
		return 1;
	}
	Responde(cl, CLI(nickserv), "*** Nicks que coinciden con el patr�n \00312%s\003 ***", param[1]);
	for (i = 0; i < nickserv->maxlist && (row = SQLFetchRow(res)); i++)
	{
		if (IsOper(cl) || !(atoi(SQLCogeRegistro(NS_SQL, row[0], "opts")) & NS_OPT_LIST))
			Responde(cl, CLI(nickserv), "\00312%s", row[0]);
		else
			i--;
	}
	Responde(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
	SQLFreeRes(res);
	EOI(nickserv, 7);
	return 0;
}
BOTFUNC(NSGhost)
{
	Cliente *al;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "GHOST nick pass");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este usuario no est� conectado.");
		return 1;
	}
	if (!strcasecmp(param[1], cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes ejecutar este comando sobre ti mismo.");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contrase�a incorrecta.");
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
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "SUSPENDER nick motivo");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
	SQLInserta(NS_SQL, param[1], "suspend", motivo);
	ircsprintf(buf, "Suspendido: %s", motivo);
	NSMarca(cl, param[1], buf);
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido suspendido.", param[1]);
	EOI(nickserv, 9);
	return 0;
}
BOTFUNC(NSLiberar)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "LIBERAR nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!SQLCogeRegistro(NS_SQL, param[1], "suspend"))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no est� suspendido.");
		return 1;
	}
	SQLInserta(NS_SQL, param[1], "suspend", "");
	NSMarca(cl, param[1], "Suspenso levantado.");
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido liberado de su suspenso.", param[1]);			
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
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "SWHOIS nick [swhois]");
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
		if ((al = (BuscaCliente(param[1], NULL))))
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
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "RENAME nick nuevonick");
		return 1;
	}
	if ((al = BuscaCliente(param[2], NULL)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "El nuevo nick ya est� en uso.");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no est� conectado.");
		return 1;
	}
	ProtFunc(P_CAMBIO_USUARIO_REMOTO)(al, param[2]);
	Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha cambiado su nick a \00312%s\003.", param[1], param[2]);
	EOI(nickserv, 12);
	return 0;
}
BOTFUNC(NSForbid)
{
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "FORBID nick [motivo]");
		return 1;
	}
	if (params >= 3)
	{
		char *motivo;
		motivo = Unifica(param, params, 1, -1);
		if (!ProtFunc(P_FORB_NICK))
		{
			Responde(cl, CLI(nickserv), ERR_NSUP);
			return 1;
		}
		SQLInserta(NS_FORBIDS, param[1], "motivo", motivo);
		ProtFunc(P_FORB_NICK)(param[1], ADD,  motivo);
		ircsprintf(buf, "Prohibido: %s", motivo);
		NSMarca(cl, param[1], buf);
		Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido prohibido.", param[1]);
	}
	else
	{
		SQLBorra(NS_FORBIDS, param[1]);
		if (!ProtFunc(P_FORB_NICK))
			ProtFunc(P_FORB_NICK)(param[1], DEL, NULL);
		NSMarca(cl, param[1], "Prohibici�n levantada.");
		Responde(cl, CLI(nickserv), "El nick \00312%s\003 ha sido permitido.", param[1]);
	}
	EOI(nickserv, 13);
	return 0;
}
BOTFUNC(NSMarcas)
{
	char *marcas;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "MARCA nick [comentario]");
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
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "El historial est� vac�o.");
			return 1;
		}
	}
	else
	{
		NSMarca(cl, param[1], Unifica(param, params, 2, -1));
		Responde(cl, CLI(nickserv), "Marca a�adida al historial de \00312%s", param[1]);
	}
	EOI(nickserv, 14);
	return 0;
}		
int NSCmdUmode(Cliente *cl, char *modos)
{
	//if (conf_set->modos && conf_set->modos->usuarios)
	//	ProtFunc(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), conf_set->modos->usuarios);
	if (modreg && strchr(modos, modreg->flag) && (cl->modos & UMODE_REGNICK))
		Senyal1(NS_SIGN_IDOK, cl);
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
				ApagaCrono(cl->nombre, cl->sck); /* es posible que tenga kill */
		}
	}
	return 0;
}
int NSCmdPostNick(Cliente *cl, int nuevo)
{
	char *motivo = NULL;
	if ((motivo = SQLCogeRegistro(NS_FORBIDS, cl->nombre, "motivo")))
	{
		Responde(cl, CLI(nickserv), "Este nick est� prohibido: %s", motivo);
		NSCambiaInv(cl);
		return 0;
	}
	if (IsReg(cl->nombre))
	{
		if (!IsId(cl))
		{
			char *kill;
			if (nickserv->opts & NS_SID)
				ircsprintf(buf, "%s@%s", nickserv->hmod->nick, me.nombre);
			else
				ircsprintf(buf, "%s", nickserv->hmod->nick);
			Responde(cl, CLI(nickserv), "Este nick est� protegido. Si es tu nick escribe \00312/msg %s IDENTIFY tupass\003. Si no lo es, escoge otro.", buf);
			kill = SQLCogeRegistro(NS_SQL, cl->nombre, "killtime");
			if (kill && atoi(kill))
			{
				Responde(cl, CLI(nickserv), "De no hacerlo, ser�s desconectado en \00312%s\003 segundos.", kill);
				IniciaCrono(cl->nombre, cl->sck, 1, atoi(kill), NSKillea, cl->nombre, sizeof(char) * (strlen(cl->nombre) + 1));
			}
		}
		else
			Senyal1(NS_SIGN_IDOK, cl);
	}		
	return 0;
}
int NSSigSQL()
{
	if (!SQLEsTabla(NS_SQL))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"n SERIAL, "
  			"item varchar(255), "
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
  			"suspend text, "
  			"killtime int2 default '0', "
  			"swhois text, "
  			"marcas text "
			");", PREFIJO, NS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, NS_SQL);
	}
	else
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
		SQLQuery("ALTER TABLE `%s%s` CHANGE `item` `item` VARCHAR( 255 )", PREFIJO, NS_SQL);
	}	
	SQLQuery("ALTER TABLE `%s%s` ADD PRIMARY KEY(`n`)", PREFIJO, NS_SQL);
	SQLQuery("ALTER TABLE `%s%s` DROP INDEX `item`", PREFIJO, NS_SQL);
	SQLQuery("ALTER TABLE `%s%s` ADD INDEX ( `item` ) ", PREFIJO, NS_SQL);
	if (!SQLEsTabla(NS_FORBIDS))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"motivo varchar(255) default NULL "
			");", PREFIJO, NS_FORBIDS))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, NS_FORBIDS);
	}
	SQLCargaTablas();
	return 1;
}
int NSSigQuit(Cliente *cl, char *msg)
{
	if (IsId(cl) && IsReg(cl->nombre))
	{
		SQLInserta(NS_SQL, cl->nombre, "quit", msg);
		SQLInserta(NS_SQL, cl->nombre, "last", "%i", time(0));
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
		ApagaCrono(cl->nombre, cl->sck);
		if ((swhois = SQLCogeRegistro(NS_SQL, cl->nombre, "swhois")))
			ProtFunc(P_WHOIS_ESPECIAL)(cl, swhois);
		cl->nivel |= N1;
	}
	return 0;
}
int NSKillea(char *quien)
{
	Cliente *al;
	if ((al = BuscaCliente(quien, NULL)) && !IsId(al))
	{
		if (nickserv->opts & NS_PROT_KILL)
			ProtFunc(P_QUIT_USUARIO_REMOTO)(al, CLI(nickserv), "Protecci�n de NICK.");
		else
			NSCambiaInv(al);
	}
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
			else if (nickserv->autodrop && (atoi(row[2]) + 86400 * nickserv->autodrop < (u_long)time(0)))
				NSBaja(row[0], 0);
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
int NSSigStartUp()
{
	modreg = BuscaModoProtocolo(UMODE_REGNICK, protocolo->umodos);
	return 0;
}
