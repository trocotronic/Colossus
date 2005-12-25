#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef UDB
#include "bdd.h"
#endif
#ifdef USA_ZLIB
#include "zip.h"
#endif

/* stuff de tklines. todas son globales */
#define TKL_GLINE 0
#define TKL_ZLINE 1
#define TKL_SHUN 2
#define TKL_SPAMF 3
#define TKL_QLINE 4
#define TKL_MAX 5 /* máximo! */

#define PROTOCOL 2305

double tburst;
static char *modcanales = NULL;
static char *modusers = NULL;
static char *autousers = NULL;
static char *autoopers = NULL;
#define NOSERVDEOP 0x40
LinkCliente *servidores = NULL;
long base64dec(char *);
char *base64enc(long);
Tkl *tklines[TKL_MAX];

IRCFUNC(m_chghost);
IRCFUNC(m_chgident);
IRCFUNC(m_chgname);
#ifdef UDB
IRCFUNC(m_db);
IRCFUNC(m_dbq);
#endif
IRCFUNC(m_eos);
IRCFUNC(m_netinfo);
IRCFUNC(m_sajoin);
IRCFUNC(m_sapart);
IRCFUNC(m_module);
IRCFUNC(m_msg);
IRCFUNC(m_whois);
IRCFUNC(m_nick);
IRCFUNC(m_topic);
IRCFUNC(m_quit);
IRCFUNC(m_kill);
IRCFUNC(m_ping);
IRCFUNC(m_pass);
IRCFUNC(m_join);
IRCFUNC(m_part);
IRCFUNC(m_mode);
IRCFUNC(m_kick);
IRCFUNC(m_version);
IRCFUNC(m_stats);
IRCFUNC(m_server);
IRCFUNC(m_squit);
IRCFUNC(m_umode);
IRCFUNC(m_error);
IRCFUNC(m_away);
IRCFUNC(m_105);
IRCFUNC(m_rehash);
IRCFUNC(sincroniza);
IRCFUNC(m_sjoin);
IRCFUNC(m_tkl);

ProtInfo PROT_INFO(Unreal) = {
	"Protocolo UnrealIRCd" ,
	0.5 ,
	"Trocotronic" ,
	"trocotronic@rallados.net" 
};
/*Com comandos[] = {
	{ "CONNECT" , "7" } ,
	{ "CHATOPS" , "p" } ,
	{ "ADMIN" , "@" } ,
	{ "ADDOMOTD" , "AR" } ,
	{ "ADDLINE" , "z" } ,
	{ "TOPIC" , ")" } ,
	{ "SERVER" , "'" } ,
	{ "GLINE" , "}" } ,
	{ "SHUN" , "BL" } ,
	{ "TEMPSHUN" , "Tz" } ,
	{ "WHOIS" , "#" } ,
	{ "NETINFO" , "AO" } ,
	{ "KILL" , "." } ,
	{ "SVSMODE" , "n" } ,
	{ "SVS2MODE" , "v" } ,
#ifdef UDB
	{ "SVS3MODE" , "vv" } ,
#endif
	{ "VHOST" , "BE" } ,
	{ "OPER" , ";" } ,
	{ "SJOIN" , "~" } ,
	{ "WHO" , "\"" } ,
	{ "PROTOCTL" , "_" } ,
	{ "LINKS" , "0" } ,
	{ "SAPART" , "AY" } ,
	{ "SAJOIN" , "AX" } ,
	{ "UNZLINE" , "r" } ,
	{ "UNKLINE" , "X" } ,
	{ "TSCTL" , "AW" } ,
	{ "SWHOIS" , "BA" } ,
	{ "SVSWATCH" , "Bw" } ,
	{ "SVSSILENCE" , "Bs" } ,
	{ "SVSPART" , "BT" } ,
	{ "SVSO" , "BB" } ,
	{ "SVSNOOP" , "f" } ,
	{ "SVSNLINE" , "BR" } ,
	{ "SVSNICK" , "e" } ,
	{ "SVSMOTD" , "AS" } ,
	{ "SVSLUSERS" , "BU" } ,
	{ "SVSJOIN" , "BX" } ,
	{ "SETNAME" , "AE" } ,
	{ "SETHOST" , "AA" } ,
	{ "SENDUMODE" , "AP" } ,
	{ "SMO" , "AU" } ,
	{ "SDESC" , "AG" } ,
	{ "RPING" , "AM" } ,
	{ "RPONG" , "AN" } ,
	{ "RAKILL" , "Y" } ,
	{ "NACHAT" , "AC" } ,
	{ "MKPASSWD" , "y" } ,
	{ "LAG" , "AF" } ,
	{ "HTM" , "BH" } ,
	{ "DUMMY" , "DU" } ,
	{ "CYCLE" , "BP" } ,
	{ "CHGNAME" , "BK" } ,
	{ "CHGIDENT" , "AZ" } ,
	{ "CHGHOST" , "AL" } ,
	{ "AWAY" , "6" } ,
	{ "AKILL" , "V" } ,
	{ "ADMINCHAT" , "x" } ,
	{ "TIME" , ">" } ,
	{ "SVSNO" , "BV" } ,
	{ "SVS2NO" , "BW" } ,
	{ "SVSKILL" , "h" } ,
	{ "SENDSNO" , "Ss" } ,
	{ "LIST" , "(" } ,
	{ "ADDMOTD" , "AQ" } ,
	{ "WHOWAS" , "$" } ,
	{ "WALLOPS" , "=" } ,
	{ "USERHOST" , "J" } ,
	{ "UNSQLINE" , "d" } ,
	{ "UNDCCDENY" , "BJ" } ,
	{ "UMODE2" , "|" } ,
	{ "TRACE" , "b" } ,
	{ "SVSFLINE" , "BC" } ,
	{ "SQUIT" , "-" } ,
	{ "SQLINE" , "c" } ,
	{ "SILENCE" , "U" } ,
	{ "SAMODE" , "o" } ,
	{ "RULES" , "t" } ,
	{ "QUIT" , "," } ,
	{ "PING" , "8" } ,
	{ "PONG" , "9" } ,
	{ "PASS" , "<" } ,
	{ "PRIVMSG" , "!" } ,
	{ "NOTICE" , "B" } ,
	{ "MAP" , "u" } ,
	{ "LOCOPS" , "^" } ,
	{ "KNOCK" , "AI" } ,
	{ "KICK" , "H" } ,
	{ "ISON" , "K" } ,
	{ "INVITE" , "*" } ,
	{ "HELP" , "4" } ,
	{ "GLOBOPS" , "]" } ,
	{ "EOS" , "ES" } ,
	{ "DCCDENY" , "BI" } ,
	{ "CLOSE" , "Q" } ,
	{ "STATS" , "2" } ,
	{ "JOIN" , "C" } ,
	{ "PART" , "D" } ,
	{ "MODE" , "G" } ,
	{ "NICK" , "&" } ,
	{ "ERROR" , "5" } ,
	{ "DNS" , "T" } ,
	{ "TKL" , "BD" } ,
	{ "MOTD" , "F" } ,
	{ "USER" , "%" } ,
	{ "INFO" , "/" } ,
	{ "SUMMON" , "1" } ,
	{ "USERS" , "3" } ,
	{ "VERSION" , "+" } ,
	{ "NAMES" , "?" } ,
	{ "LUSERS" , "E" } ,
	{ "SERVICE" , "I" } ,
	{ "WATCH" , "`" } ,
	{ "DALINFO" , "w" } ,
	{ "CREDITS" , "AJ" } ,
	{ "LICENCE" , "AK" } ,
	{ "BOTMOTD" , "BF" } ,
	{ "OPERMOTD" , "AV" } ,
	{ "MODULE" , "BQ" } ,
	{ "ALIAS" , "AH" } ,
	{ "REHASH" , "O" } ,
	{ "DIE" , "R" } ,
	{ "RESTART" , "P" } ,
	{ "DB" , "DB" } ,
	{ "DBQ" , "DBQ" } ,
	{ "GHOST" , "GT" } ,
	{ NULL , NULL }
};*/

#define MSG_SWHOIS "SWHOIS"
#define TOK_SWHOIS "BA"
#define MSG_SVSNICK "SVSNICK"
#define TOK_SVSNICK "e"
#define MSG_SVSJOIN "SVSJOIN"
#define TOK_SVSJOIN "BX"
#define MSG_SVSPART "SVSPART"
#define TOK_SVSPART "BT"
#define MSG_SQLINE "SQLINE"
#define TOK_SQLINE "c"
#define MSG_UNSQLINE "UNSQLINE"
#define TOK_UNSQLINE "d"
#define MSG_LAG "LAG"
#define TOK_LAG "AF"
#ifdef UDB
#define MSG_SVS3MODE "SVS3MODE"
#define TOK_SVS3MODE "vv"
#endif
#define MSG_SVS2MODE "SVS2MODE"
#define TOK_SVS2MODE "v"

