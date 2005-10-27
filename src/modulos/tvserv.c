/*
 * $Id: tvserv.c,v 1.7 2005-10-27 19:16:15 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/tvserv.h"

TvServ tvserv;
#define ExFunc(x) BuscaFuncion(tvserv.hmod, x, NULL)

SOCKFUNC(TSAbreProg);
SOCKFUNC(TSLeeProg);
SOCKFUNC(TSCierraProg);
SOCKFUNC(TSAbreHoroscopo);
SOCKFUNC(TSLeeHoroscopo);
SOCKFUNC(TSCierraHoroscopo);
BOTFUNC(TSTv);
BOTFUNC(TSHelp);
BOTFUNC(TSHoroscopo);
Sock *prog = NULL, *hor = NULL;
Cadena *viendo = NULL;
int horosc = 1;
int TSSigSQL();

static bCom tvserv_coms[] = {
	{ "help" , TSHelp , USERS } ,
	{ "tv" , TSTv , USERS } ,
	{ "horoscopo" , TSHoroscopo , USERS } ,
	{ 0x0 , 0x0 , 0x0 }
};

void TSSet(Conf *, Modulo *);
int TSTest(Conf *, int *);

ModInfo MOD_INFO(TvServ) = {
	"TvServ" ,
	0.3 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};

Cadena cadenas[] = {
	{ "TVE1" , "TVE1" , 'n' } ,
	{ "TVE2" , "TVE2" , 'n' } ,
	{ "T5" , "Tele+5" , 'n' } ,
	{ "C+" , "Canal+Plus" , 'n' } ,
	{ "A3" , "Antena+3" , 'n' } ,
	{ "Canal2" , "Canal+2" , 'a' } ,
	{ "Canal9" , "Canal+9" , 'a' } ,
	{ "TV3" , "TV3" , 'a' } ,
	{ "Canal33" , "Canal+33" , 'a' } ,
	{ "TVG" , "TVG" , 'a' } ,
	{ "ETB1" , "ETB+1" , 'a' } ,
	{ "ETB2" , "ETB+2" , 'a' } ,
	{ "PUNT2" , "PUNT+2" , 'a' } ,
	{ "CanalSur", "Canal+Sur" , 'a' } ,
	{ "Telemadrid" , "TVAM" , 'a' } ,
	{ "TVCanaria" , "TV+Canaria" , 'a' } ,
	{ "LaMancha" , "Castilla-La+Mancha" , 'a' } ,
	{ NULL, NULL, 0 }
};
char *horoscopos[] = {
	"Aries" ,
	"Tauro" ,
	"Geminis" ,
	"Cancer" ,
	"Leo" ,
	"Virgo" ,
	"Libra" ,
	"Escorpio" ,
	"Sagitario" ,
	"Capricornio" ,
	"Acuario" ,
	"Piscis" ,
	NULL
};
Cadena *TSBuscaCadena(char *nombre)
{
	int i;
	for (i = 0; cadenas[i].nombre; i++)
	{
		if (!strcasecmp(nombre, cadenas[i].nombre))
			return &cadenas[i];
	}
	return NULL;
}
char *TSBuscaHoroscopo(char *horo)
{
	int i;
	for (i = 0; horoscopos[i]; i++)
	{
		if (!strcasecmp(horoscopos[i], horo))
			return horoscopos[i];
	}
	return NULL;
}
int MOD_CARGA(TvServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, MOD_INFO(TvServ).nombre);
		errores++;
	}
	else
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(TvServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(TvServ).nombre))
			{
				if (!TSTest(modulo.seccion[0], &errores))
					TSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(TvServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(TvServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	return errores;
}
int MOD_DESCARGA(TvServ)()
{
	BorraSenyal(SIGN_SQL, TSSigSQL);
	BotUnset(tvserv);
	return 0;
}
int TSTest(Conf *config, int *errores)
{
	short error_parcial = 0;
	return error_parcial;
}
void TSSet(Conf *config, Modulo *mod)
{
	int i;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
			ProcesaComsMod(config->seccion[i], mod, tvserv_coms);
	}
	InsertaSenyal(SIGN_SQL, TSSigSQL);
	BotSet(tvserv);
}
char *TSQuitaTags(char *str, char *dest)
{
	char *c = str;
	while (!BadPtr(c))
	{
		if (*c == '<')
		{
			if (!(c = strchr(c, '>')))
				break;
			c++;
		}
		else if (*c == '&')
		{
			/*if (!strncmp("&nbsp;", c, 6))
			{
				*dest++ = 2;
				*dest++ = ' ';
				c += 6;
			}
			else*/
				*dest++ = *c++;
		}
		else if (*c == '\t')
			break;
		else if (*c == '\'')
		{
			*dest++ = '\\';
			*dest++ = '\'';
			c++;
		}
		else
			*dest++ = *c++;
	}
	*dest = 0;
	return dest;
}
char *TSFecha(time_t ts)
{
	static char TSFecha[9];
	struct tm *timeptr;
	ts -= 21600;
    	timeptr = localtime(&ts);
    	ircsprintf(TSFecha, "%.2d/%.2d/%.2d", timeptr->tm_mday, timeptr->tm_mon + 1, timeptr->tm_year - 100);
    	return TSFecha;
}
void TSActualizaProg()
{
	static int i = 0;
	if (prog)
		return;
	if (!i)
		SQLQuery("TRUNCATE TABLE %s%s", PREFIJO, TS_TV);
	if (cadenas[i].nombre)
	{
		viendo = &cadenas[i];
		if (!(prog = SockOpen("www.teletexto.com", 80, TSAbreProg, TSLeeProg, NULL, TSCierraProg, ADD)))
			return;
		i++;
	}
	else
		i = 0;
}
void TSActualizaHoroscopo()
{
	static int i = 1;
	if (hor)
		return;
	if (i == 1)
		SQLQuery("TRUNCATE TABLE %s%s", PREFIJO, TS_HO);
	if (i <= 12)
	{
		horosc = i++;
		if (!(hor = SockOpen("www.teletexto.com", 80, TSAbreHoroscopo, TSLeeHoroscopo, NULL, TSCierraHoroscopo, ADD)))
			return;
	}
	else
		i = 1;
}
BOTFUNC(TSTv)
{
	SQLRes res;
	SQLRow row;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, "TV cadena");
		return 1;
	}
	if (! TSBuscaCadena(param[1]))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Esta cadena no existe");
		return 1;
	}
	if (!(res = SQLQuery("SELECT programacion from %s%s where fecha='%s' AND LOWER(item)='%s'", PREFIJO, TS_TV, TSFecha(time(NULL)), strtolower(param[1]))))
	{
		TSActualizaProg();
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está siendo actualizado. Inténtelo dentro de unos minutos");
		return 1;
	}
	while ((row = SQLFetchRow(res)))
		Responde(cl, CLI(tvserv), row[0]);
	return 0;
}
BOTFUNC(TSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), "\00312%s\003 es un servicio de ocio para los usuarios que ofrece distintos servicios.", tvserv.hmod->nick);
		Responde(cl, CLI(tvserv), "Todos los servicios se actualizan a diario.");
		Responde(cl, CLI(tvserv), " ");
		if (!IsId(cl))
		{
			Responde(cl, CLI(tvserv), "Debes estar registrado e identificado para poder usar este servicio.");
			return 1;
		}
		Responde(cl, CLI(tvserv), "Servicios prestados:");
		FuncResp(tvserv, "TV", "Programación de las televisiones de España.");
		FuncResp(tvserv, "HOROSCOPO", "Horóscopo del día.");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Para más información, \00312/msg %s %s comando", tvserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "TV") && ExFunc("TV"))
	{
		int i;
		Responde(cl, CLI(tvserv), "\00312TV");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Muestra la programación de las distintas cadenas de la televisión de España.");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Cadenas disponibles:");
		for (i = 0; cadenas[i].nombre; i++)
			Responde(cl, CLI(tvserv), cadenas[i].nombre);
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Sintaxis: \00312TV cadena");
	}
	else if (!strcasecmp(param[1], "HOROSCOPO") && ExFunc("HOROSCOPO"))
	{
		int i = 0;
		Responde(cl, CLI(tvserv), "\00312HOROSCOPO");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Muestra la predicción del día del signo zodiacal que se indique.");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Zodiacos disponibles:");
		while (horoscopos[i])
			Responde(cl, CLI(tvserv), horoscopos[i++]);
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Sintaxis: \00312HOROSCOPO signo");
	}
	else
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(TSHoroscopo)
{
	SQLRes res;
	SQLRow row;
	char *h;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, "HOROSCOPO signo");
		return 1;
	}
	if (!(h = TSBuscaHoroscopo(param[1])))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Este horóscopo no existe");
		return 1;
	}
	if (!(res = SQLQuery("SELECT prediccion from %s%s where fecha='%s' AND LOWER(item)='%s'", PREFIJO, TS_HO, TSFecha(time(NULL)), strtolower(param[1]))))
	{
		TSActualizaHoroscopo();
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está siendo actualizado. Inténtelo dentro de unos minutos");
		return 1;
	}
	Responde(cl, CLI(tvserv), "\00312%s", h);
	while ((row = SQLFetchRow(res)))
		Responde(cl, CLI(tvserv), row[0]);
	return 0;
}
SOCKFUNC(TSAbreProg)
{
	SockWrite(prog, OPT_CRLF, "GET /teletexto.asp?c=%s&p=%c&f=%s HTTP/1.1", viendo->texto, viendo->tipo, TSFecha(time(NULL)));
	SockWrite(prog, OPT_CRLF, "Accept: */*");
	SockWrite(prog, OPT_CRLF, "Host: www.teletexto.com");
	SockWrite(prog, OPT_CRLF, "Connection: Keep-alive");
	SockWrite(prog, OPT_CRLF, "");
	SockWrite(prog, OPT_CRLF, "");
	return 0;
}
SOCKFUNC(TSLeeProg)
{
	static char imp = 0, txt[BUFSIZE];
	if (!imp && !strncmp("<td width=12", data, 12))
	{
		imp = 1;
		txt[0] = '\0';
	}
	else if (imp && !strcmp(data, "\t"))
	{
		imp = 0;
		SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_TV, viendo->nombre, TSFecha(time(NULL)), txt[0] == ' ' ? &txt[1] : &txt[0]);
	}
	else if (strstr(data, "</html>"))
		SockClose(prog, LOCAL);
	if (imp)
	{
		if (strstr(data, "bgcolor=#C9E7F8") || strstr(data, "bgcolor=#EBF5FC"))
			strcat(txt, " \x02");
		TSQuitaTags(data, buf);
		if (buf[0])
			strcat(txt, buf[0] == ' ' ? &buf[1] : &buf[0]);
	}
	return 0;
}
SOCKFUNC(TSCierraProg)
{
	prog = NULL;
	TSActualizaProg();
	return 0;
}
SOCKFUNC(TSAbreHoroscopo)
{
	SockWrite(hor, OPT_CRLF, "GET /teletexto.asp?horoscopo=%i HTTP/1.1", horosc);
	SockWrite(hor, OPT_CRLF, "Accept: */*");
	SockWrite(hor, OPT_CRLF, "Host: www.teletexto.com");
	SockWrite(hor, OPT_CRLF, "Connection: Keep-alive");
	SockWrite(hor, OPT_CRLF, "");
	SockWrite(hor, OPT_CRLF, "");
	return 0;
}
SOCKFUNC(TSLeeHoroscopo)
{
	static char txt[BUFSIZE], tmp[BUFSIZE];
	char *c;
	static int salud = 0, dinero = 0, amor = 0, cat = 0;
	if ((c = strstr(data, "d><f")))
	{
		txt[0] = '\0';
		cat = 1;
		TSQuitaTags(strchr(c+10, '>')+1, tmp);
		strcat(txt, tmp);
	}
	else if (cat)
	{
		TSQuitaTags(data, tmp);
		strcat(txt, tmp);
		if (strstr(data, "</center>"))
		{
			cat = 0;
			SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_HO, horoscopos[horosc-1], TSFecha(time(NULL)), txt);
		}
	}
	else if (strstr(data, "dinero.gif"))
		dinero = StrCount(data, "dinero.gif");
	else if (strstr(data, "salud.gif"))
		salud = StrCount(data, "salud.gif");
	else if (strstr(data, "amor.gif"))
		amor = StrCount(data, "amor.gif");
	if (!strcmp(data, "</td></tr><tr><td>"))
	{
		ircsprintf(txt, "Salud: \00312%s\003 - Dinero: \00312", Repite('*', salud));
		strcat(txt, Repite('*', dinero));
		strcat(txt, "\003 - Amor: \00312");
		strcat(txt, Repite('*', amor));
		SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_HO, horoscopos[horosc-1], TSFecha(time(NULL)), txt);
		salud = dinero = amor = 0;
		SockClose(hor, LOCAL);
	}
	return 0;
}
SOCKFUNC(TSCierraHoroscopo)
{
	hor = NULL;
	TSActualizaHoroscopo();
	return 0;
}
int TSSigSQL()
{
	if (!SQLEsTabla(TS_TV))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"fecha varchar(255) default NULL, "
  			"programacion text default NULL "
			");", PREFIJO, TS_TV))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, TS_TV);
	}
	if (!SQLEsTabla(TS_HO))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"fecha varchar(255) default NULL, "
  			"prediccion text default NULL "
			");", PREFIJO, TS_HO))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, TS_HO);
	}
	SQLCargaTablas();
	return 1;
}
