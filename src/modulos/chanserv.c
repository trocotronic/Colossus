/*
 * $Id: chanserv.c,v 1.26 2005-10-19 16:30:29 Trocotronic Exp $ 
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
#define ExFunc(x) BuscaFuncion(chanserv.hmod, x, NULL)
Hash csregs[UMAX];
Hash akicks[CHMAX];

BOTFUNC(CSHelp);
BOTFUNC(CSDeauth);
BOTFUNC(CSDrop);
BOTFUNC(CSIdentify);
BOTFUNC(CSInfo);
BOTFUNC(CSInvite);
BOTFUNC(CSModos);
BOTFUNC(CSClear);
BOTFUNC(CSOpts);
BOTFUNC(CSAkick);
BOTFUNC(CSAccess);
BOTFUNC(CSList);
BOTFUNC(CSJb);
BOTFUNC(CSSendpass);
BOTFUNC(CSSuspender);
BOTFUNC(CSLiberar);
BOTFUNC(CSForbid);
BOTFUNC(CSUnforbid);
BOTFUNC(CSBlock);
BOTFUNC(CSRegister);
BOTFUNC(CSToken);
#ifdef UDB
BOTFUNC(CSMigrar);
BOTFUNC(CSDemigrar);
BOTFUNC(CSProteger);
#endif

int CSSigSQL	();
int CSSigEOS	();
int CSSigSynch	();
int CSSigPreNick (Cliente *, char *);

ProcFunc(CSDropachans);
int CSDropanick(char *);
int CSBaja(char *, char);

int CSCmdMode(Cliente *, Canal *, char *[], int);
int CSCmdJoin(Cliente *, Canal *);
int CSCmdKick(Cliente *, Cliente *, Canal *, char *);
int CSCmdTopic(Cliente *, Canal *, char *);

void CSCargaRegistros(void);
void CSCargaAkicks(void);
void CSInsertaRegistro(char *, char *, u_long);
void CSBorraRegistro(char *, char *);
Akick *CSInsertaAkick(char *, char *, char *, char *);
void CSBorraAkick(char *, char *);
Akick *CSBuscaAkicks(char *);
int CSEsAkick(char *, char *);
int CSEsRegistro(char *, char *);
int CSTest(Conf *, int *);
void CSSet(Conf *, Modulo *);

static bCom chanserv_coms[] = {
	{ "help" , CSHelp , TODOS } ,
	{ "deauth" , CSDeauth , USERS } ,
	{ "drop" , CSDrop , OPERS } ,
	{ "identify" , CSIdentify , USERS } ,
	{ "info" , CSInfo , TODOS } ,
	{ "invite" , CSInvite , USERS } ,
	{ "admin" , CSModos , USERS } ,
	{ "deadmin" , CSModos , USERS } ,
	{ "op" , CSModos , USERS } ,
	{ "deop" , CSModos , USERS } ,
	{ "half" , CSModos , USERS } ,
	{ "dehalf" , CSModos , USERS } ,
	{ "voice" , CSModos , USERS } ,
	{ "devoice" , CSModos , USERS } ,
	{ "kick" , CSModos , USERS } ,
	{ "ban" , CSModos , USERS } ,
	{ "unban" , CSModos , USERS } ,
	{ "clear" , CSClear , USERS } ,
	{ "set" , CSOpts , USERS } ,
	{ "akick" , CSAkick , USERS } ,
	{ "access" , CSAccess , USERS } ,
	{ "list" , CSList , TODOS } ,
	{ "join" , CSJb , USERS } ,
	{ "sendpass" , CSSendpass , PREOS } ,
	{ "suspender" , CSSuspender , OPERS } ,
	{ "liberar" , CSLiberar , OPERS } ,
	{ "forbid" , CSForbid , ADMINS } ,
	{ "unforbid" , CSUnforbid , ADMINS } ,
	{ "block" , CSBlock , PREOS } ,
	{ "register" , CSRegister , USERS } ,
	{ "token" , CSToken , USERS } ,
#ifdef UDB
	{ "migrar" , CSMigrar , USERS } ,
	{ "demigrar" , CSDemigrar , USERS } ,
	{ "proteger" , CSProteger , USERS } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};
DLLFUNC mTab cFlags[] = {
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
	{ 0x0 , '0' }
};

ModInfo MOD_INFO(ChanServ) = {
	"ChanServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@rallados.net" 
};
	
int MOD_CARGA(ChanServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuraci�n para %s", mod->archivo, MOD_INFO(ChanServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(ChanServ).nombre);
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
					Error("[%s] La configuraci�n de %s no ha pasado el test", mod->archivo, MOD_INFO(ChanServ).nombre);
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
	DetieneProceso(CSDropachans);
	BotUnset(chanserv);
	return 0;
}
int CSTest(Conf *config, int *errores)
{
	return 0;
}
void CSSet(Conf *config, Modulo *mod)
{
	int i, p;
	chanserv.autodrop = 15;
	chanserv.maxlist = 30;
	chanserv.bantype = 3;
	ircstrdup(chanserv.tokform, "##########");
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
			ProcesaComsMod(config->seccion[i], mod, chanserv_coms);
		if (!strcmp(config->seccion[i]->item, "tokens"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "formato"))
					ircstrdup(chanserv.tokform, config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "vigencia"))
					chanserv.vigencia = atoi(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "necesarios"))
					chanserv.necesarios = atoi(config->seccion[i]->seccion[p]->data);
			}
		}
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
	IniciaProceso(CSDropachans);
	BotSet(chanserv);
}
int CSEsFundador(Cliente *al, char *canal)
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
	if ((cache = CogeCache(CACHE_FUNDADORES, al->nombre, chanserv.hmod->id)))
	{
		sprintf(buf, "%s ", canal);
		if (strstr(cache, buf))
			return cache;
	}
	return NULL;
}
int CSEsResidente(Modulo *mod, char *canal)
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
u_long CSBorraAcceso(char *usuario, char *canal)
{
	char *registros, *tok;
	u_long prev = 0L;
	if ((registros = SQLCogeRegistro(CS_SQL, canal, "regs")))
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
		CSBorraRegistro(usuario, canal);
		SQLInserta(CS_SQL, canal, "regs", buf);
	}
	return prev;
}
void CSBorraAkickDb(char *nick, char *canal)
{
	char *registros, *tok;
	if ((registros = SQLCogeRegistro(CS_SQL, canal, "akick")))
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
		CSBorraAkick(nick, canal);
		SQLInserta(CS_SQL, canal, "akick", buf);
	}
}
void CSInsertaAkickDb(char *nick, char *canal, char *emisor, char *motivo)
{
	Akick *aux;
	int i;
	aux = CSInsertaAkick(nick, canal, emisor, motivo);
	buf[0] = '\0';
	ircsprintf(buf, "%s\1%s\1%s", aux->akick[0].nick, aux->akick[0].motivo, aux->akick[0].puesto);
	for (i = 1; i < aux->akicks; i++)
	{
		char *tmp;
		tmp = (char *)Malloc(sizeof(char) * (strlen(aux->akick[i].nick) + strlen(aux->akick[i].motivo) + strlen(aux->akick[i].puesto) + 4));
		ircsprintf(tmp, "\t%s\1%s\1%s", aux->akick[i].nick, aux->akick[i].motivo, aux->akick[i].puesto);
		strcat(buf, tmp);
		Free(tmp);
	}
	SQLInserta(CS_SQL, canal, "akick", buf);
}
int CSEsAkick(char *nick, char *canal)
{
	Akick *aux;
	int i;
	if ((aux = CSBuscaAkicks(canal)))
	{
		for (i = 0; i < aux->akicks; i++)
		{
			if (!strcasecmp(aux->akick[i].nick, nick))
				return 1;
		}
	}
	return 0;
}
int CSEsRegistro(char *nick, char *canal)
{
	CsRegistros *aux;
	int i;
	if ((aux = busca_cregistro(nick)))
	{
		for (i = 0; i < aux->subs; i++)
		{
			if (!strcasecmp(aux->sub[i].canal, canal))
				return 1;
		}
	}
	return 0;
}
#ifdef UDB
void CSPropagaCanal(char *canal)
{
	SQLRes res;
	SQLRow row;
	char *modos, *c;
	res = SQLQuery("SELECT founder,modos,topic,pass from %s%s where LOWER(item)='%s'", PREFIJO, CS_SQL, strtolower(canal));
	if (!res)
		return;
	row = SQLFetchRow(res);
	modos = row[1];
	if ((c = strchr(modos, '+')))
		modos = c+1;
	if ((c = strchr(modos, '-')))
		*c = 0;
	PropagaRegistro("C::%s::F %s", canal, row[0]);
	PropagaRegistro("C::%s::M %s", canal, modos);
	PropagaRegistro("C::%s::T %s", canal, row[2]);
	PropagaRegistro("C::%s::P %s", canal, row[3]);
	//PropagaRegistro("C::%s::D md5", canal);
	SQLFreeRes(res);
}
#endif
/* hace un empaquetamiento de los users de una determinada lista. No olvidar hacer Free() */
Cliente **CSEmpaquetaClientes(Canal *cn, LinkCliente *lista, u_char opers)
{
	Cliente **al;
	LinkCliente *lk;
	u_int i, j = 0;
	al = (Cliente **)Malloc(sizeof(Cliente *) * (cn->miembros + 1)); /* como m�ximo */
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
BOTFUNC(CSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), "\00312%s\003 se encarga de gestionar los canales de la red para evitar robos o takeovers.", chanserv.hmod->nick);
		Responde(cl, CLI(chanserv), "El registro de canales permite su apropiaci�n y poder gestionarlos de tal forma que usuarios no deseados "
						"puedan tomar control de los mismos.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Comandos disponibles:");
		FuncResp(chanserv, "INFO", "Muestra distinta informaci�n de un canal.");
		FuncResp(chanserv, "LIST", "Lista canales seg�n un determinado patr�n.");
		if (IsId(cl))
		{
			FuncResp(chanserv, "IDENTIFY", "Te identifica como fundador de un canal.");
			FuncResp(chanserv, "DEAUTH", "Te quita el estado de fundador.");
			FuncResp(chanserv, "INVITE", "Invita a un usuario al canal.");
			if (MODE_ADM && (ExFunc("ADMIN") || ExFunc("DEADMIN")))
				Responde(cl, CLI(chanserv), "\00312ADMIN/DEADMIN\003 Da o quita &.");
			if (ExFunc("OP") || ExFunc("DEOP"))
				Responde(cl, CLI(chanserv), "\00312OP/DEOP\003 Da o quita @.");
			if (MODE_HALF && (ExFunc("HALF") || ExFunc("DEHALF")))
				Responde(cl, CLI(chanserv), "\00312HALF/DEHALF\003 Da o quita %%.");
			if (ExFunc("VOICE") || ExFunc("DEVOICE"))
				Responde(cl, CLI(chanserv), "\00312VOICE/DEVOICE\003 Da o quita +.");
			if (ExFunc("BAN") || ExFunc("UNBAN"))
				Responde(cl, CLI(chanserv), "\00312BAN/UNBAN\003 Pone o quita bans.");
			FuncResp(chanserv, "KICK", "Expulsa a un usuario.");
			FuncResp(chanserv, "CLEAR", "Resetea distintas opciones del canal.");
			FuncResp(chanserv, "SET", "Fija distintas opciones del canal.");
			FuncResp(chanserv, "AKICK", "Gestiona la lista de auto-kick del canal.");
			FuncResp(chanserv, "ACCESS", "Gestiona la lista de accesos.");
			FuncResp(chanserv, "JOIN", "Introduce distintos servicios.");
			FuncResp(chanserv, "REGISTER", "Registra un canal.");
			FuncResp(chanserv, "TOKEN", "Te entrega un token para las operaciones que lo requieran.");
#ifdef UDB
			FuncResp(chanserv, "MIGRAR", "Migra un canal a la base de datos de la red.");
			FuncResp(chanserv, "DEMIGRAR", "Demigra un canal de la base de datos de la red.");
			FuncResp(chanserv, "PROTEGER", "Protege un canal para que s�lo ciertos nicks puedan entrar.");
#endif
		}
		if (IsPreo(cl))
		{
			FuncResp(chanserv, "SENDPASS", "Genera y manda una clave al email del fundador.");
			FuncResp(chanserv, "BLOCK", "Bloquea y paraliza un canal.");
		}
		if (IsOper(cl))
		{
			FuncResp(chanserv, "DROP", "Desregistra un canal.");
			FuncResp(chanserv, "SUSPENDER", "Suspende un canal.");
			FuncResp(chanserv, "LIBERAR", "Quita el suspenso de un canal.");
		}
		if (IsAdmin(cl))
		{
			FuncResp(chanserv, "FORBID", "Prohibe el uso de un canal.");
			FuncResp(chanserv, "UNFORBID", "Quita la prohibici�n de utilizar un canal.");
		}
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Para m�s informaci�n, \00312/msg %s %s comando", chanserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "INFO") && ExFunc("INFO"))
	{
		Responde(cl, CLI(chanserv), "\00312INFO");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Muestra distinta informaci�n de un canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312INFO #canal");
	}
	else if (!strcasecmp(param[1], "LIST") && ExFunc("LIST"))
	{
		Responde(cl, CLI(chanserv), "\00312LIST");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Lista los canales registrados que concuerdan con un patr�n.");
		Responde(cl, CLI(chanserv), "Puedes utilizar comodines (*) para ajustar la b�squeda.");
		Responde(cl, CLI(chanserv), "Adem�s, puedes especificar el par�metro -r para listar los canales en petici�n.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312LIST [-r] patr�n");
	}
	else if (!strcasecmp(param[1], "IDENTIFY") && IsId(cl) && ExFunc("IDENTIFY"))
	{
		Responde(cl, CLI(chanserv), "\00312IDENTIFY");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Te identifica como fundador de un canal.");
		Responde(cl, CLI(chanserv), "Este comando s�lo debe usarse si el nick que llevas no es el de fundador. "
						"En caso contrario, ya quedas identificado como tal si llevas nick identificado.");
		Responde(cl, CLI(chanserv), "Esta identificaci�n se pierde en caso que te desconectes.");
		Responde(cl, CLI(chanserv), "Para proceder tienes que especificar la contrase�a del canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312IDENTIFY #canal pass");
	}
	else if (!strcasecmp(param[1], "DEAUTH") && IsId(cl) && ExFunc("DEAUTH"))
	{
		Responde(cl, CLI(chanserv), "\00312DEAUTH");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Te borra la identificaci�n que pudieras tener.");
		Responde(cl, CLI(chanserv), "Este comando s�lo puede realizarse si previamente te hab�as identificado con el comando IDENTIFY.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312DEAUTH #canal");
	}
	else if (!strcasecmp(param[1], "INVITE") && IsId(cl) && ExFunc("INVITE"))
	{
		Responde(cl, CLI(chanserv), "\00312INVITE");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Obliga a \00312%s\003 a mandar una invitaci�n para un canal.", chanserv.hmod->nick);
		Responde(cl, CLI(chanserv), "Si no especificas ning�n nick, la invitaci�n se te mandar� a ti.");
		Responde(cl, CLI(chanserv), "Por el contrario, si especificas un nick, se le ser� mandada.");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+i\003 para este canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312INVITE #canal [nick]");
	}
	else if (((!strcasecmp(param[1], "OP") && ExFunc("OP")) ||
		(!strcasecmp(param[1], "DEOP") && ExFunc("DEOP")) || 
		(!strcasecmp(param[1], "HALF") && MODE_HALF && ExFunc("HALF")) || 
		(!strcasecmp(param[1], "DEHALF") && MODE_HALF && ExFunc("DEHALF")) ||
		(!strcasecmp(param[1], "VOICE") && ExFunc("VOICE")) || 
		(!strcasecmp(param[1], "DEVOICE") && ExFunc("DEVOICE")) || 
		(!strcasecmp(param[1], "BAN") && ExFunc("BAN")) || 
		(!strcasecmp(param[1], "UNBAN") && ExFunc("UNBAN")) ||
		(!strcasecmp(param[1], "ADMIN") && MODE_ADM && ExFunc("ADMIN")) ||
		(!strcasecmp(param[1], "DEADMIN") && MODE_ADM && ExFunc("DEADMIN"))
		)&& IsId(cl))
	{
		Responde(cl, CLI(chanserv), "\00312%s", strtoupper(param[1]));
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Ejecuta este comando de forma remota por \00312%s\003 al canal.", chanserv.hmod->nick);
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312%s #canal nick1 [nick2 [nick3 [...nickN]]]", strtoupper(param[1]));
	}
	else if (!strcasecmp(param[1], "KICK") && IsId(cl) && ExFunc("KICK"))
	{
		Responde(cl, CLI(chanserv), "\00312KICK");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Fuerza a \00312%s\003 a expulsar a un usuario.", chanserv.hmod->nick);
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+r\003 para este canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312KICK #canal nick [motivo]");
	}
	else if (!strcasecmp(param[1], "CLEAR") && IsId(cl) && ExFunc("CLEAR"))
	{
		Responde(cl, CLI(chanserv), "\00312CLEAR");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Resetea distintas opciones de un canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Opciones disponibles:");
		if (MODE_ADM)
			Responde(cl, CLI(chanserv), "\00312ADMINS\003 Quita todos los +a del canal.");
		Responde(cl, CLI(chanserv), "\00312OPS\003 Quita todos los +o del canal.");
		if (MODE_HALF)
			Responde(cl, CLI(chanserv), "\00312HALFS\003 Quita todos los +h del canal.");
		Responde(cl, CLI(chanserv), "\00312VOICES\003 Quita todos los +v del canal.");
		Responde(cl, CLI(chanserv), "\00312BANS\003 Quita todos los +b del canal.");
		Responde(cl, CLI(chanserv), "\00312USERS\003 Expulsa a todos los usuarios del canal. Puede especificar un motivo.");
		Responde(cl, CLI(chanserv), "\00312ACCESOS\003 Borra todos los accesos del canal.");
		Responde(cl, CLI(chanserv), "\00312MODOS\003 Quita todos los modos del canal y lo deja en +nt.");
		if (IsOper(cl))
		{
			Responde(cl, CLI(chanserv), "\00312KILL\003 Desconecta a todos los usuarios del canal. Puede especificar un motivo.");
			Responde(cl, CLI(chanserv), "\00312GLINE\003 Pone una GLine a todos los usuarios del canal. Puede especificar un motivo.");
		}
		else
			Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+c\003.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312opcion #canal %s", IsOper(cl) ? "[tiempo]" : "");
	}
	else if (!strcasecmp(param[1], "SET") && IsId(cl) && ExFunc("SET"))
	{
		if (params < 3)
		{
			Responde(cl, CLI(chanserv), "\00312SET");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Fija distintas opciones del canal.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Opciones disponibles:");
			Responde(cl, CLI(chanserv), "\00312DESC\003 Descripci�n del canal.");
			Responde(cl, CLI(chanserv), "\00312URL\003 P�gina web.");
			Responde(cl, CLI(chanserv), "\00312EMAIL\003 Direcci�n de correo.");
			Responde(cl, CLI(chanserv), "\00312BIENVENIDA\003 Mensaje de bienvenida.");
			Responde(cl, CLI(chanserv), "\00312MODOS\003 Modos por defecto.");
			Responde(cl, CLI(chanserv), "\00312OPCIONES\003 Diferentes caracter�sticas.");
			Responde(cl, CLI(chanserv), "\00312TOPIC\003 Topic por defecto.");
			Responde(cl, CLI(chanserv), "\00312PASS\003 Cambia la contrase�a.");
			Responde(cl, CLI(chanserv), "\00312FUNDADOR\003 Cambia el fundador.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+s\003.");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal opcion [par�metros]");
			Responde(cl, CLI(chanserv), "Para m�s informaci�n, \00312/msg %s %s SET opci�n", chanserv.hmod->nick, strtoupper(param[0]));
		}
		else if (!strcasecmp(param[2], "DESC"))
		{
			Responde(cl, CLI(chanserv), "\00312SET DESC");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Fija la descripci�n del canal.");
			Responde(cl, CLI(chanserv), "Esta descripci�n debe ser clara y concisa. Debe reflejar la tem�tica del canal con la m�xima precisi�n.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal DESC descripci�n");
		}
		else if (!strcasecmp(param[2], "URL"))
		{
			Responde(cl, CLI(chanserv), "\00312SET URL");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Asocia una direcci�n web al canal.");
			Responde(cl, CLI(chanserv), "Esta direcci�n es visualizada en el comando INFO.");
			Responde(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal URL [http://pagina.web]");
		}
		else if (!strcasecmp(param[2], "EMAIL"))
		{
			Responde(cl, CLI(chanserv), "\00312SET EMAIL");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Asocia una direcci�n de correco al canal.");
			Responde(cl, CLI(chanserv), "Esta direcci�n es visualizada en el comando INFO.");
			Responde(cl, CLI(chanserv), "Si no se especifica ninguna, es borrada.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal EMAIL [direccion@de.correo]");
		}
		else if (!strcasecmp(param[2], "BIENVENIDA"))
		{
			Responde(cl, CLI(chanserv), "\00312SET BIENVENIDA");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Asocia un mensaje de bienvenida al entrar al canal.");
			Responde(cl, CLI(chanserv), "Cada vez que un usuario entre en el canal se le mandar� este mensaje v�a NOTICE.");
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
			Responde(cl, CLI(chanserv), "Se aceptan par�metros para estos modos. No obstante, los modos oqveharb no se pueden utilizar.");
			Responde(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal MODOS [+modos1-modos2]");
		}
		else if (!strcasecmp(param[2], "OPCIONES"))
		{
			Responde(cl, CLI(chanserv), "\00312SET OPCIONES");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Fija las distintas caracter�sticas del canal.");
			Responde(cl, CLI(chanserv), "Estas caracter�sticas se identifican por flags.");
			Responde(cl, CLI(chanserv), "As� pues, en un mismo comando puedes subir y bajar distintos flags.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Flags disponibles:");
			Responde(cl, CLI(chanserv), "\00312+m\003 Candado de modos: los modos no pueden modificarse.");
			Responde(cl, CLI(chanserv), "\00312+r\003 Retenci�n del topic: se guarda el �ltimo topic utilizado en el canal.");
			Responde(cl, CLI(chanserv), "\00312+k\003 Candado de topic: el topic no puede cambiarse.");
			Responde(cl, CLI(chanserv), "\00312+s\003 Canal seguro: s�lo pueden entrar los que tienen acceso.");
			Responde(cl, CLI(chanserv), "\00312+o\003 Secureops: s�lo pueden tener +a, +o, +h o +v los usuarios con el correspondiente acceso.");
			Responde(cl, CLI(chanserv), "\00312+h\003 Canal oculto: no se lista en el comando LIST.");
			Responde(cl, CLI(chanserv), "\00312+d\003 Canal debug: activa el debug del canal para visualizar los eventos y su autor.");
			if (IsOper(cl))
				Responde(cl, CLI(chanserv), "\00312+n\003 Canal no dropable: s�lo puede droparlo la administraci�n.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal OPCIONES +flags-flags");
			Responde(cl, CLI(chanserv), "Ejemplo: \00312SET #canal OPCIONES +ors-mh");
			Responde(cl, CLI(chanserv), "El canal #canal ahora posee las opciones de secureops, retenci�n de topic y canal seguro. "
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
			Responde(cl, CLI(chanserv), "Cambia la contrase�a del canal.");
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
			Responde(cl, CLI(chanserv), "Adem�s, el nuevo nick deber� estar registrado.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312SET #canal FUNDADOR nick");
		}
		else
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opci�n desconocida.");
	}
	else if (!strcasecmp(param[1], "AKICK") && IsId(cl) && ExFunc("AKICK"))
	{
		Responde(cl, CLI(chanserv), "\00312AKICK");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Gestiona la lista de auto-kick del canal.");
		Responde(cl, CLI(chanserv), "Esta lista contiene las m�scaras prohibidas.");
		Responde(cl, CLI(chanserv), "Si un usuario entra al canal y su m�scara coincide con una de ellas, immediatamente es expulsado "
						"y se le pone un ban para que no pueda entrar.");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+k\003.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal +nick|m�scara [motivo]");
		Responde(cl, CLI(chanserv), "A�ade al nick o la m�scara con el motivo opcional.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal -nick|m�scara");
		Responde(cl, CLI(chanserv), "Borra al nick o la m�scara.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312AKICK #canal");
		Responde(cl, CLI(chanserv), "Lista las diferentes entradas.");
	}
	else if (!strcasecmp(param[1], "ACCESS") && IsId(cl) && ExFunc("ACCESS"))
	{
		Responde(cl, CLI(chanserv), "\00312ACCESS");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Gestiona la lista de accesos del canal.");
		Responde(cl, CLI(chanserv), "Esta lista da privilegios a los usuarios que est�n en ella.");
		Responde(cl, CLI(chanserv), "Estos privilegios se les llama accesos y permiten acceder a distintos comandos.");
		Responde(cl, CLI(chanserv), "El fundador tiene acceso a todos ellos.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Accesos disponibles:");
		Responde(cl, CLI(chanserv), "\00312+s\003 Fijar opciones (SET)");
		Responde(cl, CLI(chanserv), "\00312+e\003 Editar la lista de accesos (ACCESS)");
		Responde(cl, CLI(chanserv), "\00312+l\003 Listar la lista de accesos (ACCESS)");
		if (MODE_ADM)
			Responde(cl, CLI(chanserv), "\00312+a\003 Tiene +a al entrar.");
		Responde(cl, CLI(chanserv), "\00312+o\003 Tiene +o al entrar.");
		if (MODE_HALF)
			Responde(cl, CLI(chanserv), "\00312+h\003 Tiene +h al entrar.");
		Responde(cl, CLI(chanserv), "\00312+v\003 Tiene +v al entrar.");
		Responde(cl, CLI(chanserv), "\00312+r\003 Comandos remotos (OP/DEOP/...)");
		Responde(cl, CLI(chanserv), "\00312+c\003 Resetear opciones (CLEAR)");
		Responde(cl, CLI(chanserv), "\00312+k\003 Gestionar la lista de auto-kicks (AKICK)");
		Responde(cl, CLI(chanserv), "\00312+i\003 Invitar (INVITE)");
		Responde(cl, CLI(chanserv), "\00312+j\003 Entrar bots (JOIN)");
		Responde(cl, CLI(chanserv), "\00312+g\003 Kick de venganza.");
		Responde(cl, CLI(chanserv), "\00312+m\003 Gesti�n de memos al canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "El nick al que se le dan los accesos debe estar registrado previamente.");
		Responde(cl, CLI(chanserv), "Un acceso no engloba otro. Por ejemplo, el tener +e no implica tener +l.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal +nick +flags-flags");
		Responde(cl, CLI(chanserv), "A�ade los accesos al nick.");
		Responde(cl, CLI(chanserv), "Ejemplo: \00312ACCESS #canal +pepito +o-lv");
		Responde(cl, CLI(chanserv), "pepito tendr�a +o al entrar en #canal y se le quitar�an los privilegios de acceder a la lista y autovoz.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal -nick");
		Responde(cl, CLI(chanserv), "Borra todos los accesos de este nick.");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312ACCESS #canal");
		Responde(cl, CLI(chanserv), "Lista todos los usuarios con acceso al canal.");
	}
	else if (!strcasecmp(param[1], "JOIN") && IsId(cl) && ExFunc("JOIN"))
	{
		Responde(cl, CLI(chanserv), "\00312JOIN");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Introduce los servicios al canal de forma permanente.");
		Responde(cl, CLI(chanserv), "Primero se quitan los servicios que pudiera haber para luego meter los que se soliciten.");
		Responde(cl, CLI(chanserv), "Si no se especifica nada, son borrados.");
		Responde(cl, CLI(chanserv), "Para realizar este comando debes tener acceso \00312+j\003.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312JOIN #canal [servicio1 [servicio2 [...servicioN]]]");
	}
	else if (!strcasecmp(param[1], "REGISTER") && IsId(cl) && ExFunc("REGISTER"))
	{
		Responde(cl, CLI(chanserv), "\00312REGISTER");
		Responde(cl, CLI(chanserv), " ");
		if (IsOper(cl))
		{
			Responde(cl, CLI(chanserv), "Registra un canal para que sea controlado por los servicios.");
			Responde(cl, CLI(chanserv), "Este registro permite el control absoluto sobre el mismo.");
			Responde(cl, CLI(chanserv), "Con este comando puedes aceptar dos tipos de canales:");
			Responde(cl, CLI(chanserv), "- Aceptar peticiones de canales presentadas por los usuarios.");
			Responde(cl, CLI(chanserv), "- Registrar tus propios canales sin la necesidad de tokens.");
			Responde(cl, CLI(chanserv), "Aceptar una petici�n significa que registras un canal presentado con su fundador para que pase a sus manos.");
			Responde(cl, CLI(chanserv), "El canal expiar� despues de \00312%i\003 d�as en los que no haya entrado ning�n usuario con acceso.", chanserv.autodrop);
			Responde(cl, CLI(chanserv), "Para listar los canales en petici�n utiliza el comando LIST.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal");
			Responde(cl, CLI(chanserv), "Acepta la petici�n presentada por un usuario.");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripci�n");
			Responde(cl, CLI(chanserv), "Registra tu propio canal.");
		}
		else
		{
			
			Responde(cl, CLI(chanserv), "Solicita el registro de un canal.");
			Responde(cl, CLI(chanserv), "Para hacer una petici�n necesitas presentar como m�nimo \00312%i\003 tokens.", chanserv.necesarios);
			Responde(cl, CLI(chanserv), "Estos tokens act�an como unas firmas que dan el visto bueno para su registro.");
			Responde(cl, CLI(chanserv), "Los tokens provienen de otros usuarios que los solicitan con el comando TOKEN.");
			Responde(cl, CLI(chanserv), "Si solicitas un token, no podr�s presentar una solicitud hasta que no pase un cierto tiempo.");
			Responde(cl, CLI(chanserv), "El canal expiar� despues de \00312%i\003 d�as en los que no haya entrado ning�n usuario con acceso.", chanserv.autodrop);
			Responde(cl, CLI(chanserv), "Para m�s informaci�n consulta la ayuda del comando TOKEN.");
			Responde(cl, CLI(chanserv), " ");
			Responde(cl, CLI(chanserv), "Sintaxis: \00312REGISTER #canal pass descripci�n tokens");
		}
	}
	else if (!strcasecmp(param[1], "TOKEN") && IsId(cl) && ExFunc("TOKEN"))
	{
		Responde(cl, CLI(chanserv), "\00312TOKEN");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Solicita un token.");
		Responde(cl, CLI(chanserv), "Este token es necesario para acceder a comandos que requieran el acuerdo de varios usuarios.");
		Responde(cl, CLI(chanserv), "S�lo puedes solicitar un token cada \00312%i\003 horas. Una vez hayan transcurrido, el token caduca y puedes solicitar otro.", chanserv.vigencia);
		Responde(cl, CLI(chanserv), "Act�a como tu firma personal. Y es �nico.");
		Responde(cl, CLI(chanserv), "Por ejemplo, si alguien solicita el registro de un canal y tu est�s a favor, deber�s darle tu token a qui�n lo solicite para presentarlos "
						"para que as� la administraci�n confirme que cuenta con el m�nimo de usuarios en acuerdo.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312TOKEN");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MIGRAR") && IsId(cl) && ExFunc("MIGRAR"))
	{
		Responde(cl, CLI(chanserv), "\00312MIGRAR");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Migra un canal a la base de datos de la red.");
		Responde(cl, CLI(chanserv), "Una vez migrado, el canal ser� permanente. Es decir, no se borrar� aunque salga el �ltimo usuario.");
		Responde(cl, CLI(chanserv), "Conservar� los modos y el topic cuando entre el primer usuario. A su vez seguir� mostr�ndose en el comando /LIST.");
		Responde(cl, CLI(chanserv), "Adem�s, el founder cuando entr� y tenga el modo +r siempre recibir� los modos +oq, acredit�ndole como tal.");
		Responde(cl, CLI(chanserv), "Finalmente, cualquier usuario que entre en el canal y est� vac�o, si no es el founder se le quitar� el estado de @ (-o).");
		Responde(cl, CLI(chanserv), "Este es independiente de los servicios de red, as� pues asegura su funcionamiento siempre aunque no est�n conectados.");
		Responde(cl, CLI(chanserv), "NOTA: este comando s�lo puede realizarlo el fundador del canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312MIGRAR #canal pass");
	}
	else if (!strcasecmp(param[1], "DEMIGRAR") && IsId(cl) && ExFunc("DEMIGRAR"))
	{
		Responde(cl, CLI(chanserv), "\0012DEMIGRAR");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Elimina este canal de la base de datos de la red.");
		Responde(cl, CLI(chanserv), "El canal volver� a su administraci�n anterior a trav�s de los servicios de red.");
		Responde(cl, CLI(chanserv), "NOTA: este comando s�lo puede realizarlo el fundador del canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312DEMIGRAR #canal pass");
	}
	else if (!strcasecmp(param[1], "PROTEGER") && IsId(cl) && ExFunc("PROTEGER"))
	{
		Responde(cl, CLI(chanserv), "\00312PROTEGER");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Protege un canal por BDD.");
		Responde(cl, CLI(chanserv), "Esta protecci�n consiste en permitir la entrada a determinados nicks.");
		Responde(cl, CLI(chanserv), "Al ser por BDD, aunque los servicios est�n ca�dos, s�lo podr�n entrar los nicks que coincidan con las m�scaras especificadas.");
		Responde(cl, CLI(chanserv), "As� pues, cualquier persona que no figure en esta lista de protegidos, no podr� acceder al canal.");
		Responde(cl, CLI(chanserv), "Si la m�scara especificada es un nick, se a�adir� la m�scara de forma nick!*@*");
		Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+e\003.");
		Responde(cl, CLI(chanserv), "Si no especeificas ninguna m�scara, toda la lista es borrada.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312PROTEGER #canal [+|-[m�scara]]");
	}
#endif
	else if (!strcasecmp(param[1], "SENDPASS") && IsPreo(cl) && ExFunc("SENDPASS"))
	{
		Responde(cl, CLI(chanserv), "\00312SENDPASS");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Regenera una nueva clave para un canal y es enviada al email del fundador.");
		Responde(cl, CLI(chanserv), "Este comando s�lo debe utilizarse a petici�n del fundador.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SENDPASS #canal");
	}
	else if (!strcasecmp(param[1], "BLOCK") && IsPreo(cl) && ExFunc("BLOCK"))
	{
		Responde(cl, CLI(chanserv), "\00312BLOCK");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Inhabilita temporalmente un canal.");
		Responde(cl, CLI(chanserv), "Se quitan todos los privilegios de un canal (+qaohv) y se ponen los modos +imMRlN 1");
		Responde(cl, CLI(chanserv), "Su durada es temporal y puede se quitado mediante el comando CLEAR.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312BLOCK #canal");
	}
	else if (!strcasecmp(param[1], "DROP") && IsOper(cl) && ExFunc("DROP"))
	{
		Responde(cl, CLI(chanserv), "\00312DROP");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Borra un canal de la base de datos.");
		Responde(cl, CLI(chanserv), "Este canal tiene que haber estado registrado previamente.");
		Responde(cl, CLI(chanserv), "Tambi�n puede utilizarse para denegar la petici�n de registro de un canal.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312DROP #canal");
	}
	else if (!strcasecmp(param[1], "SUSPENDER") && IsOper(cl) && ExFunc("SUSPENDER"))
	{
		Responde(cl, CLI(chanserv), "\00312SUSPENDER");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Suspende un canal de ser utilizado.");
		Responde(cl, CLI(chanserv), "El canal seguir� estando registrado pero sus usuarios, fundador inclu�do, no podr�n aprovechar "
						"ninguna de sus prestaciones.");
		Responde(cl, CLI(chanserv), "Este suspenso durar� indefinidamente hasta que se decida levantarlo.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312SUSPENDER #canal motivo");
	}
	else if (!strcasecmp(param[1], "LIBERAR") && IsOper(cl) && ExFunc("LIBERAR"))
	{
		Responde(cl, CLI(chanserv), "\00312LIBERAR");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Quita el suspenso puesto previamente con el comando SUSPENDER.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312LIBERAR #canal");
	}
	else if (!strcasecmp(param[1], "FORBID") && IsAdmin(cl) && ExFunc("FORBID"))
	{
		Responde(cl, CLI(chanserv), "\00312FORBID");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Prohibe el uso de este canal.");
		Responde(cl, CLI(chanserv), "Los usuarios no podr�n ni entrar ni registrarlo.");
		Responde(cl, CLI(chanserv), "Este tipo de canales se extienden por toda la red.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312FORBID #canal motivo");
	}
	else if (!strcasecmp(param[1], "UNFORBID") && IsAdmin(cl) && ExFunc("UNFORBID"))
	{
		Responde(cl, CLI(chanserv), "\00312UNFORBID");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Quita la prohibici�n de un canal puesta con el comando FORBID.");
		Responde(cl, CLI(chanserv), " ");
		Responde(cl, CLI(chanserv), "Sintaxis: \00312UNFORBID #canal");
	}
	else
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opci�n desconocida.");
	return 0;
}
BOTFUNC(CSDrop)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "DROP #canal");
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
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No se ha podido dropear. Comun�quelo a la administraci�n.");
	return 0;
}
BOTFUNC(CSIdentify)
{
	char *cache;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "IDENTIFY #canal pass");
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
	if ((chanserv.opts & CS_SID) && !parv[parc])
	{
		ircsprintf(buf, "Identificaci�n incorrecta. /msg %s@%s IDENTIFY #canal pass", chanserv.hmod->nick, me.nombre);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}	
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contrase�a incorrecta.");
		return 1;
	}
	if (CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya eres fundador de este canal.");
		return 1;
	}
	ircsprintf(buf, "%s ", param[1]);
	if ((cache = CogeCache(CACHE_FUNDADORES, cl->nombre, chanserv.hmod->id)))
	{
		if (strstr(cache, buf))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya te has identificado como fundador de este canal.");
			return 1;
		}
		strcat(buf, cache);
	}
	InsertaCache(CACHE_FUNDADORES, cl->nombre, 0, chanserv.hmod->id, buf);
	Responde(cl, CLI(chanserv), "Ahora eres reconocido como founder de \00312%s\003.", param[1]);
	return 0;
}
BOTFUNC(CSDeauth)
{
	char *cache, *tok;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "DEAUTH #canal");
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
			strcat(buf, tok);
			strcat(buf, " ");
		}
	}
	if (buf[0])
		InsertaCache(CACHE_FUNDADORES, cl->nombre, 0, chanserv.hmod->id, buf);
	Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Ya no est�s identificado como fundador de \00312%s", param[1]);
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
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "INFO #canal");
		return 1;
	}
	if ((forb = IsChanForbid(param[1])))
	{
		Responde(cl, CLI(chanserv), "Este canal est� \2PROHIBIDO\2: %s", forb);
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
	Responde(cl, CLI(chanserv), "*** Informaci�n del canal \00312%s\003 ***", param[1]);
	if ((susp = IsChanSuspend(param[1])))
		Responde(cl, CLI(chanserv), "Estado: \00312SUSPENDIDO: %s", susp);
	else
		Responde(cl, CLI(chanserv), "Estado: \00312ACTIVO");
	Responde(cl, CLI(chanserv), "Fundador: \00312%s", row[1]);
	Responde(cl, CLI(chanserv), "Descripci�n: \00312%s", row[2]);
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
	Responde(cl, CLI(chanserv), "Candado de topic: \00312%s\003 Retenci�n de topic: \00312%s", opts & CS_OPT_KTOP ? "ON" : "OFF", opts & CS_OPT_RTOP ? "ON" : "OFF");
	if (opts & CS_OPT_SOP)
		Responde(cl, CLI(chanserv), "Canal con \2SECUREOPS");
	if (opts & CS_OPT_SEC)
		Responde(cl, CLI(chanserv), "Canal \2SEGURO");
	if (opts & CS_OPT_NODROP)
		Responde(cl, CLI(chanserv), "Canal \2NO DROPABLE");
	reg = (time_t)atol(row[10]);
	Responde(cl, CLI(chanserv), "�ltimo acceso: \00312%s", Fecha(&reg));
	SQLFreeRes(res);
	return 0;
}
BOTFUNC(CSInvite)
{
	Cliente *al = NULL;
	Canal *cn;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "INVITE #canal [nick]");
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
	if (params == 3 && !(al = BuscaCliente(param[2], NULL)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este usuario no est� conectado.");
		return 1;
	}
	cn = BuscaCanal(param[1], NULL);
	ProtFunc(P_INVITE)(al ? al : cl, CLI(chanserv), cn);
	Responde(cl, CLI(chanserv), "El usuario \00312%s\003 ha sido invitado a \00312%s\003.", params == 3 ? param[2] : parv[0], param[1]);
	if (IsChanDebug(param[1]))
		ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "%s invita a %s", parv[0], params == 3 ? param[2] : parv[0]);
	return 0;
}
BOTFUNC(CSModos)
{
	int i, opts;
	Cliente *al;
	Canal *cn;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), "%s #canal par�metros", strtoupper(param[0]));
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
	if (!(cn = BuscaCanal(param[1], NULL)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El canal est� vac�o.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_RMO))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (!strcasecmp(param[0], "ADMIN") && MODE_ADM)
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || EsLink(cn->admin, al))
				continue;
			if (!CSTieneNivel(param[i], param[1], CS_LEV_AHA) && (opts & CS_OPT_SOP))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%c %s", MODEF_ADM, TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEADMIN") && MODE_ADM)
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || !EsLink(cn->admin, al) || (EsBot(al) && !IsOper(cl)))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_ADM, TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "OP"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || EsLink(cn->op, al))
				continue;
			if (!CSTieneNivel(param[i], param[1], CS_LEV_AOP) && (opts & CS_OPT_SOP))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+o %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEOP"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || !EsLink(cn->op, al) || (EsBot(al) && !IsOper(cl)))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "HALF") && MODE_HALF)
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || EsLink(cn->half, al))
				continue;
			if (!CSTieneNivel(param[i], param[1], CS_LEV_AHA) && (opts & CS_OPT_SOP))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%c %s", MODEF_HALF, TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEHALF") && MODE_HALF)
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || !EsLink(cn->half, al) || (EsBot(al) && !IsOper(cl)))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_HALF, TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "VOICE"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || EsLink(cn->voz, al))
				continue;
			if (!CSTieneNivel(param[i], param[1], CS_LEV_AVO) && (opts & CS_OPT_SOP))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+v %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "DEVOICE"))
	{
		for (i = 2; i < params; i++)
		{
			if (!(al = BuscaCliente(param[i], NULL)) || !EsLink(cn->voz, al) || (EsBot(al) && !IsOper(cl)))
				continue;
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-v %s", TRIO(al));
		}
	}
	else if (!strcasecmp(param[0], "BAN"))
	{
		for (i = 2; i < params; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+b %s", MascaraIrcd(param[i]));
	}
	else if (!strcasecmp(param[0], "UNBAN"))
	{
		for (i = 2; i < params; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-b %s", MascaraIrcd(param[i]));
	}
	else if (!strcasecmp(param[0], "KICK"))
	{
		if (!(al = BuscaCliente(param[2], NULL)))
			return 1;
		ProtFunc(P_KICK)(al, CLI(chanserv), cn, Unifica(param, params, 3, -1));
	}
	if (IsChanDebug(param[1]))
		ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "%s hace %s a %s", parv[0], strtoupper(param[0]), Unifica(param, params, 2, -1));
	return 0;
}
BOTFUNC(CSClear)
{
	Canal *cn;
	Cliente **al = NULL;
	int i;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "CLEAR #canal opcion");
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
	if (!(cn = BuscaCanal(param[1], NULL)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El canal est� vac�o.");
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
	else if (!strcasecmp(param[2], "ADMINS") && MODE_ADM)
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
	}
	else if (!strcasecmp(param[2], "ACCESOS"))
	{
		char *regs, *tok;
		if (!IsChanReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
			return 1;
		}
		regs = SQLCogeRegistro(CS_SQL, param[1], "regs");
		for (tok = strtok(regs, ":"); tok; tok = strtok(NULL, ":"))
		{
			CSBorraRegistro(tok, param[1]);
			strtok(NULL, " ");
		}
		SQLInserta(CS_SQL, param[1], "regs", NULL);
		Responde(cl, CLI(chanserv), "Lista de accesos eliminada.");
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%s", ModosAFlags(cn->modos, protocolo->cmodos, cn));
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+nt%c", IsChanReg(param[1]) && MODE_RGSTR ? MODEF_RGSTR : 0);
		Responde(cl, CLI(chanserv), "Modos resetados a +nt.");
	}
	else if (!strcasecmp(param[2], "BANS"))
	{
		while (cn->ban)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-b %s", cn->ban->ban);
	}
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
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "CLEAR #canal GLINE tiempo");
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
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opci�n desconocida.");
		return 1;
	}
	if (IsChanDebug(param[1]))
		ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "%s hace CLEAR %s", parv[0], strtoupper(param[2]));
	ircfree(al);
	return 0;
}
BOTFUNC(CSOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal par�metros");
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
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal DESC descripci�n");
			return 1;
		}
		SQLInserta(CS_SQL, param[1], "descripcion", Unifica(param, params, 3, -1));
		Responde(cl, CLI(chanserv), "Descripci�n cambiada.");
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
#ifndef UDB
			ProtFunc(P_TOPIC)(CLI(chanserv), BuscaCanal(param[1], NULL), topic);
