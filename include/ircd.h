/*
 * $Id: ircd.h,v 1.2 2004-09-11 15:54:07 Trocotronic Exp $ 
 */

#include "flags.h"
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
	char *host; /* siempre apunta al host del cliente que conecta */
	char *rvhost; /* *SIEMPRE* apunta al host resuelto: a host si ya es host o a un strdup del host resuelto */
	Cliente *server;
	char *vhost;
	char *mask;
	char *vmask;
	u_long modos;
	struct _cliente *sig, *hsig, *prev;
	Sock *sck;
	char tipo;
	LinkCanal *canal;
	int canales;
	int numeric;
	int protocol;
	char *opts;
	char *info;
	int nivel;
	int intentos;
	long ultimo_reg;
	/* stop, estas medidas de prevencion no sirven si el usuario reconecta, porque se borran.
	   aun asi, si un usuario quiere registarr 3 nicks en 5 minutos, tampoco se lo prohibiremos.
	   para restricciones ya esta el limite de nicks por cabeza. */
	char *fundador[MAX_FUN];
	int fundadores;
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
	int limite;
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
	int miembros;
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
	int (*funcion[MAXMODULOS])(Sock *sck, Cliente *cl, char *parv[], int parc);
	int funciones;
	int cuando;
	struct _comando *sig;
};

extern void carga_comandos(void);
extern void inserta_comando(char *, char *, IRCFUNC(*), int);
extern int borra_comando(char *, IRCFUNC(*));
extern Cliente *busca_cliente(char *, Cliente *);
extern Canal *busca_canal(char *, Canal *);
extern Canal *info_canal(char *, int);
extern void sendto_serv(char *, ...);
extern void sendto_serv_us(Cliente *, char *, char *, char *, ...);
Cliente *nuevo_cliente(char *, char *, char *, char *, char *, char *, char *);
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
extern IRCFUNC(sincroniza);
extern void distribuye_me(Cliente *, Sock **);
extern void carga_modulos(void);
extern MODVAR Cliente me;
extern void inserta_bot(char *, char *, char *, char *, char *, char *, char *[], int, int);
#define IsReg(x) (x && _mysql_get_registro(NS_MYSQL, x, NULL))
#define IsId(x) (x && (x->modos & UMODE_REGNICK))
#define IsRoot(x) (x && !strcasecmp(x->nombre, conf_set->root) && IsId(x))
#define IsAdmin(x) (x && ((x->modos & UMODE_NETADMIN) || IsRoot(x)))
#define IsIrcop(x) (x && ((x->modos & UMODE_OPER) || IsAdmin(x)))
#define IsOper(x) (x && ((x->modos & UMODE_HELPOP) || IsIrcop(x)))
#define IsPreo(x) (x && ((x->nivel & PREO) || IsOper(x)))
extern void envia_umodos(Cliente *, char *, char *);
extern void envia_cmodos(Canal *, char *, char *, ...);
extern void irckill(Cliente *, char *, char *, ...);
extern void procesa_umodos(Cliente *, char *);
extern void procesa_modos(Canal *, char *);
extern char *ircdmask(char *);
extern void irckick(char *, Cliente *, Canal *, char *, ...);
#define TKL_ADD 1
#define TKL_DEL 2
#define TKL_GLINE 'G'
#define TKL_ZAP   'Z'
#define TKL_SHUN  's'
#define TKL_QLINE 'Q'
extern void irctkl(char , char , char *, char *, char *, int , char *);
extern void irctopic(char *, Canal *, char *);
extern void mete_bot_en_canal(Cliente *, char *);
extern char *mascara(char *, int);
#define SIGN_MYSQL 1
#define SIGN_UMODE 2
#define SIGN_QUIT  3
#define SIGN_EOS 4
#define SIGN_CREATE_CHAN 5
#define SIGN_DESTROY_CHAN 6
extern char *cifranick(char *, char *);
#define EsCliente(x) (x->tipo == ES_CLIENTE)
#define EsServer(x) (x->tipo == ES_SERVER)
#define EsBot(x) (x->tipo == ES_BOT)
extern int es_link(LinkCliente *, Cliente *);
#ifdef UDB
DLLFUNC void envia_registro_bdd(char *, ...);
#endif
extern int abre_sock_ircd(void);
extern int intentos;
extern void libera_cliente_de_memoria(Cliente *);
extern void libera_canal_de_memoria(Canal *);
extern char backupbuf[BUFSIZE];
extern int reinicia(void);
extern Cliente *botnick(char *, char *, char *, char *, char *, char *);
extern MODVAR Cliente *clientes;
extern Sock *SockIrcd, *EscuchaIrcd;
extern SOCKFUNC(escucha_abre);
extern char *crc_bdd(char *);
extern MODVAR Cliente *link;
extern void saca_bot_de_canal(Cliente *, char *, char *);
