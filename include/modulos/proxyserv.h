/*
 * $Id: proxyserv.h,v 1.3 2005-02-18 22:12:15 Trocotronic Exp $ 
 */

#define XS_MAX_PORTS 32

typedef struct _xs ProxyServ;
struct _xs
{
	int puerto[XS_MAX_PORTS]; /* puertos a escanear */
	int puertos;
	int tiempo;
	int maxlist;
	Modulo *hmod;
};

extern ProxyServ proxyserv;

#define XS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define XS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define XS_ERR_EMPT "\00304ERROR: %s"

typedef struct _proxys Proxys;
struct _proxys
{
	char *host;
	int escaneados;
	struct subp
	{
		Sock *sck;
		int puerto;
	}*puerto[XS_MAX_PORTS];
	int puertos;
	struct _proxys *sig;
};

#define XS_MYSQL "ehosts"
#define CACHE_PROXY "proxy"
