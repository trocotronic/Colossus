/*
 * $Id: struct.h,v 1.67 2006-12-23 01:07:00 Trocotronic Exp $ 
 */

#include "setup.h"
#ifdef _WIN32
#include <winsock.h>
#include <direct.h>
#include <sys/timeb.h>
#include <process.h>
#else
#define DWORD int
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/time.h>
#ifdef UNISTDH
#include <unistd.h>
#endif
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

#include "ircsprintf.h"
#include "parseconf.h"
#include "sql.h"

extern void carga_socks(void);
#ifdef NEED_STRCASECMP
extern int strcasecmp(const char *, const char *);
#endif
#ifdef NEED_SNPRINTF
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif
#define IsDigit(c) (c >= 48 && c <= 57)
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
#define Free(x) do { Debug("Liberando %X (%s:%i)", x, __FILE__, __LINE__); free(x); }while(0)
#else
#define Free free
#endif
typedef struct _sock Sock;
#define SOCKFUNC(x) int (x)(Sock *sck, char *data)
#define MAXSOCKS MAXCONNECTIONS
#define BUFSIZE 1024
#define SOCKBUF 4096
#define DBUF 2032

#ifndef MIN
/*!
 * @desc: Devuelve el mínimo de dos números.
 * @params: $x [in] Primer número.
 	    $y [in] Segundo número.
 * @ret: Devuelve el mínimo de dos números.
 * @cat: Programa
 * @sntx: int MIN(int x, int y)
 !*/
#define MIN(x,y) (x < y ? x : y)

#endif

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
 	    $inicio: Instante en el que fue creado.
 	    $recibido: Momento desde el último recibo de datos.
 	    $contout: Timeout para conectar.
 	    $recvtout: Timeout para recibir datos.
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
	int estado;
	int opts;
	int slot;
	char buffer[SOCKBUF]; /* buffer de parseo */
	int pos; /* posicion de escritura del buffer */
	time_t inicio;
	time_t recibido;
	u_int contout;
	u_int recvtout;
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

extern Sock *SockOpen(char *, int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*));
extern Sock *SockOpenEx(char *, int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), u_int, u_int, u_int);
extern void SockWriteExVL(Sock *, int, char *, va_list);
extern void SockWriteVL(Sock *, char *, va_list);
extern void SockWriteEx(Sock *, int, char *, ...);
extern void SockWrite(Sock *, char *, ...);
extern void SockClose(Sock *, char);
extern Sock *SockListen(int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*));
extern Sock *SockListenEx(int, SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), SOCKFUNC(*), u_int);
extern void SockWriteBin(Sock *, u_long, char *);
extern MODVAR const char NTL_tolower_tab[];
extern MODVAR const char NTL_toupper_tab[];
extern MODVAR char buf[BUFSIZE];
extern Sock *SockActual;
extern int match(char *, char *);

typedef struct _opts
{
	int opt;
	char *item;
}Opts;
extern int BuscaOpt(char *, Opts *);
extern char *BuscaOptItem(int, Opts *);

extern char *Fecha(time_t *);
typedef struct _smtpData
{
	struct _smtpData *sig;
	Sock *sck;
	u_int estado;
	char *para;
	char *de;
	char *tema;
	char *cuerpo;
	char enviado;
	int intentos;
	Opts *fps;
	int (*closefunc)(struct _smtpData *);
}SmtpData;
extern void Email(char *, char *, char *, ...);
extern void EmailArchivos(char *, char *, Opts *, int (*)(SmtpData *), char *, ...);

extern char *AleatorioEx(char *);
extern u_int Aleatorio(u_int, u_int);
extern char *Mx(char *);

#define INI 1
#define FIN 2

