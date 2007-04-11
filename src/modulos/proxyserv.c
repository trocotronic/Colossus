/*
 * $Id: proxyserv.c,v 1.39 2007-04-11 20:13:13 Trocotronic Exp $ 
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
	{ "host" , PSHost , N4 , "Hosts que omiten el esc�ner." , PSHHost } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void PSSet(Conf *, Modulo *);
int PSTest(Conf *, int *);

ModInfo MOD_INFO(ProxyServ) = {
	"ProxyServ" ,
	0.11 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};
	
int MOD_CARGA(ProxyServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El m�dulo ha sido compilado para la versi�n %i y usas la versi�n %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(ProxyServ).nombre);
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
					Error("[%s] La configuraci�n de %s no ha pasado el PSTest", mod->archivo, MOD_INFO(ProxyServ).nombre);
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
	PPuerto *ppx;
	int i;
	for (px = proxys; px; px = px->sig)
	{
		for (ppx = px->puerto; ppx; ppx = ppx->sig)
				SockClose(ppx->sck, LOCAL);
	}
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
						Error("[%s:%s::%s::%i] No se reconoce el m�todo proxy %s", config->archivo, config->item, eval->item, eval->seccion[i]->linea, tok);
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
		Error("[%s:%s] No has especificado la ip del esc�ner", config->archivo, config->item);
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
		Error("[%s:%s] No has especificado el puerto del esc�ner", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int port;
		if (!eval->data || (port = atoi(eval->data)) <= 0 || port > 65535)
		{
			Error("[%s:%s::%s::%i] La directriz scan_puerto debe ser un puerto v�lido", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "patron")))
	{
		Error("[%s:%s] No has especificado ning�n patr�n para el esc�ner", config->archivo, config->item);
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
					ppt = BMalloc(struct _puerto);
					ppt->puerto = atoi(config->seccion[i]->seccion[p]->item);
					ppt->tipo = XS_T_ABIERTO;
					if (config->seccion[i]->seccion[p]->data)
					{
						char *tok;
						ppt->tipo = 0;
						strlcpy(tokbuf, config->seccion[i]->seccion[p]->data, sizeof(tokbuf));
						for (tok = strtok(tokbuf, ","); tok; tok = strtok(NULL, ","))
							ppt->tipo |= BuscaOpt(tok, TiposProxy);
					}
					AddItem(ppt, proxyserv->puerto);
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
			else if (!strcmp(config->seccion[i]->item, "forzar_salida"))
				proxyserv->fsal = 1;
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
	Responde(cl, CLI(proxyserv), "Gestiona los hosts que omiten el esc�n.");
	Responde(cl, CLI(proxyserv), "Estos hosts no se someter�n al esc�n de puertos y podr�n entrar libremente.");
	Responde(cl, CLI(proxyserv), "Cabe mencionar que si el usuario conecta sin su DNS resuelto, figurar� su ip en vez de su host.");
	Responde(cl, CLI(proxyserv), "Por este motivo, es importante a�adir el host o la ip, seg�n conecte el usuario.");
	Responde(cl, CLI(proxyserv), "Si no se especifica ni + ni -, se usar� el patr�n como b�squeda.");
	Responde(cl, CLI(proxyserv), " ");
	Responde(cl, CLI(proxyserv), "Sintaxis: \00312HOST {+|-|patr�n}[{host|ip}]");
	return 0;
}
BOTFUNC(PSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), "\00312%s\003 controla las conexiones entrantes para detectar si se tratan de proxies.", proxyserv->hmod->nick);
		Responde(cl, CLI(proxyserv), "Para ello efectua un esc�n de puertos en la m�quina. Si detecta alguno que est� abierto desconecta al usuario.");
		Responde(cl, CLI(proxyserv), "En dicha desconexi�n figuran los puertos abiertos.");
		Responde(cl, CLI(proxyserv), "El usuario podr� conectar de nuevo si cierra estos puertos.");
		Responde(cl, CLI(proxyserv), "El tiempo de desconexi�n es de \00312%i\003 minutos.", proxyserv->tiempo / 60);
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Comandos disponibles:");
		ListaDescrips(proxyserv->hmod, cl);
		Responde(cl, CLI(proxyserv), " ");
		Responde(cl, CLI(proxyserv), "Para m�s informaci�n, \00312/msg %s %s comando", proxyserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], proxyserv->hmod, param, params))
		Responde(cl, CLI(proxyserv), XS_ERR_EMPT, "Opci�n desconocida.");
	return 0;
}
BOTFUNC(PSHost)
{
	if (params < 2)
	{
		Responde(cl, CLI(proxyserv), XS_ERR_PARA, fc->com, "{+|-|patr�n} [host|ip]");
		return 1;
	}
	if (*param[1] == '+')
	{
		SQLQuery("INSERT INTO %s%s (item) values ('%s')", PREFIJO, XS_SQL, param[1] + 1);
		Responde(cl, CLI(proxyserv), "El host \00312%s\003 se ha a�adido para omitir el escaneo de puertos.", param[1] + 1);
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
		Responde(cl, CLI(proxyserv), "*** Hosts que coinciden con el patr�n \00312%s\003 ***", param[1]);
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
		ProtFunc(P_NOTICE)(cl, CLI(proxyserv), "Se va a proceder a hacer un esc�ner de puertos a tu m�quina para verificar que no se trata de un proxy.");
		PSEscanea(host);
	}
	return 0;
}
void PSEscanea(char *host)
{
	Proxy *px;
	PPuerto *ppx;
	struct _puerto *ppt;
	if (!strncmp("192.168", host, 7) || !strcmp("127.0.0.1", host))
		return;
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
					strlcat(tmp, ",WINGATE", sizeof(tmp));
				if (res & 0x2)
					strlcat(tmp, ",SOCKSx", sizeof(tmp));
				if (res & 0x4)
					strlcat(tmp, ",HTTP", sizeof(tmp));
				if (res & 0x8)
					strlcat(tmp, ",ROUTER", sizeof(tmp));
				if (res & 0x10)
					strlcat(tmp, ",POST", sizeof(tmp));
				if (res & 0x20)
					strlcat(tmp, ",GET", sizeof(tmp));
			}
			ircsprintf(motivo, "Posible PROXY %s ilegal", !BadPtr(tmp) ? &tmp[1] : "");
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", host, proxyserv->tiempo, motivo);
			return;
		}
	}
	for (px = proxys; px; px = px->sig)
	{
		if (!strcmp(px->host, host))
			return;
	}
	px = BMalloc(Proxy);
	for (ppt = proxyserv->puerto; ppt; ppt = ppt->sig)
	{
		ppx = BMalloc(PPuerto);
		if ((ppx->sck = SockOpenEx(host, ppt->puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ)))
		{
			ppx->puerto = ppt->puerto;
			ppx->tipo = ppt->tipo;
			px->puertos++;
			AddItem(ppx, px->puerto);
		}
		else
			ircfree(ppx);
	}
	px->host = strdup(host);
	AddItem(px, proxys);
}
int PSSigSQL()
{
	if (!SQLEsTabla(XS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"KEY item (item) "
			");", PREFIJO, XS_SQL);
		if (sql->_errno)
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
int PSSigSynch()
{
	if (!strcmp(proxyserv->scan_ip, "127.0.0.1"))
		ircstrdup(proxyserv->scan_ip, me.ip);
	return 0;
}
void CompruebaProxy(Proxy *px, int forzar)
{
	int i, tipo = 0;
	char psbuf[128], ptbuf[128];
	PPuerto *ppx, *tmp;
	if ((proxyserv->fsal && forzar) || px->escaneados == px->puertos)
	{
		psbuf[0] = ptbuf[0] = '\0';
		for (ppx = px->puerto; ppx; ppx = tmp)
		{
			tmp = ppx->sig;
			if (ppx->proxy)
			{
				switch (ppx->proxy)
				{
					case XS_T_HTTP:
						if (!(tipo & XS_T_HTTP))
						{
							strlcat(psbuf, ",HTTP", sizeof(psbuf));
							tipo |= XS_T_HTTP;
						}
						break;
					case XS_T_SOCKS4:
						if (!(tipo & XS_T_SOCKS4))
						{
							strlcat(psbuf, ",SOCKS4", sizeof(psbuf));
							tipo |= XS_T_SOCKS4;
						}
						break;
					case XS_T_SOCKS5:
						if (!(tipo & XS_T_SOCKS5))
						{
							strlcat(psbuf, ",SOCKS5", sizeof(psbuf));
							tipo |= XS_T_SOCKS5;
						}
						break;
					case XS_T_ROUTER:
						if (!(tipo & XS_T_ROUTER))
						{
							strlcat(psbuf, ",ROUTER", sizeof(psbuf));
							tipo |= XS_T_ROUTER;
						}
						break;
					case XS_T_WINGATE:
						if (!(tipo & XS_T_WINGATE))
						{
							strlcat(psbuf, ",WINGATE", sizeof(psbuf));
							tipo |= XS_T_WINGATE;
						}
						break;
					case XS_T_POST:
						if (!(tipo & XS_T_POST))
						{
							strlcat(psbuf, ",POST", sizeof(psbuf));
							tipo |= XS_T_POST;
						}
						break;
					case XS_T_GET:
						if (!(tipo & XS_T_GET))
						{
							strlcat(psbuf, ",GET", sizeof(psbuf));
							tipo |= XS_T_GET;
						}
						break;
					default:
						tipo |= XS_T_ABIERTO;
				}
				ircsprintf(buf, ",%i", ppx->puerto);
				strlcat(ptbuf, buf, sizeof(ptbuf));
			}
			Free(ppx);
		}
		if (tipo)
		{
			char motivo[BUFSIZE];
			if (proxyserv->detalles)
				ircsprintf(motivo, "Posible PROXY %s ilegal (%s)", !BadPtr(psbuf) ? &psbuf[1]: "", &ptbuf[1]);
			else
				strlcpy(motivo, "Posible PROXY ilegal", sizeof(motivo));
			ProtFunc(P_GLINE)(CLI(proxyserv), ADD, "*", px->host, proxyserv->tiempo, motivo);
		}
		//else
		//	InsertaCache(CACHE_PROXY, px->host, 86400, proxyserv->hmod->id, px->host);
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
	PPuerto *ppx;
	if ((px = BuscaProxy(sck)))
	{
		for (ppx = px->puerto; px; ppx = ppx->sig)
		{
			if (ppx->puerto == sck->puerto)
				return ppx;
		}
	}
	return NULL;
}
SOCKFUNC(PSAbre)
{
	PPuerto *ppx;
	if ((ppx = BuscaPPuerto(sck)))
	{
		ppx->abierto = 1;
		if (ppx->tipo & XS_T_ABIERTO)
		{
			ppx->proxy = XS_T_ABIERTO;
			SockClose(ppx->sck, LOCAL);
		}
		else if (ppx->tipo & XS_T_HTTP)
		{
			SockWrite(sck, "CONNECT %s:%i HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "");
		}
		else if (ppx->tipo & XS_T_SOCKS4)
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
		else if (ppx->tipo & XS_T_SOCKS5)
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
		else if (ppx->tipo & XS_T_ROUTER)
		{
			SockWrite(sck, "cisco");
			SockWrite(sck, "telnet %s %i", proxyserv->scan_ip, proxyserv->scan_puerto);
		}
		else if (ppx->tipo & XS_T_WINGATE)
			SockWrite(sck, "%s:%i", proxyserv->scan_ip, proxyserv->scan_puerto);
		else if (ppx->tipo & XS_T_POST)
		{
			SockWrite(sck, "POST http://%s:%d/ HTTP/1.0", proxyserv->scan_ip, proxyserv->scan_puerto);
			SockWrite(sck, "Content-type: text/plain");
			SockWrite(sck, "Content-length: 5");
			SockWrite(sck, "");
			SockWrite(sck, "quit");
		}
		else if (ppx->tipo & XS_T_GET)
		{
			SockWrite(sck, "GET http://colossus.redyc.com/chkproxy.txt HTTP/1.0");
			SockWrite(sck, "");
		}
	}	
	return 0;
}
SOCKFUNC(PSLee)
{
	PPuerto *ppx;
	if ((ppx = BuscaPPuerto(sck)))
	{
		int i;
		ppx->bytes += len;
		if (ppx->bytes > 1024)
		{
			SockClose(sck, LOCAL);
			return 1;
		}
		for (i = 0; proxyserv->patron[i]; i++)
		{
			if (strstr(data, proxyserv->patron[i]))
			{
				if (ppx->tipo & XS_T_HTTP)
					ppx->proxy = XS_T_HTTP;
				else if (ppx->tipo & XS_T_SOCKS4)
					ppx->proxy = XS_T_SOCKS4;
				else if (ppx->tipo & XS_T_SOCKS5)
					ppx->proxy = XS_T_SOCKS5;
				else if (ppx->tipo & XS_T_ROUTER)
					ppx->proxy = XS_T_ROUTER;
				else if (ppx->tipo & XS_T_WINGATE)
					ppx->proxy = XS_T_WINGATE;
				else if (ppx->tipo & XS_T_POST)
					ppx->proxy = XS_T_POST;
				else if (ppx->tipo & XS_T_GET)
					ppx->proxy = XS_T_GET;
				SockClose(sck, LOCAL);
				break;
			}
		}
	}
	return 0;
}
SOCKFUNC(PSFin)
{
	PPuerto *ppx;
	Proxy *px;
	if ((px = BuscaProxy(sck)) && (ppx = BuscaPPuerto(sck)))
	{
		if (ppx->tipo & XS_T_ABIERTO)
			ppx->tipo &= ~XS_T_ABIERTO;
		else if (ppx->tipo & XS_T_HTTP)
			ppx->tipo &= ~XS_T_HTTP;
		else if (ppx->tipo & XS_T_SOCKS4)
			ppx->tipo &= ~XS_T_SOCKS4;
		else if (ppx->tipo & XS_T_SOCKS5)
			ppx->tipo &= ~XS_T_SOCKS5;
		else if (ppx->tipo & XS_T_ROUTER)
			ppx->tipo &= ~XS_T_ROUTER;
		else if (ppx->tipo & XS_T_WINGATE)
			ppx->tipo &= ~XS_T_WINGATE;
		else if (ppx->tipo & XS_T_POST)
			ppx->tipo &= ~XS_T_POST;
		else if (ppx->tipo & XS_T_GET)
			ppx->tipo &= ~XS_T_GET;
		if (!ppx->tipo || ppx->proxy || !ppx->abierto)
		{
			px->escaneados++;
			ppx->sck = NULL;
			CompruebaProxy(px, ppx->proxy);
		}
		else
			ppx->sck = SockOpenEx(sck->host, ppx->puerto, PSAbre, PSLee, NULL, PSFin, 30, 30, OPT_NORECVQ);
	}
	return 0;
}
