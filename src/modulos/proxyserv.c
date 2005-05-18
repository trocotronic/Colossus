/*
 * $Id: proxyserv.c,v 1.13 2005-05-18 18:51:09 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "modulos/proxyserv.h"

ProxyServ proxyserv;
#define exfunc(x) busca_funcion(proxyserv.hmod, x, NULL)

BOTFUNC(proxyserv_help);
BOTFUNC(proxyserv_host);

int proxyserv_sig_mysql();
#ifdef USA_THREADS
void *proxyserv_loop_principal(void *);
HANDLE proxythread;
#else
SOCKFUNC(proxy_fin);
#endif
Proxys *proxys = NULL;

int proxyserv_nick(Cliente *, int);

static bCom proxyserv_coms[] = {
	{ "help" , proxyserv_help , ADMINS } ,
	{ "host" , proxyserv_host , ADMINS } ,
	{ 0x0 , 0x0 , 0x0 }
};

void set(Conf *, Modulo *);
int test(Conf *, int *);

ModInfo info = {
	"ProxyServ" ,
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
	borra_senyal(SIGN_POST_NICK, proxyserv_nick);
	borra_senyal(SIGN_MYSQL, proxyserv_sig_mysql);
	bot_unset(proxyserv);
	return 0;
}
int test(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = busca_entrada(config, "puertos")))
	{
		conferror("[%s:%s] No has especificado puertos a escanear.]\n", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int i, puerto;
		for (i = 0; i < eval->secciones; i++)
		{
			puerto = atoi(eval->seccion[i]->item);
			if (!puerto)
			{
				conferror("[%s:%s] El puerto %s es incorrecto.]\n", config->archivo, config->item, eval->seccion[i]->item);
				error_parcial++;
			}
			else if (puerto < 0 || puerto > 65536)
			{
				conferror("[%s:%s] Los puertos deben comprenderse entre 0-65536]\n", config->archivo, config->item);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *xs;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "tiempo"))
			proxyserv.tiempo = atoi(config->seccion[i]->data) * 60;
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				xs = &proxyserv_coms[0];
				while (xs->com != 0x0)
				{
					if (!strcasecmp(xs->com, config->seccion[i]->seccion[p]->item))
					{
						mod->comando[mod->comandos++] = xs;
						break;
					}
					xs++;
				}
				if (xs->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		mod->comando[mod->comandos] = NULL;
		if (!strcmp(config->seccion[i]->item, "puertos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
				proxyserv.puerto[proxyserv.puertos++] = atoi(config->seccion[i]->seccion[p]->item);
		}
		if (!strcmp(config->seccion[i]->item, "maxlist"))
			proxyserv.maxlist = atoi(config->seccion[i]->data);
	}
	inserta_senyal(SIGN_POST_NICK, proxyserv_nick);
	inserta_senyal(SIGN_MYSQL, proxyserv_sig_mysql);
	bot_set(proxyserv);
}
void escanea(char *host)
{
	Proxys *px;
	int i;
	if (!strncmp("192.168.", host, 8) || !strcmp("127.0.0.1", host))
	{
		Free(host);
		return;
	}
	da_Malloc(px, Proxys);
	for (i = 0; i < proxyserv.puertos; i++)
	{
		da_Malloc(px->puerto[i], struct subp);
#ifdef USA_THREADS
		if ((px->puerto[i]->sck = sockopen(host, proxyserv.puerto[i], NULL, NULL, NULL, NULL, !ADD)))
#else
		if ((px->puerto[i]->sck = sockopen(host, proxyserv.puerto[i], proxy_fin, NULL, NULL, proxy_fin, ADD)))
#endif
		{
			px->puerto[i]->puerto = proxyserv.puerto[i];
			px->puertos++;
		}
	}
	px->host = host; /* ya es un strdup */
	AddItem(px, proxys);
}
BOTFUNC(proxyserv_help)
{
	if (params < 2)
	{
		response(cl, CLI(proxyserv), "\00312%s\003 controla las conexiones entrantes para detectar si se tratan de proxies.", proxyserv.hmod->nick);
		response(cl, CLI(proxyserv), "Para ello efectua un escán de puertos en la máquina. Si detecta alguno que está abierto desconecta al usuario.");
		response(cl, CLI(proxyserv), "En dicha desconexión figuran los puertos abiertos.");
		response(cl, CLI(proxyserv), "El usuario podrá conectar de nuevo si cierra estos puertos.");
		response(cl, CLI(proxyserv), "El tiempo de desconexión es de \00312%i\003 minutos.", proxyserv.tiempo / 60);
		response(cl, CLI(proxyserv), " ");
		response(cl, CLI(proxyserv), "Comandos disponibles:");
		func_resp(proxyserv, "HOST", "Host que omiten el escán.");
		response(cl, CLI(proxyserv), " ");
		response(cl, CLI(proxyserv), "Para más información, \00312/msg %s %s comando", proxyserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "HOST") && exfunc("HOST"))
	{
		response(cl, CLI(proxyserv), "\00312HOST");
		response(cl, CLI(proxyserv), " ");
		response(cl, CLI(proxyserv), "Gestiona los hosts que omiten el escán.");
		response(cl, CLI(proxyserv), "Estos hosts no se someterán al escán de puertos y podrán entrar libremente.");
		response(cl, CLI(proxyserv), "Cabe mencionar que si el usuario conecta sin su DNS resuelto, figurará su ip en vez de su host.");
		response(cl, CLI(proxyserv), "Por este motivo, es importante añadir el host o la ip, según conecte el usuario.");
		response(cl, CLI(proxyserv), "Si no se especifica ni + ni -, se usará el patrón como búsqueda.");
		response(cl, CLI(proxyserv), " ");
		response(cl, CLI(proxyserv), "Sintaxis: \00312HOST {+|-|patrón}[{host|ip}]");
	}
	else
		response(cl, CLI(proxyserv), XS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(proxyserv_host)
{
	if (params < 2)
	{
		response(cl, CLI(proxyserv), XS_ERR_PARA, "HOST {+|-|patrón} [host|ip]");
		return 1;
	}
	if (*param[1] == '+')
	{
		_mysql_query("INSERT INTO %s%s (item) values ('%s')", PREFIJO, XS_MYSQL, param[1] + 1);
		response(cl, CLI(proxyserv), "El host \00312%s\003 se ha añadido para omitir el escaneo de puertos.", param[1] + 1);
	}
	else if (*param[1] == '-')
	{
		_mysql_del(XS_MYSQL, param[1] + 1);
		response(cl, CLI(proxyserv), "El host \00312%s\003 ha sido borrado de la lista de excepciones.");
	}
	else
	{
		MYSQL_RES *res;
		MYSQL_ROW row;
		char *rep;
		int i;
		rep = str_replace(param[1], '*', '%');
		if (!(res = _mysql_query("SELECT item from %s%s where item LIKE '%s'", PREFIJO, XS_MYSQL, rep)))
		{
			response(cl, CLI(proxyserv), XS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		response(cl, CLI(proxyserv), "*** Hosts que coinciden con el patrón \00312%s\003 ***", param[1]);
		for (i = 0; i < proxyserv.maxlist && (row = mysql_fetch_row(res)); i++)
			response(cl, CLI(proxyserv), "\00312%s", row[0]);
		response(cl, CLI(proxyserv), "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
		mysql_free_result(res);
	}
	return 0;
}
int proxyserv_nick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		Proxys *px;
		char *host;
		if (!BadPtr(cl->ip) && *(cl->ip) != '*')
			host = strdup(cl->ip);
		else
			host = strdup(cl->host);
		if (coge_cache(CACHE_PROXY, host, proxyserv.hmod->id) || _mysql_get_registro(XS_MYSQL, host, NULL))
			return 1;
		for (px = proxys; px; px = px->sig)
		{
			if (!strcasecmp(px->host, host))
				return 1;
		}
		port_func(P_NOTICE)(cl, CLI(proxyserv), "Se va a proceder a hacer un escáner de puertos a tu máquina para verificar que no se trata de un proxy.");
		escanea(host);
	}
	return 0;
}
int proxyserv_sig_mysql()
{
	if (!_mysql_existe_tabla(XS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
  			"`item` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de excepción de hosts';", PREFIJO, XS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, XS_MYSQL);
	}
	_mysql_carga_tablas();
#ifdef USA_THREADS
	if ((proxythread = _beginthread(proxyserv_loop_principal, 0, NULL)) < 0)
		return 0;
#endif
	return 1;
}
#ifdef USA_THREADS
void proxy_fin(Proxys *px)
#else
SOCKFUNC(proxy_fin)
#endif
{
#ifndef USA_THREADS	
	Proxys *px;
	for (px = proxys; px; px = px->sig)
	{
		if (!strcmp(px->host, sck->host))
			break;
	}
	if (!px)
		return 0;
#endif
	if (++px->escaneados == proxyserv.puertos) /* fin del scan */
	{
		int i;
		char abiertos[256];
		abiertos[0] = '\0';
		for (i = 0; i < px->puertos; i++)
		{
			if (EsOk(px->puerto[i]->sck))
			{
				char puerto[10];
				puerto[0] = '\0';
				sprintf_irc(puerto, " %i", px->puerto[i]->puerto);
				strcat(abiertos, puerto);
				sockclose(px->puerto[i]->sck, 0);
			}
		}
		if (abiertos[0] != '\0')
		{
			char motivo[300];
			sprintf_irc(motivo, "Posible proxy ilegal. Puertos abiertos: %s", abiertos + 1);
			port_func(P_GLINE)(CLI(proxyserv), ADD, "*", px->host, proxyserv.tiempo, motivo);
		}
		else
			inserta_cache(CACHE_PROXY, px->host, 86400, proxyserv.hmod->id, px->host);
		BorraItem(px, proxys);
		Free(px->host);
		Free(px);
	}
#ifndef USA_THREADS
	return 0;
#endif	
}
#ifdef USA_THREADS
void *proxyserv_loop_principal(void *args)
{
	while (1)
	{
		int i, sels;
		fd_set read_set, write_set, excpt_set;
		struct timeval wait;
		Sock *sck;
		Proxys *px;
		if (!proxys)
			Sleep(500);
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_ZERO(&excpt_set);
		for (px = proxys; px; px = px->sig)
		{
			for (i = 0; i < px->puertos; i++)
			{
				sck = px->puerto[i]->sck;
				if (sck->pres >= 0 && !EsCerr(sck))
				{
					FD_SET(sck->pres, &read_set);
					FD_SET(sck->pres, &excpt_set);
					if (sck->sendQ->len || EsConn(sck))
						FD_SET(sck->pres, &write_set);
				}
			}
		}
		wait.tv_sec = 1;
		wait.tv_usec = 0;
		sels = select(MAXSOCKS + 1, &read_set, &write_set, &excpt_set, &wait);
		for (px = proxys; px; px = px->sig)
		{
			if (!sels)
				break;
			for (i = 0; i < px->puertos; i++)
			{
				sck = px->puerto[i]->sck;
				if (EsConn(sck) && (FD_ISSET(sck->pres, &write_set) || FD_ISSET(sck->pres, &read_set)))
				{
					SockOk(sck);
					proxy_fin(px);
					sels--;
				}
				if (FD_ISSET(sck->pres, &excpt_set))
				{
					SockCerr(sck);
					proxy_fin(px);
					continue;
				}
			}
		}
	}
	return NULL;
}
#endif