#define MSG_PRIVATE "PRIVMSG"
#define TOK_PRIVATE "!"
#define MSG_WHOIS "WHOIS"
#define TOK_WHOIS "#"
#define MSG_NICK "NICK"
#define TOK_NICK "&"
#define MSG_TOPIC "TOPIC"
#define TOK_TOPIC ")"
#define MSG_QUIT "QUIT"
#define TOK_QUIT ","
#define MSG_KILL "KILL"
#define TOK_KILL "."
#define MSG_PING "PING"
#define TOK_PING "8"
#define MSG_PONG "PONG"
#define TOK_PONG "9"
#define MSG_PASS "PASS"
#define TOK_PASS "<"
#define MSG_NOTICE "NOTICE"
#define TOK_NOTICE "B"
#define MSG_JOIN "JOIN"
#define TOK_JOIN "C"
#define MSG_PART "PART"
#define TOK_PART "D"
#define MSG_MODE "MODE"
#define TOK_MODE "G"
#define MSG_KICK "KICK"
#define TOK_KICK "H"
#define MSG_VERSION "VERSION"
#define TOK_VERSION "+"
#define MSG_STATS "STATS"
#define TOK_STATS "2"
#define MSG_SERVER "SERVER"
#define TOK_SERVER "'"
#define MSG_SQUIT "SQUIT"
#define TOK_SQUIT "-"
#define MSG_UMODE2 "UMODE2"
#define TOK_UMODE2 "|"
#define MSG_ERROR "ERROR"
#define TOK_ERROR "5"
#define MSG_AWAY "AWAY"
#define TOK_AWAY "6"
#define MSG_REHASH "REHASH"
#define TOK_REHASH "O"
#define MSG_CHGHOST "CHGHOST"
#define TOK_CHGHOST "AL"
#define MSG_CHGIDENT "CHGIDENT"
#define TOK_CHGIDENT "AZ"
#define MSG_CHGNAME "CHGNAME"
#define TOK_CHGNAME "BK"
#ifdef UDB
#define MSG_DB "DB"
#define TOK_DB "DB"
#define MSG_DBQ "DBQ"
#define TOK_DBQ "DBQ"
#endif
#define MSG_EOS "EOS"
#define TOK_EOS "ES"
#define MSG_NETINFO "NETINFO"
#define TOK_NETINFO "AO"
#define MSG_SAJOIN "SAJOIN"
#define TOK_SAJOIN "AX"
#define MSG_SAPART "SAPART"
#define TOK_SAPART "AY"
#define MSG_MODULE "MODULE"
#define TOK_MODULE "BQ"
#define MSG_WALLOPS "WALLOPS"
#define TOK_WALLOPS "="
#define MSG_SETHOST "SETHOST"
#define TOK_SETHOST "AA"
#define MSG_INVITE "INVITE"
#define TOK_INVITE "*"
#define MSG_GLINE "GLINE"
#define TOK_GLINE "}"
#define MSG_SJOIN "SJOIN"
#define TOK_SJOIN "~"
#define MSG_TKL "TKL"
#define TOK_TKL "BD"

#define U_INVISIBLE 	0x1
#define U_OPER 		0x2
#define U_WALLOP  	0x4
#define U_FAILOP  	0x8
#define U_HELPOP  	0x10
#define U_REGNICK  	0x20
#define U_SADMIN  	0x40
#define U_ADMIN  	0x80
#define U_RGSTRONLY  	0x100
#define U_WEBTV  	0x200
#define U_SERVICES  	0x400
#define U_HIDE 		0x800
#define U_NETADMIN 	0x1000
#define U_COADMIN 	0x2000
#define U_WHOIS 	0x4000
#define U_KIX 		0x8000
#define U_BOT 		0x10000
#define U_SECURE 	0x20000
#define U_VICTIM 	0x40000
#define U_DEAF 		0x80000
#define U_HIDEOPER 	0x100000
#define U_SETHOST	0x200000
#define U_STRIPBADWORDS 0x400000
#define U_HIDEWHOIS 	0x800000
#define U_NOCTCP 	0x1000000
#define U_LOCOP		0x20000000
#ifdef UDB
#define U_SUSPEND 	0x40000000
#define U_PRIVDEAF	0x80000000
#define U_SHOWIP	0x100000000
#endif

#define C_CHANOP	0x1
#define C_VOICE		0x2
#define C_PRIVATE	0x4
#define C_SECRET	0x8
#define C_MODERATED  	0x10
#define C_TOPICLIMIT 	0x20
#define C_CHANOWNER	0x40
#define C_CHANPROT	0x80
#define C_HALFOP	0x100
#define C_EXCEPT	0x200
#define C_BAN		0x400
#define C_INVITEONLY 	0x800
#define C_NOPRIVMSGS 	0x1000
#define C_KEY		0x2000
#define C_LIMIT		0x4000
#define C_RGSTR		0x8000
#define C_RGSTRONLY 	0x10000
#define C_LINK		0x20000
#define C_NOCOLOR	0x40000
#define C_OPERONLY   	0x80000
#define C_ADMONLY   	0x100000
#define C_NOKICKS   	0x200000
#define C_STRIP	   	0x400000
#define C_NOKNOCK	0x800000
#define C_NOINVITE  	0x1000000
#define C_FLOODLIMIT	0x2000000
#define C_MODREG	0x4000000
#define C_STRIPBADWORDS	0x8000000
#define C_NOCTCP	0x10000000
#define C_AUDITORIUM	0x20000000
#define C_ONLYSECURE	0x40000000
#define C_NONICKCHANGE	0x80000000

mTab PROT_UMODOS(Unreal)[] = {
	{ U_REGNICK , 'r' },
	{ U_NETADMIN , 'N' },
	{ U_OPER , 'o' },
	{ U_HELPOP , 'h' },
	{ U_HIDE , 'x' },
#ifdef UDB
	{ U_SUSPEND , 'S' },
	{ U_PRIVDEAF, 'D' },
#endif
	{ U_INVISIBLE , 'i' },
	{ U_WALLOP , 'w' },
	{ U_FAILOP , 'g' },
	{ U_SADMIN , 'a' },
	{ U_ADMIN , 'A' },
	{ U_RGSTRONLY , 'R' },
	{ U_WEBTV , 'V' },
#ifdef UDB
	{ U_SERVICES , 'k' },
#else
	{ U_SERVICES , 'S' },
#endif
	{ U_COADMIN , 'C' },
	{ U_WHOIS , 'W' },
	{ U_KIX , 'q' },
	{ U_BOT , 'B' },
	{ U_SECURE , 'z' },
	{ U_VICTIM , 'v' },
	{ U_DEAF , 'd' },
	{ U_HIDEOPER , 'H' },
	{ U_SETHOST , 't' },
	{ U_STRIPBADWORDS , 'G' },
	{ U_HIDEWHOIS , 'p' },
	{ U_NOCTCP , 'T' },
	{ U_LOCOP , 'O' },
	{ 0x0 , '0' } /* el caracter 0 (no \0) indica fin de array */
};

mTab PROT_CMODOS(Unreal)[] = {
	{ C_RGSTR , 'r' },
	{ C_RGSTRONLY , 'R' },
	{ C_OPERONLY , 'O' },
	{ C_ADMONLY , 'A' },
	{ C_HALFOP , 'h' },
	{ C_CHANPROT , 'a' },
	{ C_CHANOWNER , 'q' },
	{ C_CHANOP , 'o' },
	{ C_VOICE , 'v' },
	{ C_PRIVATE , 'p' },
	{ C_SECRET , 's' },
	{ C_MODERATED , 'm' },
	{ C_TOPICLIMIT , 't' },
	{ C_EXCEPT , 'e' },
	{ C_BAN , 'b' },
	{ C_INVITEONLY , 'i' },
	{ C_NOPRIVMSGS , 'n' },
	{ C_KEY , 'k' },
	{ C_LIMIT , 'l' },
	{ C_LINK , 'L' },
	{ C_NOCOLOR , 'c' },
	{ C_NOKICKS , 'Q' },
	{ C_STRIP , 'S' },
	{ C_NOKNOCK , 'K' },
	{ C_NOINVITE , 'V' },
	{ C_FLOODLIMIT , 'f' },
	{ C_MODREG , 'M' },
	{ C_STRIPBADWORDS , 'G' },
	{ C_NOCTCP , 'C' },
	{ C_AUDITORIUM , 'u' },
	{ C_ONLYSECURE , 'z' },
	{ C_NONICKCHANGE , 'N' },
	{ 0x0 , '0' }
};

void ProcesaModos(Canal *, char *);
void ProcesaModo(Cliente *, Canal *, char **, int);
void EntraCliente(Cliente *, char *);
#ifdef UDB
void UdbDaleCosas(Cliente *);
int UdbCompruebaOpts(Proc *);
#endif
char *DecodeIP(char *buf)
{
	int len = strlen(buf);
	char targ[25];
	b64_decode(buf, targ, 25);
	if (len == 8)
		return inet_ntoa(*(struct in_addr *)targ);
	return NULL;
}
char *EncodeIP(char *ip)
{
	struct in_addr ia;
	static char buf[25];
	u_char *cp;
	ia.s_addr = inet_addr(ip);
	cp = (u_char *)ia.s_addr;
	b64_encode((char *)&cp, sizeof(struct in_addr), buf, 25);
	return buf;
}
int p_msg_vl(Cliente *, Cliente *, char, char *, va_list *);

char *p_trio(Cliente *cl)
{
	return cl->nombre;
}
int p_umode(Cliente *cl, char *modos)
{
	ProcesaModosCliente(cl, modos);
	return 0;
}
int p_svsmode(Cliente *cl, Cliente *bl, char *modos, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, modos);
	ircvsprintf(buf, modos, vl);
	va_end(vl);
	ProcesaModosCliente(cl, modos);
#ifdef UDB
	EnviaAServidor(":%s %s %s %s %lu", bl->nombre, TOK_SVS3MODE, cl->nombre, buf, time(0));
#else
	EnviaAServidor(":%s %s %s %s %lu", bl->nombre, TOK_SVS2MODE, cl->nombre, buf, time(0));
