/*
 * $Id: operserv.h,v 1.2 2004-12-31 12:27:56 Trocotronic Exp $ 
 */

#define OS_OPTS_AOP 0x1

typedef struct _os OperServ;
struct _os 
{
	int opts;
	Modulo *hmod;
};

extern OperServ operserv;

#define OS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define OS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define OS_ERR_EMPT "\00304ERROR: %s"
#define OS_ERR_FORB ERR_FORB

#ifndef UDB
#define OS_MYSQL "opers"
#endif
#define OS_NOTICIAS "noticias"
typedef struct _not Noticia;
struct _not
{
	char *botname;
	char *noticia;
	time_t fecha;
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
