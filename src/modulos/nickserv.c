/*
 * $Id: nickserv.c,v 1.22 2005-05-18 18:51:07 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

NickServ nickserv;
#define exfunc(x) busca_funcion(nickserv.hmod, x, NULL)

BOTFUNC(nickserv_help);
BOTFUNC(nickserv_reg);
BOTFUNC(nickserv_identify);
BOTFUNC(nickserv_set);
BOTFUNC(nickserv_drop);
BOTFUNC(nickserv_sendpass);
BOTFUNC(nickserv_info);
BOTFUNC(nickserv_list);
BOTFUNC(nickserv_ghost);
BOTFUNC(nickserv_suspend);
BOTFUNC(nickserv_liberar);
BOTFUNC(nickserv_swhois);
BOTFUNC(nickserv_rename);
#ifdef UDB
BOTFUNC(nickserv_migrar);
BOTFUNC(nickserv_demigrar);
#endif
BOTFUNC(nickserv_forbid);

int nickserv_sig_mysql	();
int nickserv_pre_nick		(Cliente *, char *);
int nickserv_post_nick		(Cliente *);
int nickserv_umode		(Cliente *, char *);
int nickserv_sig_quit 	(Cliente *, char *);
int nickserv_sig_idok 	(Cliente *);
#ifdef UDB
int nickserv_sig_eos	();
#endif
int nickserv_killid		(char *);

int nickserv_dropanicks	(Proc *);

int nickserv_baja(char *, char);
void cambia_nick_inv(Cliente *);

#ifndef _WIN32
CsRegistros *(*busca_cregistro_dl)(char *);
mTab *cFlags_dl;
#else
__declspec(dllimport) mTab cFlags[];
#define busca_cregistro_dl busca_cregistro
#define cFlags_dl cFlags
#endif
void set(Conf *, Modulo *);
int test(Conf *, int *);

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
	{ "swhois" , nickserv_swhois , PREOS } ,
	{ "rename" , nickserv_rename , OPERS } ,
#ifdef UDB
	{ "migrar" , nickserv_migrar , USERS } ,
	{ "demigrar" , nickserv_demigrar , USERS } ,
#endif	
	{ "forbid" , nickserv_forbid , ADMINS } ,
	{ 0x0 , 0x0 , 0x0 }
};
ModInfo info = {
	"NickServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
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
		libera_conf(&modulo);
	}
#ifndef _WIN32
	{
		Modulo *ex;
		for (ex = modulos; ex; ex = ex->sig)
		{
			if (!strcasecmp(ex->info->nombre, "ChanServ"))
			{
				irc_dlsym(ex->hmod, "busca_cregistro", busca_cregistro_dl);
				irc_dlsym(ex->hmod, "cFlags", cFlags_dl);
				break;
			}
		}
	}
#endif
	return errores;
}	
int descarga()
{
	borra_senyal(SIGN_PRE_NICK, nickserv_pre_nick);
	borra_senyal(SIGN_POST_NICK, nickserv_post_nick);
	borra_senyal(SIGN_UMODE, nickserv_umode);
	borra_senyal(SIGN_MYSQL, nickserv_sig_mysql);
	borra_senyal(SIGN_QUIT, nickserv_sig_quit);
#ifdef UDB
	borra_senyal(SIGN_EOS, nickserv_sig_eos);
#endif
	borra_senyal(NS_SIGN_IDOK, nickserv_sig_idok);
	proc_stop(nickserv_dropanicks);
	bot_unset(nickserv);
	return 0;
}
int test(Conf *config, int *errores)
{
	return 0;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *ns;
	nickserv.opts = NS_PROT_KILL;
	ircstrdup(nickserv.recovernick, "inv-%s-????");
	ircstrdup(nickserv.securepass,"******");
	nickserv.min_reg = 24;
	nickserv.autodrop = 15;
	nickserv.nicks = 3;
	nickserv.intentos = 3;
	nickserv.maxlist = 30;
	nickserv.forbmails = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "sid"))
			nickserv.opts |= NS_SID;
		else if (!strcmp(config->seccion[i]->item, "secure"))
			nickserv.opts |= NS_SMAIL;
		else if (!strcmp(config->seccion[i]->item, "prot"))
		{
			nickserv.opts &= ~(NS_PROT_KILL | NS_PROT_CHG);
			if (!strcasecmp(config->seccion[i]->data, "KILL"))
				nickserv.opts |= NS_PROT_KILL;
			else
				nickserv.opts |= NS_PROT_CHG;
		}
#ifdef UDB
		else if (!strcmp(config->seccion[i]->item, "automigrar"))
			nickserv.opts |= NS_AUTOMIGRAR;
#endif
		else if (!strcmp(config->seccion[i]->item, "recovernick"))
			ircstrdup(nickserv.recovernick, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "securepass"))
			ircstrdup(nickserv.securepass, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "min_reg"))
			nickserv.min_reg = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "autodrop"))
			nickserv.autodrop = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "nicks"))
			nickserv.nicks = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "intentos"))
			nickserv.intentos = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "maxlist"))
			nickserv.maxlist = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "forbmails"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++, nickserv.forbmails++)
				ircstrdup(nickserv.forbmail[nickserv.forbmails], config->seccion[i]->seccion[p]->item);
		}
		else if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ns = &nickserv_coms[0];
				while (ns->com != 0x0)
				{
					if (!strcasecmp(ns->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = ns;
						break;
					}
					ns++;
				}
				if (ns->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
			mod->comando[mod->comandos] = NULL;
		}
	}
	inserta_senyal(SIGN_PRE_NICK, nickserv_pre_nick);
	inserta_senyal(SIGN_POST_NICK, nickserv_post_nick);
	inserta_senyal(SIGN_UMODE, nickserv_umode);
	inserta_senyal(SIGN_MYSQL, nickserv_sig_mysql);
	inserta_senyal(SIGN_QUIT, nickserv_sig_quit);
#ifdef UDB
	inserta_senyal(SIGN_EOS, nickserv_sig_eos);
#endif
	inserta_senyal(NS_SIGN_IDOK, nickserv_sig_idok);
	proc(nickserv_dropanicks);
	bot_set(nickserv);
}
void envia_clave(char *nick)
{
	char *pass, *passmd5;
	pass = random_ex(nickserv.securepass);
	passmd5 = MDString(pass);
	_mysql_add(NS_MYSQL, nick, "pass", passmd5);
	email(_mysql_get_registro(NS_MYSQL, nick, "email"), "Reenvío de la contraseña", "Debido a la pérdida de tu contraseña, se te ha generado otra clave totalmente segura.\r\n"
		"A partir de ahora, la clave de tu nick es:\r\n\r\n%s\r\n\r\nPuedes cambiarla mediante el comando SET de %s.\r\n\r\nGracias por utilizar los servicios de %s.", pass, nickserv.hmod->nick, conf_set->red);
#ifdef UDB
	if (IsNickUDB(nick))
	{
		envia_registro_bdd("N::%s::pass %s", nick, passmd5);
		envia_registro_bdd("N::%s::desafio md5", nick);
	}
#endif
}
BOTFUNC(nickserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(nickserv), "\00312%s\003 se encarga de gestionar los nicks de la red para evitar robos de identidad.", nickserv.hmod->nick);
		response(cl, CLI(nickserv), "El registro de tu nick permite que otros no lo utilicen, mostrarte como propietario y aventajarte de cuantiosas oportunidades.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Comandos disponibles:");
		response(cl, CLI(nickserv), "\00312REGISTER\003 Registra tu nick.");
		response(cl, CLI(nickserv), "\00312IDENTIFY\003 Te identifica como propietario del nick.");
		response(cl, CLI(nickserv), "\00312INFO\003 Muestra información de un nick.");
		response(cl, CLI(nickserv), "\00312LIST\003 Lista todos los nicks que coinciden con un patrón.");
		response(cl, CLI(nickserv), "\00312GHOST\003 Desconecta a un usuario que utilice su nick.");
		if (IsId(cl))
		{
			func_resp(nickserv, "SET", "Permite fijar distintas opciones para tu nick.");
#ifdef UDB
			func_resp(nickserv, "MIGRAR", "Migra tu nick a la BDD.");
			func_resp(nickserv, "DEMIGRAR", "Demigra tu nick de la BDD.");
#endif			
		}
		if (IsPreo(cl))
		{
			func_resp(nickserv, "DROP", "Desregistra un usuario.");
			func_resp(nickserv, "SENDPASS", "Genera un password para el usuario y se lo envía a su correo.");
			func_resp(nickserv, "SWHOIS", "Añade o borra un whois especial al nick especificado.");
		}
		if (IsOper(cl))
		{
			func_resp(nickserv, "SUSPENDER", "Suspende los privilegios de un nick.");
			func_resp(nickserv, "LIBERAR", "Libera un nick de su suspenso.");
			func_resp(nickserv, "RENAME" ,"Cambia el nick a un usuario.");
		}
		if (IsAdmin(cl))
			func_resp(nickserv, "FORBID", "Prohibe o permite un determinado nick.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Para más información, \00312/msg %s %s comando", nickserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "REGISTER") && exfunc("REGISTER"))
	{
		response(cl, CLI(nickserv), "\00312REGISTER");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Permite registrar tu nick en la red para que nadie sin autorización pueda utilizarlo.");
		response(cl, CLI(nickserv), "Para utilizar tu nick registrado deberás identificarte como propietario mediante el comando \00312IDENTIFY\003.");
		response(cl, CLI(nickserv), "El registro del nick requiere una cuenta de email válida. Recuerda que si no lo es, no podrás recibir tu nueva contraseña en caso de que la perdieras.");
		if (nickserv.opts & NS_SMAIL)
			response(cl, CLI(nickserv), "Una vez hayas registrado tu nick, te llegará en tu correo email una clave temporal, que será la que deberás usar para identificarte.");
		response(cl, CLI(nickserv), " ");
		if (nickserv.opts & NS_SMAIL)
			response(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tu@email");
		else
			response(cl, CLI(nickserv), "Sintaxis: \00312REGISTER tupass tu@email");
	}
	else if (!strcasecmp(param[1], "IDENTIFY") && exfunc("IDENTIFY"))
	{
		response(cl, CLI(nickserv), "\00312IDENTIFY");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Te identifica como propietario del nick.");
		response(cl, CLI(nickserv), "Para ello debes proporcionar una contraseña de autentificación, la misma que utilizaste en el registro de tu nick.");
		response(cl, CLI(nickserv), "Una vez identificado como propietario podrás disponer de cuantiosas oportunidades que te ofrece el reigstro de nicks.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312IDENTIFY tupass");
	}
	else if (!strcasecmp(param[1], "INFO") && exfunc("INFO"))
	{
		response(cl, CLI(nickserv), "\00312INFO");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Muestra distinta información de un nick, como su fecha de registro o su último quit, entre otros.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312INFO nick");
	}
	else if (!strcasecmp(param[1], "LIST") && exfunc("LIST"))
	{
		response(cl, CLI(nickserv), "\00312LIST");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Lista todos los nicks registrados que coincidan con un patrón especificado.");
		response(cl, CLI(nickserv), "Para evitar abusos de flood, este comando emite como máximo \00312%i\003 entradas.", nickserv.maxlist);
		response(cl, CLI(nickserv), "El patrón puede ser exacto, o puede contener comodines (*) para cerrar el campo de búsqueda.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312LIST patron");
	}
	else if (!strcasecmp(param[1], "GHOST") && exfunc("GHOST"))
	{
		response(cl, CLI(nickserv), "\00312GHOST");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Este comando desconecta a un usuario que lleve puesto tu nick.");
		response(cl, CLI(nickserv), "Para ello deberás especificar la contraseña de propietario del nick.");
		response(cl, CLI(nickserv), "Si la contraseña es válida, %s.", nickserv.opts & NS_PROT_KILL ? "se desconectará el usuario" : "se le cambiará el nick al usuario");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312GHOST nick contraseña");
	}
	else if (!strcasecmp(param[1], "SET") && IsId(cl) && exfunc("SET"))
	{
		if (params < 3)
		{
			response(cl, CLI(nickserv), "\00312SET");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Permite fijar diferentes opciones para tu nick.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Opciones disponibles:");
			response(cl, CLI(nickserv), "\00312PASS\003 Cambia tu contraseña de usuario.");
			response(cl, CLI(nickserv), "\00312EMAIL\003 Cambia tu dirección de correo.");
			response(cl, CLI(nickserv), "\00312URL\003 Cambia tu página web.");
			response(cl, CLI(nickserv), "\00312KILL\003 Activa/Desactiva tu protección de KILL.");
			if (IsOper(cl))
				response(cl, CLI(nickserv), "\00312NODROP\003 Activa/Desactiva tu protección de DROP.");
			response(cl, CLI(nickserv), "\00312HIDE\003 Opciones para mostrar en tu INFO.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Para más información, \00312/msg %s %s SET opción", nickserv.hmod->nick, strtoupper(param[0]));
		}
		else if (!strcasecmp(param[2], "PASS"))
		{
			response(cl, CLI(nickserv), "\00312SET PASS");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Cambia tu contraseña de usuario.");
			response(cl, CLI(nickserv), "A partir de este cambio, ésta será la contraseña con la que debes identificarte.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312SET PASS nuevopass");
		}
		else if (!strcasecmp(param[2], "EMAIL"))
		{
			response(cl, CLI(nickserv), "\00312SET EMAIL");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Cambia tu dirección de correo.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312SET email direccion@correo");
		}
		else if (!strcasecmp(param[2], "URL"))
		{
			response(cl, CLI(nickserv), "\00312SET URL");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Cambia tu página web personal.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312SET url http://pagina.web");
		}
		else if (!strcasecmp(param[2], "KILL"))
		{
			response(cl, CLI(nickserv), "\00312SET KILL");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Especifica tu protección KILL.");
			response(cl, CLI(nickserv), "Con esta protección, si no te identificas antes de los segundos que especifiques %s.", nickserv.opts & NS_PROT_KILL ? "serás desconectado" : "se te cambiará el nick");
			response(cl, CLI(nickserv), "Si especificas el 0 como valor, esta protección se desactiva.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312KILL segundos");
		}
		else if (!strcasecmp(param[2], "NODROP") && IsOper(cl))
		{
			response(cl, CLI(nickserv), "\00312SET NODROP");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Evita que tu nick pueda ser dropeado, a excepción de los Administradores.");
			response(cl, CLI(nickserv), "Si está en ON, tu nick no podrá dropearse.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312SET NODROP on|off");
		}
		else if (!strcasecmp(param[2], "HIDE"))
		{
			response(cl, CLI(nickserv), "\00312SET HIDE");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Permite mostrar u ocultar distintas opciones de tu INFO.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Opciones para mostrar:");
			response(cl, CLI(nickserv), "\00312EMAIL\003 Muestra tu email.");
			response(cl, CLI(nickserv), "\00312URL\003 Muestra tu página personal.");
			response(cl, CLI(nickserv), "\00312MASK\003 Muestra tu user@host.");
			response(cl, CLI(nickserv), "\00312TIME\003 Muestra la última hora que fuiste visto en la red.");
			response(cl, CLI(nickserv), "\00312QUIT\003 Muestra tu último quit.");
			response(cl, CLI(nickserv), "\00312LIST\003 Se te mostrará cuando alguien haga un LIST.");
			response(cl, CLI(nickserv), " ");
			response(cl, CLI(nickserv), "Sintaxis: \00312SET HIDE opcion on|off");
		}
		else
			response(cl, CLI(nickserv), NS_ERR_EMPT, "Opción desconocida.");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "MIGRAR") && IsId(cl) && exfunc("MIGRAR"))
	{
		response(cl, CLI(nickserv), "\00312MIGRAR");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Migra tu nick a la base de datos.");
		response(cl, CLI(nickserv), "Una vez migrado tu nick, la única forma de ponérselo sólo podrá ser vía /nick tunick:tupass.");
		response(cl, CLI(nickserv), "Si la contraseña es correcta, recibirás el modo de usuario +r quedando identificado automáticamente.");
		response(cl, CLI(nickserv), "De lo contrario, no podrás utilizar este nick.");
		response(cl, CLI(nickserv), "Adicionalmente, puedes utilizar /nick tunick!tupass para desconectar una sesión que pudiere utilizar tu nick.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312MIGRAR tupass");
	}
	else if (!strcasecmp(param[1], "DEMIGRAR") && IsId(cl) && exfunc("DEMIGRAR"))
	{
		response(cl, CLI(nickserv), "\00312DEMIGRAR");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Borra tu nick de la BDD.");
		response(cl, CLI(nickserv), "Una vez borrado, la identificación procederá con el comando IDENTIFY.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312DEMIGRAR tupass");
	}
#endif
	else if (!strcasecmp(param[1], "DROP") && IsPreo(cl) && exfunc("DROP"))
	{
		response(cl, CLI(nickserv), "\00312DROP");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Borra un nick de la base de datos.");
		response(cl, CLI(nickserv), "Este desregistro anula completamente la propiedad de un usuario sobre un nick.");
		response(cl, CLI(nickserv), "Una vez borrado, no se puede volver a recuperar si no es registrándolo de nuevo.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312DROP nick");
	}
	else if (!strcasecmp(param[1], "SENDPASS") && IsPreo(cl) && exfunc("SENDPASS"))
	{
		response(cl, CLI(nickserv), "\00312SENDPASS");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Regenera una contraseña de usuario y se la envía a su correo.");
		response(cl, CLI(nickserv), "Este comando sólo debe en caso de pérdida de la contraseña del usuario y a petición del mismo.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312SENDPASS nick");
	}
	else if (!strcasecmp(param[1], "SWHOIS") && IsPreo(cl) && exfunc("SWHOIS"))
	{
		response(cl, CLI(nickserv), "\00312SWHOIS");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Añade o borra un whois especial a un usuario.");
		response(cl, CLI(nickserv), "Este whois se le añadirá cuando se identifique.");
		response(cl, CLI(nickserv), "Se mostrará con el comando WHOIS.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312SWHOIS nick [swhois]");
		response(cl, CLI(nickserv), "Si no se especifica swhois, se borrará el que pudiera haber.");
	}
	else if (!strcasecmp(param[1], "SUSPENDER") && IsOper(cl) && exfunc("SUSPENDER"))
	{
		response(cl, CLI(nickserv), "\00312SUSPENDER");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Suspende el uso de un determinado nick.");
		response(cl, CLI(nickserv), "Este nick podrá ponerselo pero no tendrá privilegios algunos.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312SUSPENDER nick motivo");
	}
	else if (!strcasecmp(param[1], "LIBERAR") && IsOper(cl) && exfunc("LIBERAR"))
	{
		response(cl, CLI(nickserv), "\00312LIBERAR");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Libera un nick de su suspenso.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312LIBERAR nick");
	}
	else if (!strcasecmp(param[1], "RENAME") && IsOper(cl) && exfunc("RENAME"))
	{
		response(cl, CLI(nickserv), "\00312RENAME");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Cambia el nick a un usuario conectado.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312RENAME nick nuevonick");
	}
	else if (!strcasecmp(param[1], "FORBID") && IsAdmin(cl) && exfunc("FOBRID"))
	{
		response(cl, CLI(nickserv), "\00312FORBID");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Prohibe o permite el uso de un nick o apodo.");
		response(cl, CLI(nickserv), " ");
		response(cl, CLI(nickserv), "Sintaxis: \00312FORBID {+|-}nick [motivo]");
	}
	else
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(nickserv_reg)
{
	u_int i, opts;
	u_long ultimo;
	char *dominio;
	char *mail, *pass, *usermask, *cache;
	MYSQL_RES *res = NULL;
	usermask = strchr(cl->mask, '!') + 1;
	if (params < ((nickserv.opts & NS_SMAIL) ? 2 : 3))
	{
		if (nickserv.opts & NS_SMAIL)
			response(cl, CLI(nickserv), NS_ERR_PARA, "REGISTER tu@email");
		else
			response(cl, CLI(nickserv), NS_ERR_PARA, "REGISTER tupass tu@email");
		return 1;
	}
	if ((cache = coge_cache(CACHE_ULTIMO_REG, usermask, nickserv.hmod->id)))
	{
		ultimo = atoul(cache);
		if ((ultimo + (3600 * nickserv.min_reg)) > (u_long)time(0))
		{
			char buf[512];
			sprintf_irc(buf, "No puedes efectuar otro registro hasta que no pasen %i horas.", nickserv.min_reg);
			response(cl, CLI(nickserv), NS_ERR_EMPT, buf);
			return 1;
		}
		else
			borra_cache(CACHE_ULTIMO_REG, usermask, nickserv.hmod->id);
	}
	/* comprobamos que no esté registrado */
	if (IsReg(cl->nombre))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick ya está registrado.");
		return 1;
	}
	if (nickserv.opts & NS_SMAIL)
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
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No parece ser una cuenta de email válida.");
		return 1;
	}
	for (i = 0; i < nickserv.forbmails; i++)
	{
		if (!strcasecmp(nickserv.forbmail[i], dominio))
		{
			response(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes utilizar una cuenta de correo gratuíta.");
			return 1;
		}
	}
	if (nickserv.nicks && ((res = _mysql_query("SELECT * from %s%s where email='%s'", PREFIJO, NS_MYSQL, mail)) && mysql_num_rows(res) == nickserv.nicks))
	{
		mysql_free_result(res);
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes registrar más nicks en esta cuenta.");
		return 1;
	}
	mysql_free_result(res);
	opts = NS_OPT_MASK;
#ifdef UDB
	if (nickserv.opts & NS_AUTOMIGRAR)
		opts |= NS_OPT_UDB;
#endif
	if (pass)
		_mysql_add(NS_MYSQL, parv[0], "pass", MDString(pass));
	_mysql_add(NS_MYSQL, parv[0], "email", mail);
	_mysql_add(NS_MYSQL, parv[0], "gecos", cl->info);
	_mysql_add(NS_MYSQL, parv[0], "host", "%s@%s", cl->ident, cl->host);
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	_mysql_add(NS_MYSQL, parv[0], "id", "%i", nickserv.opts & NS_SMAIL ? 0 : time(0));
	_mysql_add(NS_MYSQL, parv[0], "reg", "%i", time(0));
	_mysql_add(NS_MYSQL, parv[0], "last", "%i", time(0));
	if (nickserv.opts & NS_SMAIL)
		envia_clave(parv[0]);
	else if (UMODE_REGNICK)
		port_func(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", UMODEF_REGNICK);
	inserta_cache(CACHE_ULTIMO_REG, usermask, 3600 * nickserv.min_reg, nickserv.hmod->id, "%lu", time(0));
	response(cl, CLI(nickserv), "Tu nick ha sido registrado bajo la cuenta \00312%s\003.", mail);
#ifdef UDB
	if (nickserv.opts & NS_AUTOMIGRAR)
	{
		if (pass)
		{
			envia_registro_bdd("N::%s::pass %s", cl->nombre, MDString(pass));
			envia_registro_bdd("N::%s::desafio md5", cl->nombre);
		}
		cambia_nick_inv(cl);
	}
#endif
	senyal1(NS_SIGN_REG, parv[0]);
	return 0;
}
BOTFUNC(nickserv_identify)
{
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "IDENTIFY tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No tienes el nick registrado.");
		return 1;
	}
	if (IsSusp(parv[0])
#ifdef UDB
	 || (cl->modos & UMODE_SUSPEND)
#endif
	 )
	{
		sprintf_irc(buf, "Tienes el nick suspendido: %s", _mysql_get_registro(NS_MYSQL, parv[0], "suspend"));
		response(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if ((nickserv.opts & NS_SID) && !parv[parc])
	{
		sprintf_irc(buf, "Identificación incorrecta. /msg %s@%s IDENTIFY pass", nickserv.hmod->nick, me.nombre);
		response(cl, CLI(nickserv), NS_ERR_EMPT, buf);
		return 1;
	}
	if (!strcmp(MDString(param[1]), _mysql_get_registro(NS_MYSQL, parv[0], "pass")))
	{
		if (UMODE_REGNICK)
			port_func(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), "+%c", UMODEF_REGNICK);
		response(cl, CLI(nickserv), "Ok \00312%s\003, bienvenid@ a casa :)", parv[0]);
		senyal1(NS_SIGN_IDOK, cl);
	}
	else
	{
		int intentos = 0;
		char *cache;
		if ((cache = coge_cache(CACHE_INTENTOS_ID, cl->nombre, nickserv.hmod->id)))
			intentos = atoi(cache);
		if (++intentos == nickserv.intentos)
		{
			port_func(P_QUIT_USUARIO_REMOTO)(cl, CLI(nickserv), "Demasiadas contraseñas inválidas.");
			borra_cache(CACHE_INTENTOS_ID, cl->nombre, nickserv.hmod->id);
		}
		else
		{
			sprintf_irc(buf, "Contraseña incorrecta. Tienes %i intentos más.", nickserv.intentos - intentos);
			response(cl, CLI(nickserv), buf);
			inserta_cache(CACHE_INTENTOS_ID, cl->nombre, 86400, nickserv.hmod->id, "%i", intentos);
		}
	}
	return 0;
}
BOTFUNC(nickserv_set)
{
	if (params < 3)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "SET opción valor");
		return 1;
	}
	if (!strcasecmp(param[1], "EMAIL"))
	{
		_mysql_add(NS_MYSQL, parv[0], "email", param[2]);
		response(cl, CLI(nickserv), "Tu email se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "URL"))
	{
		_mysql_add(NS_MYSQL, parv[0], "url", param[2]);
		response(cl, CLI(nickserv), "Tu url se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "KILL"))
	{
		if (atoi(param[2]) < 10 && strcasecmp(param[2], "OFF"))
		{
			response(cl, CLI(nickserv), NS_ERR_SNTX, "El valor kill debe ser un número mayor que 10.");
			return 1;
		}
		_mysql_add(NS_MYSQL, parv[0], "kill", param[2]);
		response(cl, CLI(nickserv), "Tu kill se ha cambiado a \00312%s\003.", param[2]);
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
			response(cl, CLI(nickserv), "El DROP de tu nick se ha cambiado a \00312%s\003.", param[2]);
		}
		else
		{
			response(cl, CLI(nickserv), NS_ERR_FORB);
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
		response(cl, CLI(nickserv), "Tu contraseña se ha cambiado a \00312%s\003.", param[2]);
		return 0;
	}
	else if (!strcasecmp(param[1], "HIDE"))
	{
		int opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
		if (params < 4)
		{
			response(cl, CLI(nickserv), NS_ERR_PARA, "SET HIDE valor on|off");
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
			response(cl, CLI(nickserv), NS_ERR_EMPT, "Opción incorrecta.");
			return 1;
		}
		_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
		response(cl, CLI(nickserv), "Hide %s cambiado a \00312%s\003.", param[2], param[3]);
		return 0;
	}
	else
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Opción incorrecta.");
		return 1;
	}
	return 0;
}
BOTFUNC(nickserv_drop)
{
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "DROP nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!IsAdmin(cl) && (atoi(_mysql_get_registro(NS_MYSQL, param[1], "opts")) & NS_OPT_NODROP))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no se puede dropear.");
		return 1;
	}
	if (!nickserv_baja(param[1], 1))
		response(cl, CLI(nickserv), "Se ha dropeado el nick \00312%s\003.", param[1]);
	else
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No se ha podido dropear. Comuníquelo a la administración.");
	return 0;
}
int nickserv_baja(char *nick, char opt)
{
	Cliente *al;
	int opts = 0;
	if (!opt && (opts = atoi(_mysql_get_registro(NS_MYSQL, nick, "opts"))) & NS_OPT_NODROP)
		return 1;
	al = busca_cliente(nick, NULL);
	senyal1(NS_SIGN_DROP, nick);
	if (al)
		port_func(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "-r");
#ifdef UDB
	if (opts & NS_OPT_UDB)
		envia_registro_bdd("N::%s", nick);
#endif
	_mysql_del(NS_MYSQL, nick);
	return 0;
}
BOTFUNC(nickserv_sendpass)
{
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "SENDPASS nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	envia_clave(param[1]);
	response(cl, CLI(nickserv), "Se ha generado y enviado otra contraseña al email de \00312%s\003.", param[1]);
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
		response(cl, CLI(nickserv), NS_ERR_PARA, "INFO nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	comp = strcasecmp(param[1], parv[0]);
	res = _mysql_query("SELECT opts,gecos,reg,suspend,host,quit,last,email,url,`kill`,item from %s%s where item='%s'", PREFIJO, NS_MYSQL, param[1]);
	row = mysql_fetch_row(res);
	opts = atoi(row[0]);
	response(cl, CLI(nickserv), "Información de \00312%s", row[10]);
	response(cl, CLI(nickserv), "Realname: \00312%s", row[1]);
	reg = (time_t)atol(row[2]);
	response(cl, CLI(nickserv), "Registrado el \00312%s", _asctime(&reg));
	if (!BadPtr(row[3]))
		response(cl, CLI(nickserv), "Está suspendido: \00312%s", row[3]);
	if (!(opts & NS_OPT_MASK) || (!comp && IsId(cl)) || IsOper(cl))
		response(cl, CLI(nickserv), "Último mask: \00312%s", row[4]);
	if (!(opts & NS_OPT_TIME) || (!comp && IsId(cl)) || IsOper(cl))
	{
		Cliente *al;
		al = busca_cliente(param[1], NULL);
		if (al && IsId(al))
			response(cl, CLI(nickserv), "Este usuario está conectado. Utiliza /WHOIS %s para más información.", param[1]);
		else
		{
			reg = (time_t)atol(row[6]);
			response(cl, CLI(nickserv), "Visto por última vez el \00312%s", _asctime(&reg));
		}
	}
	if (!BadPtr(row[5]) && (!(opts & NS_OPT_QUIT) || (!comp && IsId(cl)) || IsOper(cl)))
		response(cl, CLI(nickserv), "Último quit: \00312%s", row[5]);
	if (!(opts & NS_OPT_MAIL) || (!comp && IsId(cl)) || IsOper(cl))
		response(cl, CLI(nickserv), "Email: \00312%s", row[7]);
	if (!BadPtr(row[8]) && (!(opts & NS_OPT_WEB) || (!comp && IsId(cl)) || IsOper(cl)))
		response(cl, CLI(nickserv), "Url: \00312%s", row[8]);
	if (atoi(row[9]))
		response(cl, CLI(nickserv), "Protección kill a \00312%i\003 segundos.", atoi(row[9]));
#ifdef UDB
	if (opts & NS_OPT_UDB)
		response(cl, CLI(nickserv), "Este nick está migrado a la \2BDD");
#endif
	mysql_free_result(res);
	if ((!strcasecmp(parv[0], param[1]) && IsId(cl)) || IsOper(cl))
	{
		CsRegistros *regs;
		int i;
		if ((regs = busca_cregistro_dl(param[1])))
		{
			response(cl, CLI(nickserv), "*** Niveles de acceso ***");
			for (i = 0; i < regs->subs; i++)
				response(cl, CLI(nickserv), "Canal: \00312%s \003flags: \00312+%s\003 (\00312%lu\003)", regs->sub[i].canal, 
					modes2flags(regs->sub[i].flags, cFlags_dl, NULL),
					regs->sub[i].flags);
		}
		if ((res = _mysql_query("SELECT item from %s%s where founder='%s'", PREFIJO, CS_MYSQL, param[1])))
		{
			while ((row = mysql_fetch_row(res)))
				response(cl, CLI(nickserv), "Canal: \00312%s flags: \00312+%s\003 (\00312FUNDADOR\003)", row[0], 
					modes2flags(CS_LEV_ALL, cFlags_dl, NULL));
			mysql_free_result(res);
		}
	}
	return 0;
}
BOTFUNC(nickserv_list)
{
	char *rep;
	MYSQL_RES *res;
	MYSQL_ROW row;
	u_int i;
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "LIST patrón");
		return 1;
	}
	rep = str_replace(param[1], '*', '%');
	if (!(res = _mysql_query("SELECT item from %s%s where item LIKE '%s'", PREFIJO, NS_MYSQL, rep)))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No se han encontrado coincidencias.");
		return 1;
	}
	response(cl, CLI(nickserv), "*** Nicks que coinciden con el patrón \00312%s\003 ***", param[1]);
	for (i = 0; i < nickserv.maxlist && (row = mysql_fetch_row(res)); i++)
	{
		if (IsOper(cl) || !(atoi(_mysql_get_registro(NS_MYSQL, row[0], "opts")) & NS_OPT_LIST))
			response(cl, CLI(nickserv), "\00312%s", row[0]);
		else
			i--;
	}
	response(cl, CLI(nickserv), "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
	mysql_free_result(res);
	return 0;
}
BOTFUNC(nickserv_ghost)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "GHOST nick pass");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	if (!strcasecmp(param[1], parv[0]))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "No puedes ejecutar este comando sobre ti mismo.");
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, param[1], "pass"), MDString(param[2])))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	if (nickserv.opts & NS_PROT_KILL)
		port_func(P_QUIT_USUARIO_REMOTO)(al, CLI(nickserv), "Comando GHOST utilizado por %s.", parv[0]);
	else
		cambia_nick_inv(al);
	response(cl, CLI(nickserv), "Nick \00312%s\003 liberado.", param[1]);
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
		response(cl, CLI(nickserv), NS_ERR_PARA, "SUSPENDER nick motivo");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	motivo = implode(param, params, 2, -1);
	_mysql_add(NS_MYSQL, param[1], "suspend", motivo);