#endif
	Senyal2(SIGN_UMODE, cl, modos);
	return 0;
}
int p_mode(Cliente *cl, Canal *cn, char *modos, ...)
{
	char buf[BUFSIZE], *copy;
	va_list vl;
	va_start(vl, modos);
	ircvsprintf(buf, modos, vl);
	va_end(vl);
	copy = strdup(buf);
	ProcesaModos(cn, buf);
	EnviaAServidor(":%s %s %s %s", cl->nombre, TOK_MODE, cn->nombre, copy);
	Free(copy);
	return 0;
}
int p_nick(Cliente *al, char *nuevo)
{
	EnviaAServidor(":%s %s %s %lu", al->nombre, TOK_NICK, nuevo, time(0));
	return 0;
}
int p_svsnick(Cliente *al, char *nuevo)
{
	EnviaAServidor(":%s %s %s %s %lu", me.nombre, TOK_SVSNICK, al->nombre, nuevo, time(0));
	return 0;
}
int p_join(Cliente *bl, Canal *cn)
{
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_JOIN, cn->nombre);
	return 0;
}
int p_svsjoin(Cliente *cl, char *canal)
{
	EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, cl->nombre, canal);
	return 0;
}
int p_part(Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_PART, cn->nombre, buf);
	}
	else
		EnviaAServidor(":%s %s %s", bl->nombre, TOK_PART, cn->nombre);
	return 0;
}
int p_svspart(Cliente *cl, Canal *cn, char *motivo, ...)
{
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor(":%s %s %s %s :%s", me.nombre, TOK_SVSPART, cl->nombre, cn->nombre, buf);
	}
	else
		EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSPART, cl->nombre, cn->nombre);
	return 0;
}
int p_quit(Cliente *bl, char *motivo, ...)
{
	LinkCanal *lk;
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor(":%s %s :%s", bl->nombre, TOK_QUIT, buf);
	}
	else
		EnviaAServidor(":%s %s", bl->nombre, TOK_QUIT);
	for (lk = bl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->chan, bl);
	LiberaMemoriaCliente(bl);
	return 0;
}
int p_kill(Cliente *cl, Cliente *bl, char *motivo, ...)
{
	LinkCanal *lk;
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_KILL, cl->nombre, buf);
	}
	else
		EnviaAServidor(":%s %s %s :Usuario desconectado.", bl->nombre, TOK_KILL, cl->nombre);
	Senyal2(SIGN_QUIT, cl, buf);
	for (lk = cl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->chan, cl);
	LiberaMemoriaCliente(cl);
	return 0;
}
int p_nuevonick(Cliente *al)
{
	gethostname(buf, sizeof(buf));
	EnviaAServidor("%s %s 1 %lu %s %s %s 0 +%s %s %s :%s", TOK_NICK, al->nombre, time(0), al->ident, al->host, al->server->nombre, ModosAFlags(al->modos, PROT_UMODOS(Unreal), NULL), al->host, EncodeIP(buf), al->info);
	return 0;
}
int p_priv(Cliente *cl, Cliente *bl, char *mensaje, ...)
{
	va_list vl;
	if (!cl) /* algo pasa */
		return 1;
	if (EsBot(cl))
		return 1;
	va_start(vl, mensaje);
	p_msg_vl(cl, bl, 1, mensaje, &vl);
	va_end(vl);
	return 0;
}
int p_sethost(Cliente *bl, char *host)
{
	ircstrdup(bl->host, host);
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_SETHOST, host);
	return 0;
}
int p_chghost(Cliente *cl, char *host)
{
	ircstrdup(cl->vhost, host);
	GeneraMascara(cl);
	EnviaAServidor(":%s %s %s %s", me.nombre, TOK_CHGHOST, cl->nombre, host);
	return 0;
}
int p_sqline(char *nick, char modo, char *motivo)
{
	if (modo == ADD)
	{
		if (motivo)
			EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_SQLINE, nick, motivo);
		else
			EnviaAServidor(":%s %s %s", me.nombre, TOK_SQLINE, nick);
	}
	else
		EnviaAServidor(":%s %s %s", me.nombre, TOK_UNSQLINE, nick);
	return 0;
}
int p_lag(Cliente *cl, Cliente *bl)
{
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_LAG, cl->nombre);
	return 0;
}
int p_swhois(Cliente *cl, char *swhois)
{
	EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_SWHOIS, cl->nombre, swhois);
	return 0;
}
int p_tkl(Cliente *bl, char modo, char *ident, char *host, int tiempo, char *motivo)
{
	if (modo == ADD)
		EnviaAServidor(":%s TKL + G %s %s %s %lu %lu :%s", me.nombre, ident, host, bl->mask, 
			!tiempo ? 0 : time(0) + tiempo, time(0), motivo ? motivo : "");
	else if (modo == DEL)
		EnviaAServidor(":%s TKL - G %s %s %s", me.nombre, ident, host, bl->mask);
	return 0;
}
int p_kick(Cliente *cl, Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (!cl || !cn)
		return 1;
	if (EsServidor(cl) || EsBot(cl))
		return 1;
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor(":%s %s %s %s :%s", bl->nombre, TOK_KICK, cn->nombre, cl->nombre, buf);
	}
	else
		EnviaAServidor(":%s %s %s %s :Usuario expulsado", bl->nombre, TOK_KICK, cn->nombre, cl->nombre);
	BorraCanalDeCliente(cl, cn);
	BorraClienteDeCanal(cn, cl);
	return 0;
}
int p_topic(Cliente *bl, Canal *cn, char *topic)
{
	if (!cn->topic || strcmp(cn->topic, topic))
	{
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_TOPIC, cn->nombre, topic);
		ircstrdup(cn->topic, topic);
	}
	return 0;
}
int p_notice(Cliente *cl, Cliente *bl, char *mensaje, ...)
{
	va_list vl;
	if (!cl) /* algo pasa */
		return 1;
	if (EsBot(cl))
		return 1;
	va_start(vl, mensaje);
	p_msg_vl(cl, bl, 0, mensaje, &vl);
	va_end(vl);
	return 0;
}
int p_invite(Cliente *cl, Cliente *bl, Canal *cn)
{
	EnviaAServidor(":%s %s %s %s", bl->nombre, TOK_INVITE, cl->nombre, cn->nombre);
	return 0;
}
int p_msg_vl(Cliente *cl, Cliente *bl, char tipo, char *formato, va_list *vl)
{
	char buf[BUFSIZE];
	if (!vl)
		strcpy(buf, formato);
	else
		ircvsprintf(buf, formato, *vl);
	if (tipo == 1) 
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_PRIVATE, cl->nombre, buf);
	else
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_NOTICE, cl->nombre, buf);
	return 0;
}
/*
 * Lista de comandos propio de cada ircd
 * Cada posición está asignada, no debe alterarse el orden
 * Si un comando no existe, debe especificarse COM_NULL en esa posición
 */
