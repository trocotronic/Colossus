/*
 * $Id: proxyserv.c,v 1.7 2004-10-23 22:42:22 Trocotronic Exp $ 
 */

#include "struct.h"
#include "comandos.h"
#include "ircd.h"
#include "modulos.h"
#ifdef UDB
#include "bdd.h"
#endif
#include "proxyserv.h"

ProxyServ *proxyserv = NULL;

static int proxyserv_help	(Cliente *, char *[], int, char *[], int);
static int proxyserv_host	(Cliente *, char *[], int, char *[], int);

DLLFUNC int proxyserv_sig_mysql();
#ifdef USA_THREADS
void *proxyserv_loop_principal(void *);
HANDLE proxythread;
#else
DLLFUNC SOCKFUNC(proxy_fin);
#endif
Proxys *proxys = NULL;
DLLFUNC int proxyserv_dropacache(void);

DLLFUNC IRCFUNC(proxyserv_nick);

static bCom proxyserv_coms[] = {
	{ "help" , proxyserv_help , ADMINS } ,
	{ "host" , proxyserv_host , ADMINS } ,
	{ 0x0 , 0x0 , 0x0 }
};

void set(Conf *, Modulo *);
int test(Conf *, int *);

DLLFUNC ModInfo info = {
	"ProxyServ" ,
	0.6 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" ,
	"proxyserv.inc"
};
	
