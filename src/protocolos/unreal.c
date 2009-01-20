/*
 * $Id: unreal.c,v 1.59 2008/05/03 12:01:55 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#include <arpa/inet.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef USA_ZLIB
#include "zip.h"
#endif

/* stuff de tklines. todas son globales */
#define TKL_GLINE 0
#define TKL_ZLINE 1
#define TKL_SHUN 2
#define TKL_SPAMF 3
#define TKL_QLINE 4

#define PROTOCOL 2309

double tburst;
char *autousers = NULL;
char *autoopers = NULL;
#define NOSERVDEOP 0x40
long base64dec(char *);
char *base64enc(long);
int norekill = 0;

IRCFUNC(m_chghost);
IRCFUNC(m_chgident);
IRCFUNC(m_chgname);
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
IRCFUNC(m_sethost);
IRCFUNC(m_432);

ProtInfo PROT_INFO(Unreal) = {
	"Protocolo UnrealIRCd" ,
	0.5 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

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

u_long UMODE_INVISIBLE;
u_long UMODE_OPER;
u_long UMODE_WALLOP;
u_long UMODE_FAILOP;
u_long UMODE_HELPOP;
u_long UMODE_SADMIN;
u_long UMODE_ADMIN;
u_long UMODE_RGSTRONLY;
u_long UMODE_WEBTV;
u_long UMODE_SERVICES;
u_long UMODE_NETADMIN;
u_long UMODE_COADMIN;
u_long UMODE_WHOIS;
u_long UMODE_KIX;
u_long UMODE_BOT;
u_long UMODE_SECURE;
u_long UMODE_VICTIM;
u_long UMODE_DEAF;
u_long UMODE_HIDEOPER;
u_long UMODE_SETHOST;
u_long UMODE_STRIPBADWORDS;
u_long UMODE_HIDEWHOIS;
u_long UMODE_NOCTCP;
u_long UMODE_LOCOP;

u_long CHMODE_PRIVATE;
u_long CHMODE_SECRET;
u_long CHMODE_MODERATED;
u_long CHMODE_TOPICLIMIT;
u_long CHMODE_INVITEONLY;
u_long CHMODE_NOPRIVMSGS;
u_long CHMODE_KEY;
u_long CHMODE_LIMIT;
u_long CHMODE_RGSTRONLY;
u_long CHMODE_LINK;
u_long CHMODE_NOCOLOR;
u_long CHMODE_OPERONLY;
u_long CHMODE_ADMONLY;
u_long CHMODE_NOKICKS;
u_long CHMODE_STRIP;
u_long CHMODE_NOKNOCK;
u_long CHMODE_NOINVITE;
u_long CHMODE_FLOODLIMIT;
u_long CHMODE_MODREG;
u_long CHMODE_STRIPBADWORDS;
u_long CHMODE_NOCTCP;
u_long CHMODE_AUDITORIUM;
u_long CHMODE_ONLYSECURE;
u_long CHMODE_NONICKCHANGE;

void ProcesaModos(Canal *, char *);
DLLFUNC void ProcesaModo(Cliente *, Canal *, char **, int);
DLLFUNC void EntraCliente(Cliente *, char *);
void ActualizaCanalPrivado(Canal *);
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
Cliente *BuscaNumerico(int num)
{
	LinkCliente *lk;
	for (lk = servidores; lk; lk = lk->sig)
	{
		if (lk->cl->numeric == num)
			return lk->cl;
	}
	return NULL;
}
int p_msg_vl(Cliente *, Cliente *, u_int, char *, va_list *);
char *p_trio(Cliente *cl)
{
	if (!cl)
		return NULL;
	return cl->nombre;
}
int p_umode(Cliente *cl, char *modos)
{
	if (!cl)
		return 1;
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
	ProcesaModosCliente(cl, buf);
	EnviaAServidor(":%s %s %s %s %lu", bl->nombre, TOK_SVS2MODE, cl->nombre, buf, time(0));
	LlamaSenyal(SIGN_UMODE, 2, cl, buf);
	return 0;
}
int p_mode(Cliente *cl, Canal *cn, char *modos, ...)
{
	char buf[BUFSIZE], *copy;
	va_list vl;
	if (!cl || !cn)
		return 1;
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
	if (!al)
		return 1;
	EnviaAServidor(":%s %s %s %lu", al->nombre, TOK_NICK, nuevo, time(0));
	return 0;
}
int p_svsnick(Cliente *al, char *nuevo)
{
	if (!al)
		return 1;
	EnviaAServidor(":%s %s %s %s %lu", me.nombre, TOK_SVSNICK, al->nombre, nuevo, time(0));
	return 0;
}
int p_join(Cliente *bl, Canal *cn)
{
	if (!bl || !cn)
		return 1;
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_JOIN, cn->nombre);
	return 0;
}
int p_svsjoin(Cliente *cl, char *canal)
{
	if (!cl)
		return 1;
	EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, cl->nombre, canal);
	return 0;
}
int p_part(Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (!cn || !bl)
		return 1;
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
	if (!cn || !cl)
		return 1;
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
	if (!SockIrcd)
		return 1;
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
		BorraClienteDeCanal(lk->cn, bl);
	LiberaMemoriaCliente(bl);
	return 0;
}
int p_kill(Cliente *cl, Cliente *bl, char *motivo, ...)
{
	LinkCanal *lk;
	char buf[BUFSIZE];
	if (!cl || !bl)
		return 1;
	if (motivo)
	{
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
	}
	else
		strcpy(buf, "Usuario desconectado.");
	EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_KILL, cl->nombre, buf);
	LlamaSenyal(SIGN_QUIT, 2, cl, buf);
	for (lk = cl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->cn, cl);
	LiberaMemoriaCliente(cl);
	return 0;
}
int p_nuevonick(Cliente *al)
{
	if (!al)
		return 1;
	gethostname(buf, sizeof(buf));
	EnviaAServidor("%s %s 1 %lu %s %s %s 0 +%s %s %s :%s", TOK_NICK, al->nombre, time(0), al->ident, al->host, al->server->nombre, ModosAFlags(al->modos, protocolo->umodos, NULL), al->host, EncodeIP(buf), al->info);
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
	if (!bl)
		return 1;
	ircstrdup(bl->host, host);
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_SETHOST, host);
	return 0;
}
int p_chghost(Cliente *cl, char *host)
{
	if (!cl)
		return 1;
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
	if (!cl || !bl)
		return 1;
	EnviaAServidor(":%s %s %s", bl->nombre, TOK_LAG, cl->nombre);
	return 0;
}
int p_swhois(Cliente *cl, char *swhois)
{
	if (!cl)
		return 1;
	EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_SWHOIS, cl->nombre, swhois);
	return 0;
}
int p_tkl(Cliente *bl, char modo, char *ident, char *host, int tiempo, char *motivo)
{
	Tkl *tkl = NULL;
	char *emisor = (bl ? bl->mask : me.nombre);
	ircsprintf(buf, "%s@%s", ident, host);
	if ((tkl = BuscaTKL(buf, tklines[TKL_GLINE])) && (!tkl->fin || (motivo && !strcmp(motivo, tkl->motivo))))
		return 1;
	if (modo == ADD)
	{
		time_t ini, fin;
		ini = time(0);
		fin = (!tiempo ? 0 : time(0) + tiempo);
		EnviaAServidor(":%s BD + G %s %s %s %lu %lu :%s", me.nombre, ident, host, emisor, fin, ini, motivo ? motivo : "");
		InsertaTKL('G', ident, host, emisor, motivo, ini, fin);
	}
	else if (modo == DEL)
	{
		EnviaAServidor(":%s BD - G %s %s %s", me.nombre, ident, host, emisor);
		BorraTKL(&tklines[TKL_GLINE], ident, host);
	}
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
	if (!cn)
		return 1;
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
	if (!cl || !cn || !bl)
		return 1;
	EnviaAServidor(":%s %s %s %s", bl->nombre, TOK_INVITE, cl->nombre, cn->nombre);
	return 0;
}
int p_msg_vl(Cliente *cl, Cliente *bl, u_int tipo, char *formato, va_list *vl)
{
	char buf[BUFSIZE];
	if (!vl)
		strlcpy(buf, formato, sizeof(buf));
	else
		ircvsprintf(buf, formato, *vl);
	if (tipo == 1)
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_PRIVATE, cl->nombre, buf);
	else
		EnviaAServidor(":%s %s %s :%s", bl->nombre, TOK_NOTICE, cl->nombre, buf);
	return 0;
}
int p_ping()
{
	EnviaAServidor("%s :%s", TOK_PING, me.nombre);
	return 0;
}
int p_server(char *nombre, char *info)
{
	EnviaAServidor(":%s %s %s 2 :%s", me.nombre, TOK_SERVER, nombre, info);
	return 0;
}
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
		conf_set = BMalloc(struct Conf_set);
	autousers = autoopers = NULL;
	protocolo->modusers = protocolo->modcanales = NULL;
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
					ircstrdup(autoopers, config->seccion[i]->seccion[p]->data);
			}
		}
		else if (!strcmp(config->seccion[i]->item, "modos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "usuarios"))
					ircstrdup(protocolo->modusers, config->seccion[i]->seccion[p]->data);
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "canales"))
					ircstrdup(protocolo->modcanales, config->seccion[i]->seccion[p]->data);
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
		else if (!strcmp(config->seccion[i]->item, "extension"))
		{
			if (!CreaExtension(config->seccion[i], protocolo))
				Error("[%s:%s::%i] No se ha podido crear la extensión %s", config->seccion[i]->archivo, config->seccion[i]->item, config->seccion[i]->linea, config->seccion[i]->data);
		}
	}
	ircstrdup(protocolo->modcl, modcl);
	ircstrdup(protocolo->modmk, modmk);
	ircstrdup(protocolo->modpm1, modpm1);
	ircstrdup(protocolo->modpm2, modpm2);
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
	protocolo->comandos[P_TRIO] = (int(*)())p_trio;
	protocolo->comandos[P_MODO_USUARIO_LOCAL] = (int(*)())p_umode;
	protocolo->comandos[P_MODO_USUARIO_REMOTO] = (int(*)())p_svsmode;
	protocolo->comandos[P_MODO_CANAL] = (int(*)())p_mode;
	protocolo->comandos[P_CAMBIO_USUARIO_LOCAL] = (int(*)())p_nick;
	protocolo->comandos[P_CAMBIO_USUARIO_REMOTO] = (int(*)())p_svsnick;
	protocolo->comandos[P_JOIN_USUARIO_LOCAL] = (int(*)())p_join;
	protocolo->comandos[P_JOIN_USUARIO_REMOTO] = (int(*)())p_svsjoin;
	protocolo->comandos[P_PART_USUARIO_LOCAL] = (int(*)())p_part;
	protocolo->comandos[P_PART_USUARIO_REMOTO] = (int(*)())p_svspart;
	protocolo->comandos[P_QUIT_USUARIO_LOCAL] = (int(*)())p_quit;
	protocolo->comandos[P_QUIT_USUARIO_REMOTO] = (int(*)())p_kill;
	protocolo->comandos[P_NUEVO_USUARIO] = (int(*)())p_nuevonick;
	protocolo->comandos[P_PRIVADO] = (int(*)())p_priv;
	protocolo->comandos[P_CAMBIO_HOST_LOCAL] = (int(*)())p_sethost;
	protocolo->comandos[P_CAMBIO_HOST_REMOTO] = (int(*)())p_chghost;
	protocolo->comandos[P_FORB_NICK] = (int(*)())p_sqline;
	protocolo->comandos[P_LAG] = (int(*)())p_lag;
	protocolo->comandos[P_WHOIS_ESPECIAL] = (int(*)())p_swhois;
	protocolo->comandos[P_GLINE] = (int(*)())p_tkl;
	protocolo->comandos[P_KICK] = (int(*)())p_kick;
	protocolo->comandos[P_TOPIC] = (int(*)())p_topic;
	protocolo->comandos[P_NOTICE] = (int(*)())p_notice;
	protocolo->comandos[P_INVITE] = (int(*)())p_invite;
	protocolo->comandos[P_MSG_VL] = (int(*)())p_msg_vl;
	protocolo->comandos[P_PING] = (int(*)())p_ping;
	protocolo->comandos[P_SERVER] = (int(*)())p_server;
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
	InsertaComando(MSG_EOS, TOK_EOS, m_eos, INI, MAXPARA);
	InsertaComando(MSG_NETINFO, TOK_NETINFO, m_netinfo, INI, MAXPARA);
	InsertaComando(MSG_SAJOIN, TOK_SAJOIN, m_sajoin, INI, MAXPARA);
	InsertaComando(MSG_SAPART, TOK_SAPART, m_sapart, INI, MAXPARA);
	InsertaComando(MSG_MODULE, TOK_MODULE, m_module, INI, MAXPARA);
	InsertaComando(MSG_SJOIN, TOK_SJOIN, m_sjoin, INI, MAXPARA);
	InsertaComando(MSG_TKL, TOK_TKL, m_tkl, INI, MAXPARA);
	InsertaComando(MSG_SETHOST, TOK_SETHOST, m_sethost, INI, MAXPARA);
	InsertaComando("432", "432", m_432, INI, MAXPARA);
	InsertaModoProtocolo('r', &UMODE_REGNICK, protocolo->umodos);
	InsertaModoProtocolo('N', &UMODE_NETADMIN, protocolo->umodos);
	InsertaModoProtocolo('o', &UMODE_OPER, protocolo->umodos);
	InsertaModoProtocolo('h', &UMODE_HELPOP, protocolo->umodos);
	InsertaModoProtocolo('x', &UMODE_HIDE, protocolo->umodos);
	InsertaModoProtocolo('i', &UMODE_INVISIBLE, protocolo->umodos);
	InsertaModoProtocolo('w', &UMODE_WALLOP, protocolo->umodos);
	InsertaModoProtocolo('g', &UMODE_FAILOP, protocolo->umodos);
	InsertaModoProtocolo('a', &UMODE_SADMIN, protocolo->umodos);
	InsertaModoProtocolo('A', &UMODE_ADMIN, protocolo->umodos);
	InsertaModoProtocolo('R', &UMODE_RGSTRONLY, protocolo->umodos);
	InsertaModoProtocolo('V', &UMODE_WEBTV, protocolo->umodos);
	InsertaModoProtocolo('S', &UMODE_SERVICES, protocolo->umodos);
	InsertaModoProtocolo('C', &UMODE_COADMIN, protocolo->umodos);
	InsertaModoProtocolo('w', &UMODE_WHOIS, protocolo->umodos);
	InsertaModoProtocolo('q', &UMODE_KIX, protocolo->umodos);
	InsertaModoProtocolo('B', &UMODE_BOT, protocolo->umodos);
	InsertaModoProtocolo('z', &UMODE_SECURE, protocolo->umodos);
	InsertaModoProtocolo('v', &UMODE_VICTIM, protocolo->umodos);
	InsertaModoProtocolo('d', &UMODE_DEAF, protocolo->umodos);
	InsertaModoProtocolo('H', &UMODE_HIDEOPER, protocolo->umodos);
	InsertaModoProtocolo('t', &UMODE_SETHOST, protocolo->umodos);
	InsertaModoProtocolo('G', &UMODE_STRIPBADWORDS, protocolo->umodos);
	InsertaModoProtocolo('p', &UMODE_HIDEWHOIS, protocolo->umodos);
	InsertaModoProtocolo('T', &UMODE_NOCTCP, protocolo->umodos);
	InsertaModoProtocolo('O', &UMODE_LOCOP, protocolo->umodos);
	InsertaModoProtocolo('r', &CHMODE_RGSTR, protocolo->cmodos);
	InsertaModoProtocolo('R', &CHMODE_RGSTRONLY, protocolo->cmodos);
	InsertaModoProtocolo('O', &CHMODE_OPERONLY, protocolo->cmodos);
	InsertaModoProtocolo('A', &CHMODE_ADMONLY, protocolo->cmodos);
	InsertaModoProtocolo('p', &CHMODE_PRIVATE, protocolo->cmodos);
	InsertaModoProtocolo('s', &CHMODE_SECRET, protocolo->cmodos);
	InsertaModoProtocolo('m', &CHMODE_MODERATED, protocolo->cmodos);
	InsertaModoProtocolo('t', &CHMODE_TOPICLIMIT, protocolo->cmodos);
	InsertaModoProtocolo('i', &CHMODE_INVITEONLY, protocolo->cmodos);
	InsertaModoProtocolo('n', &CHMODE_NOPRIVMSGS, protocolo->cmodos);
	InsertaModoProtocolo('k', &CHMODE_KEY, protocolo->cmodos);
	InsertaModoProtocolo('l', &CHMODE_LIMIT, protocolo->cmodos);
	InsertaModoProtocolo('L', &CHMODE_LINK, protocolo->cmodos);
	InsertaModoProtocolo('c', &CHMODE_NOCOLOR, protocolo->cmodos);
	InsertaModoProtocolo('Q', &CHMODE_NOKICKS, protocolo->cmodos);
	InsertaModoProtocolo('S', &CHMODE_STRIP, protocolo->cmodos);
	InsertaModoProtocolo('K', &CHMODE_NOKNOCK, protocolo->cmodos);
	InsertaModoProtocolo('V', &CHMODE_NOINVITE, protocolo->cmodos);
	InsertaModoProtocolo('f', &CHMODE_FLOODLIMIT, protocolo->cmodos);
	InsertaModoProtocolo('M', &CHMODE_MODREG, protocolo->cmodos);
	InsertaModoProtocolo('G', &CHMODE_STRIPBADWORDS, protocolo->cmodos);
	InsertaModoProtocolo('C', &CHMODE_NOCTCP, protocolo->cmodos);
	InsertaModoProtocolo('u', &CHMODE_AUDITORIUM, protocolo->cmodos);
	InsertaModoProtocolo('z', &CHMODE_ONLYSECURE, protocolo->cmodos);
	InsertaModoProtocolo('N', &CHMODE_NONICKCHANGE, protocolo->cmodos);
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
	BorraComando(MSG_EOS, m_eos);
	BorraComando(MSG_NETINFO, m_netinfo);
	BorraComando(MSG_SAJOIN, m_sajoin);
	BorraComando(MSG_SAPART, m_sapart);
	BorraComando(MSG_MODULE, m_module);
	BorraComando(MSG_SJOIN, m_sjoin);
	BorraComando(MSG_TKL, m_tkl);
	BorraComando(MSG_SETHOST, m_sethost);
	BorraComando("432", m_432);
	BorraComando("401", m_432);
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
	LlamaSenyal(SIGN_SOCKOPEN, 0);
	EnviaAServidor("PROTOCTL NICKv2 VHP VL TOKEN UMODE2 NICKIP SJOIN SJ3 NS SJB64 TKLEXT");
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
	int params, i, j, raw;
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
				numeric = base64dec(sender);
				if ((cl = BuscaNumerico(numeric)))
					para[0] = cl->nombre;
			}
			else
			{
				para[0] = sender;
				if (!(cl = BuscaCliente(para[0])) && !strchr(para[0], '.'))
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
	params = (comd ? comd->params : MAXPARA);
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
//	for (j = 0; j < i; j++)
//		Debug("parv[%i] = %s", j, para[j]);
	if (comd)
	{
		if (comd->cuando == INI)
		{
			for (j = 0; j < comd->funciones; j++)
			{
				if (!cl)
					cl = BuscaCliente(para[0]);
				if (comd->funcion[j])
					comd->funcion[j](sck, cl, para, i);
			}
		}
		else if (comd->cuando == FIN)
		{
			for (j = comd->funciones - 1; j >= 0; j--)
			{
				if (!cl)
					cl = BuscaCliente(para[0]);
				if (comd->funcion[j])
					comd->funcion[j](sck, cl, para, i);
			}
		}
	}
	if ((raw = atoi(p))) /* es numerico */
		LlamaSenyal(SIGN_RAW, 4, cl, para, i, raw);
	return 0;
}
IRCFUNC(m_msg)
{
	char *param[256], par[BUFSIZE];
	int params, i, resp = 0;
	Modulo *mod;
	Cliente *bl;
	strtok(parv[1], "@");
	parv[parc] = strtok(NULL, "@");
	parv[parc+1] = NULL;
	//while (*parv[2] == ':')
	//	parv[2]++;
	if (*parv[1] == '#')
	{
		LlamaSenyal(SIGN_CMSG, 3, cl, BuscaCanal(parv[1]), parv[2]);
		return 1;
	}
	if (!(bl = BuscaCliente(parv[1])))
		return 1; /* algo pasa! */
	strlcpy(par, parv[2], sizeof(par));
	for (i = 0, param[i] = strtok(par, " "); param[i]; param[++i] = strtok(NULL, " "));
	params = i;
	if (!param[0]) /* algo pasa! */
		return 1;
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
		int i;
		Responde(cl, bl, COLOSSUS_VERSION " (rv%i)- Trocotronic @2004-2009", rev);
		Responde(cl, bl, " ");
		for (i = 0; creditos[i]; i++)
			Responde(cl, bl, creditos[i]);
		return 0;
	}
	if ((mod = BuscaModulo(parv[1], modulos)))
	{
		Funcion *func;
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
						strlcpy(tmp, aux->sintaxis[i]+1, sizeof(tmp));
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
			resp = func->func(cl, parv, parc, param, params, func);
		else if (EsCliente(cl))
		{
			if (!ex)
				Responde(cl, bl, ERR_DESC);
			else if (cl->nivel & N1)
				Responde(cl, bl, ERR_FORB);
			else
				Responde(cl, bl, ERR_NOID);
			resp = 1;
		}
	}
	if (resp >= 0)
		LlamaSenyal(SIGN_PMSG, 4, cl, bl, parv[2], resp);
	return 0;
}
IRCFUNC(m_whois)
{
	Modulo *mod;
	if ((mod = BuscaModulo(parv[1], modulos)))
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
		Cliente *cl = NULL, *al = NULL;
		char *serv = parv[6];
		if (IsDigit(*parv[6]) && (al = BuscaNumerico(atoi(parv[6]))))
			serv = al->nombre;
		if (parc == 8) /* NICKv1 */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, serv, parv[6], NULL, parv[7]);
		else if (parc == 9) /* nickv1 también (¿?) */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, serv, parv[6], NULL, parv[8]);
		else if (parc == 11) /* NICKv2 */
			cl = NuevoCliente(parv[1], parv[4], parv[5], NULL, serv, parv[9], parv[8], parv[10]);
		else if (parc == 12) /* NICKIP */
			cl = NuevoCliente(parv[1], parv[4], parv[5], DecodeIP(parv[10]), serv, parv[9], parv[8], parv[11]);
		else /* protocolo desconocido !! */
		{
			Loguea(LOG_ERROR, "Protocolo para nicks desconocido (%s)", backupbuf);
			Alerta(FERR, "Se ha detectado un error en el protocolo de nicks.\nEl programa se ha cerrado para evitar daños.");
			CierraColossus(-1);
		}
		if (protocolo->modusers)
			ProtFunc(P_MODO_USUARIO_REMOTO)(cl, &me, protocolo->modusers);
		if (autousers)
			EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, parv[1], autousers);
		if (parc >= 11)
		{
			if (autoopers && (strchr(parv[8], 'h') || strchr(parv[8], 'o')))
				EnviaAServidor(":%s %s %s %s", me.nombre, TOK_SVSJOIN, parv[1], autoopers);
		}
		if (BuscaModulo(parv[1], modulos))
		{
			ProtFunc(P_QUIT_USUARIO_REMOTO)(cl, &me, "Nick protegido.");
			ReconectaBot(parv[1]);
		}
		LlamaSenyal(SIGN_POST_NICK, 2, cl, 0);
		if (parc > 10)
			LlamaSenyal(SIGN_UMODE, 2, cl, parv[8]);
	}
	else
	{
		LlamaSenyal(SIGN_PRE_NICK, 2, cl, parv[1]);
		if (strcasecmp(parv[1], cl->nombre))
		{
			ProcesaModosCliente(cl, "-r");
			CambiaNick(cl, parv[1]);
		}
		else
			CambiaNick(cl, parv[1]);
		LlamaSenyal(SIGN_POST_NICK, 2, cl, 1);
	}
	return 0;
}
IRCFUNC(m_topic)
{
	Canal *cn = CreaCanal(parv[1]);
	ircstrdup(cn->topic, parv[parc-1]);
	cn->ntopic = cl;
	LlamaSenyal(SIGN_TOPIC, 3, cl, cn, parv[parc-1]);
	return 0;
}
IRCFUNC(m_quit)
{
	LinkCanal *lk;
	LlamaSenyal(SIGN_QUIT, 2, cl, parv[1]);
	for (lk = cl->canal; lk; lk = lk->sig)
	{
		BorraClienteDeCanal(lk->cn, cl);
		ActualizaCanalPrivado(lk->cn);
	}
	LiberaMemoriaCliente(cl);
	return 0;
}
IRCFUNC(m_kill)
{
	Cliente *al;
	if ((al = BuscaCliente(parv[1])))
	{
		LinkCanal *lk;
		LlamaSenyal(SIGN_QUIT, 2, al, parv[1]);
		for (lk = al->canal; lk; lk = lk->sig)
		{
			BorraClienteDeCanal(lk->cn, al);
			ActualizaCanalPrivado(lk->cn);
		}
		LiberaMemoriaCliente(al);
		if (conf_set->opts & REKILL && !norekill)
		{
			if (BuscaModulo(parv[1], modulos))
				ReconectaBot(parv[1]);
		}
	}
	return 0;
}
IRCFUNC(m_ping)
{
	EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_PONG, me.nombre, parv[1]);
	return 0;
}
IRCFUNC(m_pass)
{
	if (strcmp(parv[1], conf_server->password.remoto))
	{
		Alerta(FERR, "Contraseñas incorrectas");
		SockClose(sck);
	}
	return 0;
}
IRCFUNC(m_join)
{
	char *canal;
	strlcpy(tokbuf, parv[1], sizeof(tokbuf));
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
		EntraCliente(cl, canal);
	return 0;
}
IRCFUNC(m_part)
{
	Canal *cn;
	cn = BuscaCanal(parv[1]);
	LlamaSenyal(SIGN_PART, 3, cl, cn, parv[2]);
	BorraCanalDeCliente(cl, cn);
	BorraClienteDeCanal(cn, cl);
	ActualizaCanalPrivado(cn);
	return 0;
}
IRCFUNC(m_mode)
{
	Canal *cn;
	char modebuf[BUFSIZE], parabuf[BUFSIZE];
	modebuf[0] = parabuf[0] = '\0';
	cn = CreaCanal(parv[1]);
	ProcesaModo(cl, cn, parv + 2, parc - 2);
	if (protocolo->modcanales)
		strlcat(modebuf, protocolo->modcanales, sizeof(modebuf));
	if (modebuf[0] != '\0')
		ProtFunc(P_MODO_CANAL)(&me, cn, "%s %s", modebuf, parabuf);
	ActualizaCanalPrivado(cn);
	LlamaSenyal(SIGN_MODE, 4, cl, cn, parv + 2, EsServidor(cl) ? parc - 3 : parc - 2);
	return 0;
}
IRCFUNC(m_sjoin)
{
	Cliente *al = NULL;
	Canal *cn = NULL;
	char *q, *p, tmp[BUFSIZE], mod[8];
	time_t creacion;
	cn = CreaCanal(parv[2]);
	creacion = base64dec(parv[1]);
	strlcpy(tmp, parv[parc-1], sizeof(tmp));
	for (p = tmp; (q = strchr(p, ' ')); p = q)
	{
		al = NULL;
		q = strchr(p, ' ');
		if (q)
			*q++ = '\0';
		mod[0] = '\0';
		while (*p)
		{
			if (*p == '*')
				strlcat(mod, "q", sizeof(mod));
			else if (*p == '~')
				strlcat(mod, "a", sizeof(mod));
			else if (*p == '@')
				strlcat(mod, "o", sizeof(mod));
			else if (*p == '%')
				strlcat(mod, "h", sizeof(mod));
			else if (*p == '+')
				strlcat(mod, "v", sizeof(mod));
			else if (*p == '&')
			{
				p++;
				strlcat(mod, "b", sizeof(mod));
				break;
			}
			else if (*p == '"')
			{
				p++;
				strlcat(mod, "e", sizeof(mod));
				break;
			}
			else
				break;
			p++;
		}
		if (mod[0] != 'b' && mod[0] != 'e') /* es un usuario */
		{
			if ((al = BuscaCliente(p)))
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
			if (al)
				LlamaSenyal(SIGN_MODE, 4, al, cn, arr, i);
		}
	}
	if (creacion < cn->creacion) /* debemos quitar todo lo nuestro */
	{
		MallaParam *mpm;
		MallaMascara *mmk;
		Mascara *mk, *tmk;
		cn->modos = 0L;
		for (mpm = cn->mallapm; mpm; mpm = mpm->sig)
			ircfree(mpm->param);
		for (mmk = cn->mallamk; mmk; mmk = mmk->sig)
		{
			for (mk = mmk->malla; mk; mk = tmk)
			{
				tmk = mk->sig;
				Free(mk->mascara);
				Free(mk);
			}
			mmk->malla = NULL;
		}
		if (cl != linkado)
		{
			MallaCliente *mcl;
			LinkCliente *lk, *tlk;
			if (conf_set->opts & NOSERVDEOP)
			{
				int i = 0;
				char *modos = (char *)Malloc(sizeof(char) * (protocolo->modos + 1));
				tmp[0] = '\0';
				for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
				{
					for (lk = mcl->malla; lk; lk = lk->sig)
					{
						if (i == protocolo->modos)
						{
							modos[i] = '\0';
							ProtFunc(P_MODO_CANAL)(&me, cn, "+%s %s", modos, tmp);
							i = 0;
							tmp[0] = '\0';
						}
						modos[i++] = mcl->flag;
						strlcat(tmp, lk->cl->nombre, sizeof(tmp));
						strlcat(tmp, " ", sizeof(tmp));
					}
				}
				if (i > 0)
				{
					modos[i] = '\0';
					ProtFunc(P_MODO_CANAL)(&me, cn, "+%s %s", modos, tmp);
				}
				Free(modos);
			}
			else
			{
				for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
				{
					for (lk = mcl->malla; lk; lk = tlk)
					{
						tlk = lk->sig;
						Free(lk);
					}
					mcl->malla = NULL;
				}
			}
		}
		cn->creacion = creacion;
	}
	if (parc > 4 && creacion <= cn->creacion) /* hay modos */
	{
		ProcesaModo(cl, cn, parv + 3, parc - 4);
		LlamaSenyal(SIGN_MODE, 4, cl, cn, parv + 3, parc - 4);
	}
	return 0;
}
IRCFUNC(m_kick)
{
	Canal *cn;
	Cliente *al;
	if (!(al = BuscaCliente(parv[2])))
		return 1;
	if (!(cn = BuscaCanal(parv[1])))
		return 1;
	LlamaSenyal(SIGN_KICK, 4, cl, al, cn, parv[3]);
	BorraCanalDeCliente(al, cn);
	BorraClienteDeCanal(cn, al);
	ActualizaCanalPrivado(cn);
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
	EnviaAServidor(":%s 351 %s :Creado el %s", me.nombre, parv[0], creado);
	EnviaAServidor(":%s 351 %s :BID %s, #%i", me.nombre, parv[0], bid, compilacion);
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
	int prim = 0;
	protocol = opts = numeric = inf = NULL;
	strlcpy(tokbuf, parv[parc - 1], sizeof(tokbuf));
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
		SockClose(sck);
		return 1;
	}
	al = NuevoCliente(parv[1], NULL, NULL, NULL, NULL, NULL, NULL, inf);
	al->protocol = protocol ? atoi(protocol + 1) : 0;
	al->numeric = numeric ? atoi(numeric) : 0;
	al->tipo = TSERVIDOR;
	al->trio = strdup(opts);
	aux = BMalloc(LinkCliente);
	aux->cl = al;
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
				CierraIrcd(NULL, NULL, 0);
				return 0;
			}
			SetZlib(sck);
			sck->zlib->primero = 1;
		}
