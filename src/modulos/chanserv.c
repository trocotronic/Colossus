/*
 * $Id: chanserv.c,v 1.12 2004-12-31 12:27:59 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"

ChanServ chanserv;
Hash csregs[UMAX];
Hash akicks[CHMAX];

static int chanserv_help		(Cliente *, char *[], int, char *[], int);
static int chanserv_deauth		(Cliente *, char *[], int, char *[], int);
static int chanserv_drop		(Cliente *, char *[], int, char *[], int);
static int chanserv_identify	(Cliente *, char *[], int, char *[], int);
static int chanserv_info		(Cliente *, char *[], int, char *[], int);
static int chanserv_invite		(Cliente *, char *[], int, char *[], int);
static int chanserv_modos		(Cliente *, char *[], int, char *[], int);
static int chanserv_clear		(Cliente *, char *[], int, char *[], int);
static int chanserv_set		(Cliente *, char *[], int, char *[], int);
static int chanserv_akick		(Cliente *, char *[], int, char *[], int);
static int chanserv_access		(Cliente *, char *[], int, char *[], int);
static int chanserv_list		(Cliente *, char *[], int, char *[], int);
static int chanserv_jb		(Cliente *, char *[], int, char *[], int);
static int chanserv_sendpass	(Cliente *, char *[], int, char *[], int);
static int chanserv_suspender	(Cliente *, char *[], int, char *[], int);
static int chanserv_liberar		(Cliente *, char *[], int, char *[], int);
static int chanserv_forbid		(Cliente *, char *[], int, char *[], int);
static int chanserv_unforbid	(Cliente *, char *[], int, char *[], int);
static int chanserv_block		(Cliente *, char *[], int, char *[], int);
static int chanserv_register 	(Cliente *, char *[], int, char *[], int);
static int chanserv_token		(Cliente *, char *[], int, char *[], int);
#ifdef UDB
static int chanserv_migrar		(Cliente *, char *[], int, char *[], int);
static int chanserv_demigrar	(Cliente *, char *[], int, char *[], int);
static int chanserv_proteger	(Cliente *, char *[], int, char *[], int);
#endif

int chanserv_sig_mysql	();
int chanserv_sig_eos	();
int chanserv_sig_synch	();

int chanserv_dropachans(Proc *);
int chanserv_dropanick(char *);
int chanserv_baja(char *, char);

int chanserv_mode(Cliente *, Canal *, char *[], int);
int chanserv_join(Cliente *, Canal *);
int chanserv_kick(Cliente *, Cliente *, Canal *, char *);
int chanserv_topic(Cliente *, Canal *, char *);

void chanserv_carga_registros(void);
void chanserv_carga_akicks(void);
void inserta_cregistro(char *, char *, u_long);
void borra_cregistro(char *, char *);
Akick *inserta_akick(char *, char *, char *, char *);
void borra_akick(char *, char *);
Akick *busca_akicks(char *);
int es_kick(char *, char *);
int es_cregistro(char *, char *);
int test(Conf *, int *);
void set(Conf *, Modulo *);

static bCom chanserv_coms[] = {
	{ "help" , chanserv_help , TODOS } ,
	{ "deauth" , chanserv_deauth , USERS } ,
	{ "drop" , chanserv_drop , OPERS } ,
	{ "identify" , chanserv_identify , USERS } ,
	{ "info" , chanserv_info , TODOS } ,
	{ "invite" , chanserv_invite , USERS } ,
	{ "op" , chanserv_modos , USERS } ,
	{ "deop" , chanserv_modos , USERS } ,
	{ "half" , chanserv_modos , USERS } ,
	{ "dehalf" , chanserv_modos , USERS } ,
	{ "voice" , chanserv_modos , USERS } ,
	{ "devoice" , chanserv_modos , USERS } ,
	{ "kick" , chanserv_modos , USERS } ,
	{ "ban" , chanserv_modos , USERS } ,
	{ "unban" , chanserv_modos , USERS } ,
	{ "clear" , chanserv_clear , USERS } ,
	{ "set" , chanserv_set , USERS } ,
	{ "akick" , chanserv_akick , USERS } ,
	{ "access" , chanserv_access , USERS } ,
	{ "list" , chanserv_list , TODOS } ,
	{ "join" , chanserv_jb , USERS } ,
	{ "sendpass" , chanserv_sendpass , PREOS } ,
	{ "suspender" , chanserv_suspender , OPERS } ,
	{ "liberar" , chanserv_liberar , OPERS } ,
	{ "forbid" , chanserv_forbid , ADMINS } ,
	{ "unforbid" , chanserv_unforbid , ADMINS } ,
	{ "block" , chanserv_block , PREOS } ,
	{ "register" , chanserv_register , USERS } ,
	{ "token" , chanserv_token , USERS } ,
#ifdef UDB
	{ "migrar" , chanserv_migrar , USERS } ,
	{ "demigrar" , chanserv_demigrar , USERS } ,
	{ "proteger" , chanserv_proteger , USERS } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};

__declspec(dllexport) mTab cFlags[] = {
	{ CS_LEV_SET , 's' } ,
	{ CS_LEV_EDT , 'e' } ,
	{ CS_LEV_LIS , 'l' } ,
	{ CS_LEV_AAD , 'a' } ,
	{ CS_LEV_AOP , 'o' } ,
	{ CS_LEV_AHA , 'h' } ,
	{ CS_LEV_AVO , 'v' } ,
	{ CS_LEV_RMO , 'r' } ,
	{ CS_LEV_RES , 'c' } ,
	{ CS_LEV_ACK , 'k' } ,
	{ CS_LEV_INV , 'i' } ,
	{ CS_LEV_JOB , 'j' } ,
	{ CS_LEV_REV , 'g' } ,
	{ CS_LEV_MEM , 'm' } ,
	{ 0x0 , 0x0 }
};

ModInfo info = {
	"ChanServ" ,
	0.8 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" 
};
	
int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		conferror("[%s] Falta especificar archivo de configuración para %s", mod->archivo, info.nombre);
		errores++;
	}
	else
	{
		if (parseconf(mod->config, &modulo, 1))
		{
			conferror("[%s] Hay errores en la configuración de %s", mod->archivo, info.nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
			{
				if (!test(modulo.seccion[0], &errores))
					set(modulo.seccion[0], mod);
				else
				{
					conferror("[%s] La configuración de %s no ha pasado el test", mod->archivo, info.nombre);
					errores++;
				}
			}
			else
			{
				conferror("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
				errores++;
			}
		}
	}
	return errores;
}
int descarga()
{
	borra_senyal(SIGN_MODE, chanserv_mode);
	borra_senyal(SIGN_TOPIC, chanserv_topic);
	borra_senyal(SIGN_JOIN, chanserv_join);
	borra_senyal(SIGN_KICK, chanserv_kick);
	borra_senyal(SIGN_MYSQL, chanserv_sig_mysql);
	borra_senyal(NS_SIGN_DROP, chanserv_dropanick);
	borra_senyal(SIGN_EOS, chanserv_sig_eos);
	borra_senyal(SIGN_SYNCH, chanserv_sig_synch);
	return 0;
}
int test(Conf *config, int *errores)
{
	return 0;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *cs;
	chanserv.autodrop = 15;
	chanserv.maxlist = 30;
	chanserv.bantype = 3;
	chanserv.tokform = strdup("##########");
	chanserv.vigencia = 24;
	chanserv.necesarios = 10;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "sid"))
			chanserv.opts |= CS_SID;
#ifdef UDB
		if (!strcmp(config->seccion[i]->item, "automigrar"))
			chanserv.opts |= CS_AUTOMIGRAR;
#endif			
		if (!strcmp(config->seccion[i]->item, "autodrop"))
			chanserv.autodrop = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "maxlist"))
			chanserv.maxlist = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "bantype"))
			chanserv.bantype = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				cs = &chanserv_coms[0];
				while (cs->com != 0x0)
				{
					if (!strcasecmp(cs->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = cs;
						break;
					}
					cs++;
				}
				if (cs->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
		if (!strcmp(config->seccion[i]->item, "tokens"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "formato"))
					chanserv.tokform = strdup(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "vigencia"))
					chanserv.vigencia = atoi(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "necesarios"))
					chanserv.necesarios = atoi(config->seccion[i]->seccion[p]->data);
			}
		}
	}
	inserta_senyal(SIGN_MODE, chanserv_mode);
	inserta_senyal(SIGN_TOPIC, chanserv_topic);
	inserta_senyal(SIGN_JOIN, chanserv_join);
	inserta_senyal(SIGN_KICK, chanserv_kick);
	inserta_senyal(SIGN_MYSQL, chanserv_sig_mysql);
	inserta_senyal(NS_SIGN_DROP, chanserv_dropanick);
	inserta_senyal(SIGN_EOS, chanserv_sig_eos);
	inserta_senyal(SIGN_SYNCH, chanserv_sig_synch);
	proc(chanserv_dropachans);
	bot_set(chanserv);
}
int es_fundador(Cliente *al, char *canal)
{
	char *fun;
	if (!al)
		return 0;
	if (!(fun = _mysql_get_registro(CS_MYSQL, canal, "founder")))
		return 0;
	if (!strcasecmp(fun, al->nombre))
		return 1;
	return 0;
}
int es_residente(Modulo *mod, char *canal)
{
	char *aux;
	strcpy(tokbuf, mod->residente);
	for (aux = strtok(tokbuf, ","); aux; aux = strtok(NULL, ","))
	{
		if (!strcasecmp(canal, aux))
			return 1;
	}
	return 0;
}
u_long borra_acceso(char *usuario, char *canal)
{
	char *registros, *tok;
	u_long prev = 0L;
	if ((registros = _mysql_get_registro(CS_MYSQL, canal, "regs")))
	{
		buf[0] = '\0';
		for (tok = strtok(registros, ":"); tok; tok = strtok(NULL, ":"))
		{
			if (!strcasecmp(tok, usuario))
			{
				prev = atol(strtok(NULL, " "));
				continue;
			}
			strcat(buf, tok);
			strcat(buf, ":");
			strcat(buf, strtok(NULL, " "));
			strcat(buf, " ");
		}
		borra_cregistro(usuario, canal);
		_mysql_add(CS_MYSQL, canal, "regs", buf);
	}
	return prev;
}
void borra_akick_bd(char *nick, char *canal)
{
	char *registros, *tok;
	if ((registros = _mysql_get_registro(CS_MYSQL, canal, "akick")))
	{
		buf[0] = '\0';
		for (tok = strtok(registros, "\1"); tok; tok = strtok(NULL, "\1"))
		{
			if (!strcasecmp(tok, nick))
			{
				strtok(NULL, "\1");
				strtok(NULL, "\t");
				continue;
			}
			strcat(buf, tok);
			strcat(buf, "\1");
			strcat(buf, strtok(NULL, "\1"));
			strcat(buf, "\1");
			strcat(buf, strtok(NULL, "\t"));
			strcat(buf, "\t");
		}
		borra_akick(nick, canal);
		_mysql_add(CS_MYSQL, canal, "akick", buf);
	}
}
void inserta_akick_bd(char *nick, char *canal, char *emisor, char *motivo)
{
	Akick *aux;
	int i;
	aux = inserta_akick(nick, canal, emisor, motivo);
	buf[0] = '\0';
	sprintf_irc(buf, "%s\1%s\1%s", aux->akick[0]->nick, aux->akick[0]->motivo, aux->akick[0]->puesto);
	for (i = 1; i < aux->akicks; i++)
	{
		char *tmp;
		tmp = (char *)Malloc(sizeof(char) * (strlen(aux->akick[i]->nick) + strlen(aux->akick[i]->motivo) + strlen(aux->akick[i]->puesto) + 4));
		sprintf_irc(tmp, "\t%s\1%s\1%s", aux->akick[i]->nick, aux->akick[i]->motivo, aux->akick[i]->puesto);
		strcat(buf, tmp);
		Free(tmp);
	}
	_mysql_add(CS_MYSQL, canal, "akick", buf);
}
int es_akick(char *nick, char *canal)
{
	Akick *aux;
	int i;
	if ((aux = busca_akicks(canal)))
	{
		for (i = 0; i < aux->akicks; i++)
		{
			if (!strcasecmp(aux->akick[i]->nick, nick))
				return 1;
		}
	}
	return 0;
}
int es_cregistro(char *nick, char *canal)
{
	CsRegistros *aux;
	int i;
	if ((aux = busca_cregistro(nick)))
	{
		for (i = 0; i < aux->subs; i++)
		{
			if (!strcasecmp(aux->sub[i]->canal, canal))
				return 1;
		}
	}
	return 0;
}
#ifdef UDB
void envia_canal_bdd(char *canal)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *modos, *c;
	res = _mysql_query("SELECT founder,modos,topic from %s%s where item='%s'", PREFIJO, CS_MYSQL, canal);
	if (!res)
		return;
	row = mysql_fetch_row(res);
	modos = row[1];
	if ((c = strchr(modos, '+')))
		modos = c+1;
	if ((c = strchr(modos, '-')))
		*c = 0;
	envia_registro_bdd("C::%s::fundador %s", canal, row[0]);
	envia_registro_bdd("C::%s::modos %s", canal, modos);
	envia_registro_bdd("C::%s::topic %s", canal, row[2]);
	mysql_free_result(res);
}
#endif
/* hace un empaquetamiento de los users de una determinada lista. No olvidar hacer Free() */
Cliente **empaqueta_clientes(Canal *cn, LinkCliente *lista, u_char opers)
{
	Cliente **al;
	LinkCliente *lk;
	u_int i, j = 0;
	al = (Cliente **)Malloc(sizeof(Cliente *) * (cn->miembros + 1)); /* como máximo */
	lk = (lista ? lista : cn->miembro);
	for (i = 0; i < cn->miembros; i++)
	{
		if (!EsBot(lk->user) && (!IsOper(lk->user) || opers))
			al[j++] = lk->user;
		lk = lk->sig;
	}
	al[j] = NULL;
	return al;
}
BOTFUNC(chanserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(chanserv), "\00312%s\003 se encarga de gestionar los canales de la red para evitar robos o takeovers.", chanserv.hmod->nick);
		response(cl, CLI(chanserv), "El registro de canales permite su apropiación y poder gestionarlos de tal forma que usuarios no deseados "
						"puedan tomar control de los mismos.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Comandos disponibles:");
		response(cl, CLI(chanserv), "\00312INFO\003 Muestra distinta información de un canal.");
		response(cl, CLI(chanserv), "\00312LIST\003 Lista canales según un determinado patrón.");
		if (IsId(cl))
		{
			response(cl, CLI(chanserv), "\00312IDENTIFY\003 Te identifica como fundador de un canal.");
			response(cl, CLI(chanserv), "\00312DEAUTH\003 Te quita el estado de fundador.");
			response(cl, CLI(chanserv), "\00312INVITE\003 Invita a un usuario al canal.");
			response(cl, CLI(chanserv), "\00312OP/DEOP\003 Da o quita @.");
			response(cl, CLI(chanserv), "\00312HALF/DEHALF\003 Da o quita %%.");
			response(cl, CLI(chanserv), "\00312VOICE/DEVOICE\003 Da o quita +.");
			response(cl, CLI(chanserv), "\00312BAN/UNBAN\003 Pone o quita bans.");
			response(cl, CLI(chanserv), "\00312KICK\003 Expulsa a un usuario.");
			response(cl, CLI(chanserv), "\00312CLEAR\003 Resetea distintas opciones del canal.");
			response(cl, CLI(chanserv), "\00312SET\003 Fija distintas opciones del canal.");
			response(cl, CLI(chanserv), "\00312AKICK\003 Gestiona la lista de auto-kick del canal.");
			response(cl, CLI(chanserv), "\00312ACCESS\003 Gestiona la lista de accesos.");
			response(cl, CLI(chanserv), "\00312JOIN\003 Introduce distintos servicios.");
			response(cl, CLI(chanserv), "\00312REGISTER\003 Registra un canal.");
			response(cl, CLI(chanserv), "\00312TOKEN\003 Te entrega un token para las operaciones que lo requieran.");
#ifdef UDB
			response(cl, CLI(chanserv), "\00312MIGRAR\003 Migra un canal a la base de datos de la red.");
			response(cl, CLI(chanserv), "\00312DEMIGRAR\003 Demigra un canal de la base de datos de la red.");
			response(cl, CLI(chanserv), "\00312PROTEGER\003 Protege un canal para que sólo ciertos nicks puedan entrar.");
#endif
		}
		if (IsPreo(cl))
		{
			response(cl, CLI(chanserv), "\00312SENDPASS\003 Genera y manda una clave al email del fundador.");
			response(cl, CLI(chanserv), "\00312BLOCK\003 Bloquea y paraliza un canal.");
		}
		if (IsOper(cl))
		{
			response(cl, CLI(chanserv), "\00312DROP\003 Desregistra un canal.");
			response(cl, CLI(chanserv), "\00312SUSPENDER\003 Suspende un canal.");
			response(cl, CLI(chanserv), "\00312LIBERAR\003 Quita el suspenso de un canal.");
		}
		if (IsAdmin(cl))
		{
			response(cl, CLI(chanserv), "\00312FORBID\003 Prohibe el uso de un canal.");
			response(cl, CLI(chanserv), "\00312UNFORBID\003 Quita la prohibición de utilizar un canal.");
		}
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Para más información, \00312/msg %s %s comando", chanserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "INFO"))
	{
		response(cl, CLI(chanserv), "\00312INFO");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Muestra distinta información de un canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312INFO #canal");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		response(cl, CLI(chanserv), "\00312LIST");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Lista los canales registrados que concuerdan con un patrón.");
		response(cl, CLI(chanserv), "Puedes utilizar comodines (*) para ajustar la búsqueda.");
		response(cl, CLI(chanserv), "Además, puedes especificar el parámetro -r para listar los canales en petición.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312LIST [-r] patrón");
	}
	else if (!strcasecmp(param[1], "IDENTIFY") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312IDENTIFY");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Te identifica como fundador de un canal.");
		response(cl, CLI(chanserv), "Este comando sólo debe usarse si el nick que llevas no es el de fundador. "
						"En caso contrario, ya quedas identificado como tal si llevas nick identificado.");
		response(cl, CLI(chanserv), "Esta identificación se pierde en caso que te desconectes.");
		response(cl, CLI(chanserv), "Para proceder tienes que especificar la contraseña del canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312IDENTIFY #canal pass");
	}
	else if (!strcasecmp(param[1], "DEAUTH") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312DEAUTH");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Te borra la identificación que pudieras tener.");
		response(cl, CLI(chanserv), "Este comando sólo puede realizarse si previamente te habías identificado con el comando IDENTIFY.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312DEAUTH #canal");
	}
	else if (!strcasecmp(param[1], "INVITE") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312INVITE");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Obliga a \00312%s\003 a mandar una invitación para un canal.", chanserv.hmod->nick);
		response(cl, CLI(chanserv), "Si no especificas ningún nick, la invitación se te mandará a ti.");
		response(cl, CLI(chanserv), "Por el contrario, si especificas un nick, se le será mandada.");
		response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+i\003 para este canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312INVITE #canal [nick]");
	}
	else if ((!strcasecmp(param[1], "OP") || !strcasecmp(param[1], "DEOP") || !strcasecmp(param[1], "HALF") || !strcasecmp(param[1], "DEHALF")
		|| !strcasecmp(param[1], "VOICE") || !strcasecmp(param[1], "DEVOICE") || !strcasecmp(param[1], "BAN") || !strcasecmp(param[1], "UNBAN"))
		&& IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312%s", strtoupper(param[1]));
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Ejecuta este comando de forma remota por \00312%s\003 al canal.", chanserv.hmod->nick);
		response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312%s #canal nick1 [nick2 [nick3 [...nickN]]]", strtoupper(param[1]));
	}
	else if (!strcasecmp(param[1], "KICK") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312KICK");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Fuerza a \00312%s\003 a expulsar a un usuario.", chanserv.hmod->nick);
		response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312KICK #canal nick [motivo]");
	}
	else if (!strcasecmp(param[1], "CLEAR") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312CLEAR");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Resetea distintas opciones de un canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Opciones disponibles:");
		response(cl, CLI(chanserv), "\00312OPS\003 Quita todos los +o del canal.");
		response(cl, CLI(chanserv), "\00312HALFS\003 Quita todos los +h del canal.");
		response(cl, CLI(chanserv), "\00312VOICES\003 Quita todos los +v del canal.");
		response(cl, CLI(chanserv), "\00312BANS\003 Quita todos los +b del canal.");
		response(cl, CLI(chanserv), "\00312USERS\003 Expulsa a todos los usuarios del canal.");
		response(cl, CLI(chanserv), "\00312ACCESOS\003 Borra todos los accesos del canal.");
		response(cl, CLI(chanserv), "\00312MODOS\003 Quita todos los modos del canal y lo deja en +nt.");
		if (IsOper(cl))
		{
			response(cl, CLI(chanserv), "\00312KILL\003 Desconecta a todos los usuarios del canal.");
			response(cl, CLI(chanserv), "\00312GLINE\003 Pone una GLine a todos los usuarios del canal.");
		}
		else
			response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+c\003.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312opcion #canal %s", IsOper(cl) ? "[tiempo]" : "");
	}
	else if (!strcasecmp(param[1], "SET") && IsId(cl))
	{
		if (params < 3)
		{
			response(cl, CLI(chanserv), "\00312SET");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Fija distintas opciones del canal.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Opciones disponibles:");
			response(cl, CLI(chanserv), "\00312DESC\003 Descripción del canal.");
			response(cl, CLI(chanserv), "\00312URL\003 Página web.");
			response(cl, CLI(chanserv), "\00312EMAIL\003 Dirección de correo.");
			response(cl, CLI(chanserv), "\00312BIENVENIDA\003 Mensaje de bienvenida.");
			response(cl, CLI(chanserv), "\00312MODOS\003 Modos por defecto.");
			response(cl, CLI(chanserv), "\00312OPCIONES\003 Diferentes características.");
			response(cl, CLI(chanserv), "\00312TOPIC\003 Topic por defecto.");
			response(cl, CLI(chanserv), "\00312PASS\003 Cambia la contraseña.");
			response(cl, CLI(chanserv), "\00312FUNDADOR\003 Cambia el fundador.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+s\003.");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal opcion [parámetros]");
			response(cl, CLI(chanserv), "Para más información, \00312/msg %s %s SET opción", chanserv.hmod->nick, strtoupper(param[0]));
		}
		else if (!strcasecmp(param[2], "DESC"))
		{
			response(cl, CLI(chanserv), "\00312SET DESC");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Fija la descripción del canal.");
			response(cl, CLI(chanserv), "Esta descripción debe ser clara y concisa. Debe reflejar la temática del canal con la máxima precisión.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal DESC descripción");
		}
		else if (!strcasecmp(param[2], "URL"))
		{
			response(cl, CLI(chanserv), "\00312SET URL");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Asocia una dirección web al canal.");
			response(cl, CLI(chanserv), "Esta dirección es visualizada en el comando INFO.");
			response(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal URL [http://pagina.web]");
		}
		else if (!strcasecmp(param[2], "EMAIL"))
		{
			response(cl, CLI(chanserv), "\00312SET EMAIL");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Asocia una dirección de correco al canal.");
			response(cl, CLI(chanserv), "Esta dirección es visualizada en el comando INFO.");
			response(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal EMAIL [direccion@de.correo]");
		}
		else if (!strcasecmp(param[2], "BIENVENIDA"))
		{
			response(cl, CLI(chanserv), "\00312SET BIENVENIDA");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Asocia un mensaje de bienvenida al entrar al canal.");
			response(cl, CLI(chanserv), "Cada vez que un usuario entre en el canal se le mandará este mensaje vía NOTICE.");
			response(cl, CLI(chanserv), "Si no se especifica nada, es borrado.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal BIENVENIDA [mensaje]");
		}
		else if (!strcasecmp(param[2], "MODOS"))
		{
			response(cl, CLI(chanserv), "\00312SET MODOS");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Define los modos de canal.");
			response(cl, CLI(chanserv), "Es altamente recomendable que se especifiquen con el siguiente formato: +modos1-modos2");
			response(cl, CLI(chanserv), "Se aceptan parámetros para estos modos. No obstante, los modos oqveharb no se pueden utilizar.");
			response(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal MODOS [+modos1-modos2]");
		}
		else if (!strcasecmp(param[2], "OPCIONES"))
		{
			response(cl, CLI(chanserv), "\00312SET OPCIONES");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Fija las distintas características del canal.");
			response(cl, CLI(chanserv), "Estas características se identifican por flags.");
			response(cl, CLI(chanserv), "Así pues, en un mismo comando puedes subir y bajar distintos flags.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Flags disponibles:");
			response(cl, CLI(chanserv), "\00312+m\003 Candado de modos: los modos no pueden modificarse.");
			response(cl, CLI(chanserv), "\00312+r\003 Retención del topic: se guarda el último topic utilizado en el canal.");
			response(cl, CLI(chanserv), "\00312+k\003 Candado de topic: el topic no puede cambiarse.");
			response(cl, CLI(chanserv), "\00312+s\003 Canal seguro: sólo pueden entrar los que tienen acceso.");
			response(cl, CLI(chanserv), "\00312+o\003 Secureops: sólo pueden tener +a, +o, +h o +v los usuarios con el correspondiente acceso.");
			response(cl, CLI(chanserv), "\00312+h\003 Canal oculto: no se lista en el comando LIST.");
			response(cl, CLI(chanserv), "\00312+d\003 Canal debug: activa el debug del canal para visualizar los eventos y su autor.");
			if (IsOper(cl))
				response(cl, CLI(chanserv), "\00312+n\003 Canal no dropable: sólo puede droparlo la administración.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal OPCIONES +flags-flags");
			response(cl, CLI(chanserv), "Ejemplo: \00312SET #canal OPCIONES +ors-mh");
			response(cl, CLI(chanserv), "El canal #canal ahora posee las opciones de secureops, retención de topic y canal seguro. "
							"Se quitan las opciones de candado de modos y canal oculto si las tuviera.");
		}
		else if (!strcasecmp(param[2], "TOPIC"))
		{
			response(cl, CLI(chanserv), "\00312SET TOPIC");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Fija el topic por defecto del canal.");
			response(cl, CLI(chanserv), "Si no se especifica nada, es borrado.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal TOPIC [topic]");
		}
		else if (!strcasecmp(param[2], "PASS"))
		{
			response(cl, CLI(chanserv), "\00312SET PASS");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Cambia la contraseña del canal.");
			response(cl, CLI(chanserv), "Para poder realizar este comando necesitas ser el fundador o haberte identificado como tal con el comando IDENTIFY.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal PASS nuevopass");
		}
		else if (!strcasecmp(param[2], "FUNDADOR"))
		{
			response(cl, CLI(chanserv), "\00312SET FUNDADOR");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Cambia el fundador del canal.");
			response(cl, CLI(chanserv), "Para poder realizar este comando necesitas ser el fundador o haberte identificado como tal con el comando IDENTIFY.");
			response(cl, CLI(chanserv), "Además, el nuevo nick deberá estar registrado.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312SET #canal FUNDADOR nick");
		}
		else
			response(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
	}
	else if (!strcasecmp(param[1], "AKICK"))
	{
		response(cl, CLI(chanserv), "\00312AKICK");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Gestiona la lista de auto-kick del canal.");
		response(cl, CLI(chanserv), "Esta lista contiene las máscaras prohibidas.");
		response(cl, CLI(chanserv), "Si un usuario entra al canal y su máscara coincide con una de ellas, immediatamente es expulsado "
						"y se le pone un ban para que no pueda entrar.");
		response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+k\003.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal +nick|máscara [motivo]");
		response(cl, CLI(chanserv), "Añade al nick o la máscara con el motivo opcional.");
		response(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal -nick|máscara");
		response(cl, CLI(chanserv), "Borra al nick o la máscara.");
		response(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal");
		response(cl, CLI(chanserv), "Lista las diferentes entradas.");
	}
	else if (!strcasecmp(param[1], "ACCESS") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312ACCESS");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Gestiona la lista de accesos del canal.");
		response(cl, CLI(chanserv), "Esta lista da privilegios a los usuarios que estén en ella.");
		response(cl, CLI(chanserv), "Estos privilegios se les llama accesos y permiten acceder a distintos comandos.");
		response(cl, CLI(chanserv), "El fundador tiene acceso a todos ellos.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Accesos disponibles:");
		response(cl, CLI(chanserv), "\00312+s\003 Fijar opciones (SET)");
		response(cl, CLI(chanserv), "\00312+e\003 Editar la lista de accesos (ACCESS)");
		response(cl, CLI(chanserv), "\00312+l\003 Listar la lista de accesos (ACCESS)");
		response(cl, CLI(chanserv), "\00312+a\003 Tiene +a al entrar.");
		response(cl, CLI(chanserv), "\00312+o\003 Tiene +o al entrar.");
		response(cl, CLI(chanserv), "\00312+h\003 Tiene +h al entrar.");
		response(cl, CLI(chanserv), "\00312+v\003 Tiene +v al entrar.");
		response(cl, CLI(chanserv), "\00312+r\003 Comandos remotos (OP/DEOP/...)");
		response(cl, CLI(chanserv), "\00312+c\003 Resetear opciones (CLEAR)");
		response(cl, CLI(chanserv), "\00312+k\003 Gestionar la lista de auto-kicks (AKICK)");
		response(cl, CLI(chanserv), "\00312+i\003 Invitar (INVITE)");
		response(cl, CLI(chanserv), "\00312+j\003 Entrar bots (JOIN)");
		response(cl, CLI(chanserv), "\00312+g\003 Kick de venganza.");
		response(cl, CLI(chanserv), "\00312+m\003 Gestión de memos al canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "El nick al que se le dan los accesos debe estar registrado previamente.");
		response(cl, CLI(chanserv), "Un acceso no engloba otro. Por ejemplo, el tener +e no implica tener +l.");
		response(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal +nick +flags-flags");
		response(cl, CLI(chanserv), "Añade los accesos al nick.");
		response(cl, CLI(chanserv), "Ejemplo: \00312ACCESS #canal +pepito +aoh-lv");
		response(cl, CLI(chanserv), "pepito tendría +aoh al entrar en #canal y se le quitarían los privilegios de acceder a la lista y autovoz.");
		response(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal -nick");
		response(cl, CLI(chanserv), "Borra todos los accesos de este nick.");
		response(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal");
		response(cl, CLI(chanserv), "Lista todos los usuarios con acceso al canal.");
	}
	else if (!strcasecmp(param[1], "JOIN") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312JOIN");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Introduce los servicios al canal de forma permanente.");
		response(cl, CLI(chanserv), "Primero se quitan los servicios que pudiera haber para luego meter los que se soliciten.");
		response(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
		response(cl, CLI(chanserv), "Para realizar este comando debes tener acceso \00312+j\003.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312JOIN #canal [servicio1 [servicio2 [...servicioN]]]");
	}
	else if (!strcasecmp(param[1], "REGISTER") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312REGISTER");
		response(cl, CLI(chanserv), " ");
		if (IsOper(cl))
		{
			response(cl, CLI(chanserv), "Registra un canal para que sea controlado por los servicios.");
			response(cl, CLI(chanserv), "Este registro permite el control absoluto sobre el mismo.");
			response(cl, CLI(chanserv), "Con este comando puedes aceptar dos tipos de canales:");
			response(cl, CLI(chanserv), "- Aceptar peticiones de canales presentadas por los usuarios.");
			response(cl, CLI(chanserv), "- Registrar tus propios canales sin la necesidad de tokens.");
			response(cl, CLI(chanserv), "Aceptar una petición significa que registras un canal presentado con su fundador para que pase a sus manos.");
			response(cl, CLI(chanserv), "El canal expiará despues de \00312%i\003 días en los que no haya entrado ningún usuario con acceso.", chanserv.autodrop);
			response(cl, CLI(chanserv), "Para listar los canales en petición utiliza el comando LIST.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal");
			response(cl, CLI(chanserv), "Acepta la petición presentada por un usuario.");
			response(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripción");
			response(cl, CLI(chanserv), "Registra tu propio canal.");
		}
		else
		{
			
			response(cl, CLI(chanserv), "Solicita el registro de un canal.");
			response(cl, CLI(chanserv), "Para hacer una petición necesitas presentar como mínimo \00312%i\003 tokens.", chanserv.necesarios);
			response(cl, CLI(chanserv), "Estos tokens actúan como unas firmas que dan el visto bueno para su registro.");
			response(cl, CLI(chanserv), "Los tokens provienen de otros usuarios que los solicitan con el comando TOKEN.");
			response(cl, CLI(chanserv), "Si solicitas un token, no podrás presentar una solicitud hasta que no pase un cierto tiempo.");
			response(cl, CLI(chanserv), "El canal expiará despues de \00312%i\003 días en los que no haya entrado ningún usuario con acceso.", chanserv.autodrop);
			response(cl, CLI(chanserv), "Para más información consulta la ayuda del comando TOKEN.");
			response(cl, CLI(chanserv), " ");
			response(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripción tokens");
		}
	}
	else if (!strcasecmp(param[1], "TOKEN") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312TOKEN");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Solicita un token.");
		response(cl, CLI(chanserv), "Este token es necesario para acceder a comandos que requieran el acuerdo de varios usuarios.");
		response(cl, CLI(chanserv), "Sólo puedes solicitar un token cada \00312%i\003 horas. Una vez hayan transcurrido, el token caduca y puedes solicitar otro.", chanserv.vigencia);
		response(cl, CLI(chanserv), "Actúa como tu firma personal. Y es único.");
		response(cl, CLI(chanserv), "Por ejemplo, si alguien solicita el registro de un canal y tu estás a favor, deberás darle tu token a quién lo solicite para presentarlos "
						"para que así la administración confirme que cuenta con el mínimo de usuarios en acuerdo.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312TOKEN");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MIGRAR") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312MIGRAR");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Migra un canal a la base de datos de la red.");
		response(cl, CLI(chanserv), "Una vez migrado, el canal será permanente. Es decir, no se borrará aunque salga el último usuario.");
		response(cl, CLI(chanserv), "Conservará los modos y el topic cuando entre el primer usuario. A su vez seguirá mostrándose en el comando /LIST.");
		response(cl, CLI(chanserv), "Además, el founder cuando entré y tenga el modo +r siempre recibirá los modos +oq, acreditándole como tal.");
		response(cl, CLI(chanserv), "Finalmente, cualquier usuario que entre en el canal y esté vacío, si no es el founder se le quitará el estado de @ (-o).");
		response(cl, CLI(chanserv), "Este es independiente de los servicios de red, así pues asegura su funcionamiento siempre aunque no estén conectados.");
		response(cl, CLI(chanserv), "NOTA: este comando sólo puede realizarlo el fundador del canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312MIGRAR #canal pass");
	}
	else if (!strcasecmp(param[1], "DEMIGRAR") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\0012DEMIGRAR");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Elimina este canal de la base de datos de la red.");
		response(cl, CLI(chanserv), "El canal volverá a su administración anterior a través de los servicios de red.");
		response(cl, CLI(chanserv), "NOTA: este comando sólo puede realizarlo el fundador del canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312DEMIGRAR #canal pass");
	}
	else if (!strcasecmp(param[1], "PROTEGER") && IsId(cl))
	{
		response(cl, CLI(chanserv), "\00312PROTEGER");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Protege un canal por BDD.");
		response(cl, CLI(chanserv), "Esta protección consiste en permitir la entrada a determinados nicks.");
		response(cl, CLI(chanserv), "Al ser por BDD, aunque los servicios estén caídos, sólo podrán entrar los nicks que coincidan con las máscaras especificadas.");
		response(cl, CLI(chanserv), "Así pues, cualquier persona que no figure en esta lista de protegidos, no podrá acceder al canal.");
		response(cl, CLI(chanserv), "Si la máscara especificada es un nick, se añadirá la máscara de forma nick!*@*");
		response(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+e\003.");
		response(cl, CLI(chanserv), "Si no especeificas ninguna máscara, toda la lista es borrada.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312PROTEGER #canal [+|-[máscara]]");
	}
#endif
	else if (!strcasecmp(param[1], "SENDPASS") && IsPreo(cl))
	{
		response(cl, CLI(chanserv), "\00312SENDPASS");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Regenera una nueva clave para un canal y es enviada al email del fundador.");
		response(cl, CLI(chanserv), "Este comando sólo debe utilizarse a petición del fundador.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312SENDPASS #canal");
	}
	else if (!strcasecmp(param[1], "BLOCK") && IsPreo(cl))
	{
		response(cl, CLI(chanserv), "\00312BLOCK");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Inhabilita temporalmente un canal.");
		response(cl, CLI(chanserv), "Se quitan todos los privilegios de un canal (+qaohv) y se ponen los modos +imMRlN 1");
		response(cl, CLI(chanserv), "Su durada es temporal y puede se quitado mediante el comando CLEAR.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312BLOCK #canal");
	}
	else if (!strcasecmp(param[1], "DROP") && IsOper(cl))
	{
		response(cl, CLI(chanserv), "\00312DROP");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Borra un canal de la base de datos.");
		response(cl, CLI(chanserv), "Este canal tiene que haber estado registrado previamente.");
		response(cl, CLI(chanserv), "También puede utilizarse para denegar la petición de registro de un canal.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312DROP #canal");
	}
	else if (!strcasecmp(param[1], "SUSPENDER") && IsOper(cl))
	{
		response(cl, CLI(chanserv), "\00312SUSPENDER");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Suspende un canal de ser utilizado.");
		response(cl, CLI(chanserv), "El canal seguirá estando registrado pero sus usuarios, fundador incluído, no podrán aprovechar "
						"ninguna de sus prestaciones.");
		response(cl, CLI(chanserv), "Este suspenso durará indefinidamente hasta que se decida levantarlo.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312SUSPENDER #canal motivo");
	}
	else if (!strcasecmp(param[1], "LIBERAR") && IsOper(cl))
	{
		response(cl, CLI(chanserv), "\00312LIBERAR");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Quita el suspenso puesto previamente con el comando SUSPENDER.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312LIBERAR #canal");
	}
	else if (!strcasecmp(param[1], "FORBID") && IsAdmin(cl))
	{
		response(cl, CLI(chanserv), "\00312FORBID");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Prohibe el uso de este canal.");
		response(cl, CLI(chanserv), "Los usuarios no podrán ni entrar ni registrarlo.");
		response(cl, CLI(chanserv), "Este tipo de canales se extienden por toda la red.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312FORBID #canal motivo");
	}
	else if (!strcasecmp(param[1], "UNFORBID") && IsAdmin(cl))
	{
		response(cl, CLI(chanserv), "\00312UNFORBID");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Quita la prohibición de un canal puesta con el comando FORBID.");
		response(cl, CLI(chanserv), " ");
		response(cl, CLI(chanserv), "Sintaxis: \00312UNFORBID #canal");
	}
	else
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(chanserv_deauth)
{
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "DEAUTH #canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	response(cl, CLI(chanserv), CS_ERR_EMPT, "No te has identificado como fundador.");
	return 1;
	
}
BOTFUNC(chanserv_drop)
{
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "DROP #canal");
		return 1;
	}
	if (!ChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsAdmin(cl) && (atoi(_mysql_get_registro(CS_MYSQL, param[1], "opts")) & CS_OPT_NODROP))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no se puede dropear.");
		return 1;
	}
	if (!chanserv_baja(param[1], 1))
		response(cl, CLI(chanserv), "Se ha dropeado el canal \00312%s\003.", param[1]);
	else
		response(cl, CLI(chanserv), CS_ERR_EMPT, "No se ha podido dropear. Comuníquelo a la administración.");
	return 0;
}
BOTFUNC(chanserv_identify)
{
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "IDENTIFY #canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if ((chanserv.opts & CS_SID) && !parv[parc])
	{
		sprintf_irc(buf, "Identificación incorrecta. /msg %s@%s IDENTIFY #canal pass", chanserv.hmod->nick, me.nombre);
		response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}	
	if (strcmp(_mysql_get_registro(CS_MYSQL, param[1], "pass"), MDString(param[2])))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	/*if (cl->fundadores == MAX_FUN)
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "No se aceptan más identificaciones.");
		return 1;
	}
	if (es_fundador(cl, param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Ya te has identificado como fundador de este canal.");
		return 1;
	}
	ircstrdup(&cl->fundador[cl->fundadores], param[1]);
	cl->fundadores++;*/
	response(cl, CLI(chanserv), "Ahora eres reconocido como founder de \00312%s\003.", param[1]);
	return 0;
}
BOTFUNC(chanserv_info)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	time_t reg;
	int opts;
	char *forb, *susp;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "INFO #canal");
		return 1;
	}
	if ((forb = IsChanForbid(param[1])))
	{
		response(cl, CLI(chanserv), "Este canal está \2PROHIBIDO\2: %s", forb);
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	res = _mysql_query("SELECT opts,founder,descripcion,registro,entry,url,email,modos,topic,ntopic,ultimo from %s%s where item='%s'", PREFIJO, CS_MYSQL, param[1]);
	row = mysql_fetch_row(res);
	opts = atoi(row[0]);
	response(cl, CLI(chanserv), "*** Información del canal \00312%s\003 ***", param[1]);
	if ((susp = IsChanSuspend(param[1])))
		response(cl, CLI(chanserv), "Estado: \00312SUSPENDIDO: %s", susp);
	else
		response(cl, CLI(chanserv), "Estado: \00312ACTIVO");
	response(cl, CLI(chanserv), "Fundador: \00312%s", row[1]);
	response(cl, CLI(chanserv), "Descripción: \00312%s", row[2]);
	reg = (time_t)atol(row[3]);
	response(cl, CLI(chanserv), "Fecha de registro: \00312%s", _asctime(&reg));
	response(cl, CLI(chanserv), "Mensaje de bienvenida: \00312%s", row[4]);
	if (!BadPtr(row[5]))
		response(cl, CLI(chanserv), "URL: \00312%s", row[5]);
	if (!BadPtr(row[6]))
		response(cl, CLI(chanserv), "Email: \00312%s", row[6]);
	response(cl, CLI(chanserv), "Modos: \00312%s\003 Candado de modos: \00312%s", row[7], opts & CS_OPT_RMOD ? "ON" : "OFF");
	response(cl, CLI(chanserv), "Topic: \00312%s\003 puesto por: \00312%s", row[8], row[9]);
	response(cl, CLI(chanserv), "Candado de topic: \00312%s\003 Retención de topic: \00312%s", opts & CS_OPT_KTOP ? "ON" : "OFF", opts & CS_OPT_RTOP ? "ON" : "OFF");
	if (opts & CS_OPT_SOP)
		response(cl, CLI(chanserv), "Canal con \2SECUREOPS");
	if (opts & CS_OPT_SEC)
		response(cl, CLI(chanserv), "Canal \2SEGURO");
	reg = (time_t)atol(row[10]);
	response(cl, CLI(chanserv), "Último acceso: \00312%s", _asctime(&reg));
	mysql_free_result(res);
	return 0;
}
BOTFUNC(chanserv_invite)
{
	Cliente *al = NULL;
	Canal *cn;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "INVITE #canal [nick]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!tiene_nivel(cl->nombre, param[1], CS_LEV_INV))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB);
		return 1;
	}
	if (params == 3 && !(al = busca_cliente(param[2], NULL)))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	cn = busca_canal(param[1], NULL);
	port_func(P_INVITE)(al ? al : cl, CLI(chanserv), cn);
	response(cl, CLI(chanserv), "El usuario \00312%s\003 ha sido invitado a \00312%s\003.", params == 3 ? param[2] : parv[0], param[1]);
	if (IsChanDebug(param[1]))
		port_func(P_NOTICE)((Cliente *)cn, CLI(chanserv), "%s invita a %s", parv[0], params == 3 ? param[2] : parv[0]);
	return 0;
}
BOTFUNC(chanserv_modos)
{
	int i, opts;
	Cliente *al;
	Canal *cn;
	if (params < 3)
	{
		response(cl, CLI(chanserv), "%s #canal parámetros", strtoupper(param[0]));
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = busca_canal(param[1], NULL)))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "El canal está vacío.");
		return 1;
	}
	if (!tiene_nivel(parv[0], param[1], CS_LEV_RMO))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	opts = atoi(_mysql_get_registro(CS_MYSQL, param[1], "opts"));
	if (!strcasecmp(param[0], "OP"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || es_link(cn->op, al))
				continue;
			if (!tiene_nivel(param[i], param[1], CS_LEV_AOP) && (opts & CS_OPT_SOP))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+o %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEOP"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || !es_link(cn->op, al))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "HALF"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || es_link(cn->half, al))
				continue;
			if (!tiene_nivel(param[i], param[1], CS_LEV_AHA) && (opts & CS_OPT_SOP))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+h %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEHALF"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || !es_link(cn->half, al))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-h %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "VOICE"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || es_link(cn->voz, al))
				continue;
			if (!tiene_nivel(param[i], param[1], CS_LEV_AVO) && (opts & CS_OPT_SOP))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+v %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEVOICE"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = busca_cliente(param[i], NULL)) || !es_link(cn->voz, al))
				continue;
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-v %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "BAN"))
	{
		for (i = 2; i < params; i++)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+b %s", ircdmask(param[i]));
	}
	else if (!strcasecmp(param[0], "UNBAN"))
	{
		for (i = 2; i < params; i++)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-b %s", ircdmask(param[i]));
	}
	else if (!strcasecmp(param[0], "KICK"))
	{
		if (!(al = busca_cliente(param[2], NULL)))
			return 1;
		port_func(P_KICK)(al, CLI(chanserv), cn, implode(param, params, 3, -1));
	}
	if (IsChanDebug(param[1]))
		port_func(P_NOTICE)((Cliente *)cn, CLI(chanserv), "%s hace %s a %s", parv[0], strtoupper(param[0]), implode(param, params, 2, -1));
	return 0;
}
BOTFUNC(chanserv_clear)
{
	Canal *cn;
	Cliente **al = NULL;
	int i;
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "CLEAR #canal opcion");
		return 1;
	}
	if (!IsChanReg(param[1]) && !IsPreo(cl))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = busca_canal(param[1], NULL)))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "El canal está vacío.");
		return 1;
	}
	if (!tiene_nivel(parv[0], param[1], CS_LEV_RES))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!strcasecmp(param[2], "USERS"))
	{
		al = empaqueta_clientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_KICK)(al[i], CLI(chanserv), cn, "CLEAR USERS");
	}
	else if (!strcasecmp(param[2], "OPS"))
	{
		al = empaqueta_clientes(cn, cn->op, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "HALFS"))
	{
		al = empaqueta_clientes(cn, cn->half, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-h %s", TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "VOCES"))
	{
		al = empaqueta_clientes(cn, cn->voz, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al[i]));
	}
	else if (!strcasecmp(param[2], "ACCESOS"))
	{
		char *regs, *tok;
		if (!IsChanReg(param[1]))
		{
			response(cl, CLI(chanserv), CS_ERR_NCHR, "");
			return 1;
		}
		regs = _mysql_get_registro(CS_MYSQL, param[1], "regs");
		for (tok = strtok(regs, ":"); tok; tok = strtok(NULL, ":"))
		{
			borra_cregistro(tok, param[1]);
			strtok(NULL, " ");
		}
		_mysql_add(CS_MYSQL, param[1], "regs", NULL);
		response(cl, CLI(chanserv), "Lista de accesos eliminada.");
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-%s", modes2flags(cn->modos, protocolo->cmodos, cn));
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+nt%s", IsChanReg(param[1]) ? "r" : "");
		response(cl, CLI(chanserv), "Modos resetados a +nt.");
	}
	else if (!strcasecmp(param[2], "BANS"))
	{
		while (cn->ban)
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-b %s", cn->ban->ban);
	}
	else if (!strcasecmp(param[2], "KILL"))
	{
		if (!IsOper(cl))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		al = empaqueta_clientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_QUIT_USUARIO_REMOTO)(al[i], CLI(chanserv), "KILL USERS");
		response(cl, CLI(chanserv), "Usuarios de \00312%s\003 desconectados.", param[1]);
	}
	else if (!strcasecmp(param[2], "GLINE"))
	{
		LinkCliente *aux;
		if (!IsOper(cl))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "CLEAR #canal GLINE tiempo");
			return 1;
		}
		if (atoi(param[3]) < 1)
		{
			response(cl, CLI(chanserv), CS_ERR_SNTX, "El tiempo debe ser superior a 1 segundo.");
			return 1;
		}
		for (aux = cn->miembro; aux; aux = aux->sig)
		{
			if (!EsBot(aux->user) && !IsOper(aux->user))
				port_func(P_GLINE)(CLI(chanserv), ADD, aux->user->ident, aux->user->host, atoi(param[3]), "GLINE USERS");
		}
		response(cl, CLI(chanserv), "Usuarios de \00312%s\003 con gline durante \00312%s\003 segundos.", param[1], param[3]);
	}
	else
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
		return 1;
	}
	ircfree(al);
	return 0;
}
BOTFUNC(chanserv_set)
{
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal parámetros");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!tiene_nivel(parv[0], param[1], CS_LEV_SET))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!strcasecmp(param[2], "DESC"))
	{
		if (params < 4)
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal DESC descripción");
			return 1;
		}
		_mysql_add(CS_MYSQL, param[1], "descripcion", implode(param, params, 3, -1));
		response(cl, CLI(chanserv), "Descripción cambiada.");
	}
	else if (!strcasecmp(param[2], "URL"))
	{
		if (params == 4)
			response(cl, CLI(chanserv), "URL cambiada.");
		else
			response(cl, CLI(chanserv), "URL desactivada.");
		_mysql_add(CS_MYSQL, param[1], "url", params == 4 ? param[3] : NULL);
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		if (params == 4)
			response(cl, CLI(chanserv), "EMAIL cambiado.");
		else
			response(cl, CLI(chanserv), "EMAIL desactivado.");
		_mysql_add(CS_MYSQL, param[1], "email", params == 4 ? param[3] : NULL);
	}
	else if (!strcasecmp(param[2], "TOPIC"))
	{
		char *topic;
		topic = params > 3 ? implode(param, params, 3, -1) : NULL;
		if (params > 3)
		{
			response(cl, CLI(chanserv), "TOPIC cambiado.");
#ifndef UDB
			port_func(P_TOPIC)(CLI(chanserv), busca_canal(param[1], NULL), topic);
#endif
		}
		else
			response(cl, CLI(chanserv), "TOPIC desactivado.");
		_mysql_add(CS_MYSQL, param[1], "topic", topic);
		_mysql_add(CS_MYSQL, param[1], "ntopic", params > 3 ? parv[0] : NULL);
#ifdef UDB
		if (IsChanUDB(param[1]))
			envia_registro_bdd("C::%s::topic %s", param[1], topic);
#endif
	}
	else if (!strcasecmp(param[2], "BIENVENIDA"))
	{
		char *entry;
		entry = params > 3 ? implode(param, params, 3, -1) : NULL;
		if (params > 3)
			response(cl, CLI(chanserv), "Mensaje de bienvenida cambiado.");
		else
			response(cl, CLI(chanserv), "Mensaje de bienvenida desactivado.");
		_mysql_add(CS_MYSQL, param[1], "entry", entry);
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		char *modos = NULL;
		if (params > 3)
		{
			char forb[] = "ohvbeqar";
			int i;
			for (i = 0; forb[i] != '\0'; i++)
			{
				if (strchr(param[3], forb[i]))
				{
					response(cl, CLI(chanserv), CS_ERR_EMPT, "Los modos ohvbeqar no se pueden especificar.");
					return 1;
				}
			}
			modos = implode(param, params, 3, -1);
			port_func(P_MODO_CANAL)(CLI(chanserv), busca_canal(param[1], NULL), modos);
			response(cl, CLI(chanserv), "Modos cambiados.");
			
		}
		else
			response(cl, CLI(chanserv), "Modos eliminados.");
		_mysql_add(CS_MYSQL, param[1], "modos", modos);
#ifdef UDB
		if (IsChanUDB(param[1]))
			envia_registro_bdd("C::%s::modos %s", param[1], modos);
#endif
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		if (!es_fundador(cl, param[1]) && !IsOper(cl))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal PASS contraseña");
			return 1;
		}
		_mysql_add(CS_MYSQL, param[1], "pass", MDString(param[3]));
		response(cl, CLI(chanserv), "Contraseña cambiada.");
	}
	else if (!strcasecmp(param[2], "FUNDADOR"))
	{
		if (!es_fundador(cl, param[1]) && !IsOper(cl))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal FUNDADOR nick");
			return 1;
		}
		if (!IsReg(param[3]))
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		_mysql_add(CS_MYSQL, param[1], "founder", param[3]);
#ifdef UDB
		if (IsChanUDB(param[1]))
			envia_registro_bdd("C::%s::fundador %s", param[1], param[3]);
