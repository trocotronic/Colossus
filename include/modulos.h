/*
 * $Id: modulos.h,v 1.5 2004-09-23 17:01:49 Trocotronic Exp $ 
 */

#ifndef _WIN32
#include <dlfcn.h>
#endif

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

#define TODOS 0x0
#define USERS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP | PREO | USER)
#define PREOS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP | PREO)
#define OPERS (BOT | ROOT | ADMIN | OPER | DEVEL | IRCOP)
#define ADMINS (BOT | ROOT | ADMIN)
#define ROOTS (BOT | ROOT)
#define BOTFUNC(x) int (x)(Cliente *cl, char *parv[], int parc, char *param[], int params)
typedef struct _bcom bCom;
struct _bcom
{
	char *com;
	int (*func)(Cliente *, char *[], int, char *[], int);
	int nivel;
};
typedef struct _modinfo
{
	char *nombre;
	double version;
	char *autor;
	char *email;
	char *config;
}ModInfo;
typedef struct _mod
{
#ifdef _WIN32
	HMODULE hmod;
#else
	void *hmod;
#endif
	char *archivo;
	char *tmparchivo;
	struct _mod *sig;
	char *nick;
	char *ident;
	char *realname;
	char *host;
	char *residente;
	char *modos;
	int id;
	char cargado;
	ModInfo *info;
	bCom **comandos;
	int (*carga)(struct _mod *);
	int (*descarga)(struct _mod *);
	char *config;
}Modulo;
	
extern MODVAR Modulo *modulos;
extern Modulo *busca_modulo(char *, Modulo *);
extern void response(Cliente *, char *, char *, ...);

#define ERR_DESC "\00304ERROR: Comando desconocido."
#define ERR_NOID "\00304ERROR: No estás identificado."
#define ERR_FORB "\00304ERROR: Acceso denegado."
extern void descarga_modulos(void);
extern int crea_modulo(char *);
