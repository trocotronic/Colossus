/*
 * $Id: zlib.h,v 1.4 2005-03-03 12:13:41 Trocotronic Exp $ 
 */
 
#include "libzlib.h"

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

extern char *descomprime(Sock *, char *, int *);
extern char *comprime(Sock *, char *, int *, int);
DLLFUNC extern void libera_zlib(Sock *);
DLLFUNC extern int inicia_zlib(Sock *, int);