#endif
		response(cl, CLI(chanserv), "Fundador cambiado.");
	}
	else if (!strcasecmp(param[2], "OPCIONES"))
	{
		char f = ADD, *modos = param[3];
		int opts = atoi(_mysql_get_registro(CS_MYSQL, param[1], "opts"));
		if (params < 4)
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal opciones +-modos");
			return 1;
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
				case 'm':
					if (f == ADD)
						opts |= CS_OPT_RMOD;
					else
						opts &= ~CS_OPT_RMOD;
					break;
				case 'r':
					if (f == ADD)
						opts |= CS_OPT_RTOP;
					else
						opts &= ~CS_OPT_RTOP;
					break;
				case 'k':
					if (f == ADD)
						opts |= CS_OPT_KTOP;
					else
						opts &= ~CS_OPT_KTOP;
					break;
				case 's':
					if (f == ADD)
						opts |= CS_OPT_SEC;
					else
						opts &= ~CS_OPT_SEC;
					break;
				case 'o':
					if (f == ADD)
						opts |= CS_OPT_SOP;
					else
						opts &= ~CS_OPT_SOP;
					break;
				case 'h':
					if (f == ADD)
						opts |= CS_OPT_HIDE;
					else
						opts &= ~CS_OPT_HIDE;
					break;
				case 'd':
					if (f == ADD)
						opts |= CS_OPT_DEBUG;
					else
						opts &= ~CS_OPT_DEBUG;
					break;
				case 'n':
					if (!IsOper(cl))
					{
						response(cl, CLI(chanserv), CS_ERR_FORB, "");
						return 1;
					}
					if (f == ADD)
						opts |= CS_OPT_NODROP;
					else
						opts &= ~CS_OPT_NODROP;
					break;
				default:
					sprintf_irc(buf, "La opción %c es desconocida.", *modos);
					response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
			}
			modos++;
		}
		_mysql_add(CS_MYSQL, param[1], "opts", "%i", opts);
		response(cl, CLI(chanserv), "Opciones cambiadas a \00312%s\003.", param[3]);
	}
	else
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Opción desconocida.");
		return 1;
	}
	return 0;
}
BOTFUNC(chanserv_akick)
{
	Akick *aux;
	Canal *cn;
	int i;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "AKICK #canal [+-nick|mascara [motivo]]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!(cn = busca_canal(param[1], NULL)))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "El canal está vacío.");
		return 1;
	}
	if (!tiene_nivel(parv[0], param[1], CS_LEV_ACK))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (params == 2)
	{
		if (!(aux = busca_akicks(param[1])) || !aux->akicks)
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ningún akick.");
			return 1;
		}
		for (i = 0; i < aux->akicks; i++)
			response(cl, CLI(chanserv), "\00312%s\003 Motivo: \00312%s\003 (por \00312%s\003)", aux->akick[i]->nick, aux->akick[i]->motivo, aux->akick[i]->puesto);
	}
	else
	{
		if (*param[2] == '+')
		{
			Cliente *al;
			char *motivo = NULL;
			if ((aux = busca_akicks(param[1])) && aux->akicks == CS_MAX_AKICK && !es_akick(param[2] + 1, param[1]))
			{
				response(cl, CLI(chanserv), CS_ERR_EMPT, "No se aceptan más entradas.");
				return 1;
			}
			if (params > 3)
				motivo = implode(param, params, 3, -1);
			else
				motivo = "No eres bienvenid@.";
			if (motivo && (strchr(motivo, '\t') || strchr(motivo, '\1')))
			{
				response(cl, CLI(chanserv), CS_ERR_EMPT, "Este motivo contiene caracteres inválidos.");
				return 1;
			}
			borra_akick_bd(param[2] + 1, param[1]);
			inserta_akick_bd(param[2] + 1, param[1], parv[0], motivo);
			if ((al = busca_cliente(param[2] + 1, NULL)))
				port_func(P_KICK)(al, CLI(chanserv), cn, motivo);
			response(cl, CLI(chanserv), "Akick a \00312%s\003 añadido.", param[2] + 1);
		}
		else if (*param[2] == '-')
		{
			if (!es_akick(param[2] + 1, param[1]))
			{
				response(cl, CLI(chanserv), CS_ERR_EMPT, "Esta entrada no existe.");
				return 1;
			}
			borra_akick_bd(param[2] + 1, param[1]);
			response(cl, CLI(chanserv), "Entrada \00312%s\003 eliminada.", param[2] + 1);
		}
		else
		{
			response(cl, CLI(chanserv), CS_ERR_SNTX, "AKICK +-nick|mascara [motivo]");
			return 1;
		}
	}	
	return 0;
}
BOTFUNC(chanserv_access)
{
	char *registros;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "ACCESS #canal [+-nick [+-flags]]");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (params == 2)
	{
		char *tok;
		if (!tiene_nivel(parv[0], param[1], CS_LEV_LIS))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		registros = _mysql_get_registro(CS_MYSQL, param[1], "regs");
		if (!registros)
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ningún acceso.");
			return 1;
		}
		response(cl, CLI(chanserv), "*** Accesos de \00312%s\003 ***", param[1]);
		for (tok = strtok(registros, ":"); tok; tok = strtok(NULL, ":"))
		{
			u_long niv = atol(strtok(NULL, " "));
			response(cl, CLI(chanserv), "Nick: \00312%s\003 flags:\00312+%s\003 (\00312%lu\003)", tok, modes2flags(niv, cFlags, NULL), niv);
		}
	}
	else
	{
		u_long prev;
		if (!tiene_nivel(parv[0], param[1], CS_LEV_EDT))
		{
			response(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (*param[2] != '+' && *param[2] != '-')
		{
			response(cl, CLI(chanserv), CS_ERR_SNTX, "ACCESS #canal +-nick [+-flags]");
			return 1;
		}
		if (!IsReg(param[2] + 1))
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no está registrado.");
			return 1;
		}
		if (*param[2] == '+')
		{
			char f = ADD, buf[512], *modos = param[3];
			param[2]++;
			if (params < 4)
			{
				response(cl, CLI(chanserv), CS_ERR_PARA, "ACCESS +-nick +-flags");
				return 1;
			}
			prev = borra_acceso(param[2], param[1]);
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
							prev |= flag2mode(*modos, cFlags);
						else
							prev &= ~flag2mode(*modos, cFlags);
				}
				modos++;
			}
			if (prev)
			{
				CsRegistros *aux;
				if ((aux = busca_cregistro(param[2])) && aux->subs == CS_MAX_REGS && !es_cregistro(param[2], param[1]))
				{
					response(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick ya no acepta más registros.");
					return 1;
				}
				registros = _mysql_get_registro(CS_MYSQL, param[1], "regs");
				sprintf_irc(buf, "%s:%lu", param[2], prev);
				_mysql_add(CS_MYSQL, param[1], "regs", "%s%s", registros ? registros : "", buf);
				inserta_cregistro(param[2], param[1], prev);
				response(cl, CLI(chanserv), "Acceso de \00312%s\003 cambiado a \00312%s\003", param[2], param[3]);
				if (IsChanDebug(param[1]))
				{
					Canal *cn;
					cn = busca_canal(param[1], NULL);
					port_func(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s cambiado a %s", param[2], param[3]);
				}
			}
			else
			{
				response(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2]);
				if (IsChanDebug(param[1]))
				{
					Canal *cn;
					cn = busca_canal(param[1], NULL);
					port_func(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s eliminado", param[2]);
				}
			}
		}
		else if (*param[2] == '-')
		{
			prev = borra_acceso(param[2] + 1, param[1]);
			response(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2] + 1);
			if (IsChanDebug(param[1]))
			{
				Canal *cn;
				cn = busca_canal(param[1], NULL);
				port_func(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s eliminado", param[2]);
			}
		}
	}	
	return 0;
}
BOTFUNC(chanserv_list)
{
	char *rep;
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patrón");
		return 1;
	}
	if (params == 3)
	{
		if (*param[1] == '-')
		{
			if (params < 3)
			{
				response(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patrón");
				return 1;
			}
			switch(*(param[1] + 1))
			{
				case 'r':
					rep = str_replace(param[2], '*', '%');
					res = _mysql_query("SELECT item,descripcion from %s%s where item LIKE '%s' AND registro='0'", PREFIJO, CS_MYSQL, rep);
					if (!res)
					{
						response(cl, CLI(chanserv), CS_ERR_EMPT, "No hay canales para listar.");
						return 1;
					}
					response(cl, CLI(chanserv), "*** Canales en petición que coinciden con el patrón \00312%s\003 ***", param[1]);
					for (i = 0; i < chanserv.maxlist && (row = mysql_fetch_row(res)); i++)
						response(cl, CLI(chanserv), "\00312%s\003 Desc:\00312%s", row[0], row[1]);
					response(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
					mysql_free_result(res);
					break;
				default:
					response(cl, CLI(chanserv), CS_ERR_SNTX, "Parámetro no válido.");
					return 1;
			}
		}
	}
	else
	{
		if (*param[1] == '-')
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patrón");
			return 1;
		}
		rep = str_replace(param[1], '*', '%');
		res = _mysql_query("SELECT item from %s%s where item LIKE '%s' AND registro!='0'", PREFIJO, CS_MYSQL, rep);
		if (!res)
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "No hay canales para listar.");
			return 1;
		}
		response(cl, CLI(chanserv), "*** Canales que coinciden con el patrón \00312%s\003 ***", param[1]);
		for (i = 0; i < chanserv.maxlist && (row = mysql_fetch_row(res)); i++)
		{
			if (IsOper(cl) || !(atoi(_mysql_get_registro(CS_MYSQL, row[0], "opts")) & CS_OPT_HIDE))
				response(cl, CLI(chanserv), "\00312%s", row[0]);
			else
				i--;
		}
		response(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
		mysql_free_result(res);
	}
	return 0;
}
BOTFUNC(chanserv_jb)
{
	Cliente *bl;
	Canal *cn;
	int i, bots = 0;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "JOIN #canal [servicios]");
		return 1;
	}
	if (!IsChanReg(param[1]) && !IsOper(cl))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	cn = busca_canal(param[1], NULL);
	if (cn)
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			bl = busca_cliente(ex->nick, NULL);
			if (es_link(cn->miembro, bl) && !es_residente(ex, param[1] + 1))
				saca_bot_de_canal(bl, cn->nombre, "Cambiando bots");
		}
	}
	for (i = 2; i < params; i++)
	{
		Modulo *ex;
		if ((ex = busca_modulo(param[i], modulos)))
		{
			bl = busca_cliente(ex->nick, NULL);
			mete_bot_en_canal(bl, param[1]);
			bots |= ex->id;
		}
	}
	if (IsChanReg(param[1]))
		_mysql_add(CS_MYSQL, param[1], "bjoins", "%i", bots);
	response(cl, CLI(chanserv), "Bots cambiados.");
	return 0;
}
BOTFUNC(chanserv_sendpass)
{
	char *pass;
	char *founder;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "SENDPASS #canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	pass = random_ex("******-******");
	founder = strdup(_mysql_get_registro(CS_MYSQL, param[1], "founder"));
	_mysql_add(CS_MYSQL, param[1], "pass", MDString(pass));
	email(_mysql_get_registro(NS_MYSQL, founder, "email"), "Reenvío de la contraseña", "Debido a la pérdida de la contraseña de tu canal, se te ha generado otra clave totalmetne segura.\r\n"
		"A partir de ahora, la clave de tu canal es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, chanserv.hmod->nick, conf_set->red);
	response(cl, CLI(chanserv), "Se generado y enviado otra contraseña al email del founder de \00312%s\003.", param[1]);
	Free(founder);
	return 0;
}
BOTFUNC(chanserv_suspender)
{
	char *motivo;
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "SUSPENDER #canal motivo");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	motivo = implode(param, params, 2, -1);
#ifdef UDB
	envia_registro_bdd("C::%s::suspendido %s", param[1], motivo);
#else
	_mysql_add(CS_MYSQL, param[1], "suspend", motivo);
#endif
	response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido suspendido.", param[1]);
	return 0;
}
BOTFUNC(chanserv_liberar)
{
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "LIBERAR #canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está suspendido.");
		return 1;
	}
