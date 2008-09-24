/*
 * $Id: smsserv.h,v 1.2 2006/04/17 14:19:44 Trocotronic Exp $ 
 */

typedef struct _ss SmsServ;
struct _ss
{
	Modulo *hmod;
	int espera;
	char *publi;
	unsigned restringido:1;
	char *id;
};

#define SS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define SS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define SS_ERR_EMPT "\00304ERROR: %s"

typedef struct _datasock
{
	char *query;
	int numero;
	Cliente *cl;
	Sock *sck;
	char *post;
}DataSock;

#define MAX_COLA 16

#define SS_SQL "sms"
#define CACHE_ULTIMO_SMS "ultimo_sms"
