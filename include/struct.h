/*
 * $Id: struct.h,v 1.32 2005-03-18 21:26:52 Trocotronic Exp $ 
 */

#include "setup.h"
#ifdef _WIN32
#include <Winsock.h>
#include <direct.h>
#include "mysql.h"
#include <sys/timeb.h>
#include <process.h>
#include "pthreadMS.h"
#else
#define DWORD int
#include <mysql/mysql.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef UNISTDH
#include <unistd.h>
#endif
#include <sys/time.h>
#include <pthread.h>
#endif /* _WIN32 */
#ifdef FCNTLH
#include <fcntl.h>
#endif
#include <stdio.h>
#ifdef STDLIBH
#include <stdlib.h>
#endif
#include <time.h>
#ifdef STRINGH
#include <string.h>
#endif
#include "sistema.h" /* portabilidades */

#ifdef USA_SSL
#include <openssl/rsa.h>       /* SSL stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>    
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/ripemd.h>
#include "ssl.h"
#endif

#include "sprintf_irc.h"
#include "parseconf.h"

extern void carga_socks(void);
#ifdef NEED_STRCASECMP
extern int strcasecmp(const char *, const char *);
#endif
extern void ircstrdup(char **, const char *);
#ifdef DEBUG
#define Free(x) free(x); Debug("Liberando %X", x)
#else
#define Free free
#endif
#define SOCKFUNC(x) int (x)(Sock *sck, char *data)
#define MAXSOCKS MAXCONNECTIONS
#define BUFSIZE 1024
#define BUF_SOCK 8192
#define DBUF 2032
#define MIN(x,y) (x < y ? x : y)

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
	char data[DBUF];
	u_int len;
	struct _dbufdata *sig;
};
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
	int slot;
	char buffer[BUFSIZE]; /* buffer de parseo */
	int pos; /* posicion de escritura del buffer */
#ifdef USA_ZLIB
	struct _zlib *zlib;
#endif
#ifdef USA_SSL
	SSL *ssl;
#endif
};
struct Sockets
{
	Sock *socket[MAXSOCKS];
	int abiertos;
	int tope;
}ListaSocks;

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
extern struct _cliente *busca_cliente_en_hash(char *, struct _cliente *, Hash *);
extern struct _canal *busca_canal_en_hash(char *, struct _canal *, Hash *);
extern void inserta_cliente_en_hash(struct _cliente*, char *, Hash *);
extern void inserta_canal_en_hash(struct _canal *, char *, Hash *);
extern int borra_cliente_de_hash(struct _cliente *, char *, Hash *);
extern int borra_canal_de_hash(struct _canal *, char *, Hash *);
extern int match(char *, char *);

extern char *_asctime(time_t *);
#define PROTOCOL 2305

