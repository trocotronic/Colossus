#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#ifdef UDB
#include "bdd.h"
#endif

double tburst;
static char *modcanales;

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
IRCFUNC(m_tburst);

ProtInfo info = {
	"Protocolo P10" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@telefonica.net" 
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
#define MSG_TBURST "TBURST"
#define TOK_TBURST "TB"
#define MSG_IDENTIFY "IDENTIFY"
#define TOK_IDENTIFY "ID"

#define U_REG 0x1
#define U_OPER 0x2
#define U_LOCOPER 0x4
#define U_HIDE 0x8
#define U_INV 0x10
#define U_WALLOP 0x20
#define U_SERVNOTICE 0x40
#define U_DEAF 0x80
#define U_CHSERV 0x100
#define U_DEBUG 0x200
#define U_ADM 0x400
#define U_SUSPEND 0x800
#define U_VIEWIP 0x1000
#define U_DEVEL 0x2000
#define U_COADM 0x4000
#define U_PREO 0x8000
#define U_ONLYREG 0x10000
#define U_USERBOT 0x20000
#define U_BOT 0x40000
#define U_IDED 0x80000

#define C_OP 0x1
#define C_VOICE 0x2
#define C_PRIVATE 0x4
#define C_SECRET 0x8
#define C_MODERATED 0x10
#define C_TOPIC 0x20
#define C_INVITE 0x40
#define C_NOPRIV 0x80
#define C_KEY 0x100
#define C_BAN 0x200
#define C_LIMIT 0x400
#define C_REG 0x800
#define C_REGONLY 0x1000
#define C_OPERONLY 0x2000
#define C_HALF 0x4000
#define C_OWN 0x8000
#define C_BADWORDS 0x10000
#define C_NOCOLORS 0x20000
#define C_AUTOOP 0x40000
#define C_AUDI 0x80000
#define C_JOINP 0x100000
#define C_SUSPEND 0x200000
#define C_REGMOD 0x400000
#define C_NOCTCPS 0x800000

mTab umodos[] = {
	{ U_REG , 'r' } , /* reg */
	{ U_ADM , 'A' } , /* netadmin */
	{ U_OPER , 'o' } , /* oper */
	{ U_LOCOPER , 'O' } , /* helpop */
	{ U_HIDE , 'x' } , /* hide */
	{ U_SUSPEND , 'S' } ,
	{ U_INV, 'i' },
  	{ U_WALLOP, 'w' },
  	{ U_SERVNOTICE, 's' },
  	{ U_DEAF, 'd' },
  	{ U_CHSERV, 'k' },
  	{ U_DEBUG, 'g' },
  	{ U_VIEWIP , 'X' } ,
  	{ U_DEVEL , 'D' } ,
  	{ U_COADM , 'a' } ,
  	{ U_PREO , 'p' } ,
  	{ U_ONLYREG , 'R' } ,
  	{ U_USERBOT , 'b' } ,
  	{ U_BOT , 'B' } ,
  	{ U_IDED , 'n' } ,
	{0x0, '0'}
};
mTab cmodos[] = {
	{ C_REG , 'r' } ,
	{ C_REGONLY , 'R' } , /* rgstronly */
	{ C_OPERONLY , 'O' } , /* operonly */
	{ 0x0 , 0x0 } , /* admonly */
	{ C_HALF , 'h' } , /* half */
	{ 0x0 , 0x0 } , /* adm */
	{ C_OWN , 'q' } , /* owner */
	{ C_OP , 'o' } ,
	{ C_VOICE , 'v' } ,
	{ C_PRIVATE , 'p' } ,
	{ C_SECRET , 's' } ,
	{ C_MODERATED , 'm' } ,
	{ C_TOPIC , 't' } ,
	{ C_INVITE , 'i' } ,
	{ C_NOPRIV , 'n' } ,
	{ C_KEY , 'k' } ,
	{ C_BAN , 'b' } ,
	{ C_LIMIT , 'l' } ,
	{ C_BADWORDS , 'B' } ,
	{ C_NOCOLORS , 'c' } ,
	{ C_AUTOOP , 'A' } ,
	{ C_AUDI , 'C' } ,
	{ C_JOINP , 'j' } ,
	{ C_SUSPEND , 'S' } ,
	{ C_REGMOD , 'M' } ,
	{ C_NOCTCPS , 'G' } ,
	{0x0, '0'}
};

void entra_usuario(Cliente *, char *);
void p_kick_vl(Cliente *, Cliente *, Canal *, char *, va_list *);
int p_msg_vl(Cliente *, Cliente *, char, char *, va_list *);

/* Para no tocar la variable ->hsig, se crea otra estructura para guardar los trios y se indexa por el numeric */
typedef struct _trio
{
	Cliente *cl;
	struct _trio *hsig;
}Trio;
void int2b64(char *destino, int num, size_t tam)
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
int b642int(char *num)
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
Cliente *busca_cliente_numerico(char *clave, Cliente *lugar)
{
	Trio *aux;
	u_int hash;
	int num;
	num = b642int(clave);
	hash = hash_cliente(clave);
	for (aux = (Trio *)uTab[hash].item; aux; aux = aux->hsig)
	{
		if (aux->cl && aux->cl->numeric == num)
			return aux->cl;
	}
	return lugar;
}
void inserta_cliente_en_numerico(Cliente *us, char *clave, Hash *tabla)
{
	Trio *aux;
	u_int hash = 0;
	hash = hash_cliente(clave);
	da_Malloc(aux, Trio);
	aux->cl = us;
	aux->hsig = tabla[hash].item;
	tabla[hash].item = aux;
	tabla[hash].items++;
}
int borra_cliente_de_numerico(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Trio *aux, *prev = NULL;
	hash = hash_cliente(clave);
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
	return cl->trio;
}
int p_umode(Cliente *cl, char *modos)
{
	procesa_umodos(cl, modos);
	return 0;
}
int p_svsmode(Cliente *cl, Cliente *bl, char *modos, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, modos);
	vsprintf_irc(buf, modos, vl);
	va_end(vl);
	if (!strcmp(buf, "+r"))
		sendto_serv("%s %s %s", me.trio, TOK_IDENTIFY, cl->nombre);
	return 0;
}
int p_mode(Cliente *cl, Canal *cn, char *modos, ...)
{
	char buf[BUFSIZE], *copy;
	va_list vl;
	if (!cn)
		return 0;
	va_start(vl, modos);
	vsprintf_irc(buf, modos, vl);
	va_end(vl);
	copy = strdup(buf);
	procesa_modos(cn, buf);
	sendto_serv("%s %s %s %s", cl->trio, TOK_MODE, cn->nombre, copy);
	Free(copy);
	return 0;
}
int p_nick(Cliente *cl, char *nuevo)
{
	sendto_serv("%s %s %s", cl->trio, TOK_NICK, nuevo);
	return 0;
}
int p_join(Cliente *bl, Canal *cn)
{
	sendto_serv("%s %s %s", bl->trio, TOK_JOIN, cn->nombre);
	return 0;
}
int p_part(Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		vsprintf_irc(buf, motivo, vl);
		va_end(vl);
		sendto_serv("%s %s %s :%s", bl->trio, TOK_PART, cn->nombre, buf);
	}
	else
		sendto_serv("%s %s %s", bl->trio, TOK_PART, cn->nombre);
	return 0;
}
int p_svspart(Cliente *cl, Canal *cn, char *motivo, ...)
{
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
	if (motivo)
	{
		char buf[BUFSIZE];
		va_list vl;
		va_start(vl, motivo);
		vsprintf_irc(buf, motivo, vl);
		va_end(vl);
		sendto_serv("%s %s :%s", bl->trio, TOK_QUIT, buf);
	}
	else
		sendto_serv("%s %s", bl->trio, TOK_QUIT);
	for (lk = bl->canal; lk; lk = lk->sig)
		borra_cliente_de_canal(lk->chan, bl);
	borra_cliente_de_numerico(bl, bl->trio, uTab);
	libera_cliente_de_memoria(bl);
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
		vsprintf_irc(buf, motivo, vl);
		va_end(vl);
		sendto_serv("%s %s %s :%s %s", bl->trio, TOK_KILL, cl->trio, me.nombre, buf);
	}
	else
		sendto_serv("%s %s %s :%s Usuario desconectado.", bl->trio, TOK_KILL, cl->trio, me.nombre);
	senyal2(SIGN_QUIT, cl, buf);
	for (lk = cl->canal; lk; lk = lk->sig)
		borra_cliente_de_canal(lk->chan, cl);
	libera_cliente_de_memoria(cl);
	return 0;
}
int p_nuevonick(Cliente *al)
{
	char *modos, ipb64[8];
	struct in_addr tmp;
	tmp.s_addr = 893387572;
	inttobase64(ipb64, ntohl(tmp.s_addr), 6);
	ircfree(al->trio);
	al->trio = (char *)Malloc(sizeof(char) * 6);
	al->trio[0] = me.trio[0];
	al->trio[1] = me.trio[1];
	int2b64(al->trio + 2, al->numeric, sizeof(char) * 4);
	al->numeric = b642int(al->trio);
	inserta_cliente_en_numerico(al, al->trio, uTab);
	modos = modes2flags(al->modos, umodos, NULL);
	sendto_serv("%s %s %s 1 %lu %s %s +%s %s %s :%s", me.trio, TOK_NICK, al->nombre, time(0), al->ident, al->host, modos, ipb64, al->trio, al->info);
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
		sendto_serv("%s %s * +%s@%s %i :%s", me.trio, TOK_GLINE, ident, host, tiempo, motivo);
	else
		sendto_serv("%s %s * -%s@%s", me.trio, TOK_GLINE, ident, host);
	return 0;
}
void p_kick_vl(Cliente *cl, Cliente *bl, Canal *cn, char *motivo, va_list *vl)
{
	if (motivo)
	{
		char buf[BUFSIZE];
		if (!vl)
			strcpy(buf, motivo);
		else
			vsprintf_irc(buf, motivo, *vl);
		sendto_serv("%s %s %s %s :%s", bl->trio, TOK_KICK, cn->nombre, cl->trio, buf);
	}
	else
		sendto_serv("%s %s %s %s :Usuario expulsado", bl->trio, TOK_KICK, cn->nombre, cl->trio);
}
int p_kick(Cliente *cl, Cliente *bl, Canal *cn, char *motivo, ...)
{
	if (!cl || !cn)
		return 1;
	if (EsServer(cl) || EsBot(cl))
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
	borra_canal_de_usuario(cl, cn);
	borra_cliente_de_canal(cn, cl);
	return 0;
}
int p_topic(Cliente *bl, Canal *cn, char *topic)
{
	if (!cn->topic || strcmp(cn->topic, topic))
	{
		sendto_serv("%s %s %s :%s", bl->trio, TOK_TOPIC, cn->nombre, topic);
		ircstrdup(&cn->topic, topic);
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
	sendto_serv("%s %s %s %s", bl->trio, TOK_INVITE, cl->trio, cn->nombre);
	return 0;
}
int p_msg_vl(Cliente *cl, Cliente *bl, char tipo, char *formato, va_list *vl)
{
	char buf[BUFSIZE];
	if (!vl)
		strcpy(buf, formato);
	else
		vsprintf_irc(buf, formato, *vl);
	if (tipo == 1) 
		sendto_serv("%s %s %s :%s", bl->trio, TOK_PRIVATE, cl->trio, buf);
	else
		sendto_serv("%s %s %s :%s", bl->trio, TOK_NOTICE, cl->trio, buf);
	return 0;
}
Com comandos_especiales[] = {
	{ MSG_NULL , TOK_NULL , (void *)p_trio } ,
	{ MSG_MODE , TOK_MODE , (void *)p_umode } ,
	{ MSG_NULL, TOK_NULL , (void *)p_svsmode } ,
	{ MSG_MODE , TOK_MODE , (void *)p_mode } ,
	{ MSG_NICK , TOK_NICK , (void *)p_nick } ,
	COM_NULL , /* no se puede cambiar un nick remotamente */
	{ MSG_JOIN , TOK_JOIN , (void *)p_join } ,
	COM_NULL , /* no se puede forzar a un usuario a entrar */
	{ MSG_PART , TOK_PART , (void *)p_part } ,
	{ MSG_KICK , TOK_KICK , (void *)p_svspart } , 
	{ MSG_QUIT , TOK_QUIT , (void *)p_quit } ,
	{ MSG_KILL , TOK_KILL , (void *)p_kill } ,
	{ MSG_NICK , TOK_NICK , (void *)p_nuevonick } ,
	{ MSG_PRIVATE , TOK_PRIVATE , (void *)p_priv } ,
	COM_NULL , /* no hay sethost */
	COM_NULL , /* no hay chghost */
	COM_NULL , /* no hay sqline */
	COM_NULL , /* no hay lag */
	COM_NULL , /* no hay swhois */
	{ MSG_GLINE , TOK_GLINE , (void *)p_gline } ,
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
	if (!(eval = busca_entrada(config, "red")))
	{
		conferror("[%s:%s] No se encuentra la directriz red.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "nicklen")))
	{
		conferror("[%s:%s] No se encuentra la directriz nicklen.", config->archivo, config->item);
		error_parcial++;
	}
	if ((eval = busca_entrada(config, "modos")))
	{
		if ((aux = busca_entrada(eval, "canales")))
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s::%i] La directriz canales esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
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
						conferror("[%s:%s::%s::%s::%i] Los modos +ovb no se permiten.", config->archivo, config->item, eval->item, aux->item, aux->linea);
						error_parcial++;
					}
				}
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config)
{
	int i, p;
	if (!conf_set)
		da_Malloc(conf_set, struct Conf_set);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "red"))
			ircstrdup(&conf_set->red, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "nicklen"))
			conf_set->nicklen = abs(atoi(config->seccion[i]->data));
		else if (!strcmp(config->seccion[i]->item, "modos"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "canales"))
					ircstrdup(&modcanales, config->seccion[i]->seccion[p]->data);
			}
		}
	}
}
int carga(Conf *config)
{
	int errores = 0;
#ifdef UDB
	conferror("Este protocolo está compilado bajo UDB y no debería estarlo.");
	return ++errores;
#endif
	if (!test(config, &errores))
		set(config);
	else
	{
		conferror("[%s] La configuracion de %s es errónea", config->archivo, info.nombre);
		return ++errores;
	}
	inserta_comando(MSG_PRIVATE, TOK_PRIVATE, m_msg, INI, 2);
	inserta_comando(MSG_WHOIS, TOK_WHOIS, m_whois, INI, MAXPARA);
	inserta_comando(MSG_NICK, TOK_NICK, m_nick, INI, MAXPARA);
	inserta_comando(MSG_TOPIC, TOK_TOPIC, m_topic, INI, 4);
	inserta_comando(MSG_QUIT, TOK_QUIT, m_quit, FIN, 1);
	inserta_comando(MSG_KILL, TOK_KILL, m_kill, INI, 2);
	inserta_comando(MSG_PING, TOK_PING, m_ping, INI, MAXPARA);
	inserta_comando(MSG_PASS, TOK_PASS, m_pass, INI, 1);
	inserta_comando(MSG_NOTICE, TOK_NOTICE, m_msg, INI, 2);
	inserta_comando(MSG_JOIN, TOK_JOIN, m_join, INI, MAXPARA);
	inserta_comando(MSG_PART, TOK_PART, m_part, FIN, 2);
	inserta_comando(MSG_MODE, TOK_MODE, m_mode, INI, MAXPARA);
	inserta_comando(MSG_KICK, TOK_KICK, m_kick, INI, 3);
	inserta_comando(MSG_VERSION, TOK_VERSION, m_version, INI, MAXPARA);
	inserta_comando(MSG_STATS, TOK_STATS, m_stats, INI, 3);
	inserta_comando(MSG_SERVER, TOK_SERVER, m_server, INI, MAXPARA);
	inserta_comando(MSG_SQUIT, TOK_SQUIT, m_squit, FIN, 2);
	inserta_comando(MSG_ERROR, TOK_ERROR, m_error, INI, MAXPARA);
	inserta_comando(MSG_AWAY, TOK_AWAY, m_away, INI, 1);
	inserta_comando(MSG_REHASH, TOK_REHASH, m_rehash, INI, MAXPARA);
	inserta_comando(MSG_EOB, TOK_EOB, m_eos, INI, MAXPARA);
	inserta_comando(MSG_BURST , TOK_BURST , m_burst , INI , MAXPARA);
	inserta_comando(MSG_CREATE , TOK_CREATE , m_create , INI , MAXPARA);
	inserta_comando(MSG_TBURST , TOK_TBURST , m_tburst , INI , 5);
	return 0;
}
int descarga()
{
	borra_comando(MSG_PRIVATE, m_msg);
	borra_comando(MSG_WHOIS, m_whois);
	borra_comando(MSG_NICK, m_nick);
	borra_comando(MSG_TOPIC, m_topic);
	borra_comando(MSG_QUIT, m_quit);
	borra_comando(MSG_KILL, m_kill);
	borra_comando(MSG_PING, m_ping);
	borra_comando(MSG_PASS, m_pass);
	borra_comando(MSG_NOTICE, m_msg);
	borra_comando(MSG_JOIN, m_join);
	borra_comando(MSG_PART, m_part);
	borra_comando(MSG_MODE, m_mode);
	borra_comando(MSG_KICK, m_kick);
	borra_comando(MSG_VERSION, m_version);
	borra_comando(MSG_STATS, m_stats);
	borra_comando(MSG_SERVER, m_server);
	borra_comando(MSG_SQUIT, m_squit);
	borra_comando(MSG_ERROR, m_error);
	borra_comando(MSG_AWAY, m_away);
	borra_comando(MSG_REHASH, m_rehash);
	borra_comando(MSG_EOB, m_eos);
	borra_comando(MSG_BURST, m_burst);
	borra_comando(MSG_CREATE , m_create);
	borra_comando(MSG_TBURST, m_tburst);
	return 0;
}
void inicia()
{
	char buf[3];
	int2b64(buf, me.numeric, sizeof(buf));
	//sendto_serv(":%s PROTOCTL REQ :TOKEN", me.nombre);
	ircstrdup(&me.trio, buf);
	sendto_serv("PROTOCTL REQ :TOKEN");
	sendto_serv("PASS :%s", conf_server->password.local);
	sendto_serv("SERVER %s 1 %lu %lu J10 %s]]] +hs :%s", me.nombre, iniciado, time(0), me.trio, me.info);
}
SOCKFUNC(parsea)
{
	char *p, *para[MAXPARA + 1], sender[HOSTLEN + 1], *s;
	Comando *comd = NULL;
	int i, j, params;
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
			cl = busca_cliente(para[0], NULL);
			if (!cl)
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
				cl = busca_cliente_numerico(npref, NULL);
				do
				{
					++p;
				} while (*p != ' ' && *p);
				if (!cl)
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
		s = strchr(p, ' ');
		if (s)
			*s++ = '\0';
		comd = busca_comando(p);
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
						cl = busca_cliente_numerico(para[0], NULL);
					if (comd->funcion[j])
						comd->funcion[j](sck, cl, para, i);
				}
			}
		}
	}
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
      			strcpy(timebuf, "0");
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
		sendto_serv("%s %s", me.trio, TOK_EOB);
		sendto_serv("%s %s", me.trio, TOK_EOB_ACK);
		sendto_serv("%s %s :Sincronización realizada en %.3f segs", me.trio, TOK_WALLOPS, abs(microtime() - tburst));
		intentos = 0;
		senyal(SIGN_EOS);
