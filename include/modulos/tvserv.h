/*
 * $Id: tvserv.h,v 1.1 2005-06-29 21:13:49 Trocotronic Exp $ 
 */

#define XS_MAX_PORTS 32

typedef struct _ts TvServ;
struct _ts
{
	Modulo *hmod;
};

#define TS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define TS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define TS_ERR_EMPT "\00304ERROR: %s"

typedef struct _cadena
{
	char *nombre;
	char *texto;
	char tipo;
}Cadena;

#define TS_TV "tv"
#define TS_HO "horoscopo"
