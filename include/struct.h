/*
 * $Id: struct.h,v 1.9 2004-09-24 22:41:11 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <Winsock.h>
#include <direct.h>
#include "mysql.h"
#include <sys/timeb.h>
#include <process.h>
#else
#define DWORD int
#include <mysql.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifndef MODVAR
 #if defined(MODULE_COMPILE) && defined(_WIN32)
  #define MODVAR __declspec(dllimport)
 #else
  #define MODVAR
 #endif
#endif
#ifdef _WIN32
#define DLLFUNC __declspec(dllexport)
#define irc_dlopen(x,y) LoadLibrary(x)
#define irc_dlclose FreeLibrary
#define irc_dlsym(x,y,z) z = (void *)GetProcAddress(x,y)
#define irc_dlerror our_dlerror
#else
#define irc_dlopen dlopen
#define irc_dlclose dlclose
#define irc_dlsym(x,y,z) z = dlsym(x,y)
#define irc_dlerror dlerror
#define DLLFUNC 
#endif

#include "sprintf_irc.h"
#include "parseconf.h"
#include "md5.h"
#ifdef USA_ZLIB
#include "zlib.h"
#endif

extern void carga_socks(void);
extern int strcasecmp(const char *, const char *);
extern void ircstrdup(char **, const char *);
#ifdef DEBUG
#define Free(x) free(x); Debug("Liberando %X", x)
#else
#define Free free
#endif
#define SOCKFUNC(x) int (x)(Sock *sck, char *data)
#define MAXSOCKS 1024
#define BUFSIZE 8192
#define ToLower(c) (NTL_tolower_tab[(int)(c)])
#define ToUpper(c) (NTL_toupper_tab[(int)(c)])
#define u_long unsigned long
#define u_char unsigned char
#define u_int unsigned int
#define u_short unsigned short
#define BadPtr(x) (!(x) || (*(x) == '\0'))
typedef struct _dbuf DBuf;

struct _dbuf
{
	u_int len; /* para datos binarios */
	u_int slots;
	struct _dbufdata *wslot, *rslot; /* al slot de escritura y lectura: nunca wslot < rslot */
	char *wchar, *rchar; /* a la posición para escribir y leer */
};
typedef struct _dbufdata DbufData;
struct _dbufdata
{
	char data[2032];
	u_int len;
	struct _dbufdata *sig, *prev;
};
#ifdef USA_ZLIB
#define ZIP_MINIMUM     4096
#define ZIP_MAXIMUM     8192
typedef struct _zlib Zlib;
struct _zlib
{
	z_stream *in;
	z_stream *out;
	char inbuf[ZIP_MAXIMUM];
	char outbuf[ZIP_MAXIMUM];
	int incount;
	int outcount;
};
#endif
typedef struct _sock Sock;
struct _sock
{
	char *host;
	int puerto;
	int pres;
	struct sockaddr_in server;
	SOCKFUNC(*openfunc);
	SOCKFUNC(*readfunc);
	SOCKFUNC(*closefunc);
	SOCKFUNC(*writefunc);
	DBuf *recvQ;
	DBuf *sendQ;
	char estado;
	int opts;
#ifdef USA_ZLIB
	Zlib *zlib;
#endif
};
extern Sock *sockopen(char *, int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), int);
extern void sockwrite(Sock *, int, char *, ...);
extern void sockclose(Sock *, char);

typedef struct _hash Hash;
struct _hash {
	int items;
	void *item;
};

extern const char NTL_tolower_tab[];
extern MODVAR char buf[BUFSIZE];
extern Sock *SockActual;
extern struct _cliente *busca_cliente_en_hash(char *, struct _cliente *);
extern struct _canal *busca_canal_en_hash(char *, struct _canal *);
extern void inserta_cliente_en_hash(struct _cliente*, char *);
extern void inserta_canal_en_hash(struct _canal *, char *);
extern int borra_cliente_de_hash(struct _cliente *, char *);
extern int borra_canal_de_hash(struct _canal *, char *);
extern int match(char *, char *);
#define bzero(x,y) memset(x,0,y)
extern char *_asctime(time_t *);
#define PROTOCOL 2304
extern u_long our_crc32(const u_char *, u_int);
#define abs(x) (x < 0) ? -x : x

extern int carga_mysql();
extern MODVAR MYSQL *mysql;
extern MYSQL_RES *_mysql_query(char *format, ...);
extern char *_mysql_get_registro(char *, char *, char *);
extern char *_mysql_get_num(MYSQL *, char *, int, char *);
extern void _mysql_add(char *, char *, char *, char *, ...);
extern void _mysql_del(char *, char *);
extern int _mysql_restaura(void);
extern int _mysql_backup(void);
struct mysql_t
{
	char *tabla[256]; /* espero que no haya tantas tablas */
	int tablas;
};
extern MODVAR struct mysql_t mysql_tablas;
typedef struct _smtpData SmtpData;
struct _smtpData
{
	char *para;
	char *de;
	char *tema;
	char *cuerpo;
	char enviado;
	int intentos;
};
extern void envia_email(char *, char *, char *, char *);
extern void email(char *, char *, char *, ...);

