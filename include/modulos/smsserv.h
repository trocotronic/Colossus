/*
 * $Id: smsserv.h,v 1.1 2006-02-17 19:19:02 Trocotronic Exp $ 
 */

typedef struct _ss SmsServ;
struct _ss
{
	Modulo *hmod;
	int espera;
	char *publi;
	unsigned restringido:1;
	char *login;
	char *pass;
};

#define SS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define SS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define SS_ERR_EMPT "\00304ERROR: %s"

typedef struct _sms Sms;
struct _sms 
{
	u_int numero;
	char *texto;
	Cliente *cl;
	Sock *sck;
	time_t tiempo;
};

#define MAX_SMS 16

#define SS_SQL "sms"
#define CACHE_ULTIMO_SMS "ultimo_sms"
