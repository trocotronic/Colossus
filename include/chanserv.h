#define CS_MAX_COMS 256
#define CS_SID 0x1
#define CS_AUTOMIGRAR 0x2
#define CS_MAX_REGS 256
#define CS_MAX_AKICK 32

typedef struct _cs ChanServ;
struct _cs
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	int maxlist;
	int autodrop;
	char bantype;
	char *tokform;
	int vigencia;
	int necesarios;
	int antig;
	char *residente;
	bCom *comando[CS_MAX_COMS]; /* comandos soportados */
	int comandos;
};
typedef struct _csregistros CsRegistros;
struct _csregistros
{
	char *nick;
	struct _subc
	{
		char *canal;
		u_long flags;
	}*sub[CS_MAX_REGS];
	int subs;
	struct _csregistros *sig;
};
typedef struct _akick Akick;
struct _akick
{
	char *canal;
	struct _akc
	{
		char *nick;
		char *motivo;
		char *puesto;
	}*akick[CS_MAX_AKICK];
	int akicks;
	struct _akick *sig;
};
/* Opciones */
#define CS_OPT_RMOD 0x1 /* candado de modos +m */
#define CS_OPT_RTOP 0x2 /* retencion de topic +r */
#define CS_OPT_KTOP 0x4 /* candado de topic +k */
#define CS_OPT_SEC 0x8 /* canal seguro +s */
#define CS_OPT_SOP 0x10 /* canal con secureops +o */
#define CS_OPT_HIDE 0x20 /* canal oculto +h */
#define CS_OPT_DEBUG 0x40 /* canal en debug +d */
#define CS_OPT_NODROP 0x80 /* canal no dropable +n */
#define CS_OPT_UDB 0x100

/* Niveles */

#define CS_LEV_SET 0x0001 /* fijar opciones +s */
#define CS_LEV_EDT 0x0002 /* editar la lista de accesos +e */
#define CS_LEV_LIS 0x0004 /* lista la lista de accesos +l */
#define CS_LEV_AAD 0x0008 /* auto +a */
#define CS_LEV_AOP 0x0010 /* auto +o */
#define CS_LEV_AHA 0x0020 /* auto +h */
#define CS_LEV_AVO 0x0040 /* auto +v */
#define CS_LEV_RMO 0x0080 /* comandos remotos +r */
#define CS_LEV_RES 0x0100 /* resetear opciones +c */
#define CS_LEV_ACK 0x0200 /* akicks +k */
#define CS_LEV_INV 0x0400 /* invites +i */
#define CS_LEV_JOB 0x0800 /* joins +j */
#define CS_LEV_REV 0x1000 /* revenge +g */
#define CS_LEV_MEM 0x2000 /* memo +m */
#define CS_LEV_ALL (CS_LEV_SET | CS_LEV_EDT | CS_LEV_LIS | CS_LEV_AAD | CS_LEV_AOP | CS_LEV_AHA | CS_LEV_AVO | CS_LEV_RMO | CS_LEV_RES | CS_LEV_ACK | CS_LEV_INV | CS_LEV_JOB | CS_LEV_REV | CS_LEV_MEM)
#define CS_LEV_MOD (CS_LEV_AAD | CS_LEV_AOP | CS_LEV_AHA | CS_LEV_AVO)

#define CS_SIGN_IDOK 20
#define CS_SIGN_DROP 21
#define CS_SIGN_REG 22

extern ChanServ *chanserv;

#define CS_MYSQL "canales"
#define CS_TOK "ctokens"
#ifndef UDB
#define CS_FORBIDS "cforbids"
#endif

#define CS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define CS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define CS_ERR_NCHR "\00304ERROR: Este canal no está registrado."
#define CS_ERR_EMPT "\00304ERROR: %s"
#define CS_ERR_SUSP "\00304ERROR: No puedes aplicar este comando sobre un canal suspendido."
#define CS_ERR_FORB ERR_FORB

#define IsChanDebug(x) (x && (atoi(_mysql_get_registro(CS_MYSQL, x, "opts")) & CS_OPT_DEBUG))
DLLFUNC extern u_long tiene_nivel(char *, char *, u_long);
DLLFUNC extern CsRegistros *busca_cregistro(char *);
DLLFUNC extern char *cflags2str(u_long);
#ifdef UDB
#define IsChanUDB(x) (atoi(_mysql_get_registro(CS_MYSQL, x, "opts")) & CS_OPT_UDB)
#endif
DLLFUNC extern int ChanReg(char *);
extern char *IsChanSuspend(char *);
extern char *IsChanForbid(char *);
#define IsChanReg(x) (ChanReg(x) == 1)
#define IsChanPReg(x) (ChanReg(x) == 2)
DLLFUNC extern int es_fundador(Cliente *, char *);