#ifdef UDB
	envia_registro_bdd("C::%s::suspendido", param[1]);
#else
	_mysql_add(CS_MYSQL, param[1], "suspend", "");
#endif
	response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su suspenso.", param[1]);
	return 0;
}
BOTFUNC(chanserv_forbid)
{
	Canal *cn;
	char *motivo;
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "FORBID #canal motivo");
		return 1;
	}
	motivo = implode(param, params, 2, -1);
	if ((cn = busca_canal(param[1], NULL)) && cn->miembros)
	{
		Cliente **al = NULL;
		int i;
		al = empaqueta_clientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			port_func(P_KICK)(al[i], CLI(chanserv), cn, "Canal \2PROHIBIDO\2: %s", motivo);
		ircfree(al);
	}
#ifdef UDB
	envia_registro_bdd("C::%s::forbid %s", param[1], motivo);
#else
	_mysql_add(CS_FORBIDS, param[1], "motivo", motivo);
#endif
	response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido prohibido.", param[1]);
	return 0;
}
BOTFUNC(chanserv_unforbid)
{
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "UNFORBID #canal");
		return 1;
	}
	if (!IsChanForbid(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está prohibido.");
		return 1;
	}
#ifdef UDB
	envia_registro_bdd("C::%s::forbid", param[1]);
#else
	_mysql_del(CS_FORBIDS, param[1]);
#endif
	response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su prohibición.", param[1]);
	return 0;
}
BOTFUNC(chanserv_block)
{
	Canal *cn;
	Cliente **al;
	int i;
	if (params < 2)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "BLOCK #canal");
		return 1;
	}
	if (!(cn = busca_canal(param[1], NULL)))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal está vacío.");
		return 1;
	}
	al = empaqueta_clientes(cn, cn->owner, ADD);
	for (i = 0; al[i]; i++)
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-q %s", TRIO(al[i]));
	ircfree(al);
	al = empaqueta_clientes(cn, cn->admin, ADD);
	for (i = 0; al[i]; i++)
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-a %s", TRIO(al[i]));
	ircfree(al);
	al = empaqueta_clientes(cn, cn->op, ADD);
	for (i = 0; al[i]; i++)
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al[i]));
	ircfree(al);
	al = empaqueta_clientes(cn, cn->half, ADD);
	for (i = 0; al[i]; i++)
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-h %s", TRIO(al[i]));
	ircfree(al);
	al = empaqueta_clientes(cn, cn->voz, ADD);
	for (i = 0; al[i]; i++)
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-v %s", TRIO(al[i]));
	ircfree(al);
	port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+iml 1");
	response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido bloqueado.", param[1]);
	/* aun asi podrán opearse si tienen nivel, se supone que este comando lo utilizan los operadores 
	   y estan supervisando el canal, para que si alguno se opea se le killee, o simplemente por hacer 
	   la gracia */
	return 0;
}
BOTFUNC(chanserv_register)
{
	if (params == 2) /* registro de una petición de canal */
	{
		if (!IsOper(cl))
		{
			response(cl, CLI(chanserv), CS_ERR_PARA, "REGISTER #canal pass descripción tokens");
			return 1;
		}
		if (!IsChanPReg(param[1]))
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está en petición de registro.");
			return 1;
		}
		goto registrar;
		_mysql_add(CS_MYSQL, param[1], "registro", "%i", time(0));
#ifdef UDB
		if (chanserv.opts & CS_AUTOMIGRAR)
		{
			_mysql_add(CS_MYSQL, param[1], "opts", "%i", CS_OPT_UDB);
			envia_canal_bdd(param[1]);
		}
#endif
		response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido registrado.", param[1]);
	}
	else if (params >= 4) /* peticion de registro */
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		int i;
		char *desc;
		if (IsChanReg(param[1]))
		{
			response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya está registrado.");
			return 1;
		}
		if (!IsOper(cl))
		{
			if ((atol(_mysql_get_registro(NS_MYSQL, parv[0], "reg")) + 86400 * chanserv.antig) > time(0))
			{
				char buf[512];
				sprintf_irc(buf, "Tu nick debe tener una antigüedad de como mínimo %i días.", chanserv.antig);
				response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				return 1;
			}
			if ((res = _mysql_query("SELECT nick from %s%s where nick='%s'", PREFIJO, CS_TOK, parv[0])))
			{
				char buf[512];
				mysql_free_result(res);
				sprintf_irc(buf, "Ya has efectuado alguna operación referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv.vigencia);
				response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				return 1;
			}
			if (params < (4 + chanserv.necesarios))
			{
				char buf[512];
				sprintf_irc(buf, "Se necesitan %i tokens.", chanserv.necesarios);
				response(cl, CLI(chanserv), CS_ERR_SNTX, buf);
				return 1;
			}
			/* comprobamos los tokens */
			for (i = 0; i < chanserv.necesarios; i++)
			{
				char buf[512];
				char *tok = param[params - i - 1];
				tokbuf[0] = '\0';
				if (strstr(tokbuf, tok)) /* ya había sido usado */
				{
					sprintf_irc(buf, "El token %s está repetido.", tok);
					response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				if (!(res = _mysql_query("SELECT item,nick,hora from %s%s where item='%s'", PREFIJO, CS_TOK, tok)))
				{
					sprintf_irc(buf, "El token %s no es válido o ha caducado.", tok);
					response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (BadPtr(row[1]) || BadPtr(row[2]))
				{
					sprintf_irc(buf, "Existe un error en el token %s. Posiblemente haya sido robado.", tok);
					response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				if ((atol(row[2]) + 24 * chanserv.vigencia) < time(0))
				{
					sprintf_irc(buf, "El token %s ha caducado.", tok);
					response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				strcat(tokbuf, tok);
				strcat(tokbuf, " ");
			}
			desc = implode(param, params, 3, params - chanserv.necesarios - 1);
		}
		else
			desc = implode(param, params, 3, -1);
		_mysql_add(CS_MYSQL, param[1], "founder", parv[0]);
		_mysql_add(CS_MYSQL, param[1], "pass", MDString(param[2]));
		_mysql_add(CS_MYSQL, param[1], "descripcion", desc);
		if (IsOper(cl))
		{
			Canal *cn;
			registrar:
			cn = busca_canal(param[1], NULL);
			_mysql_add(CS_MYSQL, param[1], "registro", "%i", time(0));
#ifdef UDB
			if (chanserv.opts & CS_AUTOMIGRAR)
			{
				_mysql_add(CS_MYSQL, param[1], "opts", "%i", CS_OPT_UDB);
				envia_canal_bdd(param[1]);
			}
			else
#endif
			if (cn)
			{
				if (RedOverride)
					mete_bot_en_canal(CLI(chanserv), cn->nombre);
				port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+r");
				port_func(P_TOPIC)(CLI(chanserv), cn, "El canal ha sido registrado.");
			}
			senyal1(CS_SIGN_REG, param[1]);
		}
		response(cl, CLI(chanserv), "El canal \00312%s\003 ha sido registrado.", param[1]);
	}
	else
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, IsOper(cl) ? "REGISTER #canal [pass descripción]" : "REGISTER #canal pass descripción tokens");
		return 1;
	}
	return 0;
}
BOTFUNC(chanserv_token)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int libres = 25, i; /* siempre tendremos 25 tokens libres */
	if ((atol(_mysql_get_registro(NS_MYSQL, parv[0], "reg")) + 86400 * chanserv.antig) > time(0))
	{
		char buf[512];
		sprintf_irc(buf, "Tu nick debe tener una antigüedad de como mínimo %i días.", chanserv.antig);
		response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}
	if ((res = _mysql_query("SELECT nick from %s%s where nick='%s'", PREFIJO, CS_TOK, parv[0])))
	{
		char buf[512];
		mysql_free_result(res);
		sprintf_irc(buf, "Ya has efectuado alguna operación referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv.vigencia);
		response(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}
	res = _mysql_query("SELECT * from %s%s where hora='0'", PREFIJO, CS_TOK);
	if (res)
		libres = 25 - (int)mysql_num_rows(res); /* ya que sera ocupada */
	for (i = 0; i < libres; i++)
	{
		char buf[512];
		sprintf_irc(buf, "%lu", rand());
		_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, CS_TOK, cifranick(buf, buf));
	}
	if (res)
		mysql_free_result(res);
	res = _mysql_query("SELECT item from %s%s where hora='0'", PREFIJO, CS_TOK);
	row = mysql_fetch_row(res);
	_mysql_add(CS_TOK, row[0], "hora", "%i", time(0));
	_mysql_add(CS_TOK, row[0], "nick", parv[0]);
	response(cl, CLI(chanserv), "Tu token es \00312%s\003. Guarda este token ya que será necesario para futuras operaciones y no podrás pedir otro hasta dentro de unas horas.", row[0]);
	mysql_free_result(res);
	return 0;
}
#ifdef UDB
BOTFUNC(chanserv_migrar)
{
	int opts;
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "MIGRAR #canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!es_fundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(_mysql_get_registro(CS_MYSQL, param[1], "pass"), MDString(param[2])))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(CS_MYSQL, param[1], "opts"));
	if (opts & CS_OPT_UDB)
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya está migrado.");
		return 1;
	}
	envia_canal_bdd(param[1]);
	response(cl, CLI(chanserv), "Migración realizada.");
	opts |= CS_OPT_UDB;
	_mysql_add(CS_MYSQL, param[1], "opts", "%i", opts);
	return 0;
}
BOTFUNC(chanserv_demigrar)
{
	int opts;
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "DEMIGRAR #canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!es_fundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(_mysql_get_registro(CS_MYSQL, param[1], "pass"), MDString(param[2])))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(CS_MYSQL, param[1], "opts"));
	if (!(opts & CS_OPT_UDB))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está migrado.");
		return 1;
	}
	envia_registro_bdd("C::%s", param[1]);
	response(cl, CLI(chanserv), "Demigración realizada.");
	opts &= ~CS_OPT_UDB;
	_mysql_add(CS_MYSQL, param[1], "opts", "%i", opts);
	return 0;
}
BOTFUNC(chanserv_proteger)
{
	if (params < 3)
	{
		response(cl, CLI(chanserv), CS_ERR_PARA, "PROTEGER #canal {+|-}nick");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!tiene_nivel(parv[0], param[1], CS_LEV_EDT))
	{
		response(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!IsChanUDB(param[1]))
	{
		response(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está migrado.");
		return 1;
	}
	if (BadPtr(param[2] + 1))
	{
		response(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER #canal {+|-}nick");
		return 1;
	}
	if (*param[2] == '+')
	{
		envia_registro_bdd("C::%s::accesos::%s -", param[1], param[2] + 1);
		response(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido añadida.", param[2] + 1);
	}
	else if (*param[2] == '-')
	{
		envia_registro_bdd("C::%s::accesos::%s", param[1], param[2] + 1);
		response(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido eliminada.", param[2] + 1);
	}
	else
	{
		response(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER {+|-}nick");
		return 1;
	}
	return 0;
}
#endif
int chanserv_mode(Cliente *cl, Canal *cn, char *mods[], int max)
{
	modebuf[0] = parabuf[0] = '\0';
	if (IsChanReg(cn->nombre))
	{
		int opts;
		char *modlock, *k;
		opts = atoi(_mysql_get_registro(CS_MYSQL, cn->nombre, "opts"));
		modlock = _mysql_get_registro(CS_MYSQL, cn->nombre, "modos");
		if (opts & CS_OPT_RMOD && modlock)
		{
			strcat(modebuf, strtok(modlock, " "));
			if ((modlock = strtok(NULL, " ")))
			{
				strcat(parabuf, modlock);
				strcat(parabuf, " ");
			}
			/* buscamos el -k */
			k = strchr(modebuf, '-');
			if (k && strchr(k, 'k'))
				port_func(P_MODO_CANAL)(CLI(chanserv), cn, "-k %s", mods[max-1]); /* siempre está en el último */
		}
		if (opts & CS_OPT_SOP)
		{
			int pars = 1;
			char *modos, f = ADD;
			modos = mods[0];
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
					case 'o':
						if (f == ADD)
						{
							if (!tiene_nivel(mods[pars], cn->nombre, CS_LEV_AOP))
							{
								strcat(modebuf, "-o");
								strcat(parabuf, TRIO(busca_cliente(mods[pars], NULL)));
								strcat(parabuf, " ");
							}
						}
						pars++;
						break;
					case 'h':
						if (f == ADD)
						{
							if (!tiene_nivel(mods[pars], cn->nombre, CS_LEV_AHA))
								{
								strcat(modebuf, "-h");
								strcat(parabuf, TRIO(busca_cliente(mods[pars], NULL)));
								strcat(parabuf, " ");
							}
						}
						pars++;
						break;
					case 'a':
						if (f == ADD)
						{
							if (!tiene_nivel(mods[pars], cn->nombre, CS_LEV_AAD))
							{
								strcat(modebuf, "-a");
								strcat(parabuf, TRIO(busca_cliente(mods[pars], NULL)));
								strcat(parabuf, " ");
							}
						}
						pars++;
						break;
					case 'q':
					case 'b':
					case 'e':
					case 'v':
					case 'L':
					case 'f':
					case 'k':
						pars++;
						break;
					case 'l':
						if (f == ADD)
							pars++;
						break;
				}
				modos++;
			}
		}
	}
	if (modebuf[0] != '\0')
		port_func(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", modebuf, parabuf);
	return 0;
}
int chanserv_topic(Cliente *cl, Canal *cn, char *topic)
{
	int opts;
	if (IsChanReg(cn->nombre))
	{
		char *topic;
		opts = atoi(_mysql_get_registro(CS_MYSQL, cn->nombre, "opts"));
		topic = _mysql_get_registro(CS_MYSQL, cn->nombre, "topic");
		if (opts & CS_OPT_KTOP)
		{
			if (topic && strcmp(cn->topic, topic))
				port_func(P_TOPIC)(CLI(chanserv), cn, topic);
		}
		else if (opts & CS_OPT_RTOP)
		{
			_mysql_add(CS_MYSQL, cn->nombre, "topic", topic);
			_mysql_add(CS_MYSQL, cn->nombre, "ntopic", cl->nombre);
		}
	}	
	return 0;
}
int chanserv_kick(Cliente *cl, Cliente *al, Canal *cn, char *motivo)
{
	if (IsChanReg(cn->nombre))
	{
		if (tiene_nivel(al->nombre, cn->nombre, CS_LEV_REV) && strcasecmp(cl->nombre, _mysql_get_registro(CS_MYSQL, cn->nombre, "founder")))
			port_func(P_KICK)(cl, CLI(chanserv), cn, "KICK revenge!");
	}	
	return 0;
}
int chanserv_join(Cliente *cl, Canal *cn)
{
	char find, *forb;
	MYSQL_RES *res;
	MYSQL_ROW row;
	u_long aux, nivel;
	int opts;
	char *entry, *modos, *topic, *susp;
	Akick *akicks;
	if (!IsOper(cl) && (forb = IsChanForbid(cn->nombre)))
	{
		if (port_existe(P_PART_USUARIO_REMOTO))
		{
			port_func(P_PART_USUARIO_REMOTO)(cl, cn, "Este canal está \2PROHIBIDO\2: %s", forb);
			port_func(P_NOTICE)(cl, CLI(chanserv), "Este canal está \2PROHIBIDO\2: %s", forb);
		}
		return 0;
	}
	if (IsChanReg(cn->nombre))
	{
		find = 0;
		res = _mysql_query("SELECT opts,entry,modos,topic from %s%s where item='%s'", PREFIJO, CS_MYSQL, cn->nombre);
		row = mysql_fetch_row(res);
		mysql_free_result(res);
		opts = atoi(row[0]);
		entry = strdup(row[1]);
		modos = strdup(row[2]);
		topic = strdup(row[3]);
		aux = 0L;
		buf[0] = '\0';
		if ((susp = IsChanSuspend(cn->nombre)))
		{
			port_func(P_NOTICE)(cl, CLI(chanserv), "Este canal está suspendido: %s", susp);
			if (!IsOper(cl))
				return 0;
		}
		aux = flags2modes(&aux, modos, protocolo->cmodos);
		if (((opts & CS_OPT_RMOD) && (((aux & MODE_OPERONLY) && !IsOper(cl)) || 
			((aux & MODE_ADMONLY) && !IsAdmin(cl)) ||
			((aux & MODE_RGSTRONLY) && !IsId(cl)))) ||
			(((opts & CS_OPT_SEC) && !IsPreo(cl) && (!tiene_nivel(cl->nombre, cn->nombre, 0L) || !IsId(cl)))))
		{
			if (port_existe(P_PART_USUARIO_REMOTO))
				port_func(P_PART_USUARIO_REMOTO)(cl, cn, NULL);
			return 0;
		}
		akicks = busca_akicks(cn->nombre);
		if (akicks)
		{
			int i;
			for (i = 0; i < akicks->akicks; i++)
			{
				if (!match(ircdmask(akicks->akick[i]->nick), cl->mask))
				{
					find = 1;
					port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+b %s", mascara(cl->mask, chanserv.bantype));
					port_func(P_KICK)(cl, CLI(chanserv), cn, "%s (%s)", akicks->akick[i]->motivo, akicks->akick[i]->puesto);
					break;
				}
			}
		}
		if (find)
			return 0;
		nivel = IsId(cl) ? tiene_nivel(cl->nombre, cn->nombre, 0L) : 0L;
		buf[0] = '\0';
		tokbuf[0] = '\0';
		if (es_fundador(cl, cn->nombre) && IsId(cl))
		{
			strcat(buf, "q");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AAD)
		{
			strcat(buf, "a");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AOP)
		{
			strcat(buf, "o");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AHA)
		{
			strcat(buf, "h");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AVO)
		{
			strcat(buf, "v");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel)
			_mysql_add(CS_MYSQL, cn->nombre, "ultimo", "%lu", time(0));
		//if (buf[0])
		//	tokbuf[strlen(tokbuf)-1] = '\0';
		if (cn->miembros == 1)
		{
			strcat(buf, "r");
			if (opts & CS_OPT_RTOP)
				port_func(P_TOPIC)(CLI(chanserv), cn, topic);
			if (opts & CS_OPT_RMOD)
			{
				strcat(buf, strtok(modos, " "));
				strcat(tokbuf, strtok(NULL, " "));
			}
		}
		if (buf[0])
			port_func(P_MODO_CANAL)(CLI(chanserv), cn, "+%s %s", buf, tokbuf);
		if (!BadPtr(entry))
			port_func(P_NOTICE)(cl, CLI(chanserv), entry);
		Free(entry);
		Free(modos);
		Free(topic);
	}
	return 0;
}
int chanserv_sig_mysql()
{
	if (!_mysql_existe_tabla(CS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`item` text NOT NULL, "
			"`founder` text NOT NULL, "
  			"`pass` text NOT NULL, "
  			"`descripcion` longtext NOT NULL, "
  			"`modos` varchar(255) NOT NULL default '+nt', "
  			"`opts` int(11) NOT NULL default '0', "
  			"`topic` varchar(255) NOT NULL default 'El canal ha sido registrado.', "
  			"`entry` varchar(255) NOT NULL default 'El canal ha sido registrado.', "
  			"`registro` bigint(20) NOT NULL default '0', "
  			"`ultimo` bigint(20) NOT NULL default '0', "
  			"`ntopic` text NOT NULL, "
  			"`limite` tinyint(4) NOT NULL default '5', "
  			"`akick` text NOT NULL, "
  			"`bjoins` int(11) NOT NULL default '0', "
#ifndef UDB
  			"`suspend` text NOT NULL, "
#endif
  			"`url` text NOT NULL, "
  			"`email` text NOT NULL, "
  			"`memos` text NOT NULL, "
  			"`regs` text NOT NULL, "
  			"KEY `item` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de canales';", PREFIJO, CS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_MYSQL);
	}
	if (!_mysql_existe_tabla(CS_TOK))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
			"`item` varchar(255) default NULL, "
			"`nick` varchar(255) default NULL, "
			"`hora` int(11) NOT NULL default '0', "
			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de tokens para canales';", PREFIJO, CS_TOK))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_TOK);
	}
#ifndef UDB
	if (!_mysql_existe_tabla(CS_FORBIDS))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`motivo` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de canales prohibidos';", PREFIJO, CS_FORBIDS))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_FORBIDS);
	}
#endif
	_mysql_carga_tablas();
	chanserv_carga_registros();
	chanserv_carga_akicks();
	return 1;
}
int chanserv_sig_eos()
{
	/* metemos los bots en los canales que lo hayan soliticado */
	MYSQL_RES *res;
	MYSQL_ROW row;
	int bjoins;
	Cliente *bl;
#ifdef UDB
	envia_registro_bdd("S::ChanServ %s", chanserv.hmod->mascara);
#endif
	if ((res = _mysql_query("SELECT item,bjoins from %s%s where bjoins !='0'", PREFIJO, CS_MYSQL)))
	{
		while ((row = mysql_fetch_row(res)))
		{
			Modulo *ex;
			bjoins = atoi(row[1]);
			for (ex = modulos; ex; ex = ex->sig)
			{
				if ((bjoins & ex->id) && !es_residente(ex, row[0]))
				{
					bl = busca_cliente(ex->nick, NULL);
					if (bl)
						mete_bot_en_canal(bl, row[0]);
				}
			}
		}
		mysql_free_result(res);
	}
	return 0;
}
int chanserv_baja(char *canal, char opt)
{
	Canal *an;
	Akick *akick;
	char *tok, *regs;
	if (!opt && atoi(_mysql_get_registro(CS_MYSQL, canal, "opts")) & CS_OPT_NODROP)
		return 1;
	an = busca_canal(canal, NULL);
	if (an)
	{
		port_func(P_MODO_CANAL)(CLI(chanserv), an, "-r");
		if (RedOverride)
			saca_bot_de_canal(CLI(chanserv), canal, NULL);
	}
	if ((regs = _mysql_get_registro(CS_MYSQL, canal, "regs")))
	{
		for (tok = strtok(regs, ":"); tok; tok = strtok(NULL, ":"))
		{
			borra_cregistro(tok, canal);
			strtok(NULL, " ");
		}
	}
	if ((akick = busca_akicks(canal)))
	{
		int akicks = akick->akicks;
		while (akicks)
		{
			borra_akick(akick->akick[0]->nick, canal);
			akicks--;
		}
	}
#ifdef UDB
	if (IsChanUDB(canal))
		envia_registro_bdd("C::%s", canal);
#endif
	senyal1(CS_SIGN_DROP, canal);
	_mysql_del(CS_MYSQL, canal);
	return 0;
}
CsRegistros *busca_cregistro(char *nick)
{
	CsRegistros *aux;
	u_int hash;
	hash = hash_cliente(nick);
	for (aux = (CsRegistros *)csregs[hash].item; aux; aux = aux->sig)
	{
		if (!strcasecmp(aux->nick, nick))
			return aux;
	}
	return NULL;
}
Akick *busca_akicks(char *canal)
{
	Akick *aux;
	u_int hash;
	hash = hash_canal(canal);
	for (aux = (Akick *)akicks[hash].item; aux; aux = aux->sig)
	{
		if (!strcasecmp(aux->canal, canal))
			return aux;
	}
	return NULL;
}
void inserta_cregistro(char *nick, char *canal, u_long flag)
{
	CsRegistros *creg;
	struct _subc *sub;
	u_int hash = 0;
	if (!(creg = busca_cregistro(nick)))
	{
		creg = (CsRegistros *)Malloc(sizeof(CsRegistros));
		creg->subs = 0;
		creg->nick = strdup(nick);
	}
	hash = hash_cliente(nick);
	sub = (struct _subc *)Malloc(sizeof(struct _subc));
	sub->canal = strdup(canal);
	sub->flags = flag;
	creg->sub[creg->subs++] = sub;
	if (!busca_cregistro(nick))
	{
		creg->sig = csregs[hash].item;
		csregs[hash].item = creg;
		csregs[hash].items++;
	}
}
Akick *inserta_akick(char *nick, char *canal, char *emisor, char *motivo)
{
	Akick *akick;
	struct _akc *akc;
	u_int hash = 0;
	if (!(akick = busca_akicks(canal)))
	{
		akick = (Akick *)Malloc(sizeof(Akick));
		akick->akicks = 0;
		akick->canal = strdup(canal);
	}
	hash = hash_canal(canal);
	akc = (struct _akc *)Malloc(sizeof(struct _akc));
	akc->nick = strdup(nick);
	akc->motivo = strdup(motivo);
	akc->puesto = strdup(emisor);
	akick->akick[akick->akicks++] = akc;
	if (!busca_akicks(canal))
	{
		akick->sig = akicks[hash].item;
		akicks[hash].item = akick;
		akicks[hash].items++;
	}
	return akick;
}
int borra_cregistro_de_hash(CsRegistros *creg, char *nick)
{
	u_int hash;
	CsRegistros *aux, *prev = NULL;
	hash = hash_cliente(nick);
	for (aux = (CsRegistros *)csregs[hash].item; aux; aux = aux->sig)
	{
		if (aux == creg)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				csregs[hash].item = aux->sig;
			csregs[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void borra_cregistro(char *nick, char *canal)
{
	CsRegistros *creg;
	int i;
	if (!(creg = busca_cregistro(nick)))
		return;
	for (i = 0; i < creg->subs; i++)
	{
		if (!strcasecmp(creg->sub[i]->canal, canal))
		{
			/* liberamos el canal en cuestión */
			Free(creg->sub[i]->canal);
			Free(creg->sub[i]);
			/* ahora tenemos vía libre para desplazar la lista para abajo */
			for (; i < creg->subs; i++)
				creg->sub[i] = creg->sub[i+1];
			if (!(--creg->subs))
			{
				borra_cregistro_de_hash(creg, nick);
				Free(creg->nick);
				Free(creg);
			}
			return;
		}
	}
}
int borra_akick_de_hash(Akick *akick, char *canal)
{
	u_int hash;
	Akick *aux, *prev = NULL;
	hash = hash_canal(canal);
	for (aux = (Akick *)akicks[hash].item; aux; aux = aux->sig)
	{
		if (aux == akick)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				akicks[hash].item = aux->sig;
			akicks[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void borra_akick(char *nick, char *canal)
{
	Akick *akick;
	int i;
	if (!(akick = busca_akicks(canal)))
		return;
	for (i = 0; i < akick->akicks; i++)
	{
		if (!strcasecmp(akick->akick[i]->nick, nick))
		{
			Free(akick->akick[i]->motivo);
			Free(akick->akick[i]->puesto);
			Free(akick->akick[i]->nick);
			Free(akick->akick[i]);
			for (; i < akick->akicks; i++)
				akick->akick[i] = akick->akick[i+1];
			if (!(--akick->akicks))
			{
				borra_akick_de_hash(akick, canal);
				Free(akick->canal);
				Free(akick);
			}
			return;
		}
	}
}
int chanserv_dropanick(char *nick)
{
	CsRegistros *regs;
	regs = busca_cregistro(nick);
	if (regs)
	{
		int subs = regs->subs;
		while(subs)
		{
			char tmp[512];
			strncpy(tmp, regs->sub[0]->canal, 512);
			borra_acceso(nick, tmp);
			subs--;
		}
	}
	return 0;
}
void chanserv_carga_registros()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	res = _mysql_query("SELECT item,regs from %s%s", PREFIJO, CS_MYSQL);
	if (!res)
		return;
	while ((row = mysql_fetch_row(res)))
	{
		char *tok;
		for (tok = strtok(row[1], ":"); tok; tok = strtok(NULL, ":"))
			inserta_cregistro(tok, row[0], atol(strtok(NULL, " ")));
	}
	mysql_free_result(res);
}
void chanserv_carga_akicks()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	res = _mysql_query("SELECT item,akick from %s%s", PREFIJO, CS_MYSQL);
	if (!res)
		return;
	while ((row = mysql_fetch_row(res)))
	{
		char *tok, *mo, *au;
		for (tok = strtok(row[1], "\1"); tok; tok = strtok(NULL, "\1"))
		{
			mo = strtok(NULL, "\1");
			au = strtok(NULL, "\t");
			inserta_akick(tok, row[0], au, mo);
		}
	}
	mysql_free_result(res);
}
u_long tiene_nivel(char *nick, char *canal, u_long flag)
{
	CsRegistros *aux;
	Cliente *al;
	int i;
	al = busca_cliente(nick, NULL);
	if ((!IsOper(al) && IsChanSuspend(canal)) || !IsId(al))
		return 0L;
	if (es_fundador(al, canal))
		return CS_LEV_ALL;
	if ((aux = busca_cregistro(nick)))
	{
		for (i = 0; i < aux->subs; i++)
		{
			if (!strcasecmp(aux->sub[i]->canal, canal))
			{
				u_long nivel = 0L;
				if (IsPreo(al))
					nivel |= (CS_LEV_ALL & ~CS_LEV_MOD);
				nivel |= aux->sub[i]->flags;		
				if (flag)
					return (nivel & flag);
				else
					return aux->sub[i]->flags;
			}
		}
	}
	if (IsPreo(al))
		return (CS_LEV_ALL & ~CS_LEV_MOD);
	return 0L;
}
int chanserv_dropachans(Proc *proc)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = _mysql_query("SELECT item from %s%s where ultimo < %i AND ultimo !='0' LIMIT %i,30", PREFIJO, CS_MYSQL, time(0) - 86400 * chanserv.autodrop, proc->proc)) || !mysql_num_rows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			_mysql_query("DELETE from %s%s where hora < %i AND hora !='0'", PREFIJO, CS_TOK, time(0) - 3600 * chanserv.vigencia);
			return 1;
		}
		while ((row = mysql_fetch_row(res)))
		{
			if (atoi(row[1]) + 86400 * chanserv.autodrop < time(0))
				chanserv_baja(row[0], 0);
		}
		proc->proc += 30;
		mysql_free_result(res);
	}
	return 0;
}
int ChanReg(char *canal)
{
	char *res;
	if (!canal)
		return 0;
	res = _mysql_get_registro(CS_MYSQL, canal, "registro");
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
#ifdef UDB
	Udb *bloq, *reg;
#else
	char *motivo;
#endif
	if (!canal)
		return NULL;
#ifdef UDB
	if ((reg = busca_registro(BDD_CHANS, canal)) && (bloq = busca_bloque("suspendido", reg)))
		return bloq->data_char;
#else
	if ((motivo = _mysql_get_registro(CS_MYSQL, canal, "suspend")))
		return motivo;
#endif
	return NULL;
}
char *IsChanForbid(char *canal)
{
#ifdef UDB
	Udb *bloq, *reg;
#else
	char *motivo;
#endif
	if (!canal)
		return NULL;
#ifdef UDB
	if ((reg = busca_registro(BDD_CHANS, canal)) && (bloq = busca_bloque("forbid", reg)))
		return bloq->data_char;
#else
	if ((motivo = _mysql_get_registro(CS_FORBIDS, canal, "motivo")))
		return motivo;
#endif
	return NULL;
}
int chanserv_sig_synch()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (RedOverride)
	{
		if ((res = _mysql_query("SELECT item from %s%s", PREFIJO, CS_MYSQL)))
		{
			while ((row = mysql_fetch_row(res)))
					mete_bot_en_canal(CLI(chanserv), row[0]);
			mysql_free_result(res);
		}
	}
	return 0;
}
