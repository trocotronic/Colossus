/*
 * $Id: ircd.h,v 1.9 2004-12-31 12:27:51 Trocotronic Exp $ 
 */

extern SOCKFUNC(inicia_ircd);
extern SOCKFUNC(procesa_ircd);
extern SOCKFUNC(cierra_ircd);

extern SOCKFUNC(procesa_smtp);

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
	struct _cliente *sig, *hsig, *prev;
	Sock *sck;
	char tipo;
	LinkCanal *canal;
	u_int canales;
	u_int numeric; /* numeric del cliente */
	u_int protocol;
	char *trio; /* representación alfanumérica del numeric del cliente (en b64 o lo que sea) */
	char *info;
	u_int nivel;
	//u_int intentos;
	//u_long ultimo_reg;
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
	Cliente *quien;
	char *ban;
	struct _ban *sig;
};
struct _canal
{
	char *nombre;
	u_long modos;
	char *topic;
	Cliente *ntopic;
	struct _canal *sig, *hsig, *prev;
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
	char *cmd;
	char *tok;
	IRCFUNC(*funcion[MAXMODULOS]);
	int funciones;
	int cuando;
	u_char params;
	struct _comando *sig;
};
#define MAXPARA 15
#define HOSTLEN 63
extern void inserta_comando(char *, char *, IRCFUNC(*), int, u_char);
extern int borra_comando(char *, IRCFUNC(*));
extern Comando *busca_comando(char *);
extern Cliente *busca_cliente(char *, Cliente *);
extern Canal *busca_canal(char *, Canal *);
extern Canal *info_canal(char *, int);
extern void sendto_serv(char *, ...);
extern void sendto_serv_us(Cliente *, char *, char *, char *, ...);
extern void response(Cliente *, Cliente *, char *, ...);
Cliente *nuevo_cliente(char *, char *, char *, char *, char *, char *, char *, char *);
extern void cambia_nick(Cliente *, char *);
extern void inserta_usuario_en_canal(Canal *, Cliente *);
extern void inserta_canal_en_usuario(Cliente *, Canal *);
extern int borra_canal_de_usuario(Cliente *, Canal *);
extern int borra_cliente_de_canal(Canal *, Cliente *);
#define INI 1
#define FIN 2
extern void procesa_modo(Cliente *, Canal *, char *[], int);
extern void inserta_ban_en_canal(Cliente *, Canal *, char *);
extern int borra_ban_de_canal(Canal *, char *);
extern void inserta_exc_en_canal(Cliente *, Canal *, char *);
extern int borra_exc_de_canal(Canal *, char *);
extern void inserta_modo_cliente_en_canal(LinkCliente **, Cliente *);
extern int borra_modo_cliente_de_canal(LinkCliente **, Cliente *);
extern void genera_mask(Cliente *);
extern void distribuye_me(Cliente *, Sock **);
extern void carga_modulos(void);
extern MODVAR Cliente me;
extern void inserta_bot(char *, char *, char *, char *, char *, char *, char *[], int, int);
#define IsReg(x) (x && _mysql_get_registro(NS_MYSQL, x, NULL))
#define IsId(x) (x && (x->nivel & USER))
#define IsRoot(x) (x && (x->nivel & ROOT) && IsId(x))
#define IsAdmin(x) (x && ((x->nivel & ADMIN) || IsRoot(x)))
#define IsIrcop(x) (x && ((x->nivel & IRCOP) || IsAdmin(x)))
#define IsOper(x) (x && ((x->nivel & OPER) || IsIrcop(x)))
#define IsPreo(x) (x && ((x->nivel & PREO) || IsOper(x)))
extern void procesa_umodos(Cliente *, char *);
extern void procesa_modos(Canal *, char *);
extern char *ircdmask(char *);
extern void mete_bot_en_canal(Cliente *, char *);
extern char *mascara(char *, int);
#define SIGN_MYSQL 1
#define SIGN_UMODE 2
#define SIGN_QUIT  3
#define SIGN_EOS 4
#define SIGN_MODE 5
#define SIGN_JOIN 6
#define SIGN_SYNCH 7
#define SIGN_KICK 8
#define SIGN_TOPIC 9
#define SIGN_NICK 10
#define SIGN_AWAY 11
#define SIGN_PART 12
#define EsCliente(x) (x->tipo == ES_CLIENTE)
#define EsServer(x) (x->tipo == ES_SERVER)
#define EsBot(x) (x->tipo == ES_BOT)
extern int es_link(LinkCliente *, Cliente *);
extern int es_linkcanal(LinkCanal *, Canal *);
#ifdef UDB
DLLFUNC void envia_registro_bdd(char *, ...);
#endif
extern int abre_sock_ircd(void);
extern MODVAR int intentos;
extern void libera_cliente_de_memoria(Cliente *);
extern void libera_canal_de_memoria(Canal *);
extern char backupbuf[BUFSIZE];
extern Cliente *botnick(char *, char *, char *, char *, char *, char *);
extern MODVAR Cliente *clientes;
extern Sock *SockIrcd, *EscuchaIrcd;
extern SOCKFUNC(escucha_abre);
extern char *crc_bdd(char *);
extern MODVAR Cliente *linkado;
extern void saca_bot_de_canal(Cliente *, char *, char *);
extern int mete_bots();
extern int mete_residentes();
extern void killbot(char *, char *);
extern void escucha_ircd();
extern void renick_bot(char *);
extern MODVAR time_t inicio;
extern void carga_comandos();
extern char *token(char *);
extern char *cmd(char *);
extern MODVAR char parabuf[BUFSIZE], modebuf[BUFSIZE];