extern int carga_mysql();
extern MODVAR MYSQL *mysql;
extern MYSQL_RES *_mysql_query(char *format, ...);
extern char *_mysql_get_registro(char *, char *, char *);
extern char *_mysql_get_num(MYSQL *, char *, int, char *);
extern void _mysql_add(char *, char *, char *, char *, ...);
extern void _mysql_del(char *, char *);
extern int _mysql_restaura(void);
extern int _mysql_backup(void);
extern int _mysql_existe_tabla(char *);
struct mysql_t
{
	char *tabla[256]; /* espero que no haya tantas tablas */
	int tablas;
};
extern MODVAR struct mysql_t mysql_tablas;
extern char *_mysql_escapa(char *);
extern char *_mysql_fetch_array(MYSQL_RES *, const char *, MYSQL_ROW);

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
#define senyal3(s,x,y,z) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z); } while(0)
#define senyal4(s,x,y,z,t) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z,t); } while(0)
#define senyal5(s,x,y,z,t,u) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z,t,u); } while(0)

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
extern MODVAR Hash uTab[UMAX];
extern MODVAR Hash cTab[CHMAX];
#define COLOSSUS_VERNUM "1.1"
#define COLOSSUS_VERSION "Colossus " COLOSSUS_VERNUM
extern char **margv;
#define Malloc(x) StsMalloc(x, __FILE__, __LINE__)
#define da_Malloc(p,s) do { p = (s *)Malloc(sizeof(s)); bzero(p, sizeof(s)); }while(0)
#define EST_DESC 0
#define EST_CONN 1
#define EST_LIST 2
#define EST_OK   3
#define EST_CERR 4
#define STAT_SSL_CONNECT_HANDSHAKE 5
#define STAT_SSL_ACCEPT_HANDSHAKE 6
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
#ifdef USA_SSL
#define IsSSLAcceptHandshake(x)	((x)->estado == STAT_SSL_ACCEPT_HANDSHAKE)
#define IsSSLConnectHandshake(x)	((x)->estado == STAT_SSL_CONNECT_HANDSHAKE)
#define IsSSLHandshake(x) (IsSSLAcceptHandshake(x) || IsSSLConnectHandshake(x))
#define SetSSLAcceptHandshake(x)	((x)->estado = STAT_SSL_ACCEPT_HANDSHAKE)
#define SetSSLConnectHandshake(x)	((x)->estado = STAT_SSL_CONNECT_HANDSHAKE)
#endif
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
extern int EsIp(char *);
extern int is_file(char *);
#define OPT_CR 0x1
#define OPT_LF 0x2
#define OPT_CRLF (OPT_CR | OPT_LF)
#ifdef USA_ZLIB
#define OPT_ZLIB 0x1
#define EsZlib(x) (x->opts & OPT_ZLIB)
#endif
#ifdef USA_SSL
#define OPT_SSL 0x2
#define EsSSL(x) (x->opts & OPT_SSL)
#endif
#define OPT_EOS 0x4
#ifdef _WIN32
extern void ChkBtCon(int, int);
extern OSVERSIONINFO VerInfo;
extern char SO[256];
extern HWND hwMain;
extern void CleanUp(void);
extern void programa_loop_principal(void *);
#endif
extern void _mysql_carga_tablas(void);
#define atoul(x) strtoul(x, NULL, 10)
extern void conferror(char *, ...);
extern int Info(char *, ...);
extern void sockwritev(Sock *, int, char *, va_list);
#define LOCAL 0
#define REMOTO 1
extern VOIDSIG cierra_colossus(int);
extern void ircd_log(int, char *, ...);
#define ircfree(x) do { if (x) Free(x); x = NULL; }while(0)
#define PID "colossus.pid"
#ifdef _WIN32
extern void CleanUpSegv(int);
#endif
extern VOIDSIG reinicia();
extern int pregunta(char *);
extern VOIDSIG refresca();
extern int copyfile(char *, char *);
#ifdef USA_SSL
#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
#endif
extern char *my_itoa(int);
extern char *decode_ip(char *);
extern char *encode_ip(char *);
extern int b64_decode(char const *src, u_char *, size_t);
extern int b64_encode(u_char const *, size_t, char *, size_t);
extern MODVAR time_t iniciado;
#define creditos() 															\
	response(cl, bl, "\00312Colossus v%s - Trocotronic ©2004-2005", COLOSSUS_VERSION);					\
	response(cl, bl, " ");													\
	response(cl, bl, "Este programa ha salido tras horas y horas de dedicación y entusiasmo.");			\
	response(cl, bl, "Si no es perfecto, ¿qué más da?");									\
	response(cl, bl, "Sé feliz. Paz.");											\
	response(cl, bl, " "); 													\
	response(cl, bl, "Puedes descargar este programa de forma gratuita en %c\00312http://www.rallados.net", 31); 	\
	response(cl, bl, "Gracias a todos por usar este programa :) (estáis locos)")
extern void resuelve_host(char **, char *);
extern void cloak_crc(char *);

extern u_int base64toint(const char *);
extern const char *inttobase64(char *, u_int, u_int);
extern void tea(u_int *, u_int *, u_int *);
extern char *cifranick(char *, char *);
extern u_long our_crc32(const u_char *, u_int);
extern char *chrcat(char *, char);
#ifdef NEED_STRNCASECMP
extern int strncasecmp(const char *, const char *, int);
#endif
/* definiciones de cache */
extern char *coge_cache(char *, char *, int);
extern void inserta_cache(char *, char *, int, int, char *, ...);
extern void borra_cache(char *, char *, int);
#define CACHE_HOST "hosts" /* cache para hosts */
#define CACHE_MX "mx" /* cache para registros mx */

extern MODVAR pthread_mutex_t mutex;
