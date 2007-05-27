/*
 * $Id: statserv.h,v 1.5 2007-05-27 19:14:36 Trocotronic Exp $ 
 */

typedef struct _ss StatServ;
struct _ss
{
	Modulo *hmod;
	int laguea;
	char *template;
};
typedef struct _stsserv StsServ;
struct _stsserv
{
	StsServ *sig;
	Cliente *cl;
	u_int users;
	u_int operadores;
	u_int max_users;
	u_int lag;
	char *version;
	u_int uptime;
	Timer *crono;
};
typedef struct _stschar StsChar;
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
#define CogeInt(x) (u_int)atoi(SQLCogeRegistro(SS_SQL, x, "valor"))
#define ActualizaInt(x, y) SQLInserta(SS_SQL, x, "valor", "%u", y)
#define GetDay(x) (x->tm_yday)
#define GetWeek(x) ((x->tm_yday - x->tm_wday +7)/7)
#define GetMon(x) (x->tm_mon)
#define GetYear(x) (x->tm_year)
