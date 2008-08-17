/*
 * $Id: proxyserv.h,v 1.10 2007/04/11 20:13:13 Trocotronic Exp $ 
 */

#define MAX_PATRON 16

typedef struct _xs ProxyServ;
struct _xs
{
	struct _puerto
	{
		struct _puerto *sig;
		int puerto; /* puertos a escanear */
		int tipo;
	}*puerto;
	int tiempo;
	int maxlist;
	char *scan_ip;
	int scan_puerto;
	char *lista;
	char *patron[MAX_PATRON];
	unsigned detalles:1;
	unsigned opm:1;
	unsigned fsal:1;
	Modulo *hmod;
};

extern ProxyServ *proxyserv;

#define XS_T_ABIERTO 0x1
#define XS_T_HTTP 0x2
#define XS_T_SOCKS4 0x4
#define XS_T_SOCKS5 0x8
#define XS_T_ROUTER 0x10
#define XS_T_WINGATE 0x20
#define XS_T_POST 0x40
#define XS_T_GET 0x80

#define XS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define XS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define XS_ERR_EMPT "\00304ERROR: %s"

typedef struct _proxy Proxy;
typedef struct _pprox PPuerto;
struct _proxy
{
	struct _proxy *sig;
	char *host;
	PPuerto *puerto;
	int escaneados;
	int puertos;
};
struct _pprox
{
	struct _pprox *sig;
	Sock *sck;
	int puerto;
	int tipo;
	int proxy;
	u_int bytes;
	int abierto;
};

#define XS_SQL "ehosts"
#define CACHE_PROXY "proxy"
