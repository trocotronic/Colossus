/*
 * $Id: bdd.h,v 1.13 2005-06-29 21:13:43 Trocotronic Exp $ 
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
	struct _udb *hsig, *up, *mid, *down;
};

MODFUNC extern MODVAR Udb *nicks;
MODFUNC extern MODVAR Udb *chans;
MODFUNC extern MODVAR Udb *ips;
MODFUNC extern MODVAR Udb *sets;
MODFUNC extern MODVAR u_int BDD_NICKS;
MODFUNC extern MODVAR u_int BDD_CHANS;
MODFUNC extern MODVAR u_int BDD_IPS;
MODFUNC extern MODVAR u_int BDD_SET;
MODFUNC extern MODVAR time_t gmts[128];

DLLFUNC extern Udb *BuscaRegistro(int, char *);
DLLFUNC extern Udb *BuscaBloque(char *, Udb *);
DLLFUNC int NivelOperBdd(char *);
MODFUNC extern MODVAR char bloques[];
MODFUNC extern MODVAR u_int BDD_TOTAL;

#define ID(x) (x->id >> 8)
#define LETRA(x) (x->id & 0xFF)

#define BDD_PREO 0x1
#define BDD_OPER 0x2
#define BDD_DEVEL 0x4
#define BDD_ADMIN 0x8
#define BDD_ROOT 0x10
#define CHAR_NUM '*'
DLLFUNC extern Udb *IdAUdb(int);
DLLFUNC extern u_int CharAId(char);
DLLFUNC extern u_char IdAChar(int);
DLLFUNC extern void CargaBloque(int);
DLLFUNC extern void DescargaBloque(int);
DLLFUNC extern int ParseaLinea(int, char *, int);
DLLFUNC extern int ActualizaHash(Udb *);
DLLFUNC extern int Optimiza(Udb *);
DLLFUNC extern int ActualizaGMT(Udb *, time_t);
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

#define C_FUN "fundador"
#define C_FUN_TOK "F"
#define C_MOD "modos"
#define C_MOD_TOK "M"
#define C_TOP "topic"
#define C_TOP_TOK "T"
#define C_ACC "accesos"
#define C_ACC_TOK "A"
#define C_FOR "forbid"
#define C_FOR_TOK "B"
#define C_SUS "suspendido"
#define C_SUS_TOK "S"
#define N_PAS "pass"
#define N_PAS_TOK "P"
#define N_VHO "vhost"
#define N_VHO_TOK "V"
#define N_FOR "forbid"
#define N_FOR_TOK "B"
#define N_SUS "suspendido"
#define N_SUS_TOK "S"
#define N_OPE "oper"
#define N_OPE_TOK "O"
#define N_DES "desafio"
#define N_DES_TOK "D"
#define N_MOD "modos"
#define N_MOD_TOK "M"
#define N_SNO "snomasks"
#define N_SNO_TOK "K"
#define N_SWO "swhois"
#define N_SWO_TOK "W"
#define I_CLO "clones"
#define I_CLO_TOK "S"
#define I_NOL "nolines"
#define I_NOL_TOK "E"
#define S_CLA "clave_cifrado"
#define S_CLA_TOK "L"
#define S_SUF "sufijo"
#define S_SUF_TOK "J"
#define S_NIC "NickServ"
#define S_NIC_TOK "N"
#define S_CHA "ChanServ"
#define S_CHA_TOK "C"
#define S_IPS "IpServ"
#define S_IPS_TOK "I"
#define S_CLO "clones"
#define S_CLO_TOK "S"
#define S_QIP "quit_ips"
#define S_QIP_TOK "T"
#define S_QCL "quit_clones"
#define S_QCL_TOK "Q"

#endif
