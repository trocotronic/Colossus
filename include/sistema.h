/*
 * $Id: sistema.h,v 1.5 2005-02-19 18:07:04 Trocotronic Exp $ 
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
#else
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

#define ToLower(c) (NTL_tolower_tab[(int)(c)])
#define ToUpper(c) (NTL_toupper_tab[(int)(c)])
#define u_long unsigned long
#define u_char unsigned char
#define u_int unsigned int
#define u_short unsigned short
#define BadPtr(x) (!(x) || (*(x) == '\0'))
#ifdef NEED_BZERO
#define bzero(x,y) memset(x,0,y)
#endif
#ifdef NEED_ABS
#define abs(x) (x < 0) ? -x : x
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
