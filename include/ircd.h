/*
 * $Id: ircd.h,v 1.15 2005-07-13 14:06:22 Trocotronic Exp $ 
 */

extern SOCKFUNC(IniciaIrcd);
extern SOCKFUNC(ProcesaIrcd);
extern SOCKFUNC(CierraIrcd);

extern SOCKFUNC(ProcesaSmtp);

#define IRCFUNC(x) int (x)(Sock *sck, Cliente *cl, char *parv[], int parc)
#define MAXMODULOS 256
#define MAX_FUN 32
#define ES_CLIENTE 1
#define ES_SERVER 2
#define ES_BOT 3

typedef struct _cliente Cliente;
typedef struct _canal Canal;
typedef struct _linkcliente LinkCliente;
typedef struct _linkcanal LinkCanal;
typedef struct _ban Ban;
struct _cliente
{
	struct _cliente *sig, *prev, *hsig;
	char *nombre;
	char *ident;
	char *host; /* siempre apunta al host del cliente que conecta (puede ser ip o host) */
	char *rvhost; /* *SIEMPRE* apunta al host resuelto: a host si ya es host o a un strdup del host resuelto 
			   si no resuelve, apunta a su ip */
	char *ip; /* *SIEMPRE* apunta a la ip */
	Cliente *server;
	char *vhost;
	char *mask;
	char *vmask;
	u_long modos;
	Sock *sck;
	char tipo;
	LinkCanal *canal;
	u_int canales;
	u_int numeric; /* numeric del cliente */
	u_int protocol;
	char *trio; /* representación alfanumérica del numeric del cliente (en b64 o lo que sea) */
	char *info;
	u_int nivel;
	/* stop, estas medidas de prevencion no sirven si el usuario reconecta, porque se borran.
	   aun asi, si un usuario quiere registarr 3 nicks en 5 minutos, tampoco se lo prohibiremos.
	   para restricciones ya esta el limite de nicks por cabeza. */
	//char *fundador[MAX_FUN];
	//u_int fundadores;
};
struct _linkcliente
{
	LinkCliente *sig;
	Cliente *user;
};
struct _ban
{
	struct _ban *sig;
	Cliente *quien;
	char *ban;
};
struct _canal
{
	struct _canal *sig, *hsig, *prev;
	char *nombre;
	u_long modos;
	char *topic;
	Cliente *ntopic;
	char *clave;
	u_int limite;
	char *flood;
	char *link;
	Ban *ban;
	Ban *exc;
	LinkCliente *owner;
	LinkCliente *admin;
	LinkCliente *op;
	LinkCliente *half;
	LinkCliente *voz;
	LinkCliente *miembro;
	u_int miembros;
};
struct _linkcanal
{
	LinkCanal *sig;
	Canal *chan;
};
typedef struct _comando Comando;
extern Comando *comandos;
struct _comando
{
	struct _comando *sig;
	char *cmd;
	char *tok;
	IRCFUNC(*funcion[MAXMODULOS]);
	int funciones;
	int cuando;
	u_char params;
};
#define MAXPARA 15
#define HOSTLEN 63
extern void InsertaComando(char *, char *, IRCFUNC(*), int, u_char);
extern int BorraComando(char *, IRCFUNC(*));
extern Comando *BuscaComando(char *);
extern Cliente *BuscaCliente(char *, Cliente *);
extern Canal *BuscaCanal(char *, Canal *);
extern Canal *InfoCanal(char *, int);
extern void EnviaAServidor(char *, ...);
extern void Responde(Cliente *, Cliente *, char *, ...);
Cliente *NuevoCliente(char *, char *, char *, char *, char *, char *, char *, char *);
extern void CambiaNick(Cliente *, char *);
extern void InsertaClienteEnCanal(Canal *, Cliente *);
extern void InsertaCanalEnCliente(Cliente *, Canal *);
extern int BorraCanalDeCliente(Cliente *, Canal *);
extern int BorraClienteDeCanal(Canal *, Cliente *);
#define INI 1
#define FIN 2
extern void InsertaBan(Cliente *, Canal *, char *);
extern int BorraBanDeCanal(Canal *, char *);
extern void inserta_exc_en_canal(Cliente *, Canal *, char *);
extern int borra_exc_de_canal(Canal *, char *);
extern void InsertaModoCliente(LinkCliente **, Cliente *);
extern int BorraModoCliente(LinkCliente **, Cliente *);
extern void GeneraMascara(Cliente *);
extern void DistribuyeMe(Cliente *, Sock **);
extern void CargaModulos(void);
extern MODVAR Cliente me;
extern void inserta_bot(char *, char *, char *, char *, char *, char *, char *[], int, int);
#define IsReg(x) (x && SQLCogeRegistro(NS_SQL, x, NULL))
#define IsId(x) (x && (x->nivel & USER))
#define IsRoot(x) (x && (x->nivel & ROOT) && IsId(x))
#define IsAdmin(x) (x && ((x->nivel & ADMIN) || IsRoot(x)))
#define IsIrcop(x) (x && ((x->nivel & IRCOP) || IsAdmin(x)))
#define IsOper(x) (x && ((x->nivel & OPER) || IsIrcop(x)))
#define IsPreo(x) (x && ((x->nivel & PREO) || IsOper(x)))
extern void ProcesaModosCliente(Cliente *, char *);
extern char *MascaraIrcd(char *);
extern void EntraBot(Cliente *, char *);
extern char *TipoMascara(char *, int);
#define SIGN_SQL 0
#define SIGN_UMODE 1
#define SIGN_QUIT  2
#define SIGN_EOS 3
#define SIGN_MODE 4
#define SIGN_JOIN 5
#define SIGN_SYNCH 6
#define SIGN_KICK 7
#define SIGN_TOPIC 8
#define SIGN_PRE_NICK 9
#define SIGN_POST_NICK 10
#define SIGN_AWAY 11
#define SIGN_PART 12
#define EsCliente(x) (x->tipo == ES_CLIENTE)
#define EsServer(x) (x->tipo == ES_SERVER)
#define EsBot(x) (x->tipo == ES_BOT)
extern int EsLink(LinkCliente *, Cliente *);
extern int EsLinkCanal(LinkCanal *, Canal *);
#ifdef UDB
DLLFUNC void PropagaRegistro(char *, ...);
#endif
extern int AbreSockIrcd(void);
extern MODVAR int intentos;
extern void LiberaMemoriaCliente(Cliente *);
extern void LiberaMemoriaCanal(Canal *);
extern char backupbuf[BUFSIZE];
extern Cliente *CreaBot(char *, char *, char *, char *, char *, char *);
extern MODVAR Cliente *clientes;
extern Sock *SockIrcd, *IrcdEscucha;
extern SOCKFUNC(EscuchaAbre);
extern char *crc_bdd(char *);
extern MODVAR Cliente *linkado;
extern void SacaBot(Cliente *, char *, char *);
extern int EntraBots();
extern int EntraResidentes();
extern void DesconectaBot(char *, char *);
extern void EscuchaIrcd();
extern void ReconectaBot(char *);
extern MODVAR time_t inicio;
extern void carga_comandos();
extern char *Token(char *);
extern char *Cmd(char *);
extern MODVAR char parabuf[BUFSIZE], modebuf[BUFSIZE];
typedef struct _modes mTab;
struct _modes
{
	u_long mode;
	char flag;
};

extern u_long FlagAModo(char , mTab []);
extern char ModoAFlag(u_long , mTab []);
extern u_long FlagsAModos(u_long *, char *, mTab []);
extern char *ModosAFlags(u_long, mTab [], Canal *);

#define RedOverride		(conf_set->opts & NO_OVERRIDE)

/* TKLines y demás */
typedef struct _tkl
{
	struct _tkl *sig;
	int tipo;
	char *mascara;
	char *autor;
	char *motivo;
	time_t inicio, fin;
}Tkl;
Tkl *InsertaTKL(int, char *, char *, char *, char *, time_t, time_t);
int BorraTKL(Tkl **, char *, char *);
