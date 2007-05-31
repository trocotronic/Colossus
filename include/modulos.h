/*
 * $Id: modulos.h,v 1.19 2007-05-31 23:06:36 Trocotronic Exp $ 
 */

#ifndef _modulos_
#define _modulos_
#define MAXMODS 128
#define MAX_RES 64
#define MAX_COMS 256
#define MAX_NIVS 16

typedef struct _funcion Funcion;
typedef struct _bcom bCom;
typedef int (*Mod_Func)(Cliente *, char *[], int, char *[], int, Funcion *);
typedef int (*Mod_FuncHelp)(Cliente *, char *[], int);
#define BOTFUNC(x) int (x)(Cliente *cl, char *parv[], int parc, char *param[], int params, Funcion *fc)
#define BOTFUNCHELP(x) int (x)(Cliente *cl, char *param[], int params)

typedef struct _mod Modulo;

/*!
 * @desc: Recurso de comandos para módulos.
 * @params: $com Nombre del comando.
 	    $func Función a ejecutar con el comando asociado.
 	    $nivel Nivel del cliente obligatorio para poder ejecutar esta función. Valores posibles: N0, N1..., N15.
 	    $descrip Breve descripción del comando.
 	    $func_help Función a ejecutar cuando se pida un help de ese comando.
 * @cat: Modulos
 * @ver: ProcesaComsMod
 !*/
 
struct _bcom
{
	char *com;
	Mod_Func func;
	u_int nivel;
	char *descrip;
	Mod_FuncHelp func_help;
};
struct _funcion
{
	char *com;
	Mod_Func func;
	u_int nivel;
	char *descrip;
	Mod_FuncHelp func_help;
	Recurso hmod;
};
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
 * @desc: Recurso de módulo. Se utiliza durante la carga del mismo.
 * @params: $hmod Puntero a la librería física cargada por el sistema operativo.
 	    $archivo Ruta de la librería original.
 	    $tmparchivo Ruta del archivo temporal en uso.
 	    $nick Apodo asociado.
 	    $ident Ident asociada.
 	    $realname Nombre asociado.
 	    $host Host asociado.
 	    $residente Lista de canales en lo que está residente.
 	    $modos Modos de usuario que utiliza.
 	    $mascara Máscara completa.
 	    $id Id o número de identificación.
 	    $config Ruta a su archivo de configuración.
 	    $cl Recurso de cliente asociado.
 	    <b>Todos los demás miembros no deben usarse.</b>
 * @cat: Modulos
 !*/
struct _mod
{
	Modulo *sig;
	Recurso hmod;
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
	unsigned activo:1;
	ModInfo *info;
	char *serial;
	Funcion *comando[MAX_COMS];
	int comandos;
	int (*carga)(Modulo *);
	int (*descarga)(Modulo *);
	void *conf; /* es un puntero a su estructura, según lo defina. habrá que hacer cast *CADA VEZ* */
};

extern MODVAR Modulo *modulos;
extern Modulo *BuscaModulo(char *, Modulo *);

#define ERR_DESC "\00304ERROR: Comando desconocido."
#define ERR_NOID "\00304ERROR: No estás identificado."
#define ERR_FORB "\00304ERROR: Acceso denegado."
#define ERR_NSUP "\00304ERROR: Este comando no está soportado en este servidor."
extern void DescargaModulos(void);
extern void DescargaModulo(Modulo *);
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
#define CLI(x) x->hmod->cl
/*!
 * @desc: Función a ejecutar dentro de cada módulo una vez se haya cargado.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 * @sntx: void BotSet(void *mod)
 * @cat: Modulos
 * @ver: BotUnset
 !*/
#define BotSet(x) x->hmod = mod; mod->conf = x
/*!
 * @desc: Función a ejecutar dentro de cada módulo una vez se haya descargado.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 * @sntx: void BotUnset(void *mod)
 * @cat: Modulos
 * @ver: BotSet
 !*/
#define BotUnset(x) LiberaComs(x->hmod); DescargaAliases(x->hmod); x->hmod->conf = NULL; x->hmod = NULL; Free(x);
/*!
 * @desc: Responde a un usuario con especificación de una función si esta función existe y está cargada. Muy útil para sistemas de AYUDA.
 * @params: $mod [in] Estructura del módulo. Dependerá de cada módulo.
 	    $nombre [in] Nombre de la función.
 	    $cont [in] Especificación de la función.
 * @cat: Modulos
 * @sntx: void FuncResp(void mod, char *nombre, char *cont)
 !*/
#define FuncResp(x,y,z) do { if (TieneNivel(cl,y,x->hmod,NULL)) { Responde(cl, CLI(x), "\00312" y "\003 " z); }}while(0)

extern void ListaDescrips(Modulo *, Cliente *); 
extern int MuestraAyudaComando(Cliente *, char *, Modulo *, char **m, int);

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
extern int TestComMod(Conf *, bCom *, int);

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
extern Nivel *BuscaNivelNum(int);
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

extern Funcion *TieneNivel(Cliente *, char *, Modulo *, char *);

extern Alias *CreaAlias(char *, char *, Modulo *);
extern void DescargaAliases(Modulo *);
extern Alias *BuscaAlias(char **, int, Modulo *);
extern int ModuloEsResidente(Modulo *, char *);
#endif