#define ADD 1
#define DEL 2

typedef struct _modes mTab;
struct _modes
{
	u_long mode;
	char flag;
};

extern u_long flag2mode(char , mTab []);
extern char mode2flag(u_long , mTab []);
extern u_long flags2modes(u_long *, char *, mTab []);
extern char *modes2flags(u_long, mTab [], Canal *);

//Macros de compatibilidad
#define MSG_NULL NULL
#define TOK_NULL NULL
#define p_null NULL
#define COM_NULL { MSG_NULL , TOK_NULL , p_null }
#define port_msg(x) (protocolo->especiales + x)->msg
#define port_tok(x) (protocolo->especiales + x)->tok
#define port_func(x) (protocolo->especiales + x)->func
#define port_existe(x) port_func(x)
#define P_TRIO 0
#define P_MODO_USUARIO_LOCAL 1
#define P_MODO_USUARIO_REMOTO 2
#define P_MODO_CANAL 3
#define P_CAMBIO_USUARIO_LOCAL 4
#define P_CAMBIO_USUARIO_REMOTO 5
#define P_JOIN_USUARIO_LOCAL 6
#define P_JOIN_USUARIO_REMOTO 7
#define P_PART_USUARIO_LOCAL 8
#define P_PART_USUARIO_REMOTO 9
#define P_QUIT_USUARIO_LOCAL 10
#define P_QUIT_USUARIO_REMOTO 11
#define P_NUEVO_USUARIO 12
#define P_PRIVADO 13
#define P_CAMBIO_HOST_LOCAL 14
#define P_CAMBIO_HOST_REMOTO 15
#define P_FORB_NICK 16
#define P_LAG 17
#define P_WHOIS_ESPECIAL 18
#define P_GLINE 19
#define P_KICK 20
#define P_TOPIC 21
#define P_NOTICE 22
#define P_INVITE 23

#define port_umodo(x) (protocolo->umodos + x)->mode
#define port_cmodo(x) (protocolo->cmodos + x)->mode

#define TRIO(x) port_func(P_TRIO)(x)

//estas macros corresponden a la posición dentro de la tabla
// *** NO CAMBIAR EL ORDEN ***
#define UMODE_REGNICK 	port_umodo(0)
#define UMODE_NETADMIN	port_umodo(1)
#define UMODE_OPER		port_umodo(2)
#define UMODE_HELPOP		port_umodo(3)
#define UMODE_HIDE		port_umodo(4)
#ifdef UDB
#define UMODE_SUSPEND	port_umodo(5)
#endif

#define MODE_RGSTR		port_cmodo(0)
#define MODE_RGSTRONLY 	port_cmodo(1)
#define MODE_OPERONLY   	port_cmodo(2)
#define MODE_ADMONLY   	port_cmodo(3)

#define RedOverride		(conf_set->opts & NO_OVERRIDE)
