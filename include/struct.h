/*
 * $Id: struct.h,v 1.46 2005-10-27 19:16:14 Trocotronic Exp $ 
 */

#include "setup.h"
#ifdef _WIN32
#include <Winsock.h>
#include <direct.h>
#include <sys/timeb.h>
#include <process.h>
#else
#define DWORD int
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef UNISTDH
#include <unistd.h>
#endif
#include <sys/time.h>
#endif /* _WIN32 */
#include <pthread.h>
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

#include "ircsprintf.h"
#include "parseconf.h"
#include "sql.h"

extern void carga_socks(void);
#ifdef NEED_STRCASECMP
extern int strcasecmp(const char *, const char *);
#endif
/*!
 * @desc: Hace una copia y libera el anterior contenido si no estuviera liberado.
 Es decir, si no apunta a NULL:
 * @params: $destino [out] Destino de la copia. Si no apunta a NULL, es liberado.
 	    $origen [in] Origen a copiar.
 * @sntx: void ircstrdup(char *destino, char *origen)
 * @cat: Programa
 !*/
#define ircstrdup(destino, origen) strcopia(&destino, origen)
extern void strcopia(char **, const char *);
/*!
 * @desc: Libera una zona de memoria apuntada por un puntero. Es un alias de free() pero se recomienda usar éste con fines de debug.
 * @params: $x [in] Puntero a liberar.
 * @cat: Programa
 * @sntx: void Free(void *x)
 !*/
#ifdef DEBUG
#define Free(x) free(x); Debug("Liberando %X", x)
#else
#define Free free
#endif
typedef struct _sock Sock;
#define SOCKFUNC(x) int (x)(Sock *sck, char *data)
#define MAXSOCKS MAXCONNECTIONS
#define BUFSIZE 1024
#define BUF_SOCK 8192
#define DBUF 2032
/*!
 * @desc: Devuelve el mínimo de dos números.
 * @params: $x [in] Primer número.
 	    $y [in] Segundo número.
 * @ret: Devuelve el mínimo de dos números.
 * @cat: Programa
 * @sntx: int MIN(int x, int y)
 !*/
#define MIN(x,y) (x < y ? x : y)

typedef struct _dbuf
{
	u_int len; /* para datos binarios */
	u_int slots;
	struct _dbufdata *wslot, *rslot; /* al slot de escritura y lectura: nunca wslot < rslot */
	char *wchar, *rchar; /* a la posición para escribir y leer */
}DBuf;
typedef struct _dbufdata
{
	struct _dbufdata *sig;
	char data[DBUF];
	u_int len;
}DbufData;

/*!
 * @desc: Es un recurso de conexión.
 * @params: $host Host o ip de la conexión.
 	    $puerto Puerto remoto.
 	    $pres Recurso de red (no modificar).
 	    $server Estructura de dirección de conexión.
 	    $openfunc Función a ejecutar cuando se abra la conexión.
 	    $readfunc Función a ejecutar cuando se reciban datos en la conexión.
 	    $closefunc Función a ejecutar cuando se cierre la conexión.
 	    $writefunc Función a ejecutar cuando se escriban datos en la conexión.
 	    $recvQ Cola de datos entrante.
 	    $sendQ Cola de datos saliente.
 	    $estado Tipos de estado de la conexión.
 	    	- EST_DESC: Estado desconocido.
 	    	- EST_CONN: Conectando.
 	    	- EST_LIST: Conexión de escucha.
 	    	- EST_OK: Conexión establecida.
 	    	- EST_CERR: Conexión cerrada.
 	    $opts: Opciones para la conexión.
 	    	- OPT_SSL: Conexión bajo SSL.
 	    	- OPT_ZLIB: Conexión con compresión de datos ZLib.
 	    $slot: Posición de la lista de conexiones.
 	    $buffer: Buffer de parseo de datos.
 	    $pos: Posición de escritura en el buffer.
 	    $zlib: Estructura del manejo de compresión de datos.
 	    $ssl: Estructura del manejo de ssl.
 * @ver: SockOpen SockClose SockListen
 * @cat: Conexiones
 !*/
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

