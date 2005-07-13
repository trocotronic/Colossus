/*
 * $Id: ipserv.h,v 1.3 2005-07-13 14:06:24 Trocotronic Exp $ 
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

#define IS_SQL "ips"
#ifndef UDB
#define IS_CLONS "clons"
#endif

extern IpServ ipserv;
