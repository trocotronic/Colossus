/*
 * $Id: statserv.c,v 1.11 2005-03-19 12:48:53 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/statserv.h"

StatServ *statserv = NULL;
char eos, *tempsrc = NULL;
time_t ini;

static int statserv_help		(Cliente *, char *[], int, char *[], int);
static int statserv_ctcpreply		(Cliente *, char *[], int, char *[], int);
static bCom statserv_coms[] = {
	{ "help" , statserv_help , PREOS } ,
	{ "\1VERSION", statserv_ctcpreply , TODOS } ,
	{ 0x0 , 0x0 , 0x0 }
};
IRCFUNC(statserv_nick);
IRCFUNC(statserv_local_uptime);
IRCFUNC(statserv_local_users);
IRCFUNC(statserv_server);
IRCFUNC(statserv_umode2);
IRCFUNC(statserv_squit);
IRCFUNC(statserv_version);
IRCFUNC(statserv_notice);
IRCFUNC(statserv_eos);
SOCKFUNC(statserv_lee_web);
SOCKFUNC(statserv_escribe_web);
static int statserv_umode		(Cliente *cl, char *);
int statserv_create_channel	(Canal *);
int statserv_destroy_channel	(char *);
int statserv_quit		(Cliente *, char *);
int statserv_mysql		();
int statserv_laguea		(char *);
int statserv_sig_bot	();
int statserv_sig_eos	();

void set(Conf *, Modulo *);
int test(Conf *, int *);

ModInfo info = {
	"StatServ" ,
	0.2 ,
	"Trocotronic" ,
	"trocotronic@rallados.net" ,
	"statserv.inc"
};
int carga(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (parseconf(mod->config, &modulo, 1))
		return 1;
	if (!strcasecmp(modulo.seccion[0]->item, info.nombre))
	{
		if (!test(modulo.seccion[0], &errores))
			set(modulo.seccion[0], mod);
	}
	else
	{
		conferror("[%s] La configuracion de %s es erronea", mod->archivo, info.nombre);
		errores++;
	}
	return errores;
}
int descarga()
{
	borra_comando(MSG_NOTICE, statserv_notice);
	borra_comando(MSG_NICK, statserv_nick);
	borra_comando("242", statserv_local_uptime);
	borra_comando("265", statserv_local_users);
	borra_comando(MSG_SERVER, statserv_server);
	if (port_existe(MODO_USUARIO))
		borra_comando(port_msg(MODO_USUARIO),  statserv_umode2);
	borra_comando(MSG_SQUIT, statserv_squit);
	borra_comando("351", statserv_version);
	if (port_existe(EOS))
		borra_comando(port_msg(EOS), statserv_eos);
	borra_senyal(SIGN_UMODE, statserv_umode);
	borra_senyal(SIGN_QUIT, statserv_quit);
	//borra_senyal(SIGN_CREATE_CHAN, statserv_create_channel);
	//borra_senyal(SIGN_DESTROY_CHAN, statserv_destroy_channel);
	borra_senyal(SIGN_MYSQL, statserv_mysql);
	borra_senyal(SIGN_BOT, statserv_sig_bot);
	borra_senyal(SIGN_EOS, statserv_sig_eos);
	return 0;
}
int test(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = busca_entrada(config, "nick")))
	{
		conferror("[%s:%s] No se encuentra la directriz nick.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "ident")))
	{
		conferror("[%s:%s] No se encuentra la directriz ident.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "realname")))
	{
		conferror("[%s:%s] No se encuentra la directriz realname.]\n", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "laguea")))
	{
		conferror("[%s:%s] No se encuentra la directriz laguea.]\n", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (atoi(eval->data) < 60)
		{
			conferror("[%s:%s::%i] La directriz laguea debe ser superior a 60 segundos.]\n", config->archivo, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "web")))
	{
		Conf *temp;
		if (!(temp = busca_entrada(eval, "template")))
		{
			conferror("[%s:%s::%i] Debes especificar la directriz template si usaras web.]\n", config->archivo, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			if (!is_file(temp->data))
			{
				conferror("[%s:%s::%s::%i] No se encuentra el template %s.]\n", config->archivo, eval->item, temp->item, temp->linea, temp->data);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void set(Conf *config, Modulo *mod)
{
	int i, p;
	bCom *ss;
	if (!statserv)
		da_Malloc(statserv, StatServ);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "nick"))
			ircstrdup(&statserv->nick, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "ident"))
			ircstrdup(&statserv->ident, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&statserv->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "realname"))
			ircstrdup(&statserv->realname, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "modos"))
			ircstrdup(&statserv->modos, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "laguea"))
			statserv->laguea = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "funciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ss = &statserv_coms[0];
				while (ss->com != 0x0)
				{
					if (!strcasecmp(ss->com, config->seccion[i]->seccion[p]->item))
					{
						statserv->comando[statserv->comandos++] = ss;
						break;
					}
					ss++;
				}
				if (ss->com == 0x0)
					conferror("[%s:%i] No se ha encontrado la funcion %s", config->seccion[i]->archivo, config->seccion[i]->seccion[p]->linea, config->seccion[i]->seccion[p]->item);
			}
		}
		if (!strcmp(config->seccion[i]->item, "web"))
		{
			statserv->puerto = 80;
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "puerto"))
					statserv->puerto = atoi(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "template"))
					ircstrdup(&statserv->template, config->seccion[i]->seccion[p]->data);
			}
		}
		if (!strcmp(config->seccion[i]->item, "residente"))
			ircstrdup(&statserv->residente, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "clientes"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				ircstrdup(&statserv->clientes[p].cliente, config->seccion[i]->seccion[p]->item);
				ircstrdup(&statserv->clientes[p].patron, config->seccion[i]->seccion[p]->data);
				statserv->clientes[p].usuarios = 0;
			}
			statserv->clientes[p].cliente = statserv->clientes[p].patron = NULL;
			statserv->clientes[p].usuarios = 0;
		}
	}
	if (statserv->mascara)
		Free(statserv->mascara);
	statserv->mascara = (char *)Malloc(sizeof(char) * (strlen(statserv->nick) + 1 + strlen(statserv->ident) + 1 + strlen(statserv->host) + 1));
	sprintf_irc(statserv->mascara, "%s!%s@%s", statserv->nick, statserv->ident, statserv->host);
	inserta_comando(MSG_NOTICE, TOK_NOTICE, statserv_notice, INI, 0);
	inserta_comando(MSG_NICK, TOK_NICK, statserv_nick, INI, 0);
	inserta_comando("242", "242", statserv_local_uptime, INI, 0);
	inserta_comando("265", "265", statserv_local_users, INI, 0);
	inserta_comando(MSG_SERVER, TOK_SERVER, statserv_server, INI, 0);
	if (port_existe(MODO_USUARIO))
		inserta_comando(port_msg(MODO_USUARIO), port_tok(MODO_USUARIO), statserv_umode2, INI, 0);
	inserta_comando(MSG_SQUIT, TOK_SQUIT, statserv_squit, INI, 0);
	inserta_comando("351", "351", statserv_version, INI, 0);
	if (port_existe(EOS))
		inserta_comando(port_msg(EOS), port_tok(EOS), statserv_eos, INI, 0);
	inserta_senyal(SIGN_UMODE, statserv_umode);
	inserta_senyal(SIGN_QUIT, statserv_quit);
	//inserta_senyal(SIGN_CREATE_CHAN, statserv_create_channel);
	//inserta_senyal(SIGN_DESTROY_CHAN, statserv_destroy_channel);
	inserta_senyal(SIGN_MYSQL, statserv_mysql);
	inserta_senyal(SIGN_BOT, statserv_sig_bot);
	inserta_senyal(SIGN_EOS, statserv_sig_eos);
	bot_mod(statserv);
}
int vuelca_template(char *temp)
{
	int fp;
	struct stat inode;
#ifdef _WIN32	
	if ((fp = open(temp, O_RDONLY|O_BINARY)) == -1)
#else
	if ((fp = open(temp, O_RDONLY)) == -1)
#endif
		return -1;
	if (fstat(fp, &inode) == -1)
	{
		close(fp);
		return -2;
	}
	if (!inode.st_size)
	{
		close(fp);
		return -3;
	}
	tempsrc = (char *)Malloc(inode.st_size + 1);
	tempsrc[inode.st_size] = '\0';
	if (read(fp, tempsrc, inode.st_size) != inode.st_size)
	{
		close(fp);
		return -4;
	}
	close(fp);
	return 0;
}
void vuelca()
{
	FILE *fp;
	int i;
	char linea[128], *dom;
	statserv->usuarios.max = coge("users_max");
	statserv->usuarios.hoy = coge("users_hoy");
	statserv->usuarios.semana = coge("users_semana");
	statserv->usuarios.mes = coge("users_mes");
	statserv->usuarios.max_time = coge("users_time");
	statserv->usuarios.hoy_time = coge("users_hoytime");
	statserv->usuarios.semana_time = coge("users_semanatime");
	statserv->usuarios.mes_time = coge("users_mestime");
	statserv->canales.max = coge("canales_max");
	statserv->canales.hoy = coge("canales_hoy");
	statserv->canales.semana = coge("canales_semana");
	statserv->canales.mes = coge("canales_mes");
	statserv->canales.max_time = coge("canales_time");
	statserv->canales.hoy_time = coge("canales_hoytime");
	statserv->canales.semana_time = coge("canales_semanatime");
	statserv->canales.mes_time = coge("canales_mestime");
	statserv->servidores.max = coge("servers_max");
	statserv->servidores.hoy = coge("servers_hoy");
	statserv->servidores.semana = coge("servers_semana");
	statserv->servidores.mes = coge("servers_mes");
	statserv->servidores.max_time = coge("servers_time");
	statserv->servidores.hoy_time = coge("servers_hoytime");
	statserv->servidores.semana_time = coge("servers_semanatime");
	statserv->servidores.mes_time = coge("servers_mestime");
	statserv->operadores.max = coge("opers_max");
	statserv->operadores.hoy = coge("opers_hoy");
	statserv->operadores.semana = coge("opers_semana");
	statserv->operadores.mes = coge("opers_mes");
	statserv->operadores.max_time = coge("opers_time");
	statserv->operadores.hoy_time = coge("opers_hoytime");
	statserv->operadores.semana_time = coge("opers_semanatime");
	statserv->operadores.mes_time = coge("opers_mestime");
	if (!(fp = fopen("dominums.db", "r")))
		return;
	for (i = 0; (fgets(linea, 128, fp)); i++)
	{
		linea[strlen(linea)-1] = '\0';
		dom = strchr(linea, ' ');
		*dom++ = '\0';
		strncpy(statserv->dominios[i].dominio, linea, 5);
		statserv->dominios[i].donde = strdup(dom);
		statserv->dominios[i].usuarios = 0;
	}
	for (i = 0; i < 256; i++)
		statserv->servidor[i].serv = NULL;
	fclose(fp);
	if (statserv->puerto) /* usa web */
	{
		socklisten(statserv->puerto, NULL, statserv_lee_web, statserv_escribe_web, NULL);
		vuelca_template(statserv->template);
	}
}
int getday(time_t tiempo)
{
	struct tm *time;
	time = localtime(&tiempo);
	return time->tm_mday;
}
int getweek(time_t tiempo)
{
	struct tm primer;
	struct tm *time;
	u_int dias, sem;
	time = localtime(&tiempo);
	dias = time->tm_yday;
	primer.tm_sec = 0;
	primer.tm_min = 0;
	primer.tm_hour = 0;
	primer.tm_mday = 1;
	primer.tm_mon = 0;
	primer.tm_year = time->tm_year;
	primer.tm_isdst = -1;
	mktime(&primer);
	dias += (primer.tm_wday == 0 ? 6 : primer.tm_wday - 1);
	sem = dias / 7;
	return sem;
}
int getmon(time_t tiempo)
{
	struct tm *time;
	time = localtime(&tiempo);
	return time->tm_mon + 1;
}
void actualiza_stats(char tipo)
{
	char *timet = NULL, *max = NULL, *hoyt = NULL, *semt = NULL, *mest = NULL, *hoy = NULL, *sem = NULL, *mes = NULL;
	struct stsno *ts = NULL;
	switch (tipo)
	{
		case STSUSERS:
			ts = &statserv->usuarios;
			timet = "users_time";
			max = "users_max";
			hoyt = "users_hoytime";
			semt = "users_semanatime";
			mest = "users_mestime";
			hoy = "users_hoy";
			sem = "users_semana";
			mes = "users_mes";
			break;
		case STSCHANS:
			ts = &statserv->canales;
			timet = "canales_time";
			max = "canales_max";
			hoyt = "canales_hoytime";
			semt = "canales_semanatime";
			mest = "canales_mestime";
			hoy = "canales_hoy";
			sem = "canales_semana";
			mes = "canales_mes";
			break;
		case STSSERVS:
			ts = &statserv->servidores;
			timet = "servers_time";
			max = "servers_max";
			hoyt = "servers_hoytime";
			semt = "servers_semanatime";
			mest = "servers_mestime";
			hoy = "servers_hoy";
			sem = "servers_semana";
			mes = "servers_mes";
			break;
		case STSOPERS:
			ts = &statserv->operadores;
			timet = "opers_time";
			max = "opers_max";
			hoyt = "opers_hoytime";
			semt = "opers_semanatime";
			mest = "opers_mestime";
			hoy = "opers_hoy";
			sem = "opers_semana";
			mes = "opers_mes";
			break;
	}
	if (ts->no++ > ts->max)
	{
		actualiza(timet, (ts->max_time = time(0)));
		actualiza(max, (ts->max = ts->no));
	}
	if (eos)
	{
		if (ts->hoy_time != getday(time(0))) /* cambio de día */
		{
			actualiza(hoyt, (ts->hoy_time = getday(time(0))));
			ts->hoy = 0;
		}
		if (ts->semana_time != getweek(time(0))) /* cambio de semana */
		{
			actualiza(semt, (ts->semana_time = getweek(time(0))));
			ts->semana = 0;
		}
		if (ts->mes_time != getmon(time(0))) /* cambio de mes */
		{
			actualiza(mest, (ts->mes_time = getmon(time(0))));
			ts->mes = 0;
		}
		actualiza(hoy, ++ts->hoy);
		actualiza(sem, ++ts->semana);
		actualiza(mes, ++ts->mes);
	}
}
int posdom(char *host)
{
	int i;
	char *dom;
	if (!host)
		return DOMINUMS-1;
	dom = strrchr(host, '.');
	if (!dom)
		return DOMINUMS-1;
	dom++;
	for (i = 0; i < DOMINUMS; i++)
	{
		if (!strcmp(dom, statserv->dominios[i].dominio))
			return i;
	}
	return DOMINUMS-1;
}
BOTFUNC(statserv_help)
{
	return 0;
}
BOTFUNC(statserv_ctcpreply)
{
	int i;
	char *version;
	version = strchr(parv[2], ' ')+1;
	for (i = 0; statserv->clientes[i].patron; i++)
	{
		if (!match(statserv->clientes[i].patron, version))
		{
			statserv->clientes[i].usuarios++;
			return 1;
		}
	}
	return 0;
}
IRCFUNC(statserv_nick)
{
	if (parc > 4)
	{
		cl = busca_cliente(parv[1], NULL);
		actualiza_stats(STSUSERS);
		statserv_umode(cl, parv[7]);
		statserv->servidor[cl->server->numeric].usuarios++;
		sendto_serv(":%s %s %s :\001VERSION\001", statserv->nick, TOK_PRIVATE, cl->nombre);
		statserv->dominios[posdom(cl->rvhost)].usuarios++;
	}
	return 0;
}
IRCFUNC(statserv_local_uptime)
{
	int d, h, m , s;
	strcpy(tokbuf, parv[2]);
	strtok(tokbuf, " ");
	strtok(NULL, " ");
	d = atoi(strtok(NULL, " "));
	strtok(NULL, " ");
	h = atoi(strtok(NULL, ":"));
	m = atoi(strtok(NULL, ":"));
	s = atoi(strtok(NULL, ":"));
	statserv->servidor[cl->numeric].uptime = d*86400 + h*3600 + m*60 + s;
	if (!_mysql_get_registro(SS_MYSQL, cl->nombre, "valor") || coge(cl->nombre) < statserv->servidor[cl->numeric].uptime)
		actualiza(cl->nombre, statserv->servidor[cl->numeric].uptime);
	return 0;
}
IRCFUNC(statserv_local_users)
{
	char *num;
	num = strrchr(parv[2], ' ')+1;
	statserv->servidor[cl->numeric].max_users = atoi(num);
	return 0;
}
IRCFUNC(statserv_server)
{
	Cliente *al;
	char *serv = NULL;
	if (!cl)
		return 0; /* algo pasa! */
	if (linkado == cl)
	{
		if (statserv->usuarios.hoy_time) /* ya habían sido arrancadas previamente */
			eos = 0;
		ini = time(0);
	}
	actualiza_stats(STSSERVS);
	if (parc == 3)
		serv = parv[0];
	else if (parc == 4)
		serv = parv[1];
	/* aprovechando que el numeric es unico, lo utilizo como indice */
	al = busca_cliente(serv, NULL);
	statserv->servidor[al->numeric].serv = al;
	statserv->servidor[al->numeric].usuarios = statserv->servidor[al->numeric].operadores = 
		statserv->servidor[al->numeric].max_users = statserv->servidor[al->numeric].lag = 
		statserv->servidor[al->numeric].uptime = 0;
	statserv->servidor[al->numeric].version = NULL;
	sendto_serv(":%s %s :%s", statserv->nick, TOK_VERSION, al->nombre);
	sendto_serv(":%s %s :%s", statserv->nick, TOK_LUSERS, al->nombre);
	sendto_serv(":%s %s u %s", statserv->nick, TOK_STATS, al->nombre);
	if (port_existe(LAG))
	{
		sendto_serv(":%s %s %s", statserv->nick, port_tok(LAG), al->nombre);
		timer(al->nombre, cl->sck, 0, statserv->laguea, statserv_laguea, al->nombre, sizeof(char) * (strlen(al->nombre) + 1));
	}
	return 0;
}
IRCFUNC(statserv_umode2)
{
	statserv_umode(cl, parv[1]);
	return 0;
}
IRCFUNC(statserv_squit)
{
	statserv->servidores.no--;
	statserv->servidor[cl->numeric].usuarios = statserv->servidor[cl->numeric].operadores = 
		statserv->servidor[cl->numeric].max_users = statserv->servidor[cl->numeric].lag = 
		statserv->servidor[cl->numeric].uptime = 0;
	statserv->servidor[cl->numeric].serv = NULL;
	timer_off(cl->nombre, cl->sck);
	return 0;
}
IRCFUNC(statserv_version)
{
	ircstrdup(&statserv->servidor[cl->numeric].version, parv[2]);
	return 0;
}
IRCFUNC(statserv_notice)
{
	if (cl && EsServer(cl))
	{
		strcpy(tokbuf, parv[2]);
		if (!strcmp(strtok(tokbuf, " "), "Lag"))
		{
			char *lag;
			lag = strrchr(parv[2], ' ')+1;
			statserv->servidor[cl->numeric].lag = time(0) - atoi(lag);
			return 0;
		}
	}
	return 0;
}