/* senyals */
typedef struct _senyal
{
	struct _senyal *sig;
	int senyal;
	int (*func)();
}Senyal;
#define MAXSIGS 256
extern MODVAR Senyal *senyals[MAXSIGS];
/*! 
 * @desc: Inserta una señal
 * Durante el transcurso del programa se generan varias señales. Cada vez que salta una señal se ejecutan las funciones que estén asociadas a ella.
 * @params: $senyal [in] Tipo de señal a seguir. Estas señales pueden pasar un número indefinido de parámetros, según sea el tipo de señal
 * 	    Sólo se aceptan las siguientes señales (entre paréntesis se detallan los parámetros que aceptan):
 		- SIGN_SQL (): Ha terminado la carga del motor SQL
 		- SIGN_UMODE (Cliente *cl, char *umodos): Se ha producido un cambio en los modos de usuario del cliente <i>cl</i>. La cadena <i>umodos</i> contiene los cambios.
 		- SIGN_QUIT (Cliente *cl, char *mensaje): El cliente <i>cl</i> ha sido desconectado con el mensaje <i>mensaje</i>.
 		- SIGN_EOS (): Se ha terminado la unión entre servidores.
 		- SIGN_MODE (Cliente *cl, Canal *cn, char *modos): El cliente <i>cl</i> ha efectuado el cambio de modos <i>modos</i> del canal <i>cn</i>.
 		- SIGN_JOIN (Cliente *cl, Canal *cn): El cliente <i>cl</i> se une al canal <i>cn</i>.
 		- SIGN_SYNCH (): Se inicia la sincronización con el servidor.
 		- SIGN_KICK (Cliente *cl, Cliente *al, Canal *cn, char *motivo): El cliente <i>cl</i> expulsa al cliente <i>al</i> del canal <i>cn</i> con el motivo <i>motivo</i>.
 		- SIGN_TOPIC (Cliente *cl, Canal *cn, char *topic): El cliente <i>cl</i> pone el topic <i>topic</i> en el canal <i>cn</i>.
 		- SIGN_PRE_NICK (Cliente *cl, char *nuevo): El cliente <i>cl</i> va a cambiarse el nick a <i>nuevo</i>.
 		- SIGN_POST_NICK (Cliente *cl, int modo): El cliente <i>cl</i> ha efectuado una operación de nick. Si es una nueva conexión, <i>modo</i> vale 0.
 		- SIGN_AWAY (Cliente *cl, char *mensaje): El cliente <i>cl</i> se pone away con el mensaje <i>mensaje</i>. Si mensaje apunta a NULL, el cliente regresa de away.
 		- SIGN_PART (Cliente *cl, Canal *cn, char *mensaje): El cliente <i>cl</i> abandona el canal <i>cn</i> con el mensaje <i>mensaje</i>. Si no hay mensaje, apunta a NULL.
 		- SIGN_STARTUP (): Se ha cargado el programa. Sólo se ejecuta una vez.
 		- SIGN_SOCKOPEN (): Se ha establecido la conexión con el ircd. Todavía no se han mandado datos.
 		- SIGN_CDESTROY (Canal *cn): Se borra este canal de la memoria. Se ha vaciado el canal.
 		- SIGN_CMSG (Cliente *cl, Canal *cn, char *msg): El cliente <i>cl</i> envía el mensaje <i>msg</i> al canal <i>cn</i>.
 		- SIGN_PMSG (Cliente *cl, Cliente *bl, char *msg, int respuesta): El cliente <i>cl</i> envía el mensaje <i>msg</i> al bot <i>bl</i>.
 			El valor respuesta toma el valor de respuesta del bot anfitrión. Si es 0, el bot anfitrión ha ejecutado el comando correctamente. 
 			Si es 1, el bot anfitrión ha emitido algún error y ha abortado. Si es -1, no existe bot anfitrión.
 			NOTA: esta señal se ejecuta <u>después</u> de haber ejecutado la función del bot anfitrión, en el caso de que hubiera.
 *	    $func [in] Función a ejecutar. Esta función debe estar definida según sea el tipo de señal que controla.
 		Recibirá los parámetros que se han descrito arriba. Por ejemplo, si es una función para una señal SIGN_UMODE, recibirá 2 parámetros.
 * @ex: 	int Umodos(Cliente *, char *);
 	InsertaSenyal(SIGN_UMODE, Umodos);
 	...
 	int Umodos(Cliente *cl, char *umodos)
 	{
 		...
 		printf("El usuario ha cambiado sus modos");
 		return 0;
 	}
 * @ver: BorraSenyal
 * @cat: Señales
 * @sntx: void InsertaSenyal(int senyal, int (*func)())
 !*/
#define InsertaSenyal(x,y) InsertaSenyalEx(x,y,FIN)
extern void InsertaSenyalEx(int, int (*)(), int);
extern int BorraSenyal(int, int (*)());
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
	int (*func)();
	void *args;
	double cuando;
	int veces;
	int lleva;
	int cada;
}Timer;
extern Timer *IniciaCrono(u_int, u_int, int (*)(), void *);
extern int ApagaCrono(Timer *);
extern void CompruebaCronos(void);
extern double microtime(void);
extern char *str_replace(char *, char, char);
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
#define COLOSSUS_VERNUM "1.7"
#define COLOSSUS_VERSION "Colossus " COLOSSUS_VERNUM
#define COLOSSUS_VERINT 10600
extern char **margv;
#define Malloc(x) ExMalloc(x, 0, __FILE__, __LINE__)
/*!
 * @desc: Aloja memoria y pone a 0 toda la cantidad de memoria solicitada. Es un alias de malloc y memset.
 * @params: $s [in] Tamaño o tipo de puntero (char, int, algún typedef, etc.). Se usa el operador sizeof() implícitamente.
 * @ex: 	MiEstructura *puntero;
 	puntero = BMalloc(MiEstructura);
 	//Hace lo mismo que
 	puntero = (MiEstructura *)malloc(sizeof(MiEstructura));
 	memset(puntero, 0, sizeof(MiEstructura));
 * @sntx: TipoDeDatos *BMalloc( TipoDeDatos )
 * @ret: Devuelve un puntero de tipo TipoDeDatos
 * @cat: Programa
 !*/
