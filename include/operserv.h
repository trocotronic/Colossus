#define OS_MAX_COMS 256

#define OS_OPTS_AOP 0x1

typedef struct _os OperServ;
struct _os 
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
#ifndef UDB
	int clones;
#endif
	char *residente;
	bCom *comando[OS_MAX_COMS]; /* comandos soportados */
	int comandos;
	char *bline;
};

extern OperServ *operserv;

#define OS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define OS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define OS_ERR_EMPT "\00304ERROR: %s"
#define OS_ERR_FORB ERR_FORB

#ifndef UDB
#define OS_MYSQL "opers"
#define OS_CLONS "clons"
#endif
#define OS_NOTICIAS "noticias"
typedef struct _not Noticia;
struct _not
{
	char *botname;
	char *noticia;
	time_t fecha;
	int id;
};
#define MAXNOT 32

#ifndef UDB
#define BDD_PREO 0x1
#define BDD_OPER 0x2
#define BDD_DEVEL 0x4
#define BDD_ADMIN 0x8
#define BDD_ROOT 0x10
#endif
