/*
 * $Id: modulos.h,v 1.13 2005-12-04 14:09:22 Trocotronic Exp $ 
 */

#define MAXMODS 128
#define MAX_RES 64
#define MAX_COMS 256
#define MAX_NIVS 16

#define BOTFUNC(x) int (x)(Cliente *cl, char *parv[], int parc, char *param[], int params)
typedef int (*Mod_Func)(Cliente *, char *[], int, char *[], int);

/*!
 * @desc: Recurso de comandos para m�dulos.
 * @params: $com Nombre del comando.
 	    $func Funci�n a ejecutar con el comando asociado.
 	    $nivel Nivel del cliente obligatorio para poder ejecutar esta funci�n. Valores posibles:
 	    	- TODOS: todos los usuarios pueden ejecutarlo.
 	    	- USER: s�lo usuarios registrados (+r).
 	    	- PREO: s�lo preopers.
 	    	- OPER: s�lo operadores (+h).
 	    	- DEVEL: s�lo devels.
 	    	- IRCOP: s�lo ircops (+o).
 	    	- ADMIN: s�lo admins (+a).
 	    	- ROOT: s�lo roots.
 * @cat: Modulos
 * @ver: ProcesaComsMod
 !*/
 
typedef struct _bcom
{
	char *com;
	Mod_Func func;
	u_int nivel;
}bCom;
typedef struct _info
{
	char *nombre;
	double version;
	char *autor;
	char *email;
}ModInfo, ProtInfo;

typedef struct _alias Alias;
struct _alias
{
	Alias *sig;
	char *formato[16];
	char *sintaxis[16];
	u_long crc;
};

/*!
 * @desc: Recurso de m�dulo. Se utiliza durante la carga del mismo.
 * @params: $hmod Puntero a la librer�a f�sica cargada por el sistema operativo.
 	    $archivo Ruta de la librer�a original.
 	    $tmparchivo Ruta del archivo temporal en uso.
 	    $nick Apodo asociado.
 	    $ident Ident asociada.
 	    $realname Nombre asociado.
 	    $host Host asociado.
 	    $residente Lista de canales en lo que est� residente.
 	    $modos Modos de usuario que utiliza.
 	    $mascara M�scara completa.
 	    $id Id o n�mero de identificaci�n.
 	    $config Ruta a su archivo de configuraci�n.
 	    $cl Recurso de cliente asociado.
 	    <b>Todos los dem�s miembros no deben usarse.</b>
 * @cat: Modulos
 !*/
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
	char *config;
	Cliente *cl;
	Alias *aliases;
	char cargado;
	ModInfo *info;
	bCom *comando[MAX_COMS];
	int comandos;
	int (*carga)(struct _mod *);
	int (*descarga)(struct _mod *);
	void *conf; /* es un puntero a su estructura, seg�n lo defina. habr� que hacer cast *CADA VEZ* */
}Modulo;

extern MODVAR Modulo *modulos;
extern Modulo *BuscaModulo(char *, Modulo *);

#define ERR_DESC "\00304ERROR: Comando desconocido."
#define ERR_NOID "\00304ERROR: No est�s identificado."
#define ERR_FORB "\00304ERROR: Acceso denegado."
#define ERR_NSUP "\00304ERROR: Este comando no est� soportado en este servidor."
extern void DescargaModulos(void);
extern Modulo *CreaModulo(char *);
#ifdef _WIN32
extern const char *ErrorDl(void);
#endif

/*!
 * @desc: Devuelve el recurso cliente del m�dulo.
 * @params: $mod [in] Estructura del m�dulo. Depender� de cada m�dulo.
 * @sntx: Cliente *CLI(void mod)
 * @cat: Modulos
 * @ret: Devuelve el recurso cliente del m�dulo.
 !*/
#define CLI(x) x.hmod->cl
/*!
 * @desc: Funci�n a ejecutar dentro de cada m�dulo una vez se haya cargado.
 * @params: $mod [in] Estructura del m�dulo. Depender� de cada m�dulo.
 * @sntx: void BotSet(void mod)
 * @cat: Modulos
 * @ver: BotUnset
 !*/
#define BotSet(x) x.hmod = mod; mod->conf = &(x)
/*!
 * @desc: Funci�n a ejecutar dentro de cada m�dulo una vez se haya descargado.
 * @params: $mod [in] Estructura del m�dulo. Depender� de cada m�dulo.
 * @sntx: void BotUnset(void mod)
 * @cat: Modulos
 * @ver: BotSet
 !*/
#define BotUnset(x) LiberaComs(x.hmod); DescargaAliases(x.hmod); x.hmod->conf = NULL; x.hmod = NULL; 
extern Mod_Func BuscaFuncion(Modulo *, char *, u_int *);
/*!
 * @desc: Responde a un usuario con especificaci�n de una funci�n si esta funci�n existe y est� cargada. Muy �til para sistemas de AYUDA.
 * @params: $mod [in] Estructura del m�dulo. Depender� de cada m�dulo.
 	    $nombre [in] Nombre de la funci�n.
 	    $cont [in] Especificaci�n de la funci�n.
 * @cat: Modulos
 * @sntx: void FuncResp(void mod, char *nombre, char *cont)
 !*/
#define FuncResp(x,y,z) do { if (TieneNivel(cl,y,x.hmod,NULL)) { Responde(cl, CLI(x), "\00312" y "\003 " z); }}while(0)

#ifdef ENLACE_DINAMICO
 #define MOD_INFO(name) Mod_Info
 #define MOD_CARGA(name) Mod_Carga
 #define MOD_DESCARGA(name) Mod_Descarga
#else
 #define MOD_INFO(name) name##_Info
 #define MOD_CARGA(name) name##_Carga
 #define MOD_DESCARGA(name) name##_Descarga
#endif

extern int ProcesaComsMod(Conf *, Modulo *, bCom *);
extern void LiberaComs(Modulo *);
extern void SetComMod(Conf *, Modulo *, bCom *);
extern int TestComMod(Conf *, bCom *, char);

typedef struct _nivel
{
	char *nombre;
	char *flags;
	u_int nivel;
}Nivel;
#define TODOS 0x0
extern u_int InsertaNivel(char *, char *);
extern void DescargaNiveles();
extern Nivel *BuscaNivel(char *);
extern int nivs;
extern Nivel *niveles[MAX_NIVS];

#define N0 0x0
#define N1 0x1
#define N2 0x2
#define N3 0x4
#define N4 0x8
#define N5 0x10
#define N6 0x20
#define N7 0x40
#define N8 0x80
#define N9 0x100
#define N10 0x200
#define N11 0x400
#define N12 0x800
#define N13 0x1000
#define N14 0x2000
#define N15 0x4000

extern Mod_Func TieneNivel(Cliente *, char *, Modulo *, char *);

extern Alias *CreaAlias(char *, char *, Modulo *);
extern void DescargaAliases(Modulo *);
extern Alias *BuscaAlias(char **, int, Modulo *);
