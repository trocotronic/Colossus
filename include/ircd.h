/*
 * $Id: ircd.h,v 1.17 2005-09-16 14:00:34 Trocotronic Exp $ 
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
typedef struct _ban Ban;
/*!
 * @desc: Recurso de cliente. Estos clientes pueden ser usuarios o servidores.
 * @params: $nombre Nickname.
 	    $ident Username.
 	    $host Host real de conexi�n. Puede ser un host o una ip.
 	    $rvhost Host real resuelto del cliente. Siempre es un host.
 	    $ip Ip. Siempre es una ip.
 	    $server Servidor al que est� conectado.
 	    $vhost Host virtual. Si no tiene, apunta NULL.
 	    $mask M�scara. Formato: nickname!username@hostname
 	    $vmask M�scara virtual. Formato: nickname!username@vhostname
 	    $modos Modos de usuario que utiliza. Est�n en forma de bits.
 	    $sck Recurso de conexi�n.
 	    $tipo Tipo de cliente.
 	    $canal Malla de canales.
 	    $canales Cantidad de canales en los que est�.
 	    $numeric Num�rico del cliente.
 	    $protocol Protocolo que usa.
 	    $trio Representaci�n alfanum�rica del cliente.
 	    $info Realname.
 * @cat: IRCd
 * @ver: EsCliente EsServidor EsBot TipoMascara MascaraIrcd
 !*/
typedef struct _cliente
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
	char *trio; /* representaci�n alfanum�rica del numeric del cliente (en b64 o lo que sea) */
	char *info;
	u_int nivel;
};
/*!
 * @desc: Malla de clientes para canales.
 * @params: $user Cliente
 	    $sig Siguiente nodo de la malla.
 * @cat: IRCd
 !*/
typedef struct _linkcliente
{
	LinkCliente *sig;
	Cliente *user;
};
/*!
 * @desc: Malla de m�scaras para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $quien Cliente que ha puesto la m�scara.
 	    $ban M�scara.
 * @cat: IRCd
 !*/
typedef struct _ban
{
	Ban *sig;
	Cliente *quien;
	char *ban;
};
/*!
 * @desc: Recurso de canal.
 * @params: $nombre Nombre.
 	    $modos Modos de canal que utiliza. Est�n en forma de bits.
 	    $topic Topic.
 	    $ntopic Cliente que ha puesto el topic.
 	    $clave Clave del canal (modo +k).
 	    $limite L�mite de usuarios del canal (modo +l).
 	    $flood Antiflood (modo +f).
 	    $link Canal de linkaje (modo +L).
 	    $ban Malla de bans (modo +b).
 	    $exc Malla de excepciones (modo +e).
 	    $owner Malla de owners (modo +q).
 	    $admin Malla de admins (modo +a).
 	    $op Malla de operadores (modo +o).
 	    $half Malla de semioperadores (modo +h).
 	    $voz Malla de voces (modo +v).
 	    $miembro Malla de usuarios en el canal.
 	    $miembros Cantidad de usuarios en el canal.
 * @cat: IRCd
 !*/
typedef struct _canal
{
	Canal *sig, *hsig, *prev;
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
/*!
 * @desc: Malla de canales para usuarios.
 * @params: $sig Siguiente nodo de la malla.
 	    $chan Canal.
 * @cat: IRCd
 !*/
typedef struct _linkcanal
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
extern void InsertaBan(Cliente *, Ban **, char *);
extern int BorraBanDeCanal(Ban **, char *);
extern void InsertaModoCliente(LinkCliente **, Cliente *);
extern int BorraModoCliente(LinkCliente **, Cliente *);
extern void GeneraMascara(Cliente *);
extern void DistribuyeMe(Cliente *, Sock **);
extern void CargaModulos(void);
extern MODVAR Cliente me;
extern void inserta_bot(char *, char *, char *, char *, char *, char *, char *[], int, int);
/*!
 * @desc: Consulta si un usuario est� registrado.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsReg(char *nick)
 * @ret: Devuelve 1 si est� registrado; 0, si no.
 * @cat: IRCd
 !*/
#define IsReg(x) (x && SQLCogeRegistro(NS_SQL, x, NULL))
/*!
 * @desc: Consulta si un usuario est� identificado
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsId(char *nick)
 * @ret: Devuelve 1 si est� identificado; 0, si no.
 * @cat: IRCd
 !*/
#define IsId(x) (x && (x->nivel & USER))
/*!
 * @desc: Consulta si un usuario tiene el estado de root.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsRoot(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsRoot(x) (x && (x->nivel & ROOT) && IsId(x))
/*!
 * @desc: Consulta si un usuario tiene el estado de admin.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsAdmin(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsAdmin(x) (x && ((x->nivel & ADMIN) || IsRoot(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de ircop.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsIrcop(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsIrcop(x) (x && ((x->nivel & IRCOP) || IsAdmin(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de oper.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsOper(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsOper(x) (x && ((x->nivel & OPER) || IsIrcop(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de preoper.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsPreo(char *nick)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
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
/*!
 * @desc: Devuelve 1 si el recurso es un cliente; 0, si no.
 * @params: $sck [in] Recurso de conexi�n.
 * @sntx: int EsCliente(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un cliente; 0, si no.
 * @cat: IRCd
 !*/
#define EsCliente(x) (x->tipo == TCLIENTE)
/*!
 * @desc: Devuelve 1 si el recurso es un servidor; 0, si no.
 * @params: $sck [in] Recurso de conexi�n.
 * @sntx: int EsServidor(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un servidor; 0, si no.
 * @cat: IRCd
 !*/
#define EsServidor(x) (x->tipo == TSERVIDOR)
/*!
 * @desc: Devuelve 1 si el recurso es un bot; 0, si no.
 * @params: $sck [in] Recurso de conexi�n.
 * @sntx: int EsBot(Sock *sck)
 * @ret: Devuelve 1 si el recurso es un bot; 0, si no.
 * @cat: IRCd
 !*/
#define EsBot(x) (x->tipo == TBOT)
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

/* TKLines y dem�s */
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
