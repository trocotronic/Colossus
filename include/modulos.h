/*
 * $Id: modulos.h,v 1.11 2005-09-14 14:45:04 Trocotronic Exp $ 
 */

/* Los rangos se definen por bits. A cada bit le corresponde un estado.
   El ultimo bit se reserva al uso de tener el nick registrado.
   Este bit es obligatorio. Si aparece como estado, debe aparecer como nivel.
   Por ejemplo:
   Un usuario tiene nivel 111001 (observese que esta registrado)
   Podria utilizar un comando de nivel 111111. Pero si tuviera nivel 110100 (sin registrar)
   no podría utilizar este comando, ya que el último bit es de aparición obligatoria.
   */
   
#define NOTID 0x0
#define USER 0x1
#define PREO 0x2
#define OPER 0x4
#define DEVEL 0x8
#define IRCOP 0x10
#define ADMIN 0x20
#define ROOT 0x40
#define BOT 0x80
#define AWAY 0x100

#define MAXMODS 128
#define MAX_RES 64
#define MAX_COMS 256

#define TODOS 0x0
#define USERS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP | PREO | USER)
#define PREOS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP | PREO)
#define OPERS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP)
#define ADMINS (BOT | ROOT | ADMIN)
#define ROOTS (BOT | ROOT)
#define BOTFUNC(x) int (x)(Cliente *cl, char *parv[], int parc, char *param[], int params)
typedef int (*Mod_Func)(Cliente *, char *[], int, char *[], int);
typedef struct _bcom
{
	char *com;
	Mod_Func func;
	int nivel;
}bCom;
typedef struct _info
{
	char *nombre;
	double version;
	char *autor;
	char *email;
}ModInfo, ProtInfo;
typedef struct _mod
{
	struct _mod *sig;
#ifdef _WIN32
	HMODULE hmod;
#else
	void *hmod;
#endif
	char *archivo;
	char *tmparchivo;
	char *nick;
	char *ident;
	char *realname;
	char *host;
	char *residente;
	char *modos;
	char *mascara;
	int id;
	char cargado;
	ModInfo *info;
	bCom *comando[MAX_COMS];
	int comandos;
	int (*carga)(struct _mod *);
	int (*descarga)(struct _mod *);
	char *config;
	void *conf; /* es un puntero a su estructura, según lo defina. habrá que hacer cast *CADA VEZ* */
	Cliente *cl;
}Modulo;

extern MODVAR Modulo *modulos;
extern Modulo *BuscaModulo(char *, Modulo *);

#define ERR_DESC "\00304ERROR: Comando desconocido."
#define ERR_NOID "\00304ERROR: No estás identificado."
#define ERR_FORB "\00304ERROR: Acceso denegado."
#define ERR_NSUP "\00304ERROR: Este comando no está soportado en este servidor."
extern void DescargaModulos(void);
extern Modulo *CreaModulo(char *);
#ifdef _WIN32
extern const char *ErrorDl(void);
#endif

/*!
 * @desc: Devuelve el recurso cliente del módulo.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 * @sntx: Cliente *CLI(void mod)
 * @cat: Modulos
 * @ret: Devuelve el recurso cliente del módulo.
 !*/
#define CLI(x) x.hmod->cl
/*!
 * @desc: Función a ejecutar dentro de cada módulo una vez se haya cargado.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 * @sntx: void BotSet(void mod)
 * @cat: Modulos
 * @ver: BotUnset
 !*/
#define BotSet(x) x.hmod = mod; mod->conf = &(x)
/*!
 * @desc: Función a ejecutar dentro de cada módulo una vez se haya descargado.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 * @sntx: void BotUnset(void mod)
 * @cat: Modulos
 * @ver: BotSet
 !*/
#define BotUnset(x) x.hmod->conf = NULL; x.hmod = NULL; 
extern Mod_Func BuscaFuncion(Modulo *, char *, int *);
/*!
 * @desc: Responde a un usuario con especificación de una función si esta función existe y está cargada. Muy útil para sistemas de AYUDA.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 	    $nombre [in] Nombre de la función.
 	    $cont [in] Especificación de la función.
 * @cat: Modulos
 * @sntx: void FuncResp(void mod, char *nombre, char *cont)
 !*/
#define FuncResp(x,y,z) do { if (BuscaFuncion(x.hmod,y,NULL)) { Responde(cl, CLI(x), "\00312" y "\003 " z); }}while(0)

#ifdef ENLACE_DINAMICO
 #define MOD_INFO(name) Mod_Info
 #define MOD_CARGA(name) Mod_Carga
 #define MOD_DESCARGA(name) Mod_Descarga
#else
 #define MOD_INFO(name) name##_Info
 #define MOD_CARGA(name) name##_Carga
 #define MOD_DESCARGA(name) name##_Descarga
#endif
