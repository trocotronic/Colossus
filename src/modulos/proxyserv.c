/*
 * $Id: proxyserv.c,v 1.42 2008/02/13 16:16:10 Trocotronic Exp $
 */

#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/proxyserv.h"

ProxyServ *proxyserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, proxyserv->hmod, NULL)

BOTFUNC(PSHelp);
BOTFUNC(PSHost);
BOTFUNCHELP(PSHHost);

int PSSigSQL();
int PSSigEOS();
int PSSigSynch();
SOCKFUNC(PSAbre);
SOCKFUNC(PSLee);
SOCKFUNC(PSFin);
Proxy *proxys = NULL;

int PSCmdNick(Cliente *, int);
void PSEscanea(char *);
void PSProxOK(Proxy *px);

Opts TiposProxy[] = {
	{ XS_T_HTTP , "HTTP" } ,
	{ XS_T_SOCKS4 , "SOCKS4" } ,
	{ XS_T_SOCKS5 , "SOCKS5" } ,
	{ XS_T_ROUTER , "ROUTER" } ,
	{ XS_T_WINGATE , "WINGATE" } ,
	{ XS_T_POST , "POST" } ,
	{ XS_T_GET , "GET" } ,
	{ 0x0 , 0x0 }
};

static bCom proxyserv_coms[] = {
	{ "help" , PSHelp , N4 , "Muestra esta ayuda." , NULL } ,
	{ "host" , PSHost , N4 , "Hosts que omiten el escáner." , PSHHost } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void PSSet(Conf *, Modulo *);
int PSTest(Conf *, int *);

ModInfo MOD_INFO(ProxyServ) = {
	"ProxyServ" ,
	0.11 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};

int MOD_CARGA(ProxyServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	//mod->activo = 1;
	if (mod->config)
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
	else
		PSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(ProxyServ)()
{
	Proxy *px;
	for (px = proxys; px; px = px->sig)
		SockClose(px->sck);
	BorraSenyal(SIGN_POST_NICK, PSCmdNick);
	BorraSenyal(SIGN_SQL, PSSigSQL);
	BorraSenyal(SIGN_EOS, PSSigEOS);
	BorraSenyal(SIGN_SYNCH, PSSigSynch);
	BotUnset(proxyserv);
	return 0;
}
int PSTest(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "puertos")))
	{
		Error("[%s:%s] No has especificado puertos a escanear", config->archivo, config->item);
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
				Error("[%s:%s::%i] El puerto %s es incorrecto", config->archivo, config->item, eval->seccion[i]->item, eval->seccion[i]->linea);
				error_parcial++;
			}
			else if (puerto < 0 || puerto > 65536)
			{
				Error("[%s:%s::%i] Los puertos deben comprenderse entre 0-65536", config->archivo, config->item, eval->seccion[i]->linea);
				error_parcial++;
			}
			if (eval->seccion[i]->data)
			{
				char *tok;
				strlcpy(tokbuf, eval->seccion[i]->data, sizeof(tokbuf));
				for (tok = strtok(tokbuf, ","); tok; tok = strtok(NULL, ","))
				{
					if (!BuscaOpt(tok, TiposProxy))
					{
						Error("[%s:%s::%s::%i] No se reconoce el método proxy %s", config->archivo, config->item, eval->item, eval->seccion[i]->linea, tok);
						error_parcial++;
						break;
					}
				}
			}
		}
	}
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, proxyserv_coms, 1);
	if ((eval = BuscaEntrada(config, "lista")))
	{
		FILE *fp;
		if (!(fp = fopen(eval->data, "r")))
		{
			Error("[%s:%s] No se puede abrir el archivo %s]\n", config->archivo, config->item, eval->data);
			error_parcial++;
		}
		else
			fclose(fp);
	}
	if (!(eval = BuscaEntrada(config, "scan_ip")))
	{
		Error("[%s:%s] No has especificado la ip del escáner", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!EsIp(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz scan_ip debe ser una IP", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "scan_puerto")))
	{
		Error("[%s:%s] No has especificado el puerto del escáner", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int port;
		if (!eval->data || (port = atoi(eval->data)) <= 0 || port > 65535)
		{
			Error("[%s:%s::%s::%i] La directriz scan_puerto debe ser un puerto válido", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "patron")))
	{
		Error("[%s:%s] No has especificado ningún patrón para el escáner", config->archivo, config->item);
		error_parcial++;
	}
	*errores += error_parcial;
	return error_parcial;
}
void PSSet(Conf *config, Modulo *mod)
{
	int i, p, q = 0;
	struct _puerto *ppt;
	if (!proxyserv)
		proxyserv = BMalloc(ProxyServ);
	proxyserv->maxlist = 30;
	proxyserv->tiempo = 3600;
	proxyserv->detalles = 0;
	for (q = 0; q < MAX_PATRON; q++)
		ircfree(proxyserv->patron[q]);
	q = 0;
	proxyserv->patron[q++] = strdup("esto es un proxy");
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "tiempo"))
				proxyserv->tiempo = atoi(config->seccion[i]->data) * 60;
			else if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, proxyserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, proxyserv_coms);
			else if (!strcmp(config->seccion[i]->item, "puertos"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					int po = atoi(config->seccion[i]->seccion[p]->item);
					if (config->seccion[i]->seccion[p]->data)
					{
						char *tok;
						strlcpy(tokbuf, config->seccion[i]->seccion[p]->data, sizeof(tokbuf));
						for (tok = strtok(tokbuf, ","); tok; tok = strtok(NULL, ","))
						{
							ppt = BMalloc(struct _puerto);
							ppt->puerto = po;
							ppt->tipo = BuscaOpt(tok, TiposProxy);
							AddItem(ppt, proxyserv->puerto);
						}
					}
					else
					{
						ppt = BMalloc(struct _puerto);
						ppt->puerto = po;
						ppt->tipo = XS_T_ABIERTO;
						AddItem(ppt, proxyserv->puerto);
					}
				}
			}
			else if (!strcmp(config->seccion[i]->item, "maxlist"))
				proxyserv->maxlist = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
			else if (!strcmp(config->seccion[i]->item, "lista"))
				ircstrdup(proxyserv->lista, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "scan_ip"))
				ircstrdup(proxyserv->scan_ip, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "scan_puerto"))
				proxyserv->scan_puerto = atoi(config->seccion[i]->data);
			else if (q < MAX_PATRON-1 && !strcmp(config->seccion[i]->item, "patron"))
				ircstrdup(proxyserv->patron[q++], config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "quits_detallados"))
				proxyserv->detalles = 1;
			else if (!strcmp(config->seccion[i]->item, "lista_online"))
				proxyserv->opm = 1;
			else if (!strcmp(config->seccion[i]->item, "ignorar_opers"))
				proxyserv->igops = 1;
			else if (!strcmp(config->seccion[i]->item, "ignorar_nicks_id"))
				proxyserv->igids = 1;
		}
	}
	else
		ProcesaComsMod(NULL, mod, proxyserv_coms);
	proxyserv->patron[q] = NULL;
	InsertaSenyal(SIGN_POST_NICK, PSCmdNick);
	InsertaSenyal(SIGN_SQL, PSSigSQL);
	InsertaSenyal(SIGN_EOS, PSSigEOS);
	InsertaSenyal(SIGN_SYNCH, PSSigSynch);
	BotSet(proxyserv);
}
BOTFUNCHELP(PSHHost)
{
	Responde(cl, CLI(proxyserv), "Gestiona los hosts que omiten el escán.");
	Responde(cl, CLI(proxyserv), "Estos hosts no se someterán al escán de puertos y podrán entrar libremente.");
	Responde(cl, CLI(proxyserv), "Cabe mencionar que si el usuario conecta sin su DNS resuelto, figurará su ip en vez de su host.");
	Responde(cl, CLI(proxyserv), "Por este motivo, es importante añadir el host o la ip, según conecte el usuario.");
	Responde(cl, CLI(proxyserv), "Si no se especifica ni + ni -, se usará el patrón como búsqueda.");
	Responde(cl, CLI(proxyserv), " ");
	Responde(cl, CLI(proxyserv), "Sintaxis: \00312HOST {+|-|patrón}[{host|ip}]");
	return 0;
}
BOTFUNC(PSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), "\00312%s\003 controla las conexiones entrantes para detectar si se tratan de proxies.", proxyserv->hmod->nick);
		Responde(cl, CLI(proxyserv), "Para ello efectua un escán de puertos en la máquina. Si detecta alguno que está abierto desconecta al usuario.");
		Responde(cl, CLI(proxyserv), "En dicha desconexión figuran los puertos abiertos.");
		Responde(cl, CLI(proxyserv), "El usuario podrá conectar de nuevo si cierra estos puertos.");
		Responde(cl, CLI(proxyserv), "El tiempo de desconexión es de \00312%i\003 minutos.", proxyserv->tiempo / 60);
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Comandos disponibles:");
		ListaDescrips(proxyserv->hmod, cl);
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Para más información, \00312/msg %s %s comando", proxyserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], proxyserv->hmod, param, params))
		Responde(cl, CLI(proxyserv), XS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(PSHost)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), XS_ERR_PARA, fc->com, "{+|-|patrón} [host|ip]");
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
		for (i = 0; i < proxyserv->maxlist && (row = SQLFetchRow(res)); i++)
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
		Proxy *px;
		char *host;