#ifdef UDB
	if ((al = busca_cliente(param[1], NULL)))
	{
		if (UMODE_SUSPEND || UMODE_REGNICK)
			port_func(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "+%c-%c", UMODEF_SUSPEND, UMODEF_REGNICK);
		response(al, CLI(nickserv), "Tu nick ha sido suspendido por \00312%s\003: %s", cl->nombre, motivo);
	}
	if (IsNickUDB(param[1]))
		envia_registro_bdd("N::%s::suspendido %s", param[1], motivo);
#endif
	response(cl, CLI(nickserv), "El nick \00312%s\003 ha sido suspendido.", param[1]);
	return 0;
}
BOTFUNC(nickserv_liberar)
{
#ifdef UDB 
	Cliente *al;
#endif
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "LIBERAR nick");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (!_mysql_get_registro(NS_MYSQL, param[1], "suspend"))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no está suspendido.");
		return 1;
	}
	_mysql_add(NS_MYSQL, param[1], "suspend", "");
#ifdef UDB
	if ((al = busca_cliente(param[1], NULL)))
		port_func(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "-S");
	if (IsNickUDB(param[1]))
		envia_registro_bdd("N::%s::suspendido", param[1]);
#endif
	response(cl, CLI(nickserv), "El nick \00312%s\003 ha sido liberado de su suspenso.", param[1]);			
	return 0;
}
BOTFUNC(nickserv_swhois)
{
	if (!port_existe(P_WHOIS_ESPECIAL))
	{
		response(cl, CLI(nickserv), ERR_NSUP);
		return 1;
	}
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "SWHOIS nick [swhois]");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (params == 3)
	{
		char *swhois;
		Cliente *al;
		swhois = implode(param, params, 2, -1);
#ifdef UDB
		if (IsNickUDB(param[1]))
			envia_registro_bdd("N::%s::swhois %s", param[1], swhois);
		else
#endif
		_mysql_add(NS_MYSQL, param[1], "swhois", swhois);
		if ((al = (busca_cliente(param[1], NULL))))
			port_func(P_WHOIS_ESPECIAL)(al, swhois);
		response(cl, CLI(nickserv), "El swhois de \00312%s\003 ha sido cambiado.", param[1]);
	}
	else
	{
#ifdef UDB
		if (IsNickUDB(param[1]))
			envia_registro_bdd("N::%s::swhois", param[1]);
		else
#endif		
		_mysql_add(NS_MYSQL, param[1], "swhois", NULL);
		response(cl, CLI(nickserv), "Se ha eliminado el swhois de \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(nickserv_rename)
{
	Cliente *al;
	if (!port_existe(P_CAMBIO_USUARIO_REMOTO))
	{
		response(cl, CLI(nickserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "RENAME nick nuevonick");
		return 1;
	}
	if ((al = busca_cliente(param[2], NULL)))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "El nuevo nick ya está en uso.");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Este nick no está conectado.");
		return 1;
	}
	port_func(P_CAMBIO_USUARIO_REMOTO)(al, param[2]);
	response(cl, CLI(nickserv), "El nick \00312%s\003 ha cambiado su nick a \00312%s\003.", param[1], param[2]);
	return 0;
}
BOTFUNC(nickserv_forbid)
{
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "FORBID {+|-}nick [motivo]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *motivo;
		if (params < 3)
		{
			response(cl, CLI(nickserv), NS_ERR_PARA, "FORBID +nick motivo");
			return 1;
		}
		motivo = implode(param, params, 1, -1);
#ifdef UDB
		envia_registro_bdd("N::%s::forbid %s", param[1] + 1, motivo);
#else
		if (!port_existe(P_FORB_NICK))
		{
			response(cl, CLI(nickserv), ERR_NSUP);
			return 1;
		}
		_mysql_add(NS_FORBIDS, param[1] + 1, "motivo", motivo);
		port_func(P_FORB_NICK)(param[1] + 1, ADD,  motivo);
#endif
		response(cl, CLI(nickserv), "El nick \00312%s\003 ha sido prohibido.", param[1] + 1);
	}
	else if (*param[1] == '-')
	{
#ifdef UDB
		envia_registro_bdd("N::%s::forbid", param[1] + 1);
#else
		_mysql_del(NS_FORBIDS, param[1] + 1);
		if (!port_existe(P_FORB_NICK))
			port_func(P_FORB_NICK)(param[1] + 1, DEL, NULL);
#endif
		response(cl, CLI(nickserv), "El nick \00312%s\003 ha sido permitido.", param[1] + 1);
	}
	else
	{
		response(cl, CLI(nickserv), NS_ERR_SNTX, "FORBID {+|-}nick [motivo]");
		return 1;
	}
	return 0;
}
#ifdef UDB
BOTFUNC(nickserv_migrar)
{
	int opts;
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "MIGRAR tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, parv[0], "pass"), MDString(param[1])))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
	if (opts & NS_OPT_UDB)
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Ya tienes el nick migrado.");
		return 1;
	}
	envia_registro_bdd("N::%s::pass %s", parv[0], MDString(param[1]));
	envia_registro_bdd("N::%s::desafio md5", parv[0]);
	response(cl, CLI(nickserv), "Migración realizada.");
	opts |= NS_OPT_UDB;
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	cambia_nick_inv(cl);
	return 0;
}
BOTFUNC(nickserv_demigrar)
{
	int opts;
	if (params < 2)
	{
		response(cl, CLI(nickserv), NS_ERR_PARA, "DEMIGRAR tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		response(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(_mysql_get_registro(NS_MYSQL, parv[0], "pass"), MDString(param[1])))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(_mysql_get_registro(NS_MYSQL, parv[0], "opts"));
	if (!(opts & NS_OPT_UDB))
	{
		response(cl, CLI(nickserv), NS_ERR_EMPT, "Tu nick no está migrado.");
		return 1;
	}
	envia_registro_bdd("N::%s", parv[0]);
	response(cl, CLI(nickserv), "Demigración realizada.");
	opts &= ~NS_OPT_UDB;
	_mysql_add(NS_MYSQL, parv[0], "opts", "%i", opts);
	return 0;
}
#endif
int nickserv_umode(Cliente *cl, char *modos)
{
	//if (conf_set->modos && conf_set->modos->usuarios)
	//	port_func(P_MODO_USUARIO_REMOTO)(cl, CLI(nickserv), conf_set->modos->usuarios);
	if (strchr(modos, 'r') && (cl->modos & UMODE_REGNICK))
		senyal1(NS_SIGN_IDOK, cl);
	return 0;
}
/* si nuevo es NULL, es un cliente nuevo. si no, es un cambio de nick */
int nickserv_pre_nick(Cliente *cl, char *nuevo)
{
	if (nuevo)
	{
		if (IsReg(cl->nombre))
		{
			if (IsId(cl))
				_mysql_add(NS_MYSQL, cl->nombre, "last", "%i", time(0));
			else
				timer_off(cl->nombre, cl->sck); /* es posible que tenga kill */
		}
	}
	return 0;
}
int nickserv_post_nick(Cliente *cl)
{
#ifndef UDB
	char *motivo = NULL;
	if ((motivo = _mysql_get_registro(NS_FORBIDS, cl->nombre, "motivo")))
	{
		response(cl, CLI(nickserv), "Este nick está prohibido: %s", motivo);
		cambia_nick_inv(cl);
		return 0;
	}
#endif
	if (!IsId(cl) && IsReg(cl->nombre))
	{
		char *kill;
		if (nickserv.opts & NS_SID)
			sprintf_irc(buf, "%s@%s", nickserv.hmod->nick, me.nombre);
		else
			sprintf_irc(buf, "%s", nickserv.hmod->nick);
		response(cl, CLI(nickserv), "Este nick está protegido. Si es tu nick escribe \00312/msg %s IDENTIFY tupass\003. Si no lo es, escoge otro.", buf);
		kill = _mysql_get_registro(NS_MYSQL, cl->nombre, "kill");
		if (kill && atoi(kill))
		{
			response(cl, CLI(nickserv), "De no hacerlo, serás desconectado en \00312%s\003 segundos.", kill);
			timer(cl->nombre, cl->sck, 1, atoi(kill), nickserv_killid, cl->nombre, sizeof(char) * (strlen(cl->nombre) + 1));
		}
	}
	return 0;
}
int nickserv_sig_mysql()
{
	if (!_mysql_existe_tabla(NS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
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
			") TYPE=MyISAM COMMENT='Tabla de nicks';", PREFIJO, NS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, NS_MYSQL);
	}
#ifndef UDB
	if (!_mysql_existe_tabla(NS_FORBIDS))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`motivo` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de nicks prohibidos';", PREFIJO, NS_FORBIDS))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, NS_FORBIDS);
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
		if (
#ifdef UDB
		!IsNickUDB(cl->nombre) && 
#endif
		(swhois = _mysql_get_registro(NS_MYSQL, cl->nombre, "swhois")))
			port_func(P_WHOIS_ESPECIAL)(cl, swhois);
		cl->nivel |= USER;
	}
	return 0;
}
int nickserv_killid(char *quien)
{
	Cliente *al;
	if ((al = busca_cliente(quien, NULL)) && !IsId(al))
	{
		if (nickserv.opts & NS_PROT_KILL)
			port_func(P_QUIT_USUARIO_REMOTO)(al, CLI(nickserv), "Protección de NICK.");
		else
			cambia_nick_inv(al);
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
			else if (nickserv.autodrop && (atoi(row[2]) + 86400 * nickserv.autodrop < (u_long)time(0)))
				nickserv_baja(row[0], 0);
		}
		proc->proc += 30;
		mysql_free_result(res);
	}
	return 0;
}
void cambia_nick_inv(Cliente *cl)
{
	char buf[BUFSIZE];
	if (!port_existe(P_CAMBIO_USUARIO_REMOTO))
		return;
	sprintf_irc(buf, nickserv.recovernick, cl->nombre);
	port_func(P_CAMBIO_USUARIO_REMOTO)(cl, random_ex(buf));
}
#ifdef UDB
int nickserv_sig_eos()
{
	envia_registro_bdd("S::NickServ %s", nickserv.hmod->mascara);
	return 0;
}
#endif

