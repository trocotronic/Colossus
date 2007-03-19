/*
 * $Id: chanserv.c,v 1.50 2007-03-19 19:16:36 Trocotronic Exp $ 
 */

#ifndef _WIN32
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"

ChanServ *chanserv = NULL;

BOTFUNC(CSHelp);
BOTFUNC(CSDeauth);
BOTFUNCHELP(CSHDeauth);
BOTFUNC(CSDrop);
BOTFUNCHELP(CSHDrop);
BOTFUNC(CSIdentify);
BOTFUNCHELP(CSHIdentify);
BOTFUNC(CSInfo);
BOTFUNCHELP(CSHInfo);
BOTFUNC(CSInvite);
BOTFUNCHELP(CSHInvite);
BOTFUNC(CSModos);
BOTFUNCHELP(CSHModos);
BOTFUNC(CSKick);
BOTFUNCHELP(CSHKick);
BOTFUNC(CSClear);
BOTFUNCHELP(CSHClear);
BOTFUNC(CSOpts);
BOTFUNCHELP(CSHSet);
BOTFUNC(CSAkick);
BOTFUNCHELP(CSHAkick);
BOTFUNC(CSAccess);
BOTFUNCHELP(CSHAccess);
BOTFUNC(CSList);
BOTFUNCHELP(CSHList);
BOTFUNC(CSJb);
BOTFUNCHELP(CSHJoin);
BOTFUNC(CSSendpass);
BOTFUNCHELP(CSHSendpass);
BOTFUNC(CSSuspender);
BOTFUNCHELP(CSHSuspender);
BOTFUNC(CSLiberar);
BOTFUNCHELP(CSHLiberar);
BOTFUNC(CSForbid);
BOTFUNCHELP(CSHForbid);
BOTFUNC(CSUnforbid);
BOTFUNCHELP(CSHUnforbid);
BOTFUNC(CSBlock);
BOTFUNCHELP(CSHBlock);
BOTFUNC(CSRegister);
BOTFUNCHELP(CSHRegister);
BOTFUNC(CSToken);
BOTFUNCHELP(CSHToken);
BOTFUNC(CSMarcas);
BOTFUNCHELP(CSHMarcas);

int CSSigSQL	();
int CSSigEOS	();
int CSSigSynch	();
int CSSigPreNick (Cliente *, char *);
int CSSigStartUp ();

ProcFunc(CSDropachans);
int CSDropanick(char *);
int CSBaja(char *, int);

int CSCmdMode(Cliente *, Canal *, char *[], int);
int CSCmdJoin(Cliente *, Canal *);
int CSCmdKick(Cliente *, Cliente *, Canal *, char *);
int CSCmdTopic(Cliente *, Canal *, char *);

int CSTest(Conf *, int *);
void CSSet(Conf *, Modulo *);
SQLRow CSEsAkick(char *, char *);
int CSTieneAuto(char *, char *, char);
SQLRes CSEsAccess(char *, char *);
mTab *cmodreg = NULL;