DLLFUNC int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (parseconf(mod->config, &modulo, 1))
		return 1;
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
	borra_comando(MSG_NICK, proxyserv_nick);
	borra_senyal(SIGN_MYSQL, proxyserv_sig_mysql);
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
	if (!proxyserv)
		da_Malloc(proxyserv, ProxyServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&proxyserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&proxyserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&proxyserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&proxyserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&proxyserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "tiempo"))
			proxyserv->tiempo = atoi(config->seccion[i]->data) * 60;
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				xs = &proxyserv_coms[0];
				while (xs->com != 0x0)
				{
					if (!strcasecmp(xs->com, config->seccion[i]->seccion[p]->item))
					{
						proxyserv->comando[proxyserv->comandos++] = xs;
						break;
					}
					xs++;
				}
				if (xs->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&proxyserv->residente, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "puertos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
				proxyserv->puerto[proxyserv->puertos++] = atoi(config->seccion[i]->seccion[p]->item);
		}
		if (!strcmp(config->seccion[i]->item, "maxlist"))
			proxyserv->maxlist = atoi(config->seccion[i]->data);
	}
	if (proxyserv->mascara)
		Free(proxyserv->mascara);
	proxyserv->mascara = (char *)Malloc(sizeof(char) * (strlen(proxyserv->nick) + 1 + strlen(proxyserv->ident) + 1 + strlen(proxyserv->host) + 1));
	sprintf_irc(proxyserv->mascara, "%s!%s@%s", proxyserv->nick, proxyserv->ident, proxyserv->host);
	inserta_comando(MSG_NICK, TOK_NICK, proxyserv_nick, INI);
	inserta_senyal(SIGN_MYSQL, proxyserv_sig_mysql);
	proc(proxyserv_dropacache);
	mod->nick = proxyserv->nick;
	mod->ident = proxyserv->ident;
	mod->host = proxyserv->host;
	mod->realname = proxyserv->realname;
	mod->residente = proxyserv->residente;
	mod->modos = proxyserv->modos;
	mod->comandos = proxyserv->comando;
}
void escanea(char *host)
{
	Proxys *px;
	int i;
	if (!match("192.168.*", host) || !strcmp("127.0.0.1", host))
		return;
	da_Malloc(px, Proxys);
	for (i = 0; i < proxyserv->puertos; i++)
	{
		da_Malloc(px->puerto[i], struct subp);
#ifdef USA_THREADS
		if ((px->puerto[i]->sck = sockopen(host, proxyserv->puerto[i], NULL, NULL, NULL, NULL, !ADD)))
#else
		if ((px->puerto[i]->sck = sockopen(host, proxyserv->puerto[i], proxy_fin, NULL, NULL, proxy_fin, ADD)))
#endif
		{
			px->puerto[i]->puerto = &proxyserv->puerto[i];
			px->puertos++;
		}
	}
	px->host = strdup(host);
	px->sig = proxys;
	px->prev = NULL;
	if (proxys)
		proxys->prev = px;
	proxys = px;
}
BOTFUNC(proxyserv_help)
{
	if (params < 2)
	{
		response(cl, proxyserv->nick, "\00312%s\003 controla las conexiones entrantes para detectar si se tratan de proxies.", proxyserv->nick);
		response(cl, proxyserv->nick, "Para ello efectua un escán de puertos en la máquina. Si detecta alguno que está abierto desconecta al usuario.");
		response(cl, proxyserv->nick, "En dicha desconexión figuran los puertos abiertos.");
		response(cl, proxyserv->nick, "El usuario podrá conectar de nuevo si cierra estos puertos.");
		response(cl, proxyserv->nick, "El tiempo de desconexión es de \00312%i\003 minutos.", proxyserv->tiempo / 60);
		response(cl, proxyserv->nick, " ");
		response(cl, proxyserv->nick, "Comandos disponibles:");
		response(cl, proxyserv->nick, "\00312HOST\003 Host que omiten el escán.");
		response(cl, proxyserv->nick, " ");
		response(cl, proxyserv->nick, "Para más información, \00312/msg %s %s comando", proxyserv->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "HOST"))
	{
		response(cl, proxyserv->nick, "\00312HOST");
		response(cl, proxyserv->nick, " ");
		response(cl, proxyserv->nick, "Gestiona los hosts que omiten el escán.");
		response(cl, proxyserv->nick, "Estos hosts no se someterán al escán de puertos y podrán entrar libremente.");
		response(cl, proxyserv->nick, "Cabe mencionar que si el usuario conecta sin su DNS resuelto, figurará su ip en vez de su host.");
		response(cl, proxyserv->nick, "Por este motivo, es importante añadir el host o la ip, según conecte el usuario.");
		response(cl, proxyserv->nick, "Si no se especifica ni + ni -, se usará el patrón como búsqueda.");
		response(cl, proxyserv->nick, " ");
		response(cl, proxyserv->nick, "Sintaxis: \00312HOST {+|-|patrón}[{host|ip}]");
	}
	else
		response(cl, proxyserv->nick, XS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(proxyserv_host)
{
	if (params < 2)
	{
		response(cl, proxyserv->nick, XS_ERR_PARA, "HOST {+|-|patrón}[{host|ip}]");
		return 1;
	}
	if (*param[1] == '+')
	{
		_mysql_query("INSERT INTO %s%s (item) values ('%s')", PREFIJO, XS_MYSQL, param[1] + 1);
		response(cl, proxyserv->nick, "El host \00312%s\003 se ha añadido para omitir el escaneo de puertos.", param[1] + 1);
	}
	else if (*param[1] == '-')
	{
		_mysql_del(XS_MYSQL, param[1] + 1);
		response(cl, proxyserv->nick, "El host \00312%s\003 ha sido borrado de la lista de excepciones.");
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
			response(cl, proxyserv->nick, XS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		response(cl, proxyserv->nick, "*** Hosts que coinciden con el patrón \00312%s\003 ***", param[1]);
		for (i = 0; i < proxyserv->maxlist && (row = mysql_fetch_row(res)); i++)
			response(cl, proxyserv->nick, "\00312%s", row[0]);
		response(cl, proxyserv->nick, "Resultado: \00312%i\003/\00312%i", i, mysql_num_rows(res));
		mysql_free_result(res);
	}
	return 0;
}
IRCFUNC(proxyserv_nick)
{
	if (parc > 4)
	{
		Proxys *px;
		char *host;
		if (cl->ip && *(cl->ip) != '*')
			host = cl->ip;
		else
			host = cl->host;
		if (_mysql_get_registro(MYSQL_CACHE, host, "hora") || _mysql_get_registro(XS_MYSQL, host, NULL))
			return 1;
		for (px = proxys; px; px = px->sig)
		{
			if (!strcasecmp(px->host, host))
				return 1;
		}
		sendto_serv(":%s %s %s :Se va a proceder a hacer un escáner de puertos a tu máquina para verificar que no se trata de un proxy.", proxyserv->nick, TOK_NOTICE, cl->nombre);
		escanea(host);
	}
	return 0;
}
int proxyserv_sig_mysql()
{
	char buf[1][BUFSIZE], tabla[1];
	int i;
	tabla[0] = 0;
	sprintf_irc(buf[0], "%s%s", PREFIJO, XS_MYSQL);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf[0]))
			tabla[0] = 1;
	}
	if (!tabla[0])
	{
		if (_mysql_query("CREATE TABLE `%s` ( "
  			"`item` varchar(255) default NULL, "
  			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla de excepción de hosts';", buf[0]))
				fecho(FADV, "Ha sido imposible crear la tabla '%s'.", buf[0]);
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
DLLFUNC SOCKFUNC(proxy_fin)
#endif
{
#ifndef USA_THREADS	
	Proxys *px;
	for (px = proxys; px; px = px->sig)
	{
		if (!strcmp(px->host, sck->host))
			break;
	}
#endif
	if (++px->escaneados == proxyserv->puertos) /* fin del scan */
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
				sprintf_irc(puerto, "%i ", *(px->puerto[i]->puerto));
				strcat(abiertos, puerto);
				sockclose(px->puerto[i]->sck, 0);
			}
			Free(px->puerto[i]);
		}
		if (abiertos[0] != '\0')
		{
			char motivo[300];
			sprintf_irc(motivo, "Posible proxy ilegal. Puertos abiertos: %s", abiertos);
			irctkl(TKL_ADD, TKL_GLINE, "*", px->host, proxyserv->mascara, proxyserv->tiempo, motivo);
		}
		else
			_mysql_add(MYSQL_CACHE, px->host, "hora", "%lu", time(0));
		if (px->prev)
			px->prev->sig = px->sig;
		else
			proxys = px->sig;
		if (px->sig)
			px->sig->prev = px->prev;
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
int proxyserv_dropacache()
{
	_mysql_query("DELETE from %s%s where hora < %i AND hora !='0'", PREFIJO, MYSQL_CACHE, time(0) - 86400); /* validez, un día */
	return 0;
}
