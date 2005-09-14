/*
 * $Id: proxyserv.c,v 1.17 2005-09-14 14:45:06 Trocotronic Exp $ 
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
#define ExFunc(x) BuscaFuncion(proxyserv.hmod, x, NULL)

BOTFUNC(PSHelp);
BOTFUNC(PSHost);

int PSSigSQL();
#ifdef USA_THREADS
void *PSLoop(void *);
HANDLE proxythread;
#else
SOCKFUNC(PSFin);
#endif
Proxys *proxys = NULL;

int PSCmdNick(Cliente *, int);

static bCom proxyserv_coms[] = {
	{ "help" , PSHelp , ADMINS } ,
	{ "host" , PSHost , ADMINS } ,
	{ 0x0 , 0x0 , 0x0 }
};

void PSSet(Conf *, Modulo *);
int PSTest(Conf *, int *);

ModInfo MOD_INFO(ProxyServ) = {
	"ProxyServ" ,
	0.9 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int MOD_CARGA(ProxyServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, MOD_INFO(ProxyServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(ProxyServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(ProxyServ).nombre))
			{
				if (!PSTest(modulo.seccion[0], &errores))
					PSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el PSTest", mod->archivo, MOD_INFO(ProxyServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(ProxyServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	return errores;
}
int MOD_DESCARGA(ProxyServ)()
{
	BorraSenyal(SIGN_POST_NICK, PSCmdNick);
	BorraSenyal(SIGN_SQL, PSSigSQL);
	BotUnset(proxyserv);
	return 0;
}
int PSTest(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "puertos")))
	{
		Error("[%s:%s] No has especificado puertos a escanear.]\n", config->archivo, config->item);
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
				Error("[%s:%s] El puerto %s es incorrecto.]\n", config->archivo, config->item, eval->seccion[i]->item);
				error_parcial++;
			}
			else if (puerto < 0 || puerto > 65536)
			{
				Error("[%s:%s] Los puertos deben comprenderse entre 0-65536]\n", config->archivo, config->item);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void PSSet(Conf *config, Modulo *mod)
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
					Error("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
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
	InsertaSenyal(SIGN_POST_NICK, PSCmdNick);
	InsertaSenyal(SIGN_SQL, PSSigSQL);
	BotSet(proxyserv);
}
void PSEscanea(char *host)
{
	Proxys *px;
	int i;
	if (!strncmp("192.168.", host, 8) || !strcmp("127.0.0.1", host))
	{
		Free(host);
		return;
	}
	BMalloc(px, Proxys);
	for (i = 0; i < proxyserv.puertos; i++)
	{
		BMalloc(px->puerto[i], struct subp);
#ifdef USA_THREADS
		if ((px->puerto[i]->sck = SockOpen(host, proxyserv.puerto[i], NULL, NULL, NULL, NULL, !ADD)))
#else
		if ((px->puerto[i]->sck = SockOpen(host, proxyserv.puerto[i], PSFin, NULL, NULL, PSFin, ADD)))
#endif
		{
			px->puerto[i]->puerto = proxyserv.puerto[i];
			px->puertos++;
		}
	}
	px->host = host; /* ya es un strdup */
	AddItem(px, proxys);
}
BOTFUNC(PSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), "\00312%s\003 controla las conexiones entrantes para detectar si se tratan de proxies.", proxyserv.hmod->nick);
		Responde(cl, CLI(proxyserv), "Para ello efectua un escán de puertos en la máquina. Si detecta alguno que está abierto desconecta al usuario.");
		Responde(cl, CLI(proxyserv), "En dicha desconexión figuran los puertos abiertos.");
		Responde(cl, CLI(proxyserv), "El usuario podrá conectar de nuevo si cierra estos puertos.");
		Responde(cl, CLI(proxyserv), "El tiempo de desconexión es de \00312%i\003 minutos.", proxyserv.tiempo / 60);
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Comandos disponibles:");
		FuncResp(proxyserv, "HOST", "Host que omiten el escán.");
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Para más información, \00312/msg %s %s comando", proxyserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "HOST") && ExFunc("HOST"))
	{
		Responde(cl, CLI(proxyserv), "\00312HOST");
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Gestiona los hosts que omiten el escán.");
		Responde(cl, CLI(proxyserv), "Estos hosts no se someterán al escán de puertos y podrán entrar libremente.");
		Responde(cl, CLI(proxyserv), "Cabe mencionar que si el usuario conecta sin su DNS resuelto, figurará su ip en vez de su host.");
		Responde(cl, CLI(proxyserv), "Por este motivo, es importante añadir el host o la ip, según conecte el usuario.");
		Responde(cl, CLI(proxyserv), "Si no se especifica ni + ni -, se usará el patrón como búsqueda.");
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Sintaxis: \00312HOST {+|-|patrón}[{host|ip}]");
	}
	else
		Responde(cl, CLI(proxyserv), XS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(PSHost)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), XS_ERR_PARA, "HOST {+|-|patrón} [host|ip]");
		return 1;
	}
	if (*param[1] == '+')
	{
		SQLQuery("INSERT INTO %s%s (item) values ('%s')", PREFIJO, XS_SQL, param[1] + 1);
		Responde(cl, CLI(proxyserv), "El host \00312%s\003 se ha añadido para omitir el escaneo de puertos.", param[1] + 1);
	}
	else if (*param[1] == '-')
	{
		SQLBorra(XS_SQL, param[1] + 1);
		Responde(cl, CLI(proxyserv), "El host \00312%s\003 ha sido borrado de la lista de excepciones.");
	}
	else
	{
		SQLRes res;
		SQLRow row;
		char *rep;
		int i;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item from %s%s where item LIKE '%s'", PREFIJO, XS_SQL, rep)))
		{
			Responde(cl, CLI(proxyserv), XS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(proxyserv), "*** Hosts que coinciden con el patrón \00312%s\003 ***", param[1]);
		for (i = 0; i < proxyserv.maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(proxyserv), "\00312%s", row[0]);
		Responde(cl, CLI(proxyserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	return 0;
}
int PSCmdNick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		Proxys *px;
		char *host;
		if (!BadPtr(cl->ip) && *(cl->ip) != '*')
			host = strdup(cl->ip);
		else
			host = strdup(cl->host);
		if (CogeCache(CACHE_PROXY, host, proxyserv.hmod->id) || SQLCogeRegistro(XS_SQL, host, NULL))
			return 1;
		for (px = proxys; px; px = px->sig)
		{
			if (!strcasecmp(px->host, host))
				return 1;
		}
		ProtFunc(P_NOTICE)(cl, CLI(proxyserv), "Se va a proceder a hacer un escáner de puertos a tu máquina para verificar que no se trata de un proxy.");
		PSEscanea(host);
	}
	return 0;
}
int PSSigSQL()
{
	if (!SQLEsTabla(XS_SQL))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL "
			");", PREFIJO, XS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, XS_SQL);
	}
	SQLCargaTablas();
