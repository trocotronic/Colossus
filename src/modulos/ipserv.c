/*
 * $Id: ipserv.c,v 1.3 2004-09-14 04:25:24 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "ipserv.h"
#include "nickserv.h"

IpServ *ipserv = NULL;

static int ipserv_help		(Cliente *, char *[], int, char *[], int);
static int ipserv_setipv	(Cliente *, char *[], int, char *[], int);
static int ipserv_setvhost	(Cliente *, char *[], int, char *[], int);
static int ipserv_temphost	(Cliente *, char *[], int, char *[], int);

DLLFUNC int ipserv_sig_mysql	();
#ifndef UDB
DLLFUNC int ipserv_idok 		(Cliente *);
#endif
DLLFUNC int ipserv_sig_drop	(char *);
DLLFUNC int ipserv_sig_eos	();

DLLFUNC int ipserv_dropaips	(Proc *);

#ifndef UDB
char *make_virtualhost (Cliente *, int);
#endif

static bCom ipserv_coms[] = {
	{ "help" , ipserv_help , PREOS } ,
	{ "setipv" , ipserv_setipv , PREOS } ,
	{ "setipv2" , ipserv_setipv , PREOS } ,
	{ "temphost" , ipserv_temphost , PREOS } ,
	{ 0x0 , 0x0 , 0x0 }
};
DLLFUNC IRCFUNC(ipserv_nick);
DLLFUNC IRCFUNC(ipserv_umode);
void set(Conf *, Modulo *);

DLLFUNC ModInfo info = {
	"IpServ" ,
	0.6 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
};
	
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	char *file, *k;
	file = (char *)Malloc(sizeof(char) * (strlen(mod->archivo) + 5));
	strcpy(file, mod->archivo);
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
	return errores;
}	
DLLFUNC int descarga()
{
#ifndef UDB
	borra_comando(MSG_NICK, ipserv_nick);
	borra_comando(MSG_UMODE2, ipserv_umode);
	signal_del(NS_SIGN_IDOK, ipserv_idok);
#endif
	signal_del(SIGN_MYSQL, ipserv_sig_mysql);
	signal_del(NS_SIGN_DROP, ipserv_sig_drop);
	signal_del(SIGN_EOS, ipserv_sig_eos);
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
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *is;
	if (!ipserv)
		da_Malloc(ipserv, IpServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&ipserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&ipserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&ipserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&ipserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&ipserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				is = &ipserv_coms[0];
				while (is->com != 0x0)
				{
					if (!strcasecmp(is->com, config->seccion[i]->seccion[p]->item))
					{
						ipserv->comando[ipserv->comandos++] = is;
						break;
					}
					is++;
				}
				if (is->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&ipserv->residente, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "sufijo"))
			ircstrdup(&ipserv->sufijo, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "cambio"))
			ipserv->cambio = atoi(config->seccion[i]->data) * 3600;
	}
	if (ipserv->mascara)
		Free(ipserv->mascara);
	ipserv->mascara = (char *)Malloc(sizeof(char) * (strlen(ipserv->nick) + 1 + strlen(ipserv->ident) + 1 + strlen(ipserv->host) + 1));
	sprintf_irc(ipserv->mascara, "%s!%s@%s", ipserv->nick, ipserv->ident, ipserv->host);
#ifndef UDB
	inserta_comando(MSG_NICK, TOK_NICK, ipserv_nick, INI);
	inserta_comando(MSG_UMODE2, TOK_UMODE2, ipserv_umode, INI);
	signal_add(NS_SIGN_IDOK, ipserv_idok);
#endif
	signal_add(SIGN_MYSQL, ipserv_sig_mysql);
	signal_add(NS_SIGN_DROP, ipserv_sig_drop);
	signal_add(SIGN_EOS, ipserv_sig_eos);
	proc(ipserv_dropaips);
	mod->nick = ipserv->nick;
	mod->ident = ipserv->ident;
	mod->host = ipserv->host;
	mod->realname = ipserv->realname;
	mod->residente = ipserv->residente;
	mod->modos = ipserv->modos;
	mod->comandos = ipserv->comando;
}
BOTFUNC(ipserv_help)
{
	if (params < 2)
	{
		response(cl, ipserv->nick, "\00312%s\003 se encarga de gestionar las ips virtuales de la red.", ipserv->nick);
		response(cl, ipserv->nick, "Según esté configurado, cambiará la clave de cifrado cada \00312%i\003 horas.", ipserv->cambio / 3600);
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Comandos disponibles:");
		response(cl, ipserv->nick, "\00312SETIPV\003 Agrega o elimina una ip virtual.");
		response(cl, ipserv->nick, "\00312SETIPV2\003 Agrega o elimina una ip virtual de tipo 2.");
		response(cl, ipserv->nick, "\00312TEMPHOST\003 Cambia el host de un usuario.");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Para más información, \00312/msg %s %s comando", ipserv->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SETIPV"))
	{
		response(cl, ipserv->nick, "\00312SETIPV");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Fija una ip virtual para un determinado nick.");
		response(cl, ipserv->nick, "Este nick debe estar registrado.");
		response(cl, ipserv->nick, "Así, cuando se identifique como propietario de su nick, se le adjudicará su ip virtual personalizada.");
		response(cl, ipserv->nick, "Puede especificarse un tiempo de duración, en días.");
		response(cl, ipserv->nick, "Pasado este tiempo, su ip será eliminada.");
		response(cl, ipserv->nick, "La ip virtual puede ser una palabra, números, etc. pero siempre caracteres alfanuméricos.");
		response(cl, ipserv->nick, "Para que se visualice, es necesario que el nick lleve el modo +x.");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Sintaxis: \00312SETIPV {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "SETIPV2"))
	{
		response(cl, ipserv->nick, "\00312SETIPV2");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Fija una ip virtual de tipo 2.");
		response(cl, ipserv->nick, "Actúa de la misma forma que \00312SETIPV\003 pero la ip es de tipo 2.");
		response(cl, ipserv->nick, "Este tipo consiste en añadir el sufijo de la red al final de la ip virtual.");
		response(cl, ipserv->nick, "Así, la ip que especifiques será adjuntada del sufijo \00312%s\003 de forma automática.", ipserv->sufijo);
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Sintaxis: \00312SETIPV2 {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "TEMPHOST"))
	{
		response(cl, ipserv->nick, "\00312TEMPHOST");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Cambia el host de un usuario.");
		response(cl, ipserv->nick, "Este host sólo se visualizará si el nick tiene el modo +x.");
		response(cl, ipserv->nick, "Si el usuario cierra su sesión, el host es eliminado de tal forma que al volverla a iniciar ya no lo tendrá.");
		response(cl, ipserv->nick, " ");
		response(cl, ipserv->nick, "Sintaxis: \00312TEMPHOST nick host");
	}
	else
		response(cl, ipserv->nick, IS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(ipserv_setipv)
{
	char mod = !strcasecmp(param[0], "SETIPV2") ? 1 : 0;
	char *ipv;
	if (params < 2)
	{
		response(cl, ipserv->nick, IS_ERR_PARA, "SETIPV +-nick [ipv [caducidad]]");
		return 1;
	}
	if (!IsReg(param[1] + 1))
	{
		response(cl, ipserv->nick, IS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
#ifdef UDB
	if (!IsNickUDB(param[1] + 1))
	{
		response(cl, ipserv->nick, IS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
#endif
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, ipserv->nick, IS_ERR_PARA, "SETIPV +nick ipv [caducidad]");
			return 1;
		}
		param[1]++;
		if (mod)
		{
			buf[0] = '\0';
			sprintf_irc(buf, "%s.%s", param[2], ipserv->sufijo);
			ipv = buf;
		} 
		else
			ipv = param[2];
		_mysql_add(IS_MYSQL, param[1], "ip", ipv);
		if (params == 4)
			_mysql_add(IS_MYSQL, param[1], "caduca", "%lu", time(0) + atol(param[3]) * 8600);
#ifdef UDB
		envia_registro_bdd("N::%s::vhost %s", param[1], ipv);
#endif
		response(cl, ipserv->nick, "Se ha añadido la ip virtual a \00312%s\003.", ipv);
		if (params == 4)
			response(cl, ipserv->nick, "Esta ip caducará dentro de \00312%i\003 días.", atoi(param[3]));
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		_mysql_del(IS_MYSQL, param[1]);
#ifdef UDB
		envia_registro_bdd("N::%s::vhost", param[1]);
#endif
		response(cl, ipserv->nick, "La ip virtual de \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		response(cl, ipserv->nick, IS_ERR_SNTX, "SETIPV {+|-}nick [ipv [caducidad]]");
		return 1;
	}
	return 0;
}
BOTFUNC(ipserv_temphost)
{
	Cliente *al;
	if (params < 3)
	{
		response(cl, ipserv->nick, IS_ERR_PARA, "TEMPHOST nick nuevohost");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, ipserv->nick, IS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	ircstrdup(&al->vhost, param[2]);
	genera_mask(al);
	sendto_serv(":%s %s %s %s", me.nombre, TOK_CHGHOST, param[1], param[2]);
	response(cl, ipserv->nick, "El host de \00312%s\003 se ha cambiado por \00312%s\003.", param[1], param[2]);
	return 0;
}
#ifndef UDB
IRCFUNC(ipserv_nick)
{
	if (parc == 10)
	{
		if (strchr(parv[7], 'x'))
			make_virtualhost(cl, 1);
	}
	return 0;
}
IRCFUNC(ipserv_umode)
{
	if ((cl->modos & UMODE_HIDE) && strchr(parv[1], 'x'))
		make_virtualhost(cl, 1);
	return 0;
}
#endif
int ipserv_sig_mysql()
{
	char buf[1][BUFSIZE], tabla[1];
	int i;
	tabla[0] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, IS_MYSQL);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`ip` varchar(255) default NULL, "
  			"`caduca` int(11) NOT NULL default '0', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de ips personalizadas';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
	}
	_mysql_carga_tablas();
	return 0;
}
int comprueba_cifrado()
{
	char clave[128];
#ifndef UDB
	char tiempo[BUFSIZE];
	FILE *fp;
#endif
	sprintf_irc(clave, "a%lu", our_crc32(conf_set->cloak->crc, strlen(conf_set->cloak->crc)) + (time(0) / ipserv->cambio));
#ifdef UDB
	envia_registro_bdd("S::clave_cifrado %s", clave);
#else
	if ((fp = fopen("ts", "r")))
	{
		if (fgets(tiempo, BUFSIZE, fp))
		{
			if (atol(tiempo) + ipserv->cambio < time(0))
			{
				fclose(fp);
				escribe:
				if ((fp = fopen("ts", "w")))
				{
					tiempo[0] = '\0';
					sprintf_irc(tiempo, "%lu\n", time(0));
					fputs(tiempo, fp);
					fclose(fp);
				}
				return 0;
			}
		}
		else
			goto escribe;
	}
	else
		goto escribe;
#endif
	return 0;
}
#ifndef UDB
int ipserv_idok(Cliente *al)
{
	if (al->modos & UMODE_HIDE)
		make_virtualhost(al, 1);
	return 0;
}
#endif
int ipserv_sig_drop(char *nick)
{
	_mysql_del(IS_MYSQL, nick);
	return 0;
}
int ipserv_sig_eos()
{
#ifdef UDB
	envia_registro_bdd("S::IpServ %s", ipserv->mascara);
#endif
	comprueba_cifrado();
	return 0;
}
#ifndef UDB
char *cifra_ip(char *ipreal)
{
	static char cifrada[512], clave[BUFSIZE];
	char *p;
	int ts = 0;
	unsigned int ourcrc, v[2], k[2], x[2];
	ourcrc = our_crc32(ipreal, strlen(ipreal));
	sprintf_irc(clave, "a%lu", our_crc32(conf_set->cloak->crc, strlen(conf_set->cloak->crc)) + (time(0) / ipserv->cambio));
	p = cifrada;
	while (1)
  	{
		x[0] = x[1] = 0;
		k[0] = base64toint(clave);
		k[1] = base64toint(clave + 6);
    		v[0] = (k[0] & 0xffff0000) + ts;
    		v[1] = ourcrc;
    		tea(v, k, x);
    		inttobase64(p, x[0], 6);
    		p[6] = '.';
    		inttobase64(p + 7, x[1], 6);
		if (strchr(p, '[') == NULL && strchr(p, ']') == NULL)
			break;
    		else
		{
			if (++ts == 65536)
			{
				strcpy(p, ipreal);
				break;
			}
		}
	}
	return cifrada;
}	
char *make_virtualhost(Cliente *al, int mostrar)
{
	char *cifrada, buf[BUFSIZE], *sufix, *vip;
	cifrada = cifra_ip(al->host);
	sufix = ipserv->sufijo ? ipserv->sufijo : "virtual";
	if (IsId(al))
	{
		if (!(vip = _mysql_get_registro(IS_MYSQL, al->nombre, "ip")))
		{
			buf[0] = '\0';
			sprintf_irc(buf, "%s.%s", cifrada, sufix);
			cifrada = buf;
		}
		else
			cifrada = vip;
	}
	else
	{
		buf[0] = '\0';
		sprintf_irc(buf, "%s.%s", cifrada, sufix);
		cifrada = buf;
	}
	if (al->vhost && strcmp(al->vhost, cifrada))
	{
		ircstrdup(&al->vhost, cifrada);
		genera_mask(al);
		if (al->modos & UMODE_HIDE)
			sendto_serv(":%s %s %s %s", me.nombre, TOK_CHGHOST, al->nombre, al->vhost);
		if (mostrar)
			sendto_serv(":%s %s %s :*** Protección IP: tu dirección virtual es %s", ipserv->nick, TOK_NOTICE, al->nombre, cifrada);
	}
	return al->vhost;
}
#endif
int ipserv_dropaips(Proc *proc)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = _mysql_query("SELECT item from %s%s where caduca != '0' && caduca < %i LIMIT %i,30", PREFIJO, IS_MYSQL, time(0), proc->proc)) || !mysql_num_rows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			comprueba_cifrado();
			return 1;
		}
		while ((row = mysql_fetch_row(res)))
		{
			_mysql_del(IS_MYSQL, row[0]);
#ifdef UDB
			envia_registro_bdd("N::%s::vhost", row[0]);
#endif
		}
		proc->proc += 30;
		mysql_free_result(res);
	}
	return 0;
}
