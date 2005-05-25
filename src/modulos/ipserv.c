/*
 * $Id: ipserv.c,v 1.15 2005-05-25 21:47:26 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "bdd.h"
#include "modulos/ipserv.h"
#include "modulos/nickserv.h"

IpServ ipserv;
#define exfunc(x) busca_funcion(ipserv.hmod, x, NULL)

BOTFUNC(ipserv_help);
BOTFUNC(ipserv_setipv);
BOTFUNC(ipserv_temphost);
BOTFUNC(ipserv_clones);
#ifdef UDB
BOTFUNC(ipserv_set);
#endif

int ipserv_sig_mysql	();
#ifndef UDB
int ipserv_idok 		(Cliente *);
char *make_virtualhost 	(Cliente *, int);
int ipserv_umode		(Cliente *, char *);
int ipserv_nick		(Cliente *, int);	
#endif
int ipserv_sig_drop	(char *);
int ipserv_sig_eos	();

int ipserv_dropaips	(Proc *);


static bCom ipserv_coms[] = {
	{ "help" , ipserv_help , PREOS } ,
	{ "setipv" , ipserv_setipv , PREOS } ,
	{ "setipv2" , ipserv_setipv , PREOS } ,
	{ "temphost" , ipserv_temphost , PREOS } ,
	{ "clones" , ipserv_clones , ADMINS } ,
#ifdef UDB
	{ "set" , ipserv_set , ADMINS } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};

void set(Conf *, Modulo *);
int test(Conf *, int *);

ModInfo info = {
	"IpServ" ,
	0.9 ,
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
	return errores;
}	
int descarga()
{
#ifndef UDB
	borra_senyal(SIGN_UMODE, ipserv_umode);
	borra_senyal(NS_SIGN_IDOK, ipserv_idok);
	borra_senyal(SIGN_POST_NICK, ipserv_nick);
#endif
	borra_senyal(SIGN_MYSQL, ipserv_sig_mysql);
	borra_senyal(NS_SIGN_DROP, ipserv_sig_drop);
	borra_senyal(SIGN_EOS, ipserv_sig_eos);
	proc_stop(ipserv_dropaips);
	bot_unset(ipserv);
	return 0;
}
int test(Conf *config, int *errores)
{
	return 0;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *is;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				is = &ipserv_coms[0];
				while (is->com != 0x0)
				{
					if (!strcasecmp(is->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = is;
						break;
					}
					is++;
				}
				if (is->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		mod->comando[mod->comandos] = NULL;
#ifndef UDB
		if (!strcmp(config->seccion[i]->item, "clones"))
			ipserv.clones = atoi(config->seccion[i]->data);
#endif
		if (!strcmp(config->seccion[i]->item, "sufijo"))
			ircstrdup(ipserv.sufijo, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "cambio"))
			ipserv.cambio = atoi(config->seccion[i]->data) * 3600;
	}
#ifndef UDB
	inserta_senyal(SIGN_UMODE, ipserv_umode);
	inserta_senyal(NS_SIGN_IDOK, ipserv_idok);
	inserta_senyal(SIGN_POST_NICK, ipserv_nick);
#endif
	inserta_senyal(SIGN_MYSQL, ipserv_sig_mysql);
	inserta_senyal(NS_SIGN_DROP, ipserv_sig_drop);
	inserta_senyal(SIGN_EOS, ipserv_sig_eos);
	proc(ipserv_dropaips);
	bot_set(ipserv);
}
BOTFUNC(ipserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(ipserv), "\00312%s\003 se encarga de gestionar las ips virtuales de la red.", ipserv.hmod->nick);
		response(cl, CLI(ipserv), "Según esté configurado, cambiará la clave de cifrado cada \00312%i\003 horas.", ipserv.cambio / 3600);
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Comandos disponibles:");
		func_resp(ipserv, "SETIPV", "Agrega o elimina una ip virtual.");
		func_resp(ipserv, "SETIPV2", "Agrega o elimina una ip virtual de tipo 2.");
		func_resp(ipserv, "TEMPHOST", "Cambia el host de un usuario.");
		if (IsAdmin(cl))
		{
			func_resp(ipserv, "CLONES", "Administra la lista de ips con más clones.");
#ifdef UDB
			func_resp(ipserv, "SET", "Fija algunos parámetros de la red.");
#endif
		}
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Para más información, \00312/msg %s %s comando", ipserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SETIPV") && exfunc("SETIPV"))
	{
		response(cl, CLI(ipserv), "\00312SETIPV");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Fija una ip virtual para un determinado nick.");
		response(cl, CLI(ipserv), "Este nick debe estar registrado.");
		response(cl, CLI(ipserv), "Así, cuando se identifique como propietario de su nick, se le adjudicará su ip virtual personalizada.");
		response(cl, CLI(ipserv), "Puede especificarse un tiempo de duración, en días.");
		response(cl, CLI(ipserv), "Pasado este tiempo, su ip será eliminada.");
		response(cl, CLI(ipserv), "La ip virtual puede ser una palabra, números, etc. pero siempre caracteres alfanuméricos.");
		response(cl, CLI(ipserv), "Para que se visualice, es necesario que el nick lleve el modo +x.");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Sintaxis: \00312SETIPV {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "SETIPV2") && exfunc("SETIPV2"))
	{
		response(cl, CLI(ipserv), "\00312SETIPV2");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Fija una ip virtual de tipo 2.");
		response(cl, CLI(ipserv), "Actúa de la misma forma que \00312SETIPV\003 pero la ip es de tipo 2.");
		response(cl, CLI(ipserv), "Este tipo consiste en añadir el sufijo de la red al final de la ip virtual.");
		response(cl, CLI(ipserv), "Así, la ip que especifiques será adjuntada del sufijo \00312%s\003 de forma automática.", ipserv.sufijo);
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Sintaxis: \00312SETIPV2 {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "TEMPHOST") && exfunc("TEMPHOST"))
	{
		response(cl, CLI(ipserv), "\00312TEMPHOST");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Cambia el host de un usuario.");
		response(cl, CLI(ipserv), "Este host sólo se visualizará si el nick tiene el modo +x.");
		response(cl, CLI(ipserv), "Si el usuario cierra su sesión, el host es eliminado de tal forma que al volverla a iniciar ya no lo tendrá.");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Sintaxis: \00312TEMPHOST nick host");
	}
	else if (!strcasecmp(param[1], "CLONES") && IsAdmin(cl) && exfunc("CLONES"))
	{
		response(cl, CLI(ipserv), "\00312CLONES");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Gestiona la lista de ips con más clones.");
		response(cl, CLI(ipserv), "Estas ips o hosts tienen un número concreto de clones permitidos.");
		response(cl, CLI(ipserv), "Si se sobrepasa el límite, los usuarios son desconectados.");
		response(cl, CLI(ipserv), "Es importante distinguir entre ip y host. Si el usuario resuelve a host, deberá introducirse el host.");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Sintaxis: \00312CLONES +{ip|host} número");
		response(cl, CLI(ipserv), "Añade una ip|host con un número propio de clones.");
		response(cl, CLI(ipserv), "Sintaxis: \00312CLONES -{ip|host}");
		response(cl, CLI(ipserv), "Borra una ip|host.");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "SET") && IsAdmin(cl) && exfunc("SET"))
	{
		response(cl, CLI(ipserv), "\00312SET");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Establece varios parámetros de la red.");
		response(cl, CLI(ipserv), "\00312QUIT_IPS\003 Establece el mensaje de desconexión para ips con capacidad personal de clones.");
		response(cl, CLI(ipserv), "\00312QUIT_CLONES\003 Fija el mensaje de desconexión cuando se supera el número de clones permitidos en la red.");
		response(cl, CLI(ipserv), "\00312CLONES\003 Indica el número de clones permitidos en la red.");
		response(cl, CLI(ipserv), " ");
		response(cl, CLI(ipserv), "Sintaxis: \00312SET quit_ips|quit_clones|clones valor");
	}
#endif
	else
		response(cl, CLI(ipserv), IS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(ipserv_setipv)
{
	char mod = !strcasecmp(param[0], "SETIPV2") ? 1 : 0;
	char *ipv;
	if (params < 2)
	{
		response(cl, CLI(ipserv), IS_ERR_PARA, "SETIPV +-nick [ipv [caducidad]]");
		return 1;
	}
	if (!IsReg(param[1] + 1))
	{
		response(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
#ifdef UDB
	if (!IsNickUDB(param[1] + 1))
	{
		response(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
#endif
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, CLI(ipserv), IS_ERR_PARA, "SETIPV +nick ipv [caducidad]");
			return 1;
		}
		param[1]++;
		if (mod)
		{
			buf[0] = '\0';
			sprintf_irc(buf, "%s.%s", param[2], ipserv.sufijo);
			ipv = buf;
		} 
		else
			ipv = param[2];
		_mysql_add(IS_MYSQL, param[1], "ip", ipv);
		if (params == 4)
			_mysql_add(IS_MYSQL, param[1], "caduca", "%lu", time(0) + atol(param[3]) * 8600);
#ifdef UDB
		envia_registro_bdd("N::%s::V %s", param[1], ipv);
#endif
		response(cl, CLI(ipserv), "Se ha añadido la ip virtual a \00312%s\003.", ipv);
		if (params == 4)
			response(cl, CLI(ipserv), "Esta ip caducará dentro de \00312%i\003 días.", atoi(param[3]));
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		_mysql_del(IS_MYSQL, param[1]);
#ifdef UDB
		envia_registro_bdd("N::%s::V", param[1]);
#endif
		response(cl, CLI(ipserv), "La ip virtual de \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		response(cl, CLI(ipserv), IS_ERR_SNTX, "SETIPV {+|-}nick [ipv [caducidad]]");
		return 1;
	}
	return 0;
}
BOTFUNC(ipserv_temphost)
{
	Cliente *al;
	if (!port_existe(P_CAMBIO_HOST_REMOTO))
	{
		response(cl, CLI(ipserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		response(cl, CLI(ipserv), IS_ERR_PARA, "TEMPHOST nick nuevohost");
		return 1;
	}
	if (!(al = busca_cliente(param[1], NULL)))
	{
		response(cl, CLI(ipserv), IS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	port_func(P_CAMBIO_HOST_REMOTO)(al, param[2]);
	response(cl, CLI(ipserv), "El host de \00312%s\003 se ha cambiado por \00312%s\003.", param[1], param[2]);
	return 0;
}
#ifdef UDB
BOTFUNC(ipserv_set)
{
	if (params < 3)
	{
		response(cl, CLI(ipserv), IS_ERR_PARA, "SET quit_ips|quit_clones|clones valor");
		return 1;
	}
	if (!strcasecmp(param[1], "QUIT_IPS"))
	{
		envia_registro_bdd("S::T %s", implode(param, params, 2, -1));
		response(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión para ips con número personal de clones.");
	}
	else if (!strcasecmp(param[1], "QUIT_CLONES"))
	{
		envia_registro_bdd("S::L %s", implode(param, params, 2, -1));
		response(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión al rebasar el número de clones permitidos en la red.");
	}
	else if (!strcasecmp(param[1], "CLONES"))
	{
		int clons = atoi(param[2]);
		if (clons < 1 || clons > 1024)
		{
			response(cl, CLI(ipserv), IS_ERR_EMPT, "El número de clones debe estar entre 1 y 1024.");
			return 1;
		}
		envia_registro_bdd("S::C %c%s", CHAR_NUM, param[2]);
		response(cl, CLI(ipserv), "El número de clones permitidos en la red se ha cambiado a \00312%s", param[2]);
	}
	else
	{
		response(cl, CLI(ipserv), IS_ERR_SNTX, "SET quit_ips|quit_clones|clones valor");
		return 1;
	}
	return 0;
}
#endif
BOTFUNC(ipserv_clones)
{
	if (params < 2)
	{
		response(cl, CLI(ipserv), IS_ERR_PARA, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			response(cl, CLI(ipserv), IS_ERR_PARA, "CLONES +{ip|host} nº");
			return 1;
		}
		if (atoi(param[2]) < 1)
		{
			response(cl, CLI(ipserv), IS_ERR_SNTX, "CLONES +{ip|host} número");
			return 1;
		}
		param[1]++;
#ifdef UDB
		envia_registro_bdd("I::%s::C %c%s", param[1], CHAR_NUM, param[2]);
#else
		_mysql_add(IS_CLONS, param[1], "clones", param[2]);
#endif
		response(cl, CLI(ipserv), "La ip/host \00312%s\003 se ha añadido con \00312%s\003 clones.", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
#ifdef UDB
		envia_registro_bdd("I::%s::C", param[1]);
#else
		_mysql_del(IS_CLONS, param[1]);
#endif
		response(cl, CLI(ipserv), "La ip/host \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		response(cl, CLI(ipserv), IS_ERR_SNTX, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	return 0;
}	
#ifndef UDB
int ipserv_nick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		Cliente *aux;
		int i = 0, clons = ipserv.clones;
		char *cc;
		for (aux = clientes; aux; aux = aux->sig)
		{
			if (EsServer(aux) || EsBot(aux))
				continue;
			if (!strcmp(cl->host, aux->host))
				i++;
		}
		if ((cc = _mysql_get_registro(IS_CLONS, cl->host, "clones")))
			clons = atoi(cc);
		if (i > clons)
			port_func(P_QUIT_USUARIO_REMOTO)(cl, CLI(ipserv), "Demasiados clones.");
	}
	return 0;
}
int ipserv_umode(Cliente *cl, char *modos)
{
	if ((cl->modos & UMODE_HIDE) && strchr(modos, 'x'))
		make_virtualhost(cl, 1);
	return 0;
}
int ipserv_idok(Cliente *al)
{
	if (al->modos & UMODE_HIDE)
		make_virtualhost(al, 1);
	return 0;
}
#endif
int ipserv_sig_mysql()
{
	if (!_mysql_existe_tabla(IS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`ip` varchar(255) default NULL, "
  			"`caduca` int(11) NOT NULL default '0', "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de ips personalizadas';", PREFIJO, IS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_MYSQL);
	}
#ifndef UDB
	if (!_mysql_existe_tabla(IS_CLONS))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"`clones` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de clones';", PREFIJO, IS_CLONS))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_CLONS);
	}
#endif
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
	sprintf_irc(clave, "a%lu", our_crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv.cambio));
#ifdef UDB
	envia_registro_bdd("S::L %s", clave);
#else
	if ((fp = fopen("ts", "r")))
	{
		if (fgets(tiempo, BUFSIZE, fp))
		{
			if (atol(tiempo) + ipserv.cambio < time(0))
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
int ipserv_sig_drop(char *nick)
{
	_mysql_del(IS_MYSQL, nick);
	return 0;
}
int ipserv_sig_eos()
{
#ifdef UDB
	envia_registro_bdd("S::I %s", ipserv.hmod->mascara);
	envia_registro_bdd("S::J %s", ipserv.sufijo);
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
	sprintf_irc(clave, "a%lu", our_crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv.cambio));
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
	if (!port_existe(P_CAMBIO_HOST_REMOTO))
		return NULL;
	cifrada = cifra_ip(al->host);
	sufix = ipserv.sufijo ? ipserv.sufijo : "virtual";
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
		if (al->modos & UMODE_HIDE)
			port_func(P_CAMBIO_HOST_REMOTO)(al, al->vhost);
		if (mostrar)
			port_func(P_NOTICE)(al, CLI(ipserv), "*** Protección IP: tu dirección virtual es %s", cifrada);
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
			if (proc->time) /* no es la primera llamada de ese proc */
				comprueba_cifrado();
			proc->proc = 0;
			proc->time = time(0);
			return 1;
		}
		while ((row = mysql_fetch_row(res)))
		{
			_mysql_del(IS_MYSQL, row[0]);
#ifdef UDB
			envia_registro_bdd("N::%s::V", row[0]);
#endif
		}
		proc->proc += 30;
		mysql_free_result(res);
	}
	return 0;
}