#ifdef USA_THREADS
	if ((proxythread = _beginthread(PSLoop, 0, NULL)) < 0)
		return 0;
#endif
	return 1;
}
#ifdef USA_THREADS
void PSFin(Proxys *px)
#else
SOCKFUNC(PSFin)
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
				ircsprintf(puerto, " %i", px->puerto[i]->puerto);
				strcat(abiertos, puerto);
				SockClose(px->puerto[i]->sck, 0);
			}
		}
		if (abiertos[0] != '\0')
		{
			char motivo[300];
			ircsprintf(motivo, "Posible proxy ilegal. Puertos abiertos: %s", abiertos + 1);
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", px->host, proxyserv.tiempo, motivo);
		}
		else
			InsertaCache(CACHE_PROXY, px->host, 86400, proxyserv.hmod->id, px->host);
		BorraItem(px, proxys);
		Free(px->host);
		Free(px);
	}
#ifndef USA_THREADS
	return 0;
#endif	
}
#ifdef USA_THREADS
void *PSLoop(void *args)
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
					PSFin(px);
					sels--;
				}
				if (FD_ISSET(sck->pres, &excpt_set))
				{
					SockCerr(sck);
					PSFin(px);
					continue;
				}
			}
		}
	}
	return NULL;
}
#endif
