/*
 * $Id: nickserv.c,v 1.2 2004-09-11 16:08:05 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "nickserv.h"
#include "chanserv.h"

NickServ *nickserv = NULL;

static int nickserv_help		(Cliente *, char *[], int, char *[], int);
static int nickserv_reg		(Cliente *, char *[], int, char *[], int);
static int nickserv_identify	(Cliente *, char *[], int, char *[], int);	
static int nickserv_set		(Cliente *, char *[], int, char *[], int);
static int nickserv_drop		(Cliente *, char *[], int, char *[], int);
static int nickserv_sendpass	(Cliente *, char *[], int, char *[], int);
static int nickserv_info		(Cliente *, char *[], int, char *[], int);
static int nickserv_list		(Cliente *, char *[], int, char *[], int);
static int nickserv_ghost		(Cliente *, char *[], int, char *[], int);
static int nickserv_suspend		(Cliente *, char *[], int, char *[], int);
static int nickserv_liberar		(Cliente *, char *[], int, char *[], int);
static int nickserv_addwhois	(Cliente *, char *[], int, char *[], int);
static int nickserv_delwhois	(Cliente *, char *[], int, char *[], int);
static int nickserv_rename 		(Cliente *, char *[], int, char *[], int);
#ifdef UDB
static int nickserv_migrar		(Cliente *, char *[], int, char *[], int);
static int nickserv_demigrar	(Cliente *, char *[], int, char *[], int);
#endif

DLLFUNC int nickserv_sig_mysql	();
DLLFUNC int nickserv_sig_quit 	(Cliente *, char *);
DLLFUNC int nickserv_sig_idok 	(Cliente *);
#ifdef UDB
DLLFUNC int nickserv_sig_eos	();
#endif
DLLFUNC int nickserv_killid		(char *);

DLLFUNC int nickserv_dropanicks	(Proc *);

DLLFUNC int nickserv_baja(char *, char);
DLLFUNC void cambia_nick_inv(char *);

DLLFUNC IRCFUNC(nickserv_nick);
DLLFUNC IRCFUNC(nickserv_quit);
DLLFUNC IRCFUNC(nickserv_umode);
#ifndef _WIN32
CsRegistros *(*busca_cregistro_dl)(char *);
char *(*cflags2str_dl)(u_long);
#else
#define busca_cregistro_dl busca_cregistro
#define cflags2str_dl cflags2str
#endif
void set(Conf *, Modulo *);
static bCom nickserv_coms[] = {
	{ "help" , nickserv_help , TODOS } ,
	{ "register" , nickserv_reg , TODOS } ,
	{ "identify" , nickserv_identify , TODOS } ,
	{ "set" , nickserv_set , USERS } ,
	{ "drop" , nickserv_drop , PREOS } , /* de momento dejo el drop no es para usuarios.
						si quieren droparlo, ya se buscaran la vida */
	{ "sendpass" , nickserv_sendpass , PREOS } ,
	{ "info" , nickserv_info , TODOS } ,
	{ "list" , nickserv_list, TODOS } ,
	{ "ghost" , nickserv_ghost , TODOS } ,
	{ "suspender" , nickserv_suspend , OPERS } ,
	{ "liberar" , nickserv_liberar , OPERS } ,
	{ "addwhois" , nickserv_addwhois , PREOS } ,
	{ "delwhois" , nickserv_delwhois , PREOS } ,
	{ "rename" , nickserv_rename , OPERS } ,
#ifdef UDB
	{ "migrar" , nickserv_migrar , USERS } ,
	{ "demigrar" , nickserv_demigrar , USERS } ,
#endif	
	{ 0x0 , 0x0 , 0x0 }
};
DLLFUNC ModInfo info = {
	"NickServ" ,
	0.7 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
};
	
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	char *file, *k;
	file = strdup(mod->archivo);
	k = strrchr(file, '.') + 1;
	*k = '\0';
	strcat(file, "inc");
	if (parseconf(file, &modulo, 1))
		return 1;
	Free(file);
	if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
	{
		if (!test(modulo.seccion[0], &errores))
			set(modulo.seccion[0], mod);
	}
	else
	{
		conferror("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
		errores++;
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
			{
				irc_dlsym(ex->hmod, "busca_cregistro", (CsRegistros *)busca_cregistro_dl);
				irc_dlsym(ex->hmod, "cflags2str", (char *) cflags2str_dl);
				break;
			}
		}
	}
