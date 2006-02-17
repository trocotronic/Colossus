/*
 * $Id: ircd.h,v 1.20 2006-02-17 19:19:02 Trocotronic Exp $ 
 */

extern SOCKFUNC(IniciaIrcd);
extern SOCKFUNC(ProcesaIrcd);
extern SOCKFUNC(CierraIrcd);

extern SOCKFUNC(ProcesaSmtp);

#define IRCFUNC(x) int (x)(Sock *sck, Cliente *cl, char *parv[], int parc)
#define MAXMODULOS 256
#define MAX_FUN 32
#define TCLIENTE 1
#define TSERVIDOR 2
#define TBOT 3

typedef struct _cliente Cliente;
typedef struct _linkcanal LinkCanal;
typedef struct _linkcliente LinkCliente;
typedef struct _canal Canal;
typedef struct _mascara Mascara;
typedef struct _mallacliente MallaCliente;
typedef struct _mallamascara MallaMascara;
typedef struct _mallaparam MallaParam;
/*!
 * @desc: Recurso de cliente. Estos clientes pueden ser usuarios o servidores.
 * @params: $nombre Nickname.
 	    $ident Username.
 	    $host Host real de conexión. Puede ser un host o una ip.
 	    $rvhost Host real resuelto del cliente. Siempre es un host.
 	    $ip Ip. Siempre es una ip.
 	    $server Servidor al que está conectado.
 	    $vhost Host virtual. Si no tiene, apunta NULL.
 	    $mask Máscara. Formato: nickname!username@hostname
 	    $vmask Máscara virtual. Formato: nickname!username@vhostname
 	    $modos Modos de usuario que utiliza. Están en forma de bits.
 	    $sck Recurso de conexión.
 	    $tipo Tipo de cliente.
 	    $canal Malla de canales.
 	    $canales Cantidad de canales en los que está.
 	    $numeric Numérico del cliente.
 	    $protocol Protocolo que usa.
 	    $trio Representación alfanumérica del cliente.
 	    $info Realname.
 * @cat: IRCd
 * @ver: EsCliente EsServidor EsBot TipoMascara MascaraIrcd
 !*/
struct _cliente
{
	Cliente *sig, *prev, *hsig;
	char *nombre;
	char *ident;
	char *host; /* siempre apunta al host del cliente que conecta (puede ser ip o host) */
	char *rvhost; /* *SIEMPRE* apunta al host resuelto: a host si ya es host o a un strdup del host resuelto si no resuelve, apunta a su ip */
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
	unsigned away:1;
};
/*!
 * @desc: Malla de clientes para canales.
 * @params: $user Cliente
 	    $sig Siguiente nodo de la malla.
 * @cat: IRCd
 !*/
struct _linkcliente
{
	LinkCliente *sig;
	Cliente *user;
};
/*!
 * @desc: Malla de máscaras para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $quien Cliente que ha puesto la máscara.
 	    $mascara Máscara.
 * @cat: IRCd
 !*/
struct _mascara
{
	Mascara *sig;
	Cliente *quien;
	char *mascara;
};
/*!
 * @desc: Malla de clientes por modo para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $flag Modo asociado a la malla.
 	    $malla Siguiente malla de modo.
 	    $clientes Cantidad de clientes en esta malla de modo.
 * @cat: IRCd
 !*/
struct _mallacliente
{
	MallaCliente *sig;
	char flag;
	LinkCliente *malla;
	u_int clientes;
};
/*!
 * @desc: Malla de máscaras por modo para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $flag Modo asociado a la malla.
 	    $malla Siguiente malla de modo.
	    $mascaras Cantidad de máscaras en esta malla.
 * @cat: IRCd
 !*/
struct _mallamascara
{
	MallaMascara *sig;
	char flag;
	Mascara *malla;
	u_int mascaras;
};
/*!
 * @desc: Malla de parámetros por modo de canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $flag Modo asociado a la malla.
 	    $param Siguiente malla de nodo.
 	    $params Cantidad de parámetros en esta malla.
 * @cat: IRCd
 !*/
struct _mallaparam
{
	MallaParam *sig;
	char flag;
	char *param;
	u_int params;
};
/*!
 * @desc: Recurso de canal.
 * @params: $nombre Nombre.
 	    $modos Modos de canal que utiliza. Están en forma de bits.
 	    $topic Topic.
 	    $ntopic Cliente que ha puesto el topic.
 	    $clave Clave del canal (modo +k).
 	    $limite Límite de usuarios del canal (modo +l).
 	    $flood Antiflood (modo +f).
 	    $link Canal de linkaje (modo +L).
 	    $mallacl Malla de clientes.
 	    $mallamk Malla de máscaras.
 	    $miembro Malla de usuarios en el canal.
 	    $miembros Cantidad de usuarios en el canal.
 * @cat: IRCd
 !*/
