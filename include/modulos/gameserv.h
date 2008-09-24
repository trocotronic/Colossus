/*
 * $Id: gameserv.h,v 1.2 2007/08/20 01:46:25 Trocotronic Exp $ 
 */

typedef struct _gs GameServ;
struct _gs
{
	Modulo *hmod;
};

extern GameServ *gameserv;

#define GS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define GS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define GS_ERR_EMPT "\00304ERROR: %s"