#endif
		}
		else
			Responde(cl, CLI(chanserv), "TOPIC desactivado.");
		SQLInserta(CS_SQL, param[1], "topic", topic);
		SQLInserta(CS_SQL, param[1], "ntopic", params > 3 ? parv[0] : NULL);
#ifdef UDB
		if (IsChanUDB(param[1]))
		{
			if (params > 3)
				PropagaRegistro("C::%s::T %s", param[1], topic);
			else
				PropagaRegistro("C::%s::T", param[1]);
		}
#endif
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
			char forb[32] = "ovb";
			int i;
			if (MODE_HALF)
				chrcat(forb, MODEF_HALF);
			if (MODE_OWNER)
				chrcat(forb, MODEF_OWNER);
			if (MODE_ADM)
				chrcat(forb, MODEF_ADM);
			if (MODE_RGSTR)
				chrcat(forb, MODEF_RGSTR);
			for (i = 0; forb[i] != '\0'; i++)
			{
				if (strchr(param[3], forb[i]))
				{
					ircsprintf(buf, "Los modos %s no se pueden especificar.", forb);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
			}
			modos = Unifica(param, params, 3, -1);
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), BuscaCanal(param[1], NULL), modos);
			Responde(cl, CLI(chanserv), "Modos cambiados.");
			
		}
		else
			Responde(cl, CLI(chanserv), "Modos eliminados.");
		SQLInserta(CS_SQL, param[1], "modos", modos);
