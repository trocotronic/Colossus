/*
 * $Id: nickserv.h,v 1.2 2004-12-31 12:27:55 Trocotronic Exp $ 
 */

#define NS_SID 0x1
#define NS_SMAIL 0x2
#define NS_PROT_KILL 0x4
#define NS_PROT_CHG 0x8
#define NS_AUTOMIGRAR 0x10

#define NS_MAX_NICK 32
#define NS_MAX_FBMAIL 64
#define NS_MAX_COMS 256

typedef struct _ns NickServ;
struct _ns
{
	int opts;
	char *recovernick; /* nick de RECOVER */
	char *securepass; /* pass segura para VALIDAR */
	//int min_reg; /* en horas */
	int autodrop; /* en dias */
	int nicks; /* maximo de nicks por cuenta */
	int intentos; /* maximo intentos antes de hacer KILL */
	int maxlist; /* maximo de entradas por LIST */
	char *forbmail[NS_MAX_FBMAIL]; /* emails prohibidos */
	int forbmails;
	Modulo *hmod;
};

extern NickServ nickserv;

#define NS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define NS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define NS_ERR_NURG "\00304ERROR: Este nick no está registrado."
#define NS_ERR_EMPT "\00304ERROR: %s"
#define NS_ERR_FORB ERR_FORB

#define NS_OPT_WEB 0x1
#define NS_OPT_MAIL 0x2
#define NS_OPT_TIME 0x4
#define NS_OPT_MASK 0x8
#define NS_OPT_QUIT 0x10
#define NS_OPT_LIST 0x20
#define NS_OPT_NODROP 0x40
#define NS_OPT_UDB 0x80

#define NS_MYSQL "nicks"
#ifndef UDB
#define NS_FORBIDS "nforbids"
#endif
#define NS_SIGN_IDOK 61
#define NS_SIGN_DROP 62
#define NS_SIGN_REG 63

#ifdef UDB
#define IsNickUDB(x) (IsReg(x) && atoi(_mysql_get_registro(NS_MYSQL, x, "opts")) & NS_OPT_UDB)
#endif
#define IsSusp(x) (_mysql_get_registro(NS_MYSQL, x, "suspend"))
