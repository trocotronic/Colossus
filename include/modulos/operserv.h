/*
 * $Id: operserv.h,v 1.8 2006-04-17 14:19:44 Trocotronic Exp $ 
 */

#define OS_OPTS_AOP 0x1

typedef struct _os OperServ;
struct _os 
{
	int opts;
	Modulo *hmod;
	u_int maxlist;
};

extern OperServ *operserv;

#define OS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define OS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define OS_ERR_EMPT "\00304ERROR: %s"
#define OS_ERR_FORB ERR_FORB

#define OS_SQL "opers"
#define OS_NOTICIAS "noticias"
#define OS_AKILL "akill"
typedef struct _not Noticia;
struct _not
{
	char *botname;
	char *noticia;
	char fecha[11];
	int id;
	Cliente *cl;
};
#define MAXNOT 32
