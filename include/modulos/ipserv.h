/*
 * $Id: ipserv.h,v 1.5 2006-04-17 14:19:44 Trocotronic Exp $ 
 */

typedef struct _is IpServ;
struct _is
{
	int clones;
	char *sufijo;
	int cambio;
	Modulo *hmod;
};

#define IS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define IS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define IS_ERR_EMPT "\00304ERROR: %s"

#define IS_SQL "ips"
#define IS_CLONS "clons"

extern IpServ *ipserv;

#define IS_SIGN_DROP 30
