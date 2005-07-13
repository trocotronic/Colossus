/*
 * $Id: operserv.h,v 1.5 2005-07-13 14:06:25 Trocotronic Exp $ 
 */

#define OS_OPTS_AOP 0x1

typedef struct _os OperServ;
struct _os 
{
	int opts;
	Modulo *hmod;
	u_int maxlist;
};

extern OperServ operserv;

#define OS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define OS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define OS_ERR_EMPT "\00304ERROR: %s"
#define OS_ERR_FORB ERR_FORB

#ifndef UDB
#define OS_SQL "opers"
#endif
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

#ifndef UDB
#define BDD_PREO 0x1
#define BDD_OPER 0x2
#define BDD_DEVEL 0x4
#define BDD_ADMIN 0x8
#define BDD_ROOT 0x10
#endif
