/*
 * $Id: bdd.h,v 1.7 2004-12-31 12:27:51 Trocotronic Exp $ 
 */

#ifdef UDB
#ifdef _WIN32
#define DB_DIR "database\\"
#else
#define DB_DIR "database/"
#endif
typedef struct _udb Udb;
struct _udb
{
	char *item;
	int id;
	char *data_char;
	u_long data_long;
	struct _udb *prev, *sig, *hsig, *up, *mid, *down;
};

MODFUNC extern MODVAR Udb *nicks;
MODFUNC extern MODVAR Udb *chans;
MODFUNC extern MODVAR Udb *ips;
MODFUNC extern MODVAR Udb *sets;
MODFUNC extern MODVAR u_int BDD_NICKS;
MODFUNC extern MODVAR u_int BDD_CHANS;
MODFUNC extern MODVAR u_int BDD_IPS;
MODFUNC extern MODVAR u_int BDD_SET;

DLLFUNC extern Udb *busca_registro(int, char *);
DLLFUNC extern Udb *busca_bloque(char *, Udb *);
DLLFUNC int level_oper_bdd(char *);
MODFUNC extern MODVAR char bloques[];
MODFUNC extern MODVAR u_int BDD_TOTAL;

#define BDD_PREO 0x1
#define BDD_OPER 0x2
#define BDD_DEVEL 0x4
#define BDD_ADMIN 0x8
#define BDD_ROOT 0x10
#define CHAR_NUM '*'
DLLFUNC extern Udb *coge_de_id(int);
DLLFUNC extern u_int coge_de_char(char);
DLLFUNC extern void carga_bloque(int);
DLLFUNC extern void descarga_bloque(int);
DLLFUNC extern int parsea_linea(int, char *, int);
DLLFUNC extern int actualiza_hash(Udb *);
extern void carga_bloques();
extern void bdd_init();
DLLFUNC extern int trunca_bloque(Cliente *, Udb *, u_long);
#define E_UDB_NODB 1
#define E_UDB_LEN 2
#define E_UDB_NOHUB 3
#define E_UDB_PARAMS 4
#define E_UDB_NOOPEN 5
#define E_UDB_FATAL 6
#endif
