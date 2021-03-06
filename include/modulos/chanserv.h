/*
 * $Id: chanserv.h,v 1.14 2008/05/26 22:07:06 Trocotronic Exp $
 */

#define CS_SID 0x1
#define CS_MAX_REGS 256
#define CS_MAX_AKICK 32

typedef struct _cs ChanServ;
struct _cs
{
	int opts;
	int maxlist;
	int autodrop;
	char bantype;
	char *tokform;
	int vigencia;
	int necesarios;
	int antig;
	Modulo *hmod;
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
/* ATENCION: CS_OPT_UDB 0x1000 */
#define CS_OPT_UDB 0x1000
#define CS_AUTOMIGRAR 0x1000

/* Niveles */

#define CS_LEV_SET 0x001 /* fijar opciones +s */
#define CS_LEV_EDT 0x002 /* editar la lista de accesos +e */
#define CS_LEV_LIS 0x004 /* lista la lista de accesos +l */
#define CS_LEV_RMO 0x008 /* comandos remotos +r */
#define CS_LEV_RES 0x010 /* resetear opciones +c */
#define CS_LEV_ACK 0x020 /* akicks +k */
#define CS_LEV_INV 0x040 /* invites +i */
#define CS_LEV_JOB 0x080 /* joins +j */
#define CS_LEV_REV 0x100 /* revenge +g */
#define CS_LEV_MEM 0x200 /* memo +m */
#define CS_LEV_ALL (CS_LEV_SET | CS_LEV_EDT | CS_LEV_LIS | CS_LEV_RMO | CS_LEV_RES | CS_LEV_ACK | CS_LEV_INV | CS_LEV_JOB | CS_LEV_REV | CS_LEV_MEM)
#define CS_LEV_MOD (CS_LEV_REV) /* el kick revenge tambi�n */

#define CS_SIGN_IDOK 32
#define CS_SIGN_DROP 33
#define CS_SIGN_REG 34

#define CS_SQL "canales"
#define CS_TOK "ctokens"
#define CS_ACCESS "accesos"
#define CS_AKICKS "akicks"
#define CACHE_FUNDADORES "fundadores"

#define CS_ERR_PARA "\00304ERROR: Faltan par�metros: %s %s "
#define CS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define CS_ERR_NCHR "\00304ERROR: Este canal no est� registrado."
#define CS_ERR_NACT "\00304ERROR: Este canal no est� activo."
#define CS_ERR_EMPT "\00304ERROR: %s"
#define CS_ERR_SUSP "\00304ERROR: No puedes aplicar este comando sobre un canal suspendido."
#define CS_ERR_FORB ERR_FORB

DLLFUNC extern u_long CSTieneNivel(Cliente *, char *, u_long);
DLLFUNC extern int ChanReg(char *);
#define IsChanReg(x) (ChanReg(x) != 0)
#define IsChanAct(x) (ChanReg(x) == 1)
#define IsChanPReg(x) (ChanReg(x) == 2)
#define IsChanSuspend(x) (ChanReg(x) == 3)
#define IsChanForbid(x) (ChanReg(x) == 4)
DLLFUNC extern int CSEsFundador(Cliente *, char *);
