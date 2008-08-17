/*
 * $Id: struct.h,v 1.90 2008/06/01 16:29:03 Trocotronic Exp $
 */

#include "setup.h"
#ifdef _WIN32
#include <windows.h>
#else
#define DWORD int
#include <stdio.h>
#include <stdlib.h>
#ifdef STRINGH
#include <string.h>
#endif
#include <dirent.h>
#include <netinet/in.h>
#ifdef UNISTDH
#include <unistd.h>
#endif
#endif /* _WIN32 */
#ifdef FCNTLH
#include <fcntl.h>
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
#include "version.h"

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
 * @desc: Libera una zona de memoria apuntada por un puntero. Es un alias de free() pero se recomienda usar �ste con fines de debug.
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
#define SOCKFUNC(x) int (x)(Sock *sck, char *data, u_int len)
#define MAXSOCKS MAXCONNECTIONS
#define BUFSIZE 1024
#define SOCKBUF 4096
#define DBUF 2032

#ifndef MIN
/*!
 * @desc: Devuelve el m�nimo de dos n�meros.
 * @params: $x [in] Primer n�mero.
 	    $y [in] Segundo n�mero.
 * @ret: Devuelve el m�nimo de dos n�meros.
 * @cat: Programa
 * @sntx: int MIN(int x, int y)
 !*/
#define MIN(x,y) (x < y ? x : y)

#endif

#ifndef MAX
/*!
 * @desc: Devuelve el m�ximo de dos n�meros.
 * @params: $x [in] Primer n�mero.
 	    $y [in] Segundo n�mero.
 * @ret: Devuelve el m�ximo de dos n�meros.
 * @cat: Programa
 * @sntx: int MAX(int x, int y)
 !*/
#define MAX(x,y) (x < y ? y : x)

#endif

typedef struct _dbuf
{
	u_int len; /* para datos binarios */
	u_int slots;
	struct _dbufdata *wslot, *rslot; /* al slot de escritura y lectura: nunca wslot < rslot */
	char *wchar, *rchar; /* a la posici�n para escribir y leer */
}DBuf;
typedef struct _dbufdata
{
	struct _dbufdata *sig;
	char data[DBUF];
	u_int len;
}DbufData;

