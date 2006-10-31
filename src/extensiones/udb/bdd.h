#define UDB_VER "3.3"
#ifdef _WIN32
#define DB_DIR "database\\"
#define DB_DIR_BCK DB_DIR "backup\\"
#else
#define DB_DIR "database/"
#define DB_DIR_BCK DB_DIR "backup/"
#endif
typedef struct _udb
{
	char *item;
	int id;
	char *data_char;
	u_long data_long;
	struct _udb *hsig, *up, *mid, *down;
}Udb;
typedef struct _bloque
{
	Udb *arbol;
	struct _bloque *sig;
	u_long crc32;
	char *path;
	u_int id;
	u_long lof;
	time_t gmt;
	Cliente *res;
	u_int regs;
	char letra;
}UDBloq;
#define DBMAX 64
#define CHAR_NUM '*'
extern UDBloq *N, *C, *I, *S;
extern Udb *UDB_NICKS, *UDB_CANALES, *UDB_IPS, *UDB_SET;

extern Udb *BuscaBloque(char *, Udb *);
u_int LevelOperUdb(char *);
extern char bloques[];
extern u_int BDD_TOTAL;
extern UDBloq *ultimo;
extern Opts NivelesBDD[];
extern UDBloq *CogeDeId(u_int);
extern void CargaBloque(u_int);
extern void DescargaBloque(u_int);
extern int ParseaLinea(u_int, char *, int);
extern int ActualizaHash(UDBloq *);
extern int OptimizaBloque(UDBloq *);
extern int ActualizaGMT(UDBloq *, time_t);
extern time_t LeeGMT(u_int);
extern u_long LeeHash(u_int);
extern int CargaBloques();
extern void IniciaUDB();
extern int TruncaBloque(Cliente *, UDBloq *, u_long);
extern void PropagaRegistro(char *, ...);
extern int CopiaSeguridad(UDBloq *, char *);
extern int RestauraSeguridad(UDBloq *, char *);

#define E_UDB_NODB 1
#define E_UDB_LEN 2
#define E_UDB_NOHUB 3
#define E_UDB_PARAMS 4
#define E_UDB_NOOPEN 5
#define E_UDB_FATAL 6
#define E_UDB_RPROG 7 
#define E_UDB_NORES 8 
#define E_UDB_FBSRV 9
#define E_UDB_REP 10


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
