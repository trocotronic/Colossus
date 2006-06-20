/*
 * $Id: httpd.h,v 1.1 2006-06-20 13:21:33 Trocotronic Exp $ 
 */

#define HTTP_GET 1
#define HTTP_POST 2
#define HTTP_HEAD 3
#define HTTP_DESC 4

#define MAX_CON 16

typedef struct _hhead HHead;
typedef struct _hdir HDir;
//typedef int (*HDirFunc)(HHead *, HDir *, char **, char **);
#define HDIRFUNC(x) int (x)(HHead *hh, HDir *hd, char **errmsg, char **data)
struct _hhead
{
	int metodo;
	char *host;
	char *user_agent;
	char *referer;
	char *ruta;
	char *archivo;
	char *param_get;
	char *param_post;
	Sock *sck;
	char *ext;
	time_t lmod;
	unsigned noclosesock:1;
	u_int slot;
};
struct _hdir
{
	struct _hdir *sig;
	char *ruta;
	char *carpeta;
	HDIRFUNC(*func);
};

extern HDir *CreaHDir(char *, char *, HDIRFUNC(*));
extern int BorraHDir(HDir *);