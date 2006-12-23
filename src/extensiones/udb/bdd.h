#define UDB_VER "3.5"
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
	u_int ver;
	int fd;
}UDBloq;

/*
 * ----------------------------------------------------------------
 * | u_int id | u_int ver | u_long hash | time_t gmt | ...data... |
 * ----------------------------------------------------------------
 */
#define INI_ID 0
#define INI_VER (INI_ID + sizeof(u_int))
#define INI_HASH (INI_VER + sizeof(u_int))
#define INI_GMT (INI_HASH + sizeof(u_long))
#define INI_DATA (INI_GMT + sizeof(time_t))

#define DBMAX 64
#define CHAR_NUM '*'
extern UDBloq *N, *C, *I, *S, *L;
extern Udb *UDB_NICKS, *UDB_CANALES, *UDB_IPS, *UDB_SET, *UDB_LINKS;

extern Udb *BuscaBloque(char *, Udb *);
u_int LevelOperUdb(char *);
extern char bloques[];
extern u_int BDD_TOTAL;
extern UDBloq *ultimo;
extern Opts NivelesBDD[];
extern UDBloq *CogeDeId(u_int);
extern int CargaBloque(u_int);
extern void DescargaBloque(u_int);
extern int ParseaLinea(UDBloq *, char *, int);
extern int ActualizaHash(UDBloq *);
extern int OptimizaBloque(UDBloq *);
extern int ActualizaGMT(UDBloq *, time_t);
extern time_t LeeGMT(u_int);
extern u_long LeeHash(u_int);
extern int CargaBloques();
extern int IniciaUDB();
extern int TruncaBloque(UDBloq *, u_long);
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

#define C_FUN "F"	/* fundador */
#define C_MOD "M"	/* modos */
#define C_TOP "T"	/* topic */
#define C_ACC "A"	/* acceso */
#define C_FOR "B"	/* forbid */
#define C_SUS "S"	/* suspendido */
#define C_PAS "P"	/* pass */
#define C_DES "D"	/* desafio */
#define C_OPT "O"	/* opciones */
#define N_ALL "A"	/* acceso */
#define N_PAS "P"	/* pass */
#define N_VHO "V"	/* vhost */
#define N_FOR "B"	/* forbid */
#define N_SUS "S"	/* suspendido */
#define N_OPE "O"	/* oper */
#define N_DES "D"	/* desafio */
#define N_MOD "M"	/* modos */
#define N_SNO "K"	/* snomasks */
#define N_SWO "W"	/* swhois */
#define I_CLO "S"	/* nº clones */
#define I_NOL "E"	/* nolines */
#define I_HOS "H"	/* host reverso */
#define S_CLA "L"	/* clave de cifrado */
#define S_SUF "J"	/* sufijo */
#define S_NIC "N"	/* nickserv */
#define S_CHA "C"	/* chanserv */
#define S_IPS "I"	/* ipserv */
#define S_CLO "S"	/* nº clones global */
#define S_QIP "T"	/* quit por ip */
#define S_QCL "Q"	/* quit por clones */
#define S_DES "D"	/* desafio global */
#define S_FLO "F"	/* pass-flood */
#define L_OPT "O"	/* opciones */

#define C_OPT_PBAN 0x1
#define C_OPT_RMOD 0x2

#define L_OPT_DEBG 0x1
#define L_OPT_PROP 0x2
#define L_OPT_CLNT 0x4
