#include <time.h>
#include <math.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/gameserv.h"
#include "modulos/gameserv/bidle.h"
#include "modulos/nickserv.h"

Bidle *bidle = NULL;
Timer *timerbidlecheck = NULL;
time_t ultime;

int BidleSynch();
int BidleSQL();
int BidlePMsg(Cliente *, Cliente *, char *, int);
int BidleJoin(Cliente *, Canal *);
int BidleDrop(char *);
int BidlePart(Cliente *, Canal *, char *);
int BidlePreNick(Cliente *, char *);
int BidlePostNick(Cliente *, int);
int BidleQuit(Cliente *, char *);
int BidleKick(Cliente *, Canal *, char *);
int BidleCMsg(Cliente *, Canal *, char *);
int BPen(Cliente *, Penas, char *);
int BDato(char *, char *);
int BidleComprueba();
int BidleHog();
int BidleBatallaEqs();
int BidleCalamidades();
int BidleRafagas();
int BidleLuz();
int BidleOscuridad();
char *BDura(u_int);
int BidleSum(char *, int);

Opts pens[] = {
	{ 30 , "pen_nick" } ,
	{ 200 , "pen_part" } ,
	{ 20 , "pen_quit" } ,
	{ 20, "pen_logout" } ,
	{ 250 , "pen_kick" } ,
	{ 0 , "pen_msg" } ,
	{ 15 , "pen_quest" } 
};