Com PROT_COMANDOS(Unreal)[] = {
	{ MSG_NULL , TOK_NULL , (void *)p_trio } ,
	{ MSG_UMODE2 , TOK_UMODE2 , (void *)p_umode } ,
#ifdef UDB
	{ MSG_SVS3MODE , TOK_SVS3MODE ,(void *) p_svsmode } ,
#else
	{ MSG_SVS2MODE , TOK_SVS2MODE , (void *)p_svsmode } ,
#endif
	{ MSG_MODE , TOK_MODE , (void *)p_mode } ,
	{ MSG_NICK , TOK_NICK , (void *)p_nick } ,
	{ MSG_SVSNICK , TOK_SVSNICK , (void *)p_svsnick } ,
	{ MSG_JOIN , TOK_JOIN , (void *)p_join } ,
	{ MSG_SVSJOIN , TOK_SVSJOIN , (void *)p_svsjoin } ,
	{ MSG_PART , TOK_PART , (void *)p_part } ,
	{ MSG_SVSPART , TOK_SVSPART , (void *)p_svspart } ,
	{ MSG_QUIT , TOK_QUIT , (void *)p_quit } ,
	{ MSG_KILL , TOK_KILL , (void *)p_kill } ,
	{ MSG_NICK , TOK_NICK , (void *)p_nuevonick } ,
	{ MSG_PRIVATE , TOK_PRIVATE , (void *)p_priv } ,
	{ MSG_SETHOST , TOK_SETHOST , (void *)p_sethost } ,
	{ MSG_CHGHOST , TOK_CHGHOST , (void *)p_chghost } ,
	{ MSG_SQLINE , TOK_SQLINE , (void *)p_sqline } ,
	{ MSG_LAG , TOK_LAG , (void *)p_lag } ,
	{ MSG_SWHOIS , TOK_SWHOIS , (void *)p_swhois } ,
	{ MSG_GLINE , TOK_GLINE , (void *)p_tkl } ,
	{ MSG_KICK , TOK_KICK , (void *)p_kick } ,
	{ MSG_TOPIC , TOK_TOPIC , (void *)p_topic } ,
	{ MSG_NOTICE , TOK_NOTICE , (void *)p_notice } ,
	{ MSG_INVITE , TOK_INVITE , (void *)p_invite } ,
	{ MSG_NULL , TOK_NULL , (void *)p_msg_vl } ,
	COM_NULL
};
int test(Conf *config, int *errores)
{
	Conf *eval, *aux;
	short error_parcial = 0;
	if ((eval = BuscaEntrada(config, "autojoin")))
	{
		if ((aux = BuscaEntrada(eval, "usuarios")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz usuarios esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
			if (*(aux->data) != '#')
			{
				Error("[%s:%s::%s::%s::%i] Los canales deben empezar por #.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if ((aux = BuscaEntrada(eval, "opers")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz opers esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
			if (*(aux->data) != '#')
			{
				Error("[%s:%s::%s::%s::%i] Los canales deben empezar por #.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
	}
	if ((eval = BuscaEntrada(config, "modos")))
	{
		if ((aux = BuscaEntrada(eval, "usuarios")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz usuarios esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if ((aux = BuscaEntrada(eval, "canales")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz canales esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
			else
			{
				char forb[] = "ohvbeqar";
				int i;
				for (i = 0; forb[i] != '\0'; i++)
				{
					if (strchr(aux->data, forb[i]))
					{
						Error("[%s:%s::%s::%s::%i] Los modos +ohvbeqar no se permiten.", config->archivo, config->item, eval->item, aux->item, aux->linea);
						error_parcial++;
					}
				}
			}
		}
	}
	if ((eval = BuscaEntrada(config, "parametros")))
	{
		if ((aux = BuscaEntrada(eval, "clientes")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz clientes está vacía.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if ((aux = BuscaEntrada(eval, "mascaras")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz mascaras está vacía.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if ((aux = BuscaEntrada(eval, "resto1")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz resto está vacía.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if ((aux = BuscaEntrada(eval, "resto2")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz resto está vacía.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
	}	
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config)
{
	int i, p;
	char *modcl = "qaohv", *modmk = "beI", *modpm1 = "kfL", *modpm2 = "lj";
	if (!conf_set)
		BMalloc(conf_set, struct Conf_set);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "no_server_deop"))
			conf_set->opts |= NOSERVDEOP;
		else if (!strcmp(config->seccion[i]->item, "autojoin"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "usuarios"))
					ircstrdup(autousers, config->seccion[i]->seccion[p]->data);
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "opers"))
					ircstrdup(autousers, config->seccion[i]->seccion[p]->data);
			}
		}
		else if (!strcmp(config->seccion[i]->item, "modos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "usuarios"))
					ircstrdup(modusers, config->seccion[i]->seccion[p]->data);
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "canales"))
					ircstrdup(modcanales, config->seccion[i]->seccion[p]->data);
			}
		}
		else if (!strcmp(config->seccion[i]->item, "parametros"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "clientes"))
					modcl = config->seccion[i]->seccion[p]->data;
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "mascaras"))
					modmk = config->seccion[i]->seccion[p]->data;
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "resto1"))
					modpm1 = config->seccion[i]->seccion[p]->data;
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "resto2"))
					modpm2 = config->seccion[i]->seccion[p]->data;
			}
		}
	}
	ircstrdup(protocolo->modcl, modcl);
	ircstrdup(protocolo->modmk, modmk);
	ircstrdup(protocolo->modpm1, modpm1);
	ircstrdup(protocolo->modpm2, modpm2);
	bzero(tklines, sizeof(tklines));
#ifdef UDB
	IniciaProceso(UdbCompruebaOpts);
#endif
}
int PROT_CARGA(Unreal)(Conf *config)
{
	int errores = 0;
	if (!test(config, &errores))
		set(config);
	else
	{
		Error("[%s] La configuracion de %s es errónea", config->archivo, PROT_INFO(Unreal).nombre);
		return ++errores;
	}
	InsertaComando(MSG_PRIVATE, TOK_PRIVATE, m_msg, INI, 2);
	InsertaComando(MSG_WHOIS, TOK_WHOIS, m_whois, INI, MAXPARA);
	InsertaComando(MSG_NICK, TOK_NICK, m_nick, INI, MAXPARA);
	InsertaComando(MSG_TOPIC, TOK_TOPIC, m_topic, INI, 4);
	InsertaComando(MSG_QUIT, TOK_QUIT, m_quit, FIN, 1);
	InsertaComando(MSG_KILL, TOK_KILL, m_kill, INI, 2);
	InsertaComando(MSG_PING, TOK_PING, m_ping, INI, MAXPARA);
	InsertaComando(MSG_PASS, TOK_PASS, m_pass, INI, 1);
	InsertaComando(MSG_NOTICE, TOK_NOTICE, m_msg, INI, 2);
	InsertaComando(MSG_JOIN, TOK_JOIN, m_join, INI, MAXPARA);
	InsertaComando(MSG_PART, TOK_PART, m_part, FIN, 2);
	InsertaComando(MSG_MODE, TOK_MODE, m_mode, INI, MAXPARA);
	InsertaComando(MSG_KICK, TOK_KICK, m_kick, INI, 3);
	InsertaComando(MSG_VERSION, TOK_VERSION, m_version, INI, MAXPARA);
	InsertaComando(MSG_STATS, TOK_STATS, m_stats, INI, 3);
	InsertaComando(MSG_SERVER, TOK_SERVER, m_server, INI, MAXPARA);
	InsertaComando(MSG_SQUIT, TOK_SQUIT, m_squit, FIN, 2);
	InsertaComando(MSG_UMODE2, TOK_UMODE2, m_umode, INI, 2);
	InsertaComando(MSG_ERROR, TOK_ERROR, m_error, INI, MAXPARA);
	InsertaComando(MSG_AWAY, TOK_AWAY, m_away, INI, 1);
	InsertaComando("105", "105", m_105, INI, MAXPARA);
	InsertaComando(MSG_REHASH, TOK_REHASH, m_rehash, INI, MAXPARA);
	InsertaComando(MSG_CHGHOST, TOK_CHGHOST, m_chghost, INI, MAXPARA);
	InsertaComando(MSG_CHGIDENT, TOK_CHGIDENT, m_chgident, INI, MAXPARA);
	InsertaComando(MSG_CHGNAME, TOK_CHGNAME, m_chgname, INI, 2);
#ifdef UDB
	InsertaComando(MSG_DB, TOK_DB, m_db, INI, 5);
	InsertaComando(MSG_DBQ, TOK_DBQ, m_dbq, INI, 2);
#endif
	InsertaComando(MSG_EOS, TOK_EOS, m_eos, INI, MAXPARA);
	InsertaComando(MSG_NETINFO, TOK_NETINFO, m_netinfo, INI, MAXPARA);
	InsertaComando(MSG_SAJOIN, TOK_SAJOIN, m_sajoin, INI, MAXPARA);
	InsertaComando(MSG_SAPART, TOK_SAPART, m_sapart, INI, MAXPARA);
	InsertaComando(MSG_MODULE, TOK_MODULE, m_module, INI, MAXPARA);
	InsertaComando(MSG_SJOIN, TOK_SJOIN, m_sjoin, INI, MAXPARA);
	InsertaComando(MSG_TKL, TOK_TKL, m_tkl, INI, MAXPARA);
	return 0;
}
int PROT_DESCARGA(Unreal)()
{
	BorraComando(MSG_PRIVATE, m_msg);
	BorraComando(MSG_WHOIS, m_whois);
	BorraComando(MSG_NICK, m_nick);
	BorraComando(MSG_TOPIC, m_topic);
	BorraComando(MSG_QUIT, m_quit);
	BorraComando(MSG_KILL, m_kill);
	BorraComando(MSG_PING, m_ping);
	BorraComando(MSG_PASS, m_pass);
	BorraComando(MSG_NOTICE, m_msg);
	BorraComando(MSG_JOIN, m_join);
	BorraComando(MSG_PART, m_part);
	BorraComando(MSG_MODE, m_mode);
	BorraComando(MSG_KICK, m_kick);
	BorraComando(MSG_VERSION, m_version);
	BorraComando(MSG_STATS, m_stats);
	BorraComando(MSG_SERVER, m_server);
	BorraComando(MSG_SQUIT, m_squit);
	BorraComando(MSG_UMODE2, m_umode);
	BorraComando(MSG_ERROR, m_error);
	BorraComando(MSG_AWAY, m_away);
	BorraComando("105", m_105);
	BorraComando(MSG_REHASH, m_rehash);
	BorraComando(MSG_CHGHOST, m_chghost);
	BorraComando(MSG_CHGIDENT, m_chgident);
	BorraComando(MSG_CHGNAME, m_chgname);
#ifdef UDB
	BorraComando(MSG_DB, m_db);
	BorraComando(MSG_DBQ, m_dbq);
#endif
	BorraComando(MSG_EOS, m_eos);
	BorraComando(MSG_NETINFO, m_netinfo);
	BorraComando(MSG_SAJOIN, m_sajoin);
	BorraComando(MSG_SAPART, m_sapart);
	BorraComando(MSG_MODULE, m_module);
	BorraComando(MSG_SJOIN, m_sjoin);
	BorraComando(MSG_TKL, m_tkl);
#ifdef UDB
	DetieneProceso(UdbCompruebaOpts);
#endif
	return 0;
}
void PROT_INICIA(Unreal)()
{
	LinkCliente *aux, *tmp;
	for (aux = servidores; aux; aux = tmp)
	{
		tmp = aux->sig;
		ircfree(aux);
	}
	servidores = NULL;
	EnviaAServidor("PROTOCTL NICKv2 VHP VL TOKEN UMODE2 NICKIP SJOIN SJ3 NS SJB64 TKLEXT");
#ifdef UDB
	EnviaAServidor("PROTOCTL UDB" UDB_VER "=%s,SD", me.nombre);
#endif
#ifdef USA_ZLIB
	if (conf_server->compresion)
		EnviaAServidor("PROTOCTL ZIP");
#endif
	EnviaAServidor("PASS :%s", conf_server->password.local);
	EnviaAServidor("SERVER %s 1 :U%i-%s-%i %s", me.nombre, PROTOCOL, me.trio, me.numeric, me.info);
}
SOCKFUNC(PROT_PARSEA(Unreal))
{
	char *p, *para[MAXPARA + 1], sender[HOSTLEN + 1], *s;
	u_char tpref = 0;
	int params, i, j;
	Comando *comd = NULL;
	Cliente *cl = linkado;
	para[0] = (linkado ? linkado->nombre : NULL);
	for (p = data; *p == ' '; p++);
	s = sender;
	*s = '\0';
	if (*p == ':' || *p == '@')
	{
		if (*p == '@')
			tpref = 1;
		for (++p; *p && *p != ' '; p++)
		{
			if (s < (sender + sizeof(sender) - 1))
				*s++ = *p;
			*s = '\0';
		}
		if (*sender)
		{
			if (tpref)
			{
				int numeric;
				LinkCliente *aux;
				numeric = base64dec(sender);
				for (aux = servidores; aux; aux = aux->sig)
				{
					if (aux->user->numeric == numeric)
					{
						cl = aux->user;
						break;
					}
				}
				if (cl)
					para[0] = cl->nombre;
			}
			else
			{
				para[0] = sender;
				if (!(cl = BuscaCliente(para[0], NULL)) && !strchr(para[0], '.'))
					Info("PANIC!! %s %s %X", data, para[0], cl);
			}
		}
		while (*p == ' ')
			p++;
	}
	if (*p == '\0') /* mensaje vacío? */
		return 1;
	s = strchr(p, ' ');
	if (s)
		*s++ = '\0';
	comd = BuscaComando(p);
	params = (comd ? comd->params : 0);
	i = 0;
	if (s)
	{
		for (;;)
		{
			while (*s == ' ')
				*s++ = '\0';
			if (*s == '\0')
				break;
			if (*s == ':')
			{
				para[++i] = s + 1;
				break;
			}
			para[++i] = s;
			if (i >= params)
				break;
			while (*s != ' ' && *s)
				s++;
		}
	}
	para[++i] = NULL;
	//for (params = 0; params < i; params++)
	//	Debug("parv[%i] = %s", params, para[params]);
	if (comd)
	{
		if (comd->cuando == INI)
		{
			for (j = 0; j < comd->funciones; j++)
			{
				if (comd->funcion[j])
					comd->funcion[j](sck, cl, para, i);
			}
		}
		else if (comd->cuando == FIN)
		{
			for (j = comd->funciones - 1; j >= 0; j--)
			{
				if (!cl)
					cl = BuscaCliente(para[0], NULL);
				if (comd->funcion[j])
					comd->funcion[j](sck, cl, para, i);
			}
		}
	}
	return 0;
}
IRCFUNC(m_msg)
{
	char *param[256], par[BUFSIZE];
	int params, i;
	Modulo *mod;
	Cliente *bl;
	strtok(parv[1], "@");
	parv[parc] = strtok(NULL, "@");
	parv[parc+1] = NULL;
	while (*parv[2] == ':')
		parv[2]++;
	strcpy(par, parv[2]);
	for (i = 0, param[i] = strtok(par, " "); param[i]; param[++i] = strtok(NULL, " "));
	params = i;
	bl = BuscaCliente(parv[1], NULL);
	if (!bl)
		return 1; /* algo passa! */
	if (!strcasecmp(param[0], "\1PING"))
	{
		EnviaAServidor(":%s %s %s :%s", parv[1], TOK_NOTICE, parv[0], parv[2]);
		return 0;
	}
	else if (!strcasecmp(param[0], "\1VERSION\1"))
	{
		EnviaAServidor(":%s %s %s :\1VERSION %s .%s.\1", parv[1], TOK_NOTICE, parv[0], COLOSSUS_VERSION, conf_set->red);
		return 0;
	}
	else if (!strcasecmp(param[0], "CREDITOS"))
	{
		Creditos();
		return 0;
	}
	if ((mod = BuscaModulo(parv[1], modulos)))
	{
		Mod_Func func;
		char ex;
		Alias *aux;
		if ((aux = BuscaAlias(param, params, mod)))
		{
			int i, j, k = 0;
			char *aparam[256];
			for (i = 0; aux->sintaxis[i]; i++)
			{
				if (*aux->sintaxis[i] == '%')
				{
					char tmp[4], *c;
					int top;
					bzero(tmp, sizeof(tmp));
					if ((c = strchr(aux->sintaxis[i], '-')))
						strncpy(tmp, aux->sintaxis[i]+1, c-aux->sintaxis[i]-1);
					else
						strcpy(tmp, aux->sintaxis[i]+1);
					j = atoi(tmp);
					top = (c ? params : j+1);
					while (j < top)
						aparam[k++] = param[j++];
				}
				else
					aparam[k++] = aux->sintaxis[i];
			}
			for (i = 0; i < k; i++)
				param[i] = aparam[i];
			params = k;
		}
		if ((func = TieneNivel(cl, param[0], mod, &ex)))
			func(cl, parv, parc, param, params);
		else
		{
			if (!ex && !EsServidor(cl))
				Responde(cl, bl, ERR_DESC);
			else if (cl->nivel & N1)
				Responde(cl, bl, ERR_FORB);
			else
				Responde(cl, bl, ERR_NOID);
		}
	}
	return 0;
}
IRCFUNC(m_whois)
{
	Modulo *mod;
	mod = BuscaModulo(parv[1], modulos);
	if (mod)
	{
		EnviaAServidor(":%s 311 %s %s %s %s * :%s", me.nombre, parv[0], mod->nick, mod->ident, mod->host, mod->realname);
		EnviaAServidor(":%s 312 %s %s %s :%s", me.nombre, parv[0], mod->nick, me.nombre, me.info);
		EnviaAServidor(":%s 307 %s %s :Tiene el nick Registrado y Protegido", me.nombre, parv[0], mod->nick);
		EnviaAServidor(":%s 310 %s %s :Es un OPERador de los servicios de red", me.nombre, parv[0], mod->nick);
		EnviaAServidor(":%s 316 %s %s :es un roBOT oficial de la Red %s", me.nombre, parv[0], mod->nick, conf_set->red);
		EnviaAServidor(":%s 313 %s %s :es un IRCop", me.nombre, parv[0], mod->nick);
		EnviaAServidor(":%s 318 %s %s :Fin de /WHOIS", me.nombre, parv[0], mod->nick);
	}
	return 0;
}
IRCFUNC(m_nick)
{
	if (parc > 3)
	{
		Cliente *cl = NULL;
		if (parc == 8) /* NICKv1 */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, parv[6], parv[6], NULL, parv[7]);
		else if (parc == 9) /* nickv1 también (¿?) */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, parv[6], parv[6], NULL, parv[8]);
		else if (parc == 11) /* NICKv2 */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, parv[6], parv[9], parv[8], parv[10]);
		else if (parc == 12) /* NICKIP */
			cl = NuevoCliente(parv[1], parv[4], parv[5], DecodeIP(parv[10]), parv[6], parv[9], parv[8], parv[11]);
		else /* protocolo desconocido !! */
		{
			Loguea(LOG_ERROR, "Protocolo para nicks desconocido (%s)", backupbuf);
			Alerta(FERR, "Se ha detectado un error en el protocolo de nicks.\nEl programa se ha cerrado para evitar daños.");
			CierraColossus(-1);
		}
		if (modusers)
			p_svsmode(cl, &me, modusers);
		if (autousers)
			EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, parv[1], autousers);
		if (parc >= 11)
		{
			if (autoopers && (strchr(parv[8], 'h') || strchr(parv[8], 'o')))
				EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, parv[1], autoopers);
		}
		if (BuscaModulo(parv[1], modulos))
		{
			p_kill(cl, &me, "Nick protegido.");
			ReconectaBot(parv[1]);
		}
		Senyal2(SIGN_POST_NICK, cl, 0);
		if (parc > 10)
			Senyal2(SIGN_UMODE, cl, parv[8]);
	}
	else
	{
		Senyal2(SIGN_PRE_NICK, cl, parv[1]);
		if (strcasecmp(parv[1], cl->nombre))
		{
			ProcesaModosCliente(cl, "-r");
#ifdef UDB
			ProcesaModosCliente(cl, "-S");
			{
				Udb *bloq, *reg;
				if ((reg = BuscaRegistro(BDD_NICKS, cl->nombre)) && (bloq = BuscaBloque(N_MOD_TOK, reg)))
				{
					char tmp[32];
					ircsprintf(tmp, "-%s", bloq->data_char);
					ProcesaModosCliente(cl, tmp);
				}
			}
#endif
			CambiaNick(cl, parv[1]);
#ifdef UDB
			UdbDaleCosas(cl);
#endif
		}
		else
			CambiaNick(cl, parv[1]);
		Senyal2(SIGN_POST_NICK, cl, 1);
	}
	return 0;
}
IRCFUNC(m_topic)
{
	Canal *cn = NULL;
	if (parc == 6)
	{
		cn = InfoCanal(parv[1], !0);
		ircstrdup(cn->topic, parv[4]);
		cn->ntopic = cl;
	}
	else if (parc == 5)
	{
		cn = InfoCanal(parv[1], !0);
		ircstrdup(cn->topic, parv[3]);
		cn->ntopic = cl;
	}
	Senyal3(SIGN_TOPIC, cl, cn, parc == 6 ? parv[4] : parv[3]);
	return 0;
}
IRCFUNC(m_quit)
{
	LinkCanal *lk;
	Senyal2(SIGN_QUIT, cl, parv[1]);
	for (lk = cl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->chan, cl);
	LiberaMemoriaCliente(cl);
	return 0;
}
IRCFUNC(m_kill)
{
	Cliente *al;
	if (EsServidor(cl))
		return 0;
	if ((al = BuscaCliente(parv[1], NULL)))
	{
		LinkCanal *lk;
		Senyal2(SIGN_QUIT, al, parv[1]);
		for (lk = al->canal; lk; lk = lk->sig)
			BorraClienteDeCanal(lk->chan, al);
		LiberaMemoriaCliente(al);
	}
	if (conf_set->opts & REKILL)
	{
		if (BuscaModulo(parv[1], modulos))
			ReconectaBot(parv[1]);
	}
	return 0;
}
IRCFUNC(m_ping)
{
	EnviaAServidor("%s :%s", TOK_PONG, parv[1]);
	return 0;
}
IRCFUNC(m_pass)
{
	if (strcmp(parv[1], conf_server->password.remoto))
	{
		Alerta(FERR, "Contraseñas incorrectas");
		SockClose(sck, LOCAL);
	}
	return 0;
}
IRCFUNC(m_join)
{
	char *canal;
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
		EntraCliente(cl, canal);
	return 0;
}
IRCFUNC(m_part)
{
	Canal *cn;
	cn = BuscaCanal(parv[1], NULL);
	Senyal3(SIGN_PART, cl, cn, parv[2]);
	BorraCanalDeCliente(cl, cn);
	BorraClienteDeCanal(cn, cl);
	return 0;
}
IRCFUNC(m_mode)
{
	Canal *cn;
	char modebuf[BUFSIZE], parabuf[BUFSIZE];
	modebuf[0] = parabuf[0] = '\0';
	cn = InfoCanal(parv[1], !0);
	ProcesaModo(cl, cn, parv + 2, parc - 2);
	if (EsServidor(cl) && (conf_set->opts & NOSERVDEOP))
	{
		int i = 3, j = 1, f = ADD;
		char *modos = parv[2];
		Canal *cn;
		if (!(cn = BuscaCanal(parv[1], NULL)))
			return 1;
		while (!BadPtr(modos))
		{
			switch (*modos)
			{
				case '+':
					f = ADD;
					break;
				case '-':
					f = DEL;
					break;
				case 'q':
				case 'a':
				case 'o':
				case 'h':
				case 'v':
					if (f == DEL)
					{
						modebuf[j++] = *modos;
						strcat(parabuf, parv[i]);
						strcat(parabuf, " ");
					}
				case 'b':
				case 'e':
				case 'k':
				case 'L':
				case 'l':
					i++;
					break;
			}
			modos++;
		}
		modebuf[j] = '\0';
	}
	if (modcanales)
		strcat(modebuf, modcanales);
	if (modebuf[0] != '\0')
		p_mode(&me, cn, "%s %s", modebuf, parabuf);
	Senyal4(SIGN_MODE, cl, cn, parv + 2, EsServidor(cl) ? parc - 3 : parc - 2);
	return 0;
}
IRCFUNC(m_sjoin)
{
	Cliente *al;
	Canal *cn;
	char *q, *p, tmp[BUFSIZE], mod[6];
	cn = InfoCanal(parv[2], !0);
	strcpy(tmp, parv[parc-1]);
	for (p = tmp; (q = strchr(p, ' ')); p = q)
	{
		q = strchr(p, ' ');
		if (q)
			*q++ = '\0';
		mod[0] = '\0';
		while (*p)
		{
#ifdef UDB
			if (*p == '.')
#else
			if (*p == '*')
#endif
				strcat(mod, "q");
#ifdef UDB
			else if (*p == '$')
#else
			else if (*p == '~')
#endif
				strcat(mod, "a");
			else if (*p == '@')
				strcat(mod, "o");
			else if (*p == '%')
				strcat(mod, "h");
			else if (*p == '+')
				strcat(mod, "v");
			else if (*p == '&')
			{
				p++;
				strcat(mod, "b");
				break;
			}
			else if (*p == '"')
			{
				p++;
				strcat(mod, "e");
				break;
			}
			else
				break;
			p++;
		}
		if (mod[0] != 'b' && mod[0] != 'e') /* es un usuario */
		{
			al = BuscaCliente(p, NULL);
			EntraCliente(al, parv[2]);
		}
		if (mod[0])
		{
			char *c, *arr[7];
			int i = 0;
			arr[i++] = mod;
			for (c = &mod[0]; *c; c++)
				arr[i++] = p;
			arr[i] = NULL;
			ProcesaModo(cl, cn, arr, i);
			Senyal4(SIGN_MODE, al, cn, arr, i);
		}
	}
	if (parc > 4) /* hay modos */
	{
		ProcesaModo(cl, cn, parv + 3, parc - 4);
		Senyal4(SIGN_MODE, cl, cn, parv + 3, parc - 4);
	}
	return 0;
}
IRCFUNC(m_kick)
{
	Canal *cn;
	Cliente *al;
	if (!(al = BuscaCliente(parv[2], NULL)))
		return 1;
	cn = InfoCanal(parv[1], 0);
	Senyal4(SIGN_KICK, cl, al, cn, parv[3]);
	BorraCanalDeCliente(al, cn);
	BorraClienteDeCanal(cn, al);
	if (EsBot(al))
	{
		if (conf_set->opts & REJOIN)
			EntraBot(al, parv[1]);
	}
	return 0;
}

