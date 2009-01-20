/*
 * $Id: p10.c,v 1.43 2008/02/16 23:19:43 Trocotronic Exp $
 */

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"

double tburst;

static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
static int i64[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 52,
	53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62, 0, 63, 0, 0, 0,
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0
};

IRCFUNC(m_eos);
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
IRCFUNC(m_error);
IRCFUNC(m_away);
IRCFUNC(m_rehash);
IRCFUNC(sincroniza);
IRCFUNC(m_burst);
IRCFUNC(m_create);

ProtInfo PROT_INFO(P10) = {
	"Protocolo P10" ,
	0.2 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};
#define MSG_PRIVATE "PRIVMSG"
#define TOK_PRIVATE "P"
#define MSG_WHOIS "WHOIS"
#define TOK_WHOIS "W"
#define MSG_NICK "NICK"
#define TOK_NICK "N"
#define MSG_TOPIC "TOPIC"
#define TOK_TOPIC "T"
#define MSG_QUIT "QUIT"
#define TOK_QUIT "Q"
#define MSG_KILL "KILL"
#define TOK_KILL "D"
#define MSG_PING "PING"
#define TOK_PING "G"
#define MSG_PASS "PASS"
#define TOK_PASS "PA"
#define MSG_NOTICE "NOTICE"
#define TOK_NOTICE "O"
#define MSG_JOIN "JOIN"
#define TOK_JOIN "J"
#define MSG_PART "PART"
#define TOK_PART "L"
#define MSG_MODE "MODE"
#define TOK_MODE "M"
#define MSG_KICK "KICK"
#define TOK_KICK "K"
#define MSG_VERSION "VERSION"
#define TOK_VERSION "V"
#define MSG_STATS "STATS"
#define TOK_STATS "R"
#define MSG_SERVER "SERVER"
#define TOK_SERVER "S"
#define MSG_SQUIT "SQUIT"
#define TOK_SQUIT "SQ"
#define MSG_ERROR "ERROR"
#define TOK_ERROR "Y"
#define MSG_AWAY "AWAY"
#define TOK_AWAY "A"
#define MSG_REHASH "REHASH"
#define TOK_REHASH "REHASH"
#define MSG_EOB "END_OF_BURST"
#define TOK_EOB "EB"
#define MSG_PONG "PONG"
#define TOK_PONG "Z"
#define MSG_ACCOUNT "ACCOUNT"
#define TOK_ACCOUNT "AC"
#define MSG_GLINE "GLINE"
#define TOK_GLINE "GL"
#define MSG_INVITE "INVITE"
#define TOK_INVITE "I"
#define MSG_WALLOPS "WALLOPS"
#define TOK_WALLOPS "WA"
#define MSG_BURST "BURST"
#define TOK_BURST "B"
#define MSG_EOB_ACK "EOB_ACK"
#define TOK_EOB_ACK "EA"
#define MSG_CREATE "CREATE"
#define TOK_CREATE "C"

u_long UMODE_OPER;
u_long UMODE_LOCOP;
u_long UMODE_INVISIBLE;
u_long UMODE_WALLOP;
u_long UMODE_SERVNOTICE;
u_long UMODE_DEAF;
u_long UMODE_SERVICES;
u_long UMODE_DEBUG;

u_long CHMODE_RGSTRONLY;
u_long CHMODE_PRIVATE;
u_long CHMODE_SECRET;
u_long CHMODE_MODERATED;
u_long CHMODE_TOPICLIMIT;
u_long CHMODE_INVITEONLY;
u_long CHMODE_NOPRIVMSGS;
u_long CHMODE_KEY;
u_long CHMODE_LIMIT;
Hash tTab[UMAX];

void EntraCliente(Cliente *, char *);
void ProcesaModos(Canal *, char *);
void ProcesaModo(Cliente *, Canal *, char **, int);
void p_kick_vl(Cliente *, Cliente *, Canal *, char *, va_list *);
int p_msg_vl(Cliente *, Cliente *, u_int, char *, va_list *);
void ActualizaCanalPrivado(Canal *);