int BidleParseaConf(Conf *cnf)
{
	int p;
	if (!bidle)
		bidle = BMalloc(Bidle);
	bidle->base = 600;
	bidle->paso = 1.16;
	bidle->paso_pen = 1.14;
	bidle->eventos = 5;
	for (p = 0; p < cnf->secciones; p++)
	{
		if (!strcmp(cnf->seccion[p]->item, "nick"))
			ircstrdup(bidle->nick, cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "base"))	
			bidle->base = atoi(cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "paso"))
			bidle->paso = atof(cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "paso_pen"))
			bidle->paso_pen = atof(cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "voz"))	
			bidle->voz = 1;
		else if (!strcmp(cnf->seccion[p]->item, "nocodes"))	
			bidle->nocodes = 1;
		else if (!strcmp(cnf->seccion[p]->item, "eventos"))	
			bidle->eventos = atoi(cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "limit_pen"))	
			bidle->limit_pen = atoi(cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "canal"))
			ircstrdup(bidle->canal, cnf->seccion[p]->data);
		else if (!strcmp(cnf->seccion[p]->item, "topic"))
			ircstrdup(bidle->topic, cnf->seccion[p]->data);
	}
	InsertaSenyal(SIGN_SYNCH, BidleSynch);
	InsertaSenyal(SIGN_SQL, BidleSQL);
	InsertaSenyal(SIGN_PMSG, BidlePMsg);
	InsertaSenyal(SIGN_JOIN, BidleJoin);
	InsertaSenyal(SIGN_PART, BidlePart);
	InsertaSenyal(SIGN_PRE_NICK, BidlePreNick);
	InsertaSenyal(SIGN_POST_NICK, BidlePostNick);
	InsertaSenyal(SIGN_QUIT, BidleQuit);
	InsertaSenyal(SIGN_KICK, BidleKick);
	InsertaSenyal(SIGN_CMSG, BidleCMsg);
	InsertaSenyal(NS_SIGN_DROP, BidleDrop);
	InsertaSenyal(SIGN_SOCKCLOSE, BidleSockClose);
	return 0;
}
int BidleSynch()
{
	if ((bidle->cl = CreaBot(bidle->nick ? bidle->nick : "Bidle", gameserv->hmod->ident, gameserv->hmod->host, "kBq", "El RPG de idle")))
	{
		bidle->cn = EntraBot(bidle->cl, bidle->canal);
		ircsprintf(buf, "Canal de juego de bidle. Para jugar, /msg %s ALTA. Más información /msg %s HELP", bidle->nick, bidle->nick);
		ProtFunc(P_TOPIC)(bidle->cl, bidle->cn, bidle->topic ? bidle->topic : buf);
		if (bidle->voz)
			ProtFunc(P_MODO_CANAL)(bidle->cl, bidle->cn, "+ntm");
		else
			ProtFunc(P_MODO_CANAL)(bidle->cl, bidle->cn, "+nt");
		timerbidlecheck = IniciaCrono(0, bidle->eventos, BidleComprueba, NULL);
		ultime = time(0);
	}
	return 0;
}
int BidleSQL()
{
	if (!SQLEsTabla(GS_BIDLE))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"item varchar(255), "
			"nombre varchar(255), "
			"grupo varchar(255), "
			"lado varchar(32), "
			"admin int, "
			"nivel int, "
			"online int, "
			"idle int, "
			"sig int, "
			"x int, y int, "
			"pen_msg int, pen_nick int, pen_part int, pen_kick int, pen_quit int, pen_quest int, pen_logout int, "
			"creado int, "
			"lastlogin int, "
			"amuleto varchar(32), "
			"coraza varchar(32), "
			"casco varchar(32), "
			"botas varchar(32), "
			"guantes varchar(32), "
			"anillo varchar(32), "
			"pantalones varchar(32), "
			"escudo varchar(32), "
			"tunica varchar(32), "
			"arma varchar(32), "
			"PRIMARY KEY item (item) "
			");", PREFIJO, GS_BIDLE);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, GS_BIDLE);
	}
	SQLCargaTablas();
	return 0;
}
int BidleDescarga()
{
	ircfree(bidle->nick);
	DesconectaBot(bidle->cl, "El juego de idle");
	bidle->cl = NULL;
	bidle->cn = NULL;
	BorraSenyal(SIGN_SYNCH, BidleSynch);
	BorraSenyal(SIGN_SQL, BidleSQL);
	BorraSenyal(SIGN_PMSG, BidlePMsg);
	BorraSenyal(SIGN_JOIN, BidleJoin);
	BorraSenyal(SIGN_PART, BidlePart);
	BorraSenyal(SIGN_PRE_NICK, BidlePreNick);
	BorraSenyal(SIGN_POST_NICK, BidlePostNick);
	BorraSenyal(SIGN_QUIT, BidleQuit);
	BorraSenyal(SIGN_KICK, BidleKick);
	BorraSenyal(SIGN_CMSG, BidleCMsg);
	BorraSenyal(NS_SIGN_DROP, BidleDrop);
	BorraSenyal(SIGN_SOCKCLOSE, BidleSockClose);
	return 0;
}
char *BDura(u_int segs)
{
	Duracion d;
	static char tmp[64];
	tmp[0] = '\0';
	MideDuracionEx(segs, &d);
	if (d.sems*7+d.dias)
	{
		ircsprintf(buf, "%u días ", d.sems*7+d.dias);
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.horas)
	{
		ircsprintf(buf, "%u horas ", d.horas);
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.mins)
	{
		ircsprintf(buf, "%u minutos ", d.mins);
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.segs)
	{
		ircsprintf(buf, "%u segundos ", d.segs);
		strlcat(tmp, buf, sizeof(tmp));
	}
	tmp[strlen(tmp)-1] = '\0';
	return tmp;
}
int BDato(char *item, char *campo)
{
	char *cres;
	if ((cres = SQLCogeRegistro(GS_BIDLE, item, campo)))
		return atoi(cres);
	return -1;
}
int BCMsg(char *msg, ...)
{
	if (bidle->cn && protocolo->eos)
	{
		va_list vl;
		va_start(vl, msg);
		ProtFunc(P_MSG_VL)((Cliente *)bidle->cn, bidle->cl, 1, msg, &vl);
		va_end(vl);
	}
	return 0;
}
int BidlePMsg(Cliente *cl, Cliente *bl, char *msg, int resp)
{
	if (bl == bidle->cl)
	{
		char msgcpy[BUFSIZE], *com;
		strlcpy(msgcpy, msg, sizeof(msgcpy));
		if (!IsId(cl))
		{
			Responde(cl, bl, GS_ERR_EMPT, "Para jugar necesitas tener el nick registrado e identificado.");
			return 1;
		}
		if ((com = strtok(msgcpy, " ")))
		{
			if (!bidle->cn || !EsLinkCanal(cl->canal, bidle->cn))
			{
				ircsprintf(buf, "Para poder usar el bot y jugar necesitas estar en %s", bidle->canal);
				Responde(cl, bl, GS_ERR_EMPT, buf);
				return 1;
			}
			if (!strcasecmp(com, "ALTA"))
			{
				char *nombre = strchr(msg, ' ');
				if (SQLCogeRegistro(GS_BIDLE, cl->nombre, NULL))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Ya estás dado de alta.");
					return 1;
				}
				if (nombre)
					nombre++;
				if (BadPtr(nombre))
				{
					Responde(cl, bl, GS_ERR_PARA, "ALTA", "descripcion_personaje");
					return 1;
				}
				if (bidle->nocodes && strpbrk(nombre, "\1\3\15\31\2\22"))
				{
					Responde(cl, bl, GS_ERR_SNTX, "La descripción del personaje no puede contener colores, negrita, etc.");
					return 1;
				}
				SQLQuery("INSERT INTO %s%s (item, nombre, lado, admin, nivel, online, idle, sig, x, y, "
					"pen_msg, pen_nick, pen_part, pen_kick, pen_quit, pen_quest, pen_logout, creado, "
					"lastlogin, amuleto, coraza, casco, botas, guantes, anillo, pantalones, escudo, tunica, arma) VALUES "
					"('%s','%s','indefinido',%i,1,1,0,%i,0,0,0,0,0,0,0,0,0,%lu,%lu,0,0,0,0,0,0,0,0,0,0)",
					PREFIJO, GS_BIDLE, cl->nombre, nombre, IsAdmin(cl) ? 1 : 0, bidle->base, time(NULL), time(NULL));
				Responde(cl, bl, "Ok, has sido dado de alta. Ahora mismo estás logueado.");
				Responde(cl, bl, "Subirás de nivel dentro de %s", BDura(bidle->base));
				BCMsg("En una muestra de misericordia, los Dioses han traído al mundo a %s, %s\xF. Bienvenido sea.", cl->nombre, nombre);
				if (bidle->voz)
					ProtFunc(P_MODO_CANAL)(bidle->cl, bidle->cn, "+v %s", cl->nombre);
			}
			else if (!strcasecmp(com, "BAJA"))
			{
				char *user = strtok(NULL, " ");
				if (!BadPtr(user) && !BDato(cl->nombre, "admin"))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				if (!BadPtr(user) && BDato(user, "admin"))
				{
					Responde(cl, bl, GS_ERR_EMPT, "No puedes eliminar a otro administrador.");
					return 1;
				}
				if (BadPtr(user))
					user = cl->nombre;
				BCMsg("Los Dioses, en un arrebato de ira, quitan la vida a %s, %s\xF. Que descanse en paz.", user, SQLCogeRegistro(GS_BIDLE, user, "nombre"));
				if (user != cl->nombre && BDato(user, "online"))
					Responde(BuscaCliente(user), bl, "%s te ha dado de baja.", cl->nombre);
				SQLBorra(GS_BIDLE, user);
				Responde(cl, bl, user == cl->nombre ? "Has sido dado de baja." : "%s ha sido dado de baja.", user);
			}
			else if (!strcasecmp(com, "LOGOUT"))
			{
				if (!SQLCogeRegistro(GS_BIDLE, cl->nombre, NULL))
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás dado de alta.");
					return 1;
				}
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
				BPen(cl, PEN_LOGOUT, NULL);
			}
			else if (!strcasecmp(com, "ADMIN"))
			{
				char *user = strtok(NULL, " ");
				if (!BDato(cl->nombre, "admin"))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				if (BadPtr(user))
				{
					Responde(cl, bl, GS_ERR_PARA, "ADMIN", "nick");
					return 1;
				}
				if (!SQLCogeRegistro(GS_BIDLE, user, NULL))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Este nick no está dado de alta.");
					return 1;
				}
				if (BDato(user, "admin"))
				{
					SQLInserta(GS_BIDLE, user, "admin", "0");
					Responde(cl, bl, "\00312%s\003 ha sido eliminado como admin del juego.", user);
				}
				else
				{
					SQLInserta(GS_BIDLE, user, "admin", "1");
					Responde(cl, bl, "\00312%s\003 ha sido añadido como admin del juego.", user);
				}
			}
			else if (!strcasecmp(com, "HOG"))
			{
				if (!BDato(cl->nombre, "admin"))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				BidleHog();
				BCMsg("%s lanza una Mano de Dios.", cl->nombre);
				Responde(cl, bl, "Mano de Dios ejecutada.");
			}
			else if (!strcasecmp(com, "PUSH"))
			{
				char *user, *inc;
				int val;
				if (!BDato(cl->nombre, "admin"))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				if (!(user = strtok(NULL, " ")))
				{
					Responde(cl, bl, GS_ERR_PARA, "PUSH", "nick segundos");
					return 1;
				}
				if (!(inc = strtok(NULL, " ")))
				{
					Responde(cl, bl, GS_ERR_PARA, "PUSH", "nick segundos");
					return 1;
				}
				if (!SQLCogeRegistro(GS_BIDLE, user, NULL))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Este nick no está dado de alta.");
					return 1;
				}
				if (!(val = atoi(inc)))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Los segundos no pueden ser 0.");
					return 1;
				}
				SQLInserta(GS_BIDLE, user, "sig", "%li", BDato(user, "sig")+val);
				BCMsg("%s %s %s a %s.", cl->nombre, val < 0 ? "resta" : "añade", BDura(val), user);
			}
			else if (!strcasecmp(com, "INFO"))
			{
				SQLRes res;
				SQLRow row;
				time_t t;
				char *user = strtok(NULL, " ");
				if (BadPtr(user))
					user = cl->nombre;
				if (!(res = SQLQuery("SELECT * FROM %s%s WHERE item='%s'", PREFIJO, GS_BIDLE, user)))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Este nick no está dado de alta.");
					return 1;
				}
				row = SQLFetchRow(res);
				Responde(cl, bl, "%s, %s", user, row[1]);
				Responde(cl, bl, "Nivel: %s", row[5]);
				if (!BadPtr(row[2]))
					Responde(cl, bl, "Pertenece al grupo de %s", row[2]);
				if (*row[3] != 'i')
					Responde(cl, bl, "Es del lado de la %s", row[3]);
				Responde(cl, bl, "Estado: %s", * row[6] == '1' ? "online" : "offline");
				Responde(cl, bl, "Siguiente nivel en: %s", BDura(atoi(row[8])));
				Responde(cl, bl, "Idle acumulado: %s", BDura(atoi(row[7])));
				Responde(cl, bl, "Suma de los ítems: %i", BidleSum(row[0], 0));
				t = atoi(row[18]);
				Responde(cl, bl, "Nacido el: %s", Fecha(&t));
			}
			else
			{
				Responde(cl, bl, GS_ERR_EMPT, "Comando desconocido. Para más ayuda use el comando HELP");
				return 1;
			}
		}
	}
	return 0;
}
int BidleJoin(Cliente *cl, Canal *cn)
{
	if (cn == bidle->cn && IsId(cl))
	{
		char *nombre;
		if ((nombre = SQLCogeRegistro(GS_BIDLE, cl->nombre, "nombre")))
		{
			BCMsg("%s, %s\xF, se une al juego.", cl->nombre, nombre);
			if (bidle->voz)
				ProtFunc(P_MODO_CANAL)(bidle->cl, bidle->cn, "+v %s", cl->nombre);
			SQLInserta(GS_BIDLE, cl->nombre, "online", "1");
			SQLInserta(GS_BIDLE, cl->nombre, "lastlogin", "%lu", time(0));
			Responde(cl, bidle->cl, "Estás logueado. Bienvenido al juego.");
		}
	}
	return 0;
}
int BidlePart(Cliente *cl, Canal *cn, char *msg)
{
	if (cn == bidle->cn && IsId(cl) && BDato(cl->nombre, "online"))
	{
		BPen(cl, PEN_PART, NULL);
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	}
	return 0;
}
int BidlePreNick(Cliente *cl, char *nuevo)
{
	if (IsId(cl) && BDato(cl->nombre, "online") && EsLinkCanal(cl->canal, bidle->cn) && BPen(cl, PEN_NICK, NULL)) // se desloguea
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidlePostNick(Cliente *cl, int cambio)
{
	if (IsId(cl) && cambio && BPen(cl, PEN_NICK, NULL)) // proviene de un cambio, lo penalizamos igual y logueamos
	{
		SQLInserta(GS_BIDLE, cl->nombre, "online", "1");
		SQLInserta(GS_BIDLE, cl->nombre, "lastlogin", "%lu", time(0));
	}
	return 0;
}
int BidleQuit(Cliente *cl, char *msg)
{
	if (IsId(cl) && BDato(cl->nombre, "online") && BPen(cl, PEN_QUIT, NULL))
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidleKick(Cliente *cl, Canal *cn, char *msg)
{
	if (IsId(cl) && cn == bidle->cn && BDato(cl->nombre, "online") && BPen(cl, PEN_KICK, NULL))
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidleCMsg(Cliente *cl, Canal *cn, char *msg)
{
	if (IsId(cl) && cn == bidle->cn && BDato(cl->nombre, "online"))
		BPen(cl, PEN_MSG, msg);
	return 0;
}
int BidleDrop(char *nick)
{
	SQLBorra(GS_BIDLE, nick);
	return 0;
}
int BPen(Cliente *cl, Penas pena, char *msg)
{
	int sig, nivel, pen;
	double inc;
	char *motivo;
	if ((nivel = BDato(cl->nombre, "nivel")) > 0)
	{
		if (pena == PEN_MSG)
			inc = strlen(msg)*pow(bidle->paso_pen, nivel);
		else
			inc = pens[pena].opt*pow(bidle->paso_pen, nivel);
		if (bidle->limit_pen)
			inc = MIN(bidle->limit_pen, inc);
		sig = BDato(cl->nombre, "sig") + (int)inc;
		pen = BDato(cl->nombre, pens[pena].item) + (int)inc;
		SQLInserta(GS_BIDLE, cl->nombre, "sig", "%li", sig);
		SQLInserta(GS_BIDLE, cl->nombre, pens[pena].item, "%li", pen);
		if (pena != PEN_QUIT)
		{
			if (pena == PEN_NICK)
				motivo = "cambiarte el nick";
			else if (pena == PEN_PART)
				motivo = "salir del canal";
			else if (pena == PEN_LOGOUT)
				motivo = "desloguearte";
			else if (pena == PEN_KICK)
				motivo = "haber sido expulsado";
			else if (pena == PEN_MSG)
				motivo = "hablar en el canal";
			else if (pena == PEN_QUEST)
				motivo = "no completar tu misión";
			Responde(cl, bidle->cl, "Has sido penalizado con \00312%s\003 de más por %s.", BDura(inc), motivo);
		}
		return 1;
	}
	return 0;
}
int BAlea(u_int max)
{
	return Aleatorio(0, max-1);
}
int BidleSum(char *user, int batalla)
{
	SQLRes res;
	SQLRow row;
	int sum = 0, i;
	if (!strcmp(bidle->nick, user))
	{
		if ((res = SQLQuery("SELECT * FROM %s%s", PREFIJO, GS_BIDLE)))
		{
			while ((row = SQLFetchRow(res)))
				sum = MAX(sum, BidleSum(user, 0));
			SQLFreeRes(res);
		}
		return sum+1;
	}
	else if ((res = SQLQuery("SELECT * FROM %s%s WHERE item='%s'", PREFIJO, GS_BIDLE, user)))
	{
		row = SQLFetchRow(res);
		for (i = 20; i < 30; i++)
			sum += atoi(row[i]);
		if (batalla)
		{
			if (*row[3] == 'L')
				return (int)1.1*sum;
			else if (*row[3] == 'O')
				return (int)0.9*sum;
			else
				return sum;
		}
		else
			return sum;
		SQLFreeRes(res);
	}
	return -1;
}
int BidleComprueba()
{
	time_t curtime;
	SQLRes res;
	SQLRow row;
	int val, onlines = 0;
	if (ultime)
	{
		curtime = time(0);
		if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
		{
			onlines = SQLNumRows(res);
			while ((row = SQLFetchRow(res)))
			{
				if ((val = atoi(row[8])-(curtime - ultime)) < 0) // cambio de nivel!
				{
					val = atoi(row[5])+1;
					SQLInserta(GS_BIDLE, row[0], "nivel", "%li", val);
					if (val > 60)
						val = bidle->base * pow(bidle->paso, 60) + (86400 * (val - 60));
					else
						val = bidle->base * pow(bidle->paso, val);
					SQLInserta(GS_BIDLE, row[0], "sig", "%li", val);
					BCMsg("%s, %s\xF, ha alcanzado el nivel %u! Próximo nivel en %s.", row[0], row[1], atoi(row[5])+1, BDura(val));
				}
				else
					SQLInserta(GS_BIDLE, row[0], "sig", "%li", val);
				val = atoi(row[7]) + (curtime - ultime);
				SQLInserta(GS_BIDLE, row[0], "idle", "%lu", val);
			}
			SQLFreeRes(res);
		}
		ultime = curtime;
	}
	if (BAlea((20*86400)/bidle->eventos) < onlines)
		BidleHog();
	if (BAlea((24*86400)/bidle->eventos) < onlines)
		BidleBatallaEqs();
	if (BAlea((8*86400)/bidle->eventos) < onlines)
		BidleCalamidades();
	if (BAlea((4*86400)/bidle->eventos) < onlines)
		BidleRafagas();
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Luz'", PREFIJO, GS_BIDLE)))
	{
		if (BAlea((12*86400)/bidle->eventos) < SQLNumRows(res))
			BidleLuz();
		SQLFreeRes(res);
	}
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Oscuridad'", PREFIJO, GS_BIDLE)))
	{
		if (BAlea((8*86400)/bidle->eventos) < SQLNumRows(res))
			BidleOscuridad();
		SQLFreeRes(res);
	}
	return 0;
}
int BidleSockClose()
{
	ApagaCrono(timerbidlecheck);
	timerbidlecheck = NULL;
	SQLQuery("UPDATE %s%s SET online=0", PREFIJO, GS_BIDLE);
	ultime = 0;
	return 0;
}
int BidleHog()
{
	SQLRes res;
	SQLRow row;
	int val;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
	{
		SQLSeek(res, BAlea(SQLNumRows(res)));
		row = SQLFetchRow(res);
		val = (5+BAlea(71))*atoi(row[8])/100;
		if (BAlea(5))
		{
			BCMsg("Los cielos han estallado y la Mano de Dios ha bendecido a %s con %s hacia el nivel %u.", row[0], BDura(val), atoi(row[5])+1);
			val = atoi(row[8])-val;
		}
		else
		{
			BCMsg("Los cielos rojizos han ardido y el fuego ha consumido a %s retrasándolo %s hacia el nivel %u.", row[0], BDura(val), atoi(row[5])+1);
			val = atoi(row[8])+val;
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleBatallaEqs()
{
	SQLRes res;
	SQLRow row[6];
	int i, j, rows, misum, opsum, inc, mirol, oprol;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
	{
		if ((rows = SQLNumRows(res)) > 6)
		{
			for (i = 0; i < 6; i++)
			{
				SQLSeek(res, BAlea(rows));
				row[i] = SQLFetchRow(res);
				for (j = 0; j < i; j++)
				{
					if (row[i] == row[j])
						break;
				}
				if (j < i) // ha encontrado coincidencia
					i--;
			}
			misum = BidleSum(row[0][0], 1) + BidleSum(row[1][0], 1) + BidleSum(row[2][0], 1);
			opsum = BidleSum(row[3][0], 1) + BidleSum(row[4][0], 1) + BidleSum(row[5][0], 1);
			inc = atoi(row[0][8]);
			inc = MIN(inc, atoi(row[1][8]));
			inc = MIN(inc, atoi(row[2][8]));
			inc = (int)inc*0.2;
			mirol = BAlea(misum);
			oprol = BAlea(opsum);
			if (mirol >= oprol)
			{
				BCMsg("%s, %s y %s [%i/%i] se han batido en duelo con %s, %s y %s [%i/%i] y han vencido! Se han descontado %s de sus relojes.",
					row[0][0], row[1][0], row[2][0], mirol, misum,
					row[3][0], row[4][0], row[5][0], oprol, opsum,
					BDura(inc));
				SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", atoi(row[0][8])-inc);
				SQLInserta(GS_BIDLE, row[1][0], "sig", "%li", atoi(row[1][8])-inc);
				SQLInserta(GS_BIDLE, row[2][0], "sig", "%li", atoi(row[2][8])-inc);
			}
			else
			{
				BCMsg("%s, %s y %s [%i/%i] se han batido en duelo con %s, %s y %s [%i/%i] y han perdido! Se han añadido %s a sus relojes.",
					row[0][0], row[1][0], row[2][0], mirol, misum,
					row[3][0], row[4][0], row[5][0], oprol, opsum,
					BDura(inc));
				SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", atoi(row[0][8])+inc);
				SQLInserta(GS_BIDLE, row[1][0], "sig", "%li", atoi(row[1][8])+inc);
				SQLInserta(GS_BIDLE, row[2][0], "sig", "%li", atoi(row[2][8])+inc);
			}
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleCalamidades()
{
	SQLRes res;
	SQLRow row;
	int val, inc;
	char *items[] = { "amuleto", "coraza", "casco", "botas", "guantes", "anillo", "pantalones", "escudo", "tunica", "arma" };
	char *plrs[] = { "el" , "la" , "el" , "las" , "los" , "el" , "los" , "el" , "la" , "la" };
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
	{
		SQLSeek(res, BAlea(SQLNumRows(res)));
		row = SQLFetchRow(res);
		if (!BAlea(10))
		{
			val = BAlea(10);
			BCMsg("A los Dioses no les ha gustado %s %s de %s y ahora están malditos. Han perdido un 10%% de su efectividad.", plrs[val], items[val], row[0]);
			inc = (int)(atoi(row[20+val])*0.9);
			SQLInserta(GS_BIDLE, row[0], items[val], "%li%c", IsDigit(row[20+val][strlen(row[20+val]-1)]) ? '\0' : row[20+val][strlen(row[20+val]-1)]);
		}
		else
		{
			inc = (5+BAlea(8))*atoi(row[8])/100;
			BCMsg("%s ha sufrido una calamidad! Se han añadido %s a su reloj.", row[0], BDura(inc));
			SQLInserta(GS_BIDLE, row[0], "sig", "%li", atoi(row[8])+inc);
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleRafagas()
{
	SQLRes res;
	SQLRow row;
	int val, inc;
	char *items[] = { "amuleto", "coraza", "casco", "botas", "guantes", "anillo", "pantalones", "escudo", "tunica", "arma" };
	char *plrs[] = { "su" , "su" , "su" , "sus" , "sus" , "su" , "sus" , "su" , "su" , "su" };
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
	{
		SQLSeek(res, BAlea(SQLNumRows(res)));
		row = SQLFetchRow(res);
		if (!BAlea(10))
		{
			val = BAlea(10);
			BCMsg("%s ha interceptado una ráfaga divina con %s %s! Ha aumentado su efectividad en un 10%%.", row[0], plrs[val], items[val]);
			inc = (int)(atoi(row[20+val])*0.9);
			SQLInserta(GS_BIDLE, row[0], items[val], "%li%c", IsDigit(row[20+val][strlen(row[20+val]-1)]) ? '\0' : row[20+val][strlen(row[20+val]-1)]);
		}
		else
		{
			inc = (5+BAlea(8))*atoi(row[8])/100;
			BCMsg("%s ha recibido una ráfaga divina! Se han descontado %s de su reloj.", row[0], BDura(inc));
			SQLInserta(GS_BIDLE, row[0], "sig", "%li", atoi(row[8])-inc);
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleLuz()
{
	SQLRes res;
	SQLRow row[2];
	int i, rows, inc;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Luz'", PREFIJO, GS_BIDLE)))
	{
		if ((rows = SQLNumRows(res)) > 2)
		{
			SQLSeek(res, BAlea(rows));
			row[0] = SQLFetchRow(res);
			do { SQLSeek(res, BAlea(rows)); row[1] = SQLFetchRow(res); }while(row[1] == row[0]);
			inc = 5 + BAlea(8);
			BCMsg("%s y %s rechazaron el veneno de la Oscuridad. Juntos han rezado y la Luz celestial les ha iluminado. Se han descontado el %i%% de sus relojes.", row[0][0], row[1][0], inc);
			SQLInserta(GS_BIDLE, row[0][0], "sig", "%i", atoi(row[0][8])*(1-inc/100));
			SQLInserta(GS_BIDLE, row[1][0], "sig", "%i", atoi(row[1][8])*(1-inc/100));
			BCMsg("%s subirá de nivel en %s.", row[0][0], BDura(atoi(row[0][8])*(1-inc/100)));
			BCMsg("%s subirá de nivel en %s.", row[1][0], BDura(atoi(row[1][8])*(1-inc/100)));
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleOscuridad()
{
	SQLRes res[2];
	SQLRow row[2];
	int val, inc;
	char *t;
	if ((res[0] = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Oscuridad'", PREFIJO, GS_BIDLE)))
	{
		row[0] = SQLFetchRow(res[0]);
		if (BAlea(2) < 1)
		{
			if ((res[1] = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Luz'", PREFIJO, GS_BIDLE)))
			{
				char *items[] = { "amuleto", "coraza", "casco", "botas", "guantes", "anillo", "pantalones", "escudo", "tunica", "arma" };
				char *plrs[] = { "el" , "la" , "el" , "las" , "los" , "el" , "los" , "el" , "la" , "la" };
				char *plrs2[] = { "su" , "su" , "su" , "sus" , "sus" , "su" , "sus" , "su" , "su" , "su" };
				row[1] = SQLFetchRow(res[1]);
				val = BAlea(10);
				if (atoi(row[1][20+val]) > atoi(row[0][20+val]))
				{
					SQLInserta(GS_BIDLE, row[0][0], items[val], row[1][20+val]);
					SQLInserta(GS_BIDLE, row[1][0], items[val], row[0][20+val]);
					BCMsg("%s roba %s %s nivel %s de %s mientras dormía! Mientras aprovecha para dejarle %s %s nivel %s.", 
						row[0][0], plrs[val], items[val], row[1][20+val], row[1][0],
						plrs2[val], items[val], row[0][20+val]);
				}
				else
				{
					BCMsg("%s ha intentado robar %s %s de %s. Pero se ha dado cuenta de que es de menor nivel y lo arroja a las tinieblas.",
						plrs[val], items[val], row[1][0]);
				}
				SQLFreeRes(res[1]);
			}
			else
				goto oscur;
		}
		else
		{
			oscur:
			inc = 1 + BAlea(5);
			val = atoi(row[0][8]);
			t = strdup(BDura(val*(1+inc/100)));
			BCMsg("%s está maldecido por los Señores de las Tinieblas. Se han añadido %s a su reloj. Subirá de nivel en %s.", row[0][0], BDura(val*inc/100), t);
			SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", val*(1+inc/100));
			Free(t);
		}
		SQLFreeRes(res[0]);
	}
	return 0;
}
