/*
 * $Id: msn.h,v 1.1 2008-02-11 00:05:43 Trocotronic Exp $ 
 */

typedef struct _msncn MSNCn;
typedef struct _msnsb MSNSB;
struct _msnsb
{
	struct _msnsb *sig;
	Sock *SBsck;
	char *SBcuenta;
	char SBMsg[BUFSIZE];
	char *SBToken;
	char *SBId;
	unsigned master:1;
	MSNCn *mcn;
};
struct MSNComs
{
	char *com;
	int (*func)(MSNSB *, char **, int);
	int params;
	int master;
};
typedef struct _msnlsb MSNLSB;
struct _msnlsb
{
	struct _msnlsb *sig;
	char *cuenta;
	MSNCn *mcn;
};
struct _msncn
{
	struct _msncn *sig;
	Canal *mcn;
	MSNLSB *lsb;
};
	
#define MSN_VER "VER 1 MSNP8 CVR0"
#define MSNFUNC(x) int (x)(MSNSB *sb, char **argv, int argc)
