/*
 * $Id: ipserv.h,v 1.2 2004-12-31 12:27:55 Trocotronic Exp $ 
 */

typedef struct _is IpServ;
struct _is
{
#ifndef UDB
	int clones;
#endif
	char *sufijo;
	int cambio;
	Modulo *hmod;
};

#define IS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define IS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define IS_ERR_EMPT "\00304ERROR: %s"

#define IS_MYSQL "ips"
#ifndef UDB
#define IS_CLONS "clons"
#endif

extern IpServ ipserv;
