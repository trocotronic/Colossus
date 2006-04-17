/*
 * $Id: proxyserv.c,v 1.22 2006-04-17 14:19:45 Trocotronic Exp $ 
 */

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
SOCKFUNC(PSAbre);
SOCKFUNC(PSLee);
SOCKFUNC(PSFin);
Proxy *proxys = NULL;

int PSCmdNick(Cliente *, int);

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
	{ "host" , PSHost , N4 , "Host que omiten el escáner." , PSHHost } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
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
	mod->activo = 1;
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
	int i;
	for (px = proxys; px; px = px->sig)
	{
		for (i = 0; i < proxyserv->puertos; i++)
			SockClose(px->puerto[i]->sck, LOCAL);
	}
	BorraSenyal(SIGN_POST_NICK, PSCmdNick);
	BorraSenyal(SIGN_SQL, PSSigSQL);
	BorraSenyal(SIGN_EOS, PSSigEOS);
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
				strncpy(tokbuf, eval->seccion[i]->data, sizeof(tokbuf));
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
	if (!proxyserv)
		BMalloc(proxyserv, ProxyServ);
	proxyserv->puertos = 0;
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
					proxyserv->puerto[proxyserv->puertos].puerto = atoi(config->seccion[i]->seccion[p]->item);
					proxyserv->puerto[proxyserv->puertos].tipo = XS_T_ABIERTO;
					if (config->seccion[i]->seccion[p]->data)
					{
						char *tok;
						proxyserv->puerto[proxyserv->puertos].tipo = 0;
						strncpy(tokbuf, config->seccion[i]->seccion[p]->data, sizeof(tokbuf));
						for (tok = strtok(tokbuf, ","); tok; tok = strtok(NULL, ","))
							proxyserv->puerto[proxyserv->puertos].tipo |= BuscaOpt(tok, TiposProxy);
					}
					proxyserv->puertos++;
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
		}
	}
	else
		ProcesaComsMod(NULL, mod, proxyserv_coms);
	proxyserv->patron[q] = NULL;
	InsertaSenyal(SIGN_POST_NICK, PSCmdNick);
	InsertaSenyal(SIGN_SQL, PSSigSQL);
	InsertaSenyal(SIGN_EOS, PSSigEOS);
	BotSet(proxyserv);
}
void PSEscanea(char *host)
{
	Proxy *px;
	int i;
	if (!strncmp("192.168", host, 7) || !strcmp("127.0.0.1", host))
	{
		Free(host);
		return;
	}
	if (proxyserv->opm && EsIp(host))
	{
		u_int tmp1, tmp2, tmp3, tmp4;
		struct hostent *he;
		sscanf(host, "%u.%u.%u.%u", &tmp1, &tmp2, &tmp3, &tmp4);
		ircsprintf(buf, "%u.%u.%u.%u.opm.blitzed.org", tmp4, tmp3, tmp2, tmp1);
		if ((he = gethostbyname(buf))) 
		{
			char tmp[128], motivo[256];
			tmp[0] = '\0';
			if (proxyserv->detalles)
			{
				int res = ntohl((*(struct in_addr *)he->h_addr).s_addr)&0xFF;
				if (res & 0x1)
					strcat(tmp, ",WINGATE");
				if (res & 0x2)
					strcat(tmp, ",SOCKSx");
				if (res & 0x4)
					strcat(tmp, ",HTTP");
				if (res & 0x8)
					strcat(tmp, ",ROUTER");
				if (res & 0x10)
					strcat(tmp, ",POST");
				if (res & 0x20)
					strcat(tmp, ",GET");
			}
			ircsprintf(motivo, "Posible PROXY %s ilegal", !BadPtr(tmp) ? &tmp[1] : "");
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", host, proxyserv->tiempo, motivo);
			return;
		}
	}
	BMalloc(px, Proxy);
	for (i = 0; i < proxyserv->puertos; i++)
	{
		BMalloc(px->puerto[i], PPuerto);
		if ((px->puerto[i]->sck = SockOpenEx(host, proxyserv->puerto[i].puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ)))
		{
			px->puerto[i]->puerto = proxyserv->puerto[i].puerto;
			px->puerto[i]->tipo = proxyserv->puerto[i].tipo;
			px->puertos++;
		}
	}
	px->host = host; /* ya es un strdup */
	AddItem(px, proxys);
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
		if (!BadPtr(cl->ip) && *(cl->ip) != '*')
			host = strdup(cl->ip);
		else
			host = strdup(cl->host);
		if (CogeCache(CACHE_PROXY, host, proxyserv->hmod->id) || SQLCogeRegistro(XS_SQL, host, NULL))
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
	return 1;
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
void CompruebaProxy(Proxy *px)
{
	int i;
	int tipo = 0;
	char psbuf[128], ptbuf[128];
	if (px->escaneados == proxyserv->puertos)
	{
		psbuf[0] = ptbuf[0] = '\0';
		for (i = 0; i < proxyserv->puertos; i++)
		{
			if (px->puerto[i]->proxy)
			{
				switch (px->puerto[i]->proxy)
				{
					case XS_T_HTTP:
						if (!(tipo & XS_T_HTTP))
						{
							strcat(psbuf, ",HTTP");
							tipo |= XS_T_HTTP;
						}
						break;
					case XS_T_SOCKS4:
						if (!(tipo & XS_T_SOCKS4))
						{
							strcat(psbuf, ",SOCKS4");
							tipo |= XS_T_SOCKS4;
						}
						break;
					case XS_T_SOCKS5:
						if (!(tipo & XS_T_SOCKS5))
						{
							strcat(psbuf, ",SOCKS5");
							tipo |= XS_T_SOCKS5;
						}
						break;
					case XS_T_ROUTER:
						if (!(tipo & XS_T_ROUTER))
						{
							strcat(psbuf, ",ROUTER");
							tipo |= XS_T_ROUTER;
						}
						break;
					case XS_T_WINGATE:
						if (!(tipo & XS_T_WINGATE))
						{
							strcat(psbuf, ",WINGATE");
							tipo |= XS_T_WINGATE;
						}
						break;
					case XS_T_POST:
						if (!(tipo & XS_T_POST))
						{
							strcat(psbuf, ",POST");
							tipo |= XS_T_POST;
						}
						break;
					case XS_T_GET:
						if (!(tipo & XS_T_GET))
						{
							strcat(psbuf, ",GET");
							tipo |= XS_T_GET;
						}
						break;
					default:
						tipo |= XS_T_ABIERTO;
				}
				ircsprintf(buf, ",%i", px->puerto[i]->puerto);
				strcat(ptbuf, buf);
			}
			Free(px->puerto[i]);
		}
		if (tipo)
		{
			char motivo[300];
			if (proxyserv->detalles)
				ircsprintf(motivo, "Posible PROXY %s ilegal (%s)", !BadPtr(psbuf) ? &psbuf[1]: "", &ptbuf[1]);
			else
				strcpy(motivo, "Posible PROXY ilegal");
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", px->host, proxyserv->tiempo, motivo);
		}
		else
			InsertaCache(CACHE_PROXY, px->host, 86400, proxyserv->hmod->id, px->host);
		BorraItem(px, proxys);
		Free(px->host);
		Free(px);
	}
}
Proxy *BuscaProxy(Sock *sck)
{
	Proxy *px;
	for (px = proxys; px; px = px->sig)
	{
		if (!strcmp(px->host, sck->host))
			return px;
	}
	return NULL;
}
PPuerto *BuscaPPuerto(Sock *sck)
{
	Proxy *px;
	if ((px = BuscaProxy(sck)))
	{
		int i;
		for (i = 0; i < px->puertos; i++)
		{
			if (px->puerto[i]->puerto == sck->puerto)
				return px->puerto[i];
		}
	}
	return NULL;
}
SOCKFUNC(PSAbre)
{
	PPuerto *ppt;
	if ((ppt = BuscaPPuerto(sck)))
	{
		if (ppt->tipo & XS_T_ABIERTO)
		{
			ppt->proxy = XS_T_ABIERTO;
			SockClose(ppt->sck, LOCAL);
		}
		else if (ppt->tipo & XS_T_HTTP)
		{
			SockWrite(sck, "CONNECT %s:%i HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "");
		}
		else if (ppt->tipo & XS_T_SOCKS4)
		{
			u_long laddr;
			laddr = htonl(inet_addr(proxyserv->scan_ip));
			sprintf(buf, "%c%c%c%c%c%c%c%c%c", 
				4, 1,
				(((u_short)proxyserv->scan_puerto)>>8) & 0xFF,
				((u_short)proxyserv->scan_puerto) & 0xFF,
				(char) (laddr >> 24) & 0xFF, (char) (laddr >> 16) & 0xFF,
              		(char) (laddr >> 8) & 0xFF, (char) laddr & 0xFF, 0);
              	send(sck->pres, buf, 9, 0);
              }
              else if (ppt->tipo & XS_T_SOCKS5)
              {
			u_long laddr;
			laddr = htonl(inet_addr(proxyserv->scan_ip));
			send(sck->pres, "\5\1\0", 3, 0);
			sprintf(buf, "%c%c%c%c%c%c%c%c%c%c",
				5, 1, 0, 1,
				(char) (laddr >> 24) & 0xFF, (char) (laddr >> 16) & 0xFF,
              		(char) (laddr >> 8) & 0xFF, (char) laddr & 0xFF,
              		(((u_short) proxyserv->puerto) >> 8) & 0xFF,
              		(((u_short) proxyserv->puerto) & 0xFF));
				send(sck->pres, buf, 10, 0);
		}
		else if (ppt->tipo & XS_T_ROUTER)
		{
			SockWrite(sck, "cisco");
			SockWrite(sck, "telnet %s %i", proxyserv->scan_ip, proxyserv->scan_puerto);
		}
		else if (ppt->tipo & XS_T_WINGATE)
			SockWrite(sck, "%s:%i", proxyserv->scan_ip, proxyserv->scan_puerto);
		else if (ppt->tipo & XS_T_POST)
		{
			SockWrite(sck, "POST http://%s:%d/ HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "Content-type: text/plain");
			SockWrite(sck, "Content-length: 5");
			SockWrite(sck, "");
			SockWrite(sck, "quit");
		}
		else if (ppt->tipo & XS_T_GET)
		{
			SockWrite(sck, "GET http://www.rallados.net/chkproxy.txt HTTP/1.0");
			SockWrite(sck, "");
		}
	}	
	return 0;
}
SOCKFUNC(PSLee)
{
	PPuerto *ppt;
	if ((ppt = BuscaPPuerto(sck)))
	{
		int i;
		ppt->bytes += strlen(data);
		if (ppt->bytes > 4096)
		{
			SockClose(sck, LOCAL);
			return 1;
		}
		for (i = 0; proxyserv->patron[i]; i++)
		{
			if (strstr(data, proxyserv->patron[i]))
			{
				if (ppt->tipo & XS_T_HTTP)
					ppt->proxy = XS_T_HTTP;
				else if (ppt->tipo & XS_T_SOCKS4)
					ppt->proxy = XS_T_SOCKS4;
				else if (ppt->tipo & XS_T_SOCKS5)
					ppt->proxy = XS_T_SOCKS5;
				else if (ppt->tipo & XS_T_ROUTER)
					ppt->proxy = XS_T_ROUTER;
				else if (ppt->tipo & XS_T_WINGATE)
					ppt->proxy = XS_T_WINGATE;
				else if (ppt->tipo & XS_T_POST)
					ppt->proxy = XS_T_POST;
				else if (ppt->tipo & XS_T_GET)
					ppt->proxy = XS_T_GET;
				SockClose(sck, LOCAL);
				break;
			}
		}
	}
	return 0;
}
SOCKFUNC(PSFin)
{
	PPuerto *ppt;
	Proxy *px;
	if ((px = BuscaProxy(sck)) && (ppt = BuscaPPuerto(sck)))
	{
		if (ppt->tipo & XS_T_ABIERTO)
			ppt->tipo &= ~XS_T_ABIERTO;
		else if (ppt->tipo & XS_T_HTTP)
			ppt->tipo &= ~XS_T_HTTP;
		else if (ppt->tipo & XS_T_SOCKS4)
			ppt->tipo &= ~XS_T_SOCKS4;
		else if (ppt->tipo & XS_T_SOCKS5)
			ppt->tipo &= ~XS_T_SOCKS5;
		else if (ppt->tipo & XS_T_ROUTER)
			ppt->tipo &= ~XS_T_ROUTER;
		else if (ppt->tipo & XS_T_WINGATE)
			ppt->tipo &= ~XS_T_WINGATE;
		else if (ppt->tipo & XS_T_POST)
			ppt->tipo &= ~XS_T_POST;
		else if (ppt->tipo & XS_T_GET)
			ppt->tipo &= ~XS_T_GET;
		if (!ppt->tipo || ppt->proxy)
		{
			px->escaneados++;
			CompruebaProxy(px);
		}
		else
			ppt->sck = SockOpenEx(sck->host, ppt->puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ);
	}
	return 0;
}
