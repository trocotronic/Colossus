/*
 * $Id: proxyserv.h,v 1.1 2004-11-05 19:59:39 Trocotronic Exp $ 
 */

#define XS_MAX_COMS 256
#define XS_MAX_PORTS 32

typedef struct _xs ProxyServ;
struct _xs
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	char *residente;
	bCom *comando[XS_MAX_COMS]; /* comandos soportados */
	int comandos;
	int puerto[XS_MAX_PORTS]; /* puertos a escanear */
	int puertos;
	int tiempo;
	int maxlist;
};

extern ProxyServ *proxyserv;

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
		int *puerto;
	}*puerto[XS_MAX_PORTS];
	int puertos;
	struct _proxys *sig, *prev;
};

#define XS_MYSQL "ehosts"
