/*
 * $Id: bdd.h,v 1.4 2004-09-17 19:09:46 Trocotronic Exp $ 
 */
 
extern u_int base64toint(const char *);
extern const char *inttobase64(char *, u_int, u_int);
extern void tea(u_int *, u_int *, u_int *);

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

extern MODVAR Udb *nicks, *chans, *ips, *sets;

#define BDD_NICKS 0
#define BDD_CHANS 1
#define BDD_IPS 2
#define BDD_SET 3

DLLFUNC extern Udb *busca_registro(int, char *), *busca_bloque(char *, Udb *);

#define BDD_PREO 0x1
#define BDD_OPER 0x2
#define BDD_DEVEL 0x4
#define BDD_ADMIN 0x8
#define BDD_ROOT 0x10
#define CHAR_NUM '*'
extern Udb *coge_de_id(int);
extern int coge_de_char(char);
extern void carga_bloque(int);
extern void descarga_bloque(int);
#define E_UDB_NODB 1
#define E_UDB_LEN 2
#define E_UDB_NOHUB 3
#define E_UDB_PARAMS 4
#define E_UDB_NOOPEN 5
#define E_UDB_FATAL 6
#endif
