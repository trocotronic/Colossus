/*
 * $Id: sistema.h,v 1.20 2007-05-31 23:06:37 Trocotronic Exp $ 
 */

#ifndef MODVAR
 #if defined(MODULE_COMPILE) && defined(_WIN32)
  #define MODVAR __declspec(dllimport)
 #else
  #define MODVAR
 #endif
#endif

#ifndef MODFUNC
 #if !defined(MODULE_COMPILE) && defined(_WIN32)
  #define MODFUNC __declspec(dllexport)
 #else
  #define MODFUNC
 #endif
#endif

#ifdef _WIN32
#define DLLFUNC __declspec(dllexport)
#define irc_dlopen(x,y) LoadLibraryEx(x, NULL, 0)
#define irc_dlclose FreeLibrary
#define irc_dlsym(x,y,z) z = (void *)GetProcAddress(x,y)
#define irc_dlerror ErrorDl
typedef HMODULE Recurso;
typedef HANDLE Directorio;
#else
#define irc_dlopen dlopen
#define irc_dlclose dlclose
#define irc_dlsym(x,y,z) z = dlsym(x,y)
#define irc_dlerror dlerror
#define DLLFUNC
typedef void * Recurso;
typedef DIR * Directorio;
  #ifndef O_BINARY
  #define O_BINARY 0x0
  #endif
#endif


/* 
 * Macros de portabilidad
 */
#ifndef _WIN32
#define SET_ERRNO(x) errno = x
#define READ_SOCK(fd, buf, len) read((fd), (buf), (len))
#define WRITE_SOCK(fd, buf, len) write((fd), (buf), (len))
#define CLOSE_SOCK(fd) close(fd)
#define IOCTL(x, y, z) ioctl((x), (y), (z))
#define ERRNO errno
#define STRERROR(x) strerror(x)

#define P_EMFILE EMFILE
#define P_ENOBUFS ENOBUFS
#define P_EWOULDBLOCK EWOULDBLOCK
#define P_EAGAIN EAGAIN
#define P_EINPROGRESS EINPROGRESS
#define P_EWORKING EINPROGRESS
#define P_EINTR EINTR
#define P_ETIMEDOUT ETIMEDOUT
#define P_ENOTSOCK ENOTSOCK
#define P_EIO	EIO
#define P_ECONNABORTED ECONNABORTED
#define P_ECONNRESET	ECONNRESET
#define P_ENOTCONN ENOTCONN
#define P_EMSGSIZE EMSGSIZE

#if defined(_MSC_VER) && (_MSV_VER >= 1400)
	#define open _open
	#define getpid _getpid
	#define write _write
	#define close _close
	#define getcwd _getcwd
#endif
#else /* _WIN32 */
#define NETDB_INTERNAL -1
#define NETDB_SUCCESS 0 

#define READ_SOCK(fd, buf, len) recv((fd), (buf), (len), 0)
#define WRITE_SOCK(fd, buf, len) send((fd), (buf), (len), 0)
#define CLOSE_SOCK(fd) closesocket(fd)
#define IOCTL(x, y, z) ioctlsocket((x), (y), (z))
#define ERRNO WSAGetLastError()
#define STRERROR(x) sock_strerror(x)
#define SET_ERRNO(x) WSASetLastError(x)

#define P_EMFILE WSAEMFILE
#define P_ENOBUFS WSAENOBUFS
#define P_EWOULDBLOCK WSAEWOULDBLOCK
#define P_EAGAIN WSAEWOULDBLOCK
#define P_EINPROGRESS WSAEINPROGRESS
#define P_EWORKING WSAEWOULDBLOCK
#define P_EINTR WSAEINTR
#define P_ETIMEDOUT WSAETIMEDOUT
#define P_ENOTSOCK WSAENOTSOCK
#define P_EIO	EIO
#define P_ECONNABORTED WSAECONNABORTED
#define P_ECONNRESET	WSAECONNRESET
#define P_ENOTCONN WSAENOTCONN
#define P_EMSGSIZE WSAEMSGSIZE
#define socklen_t int
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define FALSE (0)
#define TRUE  (!FALSE)

/*!
 * @desc: Convierte un caracter a su minúsculo.
 * @params: $c [in] Caracter.
 * @ret: Devuelve el caracter en minúscula.
 * @cat: Programa
 * @sntx: char ToLower(char c)
 * @ver: ToUpper
 !*/
#define ToLower(c) (NTL_tolower_tab[(int)(c)])
/*!
 * @desc: Convierte un caracter a su mayúsculo.
 * @params: $c [in] Caracter.
 * @ret: Devuelve el caracter en mayúscula.
 * @cat: Programa
 * @sntx: char ToUpper(char c)
 * @ver: ToLower
 !*/
#define ToUpper(c) (NTL_toupper_tab[(int)(c)])
#define u_long unsigned long
#define u_char unsigned char
#define u_int unsigned int
#define u_short unsigned short
/*!
 * @desc: Decide si un puntero apunta a NULL o su contenido está vacío.
 * @params: $ptr [in] Puntero a evaluar.
 * @ret: Devuelve 1 si apunta a NULL o su contenido está vacío; 0, si no.
 * @cat: Programa
 * @sntx: int BadPtr(void *ptr)
 !*/
#define BadPtr(x) (!(x) || (*(x) == '\0'))

#ifdef NEED_BZERO
#define bzero(x,y) memset(x,0,y)
#endif
#ifdef NEED_ABS
#define abs(x) ((x < 0) ? -(x) : x)
#endif
#ifdef NEED_INET_NTOA
#define inet_ntoa(x) inetntoa(&x)
#endif
#define VOIDSIG void
#define CPATH "colossus.conf"
#ifndef LISTEN_SIZE
#define LISTEN_SIZE 5
#endif
#ifndef MAXCONNECTIONS
#define MAXCONNECTIONS	4096
#endif

#define MAX_FNAME 128

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup
#define execv _execv
#define open _open
#define write _write
#define close _close
#define unlink _unlink
#define mkdir(x,y) _mkdir(x)
#endif

extern char *strtolower(char *);
extern char *strtoupper(char *);
extern size_t strlcpy(char *, const char *, size_t);
extern size_t strlcat(char *, const char *, size_t);

typedef struct _duracion Duracion;
/*!
 * @desc: Es un recurso de duración.
 * @params: $sems Semanas.
 	    $dias Días.
 	    $horas Horas.
 	    $mins Minutos.
 	    $segs Segundos.
 * @ver: MideDuracion
 * @cat: Programa
 !*/
struct _duracion
{
	u_int sems;
	u_int dias;
	u_int horas;
	u_int mins;
	u_int segs;
};
extern int MideDuracion(u_int, Duracion *);