#define BMalloc(s) (s *)ExMalloc(sizeof(s), 1, __FILE__, __LINE__)
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
extern char *ExMalloc(size_t, int, char *, long);
#ifndef ADD
#define ADD 1
#endif
#ifndef DEL
#define DEL 2
#endif
extern struct in_addr *Resuelve(char *);
#define SQL_CACHE "cache"
extern MODVAR char tokbuf[BUFSIZE];
#define MAX_LISTN 256
extern int EsIp(char *);
extern int EsArchivo(char *);
#define OPT_CR 0x1
#define OPT_LF 0x2
#define OPT_CRLF (OPT_CR | OPT_LF)
#define OPT_NORECVQ 0x1
#define EsNoRecvQ(x) (x->opts & OPT_NORECVQ)
#define SetNoRecvQ(x) x->opts |= OPT_NORECVQ
#define UnsetNoRecvQ(x) x->opts &= ~OPT_NORECVQ
#ifdef USA_ZLIB
#define OPT_ZLIB 0x2
#define EsZlib(x) (x->opts & OPT_ZLIB)
#define SetZlib(x) do{x->opts |= OPT_ZLIB;x->opts &= ~OPT_NORECVQ;}while(0)
#define UnsetZlib(x) x->opts &= ~OPT_ZLIB
#endif
#ifdef USA_SSL
#define OPT_SSL 0x4
#define EsSSL(x) (x->opts & OPT_SSL)
#define SetSSL(x) x->opts |= OPT_SSL
#define UnsetSSL(x) x->opts &= ~OPT_SSL
#endif
#define OPT_NADD 0x8
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
extern Recurso CopiaDll(char *, char *, char *);
#ifdef USA_SSL
#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
#endif
extern char *my_itoa(int);
extern MODVAR time_t iniciado;
extern MODVAR int refrescando;
#define Creditos() 																\
	Responde(cl, bl, "\00312%s - Trocotronic ©2004-2007", COLOSSUS_VERSION);								\
	Responde(cl, bl, " ");															\
	Responde(cl, bl, "Este programa ha salido tras horas y horas de dedicación y entusiasmo.");						\
	Responde(cl, bl, "Quiero agradecer a toda la gente que me ha ayudado y que ha colaborado, "						\
		"aportando su semilla, a que este programa vea la luz.");									\
	Responde(cl, bl, "A todos los usuarios que lo usan que contribuyen con sugerencias, informando de fallos y mejorándolo poco a poco."); 	\
	Responde(cl, bl, " "); 															\
	Responde(cl, bl, "Puedes descargar este programa de forma gratuita en %c\00312http://www.redyc.com", 31)
extern void ResuelveHost(char **, char *);
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

#ifndef _WIN32
#define PMAX PATH_MAX 
#else
#define PMAX MAX_PATH
#endif
extern MODVAR char spath[PMAX];
#define SPATH spath
typedef struct item
{
	struct item *sig;
}Item;
void add_item(Item *, Item **);
Item *del_item(Item *, Item **, int);
#define AddItem(item, lista) add_item((Item *)item, (Item **)&lista)
#define BorraItem(item, lista) del_item((Item *)item, (Item **)&lista, 0)
#define LiberaItem(item, lista) del_item((Item *)item, (Item **)&lista, 1)
char *Repite(char, int);

extern int b64_encode(char const *, size_t, char *, size_t);
extern int b64_decode(char const *, char *, size_t);
extern char *base64_encode(char *, int);
extern char *base64_decode(char *);

extern char *Encripta(char *, char *);
extern char *Desencripta(char *, char *);
extern char *Long2Char(u_long);
extern time_t GMTime();
typedef int (*ECmdFunc)(u_long, char *, void *);
extern int EjecutaComandoSinc(char *, char *, u_long *, char **);
extern int EjecutaComandoASinc(char *, char *, ECmdFunc, void *);
extern Directorio AbreDirectorio(char *);
extern char *LeeDirectorio(Directorio);
extern void CierraDirectorio(Directorio);
