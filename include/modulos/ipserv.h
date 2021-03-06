/*
 * $Id: ipserv.h,v 1.6 2006/10/31 23:49:10 Trocotronic Exp $
 */

typedef struct _is IpServ;
struct _is
{
	int clones;
	char *sufijo;
	int cambio;
	unsigned pvhost:1;
	Modulo *hmod;
	u_int maxlist;
};

#define IS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define IS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define IS_ERR_EMPT "\00304ERROR: %s"

#define IS_SQL "nicks"
#define IS_CLONS "clons"
#define IS_NOLINE "nolines"
#define IS_DNS "dns"

extern IpServ *ipserv;

#define IS_SIGN_DROP 52
#define SIGN_SQL2 53

#define IS_CACHE_VHOST "vhosts"
