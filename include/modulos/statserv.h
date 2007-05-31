/*
 * $Id: statserv.h,v 1.6 2007-05-31 23:06:37 Trocotronic Exp $ 
 */

typedef struct _ss StatServ;
typedef struct _stschar StsChar;
struct _ss
{
	Modulo *hmod;
	int refresco;
};
typedef struct _stsserv StsServ;
struct _stsserv
{
	StsServ *sig;
	Cliente *cl;
	u_int users;
	u_int max_users;
	u_int lag;
	char *version;
	u_int uptime;
	Timer *crono;
};
struct _stschar
{
	StsChar *sig;
	char *item;
	char *valor;
	u_int users;
};
typedef struct _listcl ListCl;
struct _listcl
{
	ListCl *sig;
	Cliente *cl;
	StsChar *sts;
};
struct Stats
{
	u_int users;
	u_int opers;
	u_int chans;
	u_int servs;
	StsServ *stsserv;
	StsChar *stsver;
	StsChar *ststld;
	ListCl *stsopers;
}stats;

#define SS_SQL "stats"
#define GetDay(x) (x->tm_yday)
#define GetWeek(x) ((x->tm_yday - x->tm_wday +7)/7)
#define GetMon(x) (x->tm_mon)
#define GetYear(x) (x->tm_year)
#define MAX_TEMPLATES 16

#define SS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define SS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define SS_ERR_EMPT "\00304ERROR: %s"

#define USERS 0x1
#define CHANS 0x2
#define SERVS 0x4
#define OPERS 0x8
#define NO_INC 0x10