/* Para no tocar la variable ->hsig, se crea otra estructura para guardar los trios y se indexa por el numeric */
typedef struct _trio
{
	struct _trio *hsig;
	Cliente *cl;
}Trio;
DLLFUNC void int2b64(char *destino, int num, size_t tam)
{
	char *fin = destino + tam - 1;
	memset(destino, 'A', tam);
	*fin-- = 0;
	do
	{
		*fin-- = base64[(num % 64)];
		num /= 64;
	}while (num);
}
DLLFUNC int b642int(char *num)
{
	int numero = 0, factor = 1;
	char *fin = strchr(num, '\0');
	for (fin--; fin >= num; fin--)
	{
		numero += factor*i64[(int)*fin];
		factor *= 64;
	}
	return numero;
}
DLLFUNC Cliente *BuscaClienteNumerico(char *clave, Cliente *lugar)
{
	Trio *aux;
	u_int hash;
	int num;
	num = b642int(clave);
	hash = HashCliente(clave);
	for (aux = (Trio *)tTab[hash].item; aux; aux = aux->hsig)
	{
		if (aux->cl && aux->cl->numeric == num)
			return aux->cl;
	}
	return lugar;
}
DLLFUNC void InsertaClienteEnNumerico(Cliente *us, char *clave, Hash *tabla)
{
	Trio *aux;
	u_int hash = 0;
	hash = HashCliente(clave);
	aux = BMalloc(Trio);
	aux->cl = us;
	aux->hsig = tabla[hash].item;
	tabla[hash].item = aux;
	tabla[hash].items++;
}
DLLFUNC int BorraClienteDeNumerico(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Trio *aux, *prev = NULL;
	hash = HashCliente(clave);
	for (aux = (Trio *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (aux->cl == us)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				tabla[hash].item = aux->hsig;
			tabla[hash].items--;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
char *p_trio(Cliente *cl)
{
	if (!cl)
		return NULL;
	return cl->trio;
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
	if (!cl || !bl)
		return 1;
	va_start(vl, modos);
	ircvsprintf(buf, modos, vl);
	va_end(vl);
	if (!strcmp(buf, "+r"))
	{
		char account[13];
		bzero(account, sizeof(account));
		strlcpy(account, cl->nombre, sizeof(account));
		ProcesaModosCliente(cl, buf);
		EnviaAServidor("%s %s %s %s %lu", me.trio, TOK_ACCOUNT, cl->trio, account, time(0));
	}
	else if (!strcmp(buf, "-r"))
		ProcesaModosCliente(cl, buf);
	return 0;
}
int p_mode(Cliente *cl, Canal *cn, char *modos, ...)
{
	char buf[BUFSIZE], *copy;
	va_list vl;
	if (!cn || !cl)
		return 0;
	va_start(vl, modos);
	ircvsprintf(buf, modos, vl);
	va_end(vl);
	copy = strdup(buf);
	ProcesaModos(cn, buf);
	EnviaAServidor("%s %s %s %s", cl->trio, TOK_MODE, cn->nombre, copy);
	Free(copy);
	return 0;
}
int p_nick(Cliente *cl, char *nuevo)
{
	if (!cl)
		return 1;
	EnviaAServidor("%s %s %s", cl->trio, TOK_NICK, nuevo);
	return 0;
}
int p_join(Cliente *bl, Canal *cn)
{
	if (!bl || !cn)
		return 1;
	EnviaAServidor("%s %s %s", bl->trio, TOK_JOIN, cn->nombre);
	return 0;
}
int p_part(Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (!bl || !cn)
		return 1;
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor("%s %s %s :%s", bl->trio, TOK_PART, cn->nombre, buf);
	}
	else
		EnviaAServidor("%s %s %s", bl->trio, TOK_PART, cn->nombre);
	return 0;
}
int p_svspart(Cliente *cl, Canal *cn, char *motivo, ...)
{
	if (!cl || !cn)
		return 1;
	if (motivo)
	{
		va_list vl;
		va_start(vl, motivo);
		p_kick_vl(cl, &me, cn, motivo, &vl);
		va_end(vl);
	}
	else
		p_kick_vl(cl, &me, cn, motivo, NULL);
	return 0;
}
int p_quit(Cliente *bl, char *motivo, ...)
{
	LinkCanal *lk;
	if (!bl)
		return 1;
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		ircvsprintf(buf, motivo, vl);
		va_end(vl);
		EnviaAServidor("%s %s :%s", bl->trio, TOK_QUIT, buf);
	}
	else
		EnviaAServidor("%s %s", bl->trio, TOK_QUIT);
	for (lk = bl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->cn, bl);
	BorraClienteDeNumerico(bl, bl->trio, tTab);
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
	EnviaAServidor("%s %s %s :%s %s", bl->trio, TOK_KILL, cl->trio, me.nombre, buf);
	LlamaSenyal(SIGN_QUIT, 2, cl, buf);
	for (lk = cl->canal; lk; lk = lk->sig)
		BorraClienteDeCanal(lk->cn, cl);
	LiberaMemoriaCliente(cl);
	return 0;
}
int p_nuevonick(Cliente *al)
{
	char *modos, ipb64[8];
	struct in_addr tmp;
	if (!al)
		return 1;
	tmp.s_addr = 893387572;
	inttobase64(ipb64, ntohl(tmp.s_addr), 6);
	ircfree(al->trio);
	al->trio = (char *)Malloc(sizeof(char) * 6);
	al->trio[0] = me.trio[0];
	al->trio[1] = me.trio[1];
	int2b64(al->trio + 2, al->numeric, sizeof(char) * 4);
	al->numeric = b642int(al->trio);
	InsertaClienteEnNumerico(al, al->trio, tTab);
	modos = ModosAFlags(al->modos, protocolo->umodos, NULL);
	if (strchr(modos, 'r')) /* hay account */
		EnviaAServidor("%s %s %s 1 %lu %s %s +%s %s %s %s :%s", me.trio, TOK_NICK, al->nombre, time(0), al->ident, al->host, modos, al->nombre, ipb64, al->trio, al->info);
	else
		EnviaAServidor("%s %s %s 1 %lu %s %s +%s %s %s :%s", me.trio, TOK_NICK, al->nombre, time(0), al->ident, al->host, modos, ipb64, al->trio, al->info);
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
int p_gline(Cliente *bl, char modo, char *ident, char *host, int tiempo, char *motivo)
{
	if (modo == ADD)
		EnviaAServidor("%s %s * +%s@%s %i :%s", me.trio, TOK_GLINE, ident, host, tiempo, motivo);
	else
		EnviaAServidor("%s %s * -%s@%s", me.trio, TOK_GLINE, ident, host);
	return 0;
}
void p_kick_vl(Cliente *cl, Cliente *bl, Canal *cn, char *motivo, va_list *vl)
{
	if (!cl || !bl || !cn)
		return;
	if (motivo)
	{
		char buf[BUFSIZE];
		if (!vl)
			strlcpy(buf, motivo, sizeof(buf));
		else
			ircvsprintf(buf, motivo, *vl);
		EnviaAServidor("%s %s %s %s :%s", bl->trio, TOK_KICK, cn->nombre, cl->trio, buf);
	}
	else
		EnviaAServidor("%s %s %s %s :Usuario expulsado", bl->trio, TOK_KICK, cn->nombre, cl->trio);
}
int p_kick(Cliente *cl, Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (!cl || !cn || !bl)
		return 1;
	if (EsServidor(cl) || EsBot(cl))
		return 1;
	if (motivo)
	{
		va_list vl;
		va_start(vl, motivo);
		p_kick_vl(cl, bl, cn, motivo, &vl);
		va_end(vl);
	}
	else
		p_kick_vl(cl, bl, cn, motivo, NULL);
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
		EnviaAServidor("%s %s %s :%s", bl->trio, TOK_TOPIC, cn->nombre, topic);
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
	if (!cl || !bl || !cn)
		return 1;
	EnviaAServidor("%s %s %s %s", bl->trio, TOK_INVITE, cl->trio, cn->nombre);
	return 0;
}
int p_msg_vl(Cliente *cl, Cliente *bl, u_int tipo, char *formato, va_list *vl)
{
	char buf[BUFSIZE];
	if (!bl)
		return 1;
	if (!vl)
		strlcpy(buf, formato, sizeof(buf));
	else
		ircvsprintf(buf, formato, *vl);
	if (tipo == 1)
		EnviaAServidor("%s %s %s :%s", bl->trio, TOK_PRIVATE, cl->trio, buf);
	else
		EnviaAServidor("%s %s %s :%s", bl->trio, TOK_NOTICE, cl->trio, buf);
	return 0;
}
int p_ping()
{
	return 0;
}
int test(Conf *config, int *errores)
{
	Conf *eval, *aux;
	short error_parcial = 0;
	if (!(eval = BuscaEntrada(config, "red")))
	{
		Error("[%s:%s] No se encuentra la directriz red.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "nicklen")))
	{
		Error("[%s:%s] No se encuentra la directriz nicklen.", config->archivo, config->item);
		error_parcial++;
	}
	if ((eval = BuscaEntrada(config, "modos")))
	{
		if ((aux = BuscaEntrada(eval, "canales")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz canales esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
			else
			{
				char forb[] = "ovb";
				int i;
				for (i = 0; forb[i] != '\0'; i++)
				{
					if (strchr(aux->data, forb[i]))
					{
						Error("[%s:%s::%s::%s::%i] Los modos +ovb no se permiten.", config->archivo, config->item, eval->item, aux->item, aux->linea);
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
	int i, p, modos = 6;
	char *modcl = "ov", *modmk = "b", *modpm1 = "k", *modpm2 = "l";
	if (!conf_set)
		conf_set = BMalloc(struct Conf_set);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "red"))
			ircstrdup(conf_set->red, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "nicklen"))
			protocolo->nicklen = abs(atoi(config->seccion[i]->data));
		else if (!strcmp(config->seccion[i]->item, "modos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "canales"))
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
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "max_modos"))
					modos = atoi(config->seccion[i]->seccion[p]->data);
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
	protocolo->modos = modos;
	conf_set->opts |= NO_OVERRIDE;
}
int PROT_CARGA(P10)(Conf *config)
{
	int errores = 0;
	if (!test(config, &errores))
		set(config);
	else
	{
		Error("[%s] La configuracion de %s es errónea", config->archivo, PROT_INFO(P10).nombre);
		return ++errores;
	}
	protocolo->comandos[P_TRIO] = (int(*)())p_trio;
	protocolo->comandos[P_MODO_USUARIO_LOCAL] = (int(*)())p_umode;
	protocolo->comandos[P_MODO_USUARIO_REMOTO] = (int(*)())p_svsmode;
	protocolo->comandos[P_MODO_CANAL] = (int(*)())p_mode;
	protocolo->comandos[P_CAMBIO_USUARIO_LOCAL] = (int(*)())p_nick;
	protocolo->comandos[P_JOIN_USUARIO_LOCAL] = (int(*)())p_join;
	protocolo->comandos[P_PART_USUARIO_LOCAL] = (int(*)())p_part;
	protocolo->comandos[P_PART_USUARIO_REMOTO] = (int(*)())p_svspart;
	protocolo->comandos[P_QUIT_USUARIO_LOCAL] = (int(*)())p_quit;
	protocolo->comandos[P_QUIT_USUARIO_REMOTO] = (int(*)())p_kill;
	protocolo->comandos[P_NUEVO_USUARIO] = (int(*)())p_nuevonick;
	protocolo->comandos[P_PRIVADO] = (int(*)())p_priv;
	protocolo->comandos[P_GLINE] = (int(*)())p_gline;
	protocolo->comandos[P_KICK] = (int(*)())p_kick;
	protocolo->comandos[P_TOPIC] = (int(*)())p_topic;
	protocolo->comandos[P_NOTICE] = (int(*)())p_notice;
	protocolo->comandos[P_INVITE] = (int(*)())p_invite;
	protocolo->comandos[P_MSG_VL] = (int(*)())p_msg_vl;
	protocolo->comandos[P_PING] = (int(*)())p_ping;
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
	InsertaComando(MSG_ERROR, TOK_ERROR, m_error, INI, MAXPARA);
	InsertaComando(MSG_AWAY, TOK_AWAY, m_away, INI, 1);
	InsertaComando(MSG_REHASH, TOK_REHASH, m_rehash, INI, MAXPARA);
	InsertaComando(MSG_EOB, TOK_EOB, m_eos, INI, MAXPARA);
	InsertaComando(MSG_BURST , TOK_BURST , m_burst , INI , MAXPARA);
	InsertaComando(MSG_CREATE , TOK_CREATE , m_create , INI ,MAXPARA);
	InsertaModoProtocolo('r', &UMODE_REGNICK, protocolo->umodos);
	InsertaModoProtocolo('o', &UMODE_OPER, protocolo->umodos);
	InsertaModoProtocolo('O', &UMODE_LOCOP, protocolo->umodos);
	InsertaModoProtocolo('x', &UMODE_HIDE, protocolo->umodos);
	InsertaModoProtocolo('i', &UMODE_INVISIBLE, protocolo->umodos);
	InsertaModoProtocolo('w', &UMODE_WALLOP, protocolo->umodos);
	InsertaModoProtocolo('s', &UMODE_SERVNOTICE, protocolo->umodos);
	InsertaModoProtocolo('d', &UMODE_DEAF, protocolo->umodos);
	InsertaModoProtocolo('k', &UMODE_SERVICES, protocolo->umodos);
	InsertaModoProtocolo('g', &UMODE_DEBUG, protocolo->umodos);
	InsertaModoProtocolo('r', &CHMODE_RGSTRONLY, protocolo->cmodos);
	InsertaModoProtocolo('p', &CHMODE_PRIVATE, protocolo->cmodos);
	InsertaModoProtocolo('s', &CHMODE_SECRET, protocolo->cmodos);
	InsertaModoProtocolo('m', &CHMODE_MODERATED, protocolo->cmodos);
	InsertaModoProtocolo('t', &CHMODE_TOPICLIMIT, protocolo->cmodos);
	InsertaModoProtocolo('i', &CHMODE_INVITEONLY, protocolo->cmodos);
	InsertaModoProtocolo('n', &CHMODE_NOPRIVMSGS, protocolo->cmodos);
	InsertaModoProtocolo('k', &CHMODE_KEY, protocolo->cmodos);
	InsertaModoProtocolo('l', &CHMODE_LIMIT, protocolo->cmodos);
	return 0;
}
int PROT_DESCARGA(P10)()
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
	BorraComando(MSG_ERROR, m_error);
	BorraComando(MSG_AWAY, m_away);
	BorraComando(MSG_REHASH, m_rehash);
	BorraComando(MSG_EOB, m_eos);
	BorraComando(MSG_BURST, m_burst);
	BorraComando(MSG_CREATE, m_create);
	return 0;
}
void PROT_INICIA(P10)()
{
	char buf[3];
	int2b64(buf, me.numeric, sizeof(buf));
	//EnviaAServidor(":%s PROTOCTL REQ :TOKEN", me.nombre);
	ircstrdup(me.trio, buf);
	EnviaAServidor("PASS :%s", conf_server->password.local);
	EnviaAServidor("SERVER %s 1 %lu %lu J10 %s]]] +hs :%s", me.nombre, iniciado, time(0), me.trio, me.info);
}
SOCKFUNC(PROT_PARSEA(P10))
{
	char *p, *para[MAXPARA + 1], sender[HOSTLEN + 1], *s;
	Comando *comd = NULL;
	int i, j, params, raw;
	Cliente *cl = NULL;
	para[0] = (linkado ? linkado->nombre : NULL);
	for (p = data; *p == ' '; p++);
	s = sender;
	*s = '\0';
	if (*p == ':')
	{
		if (!linkado)
		{
			for (++p; *p != ' '; p++);
			while (*p == ' ')
				p++;
		}
		else
		{
			para[0] = p + 1;
			if (!(p = strchr(p, ' ')))
				return -1;
			*p++ = '\0';
			if (!(cl = BuscaCliente(para[0])))
			{
				while (*p == ' ')
					p++;
				if (p[1] == 'Q')
				{
					para[0] = (linkado ? linkado->nombre : NULL);
					cl = linkado;
				}
				else
					return 0;
			}
		}
	}
	else
	{
		if (linkado) /* ya es servidor, podemos recibir numerics */
		{
			char npref[6];
			for (i = 0; i < 5; i++)
			{
				if (p[i] == '\0' || (npref[i] = p[i]) == ' ')
					break;
			}
			npref[i] = '\0';
			if (i == 0) // error
				cl = linkado;
			else
			{
				do
				{
					++p;
				} while (*p != ' ' && *p);
				if (!(cl = BuscaClienteNumerico(npref, NULL)))
				{
					while (*p == ' ')
						p++;
					if (p[1] == 'Q' || (*p == 'D' && p[1] == ' ') || (*p == 'K' && p[2] == 'L'))
						cl = linkado;
					para[0] = (linkado ? linkado->nombre : NULL);
				}
				else
					para[0] = cl->nombre;
				while (*p == ' ')
					p++;
			}
			if (*p == '\0') /* mensaje vacío? */
				return 0;
		}
	}
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
					cl = BuscaClienteNumerico(para[0], NULL);
				if (comd->funcion[j])
					comd->funcion[j](sck, cl, para, i);
			}
		}
	}
	if ((raw = atoi(p))) /* es numerico */
		LlamaSenyal(SIGN_RAW, 4, cl, para, i, raw);
	return 0;
}
char *militime_float(char* start)
{
	static char timebuf[18];
	char *p;
#ifdef _WIN32
	struct _timeb tv;
	_ftime(&tv);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
#endif
	if (start)
	{
		if ((p = strchr(start, '.')))
		{
			p++;
#ifdef _WIN32
			sprintf(timebuf, "%ld", (tv.time - atoi(start)) * 1000 + (tv.millitm - atoi(p)) / 1000);
#else
			sprintf(timebuf, "%ld", (tv.tv_sec - atoi(start)) * 1000 + (tv.tv_usec - atoi(p)) / 1000);
#endif
		}
		else
      			strlcpy(timebuf, "0", sizeof(timebuf));
	}
	else
#ifdef _WIN32
		sprintf(timebuf, "%ld.%ld", tv.time, tv.millitm);
#else
		sprintf(timebuf, "%ld.%ld", tv.tv_sec, tv.tv_usec);
#endif
	return timebuf;
}
IRCFUNC(m_eos)
{
	if (cl == linkado)
	{
		EnviaAServidor("%s %s", me.trio, TOK_EOB_ACK);
		EnviaAServidor("%s %s :Sincronización realizada en %.3f segs", me.trio, TOK_WALLOPS, (double)((clock() - tburst)/CLOCKS_PER_SEC));
		intentos = 0;
		LlamaSenyal(SIGN_EOS, 0);
#ifdef _WIN32
		ChkBtCon(1, 0);
#endif
	}
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
	strlcpy(par, parv[2], sizeof(par));
	for (i = 0, param[i] = strtok(par, " "); param[i]; param[++i] = strtok(NULL, " "));
	params = i;
	if (!(bl = BuscaClienteNumerico(parv[1], NULL)))
		return 1; /* algo passa! */
	if (!strcasecmp(param[0], "\1PING"))
	{
		EnviaAServidor("%s %s %s :%s", parv[1], TOK_NOTICE, cl->trio, parv[2]);
		return 0;
	}
	else if (!strcasecmp(param[0], "\1VERSION\1"))
	{
		EnviaAServidor("%s %s %s :\1VERSION %s .%s.\1", parv[1], TOK_NOTICE, cl->trio, COLOSSUS_VERSION, conf_set->red);
		return 0;
	}
	else if (!strcasecmp(param[0], "CREDITOS"))
	{
		int i;
		Responde(cl, bl, COLOSSUS_VERSION " (rv%i) - Trocotronic @2004-2009", rev);
		Responde(cl, bl, " ");
		for (i = 0; creditos[i]; i++)
			Responde(cl, bl, creditos[i]);
		return 0;
	}
	if ((mod = BuscaModulo(bl->nombre, modulos)))
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
	mod = BuscaModulo(parv[2], modulos);
	if (mod)
	{
		EnviaAServidor("%s 311 %s %s %s %s * :%s", me.trio, cl->trio, mod->nick, mod->ident, mod->host, mod->realname);
		EnviaAServidor("%s 312 %s %s %s :%s", me.trio, cl->trio, mod->nick, me.nombre, me.info);
		EnviaAServidor("%s 307 %s %s :Tiene el nick Registrado y Protegido", me.trio, cl->trio, mod->nick);
		EnviaAServidor("%s 310 %s %s :Es un OPERador de los servicios de red", me.trio, cl->trio, mod->nick);
		EnviaAServidor("%s 316 %s %s :es un roBOT oficial de la Red %s", me.trio, cl->trio, mod->nick, conf_set->red);
		EnviaAServidor("%s 313 %s %s :es un IRCop", me.trio, cl->trio, mod->nick);
		EnviaAServidor("%s 318 %s %s :Fin de /WHOIS", me.trio, cl->trio, mod->nick);
	}
	return 0;
}
IRCFUNC(m_nick)
{
	if (parc > 3)
	{
		Cliente *al = NULL;
		if (parc > 8)
		{
			struct in_addr tmp;
			tmp.s_addr = htonl(base64toint(parv[parc - 3]));
			al = NuevoCliente(parv[1], parv[4], parv[5], inet_ntoa(tmp), cl->nombre, parv[5], parc > 9 ? parv[6] : NULL, parv[parc - 1]);
			al->numeric = b642int(parv[parc - 2]);
			al->trio = strdup(parv[parc - 2]);
			InsertaClienteEnNumerico(al, parv[parc - 2], tTab);
			if (parc > 10) /* hay account */
			{
				char *d;
				if ((d = strchr(parv[7], ':')))
				{
					if (strncasecmp(parv[7], parv[1], d - parv[7]))
						ProtFunc(P_MODO_USUARIO_REMOTO)(cl, &me, "-r");
				}
			}
		}
		if (BuscaModulo(parv[1], modulos))
		{
			p_kill(al, &me, "Nick protegido.");
			ReconectaBot(parv[1]);
		}
		LlamaSenyal(SIGN_POST_NICK, 2, cl, 0);
		if (parc > 9)
			LlamaSenyal(SIGN_UMODE, 2, cl, parv[6]);
	}
	else
	{
		LlamaSenyal(SIGN_PRE_NICK, 2, cl, parv[1]);
		if (strcasecmp(parv[1], cl->nombre))
			ProcesaModosCliente(cl, "-r");
		CambiaNick(cl, parv[1]);
		LlamaSenyal(SIGN_POST_NICK, 2, cl, 1);
	}
	return 0;
}
IRCFUNC(m_topic)
{
	Canal *cn = NULL;
	if (parc == 3)
	{
		cn = CreaCanal(parv[1]);
		ircstrdup(cn->topic, parv[2]);
		cn->ntopic = cl;
		LlamaSenyal(SIGN_TOPIC, 3, cl, cn, parv[2]);
	}
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
		for (lk = al->canal; lk; lk = lk->sig)
		{
			BorraClienteDeCanal(lk->cn, al);
			ActualizaCanalPrivado(lk->cn);
		}
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
	if (parc > 3)
	{
		int diff = atoi(militime_float(parv[3]));
		EnviaAServidor("%s %s %s %s %i %s", me.trio, TOK_PONG, me.trio, parv[3], diff, militime_float(NULL));
	}
	else
		EnviaAServidor("%s %s %s %s", me.trio, TOK_PONG, parv[0], parv[1]);
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
	Cliente *bl;
	if ((bl = BuscaCliente(parv[1])))
	{
		ProcesaModosCliente(bl, parv[2]);
		LlamaSenyal(SIGN_UMODE, 2, bl, parv[2]);
	}
	else
	{
		Canal *cn;
		cn = CreaCanal(parv[1]);
		ProcesaModo(cl, cn, parv + 2, parc - 2);
		ActualizaCanalPrivado(cn);
		LlamaSenyal(SIGN_MODE, 4, cl, cn, parv + 2, EsServidor(cl) ? parc - 3 : parc - 2);
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
	char numeric[3];
	int i = 0, prim = 0;
	if (!cl && atoi(parv[5] + 1) < 10)
	{
		Alerta(FERR, "Version IRCU incorrecta. Sólo funciona con versiones de P10");
		SockClose(sck);
		return 1;
	}
	numeric[i++] = *parv[6];
	if (*(parv[6] + 3) != '\0')
		numeric[i++] = *(parv[6] + 1);
	numeric[i++] = '\0';
	al = NuevoCliente(parv[1], NULL, NULL, NULL, NULL, NULL, NULL, parv[parc-1]);
	al->protocol = atoi(parv[5] + 1);
	al->numeric = b642int(numeric);
	al->tipo = TSERVIDOR;
	al->trio = strdup(numeric);
	InsertaClienteEnNumerico(al, numeric, tTab);
	if (!cl) /* primer link */
	{
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
	if ((al = BuscaCliente(parv[1])))
	{
		LlamaSenyal(SIGN_SQUIT, 1, al);
		LiberaMemoriaCliente(al);
	}
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
IRCFUNC(m_rehash)
{
	if (!strcasecmp(parv[1], me.nombre))
		Refresca();
	return 0;
}
IRCFUNC(sincroniza)
{
	tburst = clock();
	LlamaSenyal(SIGN_SYNCH, 0);
	EnviaAServidor("%s %s", me.trio, TOK_EOB);
	return 0;
}
IRCFUNC(m_burst)
{
	int i;
	Canal *cn;
	cn = CreaCanal(parv[1]);
	for (i = 3; i < parc; i++)
	{
		if (*parv[i] == '+') /* hay modos */
		{
			int params = 0;
			if (strchr(parv[i], 'k'))
				params++;
			if (strchr(parv[i], 'l'))
				params++;
			ProcesaModo(cl, cn, parv + i, params);
			LlamaSenyal(SIGN_MODE, 4, cl, cn, parv + i, params);
		}
		else if (*parv[i] == '%')
		{
			char *bans, *b, *p;
			MallaMascara *mmk;
			if (!(mmk = BuscaMallaMascara(cn, 'b')))
				continue;
			bans = strdup(parv[i]);
			for (b = bans; !BadPtr(b); b = p)
			{
				p = strchr(b, ' ');
				if (p)
					*p++ = '\0';
				InsertaMascara(cl, &mmk->malla, b);
			}
			Free(bans);
		}
		else
		{
			char mod[3], *users, *p, *q, *d;
			Cliente *al;
			users = strdup(parv[i]);
			mod[0] = '\0';
			for (p = users; !BadPtr(p); p = q)
			{
				q = strchr(p, ',');
				if (q)
					*q++ = '\0';
				d = strchr(p, ':');
				if (d) /* cambiamos de flags */
				{
					*d++ = mod[0] = '\0';
					while (*d)
					{
						if (*d == 'o')
							strlcat(mod, "o", sizeof(mod));
						else if (*d == 'v')
							strlcat(mod, "v", sizeof(mod));
						d++;
					}
				}
				if ((al = BuscaClienteNumerico(p, NULL)))
				{
					EntraCliente(al, parv[1]);
					if (mod[0])
					{
						char *c, *arr[4];
						int j = 0;
						arr[j++] = mod;
						for (c = &mod[0]; *c; c++)
							arr[j++] = p;
						arr[j] = NULL;
						ProcesaModo(cl, cn, arr, j);
						for (c = &mod[0], j = 1; *c; c++)
							arr[j++] = al->nombre;
						LlamaSenyal(SIGN_MODE, 4, cl, cn, arr, j);
					}
				}
			}
			Free(users);
		}
	}
	return 0;
}
IRCFUNC(m_create)
{
	char *canal;
	strlcpy(tokbuf, parv[1], sizeof(tokbuf));
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
		EntraCliente(cl, canal);
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
		/*	case 'b':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaBan(cl, &cn->ban, parv[param]);
				else
					BorraBanDeCanal(&cn->ban, parv[param]);
				param++;
				break;
			case 'k':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(cn->clave, parv[param]);
					cn->modos |= FlagAModo('k', PROT_CMODOS(P10));
				}
				else
				{
					if (cn->clave)
						Free(cn->clave);
					cn->clave = NULL;
					cn->modos &= ~FlagAModo('k', PROT_CMODOS(P10));
				}
				param++;
				break;
			case 'l':
				if (modo == ADD)
				{
					if (!parv[param])
						break;
					cn->limite = atoi(parv[param]);
					cn->modos |= FlagAModo('l', PROT_CMODOS(P10));
					param++;
				}
				else
				{
					cn->limite = 0;
					cn->modos &= ~FlagAModo('l', PROT_CMODOS(P10));
				}
				break;
			case 'o':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->op, BuscaClienteNumerico(parv[param], NULL));
				else
					BorraModoCliente(&cn->op, BuscaClienteNumerico(parv[param], NULL));
				param++;
				break;
			case 'v':
				if (!parv[param])
					break;
				if (modo == ADD)
					InsertaModoCliente(&cn->voz, BuscaClienteNumerico(parv[param], NULL));
				else
					BorraModoCliente(&cn->voz, BuscaClienteNumerico(parv[param], NULL));
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
						InsertaModoCliente(&mcl->malla, BuscaClienteNumerico(parv[param], NULL));
					else if ((mmk = BuscaMallaMascara(cn, *mods)))
						InsertaMascara(cl, &mmk->malla, parv[param]);
					else if ((mpm = BuscaMallaParam(cn, *mods)))
						ircstrdup(mpm->param, parv[param]);
					else
						cn->modos |= FlagAModo(*mods, protocolo->cmodos);
				}
				else
				{
					if ((mcl = BuscaMallaCliente(cn, *mods)))
						BorraModoCliente(&mcl->malla, BuscaClienteNumerico(parv[param], NULL));
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
	cn = CreaCanal(canal);
	if (!cn->miembros)
	{
		if (conf_set->debug && !strcmp(canal, conf_set->debug))
		{
			if (!IsAdmin(cl))
			{
				EnviaAServidor("%s %s %s %s :No puedes estar aqui", me.trio, TOK_KICK, cl->trio, canal);
				return;
			}
			p_mode(&me, cn, "+sim");
		}
		else if (protocolo->modcanales)
			p_mode(&me, cn, protocolo->modcanales);
	}
	InsertaCanalEnCliente(cl, cn);
	InsertaClienteEnCanal(cn, cl);
	ActualizaCanalPrivado(cn);
	LlamaSenyal(SIGN_JOIN, 2, cl, cn);
}
void ActualizaCanalPrivado(Canal *cn)
{
	if (!(cn->modos & CHMODE_INVITEONLY || cn->modos & CHMODE_KEY || cn->modos & CHMODE_RGSTRONLY || cn->modos & CHMODE_MODERATED))
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
