/*
 * $Id: ircd.h,v 1.41 2008/05/03 12:01:55 Trocotronic Exp $
 */

#include "hash.h"
#include "iploc.h"

extern SOCKFUNC(IniciaIrcd);
extern SOCKFUNC(ProcesaIrcd);
extern SOCKFUNC(CierraIrcd);

extern SOCKFUNC(ProcesaSmtp);

#define IRCFUNC(x) int (x)(Sock *sck, Cliente *cl, char *parv[], int parc)
#define MAXMODULOS 32
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
 	    $host Host real de conexi�n. Puede ser un host o una ip.
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
 	    $away Si est� away, apunta al motivo away. Si no, a NULL.
 	    $datos Para guardar todo tipo de datos.
 	    $loc Estructura de tipo IPLoc que contiene la localizaci�n geogr�fica del usuario.
 * @cat: IRCd
 * @ver: EsCliente EsServidor EsBot TipoMascara MascaraIrcd
 !*/
struct _cliente
{
	Cliente *sig, *prev, *hsig;
	char *nombre;
	char *ident;
	char *host; /* siempre apunta al host del cliente que conecta (puede ser ip o host) */
	char *ip; /* *SIEMPRE* apunta a la ip */
	Cliente *server;
	char *vhost;
	char *mask;
	char *vmask;
	u_long modos;
	Sock *sck;
	u_int tipo;
	LinkCanal *canal;
	u_int canales;
	int numeric; /* numeric del cliente */
	u_int protocol;
	char *trio; /* representaci�n alfanum�rica del numeric del cliente (en b64 o lo que sea) */
	char *info;
	char *away;
	u_int nivel;
	char *datos;
	IPLoc *loc;
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
	Cliente *cl;
};
/*!
 * @desc: Malla de m�scaras para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $quien Cliente que ha puesto la m�scara.
 	    $mascara M�scara.
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
 * @desc: Malla de m�scaras por modo para canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $flag Modo asociado a la malla.
 	    $malla Siguiente malla de modo.
	    $mascaras Cantidad de m�scaras en esta malla.
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
 * @desc: Malla de par�metros por modo de canales.
 * @params: $sig Siguiente nodo de la malla.
 	    $flag Modo asociado a la malla.
 	    $param Par�metro asociado al modo.
 	    $params Cantidad de par�metros en esta malla.
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
 	    $modos Modos de canal que utiliza. Est�n en forma de bits.
 	    $topic Topic.
 	    $ntopic Cliente que ha puesto el topic.
	    $mallapm Malla de par�metros.
 	    $mallacl Malla de clientes.
 	    $mallamk Malla de m�scaras.
 	    $miembro Malla de usuarios en el canal.
 	    $miembros Cantidad de usuarios en el canal.
 	    $creacion Fecha en formato UNIX de creaci�n.
 	    $privado El canal es de entrada restringida (ya sea por +k, +i, etc.)
 * @cat: IRCd
 !*/
struct _canal
{
	Canal *sig, *prev, *hsig;
	char *nombre;
	u_long modos;
	char *topic;
	Cliente *ntopic;
	MallaParam *mallapm;
	MallaCliente *mallacl;
	MallaMascara *mallamk;
	LinkCliente *miembro;
	u_int miembros;
	time_t creacion;
	unsigned privado:1;
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
	Canal *cn;
};
typedef struct _comando
{
	struct _comando *sig;
	char *cmd;
	char *tok;
	IRCFUNC(*funcion[MAXMODULOS]);
	int funciones;
	int cuando;
	int params;
}Comando;
extern Comando *comandos;
#define MAXPARA 15
#define HOSTLEN 63
extern void InsertaComando(char *, char *, IRCFUNC(*), int, int);
extern int BorraComando(char *, IRCFUNC(*));
extern Comando *BuscaComando(char *);
extern Cliente *BuscaCliente(char *);
extern Canal *BuscaCanal(char *);
extern Canal *CreaCanal(char *);
extern void EnviaAServidor(char *, ...);
extern void Responde(Cliente *, Cliente *, char *, ...);
Cliente *NuevoCliente(char *, char *, char *, char *, char *, char *, char *, char *);
extern void CambiaNick(Cliente *, char *);
extern void InsertaClienteEnCanal(Canal *, Cliente *);
extern void InsertaCanalEnCliente(Cliente *, Canal *);
extern int BorraCanalDeCliente(Cliente *, Canal *);
extern int BorraClienteDeCanal(Canal *, Cliente *);
extern void InsertaMascara(Cliente *, Mascara **, char *);
extern int BorraMascaraDeCanal(Mascara **, char *);
extern void InsertaModoCliente(LinkCliente **, Cliente *);
extern int BorraModoCliente(LinkCliente **, Cliente *);
extern MallaCliente *BuscaMallaCliente(Canal *, char);
extern MallaMascara *BuscaMallaMascara(Canal *, char);
extern MallaParam *BuscaMallaParam(Canal *, char);
extern void GeneraMascara(Cliente *);
extern void DistribuyeMe(Cliente *);
extern MODVAR Cliente me;