//		Debug("******%s",proxyserv->scan_ip);
		if (!BadPtr(cl->ip) && *(cl->ip) != '*')
			host = cl->ip;
		else
			host = cl->host;
		if (CogeCache(CACHE_PROXY, host, proxyserv->hmod->id) || SQLCogeRegistro(XS_SQL, host, NULL))
			return 1;
		for (px = proxys; px; px = px->sig)
		{
			if (!strcasecmp(px->host, host))
				return 1;
		}
		if (!(proxyserv->igops && IsOper(cl)) && !(proxyserv->igids && IsId(cl)))
		{
			ProtFunc(P_NOTICE)(cl, CLI(proxyserv), "Se va a proceder a hacer un escáner de puertos a tu máquina para verificar que no se trata de un proxy.");
			PSEscanea(host);
		}
	}
	return 0;
}
void PSEscanea(char *host)
{
	Proxy *px;
	if (!strncmp("192.168", host, 7) || !strcmp("127.0.0.1", host))
		return;
	if (proxyserv->opm && EsIp(host))
	{
		u_int tmp1, tmp2, tmp3, tmp4;
		struct hostent *he;
		sscanf(host, "%u.%u.%u.%u", &tmp1, &tmp2, &tmp3, &tmp4);
		ircsprintf(buf, "%u.%u.%u.%u.dnsbl.dronebl.org", tmp4, tmp3, tmp2, tmp1);
		if ((he = gethostbyname(buf)))
		{
			char motivo[256];
			if (he && proxyserv->detalles)
			{
				char *tmp;
				int res = ntohl((*(struct in_addr *)he->h_addr).s_addr)&0xFF;
				if (res == 2)
					tmp = "SAMPLE";
				else if (res == 3)
					tmp = "IRC DRONE";
				else if (res == 5)
					tmp = "BOTTLER";
				else if (res == 6)
					tmp = "SPAMBOT";
				else if (res == 7)
					tmp = "DDOS DRONE";
				else if (res == 8)
					tmp = "SOCKSx";
				else if (res == 9)
					tmp = "HTTP";
				else if (res == 10)
					tmp = "CHAIN";
				else if (res == 12)
					tmp = "TROLLS";
				else if (res == 13)
					tmp = "BRUTEFORCE";
				else
					tmp = "UNKNOWN";
				ircsprintf(motivo, "Posible proxy %s ilegal", tmp);
			}
			else
				strcpy(motivo, "Posible proxy ilegal");
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", host, proxyserv->tiempo, motivo);
			return;
		}
	}
	for (px = proxys; px; px = px->sig)
	{
		if (!strcmp(px->host, host))
			return;
	}
	if (proxyserv->puerto)
	{
		px = BMalloc(Proxy);
		px->puerto = proxyserv->puerto;
		px->host = strdup(host);
		AddItem(px, proxys);
		while (!(px->sck = SockOpenEx(host, px->puerto->puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ)))
		{
			if (!(px->puerto = px->puerto->sig))
			{
				PSProxOK(px);
				break;
			}
		}
	}
}
int PSSigSQL()
{
	SQLNuevaTabla(XS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"KEY item (item) "
		");", PREFIJO, XS_SQL);
	return 0;
}
int PSSigEOS()
{
	FILE *fp;
	char tmp[BUFSIZE], *c;
	if ((fp = fopen(proxyserv->lista, "r")))
	{
		while ((fgets(tmp, sizeof(tmp), fp)))
		{
			if ((c = strchr(tmp, '\n')))
				*c = '\0';
			if ((c = strchr(tmp, '\r')))
				*c = '\0';
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", tmp, 0, "Posible proxy ilegal");
		}
		fclose(fp);
	}
	return 0;
}
int PSSigSynch()
{
	if (!strcmp(proxyserv->scan_ip, "127.0.0.1"))
		ircstrdup(proxyserv->scan_ip, me.ip);
	return 0;
}
Proxy *BuscaProxy(Sock *sck)
{
	Proxy *px;
	for (px = proxys; px; px = px->sig)
	{
		if (px->sck == sck)
			return px;
	}
	return NULL;
}
SOCKFUNC(PSAbre)
{
	Proxy *px;
	int tipo;;
	if ((px = BuscaProxy(sck)))
	{
		tipo = px->puerto->tipo;
		px->bytes = 0;
		if (tipo & XS_T_ABIERTO)
		{
			px->prox = 1;
			SockClose(px->sck);
		}
		else if (tipo & XS_T_HTTP)
		{
			SockWrite(sck, "CONNECT %s:%i HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "");
		}
		else if (tipo & XS_T_SOCKS4)
		{
			u_long laddr;
			laddr = htonl(inet_addr(proxyserv->scan_ip));
			sprintf(buf, "%c%c%c%c%c%c%c%c%c",
				4, 1,
				(((u_short)proxyserv->scan_puerto)>>8) & 0xFF,
				((u_short)proxyserv->scan_puerto) & 0xFF,
				(char) (laddr >> 24) & 0xFF, (char) (laddr >> 16) & 0xFF,
				(char) (laddr >> 8) & 0xFF, (char) laddr & 0xFF, 0);
			send(sck->pres, buf, 8, 0);
		}
		else if (tipo & XS_T_SOCKS5)
		{
			u_long laddr;
			laddr = htonl(inet_addr(proxyserv->scan_ip));
			send(sck->pres, "\5\1\0", 3, 0);
			sprintf(buf, "%c%c%c%c%c%c%c%c%c%c",
				5, 1, 0, 1,
				(char) (laddr >> 24) & 0xFF, (char) (laddr >> 16) & 0xFF,
              		(char) (laddr >> 8) & 0xFF, (char) laddr & 0xFF,
              		(((u_short) proxyserv->scan_puerto) >> 8) & 0xFF,
              		(((u_short) proxyserv->scan_puerto) & 0xFF));
			send(sck->pres, buf, 10, 0);
		}
		else if (tipo & XS_T_ROUTER)
		{
			SockWrite(sck, "cisco");
			SockWrite(sck, "telnet %s %i", proxyserv->scan_ip, proxyserv->scan_puerto);
		}
		else if (tipo & XS_T_WINGATE)
			SockWrite(sck, "%s:%i", proxyserv->scan_ip, proxyserv->scan_puerto);
		else if (tipo & XS_T_POST)
		{
			SockWrite(sck, "POST http://%s:%d/ HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "Content-type: text/plain");
			SockWrite(sck, "Content-length: 5");
			SockWrite(sck, "");
			SockWrite(sck, "quit");
		}
		else if (tipo & XS_T_GET)
		{
			SockWrite(sck, "GET http://colossus.redyc.com/chkproxy.txt HTTP/1.0");
			SockWrite(sck, "");
		}
	}
	return 0;
}
SOCKFUNC(PSLee)
{
	Proxy *px;
	if ((px = BuscaProxy(sck)))
	{
		int i;
		px->bytes += strlen(data);
		if (px->bytes > 1024)
		{
			SockClose(sck);
			return 1;
		}
		for (i = 0; proxyserv->patron[i]; i++)
		{
			if (strstr(data, proxyserv->patron[i]))
			{
				px->prox = 1;
				SockClose(sck);
				break;
			}
		}
	}
	return 0;
}
void PSProxOK(Proxy *px)
{
	InsertaCache(CACHE_PROXY, px->host, 86400, proxyserv->hmod->id, px->host);
	BorraItem(px, proxys);
	Free(px->host);
	Free(px);
}
SOCKFUNC(PSFin)
{
	Proxy *px;
	if ((px = BuscaProxy(sck)))
	{
		if (!px->prox)
		{
			if (!(px->puerto = px->puerto->sig))
				PSProxOK(px);
			else
			{
				if (!sck->recibido)
				{
					int po = px->puerto->puerto;
					while (px->puerto->puerto == po)
					{
						if (!(px->puerto = px->puerto->sig))
						{
							PSProxOK(px);
							break;
						}
					}
				}
				if (px->puerto)
				{
					while (!(px->sck = SockOpenEx(sck->host, px->puerto->puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ)))
					{
						if (!(px->puerto = px->puerto->sig))
						{
							PSProxOK(px);
							break;
						}
					}
				}
			}
		}
		else
		{
			char motivo[BUFSIZE];
			if (proxyserv->detalles)
			{
				char *o = BuscaOptItem(px->puerto->tipo, TiposProxy);
				ircsprintf(motivo, "Posible proxy %s ilegal (%i)", o ? o : "", px->puerto->puerto);
			}
			else
				strlcpy(motivo, "Posible proxy ilegal", sizeof(motivo));
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", px->host, proxyserv->tiempo, motivo);
		}
	}
	return 0;
}
