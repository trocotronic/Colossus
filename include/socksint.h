/*
 * $Id: socksint.h,v 1.2 2007/02/02 17:43:03 Trocotronic Exp $
 */
/*
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
	int fd;
};
extern Componente comps[MAX_COMP];
extern Signatura sgn;
*/