#endif
	return errores;
}	
DLLFUNC int descarga()
{
	borra_comando(MSG_NICK, nickserv_nick);
	borra_comando(MSG_QUIT, nickserv_quit);
	borra_comando(MSG_UMODE2, nickserv_umode);
	signal_del(SIGN_MYSQL, nickserv_sig_mysql);
	signal_del(SIGN_QUIT, nickserv_sig_quit);
#ifdef UDB
	signal_del(SIGN_EOS, nickserv_sig_eos);
#endif
	signal_del(NS_SIGN_IDOK, nickserv_sig_idok);
	return 0;
}
int test(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = busca_entrada(config, "nick")))
	{
		conferror("[%s:%s] No se encuentra la directriz nick.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "ident")))
	{
		conferror("[%s:%s] No se encuentra la directriz ident.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "realname")))
	{
		conferror("[%s:%s] No se encuentra la directriz realname.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if ((eval = busca_entrada(config, "nicks")) && atoi(eval->data) > 99)
	{
		conferror("[%s:%s::%i] El numero de nicks debe ser inferior a 99.", config->archivo, eval->item, eval->linea);
		error_parcial++;
	}
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *ns;
	if (!nickserv)
		da_Malloc(nickserv, NickServ);
	nickserv->opts = NS_PROT_KILL;
	ircstrdup(&nickserv->recovernick, "inv-%s-????");
	ircstrdup(&nickserv->securepass,"******");
	nickserv->min_reg = 24;
	nickserv->autodrop = 15;
	nickserv->nicks = 3;
	nickserv->intentos = 3;
	nickserv->maxlist = 30;
	nickserv->forbmails = 0;
	nickserv->comandos = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&nickserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&nickserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&nickserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&nickserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&nickserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "sid"))
			nickserv->opts |= NS_SID;
		if (!strcmp(config->seccion[i]->item, "secure"))
			nickserv->opts |= NS_SMAIL;
		if (!strcmp(config->seccion[i]->item, "prot"))
		{
			nickserv->opts &= ~(NS_PROT_KILL | NS_PROT_CHG);
			if (!strcasecmp(config->seccion[i]->data, "KILL"))
				nickserv->opts |= NS_PROT_KILL;
			else
				nickserv->opts |= NS_PROT_CHG;
		}
#ifdef UDB
		if (!strcmp(config->seccion[i]->item, "automigrar"))
			nickserv->opts |= NS_AUTOMIGRAR;
#endif
		if (!strcmp(config->seccion[i]->item, "recovernick"))
			ircstrdup(&nickserv->recovernick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "securepass"))
			ircstrdup(&nickserv->securepass, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "min_reg"))
			nickserv->min_reg = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "autodrop"))
			nickserv->autodrop = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "nicks"))
			nickserv->nicks = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "intentos"))
			nickserv->intentos = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "maxlist"))
			nickserv->maxlist = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "forbmails"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++, nickserv->forbmails++)
				ircstrdup(&nickserv->forbmail[nickserv->forbmails], config->seccion[i]->seccion[p]->item);
		}
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ns = &nickserv_coms[0];
				while (ns->com != 0x0)
				{
					if (!strcasecmp(ns->com, config->seccion[i]->seccion[p]->item))
					{
						nickserv->comando[nickserv->comandos++] = ns;
						break;
					}
					ns++;
				}
				if (ns->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&nickserv->residente, config->seccion[i]->data);
	}
	if (nickserv->mascara)
		Free(nickserv->mascara);
	nickserv->mascara = (char *)Malloc(sizeof(char) * (strlen(nickserv->nick) + 1 + strlen(nickserv->ident) + 1 + strlen(nickserv->host) + 1));
	sprintf_irc(nickserv->mascara, "%s!%s@%s", nickserv->nick, nickserv->ident, nickserv->host);
	inserta_comando(MSG_NICK, TOK_NICK, nickserv_nick, INI);
	inserta_comando(MSG_QUIT, TOK_QUIT, nickserv_quit, INI);
	inserta_comando(MSG_UMODE2, TOK_UMODE2, nickserv_umode, INI);
	signal_add(SIGN_MYSQL, nickserv_sig_mysql);
	signal_add(SIGN_QUIT, nickserv_sig_quit);
#ifdef UDB
	signal_add(SIGN_EOS, nickserv_sig_eos);