struct _canal
{
	Canal *sig, *hsig, *prev;
	char *nombre;
	u_long modos;
	char *topic;
	Cliente *ntopic;
	MallaParam *mallapm;
	MallaCliente *mallacl;
	MallaMascara *mallamk;
	LinkCliente *miembro;
	u_int miembros;
};
/*!
 * @desc: Malla de canales para usuarios.
 * @params: $sig Siguiente nodo de la malla.
 	    $chan Canal.
 * @cat: IRCd
 !*/
struct _linkcanal
{
	LinkCanal *sig;
	Canal *chan;
};
typedef struct _comando
{
	struct _comando *sig;
	char *cmd;
	char *tok;
	IRCFUNC(*funcion[MAXMODULOS]);
	int funciones;
	int cuando;
	u_char params;
}Comando;
extern Comando *comandos;
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
extern void InsertaMascara(Cliente *, Mascara **, char *);
extern int BorraMascaraDeCanal(Mascara **, char *);
extern void InsertaModoCliente(LinkCliente **, Cliente *);
extern int BorraModoCliente(LinkCliente **, Cliente *);
extern MallaCliente *BuscaMallaCliente(Canal *, char);
extern MallaMascara *BuscaMallaMascara(Canal *, char);
extern MallaParam *BuscaMallaParam(Canal *, char);
extern void GeneraMascara(Cliente *);
extern void DistribuyeMe(Cliente *, Sock **);
extern MODVAR Cliente me;
extern void inserta_bot(char *, char *, char *, char *, char *, char *, char *[], int, int);
/*!
 * @desc: Consulta si un usuario está registrado.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsReg(char *nick)
 * @ret: Devuelve 1 si está registrado; 0, si no.
 * @cat: IRCd
 !*/
#define IsReg(x) (x && SQLCogeRegistro(NS_SQL, x, NULL))
/*!
 * @desc: Consulta si un usuario está identificado
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsId(char *nick)
 * @ret: Devuelve 1 si está identificado; 0, si no.
 * @cat: IRCd
 !*/
#define IsId(x) (x && (x->nivel & N1))
/*!
 * @desc: Consulta si un usuario tiene el estado de root.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsRoot(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsRoot(x) (x && (x->nivel & N5) && IsId(x))
/*!
 * @desc: Consulta si un usuario tiene el estado de admin.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsAdmin(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsAdmin(x) (x && ((x->nivel & N4) || IsRoot(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de oper.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsOper(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsOper(x) (x && ((x->nivel & N3) || IsAdmin(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de preoper.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsPreo(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsPreo(x) (x && ((x->nivel & N2) || IsOper(x)))
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
#define SIGN_STARTUP 13
#define SIGN_SOCKOPEN 14
/*!
 * @desc: Devuelve 1 si el recurso es un cliente; 0, si no.
 * @params: $sck [in] Recurso de conexión.
 * @sntx: int EsCliente(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un cliente; 0, si no.
 * @cat: IRCd
 !*/
#define EsCliente(x) (x->tipo == TCLIENTE)
/*!
 * @desc: Devuelve 1 si el recurso es un servidor; 0, si no.
 * @params: $sck [in] Recurso de conexión.
 * @sntx: int EsServidor(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un servidor; 0, si no.
 * @cat: IRCd
 !*/
#define EsServidor(x) (x->tipo == TSERVIDOR)
/*!
 * @desc: Devuelve 1 si el recurso es un bot; 0, si no.
 * @params: $sck [in] Recurso de conexión.
 * @sntx: int EsBot(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un bot; 0, si no.
 * @cat: IRCd
 !*/
#define EsBot(x) (x->tipo == TBOT)
extern int EsLink(LinkCliente *, Cliente *);
extern int EsLinkCanal(LinkCanal *, Canal *);
extern int AbreSockIrcd(void);
extern MODVAR int intentos;
extern void LiberaMemoriaCliente(Cliente *);
extern void LiberaMemoriaCanal(Canal *);
extern char backupbuf[BUFSIZE];
extern Cliente *CreaBot(char *, char *, char *, char *, char *);
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
typedef struct _modes
{
	u_long mode;
	char flag;
}mTab;

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
