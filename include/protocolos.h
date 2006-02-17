/*
 * $Id: protocolos.h,v 1.8 2006-02-17 19:54:42 Trocotronic Exp $ 
 */
typedef struct _extension Extension;
typedef struct _proto Protocolo;
typedef int (*Ext_Func)(Modulo *, Cliente *, char *[], int, char *[], int);
#define EXTFUNC(x) int (x)(Modulo *mod, Cliente *cl, char *parv[], int parc, char *param[], int params)
#define MAXMOD 64
typedef struct _com
{
	char *msg;
	char *tok;
	void *(*func)();
}Com;
typedef struct _senyalext
{
	struct _senyalext *sig;
	int senyal;
	Ext_Func func;
}SenyalExt;
struct _extension
{
	Extension *sig;
	Recurso hmod;
	ModInfo *info;
	int (*carga)(Extension *, Protocolo *);
	int (*descarga)(Extension *, Protocolo *);
	char *archivo;
	char *tmparchivo;
	Conf *config;
	SenyalExt *senyals[MAXSIGS];
};
struct _proto 
{
	Recurso hprot;
	char *archivo;
	char *tmparchivo;
	ModInfo *info;
	int (*carga)();
	int (*descarga)();
	//IRCFUNC(*sincroniza);
	void (*inicia)();
	SOCKFUNC(*parsea);
	mTab umodos[MAXMOD];
	mTab cmodos[MAXMOD];
	int (*comandos[25])();
	char *modcl;
	char *modmk;
	char *modpm1;
	char *modpm2;
	Extension *extensiones;
};
	
extern MODVAR Protocolo *protocolo;
extern int CargaProtocolo(Conf *);
extern void DescargaProtocolo();
extern void LiberaMemoriaProtocolo(Protocolo *);

//Macros de compatibilidad
#define ProtFunc(x) protocolo->comandos[x]
#define P_TRIO 0
#define P_MODO_USUARIO_LOCAL 1
#define P_MODO_USUARIO_REMOTO 2
#define P_MODO_CANAL 3
#define P_CAMBIO_USUARIO_LOCAL 4
#define P_CAMBIO_USUARIO_REMOTO 5
#define P_JOIN_USUARIO_LOCAL 6
#define P_JOIN_USUARIO_REMOTO 7
#define P_PART_USUARIO_LOCAL 8
#define P_PART_USUARIO_REMOTO 9
#define P_QUIT_USUARIO_LOCAL 10
#define P_QUIT_USUARIO_REMOTO 11
#define P_NUEVO_USUARIO 12
#define P_PRIVADO 13
#define P_CAMBIO_HOST_LOCAL 14
#define P_CAMBIO_HOST_REMOTO 15
#define P_FORB_NICK 16
#define P_LAG 17
#define P_WHOIS_ESPECIAL 18
#define P_GLINE 19
#define P_KICK 20
#define P_TOPIC 21
#define P_NOTICE 22
#define P_INVITE 23
#define P_MSG_VL 24

#define ProtUmodo(x) (protocolo->umodos + x)->mode
#define ProtCmodo(x) (protocolo->cmodos + x)->mode
#define ProtUmodo_f(x) (protocolo->umodos + x)->flag
#define ProtCmodo_f(x) (protocolo->cmodos + x)->flag
#define TRIO(x) (char *)ProtFunc(P_TRIO)(x)

#ifdef ENLACE_DINAMICO
 #define PROT_INFO(name) Prot_Info
 #define PROT_CARGA(name) Prot_Carga
 #define PROT_DESCARGA(name) Prot_Descarga
 #define PROT_PARSEA(name) Prot_Parsea
 #define PROT_INICIA(name) Prot_Inicia
 #define PROT_UMODOS(name) Prot_Umodos
 #define PROT_CMODOS(name) Prot_Cmodos
 #define PROT_COMANDOS(name) Prot_Comandos
#else
 #define PROT_INFO(name) name##_Info
 #define PROT_CARGA(name) name##_Carga
 #define PROT_DESCARGA(name) name##_Descarga
 #define PROT_PARSEA(name) name##_Parsea
 #define PROT_INICIA(name) name##_Inicia
 #define PROT_UMODOS(name) name##_Umodos
 #define PROT_CMODOS(name) name##_Cmodos
 #define PROT_COMANDOS(name) name##_Comandos
#endif

extern Extension *CreaExtension(Conf *, Protocolo *);
extern int DescargaExtensiones(Protocolo *);
extern int CargaExtensiones(Protocolo *);
extern void InsertaSenyalExt(int, Ext_Func, Extension *);
extern int BorraSenyalExt(int, Ext_Func, Extension *);
//extern ExtFunc *BuscaExtFunc(Extension *, Modulo *);
extern void LlamaSenyalExt(Modulo *, int, Cliente *, char **, int, char **, int);
#define EOI(x,y) LlamaSenyalExt(x->hmod, y, cl, parv, parc, param, params)

mTab *InsertaModoProtocolo(char, u_long *, mTab *);
mTab *BuscaModoProtocolo(u_long, mTab *);
extern MODVAR u_long UMODE_REGNICK, UMODE_HIDE, CHMODE_RGSTR;