#ifdef UDB
		if (IsChanUDB(param[1]))
		{
			char *c, *str;
			strcpy(tokbuf, modos);
			str = strtok(tokbuf, " ");
			if ((c = strchr(str, '-')))
				*c = '\0';
			if (*str == '+')
				str++;
			strcpy(buf, str);
			if ((str = strtok(NULL, " ")))
			{
				strcat(buf, " ");
				strcat(buf, str);
			}
			PropagaRegistro("C::%s::M %s", param[1], buf);
		}
#endif
	}
	else if (!strcasecmp(param[2], "PASS"))
	{
		char *mdpass = MDString(param[3]);
		if (!CSEsFundador(cl, param[1]) && !IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal PASS contrase�a");
			return 1;
		}
		SQLInserta(CS_SQL, param[1], "pass", mdpass);
		Responde(cl, CLI(chanserv), "Contrase�a cambiada.");
#ifdef UDB
		if (IsChanUDB(param[1]))
			PropagaRegistro("C::%s::P %s", param[1], mdpass);
#endif
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
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal FUNDADOR nick");
			return 1;
		}
		if (!IsReg(param[3]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no est� registrado.");
			return 1;
		}
		SQLInserta(CS_SQL, param[1], "founder", param[3]);
#ifdef UDB
		if (IsChanUDB(param[1]))
			PropagaRegistro("C::%s::F %s", param[1], param[3]);
#endif
		Responde(cl, CLI(chanserv), "Fundador cambiado.");
	}
	else if (!strcasecmp(param[2], "OPCIONES"))
	{
		char f = ADD, *modos = param[3], buenos[128];
		int opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
		buenos[0] = 0;
		if (params < 4)
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "SET #canal opciones +-modos");
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
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Opci�n desconocida.");
		return 1;
	}
	return 0;
}
BOTFUNC(CSAkick)
{
	Akick *aux;
	Canal *cn;
	int i;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "AKICK #canal [+-nick|mascara [motivo]]");
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
	if (!(cn = BuscaCanal(param[1], NULL)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "El canal est� vac�o.");
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_ACK))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (params == 2)
	{
		if (!(aux = CSBuscaAkicks(param[1])) || !aux->akicks)
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ning�n akick.");
			return 1;
		}
		for (i = 0; i < aux->akicks; i++)
			Responde(cl, CLI(chanserv), "\00312%s\003 Motivo: \00312%s\003 (por \00312%s\003)", aux->akick[i].nick, aux->akick[i].motivo, aux->akick[i].puesto);
	}
	else
	{
		if (*param[2] == '+')
		{
			Cliente *al;
			char *motivo = NULL;
			if ((aux = CSBuscaAkicks(param[1])) && aux->akicks == CS_MAX_AKICK && !CSEsAkick(param[2] + 1, param[1]))
			{
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No se aceptan m�s entradas.");
				return 1;
			}
			if (params > 3)
				motivo = Unifica(param, params, 3, -1);
			else
				motivo = "No eres bienvenid@.";
			if (motivo && (strchr(motivo, '\t') || strchr(motivo, '\1')))
			{
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este motivo contiene caracteres inv�lidos.");
				return 1;
			}
			CSBorraAkickDb(param[2] + 1, param[1]);
			CSInsertaAkickDb(param[2] + 1, param[1], parv[0], motivo);
			if ((al = BuscaCliente(param[2] + 1, NULL)))
				ProtFunc(P_KICK)(al, CLI(chanserv), cn, motivo);
			Responde(cl, CLI(chanserv), "Akick a \00312%s\003 a�adido.", param[2] + 1);
		}
		else if (*param[2] == '-')
		{
			if (!CSEsAkick(param[2] + 1, param[1]))
			{
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Esta entrada no existe.");
				return 1;
			}
			CSBorraAkickDb(param[2] + 1, param[1]);
			Responde(cl, CLI(chanserv), "Entrada \00312%s\003 eliminada.", param[2] + 1);
		}
		else
		{
			Responde(cl, CLI(chanserv), CS_ERR_SNTX, "AKICK +-nick|mascara [motivo]");
			return 1;
		}
	}	
	return 0;
}
BOTFUNC(CSAccess)
{
	char *registros;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "ACCESS #canal [+-nick [+-flags]]");
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
		char *tok;
		if (!CSTieneNivel(parv[0], param[1], CS_LEV_LIS))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		registros = SQLCogeRegistro(CS_SQL, param[1], "regs");
		if (!registros)
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay ning�n acceso.");
			return 1;
		}
		Responde(cl, CLI(chanserv), "*** Accesos de \00312%s\003 ***", param[1]);
		for (tok = strtok(registros, ":"); tok; tok = strtok(NULL, ":"))
		{
			u_long niv = atol(strtok(NULL, " "));
			Responde(cl, CLI(chanserv), "Nick: \00312%s\003 flags:\00312+%s\003 (\00312%lu\003)", tok, ModosAFlags(niv, cFlags, NULL), niv);
		}
	}
	else
	{
		u_long prev;
		if (!CSTieneNivel(parv[0], param[1], CS_LEV_EDT))
		{
			Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
			return 1;
		}
		if (*param[2] != '+' && *param[2] != '-')
		{
			Responde(cl, CLI(chanserv), CS_ERR_SNTX, "ACCESS #canal +-nick [+-flags]");
			return 1;
		}
		if (!IsReg(param[2] + 1))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick no est� registrado.");
			return 1;
		}
		if (*param[2] == '+')
		{
			char f = ADD, buf[512], *modos = param[3];
			param[2]++;
			if (params < 4)
			{
				Responde(cl, CLI(chanserv), CS_ERR_PARA, "ACCESS +-nick +-flags");
				return 1;
			}
			prev = CSBorraAcceso(param[2], param[1]);
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
			if (prev)
			{
				CsRegistros *aux;
				if ((aux = busca_cregistro(param[2])) && aux->subs == CS_MAX_REGS && !CSEsRegistro(param[2], param[1]))
				{
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este nick ya no acepta m�s registros.");
					return 1;
				}
				registros = SQLCogeRegistro(CS_SQL, param[1], "regs");
				ircsprintf(buf, "%s:%lu", param[2], prev);
				SQLInserta(CS_SQL, param[1], "regs", "%s%s", registros ? registros : "", buf);
				CSInsertaRegistro(param[2], param[1], prev);
				Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 cambiado a \00312%s\003", param[2], param[3]);
				if (IsChanDebug(param[1]))
				{
					Canal *cn;
					cn = BuscaCanal(param[1], NULL);
					ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s cambiado a %s", param[2], param[3]);
				}
			}
			else
			{
				Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2]);
				if (IsChanDebug(param[1]))
				{
					Canal *cn;
					cn = BuscaCanal(param[1], NULL);
					ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s eliminado", param[2]);
				}
			}
		}
		else if (*param[2] == '-')
		{
			param[2]++;
			prev = CSBorraAcceso(param[2], param[1]);
			Responde(cl, CLI(chanserv), "Acceso de \00312%s\003 eliminado.", param[2]);
			if (IsChanDebug(param[1]))
			{
				Canal *cn;
				cn = BuscaCanal(param[1], NULL);
				ProtFunc(P_NOTICE)((Cliente *)cn, CLI(chanserv), "Acceso de %s eliminado", param[2]);
			}
		}
	}	
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
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patr�n");
		return 1;
	}
	if (params == 3)
	{
		if (*param[1] == '-')
		{
			if (params < 3)
			{
				Responde(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patr�n");
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
					Responde(cl, CLI(chanserv), "*** Canales en petici�n que coinciden con el patr�n \00312%s\003 ***", param[1]);
					for (i = 0; i < chanserv.maxlist && (row = SQLFetchRow(res)); i++)
						Responde(cl, CLI(chanserv), "\00312%s\003 Desc:\00312%s", row[0], row[1]);
					Responde(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
					SQLFreeRes(res);
					break;
				default:
					Responde(cl, CLI(chanserv), CS_ERR_SNTX, "Par�metro no v�lido.");
					return 1;
			}
		}
	}
	else
	{
		if (*param[1] == '-')
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "LIST [-r] patr�n");
			return 1;
		}
		rep = str_replace(param[1], '*', '%');
		res = SQLQuery("SELECT item from %s%s where LOWER(item) LIKE '%s' AND registro!='0'", PREFIJO, CS_SQL, strtolower(rep));
		if (!res)
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "No hay canales para listar.");
			return 1;
		}
		Responde(cl, CLI(chanserv), "*** Canales que coinciden con el patr�n \00312%s\003 ***", param[1]);
		for (i = 0; i < chanserv.maxlist && (row = SQLFetchRow(res)); i++)
		{
			if (IsOper(cl) || !(atoi(SQLCogeRegistro(CS_SQL, row[0], "opts")) & CS_OPT_HIDE))
				Responde(cl, CLI(chanserv), "\00312%s", row[0]);
			else
				i--;
		}
		Responde(cl, CLI(chanserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	return 0;
}
BOTFUNC(CSJb)
{
	Cliente *bl;
	Canal *cn;
	int i, bots = 0;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "JOIN #canal [servicios]");
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
	cn = BuscaCanal(param[1], NULL);
	if (cn)
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			bl = BuscaCliente(ex->nick, NULL);
			if (EsLink(cn->miembro, bl) && !CSEsResidente(ex, param[1] + 1))
				SacaBot(bl, cn->nombre, "Cambiando bots");
		}
	}
	for (i = 2; i < params; i++)
	{
		Modulo *ex;
		if ((ex = BuscaModulo(param[i], modulos)))
		{
			bl = BuscaCliente(ex->nick, NULL);
			EntraBot(bl, param[1]);
			bots |= ex->id;
		}
	}
	if (IsChanReg(param[1]))
		SQLInserta(CS_SQL, param[1], "bjoins", "%i", bots);
	Responde(cl, CLI(chanserv), "Bots cambiados.");
	return 0;
}
BOTFUNC(CSSendpass)
{
	char *pass;
	char *founder;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "SENDPASS #canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	pass = AleatorioEx("******-******");
	founder = strdup(SQLCogeRegistro(CS_SQL, param[1], "founder"));
	SQLInserta(CS_SQL, param[1], "pass", MDString(pass));
	Email(SQLCogeRegistro(NS_SQL, founder, "email"), "Reenv�o de la contrase�a", "Debido a la p�rdida de la contrase�a de tu canal, se te ha generado otra clave totalmetne segura.\r\n"
		"A partir de ahora, la clave de tu canal es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, chanserv.hmod->nick, conf_set->red);
	Responde(cl, CLI(chanserv), "Se generado y enviado otra contrase�a al email del founder de \00312%s\003.", param[1]);
	Free(founder);
	return 0;
}
BOTFUNC(CSSuspender)
{
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "SUSPENDER #canal motivo");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
#ifdef UDB
	PropagaRegistro("C::%s::S %s", param[1], motivo);
#else
	SQLInserta(CS_SQL, param[1], "suspend", motivo);
#endif
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido suspendido.", param[1]);
	return 0;
}
BOTFUNC(CSLiberar)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "LIBERAR #canal");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no est� suspendido.");
		return 1;
	}
