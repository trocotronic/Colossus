/*
 * $Id: nickserv.h,v 1.11 2008/05/26 22:07:06 Trocotronic Exp $
 */

#define NS_SID 0x1
#define NS_SMAIL 0x2
#define NS_PROT_KILL 0x4
#define NS_PROT_CHG 0x8
#define NS_AUTOMIGRAR 0x10
#define NS_REGCODE 0x20
#define NS_HIDEFMAIL 0x40

#define NS_MAX_NICK 32
#define NS_MAX_FBMAIL 64
#define NS_MAX_COMS 256

typedef struct _ns NickServ;
struct _ns
{
	int opts;
	char *recovernick; /* nick de RECOVER */
	char *securepass; /* pass segura para VALIDAR */
	int min_reg; /* en horas */
	int autodrop; /* en dias */
	int nicks; /* maximo de nicks por cuenta */
	int intentos; /* maximo intentos antes de hacer KILL */
	int maxlist; /* maximo de entradas por LIST */
	char *forbmail[NS_MAX_FBMAIL]; /* emails prohibidos */
	u_int forbmails;
	Modulo *hmod;
};

extern NickServ *nickserv;

#define NS_ERR_PARA "\00304ERROR: Faltan par�metros: %s %s "
#define NS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define NS_ERR_NURG "\00304ERROR: Este nick no est� registrado."
#define NS_ERR_EMPT "\00304ERROR: %s"
#define NS_ERR_FORB ERR_FORB

#define NS_OPT_WEB 0x1
#define NS_OPT_MAIL 0x2
#define NS_OPT_TIME 0x4
#define NS_OPT_MASK 0x8
#define NS_OPT_QUIT 0x10
#define NS_OPT_LIST 0x20
#define NS_OPT_NODROP 0x40
/* ATENCION: NS_OPT_UDB 0x80 */
#define NS_OPT_LOC 0x100

#define NS_SQL "nicks"
#define NS_FORBIDS "nforbids"
#define NS_SIGN_IDOK 42
#define NS_SIGN_DROP 43
#define NS_SIGN_REG 44

#define IsSusp(x) (SQLCogeRegistro(NS_SQL, x, "suspend"))

#define CACHE_INTENTOS_ID "intentos_id"
#define CACHE_ULTIMO_REG "ultimo_reg"
#define CACHE_REGCODE "regcode"

