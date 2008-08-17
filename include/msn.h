/*
 * $Id: msn.h,v 1.4 2008/02/16 23:19:43 Trocotronic Exp $ 
 */

typedef struct _msnsb MSNSB;
typedef struct _msncl MSNCl;
struct _msnsb
{
	struct _msnsb *sig;
	Sock *SBsck;
	MSNCl *mcl;
	char SBMsg[BUFSIZE];
	char *SBToken;
	char *SBId;
};
struct MSNComs
{
	char *com;
	int (*func)(MSNSB *, char **, int);
	int params;
	int master;
};
struct _msncl
{
	MSNCl *sig, *prev, *hsig;
	char *cuenta;
	char *nombre;
	char *alias;
	unsigned master:1;
	int estado;
	Cliente *cl;
};
	
#define MSN_VER "VER 1 MSNP8 CVR0"
#define MSNFUNC(x) int (x)(MSNSB *sb, char **argv, int argc)
#define BT_MSN			1117