#ifdef UDB
	PropagaRegistro("C::%s::S", param[1]);
#else
	SQLInserta(CS_SQL, param[1], "suspend", "");
#endif
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su suspenso.", param[1]);
	return 0;
}
BOTFUNC(CSForbid)
{
	Canal *cn;
	char *motivo;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "FORBID #canal motivo");
		return 1;
	}
	motivo = Unifica(param, params, 2, -1);
	if ((cn = BuscaCanal(param[1], NULL)) && cn->miembros)
	{
		Cliente **al = NULL;
		int i;
		al = CSEmpaquetaClientes(cn, NULL, !ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_KICK)(al[i], CLI(chanserv), cn, "Canal \2PROHIBIDO\2: %s", motivo);
		ircfree(al);
	}
#ifdef UDB
	PropagaRegistro("C::%s::B %s", param[1], motivo);
#else
	SQLInserta(CS_FORBIDS, param[1], "motivo", motivo);
#endif
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido prohibido.", param[1]);
	return 0;
}
BOTFUNC(CSUnforbid)
{
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "UNFORBID #canal");
		return 1;
	}
	if (!IsChanForbid(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no est� prohibido.");
		return 1;
	}
#ifdef UDB
	PropagaRegistro("C::%s::B", param[1]);
#else
	SQLBorra(CS_FORBIDS, param[1]);
#endif
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido liberado de su prohibici�n.", param[1]);
	return 0;
}
BOTFUNC(CSBlock)
{
	Canal *cn;
	Cliente **al;
	int i;
	if (params < 2)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "BLOCK #canal");
		return 1;
	}
	if (!(cn = BuscaCanal(param[1], NULL)))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal est� vac�o.");
		return 1;
	}
	if (MODE_OWNER)
	{
		al = CSEmpaquetaClientes(cn, cn->owner, ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_OWNER, TRIO(al[i]));
		ircfree(al);
	}
	if (MODE_ADM)
	{
		al = CSEmpaquetaClientes(cn, cn->admin, ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_ADM, TRIO(al[i]));
		ircfree(al);
	}
	al = CSEmpaquetaClientes(cn, cn->op, ADD);
	for (i = 0; al[i]; i++)
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-o %s", TRIO(al[i]));
	ircfree(al);
	if (MODE_HALF)
	{
		al = CSEmpaquetaClientes(cn, cn->half, ADD);
		for (i = 0; al[i]; i++)
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-%c %s", MODEF_HALF, TRIO(al[i]));
		ircfree(al);
	}
	al = CSEmpaquetaClientes(cn, cn->voz, ADD);
	for (i = 0; al[i]; i++)
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-v %s", TRIO(al[i]));
	ircfree(al);
	ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+iml 1");
	Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido bloqueado.", param[1]);
	/* aun asi podr�n opearse si tienen nivel, se supone que este comando lo utilizan los operadores 
	   y estan supervisando el canal, para que si alguno se opea se le killee, o simplemente por hacer 
	   la gracia */
	return 0;
}
BOTFUNC(CSRegister)
{
	if (params == 2) /* registro de una petici�n de canal */
	{
		if (!IsOper(cl))
		{
			Responde(cl, CLI(chanserv), CS_ERR_PARA, "REGISTER #canal pass descripci�n tokens");
			return 1;
		}
		if (!IsChanPReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no est� en petici�n de registro.");
			return 1;
		}
		goto registrar;
		SQLInserta(CS_SQL, param[1], "registro", "%i", time(0));
#ifdef UDB
		if (chanserv.opts & CS_AUTOMIGRAR)
		{
			SQLInserta(CS_SQL, param[1], "opts", "%i", CS_OPT_UDB);
			CSPropagaCanal(param[1]);
		}
#endif
		Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido registrado.", param[1]);
	}
	else if (params >= 4) /* peticion de registro */
	{
		SQLRes res;
		SQLRow row;
		int i;
		char *desc;
		if (IsChanReg(param[1]))
		{
			Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya est� registrado.");
			return 1;
		}
		if (!IsOper(cl))
		{
			if ((atol(SQLCogeRegistro(NS_SQL, parv[0], "reg")) + 86400 * chanserv.antig) > time(0))
			{
				char buf[512];
				ircsprintf(buf, "Tu nick debe tener una antig�edad de como m�nimo %i d�as.", chanserv.antig);
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				return 1;
			}
			if ((res = SQLQuery("SELECT nick from %s%s where LOWER(nick)='%s'", PREFIJO, CS_TOK, strtolower(parv[0]))))
			{
				char buf[512];
				SQLFreeRes(res);
				ircsprintf(buf, "Ya has efectuado alguna operaci�n referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv.vigencia);
				Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
				return 1;
			}
			if (params < (4 + chanserv.necesarios))
			{
				char buf[512];
				ircsprintf(buf, "Se necesitan %i tokens.", chanserv.necesarios);
				Responde(cl, CLI(chanserv), CS_ERR_SNTX, buf);
				return 1;
			}
			/* comprobamos los tokens */
			for (i = 0; i < chanserv.necesarios; i++)
			{
				char buf[512];
				char *tok = param[params - i - 1];
				tokbuf[0] = '\0';
				if (strstr(tokbuf, tok)) /* ya hab�a sido usado */
				{
					ircsprintf(buf, "El token %s est� repetido.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				if (!(res = SQLQuery("SELECT item,nick,hora from %s%s where LOWER(item)='%s'", PREFIJO, CS_TOK, strtolower(tok))))
				{
					ircsprintf(buf, "El token %s no es v�lido o ha caducado.", tok);
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
				if ((atol(row[2]) + 24 * chanserv.vigencia) < time(0))
				{
					ircsprintf(buf, "El token %s ha caducado.", tok);
					Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
					return 1;
				}
				strcat(tokbuf, tok);
				strcat(tokbuf, " ");
			}
			desc = Unifica(param, params, 3, params - chanserv.necesarios - 1);
		}
		else
			desc = Unifica(param, params, 3, -1);
		SQLInserta(CS_SQL, param[1], "founder", parv[0]);
		SQLInserta(CS_SQL, param[1], "pass", MDString(param[2]));
		SQLInserta(CS_SQL, param[1], "descripcion", desc);
		if (IsOper(cl))
		{
			Canal *cn;
			registrar:
			cn = BuscaCanal(param[1], NULL);
			SQLInserta(CS_SQL, param[1], "registro", "%i", time(0));
#ifdef UDB
			if (chanserv.opts & CS_AUTOMIGRAR)
			{
				SQLInserta(CS_SQL, param[1], "opts", "%i", CS_OPT_UDB);
				CSPropagaCanal(param[1]);
			}
			else
#endif
			if (cn)
			{
				if (RedOverride)
					EntraBot(CLI(chanserv), cn->nombre);
				if (MODE_RGSTR)
					ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%c", MODEF_RGSTR);
				ProtFunc(P_TOPIC)(CLI(chanserv), cn, "El canal ha sido registrado.");
			}
			Senyal1(CS_SIGN_REG, param[1]);
		}
		Responde(cl, CLI(chanserv), "El canal \00312%s\003 ha sido registrado.", param[1]);
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, IsOper(cl) ? "REGISTER #canal [pass descripci�n]" : "REGISTER #canal pass descripci�n tokens");
		return 1;
	}
	return 0;
}
BOTFUNC(CSToken)
{
	SQLRes res;
	SQLRow row;
	int libres = 25, i; /* siempre tendremos 25 tokens libres */
	if ((atol(SQLCogeRegistro(NS_SQL, parv[0], "reg")) + 86400 * chanserv.antig) > time(0))
	{
		char buf[512];
		ircsprintf(buf, "Tu nick debe tener una antig�edad de como m�nimo %i d�as.", chanserv.antig);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}
	if ((res = SQLQuery("SELECT nick from %s%s where LOWER(nick)='%s'", PREFIJO, CS_TOK, strtolower(parv[0]))))
	{
		char buf[512];
		SQLFreeRes(res);
		ircsprintf(buf, "Ya has efectuado alguna operaci�n referente a tokens. Debes esperar %i horas antes de realizar otra.", chanserv.vigencia);
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, buf);
		return 1;
	}
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
	Responde(cl, CLI(chanserv), "Tu token es \00312%s\003. Guarda este token ya que ser� necesario para futuras operaciones y no podr�s pedir otro hasta dentro de unas horas.", row[0]);
	SQLFreeRes(res);
	return 0;
}
#ifdef UDB
BOTFUNC(CSMigrar)
{
	int opts;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "MIGRAR #canal pass");
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
	if (!CSEsFundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contrase�a incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (opts & CS_OPT_UDB)
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya est� migrado.");
		return 1;
	}
	CSPropagaCanal(param[1]);
	Responde(cl, CLI(chanserv), "Migraci�n realizada.");
	opts |= CS_OPT_UDB;
	SQLInserta(CS_SQL, param[1], "opts", "%i", opts);
	return 0;
}
BOTFUNC(CSDemigrar)
{
	int opts;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "DEMIGRAR #canal pass");
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
	if (!CSEsFundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contrase�a incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (!(opts & CS_OPT_UDB))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no est� migrado.");
		return 1;
	}
	PropagaRegistro("C::%s", param[1]);
	Responde(cl, CLI(chanserv), "Demigraci�n realizada.");
	opts &= ~CS_OPT_UDB;
	SQLInserta(CS_SQL, param[1], "opts", "%i", opts);
	return 0;
}
BOTFUNC(CSProteger)
{
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "PROTEGER #canal {+|-}nick");
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
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_EDT))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!IsChanUDB(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no est� migrado.");
		return 1;
	}
	if (BadPtr(param[2] + 1))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER #canal {+|-}nick");
		return 1;
	}
	if (*param[2] == '+')
	{
		PropagaRegistro("C::%s::A::%s -", param[1], param[2] + 1);
		Responde(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido a�adida.", param[2] + 1);
	}
	else if (*param[2] == '-')
	{
		PropagaRegistro("C::%s::A::%s", param[1], param[2] + 1);
		Responde(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido eliminada.", param[2] + 1);
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER {+|-}nick");
		return 1;
	}
	return 0;
}
#endif
int CSCmdMode(Cliente *cl, Canal *cn, char *mods[], int max)
{
	modebuf[0] = parabuf[0] = '\0';
	if (IsChanReg(cn->nombre))
	{
		int opts;
		char *modlock, *k;
		opts = atoi(SQLCogeRegistro(CS_SQL, cn->nombre, "opts"));
		modlock = SQLCogeRegistro(CS_SQL, cn->nombre, "modos");
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
			if (k && strchr(k, 'k') && cn->clave)
				ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "-k %s", mods[max-1]); /* siempre est� en el �ltimo */
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "%s %s", modebuf, parabuf);
			modebuf[0] = parabuf[0] = '\0';
		}
		if (opts & CS_OPT_SOP && !EsServidor(cl))
		{
			int pars = 1;
			char *modos, f = ADD;
			modos = mods[0];
			while (!BadPtr(modos))
			{
				if (*modos == '+')
					f = ADD;
				else if (*modos == '-')
					f = DEL;
				else if (*modos == 'o')
				{
					if (f == ADD)
					{
						if (!CSTieneNivel(mods[pars], cn->nombre, CS_LEV_AOP))
						{
							strcat(modebuf, "-o");
							strcat(parabuf, TRIO(BuscaCliente(mods[pars], NULL)));
							strcat(parabuf, " ");
						}
					}
					pars++;
				}
				else if (*modos == MODEF_HALF)
				{
					if (f == ADD)
					{
						if (!CSTieneNivel(mods[pars], cn->nombre, CS_LEV_AHA))
						{
							char tmp[3];
							ircsprintf(tmp, "-%c", MODEF_HALF);
							strcat(modebuf, tmp);
							strcat(parabuf, TRIO(BuscaCliente(mods[pars], NULL)));
							strcat(parabuf, " ");
						}
					}
					pars++;
				}
				else if (*modos == MODEF_ADM)
				{
					if (f == ADD)
					{
						if (!CSTieneNivel(mods[pars], cn->nombre, CS_LEV_AAD))
						{
							char tmp[3];
							ircsprintf(tmp, "-%c", MODEF_ADM);
							strcat(modebuf, tmp);
							strcat(parabuf, TRIO(BuscaCliente(mods[pars], NULL)));
							strcat(parabuf, " ");
						}
					}
					pars++;
				}
				else if (*modos == MODEF_OWNER || *modos == 'b' || *modos == 'e' || *modos == 'v' || *modos == 'L' || *modos == 'f' || *modos == 'k')
					pars++;
				else if (*modos == 'l' && f == ADD)
					pars++;
				modos++;
			}
		}
	}
	if (modebuf[0] != '\0')
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
	char find, *forb;
	SQLRes res;
	SQLRow row;
	u_long aux, nivel;
	int opts;
	char *entry, *modos, *topic, *susp;
	Akick *akicks;
	if (!IsOper(cl) && (forb = IsChanForbid(cn->nombre)))
	{
		if (ProtFunc(P_PART_USUARIO_REMOTO))
		{
			ProtFunc(P_PART_USUARIO_REMOTO)(cl, cn, "Este canal est� \2PROHIBIDO\2: %s", forb);
			ProtFunc(P_NOTICE)(cl, CLI(chanserv), "Este canal est� \2PROHIBIDO\2: %s", forb);
		}
		return 0;
	}
	if (IsChanReg(cn->nombre))
	{
		find = 0;
		res = SQLQuery("SELECT opts,entry,modos,topic from %s%s where LOWER(item)='%s'", PREFIJO, CS_SQL, strtolower(cn->nombre));
		row = SQLFetchRow(res);
		opts = atoi(row[0]);
		entry = strdup(row[1]);
		modos = strdup(row[2]);
		topic = strdup(row[3]);
		SQLFreeRes(res);
		aux = 0L;
		if ((susp = IsChanSuspend(cn->nombre)))
		{
			ProtFunc(P_NOTICE)(cl, CLI(chanserv), "Este canal est� suspendido: %s", susp);
			if (!IsOper(cl))
				return 0;
		}
		aux = FlagsAModos(&aux, modos, protocolo->cmodos);
		if (((opts & CS_OPT_RMOD) && (((aux & MODE_OPERONLY) && !IsOper(cl)) || 
			((aux & MODE_ADMONLY) && !IsAdmin(cl)) ||
			((aux & MODE_RGSTRONLY) && !IsId(cl)))) ||
			(((opts & CS_OPT_SEC) && !IsPreo(cl) && (!CSTieneNivel(cl->nombre, cn->nombre, 0L) || !IsId(cl)))))
		{
			if (ProtFunc(P_PART_USUARIO_REMOTO))
				ProtFunc(P_PART_USUARIO_REMOTO)(cl, cn, NULL);
			return 0;
		}
		akicks = CSBuscaAkicks(cn->nombre);
		if (akicks)
		{
			int i;
			for (i = 0; i < akicks->akicks; i++)
			{
				if (!match(MascaraIrcd(akicks->akick[i].nick), cl->mask))
				{
					find = 1;
					ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+b %s", TipoMascara(cl->mask, chanserv.bantype));
					ProtFunc(P_KICK)(cl, CLI(chanserv), cn, "%s (%s)", akicks->akick[i].motivo, akicks->akick[i].puesto);
					break;
				}
			}
		}
		if (find)
			return 0;
		nivel = IsId(cl) ? CSTieneNivel(cl->nombre, cn->nombre, 0L) : 0L;
		buf[0] = tokbuf[0] = '\0';
		if (MODE_OWNER && IsId(cl))
		{
			if (CSEsFundador(cl, cn->nombre) || CSEsFundador_cache(cl, cn->nombre))
			{
				chrcat(buf, MODEF_OWNER);
				strcat(tokbuf, TRIO(cl));
				strcat(tokbuf, " ");
			}
		}
		if (nivel & CS_LEV_AAD && MODE_ADM)
		{
			chrcat(buf, MODEF_ADM);
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AOP)
		{
			strcat(buf, "o");
			strcat(tokbuf, TRIO(cl));
			strcat(tokbuf, " ");
		}
		if (nivel & CS_LEV_AHA && MODE_HALF)
		{
			chrcat(buf, MODEF_HALF);
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
			SQLInserta(CS_SQL, cn->nombre, "ultimo", "%lu", time(0));
		//if (buf[0])
		//	tokbuf[strlen(tokbuf)-1] = '\0';
		if (cn->miembros == 1)
		{
			if (MODE_RGSTR)
				chrcat(buf, MODEF_RGSTR);
			if (opts & CS_OPT_RMOD)
			{
				char *mod;
				if ((mod = strtok(modos, strchr(modos, '-') ? "-": " ")))
				{
					strcat(buf, *mod == '+' ? mod+1 : mod);
					if ((mod = strtok(NULL, "\0")))
					{
						char *p;
						if ((p = strchr(mod, ' '))) 
							strcat(tokbuf, p+1);
						else
							strcat(tokbuf, mod);
					}
				}
			}
		}
		if (buf[0])
			ProtFunc(P_MODO_CANAL)(CLI(chanserv), cn, "+%s %s", buf, tokbuf);
		if (cn->miembros == 1 && opts & CS_OPT_RTOP)
			ProtFunc(P_TOPIC)(CLI(chanserv), cn, topic);
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
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"n SERIAL, "
  			"item text, "
			"founder text, "
  			"pass text, "
  			"descripcion text, "
  			"modos varchar(255) default '+nt', "
  			"opts int4 default '0', "
  			"topic varchar(255) default 'El canal ha sido registrado.', "
  			"entry varchar(255) default 'El canal ha sido registrado.', "
  			"registro int4 default '0', "
  			"ultimo int4 default '0', "
  			"ntopic text, "
  			"limite int2 default '5', "
  			"akick text, "
  			"bjoins int4 default '0', "
#ifndef UDB
  			"suspend text, "
#endif
  			"url text, "
  			"email text, "
  			"memos text, "
  			"regs text "
			");", PREFIJO, CS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_SQL);
	}
	if (!SQLEsTabla(CS_TOK))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
			"item varchar(255) default NULL, "
			"nick varchar(255) default NULL, "
			"hora int4 default '0' "
			");", PREFIJO, CS_TOK))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_TOK);
	}