static bCom chanserv_coms[] = {
	{ "help" , CSHelp , N0 , "Muestra esta ayuda." , NULL } ,
	{ "deauth" , CSDeauth , N1 , "Te quita el estado de fundador." , CSHDeauth } ,
	{ "drop" , CSDrop , N3 , "Desregistra un canal." , CSHDrop } ,
	{ "identify" , CSIdentify , N1 , "Te identifica como fundador de un canal." , CSHIdentify } ,
	{ "info" , CSInfo , N0 , "Muestra distinta información de un canal." , CSHInfo } ,
	{ "invite" , CSInvite , N1 , "Invita a un usuario al canal." , CSHInvite } ,
	{ "modo" , CSModos , N1 , "Da o quita un modo de canal a varios usuarios." , CSHModos } ,
	{ "kick" , CSKick , N1 , "Expulsa a un usuario." , CSHKick } ,
	{ "clear" , CSClear , N1 , "Resetea distintas opciones del canal." , CSHClear } ,
	{ "set" , CSOpts , N1 , "Fija distintas opciones del canal." , CSHSet } ,
	{ "akick" , CSAkick , N1 , "Gestiona la lista de auto-kick del canal." , CSHAkick } ,
	{ "access" , CSAccess , N1 , "Gestiona la lista de accesos." , CSHAccess } ,
	{ "list" , CSList , N0 , "Lista canales según un determinado patrón." , CSHList } ,
	{ "join" , CSJb , N1 , "Introduce distintos servicios." , CSHJoin } ,
	{ "sendpass" , CSSendpass , N2 , "Genera y manda una clave al email del fundador." , CSHSendpass } ,
	{ "suspender" , CSSuspender , N3 , "Suspende un canal." , CSHSuspender } ,
	{ "liberar" , CSLiberar , N3 , "Quita el suspenso de un canal." , CSHLiberar } ,
	{ "forbid" , CSForbid , N4 , "Prohibe el uso de un canal." , CSHForbid } ,
	{ "unforbid" , CSUnforbid , N4 , "Quita la prohibición de utilizar un canal." , CSHUnforbid } ,
	{ "block" , CSBlock , N2 , "Bloquea y paraliza un canal." , CSHBlock } ,
	{ "register" , CSRegister , N1 , "Registra un canal." , CSHRegister } ,
	{ "token" , CSToken , N1 , "Te entrega un token para las operaciones que lo requieran." , CSHToken } ,
	{ "marca", CSMarcas , N3 , "Lista o inserta una entrada en el historial de un canal." , CSHMarcas } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

DLLFUNC mTab cFlags[] = {
	{ CS_LEV_SET , 's' } ,
	{ CS_LEV_EDT , 'e' } ,
	{ CS_LEV_LIS , 'l' } ,
	{ CS_LEV_RMO , 'r' } ,
	{ CS_LEV_RES , 'c' } ,
	{ CS_LEV_ACK , 'k' } ,
	{ CS_LEV_INV , 'i' } ,
	{ CS_LEV_JOB , 'j' } ,
	{ CS_LEV_REV , 'g' } ,
	{ CS_LEV_MEM , 'm' } ,
	{ 0x0 , 0x0 }
};

ModInfo MOD_INFO(ChanServ) = {
	"ChanServ" ,
	0.11 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" 
};
	
int MOD_CARGA(ChanServ)(Modulo *mod)
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
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(ChanServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(ChanServ).nombre))
			{
				if (!CSTest(modulo.seccion[0], &errores))
					CSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(ChanServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(ChanServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		CSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(ChanServ)()
{
	BorraSenyal(SIGN_MODE, CSCmdMode);
	BorraSenyal(SIGN_TOPIC, CSCmdTopic);
	BorraSenyal(SIGN_JOIN, CSCmdJoin);
	BorraSenyal(SIGN_KICK, CSCmdKick);
	BorraSenyal(SIGN_SQL, CSSigSQL);
	BorraSenyal(NS_SIGN_DROP, CSDropanick);
	BorraSenyal(SIGN_EOS, CSSigEOS);
	BorraSenyal(SIGN_SYNCH, CSSigSynch);
	BorraSenyal(SIGN_PRE_NICK, CSSigPreNick);
	BorraSenyal(SIGN_QUIT, CSSigPreNick);
	//BorraSenyal(SIGN_STARTUP, CSSigStartUp);
	DetieneProceso(CSDropachans);
	BotUnset(chanserv);
	return 0;
}
int CSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], chanserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void CSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!chanserv)
		chanserv = BMalloc(ChanServ);
	chanserv->autodrop = 15;
	chanserv->maxlist = 30;
	chanserv->bantype = 3;
	ircstrdup(chanserv->tokform, "##########");
	chanserv->vigencia = 24;
	chanserv->necesarios = 10;
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "sid"))
				chanserv->opts |= CS_SID;
			else if (!strcmp(config->seccion[i]->item, "autodrop"))
				chanserv->autodrop = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "maxlist"))
				chanserv->maxlist = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "bantype"))
				chanserv->bantype = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, chanserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, chanserv_coms);
			else if (!strcmp(config->seccion[i]->item, "tokens"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "formato"))
						ircstrdup(chanserv->tokform, config->seccion[i]->seccion[p]->data);
					else if (!strcmp(config->seccion[i]->seccion[p]->item, "vigencia"))
						chanserv->vigencia = atoi(config->seccion[i]->seccion[p]->data);
					else if (!strcmp(config->seccion[i]->seccion[p]->item, "necesarios"))
						chanserv->necesarios = atoi(config->seccion[i]->seccion[p]->data);
				}
			}
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
			/*else if (!strcmp(config->seccion[i]->item, "extension"))
			{
				if (!CreaExtension(config->seccion[i], mod))
					Error("[%s:%s::%i] No se ha podido crear la extensión %s", config->seccion[i]->archivo, config->seccion[i]->item, config->seccion[i]->linea, config->seccion[i]->data);
			}*/
		}
	}
	else
	{
		chanserv->opts |= CS_SID;
		ProcesaComsMod(NULL, mod, chanserv_coms);
	}
	InsertaSenyal(SIGN_MODE, CSCmdMode);
	InsertaSenyal(SIGN_TOPIC, CSCmdTopic);
	InsertaSenyal(SIGN_JOIN, CSCmdJoin);
	InsertaSenyal(SIGN_KICK, CSCmdKick);
	InsertaSenyal(SIGN_SQL, CSSigSQL);
	InsertaSenyal(NS_SIGN_DROP, CSDropanick);
	InsertaSenyal(SIGN_EOS, CSSigEOS);
	InsertaSenyal(SIGN_SYNCH, CSSigSynch);
	InsertaSenyal(SIGN_PRE_NICK, CSSigPreNick);
	InsertaSenyal(SIGN_QUIT, CSSigPreNick);
	//InsertaSenyal(SIGN_STARTUP, CSSigStartUp);
	IniciaProceso(CSDropachans);
	BotSet(chanserv);
	//CargaExtensiones(mod);
}
void CSDebug(char *canal, char *formato, ...)
{
	char *opts;
	if (!canal)
		return;
	if (!(opts = SQLCogeRegistro(CS_SQL, canal, "opts")))
		return;
	if (atoi(opts) & CS_OPT_DEBUG)
	{
		Canal *cn;
		va_list vl;
		char buf[BUFSIZE];
		va_start(vl, formato);
		ircvsprintf(buf, formato, vl);
		va_end(vl);
		cn = BuscaCanal(canal);
		ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), buf);
	}
}
DLLFUNC int CSEsFundador(Cliente *al, char *canal)
{
	char *fun;
	if (!al)
		return 0;
	if (!(fun = SQLCogeRegistro(CS_SQL, canal, "founder")))
		return 0;
	if (!strcasecmp(fun, al->nombre))
		return 1;
	return 0;
}
char *CSEsFundador_cache(Cliente *al, char *canal)
{
	char *cache;
	if ((cache = CogeCache(CACHE_FUNDADORES, al->nombre, chanserv->hmod->id)))
	{
		char *tok;
		for (tok = strtok(cache, " "); tok; tok = strtok(NULL, " "))
		{
			if (!strcasecmp(tok, canal))
				return cache;
		}
	}
	return NULL;
}
/* hace un empaquetamiento de los users de una determinada lista. No olvidar hacer Free() */
Cliente **CSEmpaquetaClientes(Canal *cn, LinkCliente *lista, u_int opers)
{
	Cliente **al;
	LinkCliente *lk;
	u_int i, j = 0;
	al = (Cliente **)Malloc(sizeof(Cliente *) * (cn->miembros + 1)); /* como máximo */
	lk = (lista ? lista : cn->miembro);
	for (i = 0; i < cn->miembros && lk; i++)
	{
		if (!EsBot(lk->user) && (!IsOper(lk->user) || opers))
			al[j++] = lk->user;
		lk = lk->sig;
	}
	al[j] = NULL;
	return al;
}
void CSMarca(Cliente *cl, char *nombre, char *marca)
{
	char *marcas;
	if (IsChanReg(nombre))
	{
		time_t fecha = time(NULL);
		if ((marcas = SQLCogeRegistro(CS_SQL, nombre, "marcas")))
			SQLInserta(CS_SQL, nombre, "marcas", "%s\t\00312%s\003 - \00312%s\003 - %s", marcas, Fecha(&fecha), cl->nombre, marca);
		else
			SQLInserta(CS_SQL, nombre, "marcas", "\00312%s\003 - \00312%s\003 - %s", Fecha(&fecha), cl->nombre, marca);
	}
}
BOTFUNCHELP(CSHInfo)
{
	Responde(cl, CLI(chanserv), "Muestra distinta información de un canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312INFO #canal");
	return 0;
}
BOTFUNCHELP(CSHList)
{
	Responde(cl, CLI(chanserv), "Lista los canales registrados que concuerdan con un patrón.");
	Responde(cl, CLI(chanserv), "Puedes utilizar comodines (*) para ajustar la búsqueda.");
	Responde(cl, CLI(chanserv), "Además, puedes especificar el parámetro -r para listar los canales en petición.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312LIST [-r] patrón");
	return 0;
}
BOTFUNCHELP(CSHIdentify)
{
	Responde(cl, CLI(chanserv), "Te identifica como fundador de un canal.");
	Responde(cl, CLI(chanserv), "Este comando sólo debe usarse si el nick que llevas no es el de fundador. "
					"En caso contrario, ya quedas identificado como tal si llevas nick identificado.");
	Responde(cl, CLI(chanserv), "Esta identificación se pierde en caso que te desconectes.");
	Responde(cl, CLI(chanserv), "Para proceder tienes que especificar la contraseña del canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312IDENTIFY #canal pass");
	return 0;
}
BOTFUNCHELP(CSHDeauth)
{
	Responde(cl, CLI(chanserv), "Te borra la identificación que pudieras tener.");
	Responde(cl, CLI(chanserv), "Este comando sólo puede realizarse si previamente te habías identificado con el comando IDENTIFY.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312DEAUTH #canal");
	return 0;
}
BOTFUNCHELP(CSHInvite)
{
	Responde(cl, CLI(chanserv), "Obliga a \00312%s\003 a mandar una invitación para un canal.", chanserv->hmod->nick);
	Responde(cl, CLI(chanserv), "Si no especificas ningún nick, la invitación se te mandará a ti.");
	Responde(cl, CLI(chanserv), "Por el contrario, si especificas un nick, se le será mandada.");
	Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+i\003 para este canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312INVITE #canal [nick]");
	return 0;
}
BOTFUNCHELP(CSHModos)
{
	Responde(cl, CLI(chanserv), "Da o quita un modo de canal a varios usuarios a la vez.");
	Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312MODO {+|-}modo #canal nick1 [nick2 [nick3 [...nickN]]]");
	Responde(cl, CLI(chanserv), "Ejemplo: \00312MODO +o #canal nick1 nick2");
	return 0;
}
BOTFUNCHELP(CSHKick)
{
	Responde(cl, CLI(chanserv), "Fuerza a \00312%s\003 a expulsar a un usuario.", chanserv->hmod->nick);
	Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312KICK #canal nick [motivo]");
	return 0;
}
BOTFUNCHELP(CSHClear)
{
	Responde(cl, CLI(chanserv), "Resetea distintas opciones de un canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Opciones disponibles:");
	Responde(cl, CLI(chanserv), "\00312USERS\003 Expulsa a todos los usuarios del canal. Puede especificar un motivo.");
	Responde(cl, CLI(chanserv), "\00312ACCESOS\003 Borra todos los accesos del canal.");
	Responde(cl, CLI(chanserv), "\00312MODOS\003 Quita todos los modos del canal y lo deja en +nt.");
	Responde(cl, CLI(chanserv), "\0012{modo}\003 Quita el estado de {modo} a todos los nicks del canal.");
	Responde(cl, CLI(chanserv), "Por ejemplo, CLEAR o quitaría el modo +o a todos los nicks del canal.");
	if (IsOper(cl))
	{
		Responde(cl, CLI(chanserv), "\00312KILL\003 Desconecta a todos los usuarios del canal. Puede especificar un motivo.");
		Responde(cl, CLI(chanserv), "\00312GLINE\003 Pone una GLine a todos los usuarios del canal. Puede especificar un motivo.");
	}
	else
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+c\003.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312opcion #canal %s", IsOper(cl) ? "[tiempo]" : "");
	return 0;
}
BOTFUNCHELP(CSHSet)
{
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), "Fija distintas opciones del canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Opciones disponibles:");
		Responde(cl, CLI(chanserv), "\00312DESC\003 Descripción del canal.");
		Responde(cl, CLI(chanserv), "\00312URL\003 Página web.");
		Responde(cl, CLI(chanserv), "\00312EMAIL\003 Dirección de correo.");
		Responde(cl, CLI(chanserv), "\00312BIENVENIDA\003 Mensaje de bienvenida.");
		Responde(cl, CLI(chanserv), "\00312MODOS\003 Modos por defecto.");
		Responde(cl, CLI(chanserv), "\00312OPCIONES\003 Diferentes características.");
		Responde(cl, CLI(chanserv), "\00312TOPIC\003 Topic por defecto.");
		Responde(cl, CLI(chanserv), "\00312PASS\003 Cambia la contraseña.");
		Responde(cl, CLI(chanserv), "\00312FUNDADOR\003 Cambia el fundador.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+s\003.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal opcion [parámetros]");
		Responde(cl, CLI(chanserv), "Para más información, \00312/msg %s %s SET opción", chanserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[2], "DESC"))
	{
		Responde(cl, CLI(chanserv), "\00312SET DESC");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Fija la descripción del canal.");
		Responde(cl, CLI(chanserv), "Esta descripción debe ser clara y concisa. Debe reflejar la temática del canal con la máxima precisión.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal DESC descripción");
	}
	else if (!strcasecmp(param[2], "URL"))
	{
		Responde(cl, CLI(chanserv), "\00312SET URL");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Asocia una dirección web al canal.");
		Responde(cl, CLI(chanserv), "Esta dirección es visualizada en el comando INFO.");
		Responde(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal URL [http://pagina.web]");
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		Responde(cl, CLI(chanserv), "\00312SET EMAIL");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Asocia una dirección de correco al canal.");
		Responde(cl, CLI(chanserv), "Esta dirección es visualizada en el comando INFO.");
		Responde(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal EMAIL [direccion@de.correo]");
	}
	else if (!strcasecmp(param[2], "BIENVENIDA"))
	{
		Responde(cl, CLI(chanserv), "\00312SET BIENVENIDA");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Asocia un mensaje de bienvenida al entrar al canal.");
		Responde(cl, CLI(chanserv), "Cada vez que un usuario entre en el canal se le mandará este mensaje vía NOTICE.");
		Responde(cl, CLI(chanserv), "Si no se especifica nada, es borrado.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal BIENVENIDA [mensaje]");
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		Responde(cl, CLI(chanserv), "\00312SET MODOS");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Define los modos de canal.");
		Responde(cl, CLI(chanserv), "Es altamente recomendable que se especifiquen con el siguiente formato: +modos1-modos2");
		Responde(cl, CLI(chanserv), "Se aceptan parámetros para estos modos. No obstante, los modos oqveharb no se pueden utilizar.");
		Responde(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal MODOS [+modos1-modos2]");
	}
	else if (!strcasecmp(param[2], "OPCIONES"))
	{
		Responde(cl, CLI(chanserv), "\00312SET OPCIONES");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Fija las distintas características del canal.");
		Responde(cl, CLI(chanserv), "Estas características se identifican por flags.");
		Responde(cl, CLI(chanserv), "Así pues, en un mismo comando puedes subir y bajar distintos flags.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Flags disponibles:");
		Responde(cl, CLI(chanserv), "\00312+m\003 Candado de modos: los modos no pueden modificarse.");
		Responde(cl, CLI(chanserv), "\00312+r\003 Retención del topic: se guarda el último topic utilizado en el canal.");
		Responde(cl, CLI(chanserv), "\00312+k\003 Candado de topic: el topic no puede cambiarse.");
		Responde(cl, CLI(chanserv), "\00312+s\003 Canal seguro: sólo pueden entrar los que tienen acceso.");
		Responde(cl, CLI(chanserv), "\00312+o\003 Secureops: sólo pueden tener +a, +o, +h o +v los usuarios con el correspondiente acceso.");
		Responde(cl, CLI(chanserv), "\00312+h\003 Canal oculto: no se lista en el comando LIST.");
		Responde(cl, CLI(chanserv), "\00312+d\003 Canal debug: activa el debug del canal para visualizar los eventos y su autor.");
		if (IsOper(cl))
			Responde(cl, CLI(chanserv), "\00312+n\003 Canal no dropable: sólo puede droparlo la administración.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal OPCIONES +flags-flags");
		Responde(cl, CLI(chanserv), "Ejemplo: \00312SET #canal OPCIONES +ors-mh");
		Responde(cl, CLI(chanserv), "El canal #canal ahora posee las opciones de secureops, retención de topic y canal seguro. "
						"Se quitan las opciones de candado de modos y canal oculto si las tuviera.");
	}
	else if (!strcasecmp(param[2], "TOPIC"))
	{
		Responde(cl, CLI(chanserv), "\00312SET TOPIC");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Fija el topic por defecto del canal.");
		Responde(cl, CLI(chanserv), "Si no se especifica nada, es borrado.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal TOPIC [topic]");
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		Responde(cl, CLI(chanserv), "\00312SET PASS");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Cambia la contraseña del canal.");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas ser el fundador o haberte identificado como tal con el comando IDENTIFY.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal PASS nuevopass");
	}
	else if (!strcasecmp(param[2], "FUNDADOR"))
	{
		Responde(cl, CLI(chanserv), "\00312SET FUNDADOR");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Cambia el fundador del canal.");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas ser el fundador o haberte identificado como tal con el comando IDENTIFY.");
		Responde(cl, CLI(chanserv), "Además, el nuevo nick deberá estar registrado.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal FUNDADOR nick");
	}
	else
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNCHELP(CSHAkick)
{
	Responde(cl, CLI(chanserv), "Gestiona la lista de auto-kick del canal.");
	Responde(cl, CLI(chanserv), "Esta lista contiene las máscaras prohibidas.");
	Responde(cl, CLI(chanserv), "Si un usuario entra al canal y su máscara coincide con una de ellas, immediatamente es expulsado "
					"y se le pone un ban para que no pueda entrar.");
	Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+k\003.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal nick|máscara motivo");
	Responde(cl, CLI(chanserv), "Añade el nick o la máscara con el motivo indicado.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal nick|máscara");
	Responde(cl, CLI(chanserv), "Borra el nick o la máscara.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal");
	Responde(cl, CLI(chanserv), "Lista las diferentes entradas.");
	return 0;
}
BOTFUNCHELP(CSHAccess)
{
	Responde(cl, CLI(chanserv), "Gestiona la lista de accesos del canal.");
	Responde(cl, CLI(chanserv), "Esta lista da privilegios a los usuarios que estén en ella.");
	Responde(cl, CLI(chanserv), "Estos privilegios se les llama accesos y permiten acceder a distintos comandos.");
	Responde(cl, CLI(chanserv), "El fundador tiene acceso a todos ellos.");
	Responde(cl, CLI(chanserv), "Paralelamente también dispone de automodos. Estos automodos se reciben cuando se entra en el canal.");
	Responde(cl, CLI(chanserv), "Se especifican entre [ y ].");
	Responde(cl, CLI(chanserv), "Si se especifican privilegios, se añadirán o quitarán a los que pudiera tener. En cambio, si se especifican automodos, "
					"no se conservarán los que pudiera tener; sino que se usarán los especificados.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Accesos disponibles:");
	Responde(cl, CLI(chanserv), "\00312+s\003 Fijar opciones (SET)");
	Responde(cl, CLI(chanserv), "\00312+e\003 Editar la lista de accesos (ACCESS)");
	Responde(cl, CLI(chanserv), "\00312+l\003 Listar la lista de accesos (ACCESS)");
	Responde(cl, CLI(chanserv), "\00312+r\003 Comando (MODO)");
	Responde(cl, CLI(chanserv), "\00312+c\003 Resetear opciones (CLEAR)");
	Responde(cl, CLI(chanserv), "\00312+k\003 Gestionar la lista de auto-kicks (AKICK)");
	Responde(cl, CLI(chanserv), "\00312+i\003 Invitar (INVITE)");
	Responde(cl, CLI(chanserv), "\00312+j\003 Entrar bots (JOIN)");
	Responde(cl, CLI(chanserv), "\00312+g\003 Kick de venganza.");
	Responde(cl, CLI(chanserv), "\00312+m\003 Gestión de memos al canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "El nick al que se le dan los accesos debe estar registrado previamente.");
	Responde(cl, CLI(chanserv), "Un acceso no engloba otro. Por ejemplo, el tener +e no implica tener +l.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal nick [+flags-flags] [automodos]");
	Responde(cl, CLI(chanserv), "Añade los accesos al nick y sus automodos.");
	Responde(cl, CLI(chanserv), "Ejemplo: \00312ACCESS #canal pepito +m-le [o]");
	Responde(cl, CLI(chanserv), "pepito tendría +o al entrar en #canal, se le daría privilegio para gestionar memos y se le quitarían los privilegios de acceder y editar lista.");
	Responde(cl, CLI(chanserv), "Ejemplo: \00312ACCESS #canal pepito [h]");
	Responde(cl, CLI(chanserv), "pepito recibiría el modo +h al entrar en #canal y no se modificarían los privilegios que tuviera.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal nick []");
	Responde(cl, CLI(chanserv), "Borra todos los automodos de este nick.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal nick");
	Responde(cl, CLI(chanserv), "Borra todos los accesos de este nick.");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal");
	Responde(cl, CLI(chanserv), "Lista todos los usuarios con acceso al canal.");
	return 0;
}
BOTFUNCHELP(CSHJoin)
{
	Responde(cl, CLI(chanserv), "Introduce los servicios al canal de forma permanente.");
	Responde(cl, CLI(chanserv), "Primero se quitan los servicios que pudiera haber para luego meter los que se soliciten.");
	Responde(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
	Responde(cl, CLI(chanserv), "Para realizar este comando debes tener acceso \00312+j\003.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312JOIN #canal [servicio1 [servicio2 [...servicioN]]]");
	return 0;
}
BOTFUNCHELP(CSHRegister)
{
	if (IsOper(cl))
	{
		Responde(cl, CLI(chanserv), "Registra un canal para que sea controlado por los servicios.");
		Responde(cl, CLI(chanserv), "Este registro permite el control absoluto sobre el mismo.");
		Responde(cl, CLI(chanserv), "Con este comando puedes aceptar dos tipos de canales:");
		Responde(cl, CLI(chanserv), "- Aceptar peticiones de canales presentadas por los usuarios.");
		Responde(cl, CLI(chanserv), "- Registrar tus propios canales sin la necesidad de tokens.");
		Responde(cl, CLI(chanserv), "Aceptar una petición significa que registras un canal presentado con su fundador para que pase a sus manos.");
		Responde(cl, CLI(chanserv), "El canal expiará despues de \00312%i\003 días en los que no haya entrado ningún usuario con acceso.", chanserv->autodrop);
		Responde(cl, CLI(chanserv), "Para listar los canales en petición utiliza el comando LIST.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal");
		Responde(cl, CLI(chanserv), "Acepta la petición presentada por un usuario.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripción");
		Responde(cl, CLI(chanserv), "Registra tu propio canal.");
	}
	else
	{
		Responde(cl, CLI(chanserv), "Solicita el registro de un canal.");
		Responde(cl, CLI(chanserv), "Para hacer una petición necesitas presentar como mínimo \00312%i\003 tokens.", chanserv->necesarios);
		Responde(cl, CLI(chanserv), "Estos tokens actúan como unas firmas que dan el visto bueno para su registro.");
		Responde(cl, CLI(chanserv), "Los tokens provienen de otros usuarios que los solicitan con el comando TOKEN.");
		Responde(cl, CLI(chanserv), "Si solicitas un token, no podrás presentar una solicitud hasta que no pase un cierto tiempo.");
		Responde(cl, CLI(chanserv), "El canal expiará despues de \00312%i\003 días en los que no haya entrado ningún usuario con acceso.", chanserv->autodrop);
		Responde(cl, CLI(chanserv), "Para más información consulta la ayuda del comando TOKEN.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripción tokens");
	}
	return 0;
}
BOTFUNCHELP(CSHToken)
{
	Responde(cl, CLI(chanserv), "Solicita un token.");
	Responde(cl, CLI(chanserv), "Este token es necesario para acceder a comandos que requieran el acuerdo de varios usuarios.");
	Responde(cl, CLI(chanserv), "Sólo puedes solicitar un token cada \00312%i\003 horas. Una vez hayan transcurrido, el token caduca y puedes solicitar otro.", chanserv->vigencia);
	Responde(cl, CLI(chanserv), "Actúa como tu firma personal. Y es único.");
	Responde(cl, CLI(chanserv), "Por ejemplo, si alguien solicita el registro de un canal y tu estás a favor, deberás darle tu token a quién lo solicite para presentarlos "
					"para que así la administración confirme que cuenta con el mínimo de usuarios en acuerdo.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312TOKEN");
	return 0;
}
BOTFUNCHELP(CSHSendpass)
{
	Responde(cl, CLI(chanserv), "Regenera una nueva clave para un canal y es enviada al email del fundador.");
	Responde(cl, CLI(chanserv), "Este comando sólo debe utilizarse a petición del fundador.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312SENDPASS #canal");
	return 0;
}
BOTFUNCHELP(CSHBlock)
{
	Responde(cl, CLI(chanserv), "Inhabilita temporalmente un canal.");
	Responde(cl, CLI(chanserv), "Se quitan todos los privilegios de un canal (+qaohv) y se ponen los modos +iml 1");
	Responde(cl, CLI(chanserv), "Su durada es temporal y puede se quitado mediante el comando CLEAR.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312BLOCK #canal");
	return 0;
}
BOTFUNCHELP(CSHDrop)
{
	Responde(cl, CLI(chanserv), "Borra un canal de la base de datos.");
	Responde(cl, CLI(chanserv), "Este canal tiene que haber estado registrado previamente.");
	Responde(cl, CLI(chanserv), "También puede utilizarse para denegar la petición de registro de un canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312DROP #canal");
	return 0;
}
BOTFUNCHELP(CSHSuspender)
{
	Responde(cl, CLI(chanserv), "Suspende un canal de ser utilizado.");
	Responde(cl, CLI(chanserv), "El canal seguirá estando registrado pero sus usuarios, fundador incluído, no podrán aprovechar "
					"ninguna de sus prestaciones.");
	Responde(cl, CLI(chanserv), "Este suspenso durará indefinidamente hasta que se decida levantarlo.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312SUSPENDER #canal motivo");
	return 0;
}
BOTFUNCHELP(CSHLiberar)
{
	Responde(cl, CLI(chanserv), "Quita el suspenso puesto previamente con el comando SUSPENDER.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312LIBERAR #canal");
	return 0;
}
BOTFUNCHELP(CSHForbid)
{
	Responde(cl, CLI(chanserv), "Prohibe el uso de este canal.");
	Responde(cl, CLI(chanserv), "Los usuarios no podrán ni entrar ni registrarlo.");
	Responde(cl, CLI(chanserv), "Este tipo de canales se extienden por toda la red.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312FORBID #canal motivo");
	return 0;
}
BOTFUNCHELP(CSHUnforbid)
{
	Responde(cl, CLI(chanserv), "Quita la prohibición de un canal puesta con el comando FORBID.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312UNFORBID #canal");
	return 0;
}
BOTFUNCHELP(CSHMarcas)
{
	Responde(cl, CLI(chanserv), "Inserta una marca en el historial de un canal.");
	Responde(cl, CLI(chanserv), "Las marcas son anotaciones que no pueden eliminarse.");
	Responde(cl, CLI(chanserv), "En ellas figura la fecha, su autor y un comentario.");
	Responde(cl, CLI(chanserv), "Si no se especifica ninguna marca, se listarán todas las que hubiera.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312MARCA [#canal]");
	return 0;
}
BOTFUNC(CSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), "\00312%s\003 se encarga de gestionar los canales de la red para evitar robos o takeovers.", chanserv->hmod->nick);
		Responde(cl, CLI(chanserv), "El registro de canales permite su apropiación y poder gestionarlos de tal forma que usuarios no deseados "
						"puedan tomar control de los mismos.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Comandos disponibles:");
		ListaDescrips(chanserv->hmod, cl);
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Para más información, \00312/msg %s %s comando", chanserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], chanserv->hmod, param, params))
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
	EOI(chanserv, 0);
	return 0;
}
BOTFUNC(CSDrop)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!ChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsAdmin(cl) && (atoi(SQLCogeRegistro(CS_SQL, param[1], "opts")) & CS_OPT_NODROP))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no se puede dropear.");
		return 1;
	}
	if (!CSBaja(param[1], 1))
		Responde(cl, CLI(chanserv), "Se ha dropeado el canal \00312%s\003.", param[1]);
	else
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No se ha podido dropear. Comuníquelo a la administración.");
	EOI(chanserv, 1);
	return 0;
}
BOTFUNC(CSIdentify)
{
	char *cache, buf[BUFSIZE];
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if ((chanserv->opts & CS_SID) && !parv[parc])
	{
		ircsprintf(buf, "Identificación incorrecta. /msg %s@%s IDENTIFY #canal pass", chanserv->hmod->nick, me.nombre);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}	
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2], 0)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	if (CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya eres fundador de este canal.");
		return 1;
	}
	ircsprintf(buf, "%s ", param[1]);
	if ((cache = CogeCache(CACHE_FUNDADORES, cl->nombre, chanserv->hmod->id)))
	{
		char *tok;
		for (tok = strtok(cache, " "); tok; tok = strtok(NULL, " "))
		{
			if (!strcasecmp(tok, param[1]))
			{
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya te has identificado como fundador de este canal.");
				return 1;
			}
		}
		strlcat(buf, cache, sizeof(buf));
	}
	InsertaCache(CACHE_FUNDADORES, cl->nombre, 0, chanserv->hmod->id, buf);
	Responde(cl, CLI(chanserv), "Ahora eres reconocido como fundador de \00312%s\003.", param[1]);
	EOI(chanserv, 2);
	return 0;
}
BOTFUNC(CSDeauth)
{
	char *cache, *tok;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Eres fundador de este canal. No puedes quitarte este estado.");
		return 1;
	}
	if (!(cache = CSEsFundador_cache(cl, param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No te has identificado como fundador.");
		return 1;
	}
	buf[0] = '\0';
	for (tok = strtok(cache, " "); tok; tok = strtok(NULL, " "))
	{
		if (strcasecmp(tok, param[1]))
		{
			strlcat(buf, tok, sizeof(buf));
			strlcat(buf, " ", sizeof(buf));
		}
	}
	if (buf[0])
		InsertaCache(CACHE_FUNDADORES, cl->nombre, 0, chanserv->hmod->id, buf);
	Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya no estás identificado como fundador de \00312%s", param[1]);
	EOI(chanserv, 3);
	return 1;
}
BOTFUNC(CSInfo)
{
	SQLRes res;
	SQLRow row;
	time_t reg;
	int opts;
	char *forb, *susp, *modos;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if ((forb = IsChanForbid(param[1])))
	{
		Responde(cl, CLI(chanserv), "Este canal está \2PROHIBIDO\2: %s", forb);
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	res = SQLQuery("SELECT opts,founder,descripcion,registro,entry,url,email,modos,topic,ntopic,ultimo from %s%s where LOWER(item)='%s'", PREFIJO, CS_SQL, strtolower(param[1]));
	row = SQLFetchRow(res);
	opts = atoi(row[0]);
	Responde(cl, CLI(chanserv), "*** Información del canal \00312%s\003 ***", param[1]);
	if ((susp = IsChanSuspend(param[1])))
		Responde(cl, CLI(chanserv), "Estado: \00312SUSPENDIDO: %s", susp);
	else
		Responde(cl, CLI(chanserv), "Estado: \00312ACTIVO");
	Responde(cl, CLI(chanserv), "Fundador: \00312%s", row[1]);
	Responde(cl, CLI(chanserv), "Descripción: \00312%s", row[2]);
	reg = (time_t)atol(row[3]);
	Responde(cl, CLI(chanserv), "Fecha de registro: \00312%s", Fecha(&reg));
	Responde(cl, CLI(chanserv), "Mensaje de bienvenida: \00312%s", row[4]);
	if (!BadPtr(row[5]))
		Responde(cl, CLI(chanserv), "URL: \00312%s", row[5]);
	if (!BadPtr(row[6]))
		Responde(cl, CLI(chanserv), "Email: \00312%s", row[6]);
	if (CSTieneNivel(cl->nombre, param[1], CS_LEV_SET))
		modos = row[7];
	else
		modos = strtok(row[7], " ");
	Responde(cl, CLI(chanserv), "Modos: \00312%s\003 Candado de modos: \00312%s", modos, opts & CS_OPT_RMOD ? "ON" : "OFF");
	Responde(cl, CLI(chanserv), "Topic: \00312%s\003 puesto por: \00312%s", row[8], row[9]);
	Responde(cl, CLI(chanserv), "Candado de topic: \00312%s\003 Retención de topic: \00312%s", opts & CS_OPT_KTOP ? "ON" : "OFF", opts & CS_OPT_RTOP ? "ON" : "OFF");
	if (opts & CS_OPT_SOP)
		Responde(cl, CLI(chanserv), "Canal con \2SECUREOPS");
	if (opts & CS_OPT_SEC)
		Responde(cl, CLI(chanserv), "Canal \2SEGURO");
	if (opts & CS_OPT_NODROP)
		Responde(cl, CLI(chanserv), "Canal \2NO DROPABLE");
	EOI(chanserv, 4);
	reg = (time_t)atol(row[10]);
	Responde(cl, CLI(chanserv), "Último acceso: \00312%s", Fecha(&reg));
	SQLFreeRes(res);
	return 0;
}
BOTFUNC(CSInvite)
{
	Cliente *al = NULL;
	Canal *cn;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal [nick]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSTieneNivel(cl->nombre, param[1], CS_LEV_INV))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB);
		return 1;
	}
	if (params == 3 && !(al = BuscaCliente(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	cn = BuscaCanal(param[1]);
	ProtFunc(P_INVITE)(al ? al : cl, CLI(chanserv), cn);
	Responde(cl, CLI(chanserv), "El usuario \00312%s\003 ha sido invitado a \00312%s\003.", params == 3 ? param[2] : parv[0], param[1]);
	CSDebug(param[1], "%s invita a %s", parv[0], params == 3 ? param[2] : parv[0]); 
	EOI(chanserv, 5);
	return 0;
}
BOTFUNC(CSKick)
{
	Cliente *al;
	Canal *cn;
	if (params < 4)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal nick motivo");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = BuscaCanal(param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no existe.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_RMO))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!(al = BuscaCliente(param[2])))
			return 1;
	ProtFunc(P_KICK)(al, CLI(chanserv), cn, Unifica(param, params, 3, -1));
	CSDebug(param[1], "%s hace KICK a %s", parv[0], Unifica(param, params, 2, -1));
	EOI(chanserv, 6);
	return 0;
}
BOTFUNC(CSModos)
{
	int i, opts;
	Cliente *al;
	Canal *cn;
	MallaCliente *mcl;
	MallaMascara *mmk;
	char flag, modo = ADD;
	if (params < 4)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal {+|-}flag");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = BuscaCanal(param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no existe.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_RMO))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (*param[2] == '+')
		flag = *(param[2]+1);
	else if (*param[2] == '-')
	{
		modo = DEL;
		flag = *(param[2]+1);
	}
	else
		flag = *param[2];
	if ((mcl = BuscaMallaCliente(cn, flag)))
	{
		for (i = 3; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i])))
				continue;
			if (modo == ADD && ((!CSTieneAuto(param[i], param[1], flag) && (opts & CS_OPT_SOP)) || EsLink(mcl->malla, al)))
				continue;
			if (modo == DEL && !EsLink(mcl->malla, al))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", param[2], TRIO(al));
		}
	}
	else if ((mmk = BuscaMallaMascara(cn, flag)))
	{
		for (i = 3; i < params; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", param[2], MascaraIrcd(param[i]));
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este modo no tiene parámetros de clientes o máscaras.");
		return 1;
	}
	CSDebug(param[1], "%s hace MODO %s", parv[0], Unifica(param, params, 2, -1));
	EOI(chanserv, 7);
	return 0;
}
BOTFUNC(CSClear)
{
	Canal *cn;
	Cliente **al = NULL;
	int i;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal opcion");
		return 1;
	}
	if (!IsChanReg(param[1]) && !IsPreo(cl))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = BuscaCanal(param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El canal está vacío.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_RES))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!strcasecmp(param[2], "USERS"))
	{
		char *motivo = "CLEAR USERS";
		al = CSEmpaquetaClientes(cn, NULL, !ADD);
		if (params > 3)
			motivo = Unifica(param, params, 3, -1);
		for (i = 0; al[i]; i++)
			ProtFunc(P_KICK)(al[i], CLI(chanserv), cn, motivo);
	}
	/*else if (!strcasecmp(param[2], "ADMINS") && MODE_ADM)
	{
		al = CSEmpaquetaClientes(cn, cn->admin, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_ADM, TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "OPS"))
	{
		al = CSEmpaquetaClientes(cn, cn->op, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "HALFS") && MODE_HALF)
	{
		al = CSEmpaquetaClientes(cn, cn->half, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_HALF, TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "VOCES"))
	{
		al = CSEmpaquetaClientes(cn, cn->voz, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-v %s", TRIO(al[i]));
	}*/
	else if (!strcasecmp(param[2], "ACCESOS"))
	{
		char *c_c;
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
			return 1;
		}
		c_c = SQLEscapa(strtolower(param[1]));
		SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_ACCESS, c_c);
		Responde(cl, CLI(chanserv), "Lista de accesos eliminada.");
		Free(c_c);
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%s", ModosAFlags(cn->modos, protocolo->cmodos, cn));
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+nt%c", IsChanReg(param[1]) && cmodreg ? cmodreg->flag : 0);
		Responde(cl, CLI(chanserv), "Modos resetados a +nt.");
	}
	/*else if (!strcasecmp(param[2], "BANS"))
	{
		while (cn->ban)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-b %s", cn->ban->ban);
	}*/
	else if (!strcasecmp(param[2], "KILL"))
	{
		char *motivo = "KILL USERS";
		if (!IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params > 3)
			motivo = Unifica(param, params, 3, -1);
		al = CSEmpaquetaClientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_QUIT_USUARIO_REMOTO)(al[i], CLI(chanserv), motivo);
		Responde(cl, CLI(chanserv), "Usuarios de \00312%s\003 desconectados.", param[1]);
	}
	else if (!strcasecmp(param[2], "GLINE"))
	{
		LinkCliente *aux;
		char *motivo = "GLINE USERS";
		if (!IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal GLINE tiempo");
			return 1;
		}
		if (atoi(param[3]) < 1)
		{
			Responde(cl, CLI(chanserv), CS_ERR_SNTX, "El tiempo debe ser superior a 1 segundo.");
			return 1;
		}
		if (params > 4)
			motivo = Unifica(param, params, 4, -1);
		for (aux = cn->miembro; aux; aux = aux->sig)
		{
			if (!EsBot(aux->user) && !IsOper(aux->user))
				ProtFunc(P_GLINE)(CLI(chanserv), ADD, aux->user->ident, aux->user->host, atoi(param[3]), motivo);
		}
		Responde(cl, CLI(chanserv), "Usuarios de \00312%s\003 con gline durante \00312%s\003 segundos.", param[1], param[3]);
	}
	else
	{
		MallaCliente *mcl;
		MallaMascara *mmk;
		if ((mcl = BuscaMallaCliente(cn, *param[2])))
		{
			al = CSEmpaquetaClientes(cn, mcl->malla, !ADD);
			for (i = 0; al[i]; i++)
				ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", *param[2], TRIO(al[i]));
		}
		else if ((mmk = BuscaMallaMascara(cn, *param[2])))
		{
			while (mmk->malla)
				ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", *param[2], mmk->malla->mascara);
		}
		else
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este modo no tiene parámetros de clientes o máscaras.");
			return 1;
		}
	}
	CSDebug(param[1], "%s hace CLEAR %s", parv[0], param[2]);
	ircfree(al);
	EOI(chanserv, 8);
	return 0;
}
BOTFUNC(CSOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal parámetros");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_SET))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!strcasecmp(param[2], "DESC"))
	{
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal DESC descripción");
			return 1;
		}
		SQLInserta(CS_SQL, param[1], "descripcion", Unifica(param, params, 3, -1));
		Responde(cl, CLI(chanserv), "Descripción cambiada.");
	}
	else if (!strcasecmp(param[2], "URL"))
	{
		if (params == 4)
			Responde(cl, CLI(chanserv), "URL cambiada.");
		else
			Responde(cl, CLI(chanserv), "URL desactivada.");
		SQLInserta(CS_SQL, param[1], "url", params == 4 ? param[3] : NULL);
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		if (params == 4)
			Responde(cl, CLI(chanserv), "EMAIL cambiado.");
		else
			Responde(cl, CLI(chanserv), "EMAIL desactivado.");
		SQLInserta(CS_SQL, param[1], "email", params == 4 ? param[3] : NULL);
	}
	else if (!strcasecmp(param[2], "TOPIC"))
	{
		char *topic;
		topic = params > 3 ? Unifica(param, params, 3, -1) : NULL;
		if (params > 3)
		{
			Responde(cl, CLI(chanserv), "TOPIC cambiado.");
			ProtFunc(P_TOPIC)(CLI(chanserv), BuscaCanal(param[1]), topic);
		}
		else
			Responde(cl, CLI(chanserv), "TOPIC desactivado.");
		SQLInserta(CS_SQL, param[1], "topic", topic);
		SQLInserta(CS_SQL, param[1], "ntopic", params > 3 ? parv[0] : NULL);
	}
	else if (!strcasecmp(param[2], "BIENVENIDA"))
	{
		char *entry;
		entry = params > 3 ? Unifica(param, params, 3, -1) : NULL;
		if (params > 3)
			Responde(cl, CLI(chanserv), "Mensaje de bienvenida cambiado.");
		else
			Responde(cl, CLI(chanserv), "Mensaje de bienvenida desactivado.");
		SQLInserta(CS_SQL, param[1], "entry", entry);
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		char *modos = NULL;
		if (params > 3)
		{
			char *c;
			for (c = protocolo->modcl; !BadPtr(c); c++)
			{
				if (strchr(param[3], *c))
				{
					ircsprintf(buf, "Los modos %s no se pueden especificar.", protocolo->modcl);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
			}
			modos = Unifica(param, params, 3, -1);
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), BuscaCanal(param[1]), modos);
			Responde(cl, CLI(chanserv), "Modos cambiados.");
			
		}
		else
			Responde(cl, CLI(chanserv), "Modos eliminados.");
		SQLInserta(CS_SQL, param[1], "modos", modos);
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		char *mdpass;
		if (!CSEsFundador(cl, param[1]) && !IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal PASS contraseña");
			return 1;
		}
		mdpass = MDString(param[3], 0);
		SQLInserta(CS_SQL, param[1], "pass", mdpass);
		CSMarca(cl, param[1], "Contraseña cambiada.");
		Responde(cl, CLI(chanserv), "Contraseña cambiada.");
	}
	else if (!strcasecmp(param[2], "FUNDADOR"))
	{
		if (!CSEsFundador(cl, param[1]) && !IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal FUNDADOR nick");
			return 1;
		}
		if (!IsReg(param[3]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		SQLInserta(CS_SQL, param[1], "founder", param[3]);
		ircsprintf(buf, "Fundador cambiado: %s", param[3]);
		CSMarca(cl, param[1], "Contraseña cambiada.");
		Responde(cl, CLI(chanserv), "Fundador cambiado a \00312%s", param[3]);
	}
	else if (!strcasecmp(param[2], "OPCIONES"))
	{
		char f = ADD, *modos = param[3], buenos[128];
		int opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
		buenos[0] = 0;
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal opciones +-modos");
			return 1;
		}
		while (!BadPtr(modos))
		{
			switch(*modos)
			{
				case '+':
					f = ADD;
					chrcat(buenos, '+');
					break;
				case '-':
					f = DEL;
					chrcat(buenos, '-');
					break;
				case 'm':
					if (f == ADD)
						opts |= CS_OPT_RMOD;
					else
						opts &= ~CS_OPT_RMOD;
					chrcat(buenos, 'm');
					break;
				case 'r':
					if (f == ADD)
						opts |= CS_OPT_RTOP;
					else
						opts &= ~CS_OPT_RTOP;
					chrcat(buenos, 'r');
					break;
				case 'k':
					if (f == ADD)
						opts |= CS_OPT_KTOP;
					else
						opts &= ~CS_OPT_KTOP;
					chrcat(buenos, 'k');
					break;
				case 's':
					if (f == ADD)
						opts |= CS_OPT_SEC;
					else
						opts &= ~CS_OPT_SEC;
					chrcat(buenos, 's');
					break;
				case 'o':
					if (f == ADD)
						opts |= CS_OPT_SOP;
					else
						opts &= ~CS_OPT_SOP;
					chrcat(buenos, 'o');
					break;
				case 'h':
					if (f == ADD)
						opts |= CS_OPT_HIDE;
					else
						opts &= ~CS_OPT_HIDE;
					chrcat(buenos, 'h');
					break;
				case 'd':
					if (f == ADD)
						opts |= CS_OPT_DEBUG;
					else
						opts &= ~CS_OPT_DEBUG;
					chrcat(buenos, 'd');
					break;
				case 'n':
					if (!IsOper(cl))
					{
						Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
						return 1;
					}
					if (f == ADD)
						opts |= CS_OPT_NODROP;
					else
						opts &= ~CS_OPT_NODROP;
					chrcat(buenos, 'n');
					break;
			}
			modos++;
		}
		SQLInserta(CS_SQL, param[1], "opts", "%i", opts);
		Responde(cl, CLI(chanserv), "Opciones cambiadas a \00312%s\003.", buenos);
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
		return 1;
	}
	EOI(chanserv, 9);
	return 0;
}
BOTFUNC(CSAkick)
{
	Canal *cn;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal [nick|mascara [motivo]]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = BuscaCanal(param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El canal está vacío.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_ACK))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (params == 2)
	{
		SQLRes res;
		SQLRow row;
		char *c_c;
		c_c = SQLEscapa(strtolower(param[1]));
		if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_AKICKS, c_c)))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ningún akick.");
			Free(c_c);
			return 1;
		}
		Free(c_c);
		while ((row = SQLFetchRow(res)))
			Responde(cl, CLI(chanserv), "\00312%s\003 Motivo: \00312%s\003 (por \00312%s\003)", row[1], row[2], row[3]);
		SQLFreeRes(res);
	}
	else if (params == 3)
	{
		char *m_c, *c_c;
		c_c = SQLEscapa(strtolower(param[1]));
		m_c = SQLEscapa(strtolower(param[2]));
		if (!SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(mascara)='%s'", PREFIJO, CS_AKICKS, c_c, m_c))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Esta entrada no existe");
			Free(c_c);
			Free(m_c);
			return 1;
		}
		SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(mascara)='%s'", PREFIJO, CS_AKICKS, c_c, m_c);
		Responde(cl, CLI(chanserv), "Entrada \00312%s\003 eliminada.", param[2]);
		Free(c_c);
		Free(m_c);
	}
	else if (params > 3)
	{
		Cliente *al;
		char *motivo = Unifica(param, params, 3, -1), *m_c;
		m_c = SQLEscapa(motivo);
		SQLQuery("INSERT INTO %s%s (canal,mascara,motivo,autor) values ('%s','%s','%s','%s')", 
			PREFIJO, CS_AKICKS,
			param[1], param[2], 
			m_c, cl->nombre);
		Free(m_c);
		if ((al = BuscaCliente(param[2])))
			ProtFunc(P_KICK)(al, CLI(chanserv), cn, motivo);
		Responde(cl, CLI(chanserv), "Akick a \00312%s\003 añadido.", param[2]);
	}
	EOI(chanserv, 10);
	return 0;
}
BOTFUNC(CSAccess)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal [nick [+-flags] [automodos]]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (params == 2)
	{
		SQLRes res;
		SQLRow row;
		char *c_c;
		if (!CSTieneNivel(parv[0], param[1], CS_LEV_LIS))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		c_c = SQLEscapa(strtolower(param[1]));
		if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_ACCESS, c_c)))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ningún acceso.");
			Free(c_c);
			return 1;
		}
		Free(c_c);
		Responde(cl, CLI(chanserv), "*** Accesos de \00312%s\003 ***", param[1]);
		while ((row = SQLFetchRow(res)))
		{
			if (!BadPtr(row[2]) && !BadPtr(row[3]))
				Responde(cl, CLI(chanserv), "Nick: \00312%s\003 flags: \00312%s\003 [\00312%s\003] (\00312%s\003)", row[1], ModosAFlags(atoul(row[2]), cFlags, NULL), row[3], row[2]);
			else if (!BadPtr(row[3]))
				Responde(cl, CLI(chanserv), "Nick: \00312%s\003 flags: [\00312%s\003]", row[1], row[3]);
			else if (!BadPtr(row[2]))
				Responde(cl, CLI(chanserv), "Nick: \00312%s\003 flags: \00312%s\003 (\00312%s\003)", row[1], ModosAFlags(atoul(row[2]), cFlags, NULL), row[2]);
		}
		SQLFreeRes(res);
	}
	else if (params == 3)
	{
		char *n_c, *c_c;
		c_c = SQLEscapa(strtolower(param[1]));
		n_c = SQLEscapa(strtolower(param[2]));
		SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, c_c, n_c);
		Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2]);
		CSDebug(param[1], "Acceso de \00312%s\003 eliminado", param[2]);
		Free(c_c);
		Free(n_c);
	}
	else if (params > 3)
	{
		SQLRes res;
		SQLRow row;
		char f = ADD, *modos = NULL, autof[256], *c_c, *n_c;
		u_long prev = 0L;
		if (!CSTieneNivel(parv[0], param[1], CS_LEV_EDT))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (!IsReg(param[2]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		autof[0] = '\0';
		if (*param[3] == '[')
		{
			strlcpy(autof, &param[3][1], sizeof(autof));
			if (params > 4)
				modos = param[4];
		}
		else
		{
			modos = param[3];
			if (params > 4 && *param[4] == '[')
				strlcpy(autof, &param[4][1], sizeof(autof));
		}
		if ((res = CSEsAccess(param[1], param[2])))
		{
			row = SQLFetchRow(res);
			prev = atoul(row[2]);
		}
		while (!BadPtr(modos))
		{
			switch(*modos)
			{
				case '+':
					f = ADD;
					break;
				case '-':
					f = DEL;
					break;
				default:
					if (f == ADD)
						prev |= FlagAModo(*modos, cFlags);
					else
						prev &= ~FlagAModo(*modos, cFlags);
			}
			modos++;
		}
		if (!BadPtr(autof))
		{
			char *c;
			if ((c = strchr(autof, ']')))
				*c = '\0';
		}
		c_c = SQLEscapa(strtolower(param[1]));
		n_c = SQLEscapa(strtolower(param[2]));
		if (!res)
			SQLQuery("INSERT INTO %s%s (canal,nick) values ('%s','%s')", PREFIJO, CS_ACCESS, c_c, n_c);
		SQLQuery("UPDATE %s%s SET nivel=%lu WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, prev, c_c, n_c);
		if (!BadPtr(autof))
			SQLQuery("UPDATE %s%s SET automodos='%s' WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, autof, c_c, n_c);
		if (prev || !BadPtr(autof))
		{
			Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 cambiado a \00312%s\003", param[2], param[3]);
			CSDebug(param[1], "Acceso de \00312%s\003 cambiado a %s", param[2], Unifica(param, params, 3, -1));
		}
		else
		{
			SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, c_c, n_c);
			Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2]);
			CSDebug(param[1], "Acceso de \00312%s\003 eliminado.", param[2]);
		}
		Free(c_c);
		Free(n_c);
		if (res)
			SQLFreeRes(res);
	}
	EOI(chanserv, 11);
	return 0;
}
BOTFUNC(CSList)
{
	char *rep;
	SQLRes res;
	SQLRow row;
	int i;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "[-r] patrón");
		return 1;
	}
	if (params == 3)
	{
		if (*param[1] == '-')
		{
			if (params < 3)
			{
				Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "[-r] patrón");
				return 1;
			}
			switch(*(param[1] + 1))
			{
				case 'r':
					rep = str_replace(param[2], '*', '%');
					res = SQLQuery("SELECT item,descripcion from %s%s where LOWER(item) LIKE '%s' AND registro='0'", PREFIJO, CS_SQL, strtolower(rep));
					if (!res)
					{
						Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay canales para listar.");
						return 1;
					}
					Responde(cl, CLI(chanserv), "*** Canales en petición que coinciden con el patrón \00312%s\003 ***", param[1]);
					for (i = 0; i < chanserv->maxlist && (row = SQLFetchRow(res)); i++)
						Responde(cl, CLI(chanserv), "\00312%s\003 Desc:\00312%s", row[0], row[1]);
					Responde(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
					SQLFreeRes(res);
					break;
				default:
					Responde(cl, CLI(chanserv), CS_ERR_SNTX, "Parámetro no válido.");
					return 1;
			}
		}
	}
	else
	{
		if (*param[1] == '-')
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "[-r] patrón");
			return 1;
		}
		rep = str_replace(param[1], '*', '%');
		res = SQLQuery("SELECT item from %s%s where LOWER(item) LIKE '%s' AND registro!='0'", PREFIJO, CS_SQL, strtolower(rep));
		if (!res)
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay canales para listar.");
			return 1;
		}
		Responde(cl, CLI(chanserv), "*** Canales que coinciden con el patrón \00312%s\003 ***", param[1]);
		for (i = 0; i < chanserv->maxlist && (row = SQLFetchRow(res)); i++)
		{
			if (IsOper(cl) || !(atoi(SQLCogeRegistro(CS_SQL, row[0], "opts")) & CS_OPT_HIDE))
				Responde(cl, CLI(chanserv), "\00312%s", row[0]);
			else
				i--;
		}
		Responde(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	EOI(chanserv, 12);
	return 0;
}
BOTFUNC(CSJb)
{
	Cliente *bl;
	Canal *cn;
	int i, bots = 0;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal [servicios]");
		return 1;
	}
	if (!IsChanReg(param[1]) && !IsOper(cl))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_JOB))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	cn = BuscaCanal(param[1]);
	if (cn)
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			bl = BuscaCliente(ex->nick);
			if (EsLink(cn->miembro, bl) && !ModuloEsResidente(ex, param[1]))
				SacaBot(bl, cn->nombre, "Cambiando bots");
		}
	}
	for (i = 2; i < params; i++)
	{
		Modulo *ex;
		if ((ex = BuscaModulo(param[i], modulos)))
		{
			bl = BuscaCliente(ex->nick);
			EntraBot(bl, param[1]);
			bots |= ex->id;
		}
	}
	if (IsChanReg(param[1]))
		SQLInserta(CS_SQL, param[1], "bjoins", "%i", bots);
	Responde(cl, CLI(chanserv), "Bots cambiados.");
	EOI(chanserv, 13);
	return 0;
}
BOTFUNC(CSSendpass)
{
	char *pass;
	char *founder;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	pass = AleatorioEx("******-******");
	founder = strdup(SQLCogeRegistro(CS_SQL, param[1], "founder"));
	SQLInserta(CS_SQL, param[1], "pass", MDString(pass, 0));
	Email(SQLCogeRegistro(NS_SQL, founder, "email"), "Reenvío de la contraseña", "Debido a la pérdida de la contraseña de tu canal, se te ha generado otra clave totalmetne segura.\r\n"
		"A partir de ahora, la clave de tu canal es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, chanserv->hmod->nick, conf_set->red);
	Responde(cl, CLI(chanserv), "Se generado y enviado otra contraseña al email del founder de \00312%s\003.", param[1]);
	Free(founder);
	EOI(chanserv, 14);
	return 0;
}
BOTFUNC(CSSuspender)
{
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal motivo");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
	SQLInserta(CS_SQL, param[1], "suspend", motivo);
	ircsprintf(buf, "Suspendido: %s", motivo);
	CSMarca(cl, param[1], buf);
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido suspendido.", param[1]);
	EOI(chanserv, 15);
	return 0;
}
BOTFUNC(CSLiberar)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está suspendido.");
		return 1;
	}
	SQLInserta(CS_SQL, param[1], "suspend", "");
	CSMarca(cl, param[1], "Suspenso levantado.");
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su suspenso.", param[1]);
	EOI(chanserv, 16);
	return 0;
}
BOTFUNC(CSForbid)
{
	Canal *cn;
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal motivo");
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
	if ((cn = BuscaCanal(param[1])) && cn->miembros)
	{
		Cliente **al = NULL;
		int i;
		al = CSEmpaquetaClientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_KICK)(al[i], CLI(chanserv), cn, "Canal \2PROHIBIDO\2: %s", motivo);
		ircfree(al);
	}
	SQLInserta(CS_FORBIDS, param[1], "motivo", motivo);
	ircsprintf(buf, "Prohibido: %s", motivo);
	CSMarca(cl, param[1], buf);
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido prohibido.", param[1]);
	EOI(chanserv, 17);
	return 0;
}
BOTFUNC(CSUnforbid)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!IsChanForbid(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está prohibido.");
		return 1;
	}
	SQLBorra(CS_FORBIDS, param[1]);
	CSMarca(cl, param[1], "Prohibición levantada.");
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su prohibición.", param[1]);
	EOI(chanserv, 18);
	return 0;
}
BOTFUNC(CSBlock)
{
	Canal *cn;
	Cliente **al;
	int i;
	MallaCliente *mcl;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!(cn = BuscaCanal(param[1])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal está vacío.");
		return 1;
	}
	for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
	{
		al = CSEmpaquetaClientes(cn, mcl->malla, ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", mcl->flag, TRIO(al[i]));
		ircfree(al);
	}
	ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+iml 1");
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido bloqueado.", param[1]);
	/* aun asi podrán opearse si tienen nivel, se supone que este comando lo utilizan los operadores 
	   y estan supervisando el canal, para que si alguno se opea se le killee, o simplemente por hacer 
	   la gracia */
	EOI(chanserv, 19);
	return 0;
}
BOTFUNC(CSRegister)
{
	if (params == 2) /* registro de una petición de canal */
	{
		if (!IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal pass descripción tokens");
			return 1;
		}
		if (!IsChanPReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está en petición de registro.");
			return 1;
		}
		goto registrar;
	}
	else if (params >= 4) /* peticion de registro */
	{
		SQLRes res;
		SQLRow row;
		int i;
		char *desc;
		if (IsChanReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya está registrado.");
			return 1;
		}
		if (*param[1] != '#')
		{
			Responde(cl, CLI(chanserv), CS_ERR_SNTX, "El nombre de canal debe empezar por #.");
			return 1;
		}
		if (!IsOper(cl))
		{
			char *n_c;
			if ((atol(SQLCogeRegistro(NS_SQL, parv[0], "reg")) + 86400 * chanserv->antig) > time(0))
			{
				char buf[512];
				ircsprintf(buf, "Tu nick debe tener una antigüedad de como mínimo %i días.", chanserv->antig);
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				return 1;
			}
			n_c = SQLEscapa(strtolower(parv[0]));
			if ((res = SQLQuery("SELECT nick from %s%s where LOWER(nick)='%s'", PREFIJO, CS_TOK, n_c)))
			{
				char buf[512];
				SQLFreeRes(res);
				ircsprintf(buf, "Ya has efectuado alguna operación referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv->vigencia);
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				Free(n_c);
				return 1;
			}
			Free(n_c);
			if (params < (4 + chanserv->necesarios))
			{
				char buf[512];
				ircsprintf(buf, "Se necesitan %i tokens.", chanserv->necesarios);
				Responde(cl, CLI(chanserv), CS_ERR_SNTX, buf);
				return 1;
			}
			/* comprobamos los tokens */
			for (i = 0; i < chanserv->necesarios; i++)
			{
				char buf[512];
				char *tok = param[params - i - 1];
				tokbuf[0] = '\0';
				if (strstr(tokbuf, tok)) /* ya había sido usado */
				{
					ircsprintf(buf, "El token %s está repetido.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				if (!(res = SQLQuery("SELECT item,nick,hora from %s%s where LOWER(item)='%s'", PREFIJO, CS_TOK, strtolower(tok))))
				{
					ircsprintf(buf, "El token %s no es válido o ha caducado.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				row = SQLFetchRow(res);
				SQLFreeRes(res);
				if (BadPtr(row[1]) || BadPtr(row[2]))
				{
					ircsprintf(buf, "Existe un error en el token %s. Posiblemente haya sido robado.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				if ((atol(row[2]) + 3600 * chanserv->vigencia) < time(0))
				{
					ircsprintf(buf, "El token %s ha caducado.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				strlcat(tokbuf, tok, sizeof(tokbuf));
				strlcat(tokbuf, " ", sizeof(tokbuf));
			}
			desc = Unifica(param, params, 3, params - chanserv->necesarios - 1);
		}
		else
			desc = Unifica(param, params, 3, -1);
		SQLInserta(CS_SQL, param[1], "founder", parv[0]);
		SQLInserta(CS_SQL, param[1], "pass", MDString(param[2], 0));
		SQLInserta(CS_SQL, param[1], "descripcion", desc);
		if (IsOper(cl))
		{
			Canal *cn;
			registrar:
			cn = BuscaCanal(param[1]);
			SQLInserta(CS_SQL, param[1], "registro", "%i", time(0));
			if (cn)
			{
				if (RedOverride)
					EntraBot(CLI(chanserv), cn->nombre);
				if (cmodreg)
					ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%c", cmodreg->flag);
				ProtFunc(P_TOPIC)(CLI(chanserv), cn, "El canal ha sido registrado.");
			}
			LlamaSenyal(CS_SIGN_REG, 1, param[1]);
			Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido registrado.", param[1]);
		}
		else
			Responde(cl, CLI(chanserv), "Solicitud aceptada. El canal \00312%s\003 está pendiente de aprobación.", param[1]);
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, IsOper(cl) ? "#canal [pass descripción]" : "#canal pass descripción tokens");
		return 1;
	}
	EOI(chanserv, 20);
	return 0;
}
BOTFUNC(CSToken)
{
	SQLRes res;
	SQLRow row;
	char *n_c;
	int libres = 25, i; /* siempre tendremos 25 tokens libres */
	if ((atol(SQLCogeRegistro(NS_SQL, parv[0], "reg")) + 86400 * chanserv->antig) > time(0))
	{
		char buf[512];
		ircsprintf(buf, "Tu nick debe tener una antigüedad de como mínimo %i días.", chanserv->antig);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}
	n_c = SQLEscapa(strtolower(parv[0]));
	if ((res = SQLQuery("SELECT nick from %s%s where LOWER(nick)='%s'", PREFIJO, CS_TOK, n_c)))
	{
		char buf[512];
		SQLFreeRes(res);
		ircsprintf(buf, "Ya has efectuado alguna operación referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv->vigencia);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		Free(n_c);
		return 1;
	}
	Free(n_c);
	res = SQLQuery("SELECT * from %s%s where hora='0'", PREFIJO, CS_TOK);
	if (res)
		libres = 25 - (int)SQLNumRows(res); /* ya que sera ocupada */
	for (i = 0; i < libres; i++)
	{
		char buf[512];
		ircsprintf(buf, "%lu", rand());
		SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, CS_TOK, CifraNick(buf, buf));
	}
	if (res)
		SQLFreeRes(res);
	res = SQLQuery("SELECT item from %s%s where hora='0'", PREFIJO, CS_TOK);
	row = SQLFetchRow(res);
	SQLInserta(CS_TOK, row[0], "hora", "%i", time(0));
	SQLInserta(CS_TOK, row[0], "nick", parv[0]);
	Responde(cl, CLI(chanserv), "Tu token es \00312%s\003. Guarda este token ya que será necesario para futuras operaciones y no podrás pedir otro hasta dentro de unas horas.", row[0]);
	SQLFreeRes(res);
	EOI(chanserv, 21);
	return 0;
}
BOTFUNC(CSMarcas)
{
	char *marcas;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, fc->com, "#canal [comentario]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (params < 3)
	{
		if ((marcas = SQLCogeRegistro(CS_SQL, param[1], "marcas")))
		{
			char *tok;
			Responde(cl, CLI(chanserv), "Historial de \00312%s", param[1]);
			for (tok = strtok(marcas, "\t"); tok; tok = strtok(NULL, "\t"))
				Responde(cl, CLI(chanserv), tok);
		}
		else
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El historial está vacío.");
			return 1;
		}
	}
	else
	{
		CSMarca(cl, param[1], Unifica(param, params, 2, -1));
		Responde(cl, CLI(chanserv), "Marca añadida al historial de \00312%s", param[1]);
	}
	EOI(chanserv, 22);
	return 0;
}
int CSCmdMode(Cliente *cl, Canal *cn, char *mods[], int max)
{
	modebuf[0] = modebuf[1] = parabuf[0] = '\0';
	if (IsChanReg(cn->nombre))
	{
		int opts;
		char *modlock, *k;
		Cliente *al;
		opts = atoi(SQLCogeRegistro(CS_SQL, cn->nombre, "opts"));
		modlock = SQLCogeRegistro(CS_SQL, cn->nombre, "modos");
		if (opts & CS_OPT_RMOD && modlock)
		{
			strlcat(modebuf, strtok(modlock, " "), sizeof(modebuf));
			if ((modlock = strtok(NULL, " ")))
			{
				strlcat(parabuf, modlock, sizeof(parabuf));
				strlcat(parabuf, " ", sizeof(parabuf));
			}
			/* buscamos el -k */
			k = strchr(modebuf, '-');
			if (k && strchr(k, 'k'))
			{
				MallaParam *mpm;
				if ((mpm = BuscaMallaParam(cn, 'k')) && mpm->param)
					ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-k %s", mods[max-1]); /* siempre está en el último */
			}
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", modebuf, parabuf);
			modebuf[0] = parabuf[0] = '\0';
		}
		if ((opts & CS_OPT_SOP) && !EsServidor(cl))
		{
			int pars = 1;
			char *modos, f = ADD;
			modos = mods[0];
			modebuf[0] = '-';
			modebuf[1] = '\0';
			while (!BadPtr(modos))
			{
				if (*modos == '+')
					f = ADD;
				else if (*modos == '-')
					f = DEL;
				else if (strchr(protocolo->modcl, *modos))
				{
					if (f == ADD)
					{
						if (!CSTieneAuto(mods[pars], cn->nombre, *modos) && (al = BuscaCliente(mods[pars])) && !EsBot(al))
						{
							chrcat(modebuf, *modos);
							strlcat(parabuf, TRIO(al), sizeof(parabuf));
							strlcat(parabuf, " ", sizeof(parabuf));
						}
					}
					pars++;
				}
				else if (strchr(protocolo->modmk, *modos) || strchr(protocolo->modpm1, *modos))
					pars++;
				else if (strchr(protocolo->modpm2, *modos) && f == ADD)
					pars++;
				modos++;
			}
		}
	}
	if (modebuf[0] && modebuf[1])
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", modebuf, parabuf);
	return 0;
}
int CSCmdTopic(Cliente *cl, Canal *cn, char *topic)
{
	int opts;
	if (IsChanReg(cn->nombre))
	{
		char *topic;
		opts = atoi(SQLCogeRegistro(CS_SQL, cn->nombre, "opts"));
		topic = SQLCogeRegistro(CS_SQL, cn->nombre, "topic");
		if (opts & CS_OPT_KTOP)
		{
			if (topic && strcmp(cn->topic, topic))
				ProtFunc(P_TOPIC)(CLI(chanserv), cn, topic);
		}
		else if (opts & CS_OPT_RTOP)
		{
			SQLInserta(CS_SQL, cn->nombre, "topic", topic);
			SQLInserta(CS_SQL, cn->nombre, "ntopic", cl->nombre);
		}
	}	
	return 0;
}
int CSCmdKick(Cliente *cl, Cliente *al, Canal *cn, char *motivo)
{
	if (IsChanReg(cn->nombre))
	{
		if (CSTieneNivel(al->nombre, cn->nombre, CS_LEV_REV) && strcasecmp(cl->nombre, SQLCogeRegistro(CS_SQL, cn->nombre, "founder")))
			ProtFunc(P_KICK)(cl, CLI(chanserv), cn, "KICK revenge!");
	}	
	return 0;
}
int CSCmdJoin(Cliente *cl, Canal *cn)
{
	char *forb;
	SQLRes res;
	SQLRow row;
	int opts, i, max;
	char *entry, *modos, *topic, *susp;
	if (!IsOper(cl) && (forb = IsChanForbid(cn->nombre)))
	{
		if (ProtFunc(P_PART_USUARIO_REMOTO))
		{
			ProtFunc(P_PART_USUARIO_REMOTO)(cl, cn, "Este canal está \2PROHIBIDO\2: %s", forb);
			ProtFunc(P_NOTICE)(cl, CLI(chanserv), "Este canal está \2PROHIBIDO\2: %s", forb);
		}
		return 0;
	}
	if (IsChanReg(cn->nombre))
	{
		res = SQLQuery("SELECT opts,entry,modos,topic from %s%s where LOWER(item)='%s'", PREFIJO, CS_SQL, strtolower(cn->nombre));
		row = SQLFetchRow(res);
		opts = atoi(row[0]);
		entry = strdup(row[1]);
		modos = strdup(row[2]);
		topic = strdup(row[3]);
		SQLFreeRes(res);
		if ((susp = IsChanSuspend(cn->nombre)))
		{
			ProtFunc(P_NOTICE)(cl, CLI(chanserv), "Este canal está suspendido: %s", susp);
			if (!IsOper(cl))
				return 0;
		}
		if ((opts & CS_OPT_SEC) && !IsPreo(cl) && (!CSTieneNivel(cl->nombre, cn->nombre, 0L) || !IsId(cl)))
		{
			if (ProtFunc(P_PART_USUARIO_REMOTO))
				ProtFunc(P_PART_USUARIO_REMOTO)(cl, cn, NULL);
			return 0;
		}
		if ((row = CSEsAkick(cn->nombre, cl->mask)))
		{
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+b %s", TipoMascara(cl->mask, chanserv->bantype));
			ProtFunc(P_KICK)(cl, CLI(chanserv), cn, "%s (%s)", row[2], row[3]);
			return 0;
		}
		buf[0] = tokbuf[0] = '\0';
		if (CSEsFundador(cl, cn->nombre) || CSEsFundador_cache(cl, cn->nombre))
			strlcpy(buf, protocolo->modcl, sizeof(buf));
		else
		{
			if (IsId(cl) && (res = CSEsAccess(cn->nombre, cl->nombre)))
			{
				row = SQLFetchRow(res);
				if (!BadPtr(row[3]))
					strlcpy(buf, row[3], sizeof(buf));
				SQLFreeRes(res);
			}
		}
		max = strlen(buf);
		for (i = 0; i < max; i++)
		{
			strlcat(tokbuf, cl->nombre, sizeof(tokbuf));
			strlcat(tokbuf, " ", sizeof(tokbuf));
		}
		if (CSTieneNivel(cl->nombre, cn->nombre, 0L))
			SQLInserta(CS_SQL, cn->nombre, "ultimo", "%lu", time(0));
		if (cn->miembros == 1)
		{
			if (cmodreg)
				chrcat(buf, cmodreg->flag);
			if (opts & CS_OPT_RMOD)
			{
				char *mod;
				if ((mod = strtok(modos, strchr(modos, '-') ? "-": " ")))
				{
					strlcat(buf, *mod == '+' ? mod+1 : mod, sizeof(buf));
					if ((mod = strtok(NULL, "\0")))
					{
						char *p;
						if ((p = strchr(mod, ' '))) 
							strlcat(tokbuf, p+1, sizeof(tokbuf));
						else
							strlcat(tokbuf, mod, sizeof(tokbuf));
					}
				}
			}
			if (opts & CS_OPT_RTOP)
				ProtFunc(P_TOPIC)(CLI(chanserv), cn, topic);
		}
		if (buf[0])
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%s %s", buf, tokbuf);
		if (!BadPtr(entry))
			ProtFunc(P_NOTICE)(cl, CLI(chanserv), entry);
		Free(entry);
		Free(modos);
		Free(topic);
	}
	return 0;
}
int CSSigSQL()
{
	if (!SQLEsTabla(CS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"n SERIAL, "
  			"item varchar(255), "
			"founder varchar(255), "
  			"pass varchar(255), "
  			"descripcion text, "
  			"modos varchar(255) default '+nt', "
  			"opts int4 default '0', "
  			"topic varchar(255) default 'El canal ha sido registrado.', "
  			"entry varchar(255) default 'El canal ha sido registrado.', "
  			"registro int4 default '0', "
  			"ultimo int4 default '0', "
  			"ntopic text, "
  			"limite int2 default '5', "
  			"bjoins int4 default '0', "
  			"suspend text, "
  			"url varchar(255), "
  			"email varchar(255), "
  			"marcas text, "
  			"KEY item (item) "
			");", PREFIJO, CS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_SQL);
	}
	else
	{
		if (!SQLEsCampo(CS_SQL, "marcas"))
			SQLQuery("ALTER TABLE %s%s ADD COLUMN marcas text", PREFIJO, CS_SQL);
		if (!SQLEsCampo(CS_SQL, "suspend"))
			SQLQuery("ALTER TABLE %s%s ADD COLUMN suspend text", PREFIJO, CS_SQL);
		SQLQuery("ALTER TABLE %s%s CHANGE item item VARCHAR( 255 )", PREFIJO, CS_SQL);
	}
	//SQLQuery("ALTER TABLE %s%s ADD PRIMARY KEY(n)", PREFIJO, CS_SQL);
	SQLQuery("ALTER TABLE %s%s DROP INDEX item", PREFIJO, CS_SQL);
	SQLQuery("ALTER TABLE %s%s ADD INDEX ( item ) ", PREFIJO, CS_SQL);
	if (!SQLEsTabla(CS_TOK))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"item varchar(255) default NULL, "
			"nick varchar(255) default NULL, "
			"hora int4 default '0', "
			"KEY item (item) "
			");", PREFIJO, CS_TOK);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_TOK);
	}
	if (!SQLEsTabla(CS_FORBIDS))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"motivo varchar(255) default NULL, "
  			"KEY item (item) "
			");", PREFIJO, CS_FORBIDS);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_FORBIDS);
	}
	if (!SQLEsTabla(CS_ACCESS))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"canal varchar(255) default NULL, "
  			"nick varchar(255) default NULL, "
  			"nivel int8 default '0', "
  			"automodos varchar(32) default NULL, "
  			"KEY canal (canal) "
			");", PREFIJO, CS_ACCESS);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_ACCESS);
		else
		{
			SQLRes res;
			SQLRow row;
			if ((res = SQLQuery("SELECT item,accesos FROM %s%s", PREFIJO, CS_SQL)))
			{
				while ((row = SQLFetchRow(res)))
				{
					if (!BadPtr(row[1]))
					{
						char *tok, *c, *lev = NULL, *autof = NULL;
						for (tok = strtok(row[1], ":"); tok; tok = strtok(NULL, ":"))
						{
							lev = strtok(NULL, " ");
							if (IsDigit(*lev))
							{
								if ((c = strchr(lev, ':')))
								{
									*c++ = '\0';
									autof = c;
								}
							}
							else
							{
								autof = lev;
								lev = NULL;
							}
							if (lev && autof)
								SQLQuery("INSERT INTO %s%s (canal,nick,nivel,automodos) values ('%s','%s','%s','%s')", 
									PREFIJO, CS_ACCESS,
									row[0], tok,
									lev, autof);
							else if (autof)
								SQLQuery("INSERT INTO %s%s (canal,nick,automodos) values ('%s','%s','%s')", 
									PREFIJO, CS_ACCESS,
									row[0], tok, autof);
							else if (lev)
								SQLQuery("INSERT INTO %s%s (canal,nick,nivel) values ('%s','%s','%s')", 
									PREFIJO, CS_ACCESS,
									row[0], tok, lev);
						}
					}
				}
				SQLFreeRes(res);
			}
			SQLQuery("ALTER TABLE %s%s DROP accesos", PREFIJO, CS_SQL);
		}
	}
	//else
	//	SQLQuery("ALTER TABLE %s%s CHANGE automodos automodos VARCHAR(32) NOT NULL", PREFIJO, CS_ACCESS);
	if (!SQLEsTabla(CS_AKICKS))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"canal varchar(255) default NULL, "
  			"mascara varchar(255) default NULL, "
  			"motivo varchar(255) default '0', "
  			"autor varchar(64) default NULL, "
  			"KEY canal (canal) "
			");", PREFIJO, CS_AKICKS);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_AKICKS);
		else
		{
			SQLRes res;
			SQLRow row;
			if ((res = SQLQuery("SELECT item,akick from %s%s", PREFIJO, CS_SQL)))
			{
				while ((row = SQLFetchRow(res)))
				{
					if (!BadPtr(row[1]))
					{
						char *tok, *mo, *au, *m_c;
						for (tok = strtok(row[1], "\1"); tok; tok = strtok(NULL, "\1"))
						{
							mo = strtok(NULL, "\1");
							m_c = SQLEscapa(mo);
							au = strtok(NULL, "\t");
							SQLQuery("INSERT INTO %s%s (canal,mascara,motivo,autor) values ('%s','%s','%s','%s')", 
								PREFIJO, CS_AKICKS,
								row[0], tok,
								m_c, au);
							Free(m_c);
						}
					}
				}
			}
			SQLFreeRes(res);
			SQLQuery("ALTER TABLE %s%s DROP akick", PREFIJO, CS_SQL);
		}
	}
	SQLCargaTablas();
	cmodreg = BuscaModoProtocolo(CHMODE_RGSTR, protocolo->cmodos);
	return 1;
}
int CSSigEOS()
{
	/* metemos los bots en los canales que lo hayan soliticado */
	SQLRes res;
	SQLRow row;
	int bjoins;
	Cliente *bl;
	if ((res = SQLQuery("SELECT item,bjoins from %s%s where bjoins !='0'", PREFIJO, CS_SQL)))
	{
		while ((row = SQLFetchRow(res)))
		{
			Modulo *ex;
			bjoins = atoi(row[1]);
			for (ex = modulos; ex; ex = ex->sig)
			{
				if ((bjoins & ex->id) && !ModuloEsResidente(ex, row[0]))
				{
					bl = BuscaCliente(ex->nick);
					if (bl)
						EntraBot(bl, row[0]);
				}
			}
		}
		SQLFreeRes(res);
	}
	return 0;
}
int CSSigPreNick(Cliente *cl, char *nuevo)
{
	BorraCache(CACHE_FUNDADORES, cl->nombre, chanserv->hmod->id);
	return 0;
}
int CSBaja(char *canal, int opt)
{
	Canal *an;
	char *c_c;
	if (!opt && atoi(SQLCogeRegistro(CS_SQL, canal, "opts")) & CS_OPT_NODROP)
		return 1;
	if ((an = BuscaCanal(canal)))
	{
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), an, "-r");
		if (RedOverride)
			SacaBot(CLI(chanserv), canal, NULL);
	}
	c_c = SQLEscapa(strtolower(canal));
	SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_ACCESS, c_c);
	SQLQuery("DELETE FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_AKICKS, c_c);
	LlamaSenyal(CS_SIGN_DROP, 1, canal);
	SQLBorra(CS_SQL, canal);
	Free(c_c);
	return 0;
}
SQLRow CSEsAkick(char *canal, char *mascara)
{
	char *c_c;
	SQLRes res;
	SQLRow row;
	c_c = SQLEscapa(strtolower(canal));
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s'", PREFIJO, CS_AKICKS, c_c)))
	{
		while ((row = SQLFetchRow(res)))
		{
			if (!match(row[1], mascara))
			{
				SQLFreeRes(res);
				Free(c_c);
				return row;
			}
		}
	}
	SQLFreeRes(res);
	Free(c_c);
	return NULL;
}
SQLRes CSEsAccess(char *canal, char *nick)
{
	char *c_c, *n_c;
	static SQLRes res;
	c_c = SQLEscapa(strtolower(canal));
	n_c = SQLEscapa(strtolower(nick));
	res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(canal)='%s' AND LOWER(nick)='%s'", PREFIJO, CS_ACCESS, c_c, n_c);
	Free(c_c);
	Free(n_c);
	return res;
}
int CSDropanick(char *nick)
{
	char *n_c;
	n_c = SQLEscapa(strtolower(nick));
	SQLQuery("DELETE FROM %s%s WHERE LOWER(nick)='%s'", PREFIJO, CS_ACCESS, n_c);
	Free(n_c);
	return 0;
}
u_long CSTieneNivel(char *nick, char *canal, u_long flag)
{
	Cliente *al;
	SQLRes res;
	SQLRow row;
	al = BuscaCliente(nick);
	if ((!IsOper(al) && IsChanSuspend(canal)) || !IsId(al))
		return 0L;
	if (CSEsFundador(al, canal) || CSEsFundador_cache(al, canal))
		return CS_LEV_ALL;
	if ((res = CSEsAccess(canal, nick)))
	{
		u_long nivel = 0L;
		row = SQLFetchRow(res);
		nivel = atoul(row[2]);
		if (IsPreo(al))
			nivel |= (CS_LEV_ALL & ~CS_LEV_MOD);
		if (flag)
			return (nivel & flag);
		else
			return nivel;
	}
	if (IsPreo(al))
		return (CS_LEV_ALL & ~CS_LEV_MOD);
	return 0L;
}
int CSTieneAuto(char *nick, char *canal, char autof)
{
	Cliente *al;
	SQLRes res;
	SQLRow row;
	al = BuscaCliente(nick);
	if ((!IsOper(al) && IsChanSuspend(canal)) || !IsId(al))
		return 0;
	if (CSEsFundador(al, canal) || CSEsFundador_cache(al, canal))
		return 1;
	if ((res = CSEsAccess(canal, nick)) && (row = SQLFetchRow(res)))
	{
		if (row[3] && strchr(row[3], autof))
			return 1;
	}
	return 0;
}
ProcFunc(CSDropachans)
{
	SQLRes res;
	SQLRow row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = SQLQuery("SELECT item from %s%s where (ultimo < %i AND ultimo !='0') OR LOWER(founder)='%s' LIMIT 30 OFFSET %i ", PREFIJO, CS_SQL, time(0) - 86400 * chanserv->autodrop, CLI(chanserv) && CLI(chanserv)->nombre && SockIrcd ? strtolower(CLI(chanserv)->nombre) : "", proc->proc)) || !SQLNumRows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			SQLQuery("DELETE from %s%s where hora < %i AND hora !='0'", PREFIJO, CS_TOK, time(0) - 3600 * chanserv->vigencia);
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			if (atoi(row[1]) + 86400 * chanserv->autodrop < time(0))
				CSBaja(row[0], 0);
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
int ChanReg(char *canal)
{
	char *res;
	if (!canal)
		return 0;
	res = SQLCogeRegistro(CS_SQL, canal, "registro");
	if (res)
	{
		if (strcmp(res, "0"))
			return 1; /* registrado */
		else
			return 2; /* peticion */
	}
	return 0;
}
char *IsChanSuspend(char *canal)
{
	char *motivo;
	if (!canal)
		return NULL;
	if ((motivo = SQLCogeRegistro(CS_SQL, canal, "suspend")))
		return motivo;
	return NULL;
}
char *IsChanForbid(char *canal)
{
	char *motivo;
	if (!canal)
		return NULL;
	if ((motivo = SQLCogeRegistro(CS_FORBIDS, canal, "motivo")))
		return motivo;
	return NULL;
}
int CSSigSynch()
{
	SQLRes res;
	SQLRow row;
	if (RedOverride)
	{
		if ((res = SQLQuery("SELECT item from %s%s", PREFIJO, CS_SQL)))
		{
			while ((row = SQLFetchRow(res)))
				EntraBot(CLI(chanserv), row[0]);
			SQLFreeRes(res);
		}
	}
	return 0;
}
int CSSigStartUp()
{
	
	return 0;
}
