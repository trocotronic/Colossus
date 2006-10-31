/*
 * $Id: md5.h,v 1.6 2006-10-31 23:49:10 Trocotronic Exp $ 
 */
 
#define MDInit MD5_Init
#define MDUpdate MD5_Update
#define MDFinal MD5_Final
#ifdef USA_SSL
#include <openssl/md5.h>
#elif !defined(_MD5_H)
#define _MD5_H

#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

/* MD5 context. */
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5_Init PROTO_LIST ((MD5_CTX *));
void MD5_Update PROTO_LIST
  ((MD5_CTX *, unsigned char *, unsigned int));
void MD5_Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

#endif

extern char *MDString(char *, u_int);