IRCFUNC(m_version)
{
	EnviaAServidor(":%s 351 %s :%s .%s. iniciado el %s", me.nombre, parv[0], COLOSSUS_VERSION, conf_set->red, Fecha(&inicio));
	return 0;
}
IRCFUNC(m_stats)
{
	switch(*parv[1])
	{
		case 'u':
		{
			time_t dura;
			dura = time(0) - inicio;
			EnviaAServidor(":%s 242 %s :Servidor online %i días, %02i:%02i:%02i", me.nombre, parv[0], dura / 86400, (dura / 3600) % 24, (dura / 60) % 60, dura % 60);
			break;
		}
	}
	EnviaAServidor(":%s 219 %s %s :Fin de /STATS", me.nombre, parv[0], parv[1]);
	return 0;
}
IRCFUNC(m_server)
{
	Cliente *al;
	LinkCliente *aux;
	char *protocol, *opts, *numeric, *inf;
	protocol = opts = numeric = inf = NULL;
	strcpy(tokbuf, parv[parc - 1]);
	protocol = strtok(tokbuf, "-");
	if (protocol) /* esto nunca dara null */
		opts = strtok(NULL, "-");
	if (opts) /* hay parámetros */
		numeric = strtok(NULL, " ");
	else
	{
		protocol = NULL;
		opts = "(null)";
		inf = parv[parc - 1];
	}
	if (numeric)
		inf = strtok(NULL, "");
	else
		numeric = parv[parc - 2];
	if (!cl && protocol && atoi(protocol + 1) < 2302)
	{
		Alerta(FERR, "Version UnrealIRCd incorrecta. Solo funciona con v3.2.x y v3.1.x");
		SockClose(sck, LOCAL);
		return 1;
	}
	al = NuevoCliente(parv[1], NULL, NULL, NULL, NULL, NULL, NULL, inf);
	al->protocol = protocol ? atoi(protocol + 1) : 0;
	al->numeric = numeric ? atoi(numeric) : 0;
	al->tipo = TSERVIDOR;
	al->trio = strdup(opts);
	BMalloc(aux, LinkCliente);
	aux->user = al;
	AddItem(aux, servidores);
	if (!cl) /* primer link */
	{
#ifdef USA_ZLIB
		if (conf_server->compresion)
		{
			if (ZLibInit(sck, conf_server->compresion) < 0)
			{
				Alerta(FERR, "No puede hacer inicia_zlib");
				ZLibLibera(sck);
				CierraIrcd(NULL, NULL);
				return 0;
			}
			sck->opts |= OPT_ZLIB;
			sck->zlib->primero = 1;
		}
#endif
		linkado = al;
		sincroniza(sck, al, parv, parc);
	}
	else
		Loguea(LOG_SERVER, "Servidor %s (%s)", al->nombre, al->info);
	return 0;
}
IRCFUNC(m_squit)
{
	Cliente *al;
	LinkCliente *aux, *prev = NULL;
	al = BuscaCliente(parv[1], NULL);
	LiberaMemoriaCliente(al);
	for (aux = servidores; aux; aux = aux->sig)
	{
		if (aux->user == al)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				servidores = aux->sig;
			ircfree(aux);
			break;
		}
		prev = aux;
	}
	return 0;
}
IRCFUNC(m_umode)
{
	ProcesaModosCliente(cl, parv[1]);
	Senyal2(SIGN_UMODE, cl, parv[1]);
	return 0;
}
IRCFUNC(m_error)
{
	Loguea(LOG_ERROR, "ERROR: %s", parv[1]);
	return 0;
}
IRCFUNC(m_away)
{
	if (parc == 2)
		cl->away = 1;
	else if (parc == 1)
		cl->away = 0;
	Senyal2(SIGN_AWAY, cl, parv[1]);
	return 0;
}
IRCFUNC(m_105)
{
	int i;
	for (i = 2; i < parc; i++)
	{
		if (!strncmp("NICKLEN", parv[i], 7))
		{
			conf_set->nicklen = atoi(strchr(parv[i], '=') + 1);
			break;
		}
		else if (!strncmp("NETWORK", parv[i], 7))
		{
			char *p = strchr(parv[i], '=') + 1;
			ircstrdup(conf_set->red, p);
			break;
		}
	}
	return 0;
}
IRCFUNC(m_rehash)
{
	if (!strcasecmp(parv[1], me.nombre))
		Refresca();
	return 0;
}
IRCFUNC(m_chghost)
{
	ircstrdup(cl->vhost, parv[2]);
	GeneraMascara(cl);
	return 0;
}
IRCFUNC(m_chgident)
{
	ircstrdup(cl->ident, parv[2]);
	GeneraMascara(cl);
	return 0;
}
IRCFUNC(m_chgname)
{
	ircstrdup(cl->info, parv[2]);
	return 0;
}
#ifdef UDB
IRCFUNC(m_db)
{
	static int bloqs = 0;
	if (parc < 5)
	{
		EnviaAServidor(":%s DB %s ERR 0 %i", me.nombre, cl->nombre, E_UDB_PARAMS);
		return 1;
	}
	if (match(parv[1], me.nombre))
		return 0;
	if (!strcasecmp(parv[2], "INF"))
	{
		Udb *bloq;
		time_t gm;
		bloq = IdAUdb(*parv[3]);
		gm = atoul(parv[5]);
		if (strcmp(parv[4], bloq->data_char))
		{
			if (gm > gmts[bloq->id])
			{
				TruncaBloque(cl, bloq, 0);
				EnviaAServidor(":%s DB %s RES %c 0", me.nombre, parv[0], *parv[3]);
				ActualizaGMT(bloq, gm);
				if (++bloqs == BDD_TOTAL)
					EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
			}
			else if (gm == gmts[bloq->id])
				EnviaAServidor(":%s DB %s RES %c %lu", me.nombre, parv[0], *parv[3], bloq->data_long);
			/* si es menor, el otro nodo vaciará su db y nos mandará un RES, será cuando empecemos el resumen. abremos terminado nuestro burst */
		}
		else
		{
			if (++bloqs == BDD_TOTAL)
				EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
		}
	}
	else if (!strcasecmp(parv[2], "RES"))
	{
		u_long bytes;
		Udb *bloq;
		bloq = IdAUdb(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes < bloq->data_long) /* tiene menos, se los mandamos */
		{
			FILE *fp;
			if ((fp = fopen(bloq->item, "rb")))
			{
				char linea[512], *d;
				fseek(fp, bytes, SEEK_SET);
				while (!feof(fp))
				{
					bzero(linea, 512);
					if (!fgets(linea, 512, fp))
						break;
					if ((d = strchr(linea, '\r')))
						*d = '\0';
					if ((d = strchr(linea, '\n')))
						*d = '\0';
					if (strchr(linea, ' '))
						EnviaAServidor(":%s DB * INS %lu %c::%s", me.nombre, bytes, *parv[3], linea);
					else
						EnviaAServidor(":%s DB * DEL %lu %c::%s", me.nombre, bytes, *parv[3], linea);
					bytes = ftell(fp);
				}
				fclose(fp);
				EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
			}
		}
		else if (!bytes && !bloq->data_long)
			EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
		if (++bloqs == BDD_TOTAL)
				EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
	}
	else if (!strcasecmp(parv[2], "INS"))
	{
		char buf[1024], tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr(bloques, tipo))
		{
			EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = IdAUdb(tipo);
		if (bytes != bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR INS %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		ircsprintf(buf, "%s %s", r, Unifica(parv, parc, 5, -1));
		ParseaLinea(bloq->id, buf, 1);
	}
	else if (!strcasecmp(parv[2], "DEL"))
	{
		char tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr(bloques, tipo))
		{
			EnviaAServidor(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = IdAUdb(tipo);
		if (bytes != bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR DEL %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		ParseaLinea(bloq->id, r, 1);
	}
	else if (!strcasecmp(parv[2], "DRP"))
	{
		Udb *bloq;
		u_long bytes;
		if (!strchr(bloques, *parv[3]))
		{
			EnviaAServidor(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		bloq = IdAUdb(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes > bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_LEN, *parv[3]);
			return 1;
		}
		TruncaBloque(cl, bloq, bytes);
	}
	else if (!strcmp(parv[2], "ERR")) /* tratamiento de errores */
	{
		int error = 0;
		error = atoi(parv[4]);
		switch (error)
		{
			case E_UDB_LEN:
			{
				Udb *bloq;
				u_long bytes;
				char *cb;
				bloq = IdAUdb(*parv[5]);
				cb = strchr(parv[5], ' ') + 1; /* esto siempre debe cumplirse, si no a cascar */
				bytes = atoul(cb);
				TruncaBloque(cl, bloq, bytes);
				break;
			}
		}
		/* una vez hemos terminado, retornamos puesto que estos comandos sólo van dirigidos a un nodo */
		return 1;
	}
	/* DB * OPT <bdd> <NULL>*/
	else if (!strcmp(parv[2], "OPT"))
	{
		Udb *bloq;
		if (!strchr(bloques, *parv[3]))
		{
			EnviaAServidor(":%s DB %s ERR OPT %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		bloq = IdAUdb(*parv[3]);
		Optimiza(bloq);
	}
	return 0;
}
IRCFUNC(m_dbq)
{
	char *cur, *pos, *ds;
	Udb *bloq;
	pos = cur = strdup(parv[2]);
	if (!(bloq = IdAUdb(*pos)))
	{
		EnviaAServidor(":%s 339 %s :La base de datos %c no existe.", me.nombre, cl->nombre, *pos);
		return 0;
	}
	if (parc == 3)
		parv[1] = parv[2];
	if (*(++cur) != '\0')
	{
		if (*cur++ != ':' || *cur++ != ':' || *cur == '\0')
		{
			EnviaAServidor(":%s 339 %s :Formato de bloque incorrecto.", me.nombre, cl->nombre);
			return 1;
		}
		
		while ((ds = strchr(cur, ':')))
		{
			if (*(ds + 1) == ':')
			{
				*ds++ = '\0';
				if (!(bloq = BuscaBloque(cur, bloq)))
					goto nobloq;
			}
			else
				break;
			cur = ++ds;
		}
		if (!(bloq = BuscaBloque(cur, bloq)))
		{
			nobloq:
			EnviaAServidor(":%s 339 %s :No se encuentra el bloque %s.", me.nombre, cl->nombre, cur);
		}
		else
		{
			if (bloq->data_long)
				EnviaAServidor(":%s 339 %s :DBQ %s %lu", me.nombre, cl->nombre, parv[1], bloq->data_long);
			else if (bloq->data_char)
				EnviaAServidor(":%s 339 %s :DBQ %s %s", me.nombre, cl->nombre, parv[1], bloq->data_char);
			else
			{
				Udb *aux;
				for (aux = bloq->down; aux; aux = aux->mid)
				{
					if (aux->data_long)
						EnviaAServidor(":%s 339 %s :DBQ %s::%c %lu", me.nombre, cl->nombre, parv[1], aux->id, aux->data_long);
					else if (aux->data_char)
						EnviaAServidor(":%s 339 %s :DBQ %s::%c %s", me.nombre, cl->nombre, parv[1], aux->id, aux->data_char);
					else
						EnviaAServidor(":%s 339 %s :DBQ %s::%c (no tiene datos)", me.nombre, cl->nombre, parv[1], aux->id);
				}
			}
		}
	}
	else
	{
		int id = bloq->id;
		EnviaAServidor(":%s 339 %s :%i %i %lu %s %lu %X", me.nombre, cl->nombre, id, regs[id], bloq->data_long, bloq->data_char, LeeGMT(id), LeeHash(id));
	} 
	Free(pos);
	return 0;
}
#endif
IRCFUNC(m_eos)
{
	if (cl == linkado)
	{
		EnviaAServidor(":%s %s", me.nombre, TOK_VERSION);
		EnviaAServidor(":%s %s :Sincronización realizada en %.3f segs", me.nombre, TOK_WALLOPS, abs(microtime() - tburst));
#ifdef UDB
		UdbCompruebaOpts(NULL);
#endif
		intentos = 0;
		Senyal(SIGN_EOS);
#ifdef _WIN32		
		ChkBtCon(1, 0);
#endif		
	}
	return 0;
}
IRCFUNC(m_netinfo)
{
	//if (strcasecmp(parv[7], conf_set->red))
	//	Alerta(FADV, "El nombre de la red es distinto");
	return 0;
}
IRCFUNC(m_sajoin)
{
	Cliente *bl;
	if ((bl = BuscaCliente(parv[1], NULL)))
		EntraBot(bl, parv[2]);
	return 0;
}
IRCFUNC(m_sapart)
{
	Cliente *al;
	if ((al = BuscaCliente(parv[1], NULL)))
		SacaBot(al, parv[2], parv[3]);
	return 0;
}
IRCFUNC(m_module)
{
	Modulo *ex;
	char send[512], minbuf[512];
	for (ex = modulos; ex; ex = ex->sig)
	{
		ircsprintf(send, "Módulo %s.", ex->info->nombre);
		if (ex->info->version)
		{
			ircsprintf(minbuf, " Versión %.2f.", ex->info->version);
			strcat(send, minbuf);
		}
		if (ex->info->autor)
		{
			ircsprintf(minbuf, " Autor: %s.", ex->info->autor); 
			strcat(send, minbuf);
		}
		if (ex->info->email)
		{
			ircsprintf(minbuf, " Email: %s.", ex->info->email);
			strcat(send, minbuf);
		}
		EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_NOTICE, cl->nombre, send);
	}
	return 0;
}
IRCFUNC(sincroniza)
{
#ifdef UDB
	Udb *aux;
#endif
	tburst = microtime();
#ifdef UDB
	for (aux = ultimo; aux; aux = aux->mid)
		EnviaAServidor(":%s DB %s INF %c %s %lu", me.nombre, cl->nombre, bloques[aux->id], aux->data_char, gmts[aux->id]);
#endif	
	Senyal(SIGN_SYNCH);
	return 0;
}
int TipoTKL(char tipo)
{
	switch(tipo)
	{
		case 'G':
			return TKL_GLINE;
		case 'Z':
			return TKL_ZLINE;
		case 's':
			return TKL_SHUN;
		case 'F':
			return TKL_SPAMF;
		case 'Q':
			return TKL_QLINE;
	}
	Info("Se ha insertado una TKL desconocida. Diríjase a http://www.rallados.net e informe al desarrollador");
	return -1; /* nunca debería pasar */
}
IRCFUNC(m_tkl)
{
	if (parc < 2)
		return 1;
	if (*parv[1] == '+')
	{
		Tkl *tkl;
		if (parc < 9)
			return 1;
		if (*parv[2] == 'F')
		{
			if (parc > 9)
				tkl = InsertaTKL(*parv[2], parv[10], NULL, parv[5], parv[9], atoul(parv[7]), atoul(parv[6]));
			else
				tkl = InsertaTKL(*parv[2], parv[8], NULL, parv[5], NULL, atoul(parv[7]), atoul(parv[6]));
		}
		else if (*parv[2] == 'Q')
			tkl = InsertaTKL(*parv[2], parv[4], NULL, parv[5], parv[8], atoul(parv[7]), atoul(parv[6]));
		else
			tkl = InsertaTKL(*parv[2], parv[3], parv[4], parv[5], parv[8], atoul(parv[7]), atoul(parv[6]));
		AddItem(tkl, tklines[TipoTKL(*parv[2])]);
	}
	else if (*parv[1] == '-')
	{
		if (parc < 6)
			return 1;
		if (*parv[2] == 'F')
			BorraTKL(&tklines[TipoTKL(*parv[2])], parv[8], NULL);
		else if (*parv[2] == 'Q')
			BorraTKL(&tklines[TipoTKL(*parv[2])], parv[4], NULL);
		else
			BorraTKL(&tklines[TipoTKL(*parv[2])], parv[3], parv[4]);
	}
	return 0;
}
void ProcesaModo(Cliente *cl, Canal *cn, char *parv[], int parc)
{
	int modo = ADD, param = 1;
	char *mods = parv[0];
	while (*mods)
	{
		switch(*mods)
		{
			case '+':
				modo = ADD;
				break;
			case '-':
				modo = DEL;
				break;
			/*case 'b':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaBan(cl, &cn->ban, parv[param]);
				else
					BorraBanDeCanal(&cn->ban, parv[param]);
				param++;
				break;
			case 'e':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaBan(cl, &cn->exc, parv[param]);
				else
					BorraBanDeCanal(&cn->exc, parv[param]);
				param++;
				break;*/
			case 'k':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(cn->clave, parv[param]);
					cn->modos |= FlagAModo('k', PROT_CMODOS(Unreal));
				}
				else
				{
					ircfree(cn->clave);
					cn->modos &= ~FlagAModo('k', PROT_CMODOS(Unreal));
				}
				param++;
				break;
			case 'f':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(cn->flood, parv[param]);
					cn->modos |= FlagAModo('f', PROT_CMODOS(Unreal));
				}
				else
				{
					ircfree(cn->flood);
					cn->modos &= ~FlagAModo('f', PROT_CMODOS(Unreal));
				}
				param++;
				break;
			case 'l':
				if (modo == ADD)
				{
					if (!parv[param])
						break;
					cn->limite = atoi(parv[param]);
					cn->modos |= FlagAModo('l', PROT_CMODOS(Unreal));
					param++;
				}
				else
				{
					cn->limite = 0;
					cn->modos &= ~FlagAModo('l', PROT_CMODOS(Unreal));
				}
				break;
			case 'L':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(cn->link, parv[param]);
					cn->modos |= FlagAModo('L', PROT_CMODOS(Unreal));
				}
				else
				{
					ircfree(cn->link);
					cn->modos &= ~FlagAModo('L', PROT_CMODOS(Unreal));
				}
				param++;
				break;
			/*case 'q':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->owner, BuscaCliente(parv[param], NULL));
				else
					BorraModoCliente(&cn->owner, BuscaCliente(parv[param], NULL));
				param++;
				break;
			case 'a':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->admin, BuscaCliente(parv[param], NULL));
				else
					BorraModoCliente(&cn->admin, BuscaCliente(parv[param], NULL));
				param++;
				break;
			case 'o':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->op, BuscaCliente(parv[param], NULL));
				else
					BorraModoCliente(&cn->op, BuscaCliente(parv[param], NULL));
				param++;
				break;
			case 'h':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->half, BuscaCliente(parv[param], NULL));
				else
					BorraModoCliente(&cn->half, BuscaCliente(parv[param], NULL));
				param++;
				break;
			case 'v':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->voz, BuscaCliente(parv[param], NULL));
				else
					BorraModoCliente(&cn->voz, BuscaCliente(parv[param], NULL));
				param++;
				break;*/
			default:
			{
				MallaCliente *mcl;
				MallaMascara *mmk;
				if (modo == ADD)
				{
					if ((mcl = BuscaMallaCliente(cn, *mods)))
						InsertaModoCliente(&mcl->malla, BuscaCliente(parv[param], NULL));
					else if ((mmk = BuscaMallaMascara(cn, *mods)))
						InsertaMascara(cl, &mmk->malla, parv[param]);
					else
						cn->modos |= FlagAModo(*mods, PROT_CMODOS(Unreal));
#ifdef DEBUG
					for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
					{
						LinkCliente *lk;
						Debug("!%c", mcl->flag);
						for (lk = mcl->malla; lk; lk = lk->sig)
							Debug("---%s", lk->user->nombre);
						Debug("*");
					}
#endif
				}
				else
				{
					if ((mcl = BuscaMallaCliente(cn, *mods)))
						BorraModoCliente(&mcl->malla, BuscaCliente(parv[param], NULL));
					else if ((mmk = BuscaMallaMascara(cn, *mods)))
						BorraMascaraDeCanal(&mmk->malla, parv[param]);
					else
						cn->modos &= ~FlagAModo(*mods, PROT_CMODOS(Unreal));
				}
			}
		}
		mods++;
	}
}
void ProcesaModos(Canal *cn, char *modos)
{
	char *arv[256];
	int arc = 0;
	bzero(arv, 256);
	arv[arc++] = modos;
	do
	{
		if (*modos == 0x20)
		{
			do
			{
				*modos++ = 0x0;
			} while(*modos == 0x20);
			arv[arc++] = modos;
		}
		modos++;
	} while(*modos != 0x0);
	ProcesaModo(&me, cn, arv, arc);
}
void EntraCliente(Cliente *cl, char *canal)
{
	Canal *cn = NULL;
	cn = InfoCanal(canal, !0);
	if (!cn->miembros)
	{
		if (conf_set->debug && !strcmp(canal, conf_set->debug))
		{
			if (!IsAdmin(cl))
			{
				EnviaAServidor(":%s SVSPART %s %s", me.nombre, cl->nombre, canal);
				return;
			}
			p_mode(&me, cn, "+sAm");
		}
		else
			p_mode(&me, cn, modcanales);
	}
	InsertaCanalEnCliente(cl, cn);
	InsertaClienteEnCanal(cn, cl);
	Senyal2(SIGN_JOIN, cl, cn);
}

/* ':' and '#' and '&' and '+' and '@' must never be in this table. */
/* these tables must NEVER CHANGE! >) */
char int6_to_base64_map[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
	    'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	    'U', 'V',
	'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	    'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	    '{', '}'
};

char base64_to_int6_map[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
	-1, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, -1, 63, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static char *int_to_base64(long val)
{
	/* 32/6 == max 6 bytes for representation, 
	 * +1 for the null, +1 for byte boundaries 
	 */
	static char base64buf[8];
	long i = 7;

	base64buf[i] = '\0';

	/* Temporary debugging code.. remove before 2038 ;p.
	 * This might happen in case of 64bit longs (opteron/ia64),
	 * if the value is then too large it can easily lead to
	 * a buffer underflow and thus to a crash. -- Syzop
	 */
	if (val > 2147483647L)
	{
		abort();
	}

	do
	{
		base64buf[--i] = int6_to_base64_map[val & 63];
	}
	while (val >>= 6);

	return base64buf + i;
}

static long base64_to_int(char *b64)
{
	int v = base64_to_int6_map[(u_char)*b64++];

	if (!b64)
		return 0;
		
	while (*b64)
	{
		v <<= 6;
		v += base64_to_int6_map[(u_char)*b64++];
	}

	return v;
}
char *base64enc(long i)
{
	if (i < 0)
		return ("0");
	return int_to_base64(i);
}
long base64dec(char *b64)
{
	if (b64)
		return base64_to_int(b64);
	else
		return 0;
}
#ifdef UDB
void UdbDaleCosas(Cliente *cl)
{
	Udb *reg, *bloq;
	if (!(reg = BuscaRegistro(BDD_NICKS, cl->nombre)))
		return;
	if (!BuscaBloque(N_SUS_TOK, reg))
	{
		ProcesaModosCliente(cl, "+r");
		if ((bloq = BuscaBloque(N_MOD_TOK, reg)))
			ProcesaModosCliente(cl, bloq->data_char);
		if ((bloq = BuscaBloque(N_OPE_TOK, reg)))
		{
			u_long nivel = bloq->data_long;
			if (nivel & BuscaOpt("OPER", NivelesBDD))
				ProcesaModosCliente(cl, "+h");
			if (nivel & BuscaOpt("ADMIN", NivelesBDD) || nivel & BuscaOpt("ROOT", NivelesBDD))
				ProcesaModosCliente(cl, "+oN");
		}
	}
	else
		ProcesaModosCliente(cl, "+S");
}
int UdbCompruebaOpts(Proc *proc)
{
	time_t hora = time(0);
	u_int i;
	Udb *aux;
	if (!SockIrcd)
		return 1;
	if (!proc || proc->time + 1800 < hora)
	{
		for (i = 0; i < BDD_TOTAL; i++)
		{
			aux = IdAUdb(i);
			if (gmts[i] && gmts[i] + 86400 < hora)
			{
				EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, bloques[i], hora);
				Optimiza(aux);
				ActualizaGMT(aux, hora);
			}
		}
		if (proc)
		{
			proc->proc = 0;
			proc->time = hora;
		}
	}
	return 0;
}
#endif