extern Sock *SockOpen(char *, int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), int);
extern void SockWrite(Sock *, int, char *, ...);
extern void SockClose(Sock *, char);

/*!
 * @desc: Estructura de datos Hash. Se utiliza para definir una lista de entradas enlazadas.
 * @params: $item Apunta al primer elemento de la lista.
 	    $items Cantidad de elementos en esa lista.
 * @cat: Programa
 * @ex: 	Hash tabla[100];
 	UnaEstructura *st;
 	...
 	//UnaEstructura debe tener como miembro a hsig
 	st->hsig = tabla[0].item;
 	tabla[0].item = cl;
 	tabla[0].items++;
 !*/	
typedef struct _hash {
	void *item;
	int items;
}Hash;

extern const char NTL_tolower_tab[];
extern MODVAR char buf[BUFSIZE];
extern Sock *SockActual;
extern struct _cliente *BuscaClienteEnHash(char *, struct _cliente *, Hash *);
extern struct _canal *BuscaCanalEnHash(char *, struct _canal *, Hash *);
extern void InsertaClienteEnHash(struct _cliente*, char *, Hash *);
extern void InsertaCanalEnHash(struct _canal *, char *, Hash *);
extern int BorraClienteDeHash(struct _cliente *, char *, Hash *);
extern int BorraCanalDeHash(struct _canal *, char *, Hash *);
extern int match(char *, char *);

extern char *Fecha(time_t *);
typedef struct _smtpData
{
	char *para;
	char *de;
	char *tema;
	char *cuerpo;
	char enviado;
	int intentos;
}SmtpData;
extern void EnviaEmail(char *, char *, char *, char *);
extern void Email(char *, char *, char *, ...);

extern char *AleatorioEx(char *);
extern int Aleatorio(int, int);
extern char *Mx(char *);

/* senyals */
typedef struct _senyal
{
	struct _senyal *sig;
	short senyal;
	int (*func)();
}Senyal;
#define MAXSIGS 256
extern MODVAR Senyal *senyals[MAXSIGS];
extern void InsertaSenyal(short, int (*)());
extern int BorraSenyal(short, int (*)());
#define Senyal(s) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(); } while(0)
#define Senyal1(s,x) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x); } while(0)
#define Senyal2(s,x,y) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y); } while(0)
#define Senyal3(s,x,y,z) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z); } while(0)
#define Senyal4(s,x,y,z,t) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z,t); } while(0)
#define Senyal5(s,x,y,z,t,u) do { Senyal *aux; for (aux = senyals[s]; aux; aux = aux->sig) if (aux->func) aux->func(x,y,z,t,u); } while(0)

/* timers */
typedef struct _timer
{
	struct _timer *sig;
	char *nombre;
	int (*func)();
	void *args;
	Sock *sck;
	double cuando;
	int veces;
	int lleva;
	int cada;
}Timer;
extern void IniciaCrono(char *, Sock *, int, int, int (*)(), void *, size_t);
extern int ApagaCrono(char *, Sock *);
extern void CompruebaCronos(void);
extern double microtime(void);
extern char *str_replace(char *, char, char);
extern char *strtolower(char *);
extern char *strtoupper(char *);
extern char *Unifica(char *[], int, int, int);
extern char *gettok(char *, int, char);
extern int StrCount(char *, char *);
/*!
 * @desc: Estructura que se pasa a una función de tipo Proceso.
 * @params: $sig [in] Obviar.
 	    $func [in] Función que se ejecuta.
 	    $proc [in] Índice que se está siguiendo. Una vez se ha llegado a su fin, hay que ponerlo a 0 y setear el miembro time para la siguiente hora a ejecutar.
 	    $time [in] Contiene el timestamp en el que hay que ejecutar la función.
 * @cat: Procesos
 * @ver: IniciaProceso DetieneProceso
 !*/