#ifdef _WIN32		
		ChkBtCon(1, 0);
#endif		
	}
	return 0;
}
IRCFUNC(m_msg)
{
	char *param[256], par[BUFSIZE];
	bCom *bcom;
	int params, i;
	Modulo *mod;
	Cliente *bl;
	while (*parv[2] == ':')
		parv[2]++;
	strcpy(par, parv[2]);
	for (i = 0, param[i] = strtok(par, " "); param[i]; param[++i] = strtok(NULL, " "));
	params = i;
	bl = busca_cliente_numerico(parv[1], NULL);
	if (!bl)
		return 1; /* algo passa! */
	if (!strcasecmp(param[0], "\1PING"))
	{
		sendto_serv("%s %s %s :%s", parv[1], TOK_NOTICE, cl->trio, parv[2]);
		return 0;
	}
	else if (!strcasecmp(param[0], "\1VERSION\1"))
	{
		sendto_serv("%s %s %s :\1VERSION %s .%s.\1", parv[1], TOK_NOTICE, cl->trio, COLOSSUS_VERSION, conf_set->red);
		return 0;
	}
	else if (!strcasecmp(param[0], "CREDITOS"))
	{
		creditos();
		return 0;
	}
	if ((mod = busca_modulo(bl->nombre, modulos)))
	{
		for (i = 0; i < mod->comandos; i++)
		{
			bcom = mod->comando[i];
			if (!strcasecmp(param[0], bcom->com))
			{
				if ((bcom->nivel & USER) && !(cl->nivel & USER))
				{
					response(cl, bl, ERR_NOID, "");
					return 0;
				}
				if (bcom->nivel & cl->nivel || bcom->nivel == TODOS)
					bcom->func(cl, parv, parc, param, params);
				else
				{
					if (cl->nivel & USER)
						response(cl, bl, ERR_FORB, "");
					else
						response(cl, bl, ERR_NOID, "");
				}
				return 0;
			}
		}
		if (!EsServer(cl))
			response(cl, bl, ERR_DESC, "");
	}
	return 0;
}
IRCFUNC(m_whois)
{
	Modulo *mod;
	mod = busca_modulo(parv[2], modulos);
	if (mod)
	{
		sendto_serv("%s 311 %s %s %s %s * :%s", me.trio, cl->trio, mod->nick, mod->ident, mod->host, mod->realname);
		sendto_serv("%s 312 %s %s %s :%s", me.trio, cl->trio, mod->nick, me.nombre, me.info);
		sendto_serv("%s 307 %s %s :Tiene el nick Registrado y Protegido", me.trio, cl->trio, mod->nick);
		sendto_serv("%s 310 %s %s :Es un OPERador de los servicios de red", me.trio, cl->trio, mod->nick);
		sendto_serv("%s 316 %s %s :es un roBOT oficial de la Red %s", me.trio, cl->trio, mod->nick, conf_set->red);
		sendto_serv("%s 313 %s %s :es un IRCop", me.trio, cl->trio, mod->nick);
		sendto_serv("%s 318 %s %s :Fin de /WHOIS", me.trio, cl->trio, mod->nick);
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
			al = nuevo_cliente(parv[1], parv[4], parv[5], inet_ntoa(tmp), cl->nombre, parv[5], parc > 9 ? parv[6] : NULL, parv[parc - 1]);
			al->numeric = b642int(parv[parc - 2]);
			al->trio = strdup(parv[parc - 2]);
			inserta_cliente_en_numerico(al, parv[parc - 2], uTab);
			if (parc > 10) /* hay account */
			{
				char *d;
				if ((d = strchr(parv[7], ':')))
				{
					if (strncasecmp(parv[7], parv[1], d - parv[7]))
						port_func(P_MODO_USUARIO_REMOTO)(cl, &me, "-r");
				}
			}
		}
		if (busca_modulo(parv[1], modulos))
		{
			p_kill(al, &me, "Nick protegido.");
			renick_bot(parv[1]);
		}
		senyal2(SIGN_POST_NICK, cl, 0);
		if (parc > 9)
			senyal2(SIGN_UMODE, cl, parv[6]);
	}
	else
	{
		senyal2(SIGN_PRE_NICK, cl, parv[1]);
		if (strcasecmp(parv[1], cl->nombre))
			procesa_umodos(cl, "-r");
		cambia_nick(cl, parv[1]);
		senyal2(SIGN_POST_NICK, cl, 1);
	}
	return 0;
}
IRCFUNC(m_topic)
{
	Canal *cn = NULL;
	if (parc == 3)
	{
		cn = info_canal(parv[1], !0);
		ircstrdup(&cn->topic, parv[2]);
		cn->ntopic = cl;
		senyal3(SIGN_TOPIC, cl, cn, parv[2]);
	}
	return 0;
}
IRCFUNC(m_quit)
{
	LinkCanal *lk;
	senyal2(SIGN_QUIT, cl, parv[1]);
	for (lk = cl->canal; lk; lk = lk->sig)
		borra_cliente_de_canal(lk->chan, cl);
	libera_cliente_de_memoria(cl);
	return 0;
}
IRCFUNC(m_kill)
{
	Cliente *al;
	if (EsServer(cl))
		return 0;
	if ((al = busca_cliente(parv[1], NULL)))
	{
		LinkCanal *lk;
		for (lk = al->canal; lk; lk = lk->sig)
			borra_cliente_de_canal(lk->chan, al);
		libera_cliente_de_memoria(al);
	}
	if (conf_set->opts & REKILL)
	{
		if (busca_modulo(parv[1], modulos))
			renick_bot(parv[1]);
	}
	return 0;
}
IRCFUNC(m_ping)
{
	if (parc > 3)
	{
		int diff = atoi(militime_float(parv[3]));
		sendto_serv("%s %s %s %s %i %s", me.trio, TOK_PONG, me.trio, parv[3], diff, militime_float(NULL));
	}
	return 0;
}
IRCFUNC(m_pass)
{
	if (strcmp(parv[1], conf_server->password.remoto))
	{
		fecho(FERR, "Contraseñas incorrectas");
		sockclose(sck, LOCAL);
	}
	return 0;
}
IRCFUNC(m_join)
{
	char *canal;
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
		entra_usuario(cl, canal);
	return 0;
}
IRCFUNC(m_part)
{
	Canal *cn;
	cn = busca_canal(parv[1], NULL);
	senyal3(SIGN_PART, cl, cn, parv[2]);
	borra_canal_de_usuario(cl, cn);
	borra_cliente_de_canal(cn, cl);
	return 0;
}
IRCFUNC(m_mode)
{
	Cliente *bl;
	if ((bl = busca_cliente(parv[1], NULL)))
	{
		procesa_umodos(bl, parv[2]);
		senyal2(SIGN_UMODE, bl, parv[2]);
	}
	else
	{
		Canal *cn;
		cn = info_canal(parv[1], !0);
		procesa_modo(cl, cn, parv + 2, parc - 2);
		senyal4(SIGN_MODE, cl, cn, parv + 2, EsServer(cl) ? parc - 3 : parc - 2);
	}
	return 0;
}
IRCFUNC(m_kick)
{
	Canal *cn;
	Cliente *al;
	if (!(al = busca_cliente(parv[2], NULL)))
		return 1;
	cn = info_canal(parv[1], 0);
	senyal4(SIGN_KICK, cl, al, cn, parv[3]);
	borra_canal_de_usuario(al, cn);
	borra_cliente_de_canal(cn, al);
	if (EsBot(al))
	{
		if (conf_set->opts & REJOIN)
			mete_bot_en_canal(al, parv[1]);
	}
	return 0;
}
IRCFUNC(m_version)
{
	sendto_serv(":%s 351 %s :%s .%s. iniciado el %s", me.nombre, parv[0], COLOSSUS_VERSION, conf_set->red, _asctime(&inicio));
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
			sendto_serv(":%s 242 %s :Servidor online %i días, %02i:%02i:%02i", me.nombre, parv[0], dura / 86400, (dura / 3600) % 24, (dura / 60) % 60, dura % 60);
			break;
		}
	}
	sendto_serv(":%s 219 %s %s :Fin de /STATS", me.nombre, parv[0], parv[1]);
	return 0;
}
IRCFUNC(m_server)
{
	Cliente *al;
	char numeric[3];
	if (!cl && atoi(parv[5] + 1) < 10)
	{
		fecho(FERR, "Version IRCU incorrecta. Sólo funciona con versiones de P10");
		sockclose(sck, LOCAL);
		return 1;
	}
	al = nuevo_cliente(parv[1], NULL, NULL, NULL, NULL, NULL, NULL, parv[parc-1]);
	al->protocol = atoi(parv[5] + 1);
	numeric[0] = *parv[6];
	numeric[1] = *(parv[6] + 1);
	numeric[2] = '\0';
	al->numeric = b642int(numeric);
	al->tipo = ES_SERVER;
	al->trio = strdup(numeric);
	inserta_cliente_en_numerico(al, numeric, uTab);
	if (!cl) /* primer link */
	{
		linkado = al;
		sincroniza(sck, al, parv, parc);
	}
	else
		ircd_log(LOG_SERVER, "Servidor %s (%s)", al->nombre, al->info);
	return 0;
}
IRCFUNC(m_squit)
{
	Cliente *al;
	al = busca_cliente(parv[1], NULL);
	libera_cliente_de_memoria(al);
	return 0;
}
IRCFUNC(m_error)
{
	ircd_log(LOG_ERROR, "ERROR: %s", parv[1]);
	return 0;
}
IRCFUNC(m_away)
{
	if (parc == 2)
		cl->nivel |= AWAY;
	else if (parc == 1)
		cl->nivel &= ~AWAY;
	senyal2(SIGN_AWAY, cl, parv[1]);
	return 0;
}
IRCFUNC(m_rehash)
{
	if (!strcasecmp(parv[1], me.nombre))
		refresca();
	return 0;
}
IRCFUNC(sincroniza)
{
	tburst = microtime();
	senyal(SIGN_SYNCH);
	return 0;
}
IRCFUNC(m_burst)
{
	int i;
	Canal *cn;
	cn = info_canal(parv[1], !0);
	for (i = 3; i < parc; i++)
	{
		if (*parv[i] == '+') /* hay modos */
		{
			int params = 0;
			if (strchr(parv[i], 'k'))
				params++;
			if (strchr(parv[i], 'l'))
				params++;
			procesa_modo(cl, cn, parv + i, params);
			senyal4(SIGN_MODE, cl, cn, parv + i, params);
		}
		else if (*parv[i] == '%')
		{
			char *bans, *b, *p;
			bans = strdup(parv[i]);
			for (b = bans; !BadPtr(b); b = p)
			{
				p = strchr(b, ' ');
				if (p)
					*p++ = '\0';
				inserta_ban_en_canal(cl, cn, b);
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
							strcat(mod, "o");
						else if (*d == 'v')
							strcat(mod, "v");
						else if (*d == 'h')
							strcat(mod, "h");
						else if (*d == 'q')
							strcat(mod, "q");
						d++;
					}
				}
				al = busca_cliente_numerico(p, NULL);
				entra_usuario(al, parv[1]);
				if (mod[0])
				{
					char *c, *arr[4];
					int j = 0;
					arr[j++] = mod;
					for (c = &mod[0]; *c; c++)
						arr[j++] = p;
					arr[j] = NULL;
					procesa_modo(cl, cn, arr, j);
					for (c = &mod[0], j = 1; *c; c++)
						arr[j++] = al->nombre;
					senyal4(SIGN_MODE, cl, cn, arr, j);
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
	strcpy(tokbuf, parv[1]);
	for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
		entra_usuario(cl, canal);
	return 0;
}
IRCFUNC(m_tburst)
{
	Canal *cn = NULL;
	Cliente *al;
	if (parc == 3)
	{
		cn = info_canal(parv[1], !0);
		al = busca_cliente_numerico(parv[3], NULL);
		ircstrdup(&cn->topic, parv[4]);
		cn->ntopic = al;
		senyal3(SIGN_TOPIC, al, cn, parv[4]);
	}
	return 0;
}
void procesa_modo(Cliente *cl, Canal *cn, char *parv[], int parc)
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
			case 'b':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_ban_en_canal(cl, cn, parv[param]);
				else
					borra_ban_de_canal(cn, parv[param]);
				param++;
				break;
			case 'k':
				if (!parv[param])
					break;
				if (modo == ADD)
				{
					ircstrdup(&cn->clave, parv[param]);
					cn->modos |= flag2mode('k', cmodos);
				}
				else
				{
					if (cn->clave)
						Free(cn->clave);
					cn->clave = NULL;
					cn->modos &= ~flag2mode('k', cmodos);
				}
				param++;
				break;
			case 'l':
				if (modo == ADD)
				{
					if (!parv[param])
						break;
					cn->limite = atoi(parv[param]);
					cn->modos |= flag2mode('l', cmodos);
					param++;
				}
				else
				{
					cn->limite = 0;
					cn->modos &= ~flag2mode('l', cmodos);
				}
				break;
			case 'o':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->op, busca_cliente_numerico(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->op, busca_cliente_numerico(parv[param], NULL));
				param++;
				break;
			case 'v':
				if (!parv[param])
					break;
				if (modo == ADD)
					inserta_modo_cliente_en_canal(&cn->voz, busca_cliente_numerico(parv[param], NULL));
				else
					borra_modo_cliente_de_canal(&cn->voz, busca_cliente_numerico(parv[param], NULL));
				param++;
				break;
			default:
				if (modo == ADD)
					cn->modos |= flag2mode(*mods, cmodos);
				else
					cn->modos &= ~flag2mode(*mods, cmodos);
		}
		mods++;
	}
}
void procesa_modos(Canal *cn, char *modos)
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
	procesa_modo(&me, cn, arv, arc);
}
void entra_usuario(Cliente *cl, char *canal)
{
	Canal *cn = NULL;
	cn = info_canal(canal, !0);
	if (conf_set->debug && cn->miembros == 0 && !strcmp(canal, conf_set->debug))
	{
		if (!IsAdmin(cl))
		{
			sendto_serv("%s %s %s %s :No puedes estar aqui", me.trio, TOK_KICK, cl->trio, canal);
			return;
		}
		p_mode(&me, cn, "+sim");
	}
	inserta_canal_en_usuario(cl, cn);
	inserta_usuario_en_canal(cn, cl);
	senyal2(SIGN_JOIN, cl, cn);
}