extern char *random_ex(char *);
extern int randomiza(int, int);
extern char *coge_mx(char *);

/* senyals */
typedef struct _senyal Senyal;
struct _senyal
{
	short senyal;
	int (*func)();
	struct _senyal *sig;
};
#define MAXSIGS 256
extern MODVAR Senyal *senyals[MAXSIGS];
extern void inserta_senyal(short, int (*)());
extern int borra_senyal(short, int (*)());
#define senyal(s) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(); } while(0)
#define senyal1(s,x) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x); } while(0)
#define senyal2(s,x,y) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y); } while(0)

/* timers */
typedef struct _timer Timer;
struct _timer
{
	char *nombre;
	int (*func)();
	void *args;
	Sock *sck;
	double cuando;
	int veces;
	int lleva;
	int cada;
	struct _timer *sig;
};
extern void timer(char *, Sock *, int, int, int (*)(), void *, size_t);
extern int timer_off(char *, Sock *);
extern void comprueba_timers(void);
extern double microtime(void);
extern char *str_replace(char *, char, char);
extern char *strtolower(char *);
extern char *strtoupper(char *);
extern char *implode(char *[], int, int, int);
extern char *gettok(char *, int, char);
typedef struct _proc Proc;
struct _proc
{
	int (*func)();
	int proc;
	time_t time;
	struct _proc *sig;
};
extern void procesos_auxiliares(void);
extern void proc(int(*)());
#define CHMAX 2048
#define UMAX 2048
#define INI_FACT 8
#define INI_SUMD 0xFF
extern u_int hash_cliente(char *);
extern u_int hash_canal(char *);
extern Hash uTab[], cTab[];
#define COLOSSUS_VERNUM "1.0a"
#define COLOSSUS_VERSION "Colossus v" COLOSSUS_VERNUM
extern char **margv;
#define Malloc(x) StsMalloc(x, __FILE__, __LINE__)
#define da_Malloc(p,s) p = (s *)Malloc(sizeof(s)); bzero(p, sizeof(s))
#define EST_DESC 0
#define EST_CONN 1
#define EST_LIST 2
#define EST_OK   3
#define EST_CERR 4
#define EsDesc(x) (x->estado == EST_DESC)
#define EsConn(x) (x->estado == EST_CONN)
#define EsList(x) (x->estado == EST_LIST)
#define EsOk(x)   (x->estado == EST_OK)
#define EsCerr(x) (x->estado == EST_CERR)
#define SockDesc(x) x->estado = EST_DESC
#define SockConn(x) x->estado = EST_CONN
#define SockList(x) x->estado = EST_LIST
#define SockOk(x)   x->estado = EST_OK
#define SockCerr(x) x->estado = EST_CERR
#define FERR 1
#define FADV 2
#define FOK  3
#define FSQL 4
extern void fecho(char , char *, ...);
extern void Debug(char *, ...);
extern char *StsMalloc(size_t, char *, long);
#define ADD 1
#define DEL 2
extern struct in_addr *resolv(char *);
#define MYSQL_CACHE "cache"
extern MODVAR char tokbuf[BUFSIZE];
#define MAX_LISTN 256
extern Sock *socklisten(int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*));
extern char *dominum(char *);
extern int EsIp(char *);
extern int is_file(char *);
#define OPT_CR 0x1
#define OPT_LF 0x2
#define OPT_CRLF (OPT_CR | OPT_LF)
#ifdef USA_ZLIB
#define OPT_ZLIB 0x2
#define EsZlib(x) (x->opts & OPT_ZLIB)
#endif
extern void programa_loop_principal(void *);
extern char reth;
#ifdef _WIN32
extern void ChkBtCon(int, int);
extern OSVERSIONINFO VerInfo;
extern char SO[256];
extern HWND hwMain;
extern void CleanUp(void);
#endif
extern void _mysql_carga_tablas(void);
#define atoul(x) strtoul(x, NULL, 10)
extern void conferror(char *, ...);
extern void sockwritev(Sock *, int, char *, va_list);
#define LOCAL 0
#define REMOTO 1
extern void cierra_colossus(int);
extern void ircd_log(int, char *, ...);
#define ircfree(x) if (x) Free(x); x = NULL
#define PID "colossus.pid"
#ifdef _WIN32
extern void CleanUpSegv(int);
#endif
extern void reinicia();
extern int pregunta(char *);
extern void refresca();
extern int copyfile(char *, char *);
#ifdef USA_ZLIB
extern char *descomprime(Sock *, char *, int *);
extern char *comprime(Sock *, char *, int *);
extern void libera_zlib(Sock *);
extern int inicia_zlib(Sock *, int);
#endif