#endif
	signal_add(NS_SIGN_IDOK, nickserv_sig_idok);
	proc(nickserv_dropanicks);
	mod->nick = nickserv->nick;
	mod->ident = nickserv->ident;
	mod->host = nickserv->host;
	mod->realname = nickserv->realname;
	mod->residente = nickserv->residente;
	mod->modos = nickserv->modos;
	mod->comandos = nickserv->comando;
}
void envia_clave(char *nick)
{
	char *pass, *passmd5;
	pass = random_ex(nickserv->securepass);
	passmd5 = MDString(pass);
	_mysql_add(NS_MYSQL, nick, "pass", passmd5);
	email(_mysql_get_registro(NS_MYSQL, nick, "email"), "Reenvío de la contraseña", "Debido a la pérdida de tu contraseña, se te ha generado otra clave totalmetne segura.\r\n"
		"A partir de ahora, la clave de tu nick es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, nickserv->nick, conf_set->red);
#ifdef UDB
	if (IsNickUDB(nick))
	{
		char buf[256];
		if (IsSusp(nick))
		{
			sprintf_irc(buf, "%s+", pass);
			pass = buf;
		}
		envia_registro_bdd("N::%s::pass %s", nick, passmd5);
		envia_registro_bdd("N::%s::desafio md5", nick);
	}
#endif
}
BOTFUNC(nickserv_help)
{
	if (params < 2)
	{
		response(cl, nickserv->nick, "\00312%s\003 se encarga de gestionar los nicks de la red para evitar robos de identidad.", nickserv->nick);
		response(cl, nickserv->nick, "El registro de tu nick permite que otros lo utilicen, mostrarte como propietario y aventajarte de cuantiosas oportunidades.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Comandos disponibles:");
		response(cl, nickserv->nick, "\00312REGISTER\003 Registra tu nick.");
		response(cl, nickserv->nick, "\00312IDENTIFY\003 Te identifica como propietario del nick.");
		response(cl, nickserv->nick, "\00312INFO\003 Muestra información de un nick.");
		response(cl, nickserv->nick, "\00312LIST\003 Lista todos los nicks que coinciden con un patrón.");
		response(cl, nickserv->nick, "\00312GHOST\003 Desconecta a un usuario que utilice su nick.");
		if (IsId(cl))
		{
			response(cl, nickserv->nick, "\00312SET\003 Permite fijar distintas opciones para tu nick.");
#ifdef UDB
			response(cl, nickserv->nick, "\00312MIGRAR\003 Migra tu nick a la BDD.");
			response(cl, nickserv->nick, "\00312DEMIGRAR\003 Demigra tu nick de la BDD.");
#endif			
		}
		if (IsPreo(cl))
		{
			response(cl, nickserv->nick, "\00312DROP\003 Desregistra un usuario.");
			response(cl, nickserv->nick, "\00312SENDPASS\003 Genera un password para el usuario y se lo envía a su correo.");
			response(cl, nickserv->nick, "\00312ADDWHOIS\003 Añade un whois especial al nick especificado.");
			response(cl, nickserv->nick, "\00312DELWHOIS\003 Borra un whois especial.");
		}
		if (IsOper(cl))
		{
			response(cl, nickserv->nick, "\00312SUSPENDER\003 Suspende los privilegios de un nick.");
			response(cl, nickserv->nick, "\00312LIBERAR\003 Libera un nick de su suspenso.");
			response(cl, nickserv->nick, "\00312RENAME\003 Cambia el nick a un usuario.");
		}
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Para más información, \00312/msg %s %s comando", nickserv->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "REGISTER"))
	{
		response(cl, nickserv->nick, "\00312REGISTER");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Permite registrar tu nick en la red para que nadie sin autorización pueda utilizarlo.");
		response(cl, nickserv->nick, "Para utilizar tu nick registrado deberás identificarte como propietario mediante el comando \00312IDENTIFY\003.");
		response(cl, nickserv->nick, "El registro del nick requiere una cuenta de email válida. Recuerda que si no lo es, no podrás recibir tu nueva contraseña en caso de que la perdieras.");
		if (nickserv->opts & NS_SMAIL)
			response(cl, nickserv->nick, "Una vez hayas registrado tu nick, te llegará en tu correo email una clave temporal, que será la que deberás usar para identificarte.");
		response(cl, nickserv->nick, " ");
		if (nickserv->opts & NS_SMAIL)
			response(cl, nickserv->nick, "Sintaxis: \00312REGISTER tu@email");
		else
			response(cl, nickserv->nick, "Sintaxis: \00312REGISTER tupass tu@email");
	}
	else if (!strcasecmp(param[1], "IDENTIFY"))
	{
		response(cl, nickserv->nick, "\00312IDENTIFY");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Te identifica como propietario del nick.");
		response(cl, nickserv->nick, "Para ello debes proporcionar una contraseña de autentificación, la misma que utilizaste en el registro de tu nick.");
		response(cl, nickserv->nick, "Una vez identificado como propietario podrás disponer de cuantiosas oportunidades que te ofrece el reigstro de nicks.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312IDENTIFY tupass");
	}
	else if (!strcasecmp(param[1], "INFO"))
	{
		response(cl, nickserv->nick, "\00312INFO");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Muestra distinta información de un nick, como su fecha de registro o su último quit, entre otros.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312INFO nick");
	}
	else if (!strcasecmp(param[1], "LIST"))
	{
		response(cl, nickserv->nick, "\00312LIST");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Lista todos los nicks registrados que coincidan con un patrón especificado.");
		response(cl, nickserv->nick, "Para evitar abusos de flood, este comando emite como máximo \00312%i\003 entradas.", nickserv->maxlist);
		response(cl, nickserv->nick, "El patrón puede ser exacto, o puede contener comodines (*) para cerrar el campo de búsqueda.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312LIST patron");
	}
	else if (!strcasecmp(param[1], "GHOST"))
	{
		response(cl, nickserv->nick, "\00312GHOST");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Este comando desconecta a un usuario que lleve puesto tu nick.");
		response(cl, nickserv->nick, "Para ello deberás especificar la contraseña de propietario del nick.");
		response(cl, nickserv->nick, "Si la contraseña es válida, %s.", nickserv->opts & NS_PROT_KILL ? "se desconectará el usuario" : "se le cambiará el nick al usuario");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312GHOST nick contraseña");
	}
	else if (!strcasecmp(param[1], "SET") && IsId(cl))
	{
		if (params < 3)
		{
			response(cl, nickserv->nick, "\00312SET");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Permite fijar diferentes opciones para tu nick.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Opciones disponibles:");
			response(cl, nickserv->nick, "\00312PASS\003 Cambia tu contraseña de usuario.");
			response(cl, nickserv->nick, "\00312EMAIL\003 Cambia tu dirección de correo.");
			response(cl, nickserv->nick, "\00312URL\003 Cambia tu página web.");
			response(cl, nickserv->nick, "\00312KILL\003 Activa/Desactiva tu protección de KILL.");
			if (IsOper(cl))
				response(cl, nickserv->nick, "\00312NODROP\003 Activa/Desactiva tu protección de DROP.");
			response(cl, nickserv->nick, "\00312HIDE\003 Opciones para mostrar en tu INFO.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Para más información, \00312/msg %s %s SET opción", nickserv->nick, strtoupper(param[0]));
		}
		else if (!strcasecmp(param[2], "PASS"))
		{
			response(cl, nickserv->nick, "\00312SET PASS");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Cambia tu contraseña de usuario.");
			response(cl, nickserv->nick, "A partir de este cambio, ésta será la contraseña con la que debes identificarte.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312SET PASS nuevopass");
		}
		else if (!strcasecmp(param[2], "EMAIL"))
		{
			response(cl, nickserv->nick, "\00312SET EMAIL");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Cambia tu dirección de correo.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312SET email direccion@correo");
		}
		else if (!strcasecmp(param[2], "URL"))
		{
			response(cl, nickserv->nick, "\00312SET URL");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Cambia tu página web personal.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312SET url http://pagina.web");
		}
		else if (!strcasecmp(param[2], "KILL"))
		{
			response(cl, nickserv->nick, "\00312SET KILL");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Especifica tu protección KILL.");
			response(cl, nickserv->nick, "Con esta protección, si no te identificas antes de los segundos que especifiques %s.", nickserv->opts & NS_PROT_KILL ? "serás desconectado" : "se te cambiará el nick");
			response(cl, nickserv->nick, "Si especificas el 0 como valor, esta protección se desactiva.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312KILL segundos");
		}
		else if (!strcasecmp(param[2], "NODROP") && IsOper(cl))
		{
			response(cl, nickserv->nick, "\00312SET NODROP");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Evita que tu nick pueda ser dropeado, a excepción de los Administradores.");
			response(cl, nickserv->nick, "Si está en ON, tu nick no podrá dropearse.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312SET NODROP on|off");
		}
		else if (!strcasecmp(param[2], "HIDE"))
		{
			response(cl, nickserv->nick, "\00312SET HIDE");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Permite mostrar u ocultar distintas opciones de tu INFO.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Opciones para mostrar:");
			response(cl, nickserv->nick, "\00312EMAIL\003 Muestra tu email.");
			response(cl, nickserv->nick, "\00312URL\003 Muestra tu página personal.");
			response(cl, nickserv->nick, "\00312MASK\003 Muestra tu user@host.");
			response(cl, nickserv->nick, "\00312TIME\003 Muestra la última hora que fuiste visto en la red.");
			response(cl, nickserv->nick, "\00312QUIT\003 Muestra tu último quit.");
			response(cl, nickserv->nick, "\00312LIST\003 Se te mostrará cuando alguien haga un LIST.");
			response(cl, nickserv->nick, " ");
			response(cl, nickserv->nick, "Sintaxis: \00312SET HIDE opcion on|off");
		}
		else
			response(cl, nickserv->nick, NS_ERR_EMPT, "Opción desconocida.");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MIGRAR") && IsId(cl))
	{
		response(cl, nickserv->nick, "\00312MIGRAR");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Migra tu nick a la base de datos.");
		response(cl, nickserv->nick, "Una vez migrado tu nick, la única forma de ponérselo sólo podrá ser vía /nick tunick:tupass.");
		response(cl, nickserv->nick, "Si la contraseña es correcta, recibirás el modo de usuario +r quedando identificado automáticamente.");
		response(cl, nickserv->nick, "De lo contrario, no podrás utilizar este nick.");
		response(cl, nickserv->nick, "Adicionalmente, puedes utilizar /nick tunick!tupass para desconectar una sesión que pudiere utilizar tu nick.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312MIGRAR tupass");
	}
	else if (!strcasecmp(param[1], "DEMIGRAR") && IsId(cl))
	{
		response(cl, nickserv->nick, "\00312DEMIGRAR");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Borra tu nick de la BDD.");
		response(cl, nickserv->nick, "Una vez borrado, la identificación procederá con el comando IDENTIFY.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312DEMIGRAR tupass");
	}
#endif
	else if (!strcasecmp(param[1], "DROP") && IsPreo(cl))
	{
		response(cl, nickserv->nick, "\00312DROP");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Borra un nick de la base de datos.");
		response(cl, nickserv->nick, "Este desregistro anula completamente la propiedad de un usuario sobre un nick.");
		response(cl, nickserv->nick, "Una vez borrado, no se puede volver a recuperar si no es registrándolo de nuevo.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312DROP nick");
	}
	else if (!strcasecmp(param[1], "SENDPASS") && IsPreo(cl))
	{
		response(cl, nickserv->nick, "\00312SENDPASS");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Regenera una contraseña de usuario y se la envía a su correo.");
		response(cl, nickserv->nick, "Este comando sólo debe en caso de pérdida de la contraseña del usuario y a petición del mismo.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312SENDPASS nick");
	}
	else if (!strcasecmp(param[1], "ADDWHOIS") && IsPreo(cl))
	{
		response(cl, nickserv->nick, "\00312ADDWHOIS");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Añade un whois especial a un usuario.");
		response(cl, nickserv->nick, "Este whois se le añadirá cuando se identifique.");
		response(cl, nickserv->nick, "Se mostrará con el comando WHOIS.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312ADDWHOIS nick swhois");
	}
	else if (!strcasecmp(param[1], "DELWHOIS") && IsPreo(cl))
	{
		response(cl, nickserv->nick, "\00312DELWHOIS");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Borra un whois especial.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312DELWHOIS nick");
	}
	else if (!strcasecmp(param[1], "SUSPENDER") && IsOper(cl))
	{
		response(cl, nickserv->nick, "\00312SUSPENDER");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Suspende el uso de un determinado nick.");
		response(cl, nickserv->nick, "Este nick podrá ponerselo pero no tendrá privilegios algunos.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312SUSPENDER nick motivo");
	}
	else if (!strcasecmp(param[1], "LIBERAR") && IsOper(cl))
	{
		response(cl, nickserv->nick, "\00312LIBERAR");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Libera un nick de su suspenso.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312LIBERAR nick");
	}
	else if (!strcasecmp(param[1], "RENAME") && IsOper(cl))
	{
		response(cl, nickserv->nick, "\00312RENAME");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Cambia el nick a un usuario conectado.");
		response(cl, nickserv->nick, " ");
		response(cl, nickserv->nick, "Sintaxis: \00312RENAME nick nuevonick");
	}
	else
		response(cl, nickserv->nick, NS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(nickserv_reg)
{
	int i, opts;
	char *dominio;
	char *mail, *pass;
	MYSQL_RES *res;
	if (params < ((nickserv->opts & NS_SMAIL) ? 2 : 3))
	{
		if (nickserv->opts & NS_SMAIL)
			response(cl, nickserv->nick, NS_ERR_PARA, "REGISTER tu@email");
		else
			response(cl, nickserv->nick, NS_ERR_PARA, "REGISTER tupass tu@email");
		return 1;
	}
	if ((cl->ultimo_reg + (3600 * nickserv->min_reg)) > time(0))
	{
		char buf[512];
		sprintf_irc(buf, "No puedes efectuar otro registro hasta que no pasen %i horas.", nickserv->min_reg);
		response(cl, nickserv->nick, NS_ERR_EMPT, buf);
		return 1;
	}	
	/* comprobamos que no esté registrado */
	if (IsReg(cl->nombre))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Este nick ya está registrado.");
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
		response(cl, nickserv->nick, NS_ERR_EMPT, "No parece ser una cuenta de email válida.");
		return 1;
	}
	for (i = 0; i < nickserv->forbmails; i++)
	{
		if (!strcasecmp(nickserv->forbmail[i], dominio))
		{
			response(cl, nickserv->nick, NS_ERR_EMPT, "No puedes utilizar una cuenta de correo gratuíta.");
			return 1;
		}
	}
	if (nickserv->nicks && ((res = _mysql_query("SELECT * from %s%s where email='%s'", PREFIJO, NS_MYSQL, mail)) && mysql_num_rows(res) == nickserv->nicks))
	{
		mysql_free_result(res);
		response(cl, nickserv->nick, NS_ERR_EMPT, "No puedes registrar más nicks en esta cuenta.");
		return 1;
	}
	mysql_free_result(res);
	opts = NS_OPT_MASK;
#ifdef UDB
	if (nickserv->opts & NS_AUTOMIGRAR)
		opts |= NS_OPT_UDB;
#endif
	if (pass)
		_mysql_add(NS_MYSQL, parv[0], "pass", MDString(pass));
	_mysql_add(NS_MYSQL, parv[0], "email", mail);
	_mysql_add(NS_MYSQL, parv[0], "gecos", cl->info);
	_mysql_add(NS_MYSQL, parv[0], "host", "%s@%s", cl->ident, cl->host);
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	_mysql_add(NS_MYSQL, parv[0], "id", "%i", nickserv->opts & NS_SMAIL ? 0 : time(0));
	_mysql_add(NS_MYSQL, parv[0], "reg", "%i", time(0));
	_mysql_add(NS_MYSQL, parv[0], "last", "%i", time(0));
	if (nickserv->opts & NS_SMAIL)
		envia_clave(parv[0]);
	else
		envia_umodos(cl, nickserv->nick, "+r");
	cl->ultimo_reg = time(0);
	response(cl, nickserv->nick, "Tu nick ha sido registrado bajo la cuenta \00312%s\003.", mail);
#ifdef UDB
	if (nickserv->opts & NS_AUTOMIGRAR)
	{
		if (pass)
		{
			envia_registro_bdd("N::%s::pass %s", cl->nombre, MDString(pass));
			envia_registro_bdd("N::%s::desafio md5", cl->nombre);
		}
		cambia_nick_inv(parv[0]);
	}
#endif
	signal1(NS_SIGN_REG, parv[0]);
	return 0;
}
BOTFUNC(nickserv_identify)
{
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "IDENTIFY tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "No tienes el nick registrado.");
		return 1;
	}
	if (IsSusp(parv[0])
#ifdef UDB
	 || (cl->modos & UMODE_SUSPEND)
#endif
	 )
	{
		sprintf_irc(buf, "Tienes el nick suspendido: %s", _mysql_get_registro(NS_MYSQL, parv[0], "suspend"));
		response(cl, nickserv->nick, NS_ERR_EMPT, buf);
		return 1;
	}
	if ((nickserv->opts & NS_SID) && !parv[parc])
	{
		sprintf_irc(buf, "Identificación incorrecta. /msg %s@%s IDENTIFY pass", nickserv->nick, me.nombre);
		response(cl, nickserv->nick, NS_ERR_EMPT, buf);
		return 1;
	}
	if (!strcmp(MDString(param[1]), _mysql_get_registro(NS_MYSQL, parv[0], "pass")))
	{
		envia_umodos(cl, nickserv->nick, "+r");
		response(cl, nickserv->nick, "Ok \00312%s\003, bienvenid@ a casa :)", parv[0]);
		signal1(NS_SIGN_IDOK, cl);
	}
	else
	{
		if (++cl->intentos == nickserv->intentos)
			irckill(cl, nickserv->nick, "Demasiadas contraseñas inválidas.");
		else
		{
			sprintf_irc(buf, "Contraseña incorrecta. Tienes %i intentos más.", nickserv->intentos - cl->intentos);
			response(cl, nickserv->nick, buf);
		}
	}
	return 0;
}
BOTFUNC(nickserv_set)
{
	if (params < 3)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "SET opción valor");
		return 1;
	}
	if (!strcasecmp(param[1], "EMAIL"))
	{
		_mysql_add(NS_MYSQL, parv[0], "email", param[2]);
		response(cl, nickserv->nick, "Tu email se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "URL"))
	{
		_mysql_add(NS_MYSQL, parv[0], "url", param[2]);
		response(cl, nickserv->nick, "Tu url se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "KILL"))
	{
		_mysql_add(NS_MYSQL, parv[0], "kill", param[2]);
		response(cl, nickserv->nick, "Tu kill se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "NODROP"))
	{
		if (IsOper(cl))
		{
			int opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
			if (!strcasecmp(param[2], "ON"))
				opts |= NS_OPT_NODROP;
			else
				opts &= ~NS_OPT_NODROP;
			_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
			response(cl, nickserv->nick, "El DROP de tu nick se ha cambiado a \00312%s\003.", param[2]);
		}
		else
		{
			response(cl, nickserv->nick, NS_ERR_FORB);
			return 1;
		}
		return 0;
	}
	else if (!strcasecmp(param[1], "PASS"))
	{
		char *passmd5 = MDString(param[2]);
		_mysql_add(NS_MYSQL, parv[0], "pass", passmd5);
#ifdef UDB
		if (IsNickUDB(parv[0]))
			envia_registro_bdd("N::%s::pass %s", parv[0], passmd5);
#endif
		response(cl, nickserv->nick, "Tu contraseña se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "HIDE"))
	{
		int opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
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
			response(cl, nickserv->nick, NS_ERR_EMPT, "Opción incorrecta.");
			return 1;
		}
		_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
		response(cl, nickserv->nick, "Hide %s cambiado a \00312%s\003.", param[2], param[3]);
		return 0;
	}
	else
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Opción incorrecta.");
		return 1;
	}
	return 0;
}
BOTFUNC(nickserv_drop)
{
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "DROP nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	if (!IsAdmin(cl) && (atoi(_mysql_get_registro(NS_MYSQL, param[1], "opts")) & NS_OPT_NODROP))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Este nick no se puede dropear.");
		return 1;
	}
	if (!nickserv_baja(param[1], 1))
		response(cl, nickserv->nick, "Se ha dropeado el nick \00312%s\003.", param[1]);
	else
		response(cl, nickserv->nick, NS_ERR_EMPT, "No se ha podido dropear. Comuníquelo a la administración.");
	return 0;
}
int nickserv_baja(char *nick, char opt)
{
	Cliente *al;
	if (!opt && atoi(_mysql_get_registro(NS_MYSQL, nick, "opts")) & NS_OPT_NODROP)
		return 1;
	al = busca_cliente(nick, NULL);
	signal1(NS_SIGN_DROP, nick);
	if (al)
		envia_umodos(al, nickserv->nick, "-r");
#ifdef UDB
	if (IsNickUDB(nick))
		envia_registro_bdd("N::%s", nick);
#endif
	_mysql_del(NS_MYSQL, nick);
	return 0;
}
BOTFUNC(nickserv_sendpass)
{
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "SENDPASS nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	envia_clave(param[1]);
	response(cl, nickserv->nick, "Se ha generado y enviado otra contraseña al email de \00312%s\003.", param[1]);
	return 0;
}
BOTFUNC(nickserv_info)
{
	int comp;
	MYSQL_RES *res;
	MYSQL_ROW row;
	time_t reg;
	int opts;
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "INFO nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	comp = strcasecmp(param[1], parv[0]);
	res = _mysql_query("SELECT opts,gecos,reg,suspend,host,quit,last,email,url,`kill`,item from %s%s where item='%s'", PREFIJO, NS_MYSQL, param[1]);
	row = mysql_fetch_row(res);
	opts = atoi(row[0]);
	response(cl, nickserv->nick, "Información de \00312%s", row[10]);
	response(cl, nickserv->nick, "Realname: \00312%s", row[1]);
	reg = (time_t)atol(row[2]);
	response(cl, nickserv->nick, "Registrado el \00312%s", _asctime(&reg));
	if (!BadPtr(row[3]))
		response(cl, nickserv->nick, "Está suspendido: \00312%s", row[3]);
	if (!(opts & NS_OPT_MASK) || (!comp && IsId(cl)) || IsOper(cl))
		response(cl, nickserv->nick, "Último mask: \00312%s", row[4]);
	if (!(opts & NS_OPT_TIME) || (!comp && IsId(cl)) || IsOper(cl))
	{
		Cliente *al;
		al = busca_cliente(param[1], NULL);
		if (al && IsId(al))
			response(cl, nickserv->nick, "Este usuario está conectado. Utiliza /WHOIS %s para más información.", param[1]);
		else
		{
			reg = (time_t)atol(row[6]);
			response(cl, nickserv->nick, "Visto por última vez el \00312%s", _asctime(&reg));
		}
	}
	if (!BadPtr(row[5]) && (!(opts & NS_OPT_QUIT) || (!comp && IsId(cl)) || IsOper(cl)))
		response(cl, nickserv->nick, "Último quit: \00312%s", row[5]);
	if (!(opts & NS_OPT_MAIL) || (!comp && IsId(cl)) || IsOper(cl))
		response(cl, nickserv->nick, "Email: \00312%s", row[7]);
	if (!BadPtr(row[8]) && (!(opts & NS_OPT_WEB) || (!comp && IsId(cl)) || IsOper(cl)))
		response(cl, nickserv->nick, "Url: \00312%s", row[8]);
	if (atoi(row[9]))
		response(cl, nickserv->nick, "Protección kill a \00312%i\003 segundos.", atoi(row[9]));
#ifdef UDB
	if (opts & NS_OPT_UDB)
		response(cl, nickserv->nick, "Este nick está migrado a la \2BDD");
#endif
	mysql_free_result(res);
	if ((!strcasecmp(parv[0], param[1]) && IsId(cl)) || IsOper(cl))
	{
		CsRegistros *regs;
		int i;
		if ((regs = busca_cregistro_dl(param[1])))
		{
			response(cl, nickserv->nick, "*** Niveles de acceso ***");
			for (i = 0; i < regs->subs; i++)
				response(cl, nickserv->nick, "Canal: \00312%s\003 flags:\00312+%s\003 (\00312%lu\003)", regs->sub[i]->canal, 
#ifdef _WIN32				
				cflags2str(regs->sub[i]->flags),
#else
				cflags2str_dl(regs->sub[i]->flags),
#endif
				regs->sub[i]->flags);
		}
	}
	return 0;
}
BOTFUNC(nickserv_list)
{
	char *rep;
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "LIST patrón");
		return 1;
	}
	rep = str_replace(param[1], '*', '%');
	if (!(res = _mysql_query("SELECT item from %s%s where item LIKE '%s'", PREFIJO, NS_MYSQL, rep)))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "No se han encontrado coincidencias.");
		return 1;
	}
	response(cl, nickserv->nick, "*** Nicks que coinciden con el patrón \00312%s\003 ***", param[1]);
	for (i = 0; i < nickserv->maxlist && (row = mysql_fetch_row(res)); i++)
	{
		if (IsOper(cl) || !(atoi(_mysql_get_registro(NS_MYSQL, row[0], "opts")) & NS_OPT_LIST))
			response(cl, nickserv->nick, "\00312%s", row[0]);
		else
			i--;
	}
	response(cl, nickserv->nick, "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
	mysql_free_result(res);
	return 0;
}
BOTFUNC(nickserv_ghost)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "GHOST nick pass");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (!strcasecmp(param[1], parv[0]))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "No puedes ejecutar este comando sobre ti mismo.");
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, param[1], "pass"), MDString(param[2])))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	if (nickserv->opts & NS_PROT_KILL)
		irckill(al, nickserv->nick, "Comando GHOST utilizado por %s.", parv[0]);
	else
		cambia_nick_inv(al->nombre);
	response(cl, nickserv->nick, "Nick \00312%s\003 liberado.", param[1]);
	return 0;
}
BOTFUNC(nickserv_suspend)
{
#ifdef UDB
	Cliente *al;
#endif	
	char *motivo;
	if (params < 3)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "SUSPENDER nick motivo");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	motivo = implode(param, params, 2, -1);
	_mysql_add(NS_MYSQL, param[1], "suspend", motivo);
