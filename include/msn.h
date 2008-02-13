/*
 * $Id: msn.h,v 1.3 2008-02-13 18:45:23 Trocotronic Exp $ 
 */

typedef struct _msncn MSNCn;
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
typedef struct _msnlcl MSNLCl;
struct _msnlcl
{
	struct _msnlcl *sig;
	MSNCl *mcl;
	MSNCn *mcn;
};
typedef struct _msnlcl MSNLCn;
struct _msnlcn
{
	struct _msnlcn *sig;
	MSNCl *mcl;
	MSNCn *mcn;
};
struct _msncn
{
	MSNCn *sig, *prev, *hsig;
	Canal *cn;
	MSNLCl *lcl;
};
struct _msncl
{
	MSNCl *sig, *prev, *hsig;
	char *cuenta;
	char *nombre;
	char *alias;
	unsigned master:1;
	int estado;
	MSNLCn *lcn;
};
	
#define MSN_VER "VER 1 MSNP8 CVR0"
#define MSNFUNC(x) int (x)(MSNSB *sb, char **argv, int argc)
#define BT_MSN			1117