typedef struct _proc
{
	struct _proc *sig;
	int (*func)();
	int proc;
	time_t time;
}Proc;
#define ProcFunc(x) int (x)(Proc *proc)
extern void ProcesosAuxiliares(void);
extern void IniciaProceso(int(*)());
extern int DetieneProceso(int (*)());
#define CHMAX 2048
#define UMAX 2048
#define INI_FACT 8
#define INI_SUMD 0xFF
extern u_int HashCliente(char *);
extern u_int HashCanal(char *);
extern MODVAR Hash uTab[UMAX];
extern MODVAR Hash cTab[CHMAX];
#define COLOSSUS_VERNUM "1.2a"
#define COLOSSUS_VERSION "Colossus " COLOSSUS_VERNUM
extern char **margv;
#define Malloc(x) ExMalloc(x, __FILE__, __LINE__)
/*!
 * @desc: Aloja memoria y pone a 0 toda la cantidad de memoria solicitada. Es un alias de malloc y memset.
 * @params: $p [in] Puntero a la zona de memoria a reservar.
	    $s [in] Tamaño o tipo de puntero.
 * @ex: 	MiEstructura *puntero;
 	BMalloc(puntero, MiEstructura);
 	//Hace lo mismo que
 	puntero = (MiEstructura *)malloc(sizeof(MiEstructura));
 	memset(puntero, 0, sizeof(MiEstructura));
 * @sntx: void BMalloc(void *p, s )
 * @cat: Programa
 !*/
#define BMalloc(p,s) do { p = (s *)Malloc(sizeof(s)); bzero(p, sizeof(s)); }while(0)
#define EST_DESC 0
#define EST_CONN 1
#define EST_LIST 2
#define EST_OK   3
#define EST_CERR 4
#define STAT_SSL_CONNECT_HANDSHAKE 5
#define STAT_SSL_ACCEPT_HANDSHAKE 6
#define EsDesc(x) (x && x->estado == EST_DESC)
#define EsConn(x) (x && x->estado == EST_CONN)
#define EsList(x) (x && x->estado == EST_LIST)
#define EsOk(x)   (x && x->estado == EST_OK)
#define EsCerr(x) (x && x->estado == EST_CERR)
#define SockDesc(x) x->estado = EST_DESC
#define SockConn(x) x->estado = EST_CONN
#define SockList(x) x->estado = EST_LIST
#define SockOk(x)   x->estado = EST_OK
#define SockCerr(x) x->estado = EST_CERR
#ifdef USA_SSL
#define EsSSLAcceptHandshake(x)	((x)->estado == STAT_SSL_ACCEPT_HANDSHAKE)
#define EsSSLConnectHandshake(x)	((x)->estado == STAT_SSL_CONNECT_HANDSHAKE)
#define EsSSLHandshake(x) (EsSSLAcceptHandshake(x) || EsSSLConnectHandshake(x))
#define SetSSLAcceptHandshake(x)	((x)->estado = STAT_SSL_ACCEPT_HANDSHAKE)
#define SetSSLConnectHandshake(x)	((x)->estado = STAT_SSL_CONNECT_HANDSHAKE)
#endif
#define FERR 1
#define FADV 2
#define FOK  3
#define FSQL 4
extern void Alerta(char , char *, ...);
extern void Debug(char *, ...);
extern char *ExMalloc(size_t, char *, long);
#define ADD 1
#define DEL 2
extern struct in_addr *Resuelve(char *);
#define SQL_CACHE "cache"
extern MODVAR char tokbuf[BUFSIZE];
#define MAX_LISTN 256
extern Sock *SockListen(int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*));
extern int EsIp(char *);
extern int EsArchivo(char *);
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
#ifdef _WIN32
extern void ChkBtCon(int, int);
extern char *PreguntaCampo(char *, char *, char *);
extern OSVERSIONINFO VerInfo;
extern char SO[256];
extern HWND hwMain;
extern void CleanUp(void);
extern void LoopPrincipal(void *);
#endif
extern void SQLCargaTablas(void);
/*!
 * @desc: Convierte una cadena a entero largo.
 * @params: $str [in] Cadena a convertir.
 * @ret: Devuelve el entero largo.
 * @cat: Programa
 * @sntx: u_long atoul(char *str)
 !*/