#ifndef UDB
	if (!SQLEsTabla(CS_FORBIDS))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"motivo varchar(255) default NULL "
			");", PREFIJO, CS_FORBIDS))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, CS_FORBIDS);
	}
#endif
	SQLCargaTablas();
	CSCargaRegistros();
	CSCargaAkicks();
	return 1;
}
int CSSigEOS()
{
	/* metemos los bots en los canales que lo hayan soliticado */
	SQLRes res;
	SQLRow row;
	int bjoins;
	Cliente *bl;
#ifdef UDB
	PropagaRegistro("S::C %s", chanserv.hmod->mascara);
#endif
	if ((res = SQLQuery("SELECT item,bjoins from %s%s where bjoins !='0'", PREFIJO, CS_SQL)))
	{
		while ((row = SQLFetchRow(res)))
		{
			Modulo *ex;
			bjoins = atoi(row[1]);
			for (ex = modulos; ex; ex = ex->sig)
			{
				if ((bjoins & ex->id) && !CSEsResidente(ex, row[0]))
				{
					bl = BuscaCliente(ex->nick, NULL);
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
	BorraCache(CACHE_FUNDADORES, cl->nombre, chanserv.hmod->id);
	return 0;
}
int CSBaja(char *canal, char opt)
{
	Canal *an;
	Akick *akick;
	char *tok, *regs;
	if (!opt && atoi(SQLCogeRegistro(CS_SQL, canal, "opts")) & CS_OPT_NODROP)
		return 1;
	an = BuscaCanal(canal, NULL);
	if (an)
	{
		ProtFunc(P_MODO_CANAL)(CLI(chanserv), an, "-r");
		if (RedOverride)
			SacaBot(CLI(chanserv), canal, NULL);
	}
	if ((regs = SQLCogeRegistro(CS_SQL, canal, "regs")))
	{
		for (tok = strtok(regs, ":"); tok; tok = strtok(NULL, ":"))
		{
			CSBorraRegistro(tok, canal);
			strtok(NULL, " ");
		}
	}
	if ((akick = CSBuscaAkicks(canal)))
	{
		int akicks = akick->akicks;
		while (akicks)
		{
			CSBorraAkick(akick->akick[0].nick, canal);
			akicks--;
		}
	}
#ifdef UDB
	if (IsChanUDB(canal))
		PropagaRegistro("C::%s", canal);
#endif
	Senyal1(CS_SIGN_DROP, canal);
	SQLBorra(CS_SQL, canal);
	return 0;
}
CsRegistros *busca_cregistro(char *nick)
{
	CsRegistros *aux;
	u_int hash;
	hash = HashCliente(nick);
	for (aux = (CsRegistros *)csregs[hash].item; aux; aux = aux->sig)
	{
		if (!strcasecmp(aux->nick, nick))
			return aux;
	}
	return NULL;
}
Akick *CSBuscaAkicks(char *canal)
{
	Akick *aux;
	u_int hash;
	hash = HashCanal(canal);
	for (aux = (Akick *)akicks[hash].item; aux; aux = aux->sig)
	{
		if (!strcasecmp(aux->canal, canal))
			return aux;
	}
	return NULL;
}
void CSInsertaRegistro(char *nick, char *canal, u_long flag)
{
	CsRegistros *creg;
	u_int hash = 0;
	if (!(creg = busca_cregistro(nick)))
	{
		creg = (CsRegistros *)Malloc(sizeof(CsRegistros));
		creg->subs = 0;
		creg->nick = strdup(nick);
	}
	hash = HashCliente(nick);
	creg->sub[creg->subs].canal = strdup(canal);
	creg->sub[creg->subs].flags = flag;
	creg->subs++;
	if (!busca_cregistro(nick))
	{
		AddItem(creg, csregs[hash].item);
		csregs[hash].items++;
	}
}
Akick *CSInsertaAkick(char *nick, char *canal, char *emisor, char *motivo)
{
	Akick *akick;
	u_int hash = 0;
	if (!(akick = CSBuscaAkicks(canal)))
	{
		akick = (Akick *)Malloc(sizeof(Akick));
		akick->akicks = 0;
		akick->canal = strdup(canal);
	}
	hash = HashCanal(canal);
	akick->akick[akick->akicks].nick = strdup(nick);
	akick->akick[akick->akicks].motivo = strdup(motivo);
	akick->akick[akick->akicks].puesto = strdup(emisor);
	akick->akicks++;
	if (!CSBuscaAkicks(canal))
	{
		AddItem(akick, akicks[hash].item);
		akicks[hash].items++;
	}
	return akick;
}
int CSBorraRegistro_de_hash(CsRegistros *creg, char *nick)
{
	u_int hash;
	hash = HashCliente(nick);
	if (BorraItem(creg, csregs[hash].item))
	{
		csregs[hash].items--;
		return 1;
	}
	return 0;
}
void CSBorraRegistro(char *nick, char *canal)
{
	CsRegistros *creg;
	int i;
	if (!(creg = busca_cregistro(nick)))
		return;
	for (i = 0; i < creg->subs; i++)
	{
		if (!strcasecmp(creg->sub[i].canal, canal))
		{
			/* liberamos el canal en cuesti�n */
			Free(creg->sub[i].canal);
			/* ahora tenemos v�a libre para desplazar la lista para abajo */
			for (; i < creg->subs; i++)
				creg->sub[i] = creg->sub[i+1];
			if (!(--creg->subs))
			{
				CSBorraRegistro_de_hash(creg, nick);
				Free(creg->nick);
				Free(creg);
			}
			return;
		}
	}
}
int CSBorraAkick_de_hash(Akick *akick, char *canal)
{
	u_int hash;
	hash = HashCanal(canal);
	if (BorraItem(akick, akicks[hash].item))
	{
			akicks[hash].items--;
			return 1;
	}
	return 0;
}
void CSBorraAkick(char *nick, char *canal)
{
	Akick *akick;
	int i;
	if (!(akick = CSBuscaAkicks(canal)))
		return;
	for (i = 0; i < akick->akicks; i++)
	{
		if (!strcasecmp(akick->akick[i].nick, nick))
		{
			Free(akick->akick[i].motivo);
			Free(akick->akick[i].puesto);
			Free(akick->akick[i].nick);
			for (; i < akick->akicks; i++)
				akick->akick[i] = akick->akick[i+1];
			if (!(--akick->akicks))
			{
				CSBorraAkick_de_hash(akick, canal);
				Free(akick->canal);
				Free(akick);
			}
			return;
		}
	}
}
int CSDropanick(char *nick)
{
	CsRegistros *regs;
	regs = busca_cregistro(nick);
	if (regs)
	{
		int subs = regs->subs;
		while(subs)
		{
			char tmp[512];
			strncpy(tmp, regs->sub[0].canal, 512);
			CSBorraAcceso(nick, tmp);
			if (!strcasecmp(SQLCogeRegistro(CS_SQL, tmp, "founder"), nick))
				SQLInserta(CS_SQL, tmp, "founder", CLI(chanserv)->nombre);
			subs--;
		}
	}
	return 0;
}
void CSCargaRegistros()
{
	SQLRes res;
	SQLRow row;
	res = SQLQuery("SELECT item,regs from %s%s", PREFIJO, CS_SQL);
	if (!res)
		return;
	while ((row = SQLFetchRow(res)))
	{
		char *tok;
		for (tok = strtok(row[1], ":"); tok; tok = strtok(NULL, ":"))
			CSInsertaRegistro(tok, row[0], atol(strtok(NULL, " ")));
	}
	SQLFreeRes(res);
}
void CSCargaAkicks()
{
	SQLRes res;
	SQLRow row;
	res = SQLQuery("SELECT item,akick from %s%s", PREFIJO, CS_SQL);
	if (!res)
		return;
	while ((row = SQLFetchRow(res)))
	{
		char *tok, *mo, *au;
		for (tok = strtok(row[1], "\1"); tok; tok = strtok(NULL, "\1"))
		{
			mo = strtok(NULL, "\1");
			au = strtok(NULL, "\t");
			CSInsertaAkick(tok, row[0], au, mo);
		}
	}
	SQLFreeRes(res);
}
u_long CSTieneNivel(char *nick, char *canal, u_long flag)
{
	CsRegistros *aux;
	Cliente *al;
	int i;
	al = BuscaCliente(nick, NULL);
	if ((!IsOper(al) && IsChanSuspend(canal)) || !IsId(al))
		return 0L;
	if (CSEsFundador(al, canal) || CSEsFundador_cache(al, canal))
		return CS_LEV_ALL;
	if ((aux = busca_cregistro(nick)))
	{
		for (i = 0; i < aux->subs; i++)
		{
			if (!strcasecmp(aux->sub[i].canal, canal))
			{
				u_long nivel = 0L;
				if (IsPreo(al))
					nivel |= (CS_LEV_ALL & ~CS_LEV_MOD);
				nivel |= aux->sub[i].flags;		
				if (flag)
					return (nivel & flag);
				else
					return aux->sub[i].flags;
			}
		}
	}
	if (IsPreo(al))
		return (CS_LEV_ALL & ~CS_LEV_MOD);
	return 0L;
}
ProcFunc(CSDropachans)
{
	SQLRes res;
	SQLRow row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = SQLQuery("SELECT item from %s%s where (ultimo < %i AND ultimo !='0') OR LOWER(founder)='%s' LIMIT 30 OFFSET %i ", PREFIJO, CS_SQL, time(0) - 86400 * chanserv.autodrop, CLI(chanserv) && CLI(chanserv)->nombre && SockIrcd ? strtolower(CLI(chanserv)->nombre) : "", proc->proc)) || !SQLNumRows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			SQLQuery("DELETE from %s%s where hora < %i AND hora !='0'", PREFIJO, CS_TOK, time(0) - 3600 * chanserv.vigencia);
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			if (atoi(row[1]) + 86400 * chanserv.autodrop < time(0))
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
#ifdef UDB
	Udb *bloq, *reg;
#else
	char *motivo;
#endif
	if (!canal)
		return NULL;
#ifdef UDB
	if ((reg = BuscaRegistro(BDD_CHANS, canal)) && (bloq = BuscaBloque(C_SUS_TOK, reg)))
		return bloq->data_char;
#else
	if ((motivo = SQLCogeRegistro(CS_SQL, canal, "suspend")))
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
	if ((reg = BuscaRegistro(BDD_CHANS, canal)) && (bloq = BuscaBloque(C_FOR_TOK, reg)))
		return bloq->data_char;
#else
	if ((motivo = SQLCogeRegistro(CS_FORBIDS, canal, "motivo")))
		return motivo;
#endif
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