#ifdef UDB
	if ((al = busca_cliente(param[1], NULL)))
		envia_umodos(al, nickserv->nick, "+S-r");
	if (IsNickUDB(param[1]))
		envia_registro_bdd("N::%s::suspendido %s", param[1], motivo);
#endif
	response(cl, nickserv->nick, "El nick \00312%s\003 ha sido suspendido.", param[1]);
	return 0;
}
BOTFUNC(nickserv_liberar)
{
#ifdef UDB 
	Cliente *al;
#endif
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "LIBERAR nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	if (!_mysql_get_registro(NS_MYSQL, param[1], "suspend"))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Este nick no está suspendido.");
		return 1;
	}
	_mysql_add(NS_MYSQL, param[1], "suspend", "");
#ifdef UDB
	if ((al = busca_cliente(param[1], NULL)))
		envia_umodos(al, nickserv->nick, "-S");
	if (IsNickUDB(param[1]))
		envia_registro_bdd("N::%s::suspendido", param[1]);
#endif
	response(cl, nickserv->nick, "El nick \00312%s\003 ha sido liberado de su suspenso.", param[1]);			
	return 0;
}
BOTFUNC(nickserv_addwhois)
{
	char *swhois;
	if (params < 3)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "ADDWHOIS nick swhois");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	swhois = implode(param, params, 2, -1);
	_mysql_add(NS_MYSQL, param[1], "swhois", swhois);
	if ((busca_cliente(param[1], NULL)))
		sendto_serv_us(&me, MSG_SWHOIS, TOK_SWHOIS, "%s :%s", param[1], swhois);
	response(cl, nickserv->nick, "El swhois de \00312%s\003 ha sido cambiado.", param[1]);
	return 0;
}
BOTFUNC(nickserv_delwhois)
{
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "DELWHOIS nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, nickserv->nick, NS_ERR_NURG);
		return 1;
	}
	_mysql_add(NS_MYSQL, param[1], "swhois", "");
	if ((busca_cliente(param[1], NULL)))
		sendto_serv_us(&me, MSG_SWHOIS, TOK_SWHOIS, "%s :", param[1]);
	response(cl, nickserv->nick, "El swhois de \00312%s\003 ha sido borrado.", param[1]);
	return 0;
}
BOTFUNC(nickserv_rename)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "RENAME nick nuevonick");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Este nick no está conectado.");
		return 1;
	}
	if ((al = busca_cliente(param[2], NULL)))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "El nuevo nick ya está en uso.");
		return 1;
	}
	sendto_serv_us(&me, MSG_SVSNICK, TOK_SVSNICK, "%s %s %lu", param[1], param[2], time(0));
	response(cl, nickserv->nick, "El nick \00312%s\003 ha cambiado su nick a \00312%s\003.", param[1], param[2]);
	return 0;
}
#ifdef UDB
BOTFUNC(nickserv_migrar)
{
	int opts;
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "MIGRAR tupass");
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, parv[0], "pass"), MDString(param[1])))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
	if (opts & NS_OPT_UDB)
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Ya tienes el nick migrado.");
		return 1;
	}
	envia_registro_bdd("N::%s::pass %s", parv[0], MDString(param[1]));
	envia_registro_bdd("N::%s::desafio md5", parv[0]);
	response(cl, nickserv->nick, "Migración realizada.");
	opts |= NS_OPT_UDB;
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	cambia_nick_inv(parv[0]);
	return 0;
}
BOTFUNC(nickserv_demigrar)
{
	int opts;
	if (params < 2)
	{
		response(cl, nickserv->nick, NS_ERR_PARA, "DEMIGRAR tupass");
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, parv[0], "pass"), MDString(param[1])))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
	if (!(opts & NS_OPT_UDB))
	{
		response(cl, nickserv->nick, NS_ERR_EMPT, "Tu nick no está migrado.");
		return 1;
	}
	envia_registro_bdd("N::%s", parv[0]);
	response(cl, nickserv->nick, "Demigración realizada.");
	opts &= ~NS_OPT_UDB;
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	return 0;
}
#endif
IRCFUNC(nickserv_umode)
{
	if (conf_set->modos && conf_set->modos->usuarios)
		envia_umodos(cl, nickserv->nick, conf_set->modos->usuarios);
	if (strchr(parc < 3 ? parv[1] : parv[7], 'r') && (cl->modos & UMODE_REGNICK))
		signal1(NS_SIGN_IDOK, cl);
	return 0;
}
IRCFUNC(nickserv_quit)
{
	signal2(SIGN_QUIT, cl, parv[1]);
	return 0;
}
IRCFUNC(nickserv_nick)
{
	char *nick = (parc < 4 ? parv[1] : parv[0]), *motivo = NULL;
#ifndef UDB
	if ((motivo = _mysql_get_registro(NS_FORBIDS, nick, "motivo")))
	{
		response(cl, nickserv->nick, "Este nick está prohibido: %s", motivo);
		cambia_nick_inv(nick);
		return 0;
	}
#endif
	if (parc < 4)
	{
		if (IsReg(parv[0]))
		{
			if (IsId(cl))
				_mysql_add(NS_MYSQL,parv[0], "last", "%i", time(0));
			else
				timer_off(parv[0], cl->sck); /* es posible que tenga kill */
		}
	}
	else if (parc == 10)
	{
		cl->intentos = 0;
		nickserv_umode(sck, cl, parv, parc);
	}
	if (!IsId(cl) && IsReg(nick)
#ifdef UDB
	 && !IsNickUDB(nick)
#endif
	 )
	{
		char *kill;
		if (nickserv->opts & NS_SID)
			sprintf_irc(buf, "%s@%s", nickserv->nick, me.nombre);
		else
			sprintf_irc(buf, "%s", nickserv->nick);
		response(cl, nickserv->nick, "Este nick está protegido. Si es tu nick escribe \00312/msg %s IDENTIFY tupass\003. Si no lo es, escoge otro.", buf);
		kill = _mysql_get_registro(NS_MYSQL, nick, "kill");
		if (kill && atoi(kill))
		{
			response(cl, nickserv->nick, "De no hacerlo, serás desconectado en \00312%s\003 segundos.", kill);
			timer(nick, cl->sck, 1, atoi(kill), nickserv_killid, nick, sizeof(char) * (strlen(nick) + 1));
		}
	}
	return 0;
}
int nickserv_sig_mysql()
{
	int i;
#ifndef UDB
	char buf[2][BUFSIZE], tabla[2];
	tabla[0] = tabla[1] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, NS_MYSQL);
	sprintf_irc(buf[1], "%s%s", PREFIJO, NS_FORBIDS);
