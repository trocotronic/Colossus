/*
 * $Id: socksint.h,v 1.1 2007-01-18 12:46:27 Trocotronic Exp $ 
 */

#include "setup.h"

#define MAX_MDS 8
typedef struct _mds MDS;
struct _mds
{
	MDS *sig;
	Sock *sck;
	struct MD
	{
		Modulo *hmod;
		unsigned res:1;
	}md[MAX_MDS];
};
typedef struct _signatura Signatura;
struct _signatura
{
	char cpuid[64];
	MDS *mds;
	char sgn[2048];
};

#define MAX_COMP 32
typedef struct _componente Componente;
struct _componente
{
	Sock *sck;
	char *ruta;
	char *tmp;
	u_long ts;
	int fd;
};
extern Componente comps[MAX_COMP];
extern Signatura sgn;
