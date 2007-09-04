#include <time.h>
#include <math.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "httpd.h"
#include "modulos/gameserv.h"
#include "modulos/gameserv/bidle.h"
#include "modulos/nickserv.h"

Bidle *bidle = NULL;
Timer *timerbidlecheck = NULL;
time_t ultime;
char *items[] = { "amuleto", "coraza", "casco", "botas", "guantes", "anillo", "pantalones", "escudo", "tunica", "arma" };
char *plrs[] = { "el" , "la" , "el" , "las" , "los" , "el" , "los" , "el" , "la" , "la" };
char *plrs2[] = { "su" , "su" , "su" , "sus" , "sus" , "su" , "sus" , "su" , "su" , "su" };
char *plrs3[] = { "un" , "una", "un", "unas", "unos", "un", "unos", "un", "una", "un" };
char *plrs4[] = { "tu", "tu", "tu", "tus", "tus", "tu", "tus", "tu", "tu", "tu" };
char *ver = "0.9";
HDir *bhdir = NULL;

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
long BDato(char *, char *);
int BidleComprueba();
int BidleHog();
int BidleBatallaEqs();
int BidleCalamidades();
int BidleRafagas();
int BidleLuz();
int BidleOscuridad();
char *BDura(u_int);
int BidleSum(char *, int);
int BidleMueve();
int BidleEncuentraItem(SQLRow);
int BidleReta(SQLRow, SQLRow);
int BidleQuest();
int BidleHaceQuest(char *);
HDIRFUNC(BidleLeeHDir);

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
	int i;
	if (!bidle)
		bidle = BMalloc(Bidle);
	bidle->base = 600;
	bidle->paso = 1.16;
	bidle->paso_pen = 1.14;
	bidle->eventos = 5;
	bidle->maxx = bidle->maxy = 500;
	bidle->paso_vende = 0.8;
	bidle->paso_compra = 1.2;
	ircstrdup(bidle->nick, "bidle");
	for (i = 0; i < cnf->secciones; i++)
	{
		if (!strcmp(cnf->seccion[i]->item, "nick"))
			ircstrdup(bidle->nick, cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "base"))	
			bidle->base = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "paso"))
			bidle->paso = atof(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "paso_pen"))
			bidle->paso_pen = atof(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "voz"))	
			bidle->voz = 1;
		else if (!strcmp(cnf->seccion[i]->item, "nocodes"))	
			bidle->nocodes = 1;
		else if (!strcmp(cnf->seccion[i]->item, "eventos"))	
			bidle->eventos = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "limit_pen"))	
			bidle->limit_pen = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "canal"))
			ircstrdup(bidle->canal, cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "topic"))
			ircstrdup(bidle->topic, cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "max_x"))	
			bidle->maxx = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "max_y"))	
			bidle->maxy = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "max_top"))	
			bidle->maxtop = atoi(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "paso_vende"))
			bidle->paso_vende = atof(cnf->seccion[i]->data);
		else if (!strcmp(cnf->seccion[i]->item, "paso_compra"))
			bidle->paso_compra = atof(cnf->seccion[i]->data);
	}
	if ((60 % bidle->eventos))
		bidle->eventos = 5;
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
	bidle->quest.tiempo = time(0)+3600;
	bidle->pos = (struct BidlePos **)Malloc(sizeof(struct BidlePos *)*bidle->maxx);
	for (i = 0; i < bidle->maxx; i++)
	{
		bidle->pos[i] = (struct BidlePos *)Malloc(sizeof(struct BidlePos)*bidle->maxy);
		bzero(bidle->pos[i], sizeof(struct BidlePos)*bidle->maxy);
	}
	if (conf_httpd)
	{
		ircsprintf(buf, "/%s", bidle->nick);
		if (!(bhdir = CreaHDir(buf, BidleLeeHDir)))
			return 1;
	}
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
			"clan varchar(255), "
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
			"oro int, "
			"mejora int, "
			"claner int, "
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
	int i;
	if (conf_httpd)
		BorraHDir(bhdir);
	ircfree(bidle->nick);
	DesconectaBot(bidle->cl, "El juego de idle");
	bidle->cl = NULL;
	bidle->cn = NULL;
	ApagaCrono(timerbidlecheck);
	timerbidlecheck = NULL;
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
	for (i = 0; i < bidle->maxx; i++)
		Free(bidle->pos[i]);
	Free(bidle->pos);
	if (bidle->quest.tipo)
	{
		for (i = 0; i < 4; i++)
			ircfree(bidle->quest.user[i].user);
	}
	ircfree(bidle);
	return 0;
}
char *BDura(u_int segs)
{
	Duracion d;
	static char tmp[64];
	if (!segs)
		return "0 segundos";
	tmp[0] = '\0';
	MideDuracionEx(segs, &d);
	if (d.sems*7+d.dias)
	{
		ircsprintf(buf, "%u día%s ", d.sems*7+d.dias, (d.sems*7+d.dias > 1 ? "s" : ""));
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.horas)
	{
		ircsprintf(buf, "%u hora%s ", d.horas, (d.horas > 1 ? "s" : ""));
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.mins)
	{
		ircsprintf(buf, "%u minuto%s ", d.mins, (d.mins > 1 ? "s" : ""));
		strlcat(tmp, buf, sizeof(tmp));
	}
	if (d.segs)
	{
		ircsprintf(buf, "%u segundo%s ", d.segs, (d.segs > 1 ? "s" : ""));
		strlcat(tmp, buf, sizeof(tmp));
	}
	tmp[strlen(tmp)-1] = '\0';
	return tmp;
}
long BDato(char *item, char *campo)
{
	char *cres;
	if ((cres = SQLCogeRegistro(GS_BIDLE, item, campo)))
		return atol(cres);
	return -1;
}
int BCuenta()
{
	time_t t = time(0);
	struct tm *ttm = localtime(&t);
	return ttm->tm_sec+ttm->tm_min*60+ttm->tm_hour*3600;
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
					"lastlogin, amuleto, coraza, casco, botas, guantes, anillo, pantalones, escudo, tunica, arma, oro, mejora,claner) VALUES "
					"('%s','%s','Neutral',%i,1,1,0,%i,%i,%i,0,0,0,0,0,0,0,%lu,%lu,0,0,0,0,0,0,0,0,0,0,0,0,0)",
					PREFIJO, GS_BIDLE, cl->nombre, nombre, IsAdmin(cl) ? 1 : 0, bidle->base, BAlea(bidle->maxx), BAlea(bidle->maxy), time(NULL), time(NULL));
				Responde(cl, bl, "Ok, has sido dado de alta. Ahora mismo estás logueado.");
				Responde(cl, bl, "Subirás de nivel dentro de %s", BDura(bidle->base));
				BCMsg("En una muestra de misericordia, los Dioses han traído al mundo a %s, %s\xF. Bienvenido sea.", cl->nombre, nombre);
				if (bidle->voz)
					ProtFunc(P_MODO_CANAL)(bidle->cl, bidle->cn, "+v %s", cl->nombre);
			}
			else if (!strcasecmp(com, "BAJA"))
			{
				char *user = strtok(NULL, " ");
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (!BadPtr(user) && BDato(cl->nombre, "admin") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				if (!BadPtr(user) && BDato(user, "admin") == 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No puedes eliminar a otro administrador.");
					return 1;
				}
				if (BadPtr(user))
					user = cl->nombre;
				if (!SQLCogeRegistro(GS_BIDLE, user, NULL))
				{
					if (user == cl->nombre)
						Responde(cl, bl, GS_ERR_EMPT, "No estás dado de alta.");
					else
						Responde(cl, bl, GS_ERR_EMPT, "Este usuario no existe.");
					return 1;
				}
				BCMsg("Los Dioses, en un arrebato de ira, quitan la vida a %s, %s\xF. Que descanse en paz.", user, SQLCogeRegistro(GS_BIDLE, user, "nombre"));
				if (user != cl->nombre && BDato(user, "online") == 1)
					Responde(BuscaCliente(user), bl, "%s te ha dado de baja.", cl->nombre);
				SQLBorra(GS_BIDLE, user);
				Responde(cl, bl, user == cl->nombre ? "Has sido dado de baja." : "%s ha sido dado de baja.", user);
			}
			else if (!strcasecmp(com, "LOGOUT"))
			{
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
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BDato(cl->nombre, "admin") != 1)
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
				if (BDato(user, "admin") == 1)
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
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BDato(cl->nombre, "admin") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "Acceso denegado.");
					return 1;
				}
				BCMsg("%s lanza una Mano de Dios.", cl->nombre);
				BidleHog();
				Responde(cl, bl, "Mano de Dios ejecutada.");
			}
			else if (!strcasecmp(com, "PUSH"))
			{
				char *user, *inc;
				long val;
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BDato(cl->nombre, "admin") != 1)
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
				if (!(val = atol(inc)))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Los segundos no pueden ser 0.");
					return 1;
				}
				SQLInserta(GS_BIDLE, user, "sig", "%li", (long)(BDato(user, "sig")+val));
				BCMsg("%s %s %s a %s.", cl->nombre, val < 0 ? "resta" : "añade", BDura(abs(val)), user);
			}
			else if (!strcasecmp(com, "INFO"))
			{
				SQLRes res;
				SQLRow row;
				time_t t;
				char *user = strtok(NULL, " ");
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BadPtr(user))
					user = cl->nombre;
				if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s'", PREFIJO, GS_BIDLE, strtolower(user))))
				{
					Responde(cl, bl, GS_ERR_EMPT, "Este nick no está dado de alta.");
					return 1;
				}
				row = SQLFetchRow(res);
				Responde(cl, bl, "%s, %s", row[0], row[1]);
				Responde(cl, bl, "Nivel: %s", row[5]);
				if (!BadPtr(row[2]))
				{
					Responde(cl, bl, "Pertenece al clan %s", row[2]);
					if (atoi(row[32]) & B_FUND)
						Responde(cl, bl, "Es Patriarca");
					if (atoi(row[32]) & B_RECV)
						Responde(cl, bl, "Es Claner");
				}
				if (*row[3] != 'N')
					Responde(cl, bl, "Es del lado de la %s", row[3]);
				else
					Responde(cl, bl, "Es del lado Neutral");
				Responde(cl, bl, "Estado: %s", *row[6] == '1' ? "online" : "offline");
				Responde(cl, bl, "Siguiente nivel en: %s", BDura(atoi(row[8])));
				Responde(cl, bl, "Idle acumulado: %s", BDura(atoi(row[7])));
				Responde(cl, bl, "Posición: [%i,%i]", atoi(row[9]), atoi(row[10]));
				Responde(cl, bl, "Suma de los ítems: %i", BidleSum(row[0], 0));
				if (*row[31] != '0')
					Responde(cl, bl, "Mejoras: +%s", row[31]);
				if (*row[4] == '1' || user == cl->nombre || !strcasecmp(user, cl->nombre) || BDato(cl->nombre, "admin") == 1)
					Responde(cl, bl, "Oro: %s", row[30]);
				t = atol(row[18]);
				Responde(cl, bl, "Nacido el: %s", Fecha(&t));
			}
			else if (!strcasecmp(com, "LADO"))
			{
				char *lado = strtok(NULL, " ");
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BadPtr(lado))
				{
					Responde(cl, bl, GS_ERR_PARA, "LADO", "{Luz|Neutral|Oscuridad}");
					return 1;
				}
				if (strcasecmp(lado, "LUZ") && strcasecmp(lado, "NEUTRAL") && strcasecmp(lado, "OSCURIDAD"))
				{
					Responde(cl, bl, GS_ERR_SNTX, "LADO {Luz|Neutral|Oscuridad}");
					return 1;
				}
				if (ToLower(*lado) == 'l')
					SQLInserta(GS_BIDLE, cl->nombre, "lado", "Luz");
				else if (ToLower(*lado) == 'o')
					SQLInserta(GS_BIDLE, cl->nombre, "lado", "Oscuridad");
				else
					SQLInserta(GS_BIDLE, cl->nombre, "lado", "Neutral");
				Responde(cl, bl, "Lado cambiado a \00312%s", lado);
			}
			else if (!strcasecmp(com, "QUEST"))
			{
				if (!bidle->quest.tipo)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No existen misiones en este momento.");
					return 1;
				}
				if (bidle->quest.tipo == 1)
				{
					if (bidle->quest.user[3].user)
						Responde(cl, bl, "%s, %s, %s y %s deben terminar la misión en %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, BDura(bidle->quest.tiempo - time(0)));
					else if (bidle->quest.user[2].user)
						Responde(cl, bl, "%s, %s y %s deben terminar la misión en %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, BDura(bidle->quest.tiempo - time(0)));	
					else if (bidle->quest.user[1].user)
						Responde(cl, bl, "%s y %s deben terminar la misión en %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, BDura(bidle->quest.tiempo - time(0)));
				}
				else if (bidle->quest.tipo == 2)
				{
					if (bidle->quest.user[3].user)
						Responde(cl, bl, "%s, %s, %s y %s deben ir primero a [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
					else if (bidle->quest.user[2].user)
						Responde(cl, bl, "%s, %s y %s deben ir primero a [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
					else if (bidle->quest.user[1].user)
						Responde(cl, bl, "%s y %s deben ir primero a [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
				}
			}
			else if (!strcasecmp(com, "CLAN"))
			{
				char *acc = strtok(NULL, " "), *opt;
				SQLRes res;
				SQLRow row;
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BadPtr(acc))
				{
					Responde(cl, bl, GS_ERR_PARA, "CLAN", "{CREAR|ENTRAR|ACEPTAR|RECHAZAR|LISTAR|SALIR|CLANER} [opción]");
					return 1;
				}
				opt = strchr(msg, ' ')+1;
				if ((opt = strchr(opt, ' ')))
					opt++; 
				if (!strcasecmp(acc, "CREAR"))
				{
					char *clan;
					if (BadPtr(opt))
					{
						Responde(cl, bl, GS_ERR_PARA, "CLAN", "CREAR nombre_clan");
						return 1;
					}
					clan = SQLEscapa(strtolower(opt));
					if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(clan)='%s'", PREFIJO, GS_BIDLE, clan)))
					{
						SQLFreeRes(res);
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "Este clan ya existe. Si quieres entrar, ejecuta el comando CLAN ENTRAR nombre_clan.");
						return 1;
					}
					Free(clan);
					if (SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan"))
					{
						Responde(cl, bl, GS_ERR_EMPT, "Ya has ingresado o estás solicitando la entrada en un clan. Si quieres salir de este clan, ejecuta el comando CLAN SALIR nombre_clan");
						return 1;
					}
					SQLInserta(GS_BIDLE, cl->nombre, "clan", opt);
					SQLInserta(GS_BIDLE, cl->nombre, "claner", "%i", B_ISIN | B_FUND | B_RECV);
					Responde(cl, bl, "Se ha creado el clan \00312%s", opt);
					BCMsg("%s ha sido escogido por los Dioses para tejer la estirpe del clan %s.", cl->nombre, opt);
				}
				else if (!strcasecmp(acc, "ENTRAR"))
				{
					char *clan;
					if (BadPtr(opt))
					{
						Responde(cl, bl, GS_ERR_PARA, "CLAN", "ENTRAR nombre_clan");
						return 1;
					}
					if (SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan"))
					{
						Responde(cl, bl, GS_ERR_EMPT, "Ya has ingresado o estás solicitando la entrada en un clan. Si quieres salir de este clan, ejecuta el comando CLAN SALIR nombre_clan");
						return 1;
					}
					clan = SQLEscapa(strtolower(opt));
					if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(clan)='%s'", PREFIJO, GS_BIDLE, clan)))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "Este clan no existe. Si quieres crearlo, ejecuta el comando CLAN CREAR nombre_clan.");
						return 1;
					}
					Free(clan);
					row = SQLFetchRow(res);
					clan = strdup(row[2]);
					SQLFreeRes(res);
					SQLInserta(GS_BIDLE, cl->nombre, "clan", clan);
					SQLInserta(GS_BIDLE, cl->nombre, "claner", "%i", B_PEND);
					Responde(cl, bl, "Tu solicitud para ingresar al clan \00312%s\003 está pendiente de aprobación.", clan);
					if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(clan)='%s' AND claner >= %i", PREFIJO, GS_BIDLE, strtolower(clan), B_RECV)))
					{
						Cliente *al;
						row = SQLFetchRow(res);
						if ((al = BuscaCliente(row[0])))
							Responde(al, bl, "%s solicita entrar en el clan. Para aceptarlo, usa el comando CLAN ACEPTAR %s. Para rechazarlo, CLAN RECHAZAR %s.", cl->nombre, cl->nombre, cl->nombre);
						SQLFreeRes(res);
					}
					Free(clan);
				}
				else if (!strcasecmp(acc, "ACEPTAR"))
				{
					char *clan;
					Cliente *al;
					if (BadPtr(opt))
					{
						Responde(cl, bl, GS_ERR_PARA, "CLAN", "ACEPTAR nick");
						return 1;
					}
					if (!(clan = SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan")))
					{
						Responde(cl, bl, GS_ERR_EMPT, "No perteneces a ningún clan.");
						return 1;
					}
					clan = strdup(clan);
					if (!(BDato(cl->nombre, "claner") & B_RECV))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No tienes nivel para ACEPTAR.");
						return 1;
					}
					if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s' AND clan='%s' AND claner=%i", PREFIJO, GS_BIDLE, strtolower(opt), clan, B_PEND)))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "Este usuario no ha solicitado la entrada al clan.");
						return 1;
					}
					SQLFreeRes(res);
					SQLInserta(GS_BIDLE, opt, "claner", "%i", B_ISIN);
					Responde(cl, bl, "%s ha sido aceptado.", opt);
					BCMsg("Los Dioses son benevolentes y han unido a %s al clan %s.", opt, clan);
					if ((al = BuscaCliente(opt)))
						Responde(al, bl, "Has sido aceptado en el clan \00312%s", clan);
					Free(clan);
				}
				else if (!strcasecmp(acc, "RECHAZAR"))
				{
					char *clan;
					Cliente *al;
					if (BadPtr(opt))
					{
						Responde(cl, bl, GS_ERR_PARA, "CLAN", "RECHAZAR nick");
						return 1;
					}
					if (!(clan = SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan")))
					{
						Responde(cl, bl, GS_ERR_EMPT, "No perteneces a ningún clan.");
						return 1;
					}
					clan = strdup(clan);
					if (!(BDato(cl->nombre, "claner") & B_RECV))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No tienes nivel para RECHAZAR.");
						return 1;
					}
					if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s' AND clan='%s' AND claner=%i", PREFIJO, GS_BIDLE, strtolower(opt), clan, B_PEND)))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "Este usuario no ha solicitado la entrada al clan.");
						return 1;
					}
					SQLFreeRes(res);
					SQLInserta(GS_BIDLE, opt, "claner", "0");
					SQLInserta(GS_BIDLE, opt, "clan", NULL);
					Responde(cl, bl, "%s ha sido rechazado.", opt);
					if ((al = BuscaCliente(opt)))
						Responde(al, bl, "Has sido rechazado del clan \00312%s", clan);
					Free(clan);
				}
				else if (!strcasecmp(acc, "LISTAR"))
				{
					char *clan;
					if (!(clan = SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan")))
					{
						Responde(cl, bl, GS_ERR_EMPT, "No perteneces a ningún clan.");
						return 1;
					}
					clan = strdup(clan);
					if (!(res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(clan)='%s' AND claner=%i", PREFIJO, GS_BIDLE, strtolower(clan), B_PEND)))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No existen solicitudes pendientes.");
						return 1;
					}
					Responde(cl, bl, "Solicitudes pendientes:");
					while ((row = SQLFetchRow(res)))
						Responde(cl, bl, row[0]);
					SQLFreeRes(res);
					Free(clan);
				}
				else if (!strcasecmp(acc, "CLANER"))
				{
					char *clan, *oc;
					if (BadPtr(opt))
					{
						Responde(cl, bl, GS_ERR_PARA, "CLAN", "CLANER nick");
						return 1;
					}
					if (!(clan = SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan")))
					{
						Responde(cl, bl, GS_ERR_EMPT, "No perteneces a ningún clan.");
						return 1;
					}
					clan = strdup(clan);
					if (!(BDato(cl->nombre, "claner") & B_RECV))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No tienes nivel para CLANER.");
						return 1;
					}
					if (!strcasecmp(opt, cl->nombre))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No puedes ponerte a ti mismo.");
						return 1;
					}
					if (!(oc = SQLCogeRegistro(GS_BIDLE, opt, "clan")) || strcmp(clan, oc))
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "Este usuario no pertenece a tu clan.");
						return 1;
					}
					Responde(cl, bl, "\00312%s\003 inscrito como claner. Dejas de ser claner.", opt);
					SQLInserta(GS_BIDLE, opt, "claner", "%i", BDato(opt, "claner") | B_RECV);
					SQLInserta(GS_BIDLE, cl->nombre, "claner", "%i", BDato(cl->nombre, "claner") & ~B_RECV);
				}
				else if (!strcasecmp(acc, "SALIR"))
				{
					char *clan;
					if (!(clan = SQLCogeRegistro(GS_BIDLE, cl->nombre, "clan")))
					{
						Responde(cl, bl, GS_ERR_EMPT, "No perteneces a ningún clan.");
						return 1;
					}
					clan = strdup(clan);
					if (BDato(cl->nombre, "claner") & B_RECV)
					{
						Free(clan);
						Responde(cl, bl, GS_ERR_EMPT, "No puedes salir del clan mientras seas claner. Designa a otro como claner con el comando CLAN CLANER nick.");
						return 1;
					}
					Responde(cl, bl, "Te retiras del clan \00312%s\003.", clan);
					BCMsg("El clan %s quita un sello de su libro y llora la pérdida de %s.", clan, cl->nombre);
					SQLInserta(GS_BIDLE, cl->nombre, "claner", "0");
					SQLInserta(GS_BIDLE, cl->nombre, "clan", NULL);
					Free(clan);
				}
				else
				{
					Responde(cl, bl, GS_ERR_PARA, "CLAN", "{CREAR|ENTRAR|ACEPTAR|RECHAZAR|LISTAR|SALIR|CLANER} [opción]");
					return 1;
				}
				
			}
			else if (!strcasecmp(com, "COMPRAR"))
			{
				char *opt = strtok(NULL, " ");
				int i, nivel = BDato(cl->nombre, "nivel"), lvl, mej = 0;
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BadPtr(opt))
				{
					Responde(cl, bl, "Puedes comprar los siguientes objetos al siguiente precio:");
					for (i = 0; i < BIDLE_ITEMS; i++)
					{
						lvl = (int)(nivel * 1.5 * (1+((nivel+i*i)%11)*0.02));
						if (!((nivel+i)%10))
						{
							mej = (int)((((i*(BCuenta()+nivel*100))%86400)*50*BidleSum(cl->nombre, 0))/8640000);
							Responde(cl, bl, "%c%s: nivel \00312%i\003, mejora \x2+%i\x2, \00312%i\003 oros", *items[i]-32, items[i]+1, lvl, mej, (int)((lvl+mej)*bidle->paso_compra));
						}
						else
							Responde(cl, bl, "%c%s: nivel \00312%i\003, \00312%i\003 oros", *items[i]-32, items[i]+1, lvl, (int)(lvl*bidle->paso_compra));
					}
				}
				else
				{
					for (i = 0; i < BIDLE_ITEMS; i++)
					{
						if (!strcasecmp(items[i], opt))
						{
							int precio;
							lvl = (int)(nivel * 1.5 * (1+((nivel+i*i)%11)*0.02));
							if (!((nivel+i)%10))
								mej = (int)((((i*(BCuenta()+nivel*100))%86400)*50*BidleSum(cl->nombre, 0))/8640000);
							precio = (int)((lvl+mej)*bidle->paso_compra);
							if (BDato(cl->nombre, "oro") < precio)
								Responde(cl, bl, GS_ERR_EMPT, "No tienes suficientes oros");
							else
							{
								SQLInserta(GS_BIDLE, cl->nombre, items[i], "%li", lvl);
								if (mej)
									SQLInserta(GS_BIDLE, cl->nombre, "mejora", "%li", mej);
								SQLInserta(GS_BIDLE, cl->nombre, "oro", "%li", BDato(cl->nombre, "oro")-precio);
								Responde(cl, bl, "Has comprado %s %s de nivel %i por %i oros.", plrs3[i], items[i], lvl, precio);
							}
							return 0;
						}
					}
					Responde(cl, bl, GS_ERR_EMPT, "Este objeto no existe.");
					return 1;
				}
			}
			else if (!strcasecmp(com, "VENDER"))
			{
				char *opt = strtok(NULL, " ");
				int i, lvl;
				if (BDato(cl->nombre, "online") != 1)
				{
					Responde(cl, bl, GS_ERR_EMPT, "No estás logueado.");
					return 1;
				}
				if (BadPtr(opt))
				{
					SQLRes res;
					SQLRow row;
					if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s'", PREFIJO, GS_BIDLE, strtolower(cl->nombre))))
					{
						row = SQLFetchRow(res);
						Responde(cl, bl, "Puedes vender los siguientes objetos al siguiente precio:");
						for (i = 0; i < BIDLE_ITEMS; i++)
						{
							if ((lvl = atoi(row[BIDLE_ITEMS_POS+i])))
								Responde(cl, bl, "%c%s: nivel \00312%i\003, \00312%i\003 oros", *items[i]-32, items[i]+1, lvl, (int)(lvl*bidle->paso_compra*bidle->paso_vende));
						}
					}
					SQLFreeRes(res);
				}
				else
				{
					for (i = 0; i < BIDLE_ITEMS; i++)
					{
						if (!strcasecmp(items[i], opt))
						{
							int precio, lvl = BDato(cl->nombre, items[i]);
							if ((precio = (int)(lvl*bidle->paso_compra*bidle->paso_vende)))
							{
								SQLInserta(GS_BIDLE, cl->nombre, "oro", "%li", BDato(cl->nombre, "oro")+precio);
								SQLInserta(GS_BIDLE, cl->nombre, items[i], "0");
								Responde(cl, bl, "Has vendido %s %s de nivel %i por %i oros.", plrs4[i], items[i], lvl, precio);
							}
							else
								Responde(cl, bl, "Este objeto no tiene nivel.");
							return 0;
						}
					}
					Responde(cl, bl, GS_ERR_EMPT, "Este objeto no existe.");
					return 1;
				}
			}
			else if (!strcasecmp(com, "HELP"))
			{
				char *para = strtok(NULL, " ");
				if (!para)
				{
					Responde(cl, bl, "Juego de IDLE en el IRC - Ver: \00312%s", ver);
					Responde(cl, bl, " ");
					Responde(cl, bl, "El objetivo de este RPG es conseguir idle, cuanto más mejor.");
					Responde(cl, bl, "El idle es el tiempo que transcurre sin que realices ninguna acción (no salir, no desconectar, no hablar, no cambiarte el nick, etc.)");
					Responde(cl, bl, "A medida que transcurra ese tiempo irás subiendo de nivel.");
					Responde(cl, bl, "Pero si rompes ese idle y realizas una acción, serás penalizado y se añadirán varios segundos a tu reloj, con lo que tardarás más en subir de nivel.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Durante el juego irás descubriendo objetos, algunos únicos, luchando contra otros oponentes, realizando misiones... y todo ello te sumará o restará segundos a tu reloj.");
					Responde(cl, bl, "Además, en el transcurso irán sucediendo acontenicimientos como Manos de Dios, Calamidades, Ráfagas, Batallas, etc. de forma aleatoria. Recuerda que pueden beneficiarte o perjudicarte.");
					Responde(cl, bl, "También puedes unirte a un clan o bien fundar el tuyo. ");
					Responde(cl, bl, "Y no sólo eso. También puedes definir a qué lado perteneces: al de La Luz Divina, al de la Oscuridad Tenebrosa o al Neutral.");
					Responde(cl, bl, "Cada lado proporciona ventajas e inconvenientes exclusivos que no tiene el otro lado.");
					Responde(cl, bl, "Prueba y descubre cuál es el que mejor te beneficia.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Todos los personajes se mueven en un tablero de juego. Cada pocos segundos todos los jugadores online se van desplazando y cuando encuentran a otro jugador en su casilla entran en combate con él.");
					Responde(cl, bl, "Se trata de un juego muy fácil. Sólo tienes que hacer una cosa: NADA. Y ver como jugador va subiendo de nivel, consiguiendo nuevos objetos y ganando batallas.");
					Responde(cl, bl, "Para eso sólo tienes que entrar en \00312%s\003 y no hacer nada.", bidle->canal);
					Responde(cl, bl, "Una vez estés dado de alta, cada vez que entres al canal %s con el nick identificado quedarás logueado y empezarás a jugar automáticamente.", bidle->canal);
					Responde(cl, bl, "Recuerda que puedes dejar de jugar en cualquier momento. Tendrás que desloguearte, pero sufrirás una penalización.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Comandos disponibles:");
					Responde(cl, bl, "\00312ALTA\003 Te das de alta en el juego.");
					Responde(cl, bl, "\00312BAJA\003 Te das de baja. Perderás toda la puntuación y no podrás recuperar tu personaje.");
					Responde(cl, bl, "\00312LOGOUT\003 Dejas de jugar. Sufres una penalización.");
					Responde(cl, bl, "\00312INFO\003 Muestra tu información o de otro jugador, como el nivel, cuánto queda para subir al siguiente, la puntuación, etc.");
					Responde(cl, bl, "\00312LADO\003 Fija el Lado al que perteneces.");
					Responde(cl, bl, "\00312QUEST\003 Da información sobre la misión en curso.");
					Responde(cl, bl, "\00312CLAN\003 Crea, solicita una invitación, acepta una invitación o abandona un clan.");
					Responde(cl, bl, "\00312COMPRAR\003 Compra objetos.");
					Responde(cl, bl, "\00312VENDER\003 Vende objetos.");
					if (BDato(cl->nombre, "admin") == 1)
					{
						Responde(cl, bl, "\00312ADMIN\003 Añade o elimina jugadores para que puedan administrar el juego.");
						Responde(cl, bl, "\00312HOG\003 Lanza una Mano de Dios. Un jugador aleatorio se ve beneficiado o perjudicado.");
						Responde(cl, bl, "\00312PUSH\003 Añade o resta segundos al reloj de un jugador.");
					}
					Responde(cl, bl, " ");
					Responde(cl, bl, "Para más información, /msg %s HELP comando", bidle->nick);
				}
				else if (!strcasecmp(para, "ALTA"))
				{
					Responde(cl, bl, "\00312ALTA");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Te das de alta en el juego.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312ALTA descripción_del_personaje");
				}
				else if (!strcasecmp(para, "BAJA"))
				{
					Responde(cl, bl, "\00312BAJA");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Te das de baja del juego. Perderás toda la puntuación y no podrás recuperarla.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312BAJA");
				}
				else if (!strcasecmp(para, "LOGOUT"))
				{
					Responde(cl, bl, "\00312LOGOUT");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Te desconecta del juego. Este comando tiene una penalización.");
					Responde(cl, bl, "Una vez desconectado no tendrás más penalizaciones.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312LOGOUT");
				}
				else if (!strcasecmp(para, "INFO"))
				{
					Responde(cl, bl, "\00312INFO");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Da información sobre un jugador o de ti mismo.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312INFO [nick]");
				}
				else if (!strcasecmp(para, "LADO"))
				{
					Responde(cl, bl, "\00312LADO");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Fija el lado al que perteneces.");
					Responde(cl, bl, "Existen tres lados: Luz, Oscuridad y Neutral. Cada lado aporta ventajas e inconvenientes que ninguno da.");
					Responde(cl, bl, "Por ejemplo, los de la Luz suben de nivel más rápido pero los de la Oscuridad pegan más fuerte.");
					Responde(cl, bl, "Prueba el lado que más te favorezca.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312LADO {LUZ|OSCURIDAD|NEUTRAL}");
				}
				else if (!strcasecmp(para, "QUEST"))
				{
					Responde(cl, bl, "\00312QUEST");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Da información sobre la misión actual.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312QUEST");
				}
				else if (!strcasecmp(para, "CLAN"))
				{
					Responde(cl, bl, "\00312CLAN");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Gestiona el clan. Cuando un miembro del clan gana una lucha, todos los que forman parte del clan recibirán la misma bonificación. Lo mismo si pierde.");
					Responde(cl, bl, "Así, cuantas más batallas ganen entre todos, más rápido subirán de nivel los del mismo clan.");
					Responde(cl, bl, "Existen varios rangos:");
					Responde(cl, bl, "- Miembro: miembro corriente.");
					Responde(cl, bl, "- Patriarca: fundador del clan.");
					Responde(cl, bl, "- Claner: se encarga de aceptar y rechazar las invitaciones. Cuando alguien solicite una invitación para ingresar, se le avisará y será él el que la acepte o no.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis:");
					Responde(cl, bl, "\00312CLAN CREAR nombre_clan\003 Crea un nuevo clan en caso de que no exista.");
					Responde(cl, bl, "\00312CLAN ENTRAR nombre_clan\003 Manda una solicitud para ingresar en el clan. El claner se encargará de aceptarla o no.");
					Responde(cl, bl, "\00312CLAN ACEPTAR nick\003 Acepta un usuario que haya solicitado ingresar al clan.");
					Responde(cl, bl, "\00312CLAN RECHAZAR nick\003 Rechaza la solicitud.");
					Responde(cl, bl, "\00312CLAN LISTAR\003 Lista las solicitudes de ingreso pendientes.");
					Responde(cl, bl, "\00312CLAN CLANER nick\003 Cambia el claner del clan.");
					Responde(cl, bl, "\00312CLAN SALIR\003 Abandona el clan o cancela tu solicitud.");
				}
				else if (!strcasecmp(para, "COMPRAR"))
				{
					Responde(cl, bl, "\00312COMPRAR");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Compra objetos. El precio de los objetos se define según el nivel que sean.");
					Responde(cl, bl, "Depende de la demanda y la oferta, habrá un objeto que ofrezca mejoras de forma oscilante.");
					Responde(cl, bl, "La mejora es un valor fijo que se añadirá siempre al valor aleatorio durante un combate. Es decir, como mínimo siempre sacarás esa mejora.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Si no se especifica el objeto a comprar, muestra una lista de los precios de compra.");
					Responde(cl, bl, "Para ver el oro disponible, ejecuta el comando INFO.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312COMPRAR [objeto]");
				}
				else if (!strcasecmp(para, "VENDER"))
				{
					Responde(cl, bl, "\00312VENDER");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Vende un objeto a cambio de oros.");
					Responde(cl, bl, "El precio de venda se fija según el nivel que sea y del precio de compra.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Si no se especifica el objeto a vender, muestra una lista de los precios de venta.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312VENDER [objeto]");
				}
				else if (!strcasecmp(para, "ADMIN") && BDato(cl->nombre, "admin") == 1)
				{
					Responde(cl, bl, "\00312ADMIN");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Añade o elimina un administrador del juego.");
					Responde(cl, bl, "Si dicho jugador no es administrador, se le añade como tal. Si ya lo es, se elimina.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312ADMIN nick");
				}
				else if (!strcasecmp(para, "HOG") && BDato(cl->nombre, "admin") == 1)
				{
					Responde(cl, bl, "\00312HOG");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Lanza una mano de Dios.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312HOG");
				}
				else if (!strcasecmp(para, "PUSH") && BDato(cl->nombre, "admin") == 1)
				{
					Responde(cl, bl, "\00312PUSH");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Añade o resta segundos al reloj de un jugador.");
					Responde(cl, bl, "Si el valor de los segundos es positivo, se añaden; si es negativo, se restan.");
					Responde(cl, bl, " ");
					Responde(cl, bl, "Sintaxis: \00312PUSH nick {+|-}segundos");
				}
				else
				{
					Responde(cl, bl, GS_ERR_EMPT, "Comando desconocido. Para más ayuda use el comando HELP");
					BPen(cl, PEN_MSG, msg);
					return 1;
				}
			}
			else
			{
				Responde(cl, bl, GS_ERR_EMPT, "Comando desconocido. Para más ayuda use el comando HELP");
				BPen(cl, PEN_MSG, msg);
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
	if (cn == bidle->cn && IsId(cl) && BDato(cl->nombre, "online") == 1)
	{
		BPen(cl, PEN_PART, NULL);
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	}
	return 0;
}
int BidlePreNick(Cliente *cl, char *nuevo)
{
	if (IsId(cl) && BDato(cl->nombre, "online") == 1 && EsLinkCanal(cl->canal, bidle->cn) && BPen(cl, PEN_NICK, NULL)) // se desloguea
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidlePostNick(Cliente *cl, int cambio)
{
	if (IsId(cl) && cambio && SQLCogeRegistro(GS_BIDLE, cl->nombre, "nombre")) // proviene de un cambio, lo penalizamos igual y logueamos
	{
		SQLInserta(GS_BIDLE, cl->nombre, "online", "1");
		SQLInserta(GS_BIDLE, cl->nombre, "lastlogin", "%lu", time(0));
	}
	return 0;
}
int BidleQuit(Cliente *cl, char *msg)
{
	if (IsId(cl) && BDato(cl->nombre, "online") == 1 && BPen(cl, PEN_QUIT, NULL))
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidleKick(Cliente *cl, Canal *cn, char *msg)
{
	if (IsId(cl) && cn == bidle->cn && BDato(cl->nombre, "online") == 1 && BPen(cl, PEN_KICK, NULL))
		SQLInserta(GS_BIDLE, cl->nombre, "online", "0");
	return 0;
}
int BidleCMsg(Cliente *cl, Canal *cn, char *msg)
{
	if (IsId(cl) && cn == bidle->cn && BDato(cl->nombre, "online") == 1)
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
	int nivel, pen;
	char *motivo;
	long inc;
	if (BDato(cl->nombre, "online") == 1 && (nivel = BDato(cl->nombre, "nivel")) > 0)
	{
		if (BidleHaceQuest(cl->nombre))
		{
			int i, rows;
			inc = (long)(pens[PEN_QUEST].opt * pow(bidle->paso_pen, nivel));
			if (bidle->limit_pen)
				inc = MIN(bidle->limit_pen, inc);
			BCMsg("La imprudencia de %s le ha salido cara. Los Dioses llenos de ira han añadido %s a todos los participantes de la misión.", cl->nombre, BDura(inc));
			for (i = 0; i < 4; i++)
			{
				SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "pen_quest", "%li", BDato(bidle->quest.user[i].user, "pen_quest")+inc);
				SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "sig", "%li", BDato(bidle->quest.user[i].user, "sig")+inc);
				Responde(BuscaCliente(bidle->quest.user[i].user), bidle->cl, "Has sido penalizado con \00312%s\003 de más.", BDura(inc));
				ircfree(bidle->quest.user[i].user);
			}
			bidle->quest.tipo = 0;
			bidle->quest.tiempo = time(0)+3600;
		}
		if (pena == PEN_MSG)
			inc = (long)(strlen(msg) * pow(bidle->paso_pen, nivel));
		else
			inc = (long)(pens[pena].opt * pow(bidle->paso_pen, nivel));
		if (bidle->limit_pen)
			inc = MIN(bidle->limit_pen, inc);
		SQLInserta(GS_BIDLE, cl->nombre, "sig", "%li", (long)(BDato(cl->nombre, "sig") + inc));
		SQLInserta(GS_BIDLE, cl->nombre, pens[pena].item, "%li", (long)(BDato(cl->nombre, pens[pena].item) + inc));
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
				motivo = "hablar en el canal o en privado";
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
	if (!max)
		return 0;
	return Aleatorio(0, max-1);
}
int BidleSum(char *user, int batalla)
{
	SQLRes res;
	SQLRow row;
	int sum = 0, i;
	if (user == bidle->nick || !strcmp(bidle->nick, user))
	{
		if ((res = SQLQuery("SELECT * FROM %s%s", PREFIJO, GS_BIDLE)))
		{
			while ((row = SQLFetchRow(res)))
				sum = MAX(sum, BidleSum(row[0], 0));
			SQLFreeRes(res);
		}
		return sum+1;
	}
	else if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(item)='%s'", PREFIJO, GS_BIDLE, strtolower(user))))
	{
		row = SQLFetchRow(res);
		for (i = BIDLE_ITEMS_POS; i < BIDLE_ITEMS_POS+BIDLE_ITEMS; i++)
			sum += atoi(row[i]);
		SQLFreeRes(res);
		if (batalla)
		{
			if (*row[3] == 'L')
				return (int)(1.1*sum);
			else if (*row[3] == 'O')
				return (int)(0.9*sum);
			else
				return sum;
		}
		else
			return sum;
	}
	return -1;
}
int BidleComprueba()
{
	time_t curtime;
	SQLRes res;
	SQLRow row;
	int onlines = 0;
	static int count = 0;
	long val;
	if (ultime)
	{
		curtime = time(0);
		if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
		{
			onlines = SQLNumRows(res);
			while ((row = SQLFetchRow(res)))
			{
				if ((val = atol(row[8])-(curtime - ultime)) < 0) // cambio de nivel!
				{
					val = atol(row[5])+1;
					SQLInserta(GS_BIDLE, row[0], "nivel", "%li", val);
					if (val > 60)
						val = (long)(bidle->base * pow(bidle->paso, 60) + (86400 * (val - 60)));
					else
						val = (long)(bidle->base * pow(bidle->paso, val));
					SQLInserta(GS_BIDLE, row[0], "sig", "%li", val);
					BCMsg("%s, %s\xF, ha alcanzado el nivel %u! Próximo nivel en %s.", row[0], row[1], atoi(row[5])+1, BDura(val));
					snprintf(row[8], 11, "%li", val);
					BidleEncuentraItem(row);
					BidleReta(row, NULL);
				}
				else
					SQLInserta(GS_BIDLE, row[0], "sig", "%li", val);
				val = atol(row[7]) + (curtime - ultime);
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
	BidleMueve();
	if (time(0) > bidle->quest.tiempo)
		BidleQuest();
	if (count && !(count % 36000))
	{
		int i;
		if ((res = SQLQuery("SELECT * FROM %s%s ORDER BY nivel DESC, sig ASC LIMIT 0,%i", PREFIJO, GS_BIDLE, bidle->maxtop)))
		{
			BCMsg("Ránking jugadores:");
			for (i = 0; i < bidle->maxtop && (row = SQLFetchRow(res)); i++)
				BCMsg("#%i - %s, %s\xF, nivel %s. Siguiente nivel en %s.", i+1, row[0], row[1], row[5], BDura(atol(row[8])));
			SQLFreeRes(res);
		}
		count = 0;
	}
	if (count && !(count % 3600))
	{
		SQLRes res[2];
		if ((res[0] = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND nivel > 34 ORDER BY RAND() LIMIT 1", PREFIJO, GS_BIDLE)))
		{
			res[1] = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE);
			if ((double)(SQLNumRows(res[0])/SQLNumRows(res[1])) > 0.15)
			{
				row = SQLFetchRow(res[0]);
				BidleReta(row, NULL);
			}
			SQLFreeRes(res[0]);
			SQLFreeRes(res[1]);
		}
	}
	count += bidle->eventos;
	return 0;
}
int BidleSockClose()
{
	int i;
	ApagaCrono(timerbidlecheck);
	timerbidlecheck = NULL;
	SQLQuery("UPDATE %s%s SET online=0", PREFIJO, GS_BIDLE);
	ultime = 0;
	if (bidle->quest.tipo)
	{
		for (i = 0; i < 4; i++)
			ircfree(bidle->quest.user[i].user);
		bidle->quest.tipo = 0;
	}
	return 0;
}
int BidleHog()
{
	SQLRes res;
	SQLRow row;
	long val;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 ORDER BY RAND() LIMIT 1", PREFIJO, GS_BIDLE)))
	{
		row = SQLFetchRow(res);
		val = (long)((5+BAlea(71))*atol(row[8])/100);
		if (BAlea(5))
		{
			BCMsg("Los cielos han estallado y la Mano de Dios ha bendecido a %s con %s hacia el nivel %u.", row[0], BDura(val), atoi(row[5])+1);
			val = atol(row[8])-val;
		}
		else
		{
			BCMsg("Los cielos rojizos han ardido y el fuego ha consumido a %s retrasándolo %s hacia el nivel %u.", row[0], BDura(val), atoi(row[5])+1);
			val = atol(row[8])+val;
		}
		SQLInserta(GS_BIDLE, row[0], "sig", "%li", val);
		SQLFreeRes(res);
	}
	return 0;
}
int BidleBatallaEqs()
{
	SQLRes res;
	SQLRow row[6];
	int i, rows, misum, opsum, mirol, oprol;
	long inc;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 ORDER BY RAND() LIMIT 6", PREFIJO, GS_BIDLE)))
	{
		if ((rows = SQLNumRows(res)) == 6)
		{
			for (i = 0; i < 6; i++)
				row[i] = SQLFetchRow(res);
			misum = BidleSum(row[0][0], 1) + BidleSum(row[1][0], 1) + BidleSum(row[2][0], 1);
			opsum = BidleSum(row[3][0], 1) + BidleSum(row[4][0], 1) + BidleSum(row[5][0], 1);
			inc = atol(row[0][8]);
			inc = MIN(inc, atol(row[1][8]));
			inc = MIN(inc, atol(row[2][8]));
			inc = (long)(inc*0.2);
			mirol = BAlea(misum);
			oprol = BAlea(opsum);
			if (mirol >= oprol)
			{
				BCMsg("%s, %s y %s [%i/%i] se han batido en duelo con %s, %s y %s [%i/%i] y han vencido! Se han descontado %s de sus relojes.",
					row[0][0], row[1][0], row[2][0], mirol, misum,
					row[3][0], row[4][0], row[5][0], oprol, opsum,
					BDura(inc));
				SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", atol(row[0][8])-inc);
				SQLInserta(GS_BIDLE, row[1][0], "sig", "%li", atol(row[1][8])-inc);
				SQLInserta(GS_BIDLE, row[2][0], "sig", "%li", atol(row[2][8])-inc);
			}
			else
			{
				BCMsg("%s, %s y %s [%i/%i] se han batido en duelo con %s, %s y %s [%i/%i] y han perdido! Se han añadido %s a sus relojes.",
					row[0][0], row[1][0], row[2][0], mirol, misum,
					row[3][0], row[4][0], row[5][0], oprol, opsum,
					BDura(inc));
				SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", atol(row[0][8])+inc);
				SQLInserta(GS_BIDLE, row[1][0], "sig", "%li", atol(row[1][8])+inc);
				SQLInserta(GS_BIDLE, row[2][0], "sig", "%li", atol(row[2][8])+inc);
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
	long val, inc;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 ORDER BY RAND() LIMIT 1", PREFIJO, GS_BIDLE)))
	{
		row = SQLFetchRow(res);
		if (!BAlea(10))
		{
			val = BAlea(10);
			BCMsg("A los Dioses no les ha gustado %s %s de %s y ahora están malditos. Han perdido un 10%% de su efectividad.", plrs[val], items[val], row[0]);
			inc = (long)(atol(row[BIDLE_ITEMS_POS+val])*0.9);
			SQLInserta(GS_BIDLE, row[0], items[val], "%li%c", inc, IsDigit(row[BIDLE_ITEMS_POS+val][strlen(row[BIDLE_ITEMS_POS+val]-1)]) ? '\0' : row[BIDLE_ITEMS_POS+val][strlen(row[BIDLE_ITEMS_POS+val]-1)]);
		}
		else
		{
			inc = (long)((5+BAlea(8))*atol(row[8])/100);
			BCMsg("%s ha sufrido una calamidad! Se han añadido %s a su reloj.", row[0], BDura(inc));
			SQLInserta(GS_BIDLE, row[0], "sig", "%li", atol(row[8])+inc);
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleRafagas()
{
	SQLRes res;
	SQLRow row;
	long val, inc;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 ORDER BY RAND() LIMIT 1", PREFIJO, GS_BIDLE)))
	{
		row = SQLFetchRow(res);
		if (!BAlea(10))
		{
			val = BAlea(10);
			BCMsg("%s ha interceptado una ráfaga divina con %s %s! Ha aumentado su efectividad en un 10%%.", row[0], plrs2[val], items[val]);
			inc = (long)(atol(row[BIDLE_ITEMS_POS+val])*0.9);
			SQLInserta(GS_BIDLE, row[0], items[val], "%li%c", inc, IsDigit(row[BIDLE_ITEMS_POS+val][strlen(row[BIDLE_ITEMS_POS+val]-1)]) ? '\0' : row[BIDLE_ITEMS_POS+val][strlen(row[BIDLE_ITEMS_POS+val]-1)]);
		}
		else
		{
			inc = (long)((5+BAlea(8))*atol(row[8])/100);
			BCMsg("%s ha recibido una ráfaga divina! Se han descontado %s de su reloj.", row[0], BDura(inc));
			SQLInserta(GS_BIDLE, row[0], "sig", "%li", atol(row[8])-inc);
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
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Luz' ORDER BY RAND() LIMIT 2", PREFIJO, GS_BIDLE)))
	{
		if ((rows = SQLNumRows(res)) == 2)
		{
			row[0] = SQLFetchRow(res);
			row[1] = SQLFetchRow(res);
			inc = 5 + BAlea(8);
			BCMsg("%s y %s rechazaron el veneno de la Oscuridad. Juntos han rezado y la Luz celestial les ha iluminado. Se han descontado el %i%% de sus relojes.", row[0][0], row[1][0], inc);
			SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", (long)(atol(row[0][8])*(1-inc/100)));
			SQLInserta(GS_BIDLE, row[1][0], "sig", "%li", (long)(atol(row[1][8])*(1-inc/100)));
			BCMsg("%s subirá de nivel en %s.", row[0][0], BDura(atol(row[0][8])*(1-inc/100)));
			BCMsg("%s subirá de nivel en %s.", row[1][0], BDura(atol(row[1][8])*(1-inc/100)));
		}
		SQLFreeRes(res);
	}
	return 0;
}
int BidleOscuridad()
{
	SQLRes res[2];
	SQLRow row[2];
	long val, inc;
	char *t;
	if ((res[0] = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Oscuridad'", PREFIJO, GS_BIDLE)))
	{
		row[0] = SQLFetchRow(res[0]);
		if (BAlea(2) < 1)
		{
			if ((res[1] = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND lado='Luz'", PREFIJO, GS_BIDLE)))
			{
				row[1] = SQLFetchRow(res[1]);
				val = BAlea(10);
				if (atoi(row[1][BIDLE_ITEMS_POS+val]) > atoi(row[0][BIDLE_ITEMS_POS+val]))
				{
					SQLInserta(GS_BIDLE, row[0][0], items[val], row[1][BIDLE_ITEMS_POS+val]);
					SQLInserta(GS_BIDLE, row[1][0], items[val], row[0][BIDLE_ITEMS_POS+val]);
					BCMsg("%s roba %s %s nivel %s de %s mientras dormía! Mientras aprovecha para dejarle %s %s nivel %s.", 
						row[0][0], plrs[val], items[val], row[1][BIDLE_ITEMS_POS+val], row[1][0],
						plrs2[val], items[val], row[0][BIDLE_ITEMS_POS+val]);
				}
				else
				{
					BCMsg("%s ha intentado robar %s %s de %s. Pero se ha dado cuenta de que es de menor nivel y lo arroja a las tinieblas.",
						row[0][0], plrs[val], items[val], row[1][0]);
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
			val = atol(row[0][8]);
			t = strdup(BDura(val*(1+inc/100)));
			BCMsg("%s está maldecido por los Señores de las Tinieblas. Se han añadido %s a su reloj. Subirá de nivel en %s.", row[0][0], BDura(val*inc/100), t);
			SQLInserta(GS_BIDLE, row[0][0], "sig", "%li", (long)(val*(1+inc/100)));
			Free(t);
		}
		SQLFreeRes(res[0]);
	}
	return 0;
}
int BidleHaceQuest(char *user)
{
	int i;
	if (!bidle->quest.tipo)
		return 0;
	for (i = 0; i < 4; i++)
	{
		if (!strcmp(user, bidle->quest.user[i].user))
			return 1;
	}
	return 0;
}
int BidleMueve()
{
	SQLRes res;
	SQLRow row;
	int i;
	for (i = 0; i < bidle->maxx; i++)
		bzero(bidle->pos[i], sizeof(struct BidlePos)*bidle->maxy);
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1", PREFIJO, GS_BIDLE)))
	{
		int x, y, rows = SQLNumRows(res);
		rows = SQLNumRows(res);
		while ((row = SQLFetchRow(res)))
		{
			if (bidle->quest.tipo == 2 && BidleHaceQuest(row[0]))
				continue;
			x = atoi(row[9])+BAlea(3)-1;
			y = atoi(row[10])+BAlea(3)-1;
			if (x >= bidle->maxx)
				x = 0;
			else if (x < 0)
				x = bidle->maxx-1;
			if (y >= bidle->maxy)
				y = 0;
			else if (y < 0)
				y = bidle->maxy-1;
			SQLInserta(GS_BIDLE, row[0], "x", "%i", x);
			SQLInserta(GS_BIDLE, row[0], "y", "%i", y);
			if (bidle->pos[x][y].user && !bidle->pos[x][y].batallado)
			{
				if (atoi(bidle->pos[x][y].user[4]) && !atoi(row[4]) && !BAlea(100))
					BCMsg("%s se encuentra con %s y lo evita sigilosamente.", row[0], bidle->pos[x][y].user[0]);
				if (!BAlea(rows))
				{
					bidle->pos[x][y].batallado = 1;
					BidleReta(row, bidle->pos[x][y].user);
				}
			}
			else
			{
				bidle->pos[x][y].batallado = 0;
				bidle->pos[x][y].user = row;
			}
		}
		SQLFreeRes(res);
		if (bidle->quest.tipo == 2)
		{
			int i, idos = 1, oros;
			for (i = 0; i < 4; i++)
			{
				if (bidle->quest.user[i].x != bidle->quest.x[bidle->quest.fase] || bidle->quest.user[i].y != bidle->quest.y[bidle->quest.fase])
				{
					idos = 0;
					if (bidle->quest.user[i].x != bidle->quest.x[bidle->quest.fase])
					{
						bidle->quest.user[i].x += (bidle->quest.user[i].x < bidle->quest.x[bidle->quest.fase] ? 1 : -1);
						SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "x", "%i", bidle->quest.user[i].x);
					}
					if (bidle->quest.user[i].y != bidle->quest.y[bidle->quest.fase])
					{
						bidle->quest.user[i].y += (bidle->quest.user[i].y < bidle->quest.y[bidle->quest.fase] ? 1 : -1);
						SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "y", "%i", bidle->quest.user[i].y);
					}
				}
			}
			if (idos)
			{
				if (bidle->quest.fase == 1)
				{
					oros = BAlea((time(0)-bidle->quest.initime)/360);
					if (bidle->quest.user[3].user)
						BCMsg("%s, %s, %s y %s han logrado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, oros);
					else if (bidle->quest.user[2].user)
						BCMsg("%s, %s y %s han logrado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, oros);
					else if (bidle->quest.user[1].user)
						BCMsg("%s y %s han logrado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, oros);
					for (i = 0; i < 4; i++)
					{
						SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "sig", "%li", (long)(BDato(bidle->quest.user[i].user, "sig")*0.75));
						SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "oro", "%li", BDato(bidle->quest.user[i].user, "oro")+oros);
						ircfree(bidle->quest.user[i].user);
					}
					bidle->quest.tipo = 0;
					bidle->quest.tiempo = time(0)+3600;
				}
				else
					bidle->quest.fase++;
			}
		}
	}
	return 0;
}
int BidleEncuentraItem(SQLRow row)
{
	int it = BAlea(10), lvl = atoi(row[5]), nivel = 1, univel, i, m = (int)(lvl*1.5), oro;
	Cliente *cl;
	if (!(cl = BuscaCliente(row[0])))
		return -1;
	for (i = 1; i <= m; i++)
	{
		if (BAlea((u_int)pow(1.4, i/4)) < 1)
			nivel = i;
	}
	if (lvl >= 25 && BAlea(40) < 1)
	{
		univel = 50+BAlea(25);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+2]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado la Corona del Omnisciente Mattt de nivel %li! Tus enemigos caerán antes de que puedan moverse.", univel);
			SQLInserta(GS_BIDLE, row[0], "casco", "%lia", univel);
			return 1;
		}
	}
	else if (lvl >= 25 && BAlea(40) < 1)
	{
		univel = 50+BAlea(25);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+5]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado el Glorioso Anillo de Cenizas de Julieta de nivel %li! Tus enemigos se quedarán ciegos por su gloria.", univel);
			SQLInserta(GS_BIDLE, row[0], "anillo", "%lih", univel);
			return 1;
		}
	}
	else if (lvl >= 30 && BAlea(40) < 1)
	{
		univel = 75+BAlea(25);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+8]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado la Cota de Mallas Protectora de Res0 de nivel %li! Tus enemigos se quedarán inmóviles cuando vean que sus ataques no tienen efecto.", univel);
			SQLInserta(GS_BIDLE, row[0], "tunica", "%lib", univel);
			return 1;
		}
	}
	else if (lvl >= 35 && BAlea(40) < 1)
	{
		univel = 100+BAlea(25);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado el Amuleto Mágico de la Tormenta de Dwyn de nivel %li! Tus enemigos huirán por la furia que nunca antes habrán visto.", univel);
			SQLInserta(GS_BIDLE, row[0], "amuleto", "%lic", univel);
			return 1;
		}
	}
	else if (lvl >= 40 && BAlea(40) < 1)
	{
		univel = 150+BAlea(25);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+9]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado la Espada Colosal de la Furia de Jotun de nivel %li! Tus enemigos tendrán un final rápido porque tu arma les doblegará como si fueran de papel.", univel);
			SQLInserta(GS_BIDLE, row[0], "arma", "%lid", univel);
			return 1;
		}
	}
	else if (lvl >= 45 && BAlea(40) < 1)
	{
		univel = 175+BAlea(26);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+9]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado el Báculo de la Rabia Oculta de nivel %li! Tus enemigos serán sacudidos y odiarán haberte conocido.", univel);
			SQLInserta(GS_BIDLE, row[0], "arma", "%lie", univel);
			return 1;
		}
	}
	else if (lvl >= 48 && BAlea(40) < 1)
	{
		univel = 250+BAlea(51);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+3]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado las Botas Mágicas Rápidas de Mrquick de nivel %li! Tus enemigos abandonarán cuando vean el polvo que levantas cuando corras hacia ellos tan rápido.", univel);
			SQLInserta(GS_BIDLE, row[0], "botas", "%lif", univel);
			return 1;
		}
	}
	else if (lvl >= 52 && BAlea(40) < 1)
	{
		univel = 300+BAlea(51);
		if (univel >= nivel && univel > atoi(row[BIDLE_ITEMS_POS+9]))
		{
			Responde(cl, bidle->cl, "La Luz de los Dioses está de tu parte! Has encontrado el Martillo de la Condenación de Jeff de nivel %li! Tus enemigos se rendirán cuando vean el resplandor que desprendre su relieve.", univel);
			SQLInserta(GS_BIDLE, row[0], "arma", "%lig", univel);
			return 1;
		}
	}
	if (nivel > atoi(row[BIDLE_ITEMS_POS+it]))
	{
		Responde(cl, bidle->cl, "Has encontrado %s %s de nivel %i. T%s %s sólo es de nivel %i. Parece que la Suerte está de tu lado!", plrs3[it], items[it], nivel, plrs4[it]+1, items[it], atoi(row[BIDLE_ITEMS_POS+it]));
		SQLInserta(GS_BIDLE, row[0], items[it], "%li", nivel);
		return 1;
	}
	else
		Responde(cl, bidle->cl, "Has encontrado %s %s de nivel %i. T%s %s es de nivel %i. Parece que la Suerte está contra ti. Te deshaces del objeto.", plrs3[it], items[it], nivel, plrs4[it]+1, items[it], atoi(row[BIDLE_ITEMS_POS+it]));
	if ((oro = BAlea(lvl)))
	{
		SQLInserta(GS_BIDLE, row[0], "oro", "%li", atoi(row[30])+oro);
		Responde(cl, bidle->cl, "Se han añadido %i oros a tu cuenta.", oro);
	}
	return 0;
}
int BidleClan(char *clan, char *exc, int segs)
{
	SQLRes res;
	SQLRow row;
	char *clw = strtolower(clan);
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE LOWER(clan)='%s' AND claner >= %i AND LOWER(item)!='%s' AND online=1", PREFIJO, GS_BIDLE, clw, B_ISIN, strtolower(exc))))
	{
		while ((row = SQLFetchRow(res)))
			SQLInserta(GS_BIDLE, row[0], "sig", "%li", atol(row[8])+segs);
		SQLFreeRes(res);
	}
	Free(clw);
	return 0;
}
int BidleReta(SQLRow mirow, SQLRow op)
{
	SQLRes res = NULL;
	SQLRow oprow = NULL;
	int rows, misum, opsum, mirol, oprol, factor = 35, it;
	long sig = atol(mirow[8]), inc;
	char *opuser;
	if (op)
	{
		oprow = op;
		opuser = op[0];
	}
	else 
	{
		if (atoi(mirow[5]) < 25 && BAlea(4) > 0)
			return 0;
		res = SQLQuery("SELECT COUNT(*) FROM %s%s", PREFIJO, GS_BIDLE);
		oprow = SQLFetchRow(res);
		rows = atoi(oprow[0]);
		SQLFreeRes(res);
		res = NULL;
		oprow = NULL;
		if (rows == 1 || BAlea(rows+1) < 1)
			opuser = bidle->nick;
		else
		{
			if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND LOWER(item)!='%s' ORDER BY RAND() LIMIT 1", PREFIJO, GS_BIDLE, strtolower(mirow[0]))))
			{
				oprow = SQLFetchRow(res);
				opuser = oprow[0];
			}
			else
				opuser = bidle->nick;
		}
	}
	misum = BidleSum(mirow[0], 1);
	opsum = BidleSum(opuser, 1);
	mirol = BAlea(misum)+atoi(mirow[31]);
	oprol = BAlea(opsum)+(oprow ? atoi(oprow[31]) : 0);
	if (mirol >= oprol)
	{
		inc = (long)(opuser == bidle->nick ? 20 : atol(oprow[5])/4);
		inc = MAX(7, inc);
		inc = (long)(inc*sig/100);
		if (!BadPtr(mirow[2]))
		{
			BCMsg("%s [%i/%i]%s%s ha retado a %s [%i/%i]%s%s en combate y ha ganado! Se han descontado %s del reloj de todos los miembros de %s.", 
				mirow[0], mirol, misum, (*mirow[31] != '0' ? "+" : ""), (*mirow[31] != '0' ? mirow[31] : ""), 
				opuser, oprol, opsum, (oprow && *oprow[31] != '0' ? "+" : ""), (oprow && *oprow[31] != '0' ? oprow[31] : ""), BDura(inc), mirow[2]);
			BidleClan(mirow[2], opuser, -inc);
		}
		else
		{
			BCMsg("%s [%i/%i]%s%s ha retado a %s [%i/%i]%s%s en combate y ha ganado! Se han descontado %s de su reloj.", 
				mirow[0], mirol-atoi(mirow[31]), misum, (*mirow[31] != '0' ? "+" : ""), (*mirow[31] != '0' ? mirow[31] : ""), 
				opuser, oprol-(oprow ? atoi(oprow[31]) : 0), opsum, (oprow && *oprow[31] != '0' ? "+" : ""), (oprow && *oprow[31] != '0' ? oprow[31] : ""), BDura(inc));
			SQLInserta(GS_BIDLE, mirow[0], "sig", "%li", sig-inc);
		}
		if (*mirow[3] == 'L')
			factor = 50;
		else if (*mirow[3] == 'O')
			factor = 20;
		if (BAlea(factor) < 1 && opuser != bidle->nick)
		{
			sig = atol(oprow[8]);
			inc = (long)((5+BAlea(20))*sig/100);
			BCMsg("%s ha propinado un Golpe Crítico a %s! Se han añadido %s al reloj de %s.", mirow[0], opuser, BDura(inc), opuser);
			SQLInserta(GS_BIDLE, opuser, "sig", "%li", sig+inc);
		}
		else if (BAlea(25) < 1 && opuser != bidle->nick && atoi(mirow[5]) > 19)
		{
			it = BAlea(10);
			if (atoi(oprow[BIDLE_ITEMS_POS+it]) > atoi(mirow[BIDLE_ITEMS_POS+it]))
			{
				BCMsg("En el feroz combate, se le ha caído %s %s de nivel %i a %s! %s se aprovecha y lo intercambia por %s %s de nivel %i.",
					plrs[it], items[it], atoi(oprow[BIDLE_ITEMS_POS+it]), opuser, mirow[0], plrs2[it], items[it], atoi(mirow[BIDLE_ITEMS_POS+it]));
				SQLInserta(GS_BIDLE, mirow[0], items[it], oprow[BIDLE_ITEMS_POS+it]);
				SQLInserta(GS_BIDLE, oprow[0], items[it], mirow[BIDLE_ITEMS_POS+it]);
			}
		}
	}
	else
	{
		inc = (long)(opuser == bidle->nick ? 10 : atol(oprow[5])/7);
		inc = MAX(7, inc);
		inc = (long)(inc*sig/100);
		if (!BadPtr(mirow[2]))
		{
			BCMsg("%s [%i/%i]%s%s ha retado a %s [%i/%i]%s%s en combate y ha perdido! Se han añadido %s al reloj de todos los miembros de %s.", 
				mirow[0], mirol-atoi(mirow[31]), misum, (*mirow[31] != '0' ? "+" : ""), (*mirow[31] != '0' ? mirow[31] : ""), 
				opuser, oprol-(oprow ? atoi(oprow[31]) : 0), opsum, (oprow && *oprow[31] != '0' ? "+" : ""), (oprow && *oprow[31] != '0' ? oprow[31] : ""), BDura(inc), mirow[2]);
			BidleClan(mirow[2], opuser, inc);
		}
		else
		{
			BCMsg("%s [%i/%i]%s%s ha retado a %s [%i/%i]%s%s en combate y ha perdido! Se han añadido %s a su reloj.", 
				mirow[0], mirol-atoi(mirow[31]), misum, (*mirow[31] != '0' ? "+" : ""), (*mirow[31] != '0' ? mirow[31] : ""), 
				opuser, oprol-(oprow ? atoi(oprow[31]) : 0), opsum, (oprow && *oprow[31] != '0' ? "+" : ""), (oprow && *oprow[31] != '0' ? oprow[31] : ""), BDura(inc));
			SQLInserta(GS_BIDLE, mirow[0], "sig", "%li", sig+inc);
		}
	}
	if (res)
		SQLFreeRes(res);
	return 1;
}
int BidleQuest()
{
	SQLRes res;
	SQLRow row;
	int rows;
	if (!bidle->quest.tipo)
	{
		if ((res = SQLQuery("SELECT * FROM %s%s WHERE online=1 AND nivel > 29 AND lastlogin < %lu ORDER BY RAND() LIMIT 4", PREFIJO, GS_BIDLE, time(0)-1200)))
		{
			int i;
			if ((rows = SQLNumRows(res)) > 1)
			{
				for (i = 0; i < MIN(rows,4); i++)
				{
					row = SQLFetchRow(res);
					ircstrdup(bidle->quest.user[i].user, row[0]);
				}
				bidle->quest.tipo = BAlea(2)+1;
				bidle->quest.initime = time(0);
				if (bidle->quest.tipo == 1)
				{
					bidle->quest.tiempo = time(0)+43200+BAlea(43201);
					if (bidle->quest.user[3].user)
						BCMsg("%s, %s, %s y %s han sido escogidos por los Dioses para realizar una dura misión. Terminarán dentro de %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, BDura(bidle->quest.tiempo-time(0)));
					else if (bidle->quest.user[2].user)
						BCMsg("%s, %s y %s han sido escogidos por los Dioses para realizar una dura misión. Terminarán dentro de %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, BDura(bidle->quest.tiempo-time(0)));
					else if (bidle->quest.user[1].user)
						BCMsg("%s y %s han sido escogidos por los Dioses para realizar una dura misión. Terminarán dentro de %s.", bidle->quest.user[0].user, bidle->quest.user[1].user, BDura(bidle->quest.tiempo-time(0)));
				}
				else if (bidle->quest.tipo == 2)
				{
					bidle->quest.x[0] = BAlea(bidle->maxx);
					bidle->quest.x[1] = BAlea(bidle->maxx);
					bidle->quest.y[0] = BAlea(bidle->maxy);
					bidle->quest.y[1] = BAlea(bidle->maxy);
					bidle->quest.fase = 0;
					if (bidle->quest.user[3].user)
						BCMsg("%s, %s, %s y %s han sido escogidos por los Dioses para realizar una dura misión. Deberán ir a las coordenadas [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
					else if (bidle->quest.user[2].user)
						BCMsg("%s, %s y %s han sido escogidos por los Dioses para realizar una dura misión. Deberán ir a las coordenadas [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
					else if (bidle->quest.user[1].user)
						BCMsg("%s y %s han sido escogidos por los Dioses para realizar una dura misión. Deberán ir a las coordenadas [%i,%i] y luego a [%i,%i].", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.x[0], bidle->quest.y[0], bidle->quest.x[1], bidle->quest.y[1]);
				}
			}
			SQLFreeRes(res);
		}
	}
	else if (bidle->quest.tipo == 1)
	{
		int i, oros = BAlea((time(0)-bidle->quest.initime)/360);
		if (bidle->quest.user[3].user)
			BCMsg("%s, %s, %s y %s han sido bendecidos por el reino celestial y han completado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, bidle->quest.user[3].user, oros);
		else if (bidle->quest.user[2].user)
			BCMsg("%s, %s y %s han sido bendecidos por el reino celestial y han completado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, bidle->quest.user[2].user, oros);
		else if (bidle->quest.user[1].user)
			BCMsg("%s y %s han sido bendecidos por el reino celestial y han completado su misión! Se ha descontado el 25%% de sus relojes y han ganado %i oros.", bidle->quest.user[0].user, bidle->quest.user[1].user, oros);
		for (i = 0; i < 4; i++)
		{
			SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "sig", "%li", (long)(BDato(bidle->quest.user[i].user, "sig")*0.75));
			SQLInserta(GS_BIDLE, bidle->quest.user[i].user, "oro", "%li", BDato(bidle->quest.user[i].user, "oro")+oros);
			ircfree(bidle->quest.user[i].user);
		}
		bidle->quest.tipo = 0;
		bidle->quest.tiempo = time(0)+3600;
	}
	return 0;
}
HDIRFUNC(BidleLeeHDir)
{
	return 200;
}
