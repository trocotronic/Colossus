/*
 * $Id: gameserv.h,v 1.1 2006-11-01 11:38:26 Trocotronic Exp $ 
 */

typedef struct _gs GameServ;
struct _gs
{
	Modulo *hmod;
};

#define GS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define GS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define GS_ERR_EMPT "\00304ERROR: %s"