#else
	char buf[1][BUFSIZE], tabla[1];
	tabla[0] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, NS_MYSQL);
#endif
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
#ifndef UDB
		else if (!strcasecmp(mysql_tablas.tabla[i], buf[1]))
			tabla[1] = 1;
#endif
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`n` int(11) NOT NULL auto_increment, "
  			"`item` text NOT NULL, "
  			"`pass` text NOT NULL, "
  			"`email` text NOT NULL, "
  			"`url` text NOT NULL, "
  			"`gecos` text NOT NULL, "
  			"`host` text NOT NULL, "
  			"`opts` mediumint(9) NOT NULL default '0', "
  			"`id` bigint(20) NOT NULL default '0', "
  			"`reg` bigint(20) NOT NULL default '0', "
  			"`last` bigint(20) NOT NULL default '0', "
  			"`quit` text NOT NULL, "
  			"`suspend` text, "
  			"`kill` smallint(6) NOT NULL default '0', "
  			"`swhois` text NOT NULL, "
  			"KEY `item` (`n`) "
			") TYPE=MyISAM COMMENT='Tabla de nicks';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
#ifndef UDB
	if (!tabla[1])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`motivo` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de nicks prohibidos';", buf[1]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[1]);
	}
#endif
	_mysql_carga_tablas();
	return 1;
}
int nickserv_sig_quit(Cliente *cl, char *msg)
{
	if (IsId(cl) && IsReg(cl->nombre))
	{
		_mysql_add(NS_MYSQL, cl->nombre, "quit", msg);
		_mysql_add(NS_MYSQL, cl->nombre, "last", "%i", time(0));
	}
	return 0;
}
int nickserv_sig_idok(Cliente *cl)
{
	if (IsReg(cl->nombre))
	{
		char *swhois;
		_mysql_add(NS_MYSQL, cl->nombre, "id", "%i", time(0));
		_mysql_add(NS_MYSQL, cl->nombre, "host", "%s@%s", cl->ident, cl->host);
		_mysql_add(NS_MYSQL, cl->nombre, "gecos", cl->info);
		timer_off(cl->nombre, cl->sck);
		cl->intentos = 0;
		if ((swhois = _mysql_get_registro(NS_MYSQL, cl->nombre, "swhois")))
			sendto_serv_us(&me, MSG_SWHOIS, TOK_SWHOIS, "%s :%s", cl->nombre, swhois);
		cl->nivel |= USER;
	}
	return 0;
}
int nickserv_killid(char *quien)
{
	Cliente *al;
	if ((al = busca_cliente(quien, NULL)) && !IsId(al))
	{
		if (nickserv->opts & NS_PROT_KILL)
			irckill(al, nickserv->nick, "Protección de NICK.");
		else
			cambia_nick_inv(quien);
	}
	return 0;
}
int nickserv_dropanicks(Proc *proc)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = _mysql_query("SELECT item,reg,id from %s%s LIMIT %i,30", PREFIJO, NS_MYSQL, proc->proc)) || !mysql_num_rows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			return 1;
		}
		while ((row = mysql_fetch_row(res)))
		{
			if (atoi(row[2]) == 0) /* primer registro */
			{
				if (atoi(row[1]) + 86400 < time(0)) /* 24 horas */
					nickserv_baja(row[0], 0);
			}
			else if (nickserv->autodrop && (atoi(row[2]) + 86400 * nickserv->autodrop < time(0)))
				nickserv_baja(row[0], 0);
		}
		proc->proc += 30;
		mysql_free_result(res);
	}
	return 0;
}
void cambia_nick_inv(char *nick)
{
	buf[0] = '\0';
	sprintf_irc(buf, nickserv->recovernick, nick);
	sendto_serv_us(&me, MSG_SVSNICK, TOK_SVSNICK, "%s %s %lu", nick, random_ex(buf), time(0));
}
#ifdef UDB
DLLFUNC int nickserv_sig_eos()
{
	envia_registro_bdd("S::NickServ %s", nickserv->mascara);
	return 0;
}
#endif
