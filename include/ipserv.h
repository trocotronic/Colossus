/*
 * $Id: ipserv.h,v 1.2 2004-09-11 15:54:07 Trocotronic Exp $ 
 */

#define IS_MAX_COMS 256

typedef struct _is IpServ;
struct _is
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	char *residente;
	bCom *comando[IS_MAX_COMS]; /* comandos soportados */
	int comandos;
	char *sufijo;
	int cambio;
};

#define IS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define IS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define IS_ERR_EMPT "\00304ERROR: %s"

#define IS_MYSQL "ips"

extern IpServ *ipserv;