int statserv_umode(Cliente *cl, char *modos)
{
	if ((strchr(modos, 'o') && IsIrcop(cl)) || (strchr(modos, 'h') && IsOper(cl)))
	{
		actualiza_stats(STSOPERS);
		statserv->servidor[cl->server->numeric].operadores++;
	}
	else if ((strchr(modos, 'o') && !IsIrcop(cl)) || (strchr(modos, 'h') && !IsOper(cl)))
	{
		statserv->operadores.no--;
		statserv->servidor[cl->server->numeric].operadores--;
	}
	return 0;
}
IRCFUNC(statserv_eos)
{
	if (cl == linkado)
		eos = 1;
	return 0;
}
int statserv_quit(Cliente *cl, char *msg)
{
	statserv->usuarios.no--;
	statserv->servidor[cl->server->numeric].usuarios--;
	if (cl->rvhost)
		statserv->dominios[posdom(cl->rvhost)].usuarios--;
	return 0;
}
int statserv_create_channel(Canal *cn)
{
	actualiza_stats(STSCHANS);
	return 0;
}
int statserv_destroy_channel(char *nombre)
{
	statserv->canales.no--;
	return 0;
}
int statserv_mysql()
{
	if (!_mysql_existe_tabla(SS_MYSQL))
	{
		if (_mysql_query("CREATE TABLE `%s%s` ( "
			"`item` varchar(255) default NULL, "
  			"`valor` varchar(255) NOT NULL default '0', "
			"KEY `item` (`item`) "
			") TYPE=MyISAM COMMENT='Tabla estadísticas';", PREFIJO, SS_MYSQL))
				fecho(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SS_MYSQL);
		else
		{
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_max");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_hoy");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_semana");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_mes");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_time");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_hoytime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_semanatime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "users_mestime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_max");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_hoy");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_semana");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_mes");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_time");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_hoytime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_semanatime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "canales_mestime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_max");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_hoy");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_semana");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_mes");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_time");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_hoytime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_semanatime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "servers_mestime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_max");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_hoy");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_semana");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_mes");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_time");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_hoytime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_semanatime");
			_mysql_query("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_MYSQL, "opers_mestime");
		}
	}
	_mysql_carga_tablas();
	vuelca();
	return 1;
}
int statserv_laguea(char *servidor)
{
	sendto_serv(":%s %s %s", statserv->nick, port_tok(LAG), servidor);
	return 0;
}
SOCKFUNC(statserv_escribe_web)
{
	sockclose(sck, 0);
	return 0;
}
SOCKFUNC(statserv_lee_web)
{
	if (BadPtr(data))
	{
		{
			char *td, *ac, *bc = NULL, *fd;
			time_t tm;
			double tb = microtime();
			fd = td = strdup(tempsrc);
			while ((ac = strchr(td, '$')))
			{
				*ac++ = '\0';
				sockwrite(sck, 0x0, "%s", td);
				bc = strchr(++ac, ')');
				*bc++ = '\0';
				if (!strcmp(ac, "USERS_AHORA"))
					sockwrite(sck, 0x0, "%i", statserv->usuarios.no);
				else if (!strcmp(ac, "USERS_MAX"))
					sockwrite(sck, 0x0, "%i", statserv->usuarios.max);
				else if (!strcmp(ac, "USERS_HOY"))
					sockwrite(sck, 0x0, "%i", statserv->usuarios.hoy);
				else if (!strcmp(ac, "USERS_SEMANA"))
					sockwrite(sck, 0x0, "%i", statserv->usuarios.semana);
				else if (!strcmp(ac, "USERS_MES"))
					sockwrite(sck, 0x0, "%i", statserv->usuarios.mes);
				else if (!strcmp(ac, "USERS_TIME"))
				{
					tm = statserv->usuarios.max_time;
					sockwrite(sck, 0x0, "%s",  _asctime(&tm));
				}
				else if (!strcmp(ac, "CANALES_AHORA"))
					sockwrite(sck, 0x0, "%i", statserv->canales.no);
				else if (!strcmp(ac, "CANALES_MAX"))
					sockwrite(sck, 0x0, "%i", statserv->canales.max);
				else if (!strcmp(ac, "CANALES_HOY"))
					sockwrite(sck, 0x0, "%i", statserv->canales.hoy);
				else if (!strcmp(ac, "CANALES_SEMANA"))
					sockwrite(sck, 0x0, "%i", statserv->canales.semana);
				else if (!strcmp(ac, "CANALES_MES"))
					sockwrite(sck, 0x0, "%i", statserv->canales.mes);
				else if (!strcmp(ac, "CANALES_TIME"))
				{
					tm = statserv->canales.max_time;
					sockwrite(sck, 0x0, "%s",  _asctime(&tm));
				}
				else if (!strcmp(ac, "SERVERS_AHORA"))
					sockwrite(sck, 0x0, "%i", statserv->servidores.no);
				else if (!strcmp(ac, "SERVERS_MAX"))
					sockwrite(sck, 0x0, "%i", statserv->servidores.max);
				else if (!strcmp(ac, "SERVERS_HOY"))
					sockwrite(sck, 0x0, "%i", statserv->servidores.hoy);	
				else if (!strcmp(ac, "SERVERS_SEMANA"))
					sockwrite(sck, 0x0, "%i", statserv->servidores.semana);
				else if (!strcmp(ac, "SERVERS_MES"))
					sockwrite(sck, 0x0, "%i", statserv->servidores.mes);
				else if (!strcmp(ac, "SERVERS_TIME"))
				{
					tm = statserv->servidores.max_time;
					sockwrite(sck, 0x0, "%s",  _asctime(&tm));
				}
				else if (!strcmp(ac, "OPERS_AHORA"))
					sockwrite(sck, 0x0, "%i", statserv->operadores.no);
				else if (!strcmp(ac, "OPERS_MAX"))
					sockwrite(sck, 0x0, "%i", statserv->operadores.max);
				else if (!strcmp(ac, "OPERS_HOY"))
					sockwrite(sck, 0x0, "%i", statserv->operadores.hoy);	
				else if (!strcmp(ac, "OPERS_SEMANA"))
					sockwrite(sck, 0x0, "%i", statserv->operadores.semana);
				else if (!strcmp(ac, "OPERS_MES"))
					sockwrite(sck, 0x0, "%i", statserv->operadores.mes);
				else if (!strcmp(ac, "OPERS_TIME"))
				{
					tm = statserv->operadores.max_time;
					sockwrite(sck, 0x0, "%s",  _asctime(&tm));
				}
				else if (!strcmp(ac, "SERVERS"))
				{
					char *pc, *qc, *wc, *yc, *rc;
					pc = strchr(strchr(bc, '!'), '$');
					*pc++ = '\0';
					qc = strchr(++pc, ')');
					*qc++ = '\0';
					rc = qc;
					if (!strcmp(pc, "FIN"))
					{
						int i;
						for (i = 0; i < 256; i++)
						{
							if (statserv->servidor[i].serv)
							{
								yc = wc = strdup(bc);
								while ((pc = strchr(wc, '$')))
								{
									*pc++ = '\0';
									sockwrite(sck, 0x0, "%s", wc);
									qc = strchr(++pc, ')');
									*qc++ = '\0';
									if (!strcmp(pc, "SERVER_NOMBRE"))
										sockwrite(sck, 0x0, "%s", statserv->servidor[i].serv->nombre);
									else if (!strcmp(pc, "SERVER_USERS"))
										sockwrite(sck, 0x0, "%i", statserv->servidor[i].usuarios);
									else if (!strcmp(pc, "SERVER_OPERS"))
										sockwrite(sck, 0x0, "%i", statserv->servidor[i].operadores);
									else if (!strcmp(pc, "SERVER_MAX"))
										sockwrite(sck, 0x0, "%i", statserv->servidor[i].max_users);
									else if (!strcmp(pc, "SERVER_LAG"))
										sockwrite(sck, 0x0, "%i", statserv->servidor[i].lag);
									else if (!strcmp(pc, "SERVER_VERSION"))
										sockwrite(sck, 0x0, "%s", statserv->servidor[i].version);
									else if (!strcmp(pc, "SERVER_UPTIME"))
									{
										u_int dura = statserv->servidor[i].uptime + (time(0) - ini);
										sockwrite(sck, 0x0, "%i días, %02i:%02i:%02i", dura / 86400, (dura / 3600) % 24, (dura / 60) % 60, dura % 60);
									}
									wc = qc;
								}
								sockwrite(sck, OPT_CRLF, "%s", qc);
								Free(yc);
							}
						}
					}
					bc = rc;
				}
				else if (!strcmp(ac, "CLIENTES"))
				{
					char *pc, *qc, *wc, *yc, *rc;
					pc = strchr(strchr(bc, '!'), '$');
					*pc++ = '\0';
					qc = strchr(++pc, ')');
					*qc++ = '\0';
					rc = qc;
					if (!strcmp(pc, "FIN"))
					{
						int i;
						for (i = 0; statserv->clientes[i].cliente; i++)
						{
							if (statserv->clientes[i].usuarios)
							{
								yc = wc = strdup(bc);
								while ((pc = strchr(wc, '$')))
								{
									*pc++ = '\0';
									sockwrite(sck, 0x0, "%s", wc);
									qc = strchr(++pc, ')');
									*qc++ = '\0';
									if (!strcmp(pc, "CLIENTE_NOMBRE"))
										sockwrite(sck, 0x0, "%s", statserv->clientes[i].cliente);
									else if (!strcmp(pc, "CLIENTE_USERS"))
										sockwrite(sck, 0x0, "%i", statserv->clientes[i].usuarios);
									wc = qc;
								}
								sockwrite(sck, OPT_CRLF, "%s", qc);
								Free(yc);
							}
						}
					}
					bc = rc;
				}
				else if (!strcmp(ac, "DOMINUMS"))
				{
					char *pc, *qc, *wc, *yc, *rc;
					pc = strchr(strchr(bc, '!'), '$');
					*pc++ = '\0';
					qc = strchr(++pc, ')');
					*qc++ = '\0';
					rc = qc;
					if (!strcmp(pc, "FIN"))
					{
						int i;
						for (i = 0; i < DOMINUMS; i++)
						{
							if (statserv->dominios[i].usuarios)
							{
								yc = wc = strdup(bc);
								while ((pc = strchr(wc, '$')))
								{
									*pc++ = '\0';
									sockwrite(sck, 0x0, "%s", wc);
									qc = strchr(++pc, ')');
									*qc++ = '\0';
									if (!strcmp(pc, "DOMINUMS_DOM"))
										sockwrite(sck, 0x0, "%s", statserv->dominios[i].dominio);
									else if (!strcmp(pc, "DOMINUMS_LUGAR"))
										sockwrite(sck, 0x0, "%s", statserv->dominios[i].donde);
									else if (!strcmp(pc, "DOMINUMS_USERS"))
										sockwrite(sck, 0x0, "%i", statserv->dominios[i].usuarios);
									wc = qc;
								}
								sockwrite(sck, OPT_CRLF, "%s", qc);
								Free(yc);
							}
						}
					}
					bc = rc;
				}
				else if (!strcmp(ac, "CARGA"))
					sockwrite(sck, 0x0, "%.3f", abs(microtime() - tb));
				td = bc;
			}
			sockwrite(sck, OPT_CRLF, "%s", bc);
			Free(fd);
		}
	}
	return 0;
}
int statserv_sig_bot()
{
	Cliente *bl;
	bl = botnick(statserv->nick, statserv->ident, statserv->host, me.nombre, statserv->modos, statserv->realname);
	statserv->cl = bl;
	return 0;
}
int statserv_sig_eos()
{
	char *canal;
	if (statserv->residente)
	{
		strcpy(tokbuf, statserv->residente);
		for (canal = strtok(tokbuf, ","); canal; canal = strtok(NULL, ","))
			mete_bot_en_canal(statserv->cl, canal);
	}
	return 0;
}