/*!
 * @desc: Es un recurso de conexi�n.
 * @params: $host Host o ip de la conexi�n.
 	    $puerto Puerto remoto.
 	    $pres Recurso de red (no modificar).
 	    $server Estructura de direcci�n de conexi�n.
 	    $openfunc Funci�n a ejecutar cuando se abra la conexi�n.
 	    $readfunc Funci�n a ejecutar cuando se reciban datos en la conexi�n.
 	    $closefunc Funci�n a ejecutar cuando se cierre la conexi�n.
 	    $writefunc Funci�n a ejecutar cuando se escriban datos en la conexi�n.
 	    $recvQ Cola de datos entrante.
 	    $sendQ Cola de datos saliente.
 	    $estado Tipos de estado de la conexi�n.
 	    	- EST_DESC: Estado desconocido.
 	    	- EST_CONN: Conectando.
 	    	- EST_LIST: Conexi�n de escucha.
 	    	- EST_OK: Conexi�n establecida.
 	    	- EST_CERR: Conexi�n cerrada.
 	    $opts: Opciones para la conexi�n.
 	    	- OPT_SSL: Conexi�n bajo SSL.
 	    	- OPT_ZLIB: Conexi�n con compresi�n de datos ZLib.
 	    $slot: Posici�n de la lista de conexiones.
 	    $buffer: Buffer de parseo de datos.
 	    $pos: Posici�n de escritura en el buffer.
 	    $inicio: Instante en el que fue creado.
 	    $recibido: Momento desde el �ltimo recibo de datos.
 	    $contout: Timeout para conectar.
 	    $recvtout: Timeout para recibir datos.
 	    $zlib: Estructura del manejo de compresi�n de datos.
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
extern void SockCloseEx(Sock *, int);
extern void SockClose(Sock *);
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
 * @desc: Inserta una se�al
 * Durante el transcurso del programa se generan varias se�ales. Cada vez que salta una se�al se ejecutan las funciones que est�n asociadas a ella.
 * @params: $senyal [in] Tipo de se�al a seguir. Estas se�ales pueden pasar un n�mero indefinido de par�metros, seg�n sea el tipo de se�al
 * 	    S�lo se aceptan las siguientes se�ales (entre par�ntesis se detallan los par�metros que aceptan):
 		- SIGN_SQL (): Ha terminado la carga del motor SQL
 		- SIGN_UMODE (Cliente *cl, char *umodos): Se ha producido un cambio en los modos de usuario del cliente <i>cl</i>. La cadena <i>umodos</i> contiene los cambios.
 		- SIGN_QUIT (Cliente *cl, char *mensaje): El cliente <i>cl</i> ha sido desconectado con el mensaje <i>mensaje</i>.
 		- SIGN_EOS (): Se ha terminado la uni�n entre servidores.
 		- SIGN_MODE (Cliente *cl, Canal *cn, char *modos): El cliente <i>cl</i> ha efectuado el cambio de modos <i>modos</i> del canal <i>cn</i>.
 		- SIGN_JOIN (Cliente *cl, Canal *cn): El cliente <i>cl</i> se une al canal <i>cn</i>.
 		- SIGN_SYNCH (): Se inicia la sincronizaci�n con el servidor.
 		- SIGN_KICK (Cliente *cl, Cliente *al, Canal *cn, char *motivo): El cliente <i>cl</i> expulsa al cliente <i>al</i> del canal <i>cn</i> con el motivo <i>motivo</i>.
 		- SIGN_TOPIC (Cliente *cl, Canal *cn, char *topic): El cliente <i>cl</i> pone el topic <i>topic</i> en el canal <i>cn</i>.
 		- SIGN_PRE_NICK (Cliente *cl, char *nuevo): El cliente <i>cl</i> va a cambiarse el nick a <i>nuevo</i>.
 		- SIGN_POST_NICK (Cliente *cl, int modo): El cliente <i>cl</i> ha efectuado una operaci�n de nick. Si es una nueva conexi�n, <i>modo</i> vale 0.
 		- SIGN_AWAY (Cliente *cl, char *mensaje): El cliente <i>cl</i> se pone away con el mensaje <i>mensaje</i>. Si mensaje apunta a NULL, el cliente regresa de away.
 		- SIGN_PART (Cliente *cl, Canal *cn, char *mensaje): El cliente <i>cl</i> abandona el canal <i>cn</i> con el mensaje <i>mensaje</i>. Si no hay mensaje, apunta a NULL.
 		- SIGN_STARTUP (): Se ha cargado el programa. S�lo se ejecuta una vez.
 		- SIGN_SOCKOPEN (): Se ha establecido la conexi�n con el ircd. Todav�a no se han mandado datos. Se llama antes de mandar PROTOCTL.
 		- SIGN_CDESTROY (Canal *cn): Se borra este canal de la memoria. Se ha vaciado el canal.
 		- SIGN_CMSG (Cliente *cl, Canal *cn, char *msg): El cliente <i>cl</i> env�a el mensaje <i>msg</i> al canal <i>cn</i>.
 		- SIGN_PMSG (Cliente *cl, Cliente *bl, char *msg, int respuesta): El cliente <i>cl</i> env�a el mensaje <i>msg</i> al bot <i>bl</i>.
 			El valor respuesta toma el valor de respuesta del bot anfitri�n. Si es 0, el bot anfitri�n ha ejecutado el comando correctamente.
 			Si es 1, el bot anfitri�n ha emitido alg�n error y ha abortado. Si es -1, no existe bot anfitri�n.
 			NOTA: esta se�al se ejecuta <u>despu�s</u> de haber ejecutado la funci�n del bot anfitri�n, en el caso de que hubiera.
 		- SIGN_SERVER (Cliente *cl, int primer): Se ha conectado un servidor nuevo. Si es el primer link, primero vale 1.
 		- SIGN_SOCKCLOSE (): La conexi�n con el ircd se ha perdido.
 *	    $func [in] Funci�n a ejecutar. Esta funci�n debe estar definida seg�n sea el tipo de se�al que controla.
 		Recibir� los par�metros que se han descrito arriba. Por ejemplo, si es una funci�n para una se�al SIGN_UMODE, recibir� 2 par�metros.
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
 * @cat: Se�ales
 * @sntx: void InsertaSenyal(int senyal, int (*func)())
 !*/
#define InsertaSenyal(x,y) InsertaSenyalEx(x,y,FIN)
extern void InsertaSenyalEx(int, int (*)(), int);
extern int BorraSenyal(int, int (*)());
extern void LlamaSenyal(int, int, ...);

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
 * @desc: Estructura que se pasa a una funci�n de tipo Proceso.
 * @params: $sig [in] Obviar.
 	    $func [in] Funci�n que se ejecuta.
 	    $proc [in] �ndice que se est� siguiendo. Una vez se ha llegado a su fin, hay que ponerlo a 0 y setear el miembro time para la siguiente hora a ejecutar.
 	    $time [in] Contiene el timestamp en el que hay que ejecutar la funci�n.
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
extern char **margv;
#define Malloc(x) ExMalloc(x, 0, __FILE__, __LINE__)
/*!
 * @desc: Aloja memoria y pone a 0 toda la cantidad de memoria solicitada. Es un alias de malloc y memset.
 * @params: $s [in] Tama�o o tipo de puntero (char, int, alg�n typedef, etc.). Se usa el operador sizeof() impl�citamente.
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
#define SQL_VERSIONES "versiones"
#define SQL_CONFIG "configuracion"
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
#define OPT_BIN 0x10
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
extern int CierraColossus(int);
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
extern void setfilemodtime(char *, time_t);
extern Recurso CopiaDll(char *, char *, char *);
#ifdef USA_SSL
#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
#endif
extern MODVAR time_t iniciado;
extern MODVAR int refrescando;

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
#ifdef NEED_STRISTR
extern const char *stristr(const char *, const char *);
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
extern MODVAR int t_off;
#define GMTime() (time(0) + t_off)
typedef int (*ECmdFunc)(u_long, char *, void *);
extern int EjecutaComandoSinc(char *, char *, u_long *, char **);
extern int EjecutaComandoASinc(char *, char *, ECmdFunc, void *);
extern Directorio AbreDirectorio(char *);
extern char *LeeDirectorio(Directorio);
extern void CierraDirectorio(Directorio);
extern char *URLEncode(char *);
extern char *URLDecode(char *);