#endif
		linkado = al;
		sincroniza(sck, al, parv, parc);
		prim = 1;
	}
	else
		Loguea(LOG_SERVER, "Servidor %s (%s)", al->nombre, al->info);
	LlamaSenyal(SIGN_SERVER, 2, al, prim);
	return 0;
}
IRCFUNC(m_squit)
{
	Cliente *al;
	LinkCliente *aux, *prev = NULL;
	if ((al = BuscaCliente(parv[1])))
	{
		LlamaSenyal(SIGN_SQUIT, 1, al);
		LiberaMemoriaCliente(al);
		for (aux = servidores; aux; aux = aux->sig)
		{
			if (aux->cl == al)
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
	}
	return 0;
}
IRCFUNC(m_umode)
{
	ProcesaModosCliente(cl, parv[1]);
	LlamaSenyal(SIGN_UMODE, 2, cl, parv[1]);
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
		ircstrdup(cl->away, parv[1]);
	else if (parc == 1)
		ircfree(cl->away);
	LlamaSenyal(SIGN_AWAY, 2, cl, parv[1]);
	return 0;
}
IRCFUNC(m_105)
{
	int i;
	for (i = 2; i < parc; i++)
	{
		if (!strncmp("NICKLEN", parv[i], 7))
		{
			protocolo->nicklen = atoi(strchr(parv[i], '=') + 1);
			continue;
		}
		else if (!strncmp("NETWORK", parv[i], 7))
		{
			char *p = strchr(parv[i], '=') + 1;
			ircstrdup(conf_set->red, p);
			continue;
		}
		else if (!strncmp("MODES", parv[i], 5))
		{
			protocolo->modos = atoi(strchr(parv[i], '=') + 1);
			continue;
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
IRCFUNC(m_eos)
{
	if (cl == linkado)
	{
		EnviaAServidor(":%s %s", me.nombre, TOK_VERSION);
		EnviaAServidor(":%s %s :Sincronización realizada en %.3f segs", me.nombre, TOK_WALLOPS, (double)((clock() - tburst)/CLOCKS_PER_SEC));
		LlamaSenyal(SIGN_EOS, 0);
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
	if ((bl = BuscaCliente(parv[1])))
		EntraBot(bl, parv[2]);
	return 0;
}
IRCFUNC(m_sapart)
{
	Cliente *al;
	if ((al = BuscaCliente(parv[1])))
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
			strlcat(send, minbuf, sizeof(send));
		}
		if (ex->info->autor)
		{
			ircsprintf(minbuf, " Autor: %s.", ex->info->autor);
			strlcat(send, minbuf, sizeof(send));
		}
		if (ex->info->email)
		{
			ircsprintf(minbuf, " Email: %s.", ex->info->email);
			strlcat(send, minbuf, sizeof(send));
		}
		EnviaAServidor(":%s %s %s :%s", me.nombre, TOK_NOTICE, cl->nombre, send);
	}
	return 0;
}
IRCFUNC(sincroniza)
{
	tburst = clock();
	LlamaSenyal(SIGN_SYNCH, 1, cl);
	return 0;
}
IRCFUNC(m_432)
{
	Cliente *bl;
	if ((bl = BuscaCliente(parv[2])))
		DesconectaBot(bl, parv[3]);
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
	Info("Se ha insertado una TKL desconocida. Diríjase a http://www.redyc.com e informe al desarrollador");
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
IRCFUNC(m_sethost)
{
	ircstrdup(cl->vhost, parv[1]);
	GeneraMascara(cl);
	return 1;
}
void ProcesaModo(Cliente *cl, Canal *cn, char *parv[], int parc)
{
	int modo = ADD, param = 1;
	char *mods = parv[0];
	while (!BadPtr(mods))
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
				break;
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
			case 'q':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->owner, BuscaCliente(parv[param]));
				else
					BorraModoCliente(&cn->owner, BuscaCliente(parv[param]));
				param++;
				break;
			case 'a':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->admin, BuscaCliente(parv[param]));
				else
					BorraModoCliente(&cn->admin, BuscaCliente(parv[param]));
				param++;
				break;
			case 'o':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->op, BuscaCliente(parv[param]));
				else
					BorraModoCliente(&cn->op, BuscaCliente(parv[param]));
				param++;
				break;
			case 'h':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->half, BuscaCliente(parv[param]));
				else
					BorraModoCliente(&cn->half, BuscaCliente(parv[param]));
				param++;
				break;
			case 'v':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->voz, BuscaCliente(parv[param]));
				else
					BorraModoCliente(&cn->voz, BuscaCliente(parv[param]));
				param++;
				break;*/
			default:
			{
				MallaCliente *mcl;
				MallaMascara *mmk;
				MallaParam *mpm;
				if (modo == ADD)
				{
					if ((mcl = BuscaMallaCliente(cn, *mods)))
						InsertaModoCliente(&mcl->malla, BuscaCliente(parv[param]));
					else if ((mmk = BuscaMallaMascara(cn, *mods)))
						InsertaMascara(cl, &mmk->malla, parv[param]);
					else if ((mpm = BuscaMallaParam(cn, *mods)))
						ircstrdup(mpm->param, parv[param]);
					else
						cn->modos |= FlagAModo(*mods, protocolo->cmodos);
#ifdef DEBUG
					for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
					{
						LinkCliente *lk;
						Debug("!%c", mcl->flag);
						for (lk = mcl->malla; lk; lk = lk->sig)
							Debug("---%s", lk->cl->nombre);
						Debug("*");
					}
#endif
#ifdef DEBUG
					for (mpm = cn->mallapm; mpm; mpm = mpm->sig)
						Debug("!%c %s", mpm->flag, mpm->param);
#endif
				}
				else
				{
					if ((mcl = BuscaMallaCliente(cn, *mods)))
						BorraModoCliente(&mcl->malla, BuscaCliente(parv[param]));
					else if ((mmk = BuscaMallaMascara(cn, *mods)))
						BorraMascaraDeCanal(&mmk->malla, parv[param]);
					else if ((mpm = BuscaMallaParam(cn, *mods)))
						ircfree(mpm->param);
					else
						cn->modos &= ~FlagAModo(*mods, protocolo->cmodos);
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
	if (conf_set->debug && !strcmp(canal, conf_set->debug) && !IsAdmin(cl))
	{
		EnviaAServidor(":%s SVSPART %s %s", me.nombre, cl->nombre, canal);
		return;
	}
	cn = CreaCanal(canal);
	if (!cn->miembros)
	{
		if (conf_set->debug && !strcmp(canal, conf_set->debug))
			ProtFunc(P_MODO_CANAL)(&me, cn, "+sAm");
		else if (protocolo->modcanales)
			ProtFunc(P_MODO_CANAL)(&me, cn, protocolo->modcanales);
	}
	InsertaCanalEnCliente(cl, cn);
	InsertaClienteEnCanal(cn, cl);
	ActualizaCanalPrivado(cn);
	LlamaSenyal(SIGN_JOIN, 2, cl, cn);
}
void ActualizaCanalPrivado(Canal *cn)
{
	if (!(cn->modos & CHMODE_INVITEONLY || cn->modos & CHMODE_KEY ||
		cn->modos & CHMODE_OPERONLY || cn->modos & CHMODE_ADMONLY ||
		cn->modos & CHMODE_RGSTRONLY || cn->modos & CHMODE_ONLYSECURE))
	{
		if (cn->modos & CHMODE_LIMIT)
		{
			MallaParam *mpm;
			if ((mpm = BuscaMallaParam(cn, 'l')) && atoi(mpm->param) <= cn->miembros)
				cn->privado = 0;
			else
				cn->privado = 1;
		}
		else
			cn->privado = 0;
	}
	else
		cn->privado = 1;
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
