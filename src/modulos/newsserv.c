/*
 * $Id: newsserv.c,v 1.16 2008/02/14 21:32:11 Trocotronic Exp $
 */

#define XML_STATIC
#include <expat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/newsserv.h"
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"

NewsServ *newsserv = NULL;
Noticia *noticias = NULL;
Timer *timerrss = NULL;

void WSSet(Conf *, Modulo *);
int WSTest(Conf *, int *);
int DescargaRSS();
int WSSigSQL		();
int WSEmiteRSS (Proc *);
char *WSEntities(char *, int *);
int WSSigDrop(char *);

SOCKFUNC(WSAbre);
SOCKFUNC(WSLee);
SOCKFUNC(WSCierra);
BOTFUNC(WSHelp);
BOTFUNC(WSAlta);
BOTFUNCHELP(WSHAlta);
BOTFUNC(WSBaja);
BOTFUNCHELP(WSHBaja);
BOTFUNC(Prueba);

static bCom newsserv_coms[] = {
	{ "help" , WSHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "alta" , WSAlta , N1 , "Te da de alta en el servicio de noticias." , WSHAlta } ,
	{ "baja" , WSBaja , N1 , "Te da de baja en el servicio de noticias." , WSHBaja } ,
	{ "noticias" , Prueba , N1 , "ble" , NULL } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

#define MAX_RSS 23

Opts urls[MAX_RSS+1] = {
	{ 0x1 , "" } ,
	{ 0x2 , "ultima_hora" } ,
	{ 0x4 , "internacional" } ,
	{ 0x8 , "espana" } ,
	{ 0x10 , "deportes" } ,
	{ 0x20 , "coruna" } ,
	{ 0x40 , "alicante" } ,
	{ 0x80 , "barcelona" } ,
	{ 0x100 , "bilbao" } ,
	{ 0x200 , "cordoba" } ,
	{ 0x400 , "granada" } ,
	{ 0x800 , "madrid" } ,
	{ 0x1000 , "malaga" } ,
	{ 0x2000 , "murcia" } ,
	{ 0x4000 , "sevilla" } ,
	{ 0x8000 , "valencia" } ,
	{ 0x10000 , "valladolid" } ,
	{ 0x20000 , "vigo" } ,
	{ 0x40000 , "zaragoza" } ,
	{ 0x80000 , "dinero" } ,
	{ 0x100000 , "gente" } ,
	{ 0x200000 , "tecnologia_internet" } ,
	{ 0x400000 , "ocio" } ,
	{ 0x0 , 0x0 }
};
char *trads[] = {
	"General" , "Última Hora" , "Internacional" , "España" , "Deportes" ,
	"Coruña" , "Alicante" , "Barcelona" , "Bilbao" , "Córdoba" , "Granada" ,
	"Madrid" , "Málaga" , "Murcia" , "Sevilla" , "Valencia" , "Valladolid" ,
	"Vigo" , "Zaragoza" , "Dinero" , "Gente" , "Tecnología e Internet" , "Ocio"
};

Rss *rss[MAX_RSS];

ModInfo MOD_INFO(NewsServ) = {
	"NewsServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(NewsServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(NewsServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(NewsServ).nombre))
			{
				if (!WSTest(modulo.seccion[0], &errores))
					WSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(NewsServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(NewsServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		WSSet(NULL, mod);
	timerrss = IniciaCrono(0, 300, DescargaRSS, NULL);
	DescargaRSS();
	return errores;
}
int MOD_DESCARGA(NewsServ)()
{
	int i;
	for (i = 0; i < MAX_RSS; i++)
	{
		if (rss[i])
		{
			SockClose(rss[i]->sck);
			if (rss[i]->fp > 0)
				close(rss[i]->fp);
			ircsprintf(tokbuf, "tmp/%s.xml", urls[rss[i]->opt].item);
			unlink(tokbuf);
			if (rss[i]->pxml)
				XML_ParserFree(rss[i]->pxml);
			Free(rss[i]);
		}
	}
	BotUnset(newsserv);
	if (timerrss)
	{
		ApagaCrono(timerrss);
		timerrss = NULL;
	}
	BorraSenyal(SIGN_SQL, WSSigSQL);
	BorraSenyal(CS_SIGN_DROP, WSSigDrop);
	BorraSenyal(NS_SIGN_DROP, WSSigDrop);
	DetieneProceso(WSEmiteRSS);
	return 0;
}
int WSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{

	}
	*errores += error_parcial;
	return error_parcial;
}
void WSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!newsserv)
		newsserv = BMalloc(NewsServ);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, newsserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, newsserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, newsserv_coms);
	InsertaSenyal(SIGN_SQL, WSSigSQL);
	InsertaSenyal(CS_SIGN_DROP, WSSigDrop);
	InsertaSenyal(NS_SIGN_DROP, WSSigDrop);
	for (i = 0; i < MAX_RSS; i++)
		rss[i] = NULL;
	BotSet(newsserv);
}
Noticia *BuscaNoticia(u_int id, Noticia *lugar)
{
	Noticia *aux;
	for (aux = lugar; aux; aux = aux->sig)
	{
		if (aux->id == id)
			return aux;
	}
	return NULL;
}
Noticia *InsertaNoticia(u_int id, char *titular, char *descripcion, u_int servicio)
{
//	Noticia *aux;
//	if (!BuscaNoticia(id, anteriores) && (!(aux = BuscaNoticia(id, noticias)) || BadPtr(aux->descripcion)))
//	{
		Noticia *not;
		not = BMalloc(Noticia);
		not->id = id;
		strlcpy(not->titular, titular, sizeof(not->titular));
		if (!BadPtr(descripcion))
			strlcpy(not->descripcion, descripcion, sizeof(not->descripcion));
		not->servicio = servicio;
		AddItem(not, noticias);
		return not;
//	}
//	return NULL;
}
void XMLCALL xmlInicio(Rss *rs, const XML_Char *nombre, const XML_Char **atts)
{
	int i;
	if (!strcmp(nombre, "item"))
	{
		for (i = 0; atts[i]; i++)
		{
			if (!strcmp(atts[i], "rdf:about"))
			{
				int j;
				u_int id;
				char *c, *d, *e;
				e = c = strdup(atts[i+1]);
				for (j = 0; j < 4; j++)
					c = strchr(c, '/')+1;
				d = strchr(c, '/');
				*d = '\0';
				id = atoi(c);
				rs->not.id = id;
				rs->not.t = &(rs->not.titular[0]);
				rs->not.d = &(rs->not.descripcion[0]);
				Free(e);
			}
		}
	}
	else if (!strcmp(nombre, "rdf:RDF"))
		rs->not.id = 0;
	else if (!strcmp(nombre, "title"))
		rs->tipo = 1;
	else if (!strcmp(nombre, "description"))
		rs->tipo = 2;
	else if (!strcmp(nombre, "link"))
		rs->tipo = 3;
	else if (!strcmp(nombre, "dc:date"))
		rs->tipo = 4;
}
void XMLCALL xmlFin(Rss *rs, const char *nombre)
{
	if (!strcmp(nombre, "item"))
	{
		if (rs->ids)
		{
			int i;
			for (i = 0; rs->ids[i]; i++)
			{
				if (rs->ids[i] == rs->not.id)
					break;
			}
			if (!rs->ids[i])
			{
				char tmp[BUFSIZE];
				CambiarCharset("UTF-8", "ISO-8859-1", rs->not.titular, tmp, BUFSIZE);
				strlcpy(rs->not.titular, tmp, sizeof(rs->not.titular));
				CambiarCharset("UTF-8", "ISO-8859-1", rs->not.descripcion, tmp, BUFSIZE);
				strlcpy(rs->not.descripcion, tmp, sizeof(rs->not.descripcion));
				InsertaNoticia(rs->not.id, rs->not.titular, rs->not.descripcion, rs->servicio);
				/*for (cl = clientes; cl; cl = cl->sig)
				{
					if (EsCliente(cl))
					{
						Responde(cl, CLI(newsserv), "\x1F%s", not.titular);
						Responde(cl, CLI(newsserv), strchr(not.descripcion, '-')+2);
						Responde(cl, CLI(newsserv), " ");
						Responde(cl, CLI(newsserv), not.link);
					}
				}
				for (cn = canales; cn; cn = cn->sig)
				{
					Responde((Cliente *)cn, CLI(newsserv), " ");
					Responde((Cliente *)cn, CLI(newsserv), "\x1F%s", rs->not.titular);
					if (!BadPtr(rs->not.descripcion))
						Responde((Cliente *)cn, CLI(newsserv), rs->not.descripcion);
					Responde((Cliente *)cn, CLI(newsserv), "\x1F\00312http://noticias.redyc.com/?%u", rs->not.id);
				}*/
			}
		}
		if (rs->q < rs->items)
			rs->tmp[rs->q++] = rs->not.id;
	}
	else if (!strcmp(nombre, "rdf:RDF"))
	{
		if (rs->ids)
			Free(rs->ids);
		rs->ids = (u_int *)Malloc(sizeof(u_int) * (rs->items+1));
		for (rs->q = 0; rs->q < rs->items; rs->q++)
			rs->ids[rs->q] = rs->tmp[rs->q];
		rs->ids[rs->q] = 0;
		Free(rs->tmp);
		rs->q = rs->items = 0;
	}
	else if (!strcmp(nombre, "rdf:li"))
		rs->items++;
	else if (!strcmp(nombre, "rdf:Seq"))
		rs->tmp = (u_int *)Malloc(sizeof(u_int) * rs->items);
}
void XMLCALL xmlData(Rss *rs, const char *s, int len)
{
	if (len > 1 && *s != 9 && rs->not.id)
	{
		int min;
		switch(rs->tipo)
		{
			case 1:
				min = MIN(len, rs->not.titular + sizeof(rs->not.titular) - rs->not.t);
				strncpy(rs->not.t, s, min);
				rs->not.t += min;
				break;
			case 2:
				min = MIN(len, rs->not.descripcion + sizeof(rs->not.descripcion) - rs->not.d);
				strncpy(rs->not.d, s, min);
				rs->not.d += min;
				break;
		}
	}
}
int DescargaRSS()
{
	int i;
	SQLRes *res;
	if (!(res = SQLQuery("SELECT * FROM %s%s", PREFIJO, WS_SQL)) || !SQLNumRows(res))
	{
		SQLFreeRes(res);
		return 0;
	}
	for (i = 0; i < MAX_RSS; i++)
	{
		Rss *tmp;
		if (!rss[i])
			tmp = BMalloc(Rss);
		else
			tmp = rss[i];
		tmp->pxml = XML_ParserCreate(NULL);
		tmp->opt = i;
		XML_SetStartElementHandler(tmp->pxml, xmlInicio);
		XML_SetEndElementHandler(tmp->pxml, xmlFin);
		XML_SetCharacterDataHandler(tmp->pxml, xmlData);
		XML_SetUserData(tmp->pxml, tmp);
		tmp->servicio = urls[i].opt;
		tmp->sck = SockOpenEx("www.20minutos.es", 80, WSAbre, WSLee, NULL, WSCierra, 30, 30, OPT_NORECVQ);
		rss[i] = tmp;
	}
	/*FILE *fp;
	int len;
	IniciaXML();
	fp = fopen("datos.xml", "r");
	while ((len = fread(buf, 1, 512, fp)))
	{
		if (!XML_Parse(pxml, buf, len, feof(fp)))
		{
#ifdef DEBUG
			Debug("error %s en %i %i", XML_ErrorString(XML_GetErrorCode(pxml)), XML_GetCurrentLineNumber(pxml), XML_GetCurrentColumnNumber(pxml));
#endif
			break;
		}
	}
	fclose(fp);
	XML_ParserFree(pxml);*/
	return 0;
}
BOTFUNC(WSAlta)
{
	char *t = NULL;
	u_int opt, opts = 0x0, i = 1;
	if (params > 1 && *param[1] == '#')
	{
		if (!CSEsFundador(cl, param[1]))
		{
			Responde(cl, CLI(newsserv), WS_ERR_EMPT, "Sólo puede dar de alta un canal el propio fundador.");
			return 1;
		}
		t = param[1];
		i = 2;
	}
	if (i == params)
		opts = 0x1;
	else
	{
		for (; i < (u_int)params; i++)
		{
			if (!strcasecmp(param[i], "TODOS"))
			{
				opts = 0xFFFFFFFF;
				break;
			}
			else if (!strcasecmp(param[i], "GENERAL"))
				opt = 0x1;
			else if (!(opt = BuscaOpt(param[i], urls)))
			{
				ircsprintf(buf, "No se encuentra el servicio de %s", param[i]);
				Responde(cl, CLI(newsserv), WS_ERR_EMPT, buf);
				return 1;
			}
			opts |= opt;
		}
	}
	SQLInserta(WS_SQL, t ? t : cl->nombre, "servicios", "%u", opts);
	if (t)
		Responde(cl, CLI(newsserv), "El canal \00312%s\003 ha sido dado de alta en el servicio de noticias.", t);
	else
		Responde(cl, CLI(newsserv), "Has sido dado de alta en el servicio de noticias. Te llegarán por privado.");
	return 0;
}
BOTFUNC(WSBaja)
{
	char *t = NULL, *s;
	u_int opt, opts, i = 1;
	if (params > 1 && *param[1] == '#')
	{
		if (!CSEsFundador(cl, param[1]))
		{
			Responde(cl, CLI(newsserv), WS_ERR_EMPT, "Sólo puede dar de baja un canal el propio fundador.");
			return 1;
		}
		t = param[1];
		i = 2;
	}
	if (!(s = SQLCogeRegistro(WS_SQL, t ? t : cl->nombre, "servicios")))
	{
		Responde(cl, CLI(newsserv), WS_ERR_EMPT, "No parece que esté dado de alta.");
		return 1;
	}
	opts = atoul(s);
	if (i == params)
		opts = 0x0;
	else
	{
		for (; i < (u_int)params; i++)
		{
			if (!strcasecmp(param[i], "TODOS"))
			{
				opts = 0x0;
				break;
			}
			else if (!strcasecmp(param[i], "GENERAL"))
				opt = 0x1;
			else if (!(opt = BuscaOpt(param[i], urls)))
			{
				ircsprintf(buf, "No se encuentra el servicio de %s", param[i]);
				Responde(cl, CLI(newsserv), WS_ERR_EMPT, buf);
				return 1;
			}
			opts &= ~opt;
		}
	}
	if (!opts)
	{
		SQLBorra(WS_SQL, t ? t : cl->nombre);
		if (t)
			Responde(cl, CLI(newsserv), "El canal \00312%s\003 ha sido dado de baja en el servicio de noticias.", t);
		else
			Responde(cl, CLI(newsserv), "Has sido dado de baja en el servicio de noticias.");
	}
	else
	{
		SQLInserta(WS_SQL, t ? t : cl->nombre, "servicios", "%u", opts);
		if (t)
			Responde(cl, CLI(newsserv), "El canal \00312%s\003 ha sido dado de baja de algunos servicios de noticias.", t);
		else
			Responde(cl, CLI(newsserv), "Has sido dado de baja de algunos servicios de noticias.");
	}
	return 0;
}
BOTFUNC(WSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(newsserv), "\00312%s\003 es un servicio de información general.", newsserv->hmod->nick);
		Responde(cl, CLI(newsserv), "Te permite estar al día de las noticias que ocurren en el mundo en tiempo real y de forma instantánea.");
		Responde(cl, CLI(newsserv), "También puedes decidir sobre qué temas quieres estar informado, por ejemplo Deportes o Prensa Rosa.");
		Responde(cl, CLI(newsserv), "Y si tienes un canal propio, también puedes decidir que toda la gente de tu canal se percate de lo que sucede.");
		Responde(cl, CLI(newsserv), " ");
		Responde(cl, CLI(newsserv), "Comandos disponibles:");
		ListaDescrips(newsserv->hmod, cl);
		Responde(cl, CLI(newsserv), " ");
		Responde(cl, CLI(newsserv), "Para más información, \00312/msg %s %s comando", newsserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], newsserv->hmod, param, params))
		Responde(cl, CLI(newsserv), WS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNCHELP(WSHAlta)
{
	int i;
	Responde(cl, CLI(newsserv), "Te da de alta o da de alta un canal tuyo en el servicio de noticias.");
	Responde(cl, CLI(newsserv), "Cuando se produzca una noticia de tu interés, serás informado vía privado.");
	Responde(cl, CLI(newsserv), "Si das de alta un canal, la noticia se emitirá por el canal.");
	Responde(cl, CLI(newsserv), "Sólo puedes dar de alta un canal si eres el founder del mismo.");
	Responde(cl, CLI(newsserv), "Existen varios tipos de noticias. Puedes escoger cuáles prefieres darte de alta.");
	Responde(cl, CLI(newsserv), "Por ejemplo, puedes escoger los servicios de Deportes y Ocio si así lo prefieres.");
	Responde(cl, CLI(newsserv), " ");
	Responde(cl, CLI(newsserv), "Este comando también te permite modificar los servicios de los que habías sido dado de alta.");
	Responde(cl, CLI(newsserv), " ");
	Responde(cl, CLI(newsserv), "Las distintas secciones de noticias son:");
	Responde(cl, CLI(newsserv), "- general");
	for (i = 1; i < MAX_RSS; i++)
		Responde(cl, CLI(newsserv), "- %s", urls[i].item);
	Responde(cl, CLI(newsserv), "- todos");
	Responde(cl, CLI(newsserv), " ");
	Responde(cl, CLI(newsserv), "Sintaxis: \00312ALTA [#canal] [seccion1 [ ... seccionN ] ]");
	Responde(cl, CLI(newsserv), "Ejemplo: \00312ALTA [#micanal] ocio deportes");
	Responde(cl, CLI(newsserv), "Si no especificas ningún parámetro, se darán de alta por defecto en la sección general.");
	Responde(cl, CLI(newsserv), "La sección \"todos\" da de alta en todos los servicios de noticias. No hace falta especificarlos todos.");
	return 0;
}
BOTFUNCHELP(WSHBaja)
{
	Responde(cl, CLI(newsserv), "Da de baja en el servicio de noticias.");
	Responde(cl, CLI(newsserv), "Puedes especificar darte de varia en un servicio, en varios o en todos.");
	Responde(cl, CLI(newsserv), "Si no especificas ningún parámetro, se asumirá que quieres darte de baja por completo.");
	Responde(cl, CLI(newsserv), "Análogamente, puedes especificar un canal si eres el fundador del mismo.");
	Responde(cl, CLI(newsserv), " ");
	Responde(cl, CLI(newsserv), "Sintaxis: \00312BAJA [#canal] [sección1 [... seccionN ] ]");
	Responde(cl, CLI(newsserv), "Ejemplo: \00312BAJA [#micanal] ocio ultima_hora");
	return 0;
}
Rss *BuscaRSS(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_RSS; i++)
	{
		if (rss[i] && rss[i]->sck == sck)
			return rss[i];
	}
	return NULL;
}
SOCKFUNC(WSAbre)
{
	Rss *rs;
	if (!(rs = BuscaRSS(sck)))
		return 1;
	ircsprintf(buf, "tmp/%s.xml", urls[rs->opt].item);
	rs->fp = open(buf, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
	SockWrite(sck, "GET /rss/%s/ HTTP/1.1", urls[rs->opt].item);
	SockWrite(sck, "Accept: */*");
	SockWrite(sck, "Host: www.20minutos.es");
	SockWrite(sck, "Connection: close");
	SockWrite(sck, "");
	return 0;
}
SOCKFUNC(WSLee)
{
	char *c;
	Rss *rs;
	if (!(rs = BuscaRSS(sck)))
		return 1;
	c = data;
	if (!strncmp(c, "HTTP/1.", 7))
	{
		while ((c = strchr(c, '\n')))
		{
			c++;
			if (*c == '\r' || *c == '\n')
			{
				c++;
				if (*c == '\r')
					c += 2;
				else if (*c == '\n')
					c += 1;
				break;
			}
		}
	}
	write(rs->fp, c, strlen(c));
	return 0;
}
char *iso_8859_1[] = {
	"nbsp", "iexcl", "cent", "pound", "curren", "yen", "brvbar",
	"sect", "uml", "copy", "ordf", "laquo", "not", "shy", "reg",
	"macr", "deg", "plusmn", "sup2", "sup3", "acute", "micro",
	"para", "middot", "cedil", "sup1", "ordm", "raquo", "frac14",
	"frac12", "frac34", "iquest", "Agrave", "Aacute", "Acirc",
	"Atilde", "Auml", "Aring", "AElig", "Ccedil", "Egrave",
	"Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc",
	"Iuml", "ETH", "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde",
	"Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
	"Yacute", "THORN", "szlig", "agrave", "aacute", "acirc",
	"atilde", "auml", "aring", "aelig", "ccedil", "egrave",
	"eacute", "ecirc", "euml", "igrave", "iacute", "icirc",
	"iuml", "eth", "ntilde", "ograve", "oacute", "ocirc", "otilde",
	"ouml", "divide", "oslash", "ugrave", "uacute", "ucirc",
	"uuml", "yacute", "thorn", "yuml"
};
char *WSEntities(char *str, int *len)
{
	int i, j, k, l;
	for (i = k = 0; i < *len; i++)
	{
		if (str[i] == '&')
		{
			i++;
			if (str[i] == 'a' && str[i+1] == 'm' && str[i+2] == 'p' && str[i+3] == ';')
				i += 4;
			if (str[i] == 'q' && str[i+1] == 'u' && str[i+2] == 'o' && str[i+3] == 't' && str[i+4] == ';')
			{
				i += 4;
				str[k++] = '"';
				continue;
			}
			for (j = 160; j < 256; j++)
			{
				l = strlen(iso_8859_1[j-160]);

				if (str[i+l] == ';' && !strncmp(iso_8859_1[j-160], &str[i], l))
				{
					str[k++] = j;
					i += l;
					break;
				}
			}
		}
		else
			str[k++] = str[i];
	}
	*len = k;
	return str;
}
SOCKFUNC(WSCierra)
{
	int l;
	static int total = 0;
	char *tmp;
	Rss *rs;
	struct stat inode;
	if (data)
		return 1;
	if (!(rs = BuscaRSS(sck)))
		return 1;
	if (rs->fp > 0 && fstat(rs->fp, &inode) != -1)
	{
		lseek(rs->fp, 0, SEEK_SET);
		tmp = (char *)Malloc(sizeof(char) * (inode.st_size+1));
		tmp[inode.st_size] = '\0';
		ircsprintf(tokbuf, "tmp/%s.xml", urls[rs->opt].item);
		if ((l = read(rs->fp, tmp, inode.st_size)) == inode.st_size)
		{
			WSEntities(tmp, &l);
			if (!XML_Parse(rs->pxml, tmp, l, 1))
			{
			//	Debug("error %s (%s) en %i %i", XML_ErrorString(XML_GetErrorCode(rs->pxml)), tokbuf, XML_GetCurrentLineNumber(rs->pxml), XML_GetCurrentColumnNumber(rs->pxml));
			//	Debug("%s", buf);
			}
		}
		close(rs->fp);
		unlink(tokbuf);
		Free(tmp);
	}
	XML_ParserFree(rs->pxml);
	rs->fp = -1;
	rs->sck = NULL;
	rs->pxml = NULL;
	if (++total == MAX_RSS)
	{
		if (noticias)
			IniciaProceso(WSEmiteRSS);
		total = 0;
	}
	return 0;
}
int WSSigSQL()
{
	SQLNuevaTabla(WS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255), "
  		"servicios int4, "
  		"KEY item (item) "
		");", PREFIJO, WS_SQL);
	return 0;
}
int WSEmiteRSS(Proc *proc)
{
	SQLRes res;
	SQLRow row;
	u_int servicios;
	Cliente *cl;
	Canal *cn;
	Noticia *not, *sig;
	int i;
	if (!(res = SQLQuery("SELECT item,servicios from %s%s LIMIT 30 OFFSET %i ", PREFIJO, WS_SQL, proc->proc)) || !SQLNumRows(res))
	{
		for (not = noticias; not; not = sig)
		{
			sig = not->sig;
			Free(not);
		}
		//anteriores = noticias;
		noticias = NULL;
		DetieneProceso(proc->func);
		return 1;
	}
	while ((row = SQLFetchRow(res)))
	{
		if (*row[0] == '#')
		{
			if (!(cn = BuscaCanal(row[0])))
				continue;
			cl = (Cliente *)cn;
		}
		else
		{
			if (!(cl = BuscaCliente(row[0])))
				continue;
		}
		servicios = (u_int)atoul(row[1]);
		for (not = noticias; not; not = not->sig)
		{
			for (i = 0; i < MAX_RSS; i++)
			{
				if (urls[i].opt == not->servicio)
					break;
			}
			if (i == MAX_RSS) /* nunca debería pasar */
				break;
			if (not->servicio & servicios)
			{
				Responde((Cliente *)cl, CLI(newsserv), " ");
				Responde((Cliente *)cl, CLI(newsserv), "\x02%s\x02:", trads[i]);
				Responde((Cliente *)cl, CLI(newsserv), "%s", not->titular);
				if (!BadPtr(not->descripcion))
					Responde((Cliente *)cl, CLI(newsserv), "%s", not->descripcion);
				Responde((Cliente *)cl, CLI(newsserv), "\x1F\00312http://noticias.redyc.com/?%u", not->id);
			}
		}
	}
	proc->proc += 30;
	SQLFreeRes(res);
	return 0;
}
BOTFUNC(Prueba)
{
	int i, len;
	Rss *rs;
	struct stat inode;
	char *tmp;
	for (i = 0; i < MAX_RSS; i++)
	{
		rs = rss[i];
		rs->pxml = XML_ParserCreate(NULL);
		XML_SetStartElementHandler(rs->pxml, xmlInicio);
		XML_SetEndElementHandler(rs->pxml, xmlFin);
		XML_SetCharacterDataHandler(rs->pxml, xmlData);
		XML_SetUserData(rs->pxml, rs);
		ircsprintf(tokbuf, "tmp/%s.xml", urls[rs->opt].item);
		rs->fp = open(tokbuf, O_RDONLY|O_BINARY, S_IREAD);
		if (rs->fp > 0 && fstat(rs->fp, &inode) != -1)
		{
			lseek(rs->fp, 0, SEEK_SET);
			tmp = (char *)Malloc(sizeof(char) * (inode.st_size+1));
			tmp[inode.st_size] = '\0';
			if ((len = read(rs->fp, tmp, inode.st_size)) == inode.st_size)
			{
				WSEntities(tmp, &len);
				if (!XML_Parse(rs->pxml, tmp, len, 1))
				{
				//	Debug("error %s (%s) en %i %i", XML_ErrorString(XML_GetErrorCode(rs->pxml)), tokbuf, XML_GetCurrentLineNumber(rs->pxml), XML_GetCurrentColumnNumber(rs->pxml));
				//	Debug("%s", buf);
				}
			}
			close(rs->fp);
			//unlink(tokbuf);
			Free(tmp);
		}
		XML_ParserFree(rs->pxml);
		rs->fp = -1;
		rs->sck = NULL;
		rs->pxml = NULL;
	}
	if (noticias)
		IniciaProceso(WSEmiteRSS);
	return 0;
}
int WSSigDrop(char *dropado)
{
	SQLBorra(WS_SQL, dropado);
	return 0;
}