int NickReg(char *);
/*!
 * @desc: Consulta si un usuario est� registrado.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsReg(char *nick)
 * @ret: Devuelve 1 si est� registrado; 0, si no.
 * @cat: IRCd
 !*/
#define IsReg(x) (NickReg(x) != 0)
/*!
 * @desc: Consulta si un usuario est� registrado.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsActivo(char *nick)
 * @ret: Devuelve 1 si est� activo; 0, si no.
 * @cat: IRCd
 !*/
#define IsActivo(x)(NickReg(x) == 1)
/*!
 * @desc: Consulta si un usuario est� suspendido.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsSusp(char *nick)

 * @ret: Devuelve 1 si est� suspendido; 0, si no.
 * @cat: IRCd
 !*/
#define IsSusp(x) (NickReg(x) == 2)
/*!
 * @desc: Consulta si un usuario est� prohibido.
 * @params: $nick [in] Nick del usuario.
 * @sntx: int IsForbid(char *nick)

 * @ret: Devuelve 1 si est� prohibido; 0, si no.
 * @cat: IRCd
 !*/
#define IsForbid(x) (NickReg(x) == 3)
/*!
 * @desc: Consulta si un usuario est� identificado
 * @params: $cl [in] Cliente
 * @sntx: int IsId(Cliente *cl)
 * @ret: Devuelve 1 si est� identificado; 0, si no.
 * @cat: IRCd
 !*/
#define IsId(x) (x && (x->nivel & N1))
/*!
 * @desc: Consulta si un usuario tiene el estado de root.
 * @params: $cl [in] Cliente
 * @sntx: int IsRoot(Cliente *cl)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsRoot(x) (x && (x->nivel & N5) && IsId(x))
/*!
 * @desc: Consulta si un usuario tiene el estado de admin.
 * @params: $cl [in] Cliente
 * @sntx: int IsAdmin(Cliente *cl)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsAdmin(x) (x && ((x->nivel & N4) || IsRoot(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de oper.
 * @params: $cl [in] Cliente
 * @sntx: int IsOper(Cliente *cl)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsOper(x) (x && ((x->nivel & N3) || IsAdmin(x)))
/*!
 * @desc: Consulta si un usuario tiene el estado de preoper.
 * @params: $cl [in] Cliente
 * @sntx: int IsPreo(Cliente *cl)
 * @ret: Devuelve 1 tiene ese estado; 0, si no.
 * @cat: IRCd
 !*/
#define IsPreo(x) (x && ((x->nivel & N2) || IsOper(x)))
extern void ProcesaModosCliente(Cliente *, char *);
extern char *MascaraIrcd(char *);
extern Canal *EntraBot(Cliente *, char *);
extern char *MascaraTKL(char *, char *);
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
#define SIGN_CDESTROY 15
#define SIGN_CMSG 16
#define SIGN_PMSG 17
#define SIGN_SERVER 18
#define SIGN_SOCKCLOSE 19
#define SIGN_SQUIT 20
#define SIGN_RAW 21
#define SIGN_CCREATE 22
#define SIGN_CLOSE 23

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
#define EsBot(x) (x && x->tipo == TBOT)
extern int EsLink(LinkCliente *, Cliente *);
extern int EsLinkCanal(LinkCanal *, Canal *);
extern VOIDSIG AbreSockIrcd();
extern MODVAR int intentos;
extern void LiberaMemoriaCliente(Cliente *);
extern void LiberaMemoriaCanal(Canal *);
extern char backupbuf[BUFSIZE];
extern Cliente *CreaBot(char *, char *, char *, char *, char *);
extern Cliente *CreaBotEx(char *, char *, char *, char *, char *, char *);
extern MODVAR Cliente *clientes;
extern MODVAR Canal *canales;
extern MODVAR LinkCliente *servidores;
extern MODVAR Sock *SockIrcd;
extern MODVAR Sock *IrcdEscucha;
extern SOCKFUNC(EscuchaAbre);
extern char *crc_bdd(char *);
extern MODVAR Cliente *linkado;
extern void SacaBot(Cliente *, char *, char *);
extern int EntraBots();
extern int EntraResidentes();
extern void DesconectaBot(Cliente *, char *);
extern void EscuchaIrcd();
extern void ReconectaBot(char *);
extern Cliente *CreaServidor(char *, char *);
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
extern Tkl *InsertaTKL(int, char *, char *, char *, char *, time_t, time_t);
extern int BorraTKL(Tkl **, char *, char *);
extern Tkl *BuscaTKL(char*, Tkl *);
#define TKL_MAX 16 /* m�ximo! */
extern MODVAR Tkl *tklines[TKL_MAX];