#define atoul(x) strtoul(x, NULL, 10)
extern void Error(char *, ...);
extern int Info(char *, ...);
extern void SockWriteVL(Sock *, int, char *, va_list);
#define LOCAL 0
#define REMOTO 1
extern VOIDSIG CierraColossus(int);
extern void Loguea(int, char *, ...);
/*!
 * @desc: Pone a NULL un puntero y lo libera si es necesario.
 * @params: $x [in] Puntero a setear.
 * @cat: Programa
 * @sntx: void ircfree(void *x)
 !*/
#define ircfree(x) do { if (x) Free(x); x = NULL; }while(0)
#define PID "colossus.pid"
#ifdef _WIN32
extern void CleanUpSegv(int);
#endif
extern VOIDSIG Reinicia();
extern int Pregunta(char *);
extern VOIDSIG Refresca();
extern int copyfile(char *, char *);
#ifdef USA_SSL
#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
#endif
extern char *my_itoa(int);
extern int b64_decode(char const *src, char *, size_t);
extern int b64_encode(char const *, size_t, char *, size_t);
extern MODVAR time_t iniciado;
#define Creditos() 																\
	Responde(cl, bl, "\00312%s - Trocotronic ©2004-2005", COLOSSUS_VERSION);								\
	Responde(cl, bl, " ");															\
	Responde(cl, bl, "Este programa ha salido tras horas y horas de dedicación y entusiasmo.");						\
	Responde(cl, bl, "Quiero agradecer a toda la gente que me ha ayudado y que ha colaborado, "						\
		"aportando su semilla, a que este programa vea la luz.");									\
	Responde(cl, bl, "A todos los usuarios que lo usan que contribuyen con sugerencias, informando de fallos y mejorándolo poco a poco."); 	\
	Responde(cl, bl, " "); 															\
	Responde(cl, bl, "Puedes descargar este programa de forma gratuita en %c\00312http://www.rallados.net", 31)
extern void ResuelveHost(char **, char *);
extern void cloak_crc(char *);

extern u_int base64toint(const char *);
extern const char *inttobase64(char *, u_int, u_int);
extern void tea(u_int *, u_int *, u_int *);
extern char *CifraNick(char *, char *);
extern u_long Crc32(const char *, u_int);
extern char *chrcat(char *, char);
#ifdef NEED_STRNCASECMP
extern int strncasecmp(const char *, const char *, int);
#endif
/* definiciones de cache */
extern char *CogeCache(char *, char *, int);
extern void InsertaCache(char *, char *, int, int, char *, ...);
extern void BorraCache(char *, char *, int);
#define CACHE_HOST "hosts" /* cache para hosts */
#define CACHE_MX "mx" /* cache para registros mx */

extern MODVAR pthread_mutex_t mutex;
#ifdef _WIN32
extern MODVAR char spath[MAX_PATH];
#else
extern MODVAR char spath[PATH_MAX];
#endif
#define SPATH spath
typedef struct item
{
	struct item *sig;
}Item;
void add_item(Item *, Item **);
Item *del_item(Item *, Item **, char);
#define AddItem(item, lista) add_item((Item *)item, (Item **)&lista)
#define BorraItem(item, lista) del_item((Item *)item, (Item **)&lista, 0)
#define LiberaItem(item, lista) del_item((Item *)item, (Item **)&lista, 1)
char *Repite(char, int);
