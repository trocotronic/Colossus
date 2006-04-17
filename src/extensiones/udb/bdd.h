#define UDB_VER "3.2.3"
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
#define CHAR_NUM '*'
extern Udb *nicks, *chans, *ips, *sets;
extern u_int BDD_NICKS, BDD_CHANS, BDD_IPS, BDD_SET;
extern time_t gmts[DBMAX];
extern int regs[DBMAX];

extern Udb *BuscaRegistro(int, char *);
extern Udb *BuscaBloque(char *, Udb *);
int NivelOperBdd(char *);
extern char bloques[];
extern u_int BDD_TOTAL;
extern Udb *ultimo;
extern Opts NivelesBDD[];
extern Udb *IdAUdb(int);
extern void CargaBloque(int);
extern void DescargaBloque(int);
extern int ParseaLinea(int, char *, int);
extern int ActualizaHash(Udb *);
extern int Optimiza(Udb *);
extern int ActualizaGMT(Udb *, time_t);
extern time_t LeeGMT(int);
extern u_long LeeHash(int);
extern int CargaBloques();
extern void BddInit();
extern int TruncaBloque(Cliente *, Udb *, u_long);
extern void PropagaRegistro(char *, ...);

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
#define C_OPT "opciones"
#define C_OPT_TOK "O"

#define BDD_C_OPT_PBAN 0x1
