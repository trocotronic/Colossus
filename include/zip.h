/*
 * $Id: zip.h,v 1.4 2005-06-29 21:13:48 Trocotronic Exp $ 
 */
 
#include <zlib.h>

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
	int primero;
};

extern char *ZLibDescomprime(Sock *, char *, int *);
extern char *ZLibComprime(Sock *, char *, int *, int);
DLLFUNC extern void ZLibLibera(Sock *);
DLLFUNC extern int ZLibInit(Sock *, int);
