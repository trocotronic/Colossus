/*
 * $Id: tvserv.h,v 1.3 2006-04-17 14:19:44 Trocotronic Exp $ 
 */

#define XS_MAX_PORTS 32

typedef struct _ts TvServ;
struct _ts
{
	Modulo *hmod;
};

#define TS_ERR_PARA "\00304ERROR: Faltan par�metros: %s %s "
#define TS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define TS_ERR_EMPT "\00304ERROR: %s"

typedef struct _datasock
{
	char *query;
	int numero;
	Cliente *cl;
	Sock *sck;
}DataSock;

typedef struct _cadena
{
	char *nombre;
	char *texto;
	char tipo;
}Cadena;

#define TS_TV "tv"
#define TS_HO "horoscopo"
#define CACHE_TIEMPO "eltiempo"
#define TS_LI "liga"
#define MAX_COLA 16
