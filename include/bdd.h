/*
 * $Id: bdd.h,v 1.18 2005-12-25 21:12:26 Trocotronic Exp $ 
 */

#ifdef UDB
#define UDB_VER "3.2.2"
#ifdef _WIN32
#define DB_DIR "database\\"
#else
#define DB_DIR "database/"
#endif
typedef struct _udb
{
	char *item;
	int id;
	char *data_char;
	u_long data_long;
	struct _udb *hsig, *up, *mid, *down;
}Udb;

#define DBMAX 128
MODFUNC extern MODVAR Udb *nicks;
MODFUNC extern MODVAR Udb *chans;
MODFUNC extern MODVAR Udb *ips;
MODFUNC extern MODVAR Udb *sets;
MODFUNC extern MODVAR u_int BDD_NICKS;
MODFUNC extern MODVAR u_int BDD_CHANS;
MODFUNC extern MODVAR u_int BDD_IPS;
MODFUNC extern MODVAR u_int BDD_SET;
MODFUNC extern MODVAR time_t gmts[DBMAX];
MODFUNC extern MODVAR int regs[DBMAX];

DLLFUNC extern Udb *BuscaRegistro(int, char *);
DLLFUNC extern Udb *BuscaBloque(char *, Udb *);
DLLFUNC int NivelOperBdd(char *);
MODFUNC extern MODVAR char bloques[];
MODFUNC extern MODVAR u_int BDD_TOTAL;
MODFUNC extern MODVAR Udb *ultimo;

#define CHAR_NUM '*'
MODFUNC extern MODVAR Opts NivelesBDD[];
DLLFUNC extern Udb *IdAUdb(int);
DLLFUNC extern void CargaBloque(int);
DLLFUNC extern void DescargaBloque(int);
DLLFUNC extern int ParseaLinea(int, char *, int);
DLLFUNC extern int ActualizaHash(Udb *);
DLLFUNC extern int Optimiza(Udb *);
DLLFUNC extern int ActualizaGMT(Udb *, time_t);
DLLFUNC extern time_t LeeGMT(int);
DLLFUNC extern u_long LeeHash(int);
extern void CargaBloques();
extern void BddInit();
DLLFUNC extern int TruncaBloque(Cliente *, Udb *, u_long);
#define E_UDB_NODB 1
#define E_UDB_LEN 2
#define E_UDB_NOHUB 3
#define E_UDB_PARAMS 4
#define E_UDB_NOOPEN 5
#define E_UDB_FATAL 6
#define E_UDB_RPROG 7 
#define E_UDB_NORES 8 

#define N_SUS "suspendido"
#define N_SUS_TOK "S"
#define N_OPE "oper"
#define N_OPE_TOK "O"
#define N_MOD "modos"
#define N_MOD_TOK "M"
#define S_CLA "clave_cifrado"
#define S_CLA_TOK "L"
#define C_SUS "suspendido"
#define C_SUS_TOK "S"
#define C_FOR "forbid"
#define C_FOR_TOK "B"

#endif
