/*
 * $Id: statserv.c,v 1.27 2007-06-03 18:20:08 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <time.h>
#include <sys/time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/statserv.h"
#include "httpd.h"

StatServ *statserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, statserv->hmod, NULL)
HDir *hdtest = NULL;
ListCl *cltld = NULL, *clver = NULL;
char *fecha_fmt = "%a, %d %b %Y %H:%M:%S GMT";
char *hora_fmt = "%H:%M:%S GMT";
char *sem_fmt = "%a, %H:%M:%S GMT";
char *diasem_fmt = "%a %d, %H:%M:%S GMT";
BOTFUNC(SSReplyVer);
BOTFUNC(SSHelp);
BOTFUNC(SSStats);
BOTFUNCHELP(SSHStats);

static bCom statserv_coms[] = {
	{ "\1VERSION" , SSReplyVer , N0 , "Respuesta reply" , NULL } ,
	{ "HELP" , SSHelp , N0 , "Muestra la ayuda" , NULL } ,
	{ "STATS" , SSStats , N0 , "Muestra las estadísticas globales de la red" , SSHStats } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};
char *vers[] = {
	"mIRC",
	"xchat",
	"irssi",
	"Colloquy",
	"BitchX",
	NULL
};
char *tlds[] = {
	"zw", "Zimbabwe", 
	"zm", "Zambia",
	"zr", "Zaire",
	"yu", "Yugoslavia",
	"ye", "Yemen",
	"eh", "Western Sahara",
	"wf", "Wallis and Futuna Islands",
	"vi", "Virgin Islands - US",
	"vg", "Virgin Islands - British",
	"vn", "Vietnam",
	"ve", "Venezuela",
	"va", "Vatican City State",
	"vu", "Vanuatu",
	"uz", "Uzbekistan",
	"uy", "Uruguay",
	"um", "United States Minor Outlying Islands",
	"us", "United States",
	"uk", "United Kingdom",
	"ae", "United Arab Emirates",
	"ua", "Ukraine",
	"ug", "Uganda",
	"tv", "Tuvalu",
	"tc", "Turks and Caicos Islands",
	"tm", "Turkmenistan",
	"tr", "Turkey",
	"tn", "Tunisia",
	"tt", "Trinidad and Tobago",
	"to", "Tonga",
	"tk", "Tokelau",
	"tg", "Togo",
	"th", "Thailand",
	"tz", "Tanzania",
	"tj", "Tajikistan",
	"tw", "Taiwan",
	"sy", "Syria",
	"ch", "Switzerland",
	"se", "Sweden",
	"sz", "Swaziland",
	"sj", "Svalbard and Jan Mayen Islands",
	"sr", "Suriname",
	"sd", "Sudan",
	"pm", "St. Pierre and Miquelon",
	"sh", "St. Helena",
	"lk", "Sri Lanka",
	"es", "Spain",
	"gs", "South Georgia and the South Sandwich Islands",
	"za", "South Africa",
	"so", "Somalia",
	"sb", "Solomon Islands",
	"si", "Slovenia",
	"sk", "Slovakia",
	"sg", "Singapore",
	"sl", "Sierra Leone",
	"sc", "Seychelles",
	"sn", "Senegal",
	"sa", "Saudi Arabia",
	"st", "Sao Tome and Principe",
	"sm", "San Marino",
	"ws", "Samoa",
	"vc", "Saint Vincent and the Grenadines",
	"lc", "Saint Lucia",
	"kn", "Saint Kitts and Nevis",
	"rw", "Rwanda",
	"ru", "Russia",
	"ro", "Romania",
	"re", "Reunion",
	"qa", "Qatar",
	"pr", "Puerto Rico",
	"pt", "Portugal",
	"pl", "Poland",
	"pn", "Pitcairn",
	"ph", "Philippines",
	"pe", "Peru",
	"py", "Paraguay",
	"pg", "Papua New Guinea",
	"pa", "Panama",
	"ps", "Palestinian Territories",
	"pw", "Palau",
	"pk", "Pakistan",
	"om", "Oman",
	"no", "Norway",
	"mp", "Northern Mariana Islands",
	"nf", "Norfolk Island",
	"nu", "Niue",
	"ng", "Nigeria",
	"ne", "Niger",
	"ni", "Nicaragua",
	"nz", "New Zealand",
	"nc", "New Caledonia",
	"an", "Netherlands Antilles",
	"nl", "Netherlands",
	"np", "Nepal",
	"nr", "Nauru",
	"na", "Namibia",
	"mm", "Myanmar",
	"mz", "Mozambique",
	"ma", "Morocco",
	"ms", "Montserrat",
	"mn", "Mongolia",
	"mc", "Monaco",
	"md", "Moldova",
	"fm", "Micronesia",
	"mx", "Mexico",
	"yt", "Mayotte",
	"mv", "Maldives",
	"mu", "Mauritius",
	"mr", "Mauritania",
	"mq", "Martinique",
	"mh", "Marshall Islands",
	"mt", "Malta",
	"ml", "Mali",
	"my", "Malaysia",
	"mw", "Malawi",
	"mg", "Madagascar",
	"mk", "Macedonia",
	"mo", "Macao",
	"lu", "Luxembourg",
	"lt", "Lithuania",
	"li", "Liechtenstein",
	"ly", "Libyan Arab Jamahiriya",
	"lr", "Liberia",
	"ls", "Lesotho",
	"lb", "Lebanon",
	"lv", "Latvia",
	"la", "Laos",
	"kg", "Kyrgyzstan",
	"kw", "Kuwait",
	"kr", "Korea, Republic of",
	"kp", "Korea, Democratic People's Republic of",
	"ki", "Kiribati",
	"ke", "Kenya",
	"kz", "Kazakhstan",
	"jo", "Jordan",
	"jp", "Japan",
	"jm", "Jamaica",
	"it", "Italy",
	"il", "Israel",
	"im", "Isle of Man",
	"ie", "Ireland",
	"iq", "Iraq",
	"ir", "Iran",
	"id", "Indonesia",
	"in", "India",
	"is", "Iceland",
	"hu", "Hungary",
	"hk", "Hong Kong",
	"hn", "Honduras",
	"hm", "Heard and Mc Donald Islands",
	"ht", "Haiti",
	"gy", "Guyana",
	"gw", "Guinea-Bissau",
	"gn", "Guinea",
	"gt", "Guatemala",
	"gu", "Guam",
	"gp", "Guadeloupe",
	"gd", "Grenada",
	"gl", "Greenland",
	"gr", "Greece",
	"gi", "Gibraltar",
	"gh", "Ghana",
	"de", "Germany",
	"ge", "Georgia",
	"gm", "Gambia",
	"ga", "Gabon",
	"tf", "French Southern Territories",
	"pf", "French Polynesia",
	"gf", "French Guiana",
	"fx", "French, Metropolitan",
	"fr", "France",
	"fi", "Finland",
	"fj", "Fiji",
	"fo", "Faroe Islands",
	"fk", "Falklands Islands",
	"et", "Ethiopia",
	"ee", "Estonia",
	"er", "Eritrea",
	"gq", "Equatorial Guinea",
	"sv", "El Salvador",
	"eg", "Egypt",
	"ec", "Ecuador",
	"tp", "East Timor",
	"do", "Dominican Republic",
	"dm", "Dominica",
	"dj", "Djibouti",
	"dk", "Denmark",
	"cz", "Czech Republic",
	"cy", "Cyprus",
	"cu", "Cuba",
	"hr", "Croatia",
	"ci", "Cote d'Ivoire",
	"cr", "Costa Rica",
	"ck", "Cook Islands",
	"cg", "Congo",
	"km", "Comoros",
	"co", "Colombia",
	"cc", "Cocos (Keeling) Islands",
	"cx", "Christmas Island",
	"cn", "China",
	"cl", "Chile",
	"je", "Channel Islands, Jersey",
	"gg", "Channel Islands, Guernsey",
	"td", "Chad",
	"cf", "Central African Republic",
	"ky", "The Cayman Islands",
	"cv", "Cape Verde",
	"ca", "Canada",
	"cm", "Cameroon",
	"kh", "Cambodia",
	"bi", "Burundi",
	"bf", "Burkina Faso",
	"bg", "Bulgaria",
	"bn", "Brunei",
	"io", "British Indian Ocean Territory",
	"br", "Brazil",
	"bv", "Bouvet Island",
	"bw", "Botswana",
	"ba", "Bosnia and Herzegovina",
	"bo", "Bolivia",
	"bt", "Bhutan",
	"bm", "Bermuda",
	"bj", "Benin",
	"bz", "Belize",
	"be", "Belgium",
	"by", "Belarus",
	"bb", "Barbados",
	"bd", "Bangladesh",
	"bh", "Bahrain",
	"bs", "The Bahamas",
	"az", "Azerbaijan",
	"au", "Australia",
	"at", "Austria",
	"ac", "Ascension Island",
	"aw", "Aruba",
	"am", "Armenia",
	"ar", "Argentina",
	"ag", "Antigua and Barbuda",
	"aq", "Antarctica",
	"ai", "Anguilla",
	"ao", "Angola",
	"ad", "Andorra",
	"as", "American Samoa",
	"dz", "Algeria",
	"al", "Albania",
	"af", "Afghanistan",
	"pro", "Professionals",
	"name", "Individuals",
	"museum", "Museum",
	"coop", "Cooperative Institution",
	"aero", "Air-Transport Industry",
	"biz", "Business Institution",
	"info", "Generic Top Level Domain",
	"mil", "United States Military",
	"gov", "United States Government",
	"edu", "Educational Institution",
	"org", "Generic Top Level Domain",
	"com", "Generic Top Level Domain",
	"net", "Generic Top Level Domain",
	"vhost", "Nonexistant/virtual host",
	NULL
};
char *templates[MAX_TEMPLATES];

#define MSG_RPONG "RPONG"
#define TOK_RPONG "AN"
void SSSet(Conf *, Modulo *);
int SSTest(Conf *, int *);
int SSRefrescaCl(Cliente *);
int SSSigSQL();
int SSSigEOS();
int SSSigSockClose();
HDIRFUNC(LeeHDir);
int SSCmdPostNick(Cliente *, int);
int SSCmdQuit(Cliente *, char *);
int SSCmdUmode(Cliente *, char *);
int SSCmdJoin(Cliente *, Canal *);
int SSCmdDestroyChan(Canal *);
int SSCmdServer(Cliente *, int);
int SSCmdSquit(Cliente *);
int SSCmdRaw(Cliente *, char **, int, int);
IRCFUNC(SSReplyPing);
int SSRefresca(int);
StsChar *InsertaSts(char *, char *, StsChar **);
StsChar *BuscaSts(char *, StsChar *);
int SSTemplates(Proc *);

ModInfo MOD_INFO(StatServ) = {
	"StatServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(StatServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(StatServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(StatServ).nombre))
			{
				if (!SSTest(modulo.seccion[0], &errores))
					SSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(StatServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(StatServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		SSSet(NULL, mod);
	if (conf_httpd)
	{
		ircsprintf(buf, "/%s", MOD_INFO(StatServ).nombre);
		if (!(hdtest = CreaHDir(buf, LeeHDir)))
			errores++;
	}
	return errores;
}
int MOD_DESCARGA(StatServ)()
{
	StsServ *sts;
	for (sts = stats.stsserv; sts; sts = sts->sig)
		ApagaCrono(sts->crono);
	if (conf_httpd)
		BorraHDir(hdtest);
	BorraSenyal(SIGN_SQL, SSSigSQL);
	BorraSenyal(SIGN_POST_NICK, SSCmdPostNick);
	BorraSenyal(SIGN_QUIT, SSCmdQuit);
	BorraSenyal(SIGN_UMODE, SSCmdUmode);
	BorraSenyal(SIGN_JOIN, SSCmdJoin);
	BorraSenyal(SIGN_CDESTROY, SSCmdDestroyChan);
	BorraSenyal(SIGN_SERVER, SSCmdServer);
	BorraSenyal(SIGN_SQUIT, SSCmdSquit);
	BorraSenyal(SIGN_RAW, SSCmdRaw);
	BorraSenyal(SIGN_EOS, SSSigEOS);
	BorraSenyal(SIGN_SOCKCLOSE, SSSigSockClose);
	BorraComando(MSG_RPONG, SSReplyPing);
	DetieneProceso(SSTemplates);
	BotUnset(statserv);
	return 0;
}
int SSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, statserv_coms, 1);
	if (!(eval = BuscaEntrada(config, "refresco")))
	{
		Error("[%s:%s] No se encuentra la directriz refresco.]\n", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (atoi(eval->data) < 60)
		{
			Error("[%s:%s::%i] La directriz refresco debe ser superior a 60 segundos.]\n", config->archivo, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "template")))
	{
		if (!EsArchivo(eval->data))
		{
			Error("[%s:%s::%s::%i] No se encuentra el template %s.]\n", config->archivo, eval->item, eval->item, eval->linea, eval->data);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void SSSet(Conf *config, Modulo *mod)
{
	int i, p, j = 0;
	if (!statserv)
	{
		statserv = BMalloc(StatServ);
		bzero(&stats, sizeof(stats));
	}
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, statserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, statserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
			else if (!strcmp(config->seccion[i]->item, "refresco"))
				statserv->refresco = atoi(config->seccion[i]->data);
			else if (j < MAX_TEMPLATES && !strcmp(config->seccion[i]->item, "template"))
				ircstrdup(templates[j++], config->seccion[i]->data);
		}
	}
	else
		ProcesaComsMod(NULL, mod, statserv_coms);
	templates[j] = NULL;
	InsertaSenyal(SIGN_SQL, SSSigSQL);
	InsertaSenyal(SIGN_POST_NICK, SSCmdPostNick);
	InsertaSenyal(SIGN_QUIT, SSCmdQuit);
	InsertaSenyal(SIGN_UMODE, SSCmdUmode);
	InsertaSenyal(SIGN_JOIN, SSCmdJoin);
	InsertaSenyal(SIGN_CDESTROY, SSCmdDestroyChan);
	InsertaSenyal(SIGN_SERVER, SSCmdServer);
	InsertaSenyal(SIGN_SQUIT, SSCmdSquit);
	InsertaSenyal(SIGN_RAW, SSCmdRaw);
	InsertaSenyal(SIGN_EOS, SSSigEOS);
	InsertaSenyal(SIGN_SOCKCLOSE, SSSigSockClose);
	InsertaComando(MSG_RPONG, TOK_RPONG, SSReplyPing, INI, MAXPARA);
	IniciaProceso(SSTemplates);
	BotSet(statserv);
}
HDIRFUNC(LeeHDir)
{
	char *c;
	int i;
	for (i = 0; templates[i]; i++)
	{
		if ((c = strchr(templates[i], '/')) && !strcasecmp(hh->archivo, c+1))
		{
			*errmsg = "Acceso denegado";
			return 403;
		}
	}
	return 200;
}
char *BuscaTld(char *tld)
{
	int i;
	for (i = 0; tlds[i]; i += 2)
	{
		if (!strcasecmp(tld, tlds[i]))
			return tlds[i+1];
	}
	return "(desconocido)";
}
StsChar *BuscaSts(char *item, StsChar *l)
{
	StsChar *sts;
	for (sts = l; sts; sts = sts->sig)
	{
		if (!strcasecmp(sts->item, item))
			return sts;
	}
	return NULL;
}
StsChar *InsertaSts(char *item, char *valor, StsChar **l)
{
	StsChar *sts;
	if (!(sts = BuscaSts(item, *l)))
	{
		sts = BMalloc(StsChar);
		sts->item = strdup(item);
		if (valor)
			sts->valor = strdup(valor);
		AddItem(sts, *l);
	}
	return sts;
}
ListCl *BuscaCl(Cliente *cl, ListCl *l)
{
	ListCl *lcl;
	for (lcl = l; lcl; lcl = lcl->sig)
	{
		if (lcl->cl == cl)
			return lcl;
	}
	return NULL;
}
ListCl *InsertaCl(Cliente *cl, StsChar *sts, ListCl **l)
{
	ListCl *lcl;
	if (!(lcl = BuscaCl(cl, *l)))
	{
		lcl = BMalloc(ListCl);
		lcl->cl = cl;
		lcl->sts = sts;
		AddItem(lcl, *l);
	}
	return lcl;
}
int BorraCl(Cliente *cl, ListCl **l)
{
	ListCl *lcl, *prev = NULL;
	for (lcl = *l; lcl; lcl = lcl->sig)
	{
		if (lcl->cl == cl)
		{
			if (lcl->sts)
				lcl->sts->users--;
			if (prev)
				prev->sig = lcl->sig;
			else
				*l = lcl->sig;
			Free(lcl);
			return 1;
		}
		prev = lcl;
	}
	return 0;
}
StsServ *BuscaServ(Cliente *cl)
{
	StsServ *sts;
	for (sts = stats.stsserv; sts; sts = sts->sig)
	{
		if (sts->cl == cl)
			return sts;
	}
	return NULL;
}
int SSCmdPostNick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		char *c;
		if ((c = strrchr(cl->host, '.')))
		{
			ListCl *lcl;
			StsChar *sts;
			StsServ *stsserv;
			c++;
			sts = InsertaSts(c, BuscaTld(c), &(stats.ststld));
			sts->users++;
			lcl = InsertaCl(cl, sts, &cltld);
			if (IsOper(cl) && !BuscaCl(cl, stats.stsopers))
			{
				InsertaCl(cl, NULL, &(stats.stsopers));
				SSRefresca(OPERS);
			}
			if ((stsserv = BuscaServ(cl->server)) && (++stsserv->users > stsserv->max_users))
				stsserv->max_users = stsserv->users;
		}
		SSRefresca(USERS);
		ProtFunc(P_MSG_VL)(cl, CLI(statserv), 1, "\1VERSION\1", NULL);
	}
	return 0;
}
int SSCmdQuit(Cliente *cl, char *motivo)
{
	ListCl *lcl;
	StsServ *sts;
	stats.users--;
	if ((lcl = BuscaCl(cl, cltld)))
	{
		if (lcl->sts && !--lcl->sts->users)
		{
			ircfree(lcl->sts->item);
			ircfree(lcl->sts->valor);
			LiberaItem(lcl->sts, stats.ststld);
		}
		LiberaItem(lcl, cltld);
	}
	if ((lcl = BuscaCl(cl, clver)))
	{
		if (lcl->sts && !--lcl->sts->users)
		{
			ircfree(lcl->sts->item);
			ircfree(lcl->sts->valor);
			LiberaItem(lcl->sts, stats.stsver);
		}
		LiberaItem(lcl, clver);
	}
	if (IsOper(cl) && BorraCl(cl, &(stats.stsopers)))
		stats.opers--;
	if ((sts = BuscaServ(cl->server)))
		sts->users--;
	return 0;
}
int SSCmdUmode(Cliente *cl, char *umodes)
{
	if (strchr(umodes, 'o'))
	{
		if (IsOper(cl))
		{
			if (!BuscaCl(cl, stats.stsopers))
			{
				InsertaCl(cl, NULL, &(stats.stsopers));
				SSRefresca(OPERS);
			}
		}
		else if (BorraCl(cl, &(stats.stsopers)))
			stats.opers--;
	}
	return 0;
}
int SSCmdJoin(Cliente *cl, Canal *cn)
{
	if (!cl || cn->miembros == 1)
		SSRefresca(CHANS);
	return 0;
}
int SSCmdDestroyChan(Canal *cn)
{
	stats.chans--;
	return 0;
}
int SSCmdServer(Cliente *cl, int primero)
{
	StsServ *sts = BMalloc(StsServ);
	char *n = CLI(statserv)->nombre;
	sts->cl = cl;
	AddItem(sts, stats.stsserv);
	EnviaAServidor(":%s VERSION :%s", n, cl->nombre);
	EnviaAServidor(":%s LUSERS :%s", n, cl->nombre);
	EnviaAServidor(":%s STATS u %s", n, cl->nombre);
	SSRefrescaCl(cl);
	sts->crono = IniciaCrono(0, statserv->refresco, SSRefrescaCl, cl);
	SSRefresca(SERVS);
	return 0;
}
int SSRefrescaCl(Cliente *cl)
{
#ifdef _WIN32
	struct _timeb tv;
	_ftime(&tv);
	EnviaAServidor(":%s RPING %s %s %lu %lu :%lu", me.nombre, cl->nombre, CLI(statserv)->nombre, tv.time, tv.millitm, clock());
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	EnviaAServidor(":%s RPING %s %s %lu %lu :%lu", me.nombre, cl->nombre, CLI(statserv)->nombre, tv.tv_sec, tv.tv_usec, clock());
#endif
	return 0;
}
int SSCmdSquit(Cliente *cl)
{
	StsServ *sts;
	if ((sts = BuscaServ(cl)))
	{
		stats.servs--;
		Free(sts->version);
		ApagaCrono(sts->crono);
		LiberaItem(sts, stats.stsserv);
	}
	return 0;
}
int SSCmdRaw(Cliente *cl, char *parv[], int parc, int raw)
{
	switch (raw)
	{
		case 242:
		{
			StsServ *sts;
			int d = 0, h = 0, m = 0, s = 0;
			if ((sts = BuscaServ(cl)) && sscanf(parv[2], "%*s %*s %i %*s %i:%i:%i", &d, &h, &m, &s))
				sts->uptime = GMTime() - (d*86400+h*3600+m*60+s);
			break;
		}
		case 265:
		{
			StsServ *sts;
			char *c;
			int max;
			if ((sts = BuscaServ(cl)) && (c = strrchr(parv[2], ' ')) && (max = atoi(c+1)) > sts->max_users)
				sts->max_users = max;
			break;
		}
		case 351:
		{
			StsServ *sts;
			if ((sts = BuscaServ(cl)))
				ircstrdup(sts->version, parv[2]);
			break;
		}
	}
	return 0;
}
int SSRefresca(int val)
{
	SQLRes res;
	SQLRow row;
	time_t t = GMTime();
	struct tm *l = localtime(&t);
	ircsprintf(buf, "%02u-%02u-%02u", l->tm_year+1900, l->tm_mon+1, l->tm_mday);
	if (!(val & NO_INC))
	{
		if (val & USERS)
			stats.users++;
		if (val & CHANS)
			stats.chans++;
		if (val & SERVS)
			stats.servs++;
		if (val & OPERS)
			stats.opers++;
	}
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE dia='%s'", PREFIJO, SS_SQL, buf)))
	{
		int ok = 0;
		row = SQLFetchRow(res);
		if ((val & USERS) && stats.users > atoi(row[1]))
		{
			ircsprintf(row[1], "%u", stats.users);
			ircsprintf(row[2], "%lu", t);
			ok = 1;
		}
		if ((val & CHANS) && stats.chans > atoi(row[3]))
		{
			ircsprintf(row[3], "%u", stats.chans);
			ircsprintf(row[4], "%lu", t);
			ok = 1;
		}
		if ((val & SERVS) && stats.servs > atoi(row[5]))
		{
			ircsprintf(row[5], "%u", stats.servs);
			ircsprintf(row[6], "%lu", t);
			ok = 1;
		}
		if ((val & OPERS) && stats.opers > atoi(row[7]))
		{
			ircsprintf(row[7], "%u", stats.opers);
			ircsprintf(row[8], "%lu", t);
			ok = 1;
		}
		if (ok)
			SQLQuery("UPDATE %s%s SET users=%s, users_time=%s, canales=%s, canales_time=%s, servers=%s, servers_time=%s, opers=%s, opers_time=%s WHERE dia='%s'",
				PREFIJO, SS_SQL, row[1], row[2], row[3], row[4],
				row[5], row[6], row[7], row[8], row[0]);
	}
	else
	{
		SQLQuery("INSERT INTO %s%s (dia, users, users_time, canales, canales_time, servers, servers_time, opers, opers_time) VALUES "
			"('%s', %u, %lu, %u, %lu, %u, %lu, %u, %lu)",
			PREFIJO, SS_SQL, buf, 
			stats.users, t, stats.chans, t, stats.servs, t, stats.opers, t);
	}
	return 0;
}
IRCFUNC(SSReplyPing)
{
	StsServ *sts;
	if ((sts = BuscaServ(cl)))
		sts->lag = (clock() - atoi(parv[5]))*1000/CLOCKS_PER_SEC;
	return 0;
}
BOTFUNC(SSReplyVer)
{
	ListCl *lcl;
	StsChar *sts;
	int i;
	if (!BuscaCl(cl, clver))
	{
		for (i = 0; vers[i]; i++)
		{
			if (stristr(parv[2], vers[i]))
			{
				sts = InsertaSts(vers[i], NULL, &(stats.stsver));
				sts->users++;
				lcl = InsertaCl(cl, sts, &clver);
				return 0;
			}
		}
		sts = InsertaSts("(desconocido)", NULL, &(stats.stsver));
		sts->users++;
		lcl = InsertaCl(cl, sts, &clver);
	}
	return 0;
}
BOTFUNC(SSHelp)
{
	return 0;
}
BOTFUNC(SSStats)
{
	if (params < 2)
	{
		char timebuf[128];
		time_t ut, ct, st, ot;
		u_int u, c, s, o;
		SQLRes res;
		SQLRow row;
		if (!(res = SQLQuery("SELECT * FROM %s%s WHERE DATE(CURDATE())=DATE(dia)", PREFIJO, SS_SQL)))
			SSRefresca(USERS|CHANS|SERVS|OPERS|NO_INC);
		else
			SQLFreeRes(res);
		Responde(cl, CLI(statserv), "Estadísticas totales:");
		res = SQLQuery("SELECT MAX(users), users_time FROM %s%s GROUP BY users ORDER BY users DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		u = atoi(row[0]);
		ut = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), fecha_fmt, localtime(&ut));
		Responde(cl, CLI(statserv), "Usuarios en la red: \00312%u\003 - Máximo: \00312%u\003 - Fecha: \00312%s", stats.users, u, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(canales), canales_time FROM %s%s GROUP BY canales ORDER BY canales DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		c = atoi(row[0]);
		ct = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), fecha_fmt, localtime(&ct));
		Responde(cl, CLI(statserv), "Canales en la red: \00312%u\003 - Máximo: \00312%u\003 - Fecha: \00312%s", stats.chans, c, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(servers), servers_time FROM %s%s GROUP BY servers ORDER BY servers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		s = atoi(row[0]);
		st = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), fecha_fmt, localtime(&st));
		Responde(cl, CLI(statserv), "Servidores en la red: \00312%u\003 - Máximo: \00312%u\003 - Fecha: \00312%s", stats.servs, s, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(opers), opers_time FROM %s%s GROUP BY opers ORDER BY opers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		o = atoi(row[0]);
		ot = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), fecha_fmt, localtime(&ot));
		Responde(cl, CLI(statserv), "Operadores en la red: \00312%u\003 - Máximo: \00312%u\003 - Fecha: \00312%s", stats.opers, o, timebuf);
		SQLFreeRes(res);
		Responde(cl, CLI(statserv), " ");
		Responde(cl, CLI(statserv), "Estadísticas diarias:");
		res = SQLQuery("SELECT * FROM %s%s WHERE DATE(CURDATE())=DATE(dia)", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		ut = atoi(row[2]);
		strftime(timebuf, sizeof(timebuf), hora_fmt, localtime(&ut));
		Responde(cl, CLI(statserv), "Máximo usuarios: \00312%s\003 - Hora: \00312%s", row[1], timebuf);
		ct = atoi(row[4]);
		strftime(timebuf, sizeof(timebuf), hora_fmt, localtime(&ct));
		Responde(cl, CLI(statserv), "Máximo canales: \00312%s\003 - Hora: \00312%s", row[3], timebuf);
		st = atoi(row[6]);
		strftime(timebuf, sizeof(timebuf), hora_fmt, localtime(&st));
		Responde(cl, CLI(statserv), "Máximo servidores: \00312%s\003 - Hora: \00312%s", row[5], timebuf);
		ot = atoi(row[8]);
		strftime(timebuf, sizeof(timebuf), hora_fmt, localtime(&ot));
		Responde(cl, CLI(statserv), "Máximo operadores: \00312%s\003 - Hora: \00312%s", row[7], timebuf);
		SQLFreeRes(res);
		Responde(cl, CLI(statserv), " ");
		Responde(cl, CLI(statserv), "Estadísticas semanales:");
		res = SQLQuery("SELECT MAX(users), users_time FROM %s%s WHERE WEEK(CURDATE())=WEEK(dia) GROUP BY users ORDER BY users DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		u = atoi(row[0]);
		ut = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), sem_fmt, localtime(&ut));
		Responde(cl, CLI(statserv), "Máximo usuarios: \00312%u\003 - Día y hora: \00312%s", u, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(canales), canales_time FROM %s%s WHERE WEEK(CURDATE())=WEEK(dia) GROUP BY canales ORDER BY canales DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		c = atoi(row[0]);
		ct = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), sem_fmt, localtime(&ct));
		Responde(cl, CLI(statserv), "Máximo canales: \00312%u\003 - Día y hora: \00312%s", c, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(servers), servers_time FROM %s%s WHERE WEEK(CURDATE())=WEEK(dia) GROUP BY servers ORDER BY servers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		s = atoi(row[0]);
		st = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), sem_fmt, localtime(&st));
		Responde(cl, CLI(statserv), "Máximo servidores: \00312%u\003 - Día y hora: \00312%s", s, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(opers), opers_time FROM %s%s WHERE WEEK(CURDATE())=WEEK(dia) GROUP BY opers ORDER BY opers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		o = atoi(row[0]);
		ot = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), sem_fmt, localtime(&ot));
		Responde(cl, CLI(statserv), "Máximo operadores: \00312%u\003 - Día y hora: \00312%s", o, timebuf);
		SQLFreeRes(res);
		Responde(cl, CLI(statserv), " ");
		Responde(cl, CLI(statserv), "Estadísticas mensuales:");
		res = SQLQuery("SELECT MAX(users), users_time FROM %s%s WHERE MONTH(CURDATE())=MONTH(dia) GROUP BY users ORDER BY users DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		u = atoi(row[0]);
		ut = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), diasem_fmt, localtime(&ut));
		Responde(cl, CLI(statserv), "Máximo usuarios: \00312%u\003 - Día y hora: \00312%s", u, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(canales), canales_time FROM %s%s WHERE MONTH(CURDATE())=MONTH(dia) GROUP BY canales ORDER BY canales DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		c = atoi(row[0]);
		ct = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), diasem_fmt, localtime(&ct));
		Responde(cl, CLI(statserv), "Máximo canales: \00312%u\003 - Día y hora: \00312%s", c, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(servers), servers_time FROM %s%s WHERE MONTH(CURDATE())=MONTH(dia) GROUP BY servers ORDER BY servers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		s = atoi(row[0]);
		st = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), diasem_fmt, localtime(&st));
		Responde(cl, CLI(statserv), "Máximo servidores: \00312%u\003 - Día y hora: \00312%s", s, timebuf);
		SQLFreeRes(res);
		res = SQLQuery("SELECT MAX(opers), opers_time FROM %s%s WHERE MONTH(CURDATE())=MONTH(dia) GROUP BY opers ORDER BY opers DESC", PREFIJO, SS_SQL);
		row = SQLFetchRow(res);
		o = atoi(row[0]);
		ot = atoi(row[1]);
		strftime(timebuf, sizeof(timebuf), diasem_fmt, localtime(&ot));
		Responde(cl, CLI(statserv), "Máximo operadores: \00312%u\003 - Día y hora: \00312%s", o, timebuf);
		SQLFreeRes(res);
	}
	else if (!strcasecmp(param[1], "SERVERS") || !strcasecmp(param[1], "SERVIDORES"))
	{
		StsServ *sts;
		char timebuf[128];
		time_t t;
		Duracion d;
		for (sts = stats.stsserv; sts; sts = sts->sig)
		{
			Responde(cl, CLI(statserv), "Servidor: \00312%s", sts->cl->nombre);
			Responde(cl, CLI(statserv), "Información: \00312%s", sts->cl->info);
			Responde(cl, CLI(statserv), "Usuarios: \00312%u\003 - Máximo: \00312%u", sts->users, sts->max_users);
			Responde(cl, CLI(statserv), "Lag: %\00312%.3f segs", (double)sts->lag/1000);
			Responde(cl, CLI(statserv), "Versión del servidor: \00312%s", sts->version);
			t = GMTime() - sts->uptime;
			MideDuracion(t, &d);
			Responde(cl, CLI(statserv), "Uptime: \00312%u días, %02u:%02u:%02u", d.sems*7+d.dias, d.horas, d.mins, d.segs);
			Responde(cl, CLI(statserv), " ");
		}
	}
	else if (!strcasecmp(param[1], "OPERS") || !strcasecmp(param[1], "OPERADORES"))
	{
		ListCl *lc;
		for (lc = stats.stsopers; lc; lc = lc->sig)
		{
			Responde(cl, CLI(statserv), "Operador: \00312%s", lc->cl->nombre);
			if (lc->cl->away)
				Responde(cl, CLI(statserv), "Está ausente: \00312%s", lc->cl->away);
			Responde(cl, CLI(statserv), " ");
		}
	}
	else if (!strcasecmp(param[1], "VERS") || !strcasecmp(param[1], "VERSIONES"))
	{
		StsChar *sts;
		for (sts = stats.stsver; sts; sts = sts->sig)
			Responde(cl, CLI(statserv), "\00312%s\003 - Usuarios: \00312%u", sts->item, sts->users);
	}
	else if (!strcasecmp(param[1], "TLDS"))
	{
		StsChar *sts;
		for (sts = stats.ststld; sts; sts = sts->sig)
			Responde(cl, CLI(statserv), "Dominio: \00312.%s\003 - Lugar: \00312%s\003 Usuarios: \00312%u", sts->item, sts->valor, sts->users);
	}
	else
		Responde(cl, CLI(statserv), SS_ERR_EMPT, "Opción incorrecta");	
	return 0;
}
BOTFUNCHELP(SSHStats)
{
	return 0;
}
int SSSigEOS()
{
	if (refrescando)
	{
		Cliente *cl;
		Canal *cn;
		LinkCliente *lk;
		for (lk = servidores; lk; lk = lk->sig)
			SSCmdServer(lk->user, 0);
		for (cn = canales; cn; cn = cn->sig)
			SSCmdJoin(NULL, cn);
		for (cl = clientes; cl; cl = cl->sig)
		{
			if (EsCliente(cl))
				SSCmdPostNick(cl, 0);
		}
	}
	return 0;
}
int SSSigSQL()
{
	if (!SQLEsTabla(SS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"dia date default NULL, "
  			"users int, "
  			"users_time int, "
  			"canales int, "
  			"canales_time int, "
  			"servers int, "
  			"servers_time int, "
  			"opers int, "
  			"opers_time int, "
			"KEY dia (dia) "
			");", PREFIJO, SS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SS_SQL);
	}
	SQLCargaTablas();
	return 1;
}
int SSSigSockClose()
{
	StsServ *stsserv, *ssig;
	StsChar *sts, *stsig;
	ListCl *lc, *lcsig;
	for (stsserv = stats.stsserv; stsserv; stsserv = ssig)
	{
		ssig = stsserv->sig;
		ApagaCrono(stsserv->crono);
		Free(stsserv->version);
		Free(stsserv);
	}
	for (sts = stats.stsver; sts; sts = stsig)
	{
		stsig = sts->sig;
		Free(sts->item);
		if (sts->valor)
			Free(sts->valor);
		Free(sts);
	}
	for (sts = stats.ststld; sts; sts = stsig)
	{
		stsig = sts->sig;
		Free(sts->item);
		if (sts->valor)
			Free(sts->valor);
		Free(sts);
	}
	for (lc = stats.stsopers; lc; lc = lcsig)
	{
		lcsig = lc->sig;
		Free(lc);
	}
	for (lc = cltld; lc; lc = lcsig)
	{
		lcsig = lc->sig;
		Free(lc);
	}
	for (lc = clver; lc; lc = lcsig)
	{
		lcsig = lc->sig;
		Free(lc);
	}
	bzero(&stats, sizeof(stats));
	cltld = clver = NULL;
	return 0;
}
void ParseaTemplate(char *f)
{
	char *c, *d, *p = NULL, *s, *e, *g;
	int fdout;
	u_long len = 0;
	SQLRes res;
	SQLRow row;
#ifdef _WIN32
	HANDLE fdin, mp;
#else
	int fdin;
	struct stat sb;
#endif
	strncpy(buf, f, sizeof(buf));
	if ((c = strrchr(buf, '.')))
		*c = '\0';
#ifdef _WIN32					
	if ((fdin = CreateFile(f, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
#else
	if ((fdin = open(f, O_RDONLY)) == -1)
#endif
		return;
#ifdef _WIN32
					
	len = GetFileSize(fdin, NULL);
	if (!(mp = CreateFileMapping(fdin, NULL, PAGE_READONLY|SEC_COMMIT, 0, len, NULL)))
	{
		CloseHandle(fdin);
		return;
	}
	if (!(p = MapViewOfFile(mp, FILE_MAP_READ, 0, 0, 0)))
	{
		CloseHandle(mp);
		CloseHandle(fdin);
		return;
	}
	if ((fdout = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0600)) == -1)
	{
		CloseHandle(mp);
		CloseHandle(fdin);
		return;
	}
#else	
	if (fstat(fdin, &sb) == -1)
	{
		close(fd);
		return;
	}
	if (sb.st_mtime <= hh->lmod)
	{
		close(fdin);
		return;
	}
	len = sb.st_size;
	if (!(p = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0)))
	{
		close(fdin);
		return;
	}
	if ((fdout = open(buf, O_WRONLY | O_CREAT, 0600)) == -1)
	{
		munmap(p, len);
		close(fdin);
		return;
	}
#endif
	s = p;
	while ((c = strchr(s, '@')))
	{
		write(fdout, s, c-s);
		if (*(c-1) == '\\')
		{
			s = c+1;
			continue;
		}
		c++;
		if (!(d = strchr(c, '@')))
			break;
		if (!strncmp(c, "USERS_CUR", d-c))
		{
			ircsprintf(buf, "%u", stats.users);
			write(fdout, buf, strlen(buf));
		}
		else if (!strncmp(c, "CHANS_CUR", d-c))
		{
			ircsprintf(buf, "%u", stats.chans);
			write(fdout, buf, strlen(buf));
		}
		else if (!strncmp(c, "SERVS_CUR", d-c))
		{
			ircsprintf(buf, "%u", stats.servs);
			write(fdout, buf, strlen(buf));
		}
		else if (!strncmp(c, "OPERS_CUR", d-c))
		{
			ircsprintf(buf, "%u", stats.opers);
			write(fdout, buf, strlen(buf));
		}
		else if (!strncmp(c, "VERSIONS_INICIO", d-c))
		{
			StsChar *sts;
			char *e, *f = 0;
			c += d-c+1;
			e = c;
			for (sts = stats.stsver; sts; sts = sts->sig)
			{
				while ((f = strchr(e, '@')))
				{
					write(fdout, e, f-e);
					if (*(f-1) == '\\')
					{
						e = f+1;
						continue;
					}
					f++;
					if (!(d = strchr(f, '@')))
						break;
					if (!strncmp(f, "VERSIONS_NOMBRE", d-f))
						write(fdout, sts->item, strlen(sts->item));
					else if (!strncmp(f, "VERSIONS_VALOR", d-f))
					{
						ircsprintf(buf, "%u", sts->users);
						write(fdout, buf, strlen(buf));
					}
					else if (!strncmp(f, "VERSIONS_FIN", d-f))
						break;
					e = d+1;
				}
				e = c;
			}
		}
		else if (!strncmp(c, "TLDS_INICIO", d-c))
		{
			StsChar *sts;
			char *e, *f = 0;
			c += d-c+1;
			e = c;
			for (sts = stats.ststld; sts; sts = sts->sig)
			{
				while ((f = strchr(e, '@')))
				{
					write(fdout, e, f-e);
					if (*(f-1) == '\\')
					{
						e = f+1;
						continue;
					}
					f++;
					if (!(d = strchr(f, '@')))
						break;
					if (!strncmp(f, "TLDS_NOMBRE", d-f))
						write(fdout, sts->item, strlen(sts->item));
					else if (!strncmp(f, "TLDS_VALOR", d-f))
					{
						ircsprintf(buf, "%u", sts->users);
						write(fdout, buf, strlen(buf));
					}
					else if (!strncmp(f, "TLDS_FIN", d-f))
						break;
					e = d+1;
				}
				e = c;
			}
		}
		else if (!strncmp(c, "SERVERS_INICIO", d-c))
		{
			StsServ *sts;
			char *e, *f = 0;
			c += d-c+1;
			e = c;
			for (sts = stats.stsserv; sts; sts = sts->sig)
			{
				while ((f = strchr(e, '@')))
				{
					write(fdout, e, f-e);
					if (*(f-1) == '\\')
					{
						e = f+1;
						continue;
					}
					f++;
					if (!(d = strchr(f, '@')))
						break;
					if (!strncmp(f, "SERVERS_NOMBRE", d-f))
						write(fdout, sts->cl->nombre, strlen(sts->cl->nombre));
					else if (!strncmp(f, "SERVERS_INFO", d-f))
						write(fdout, sts->cl->info, strlen(sts->cl->info));
					else if (!strncmp(f, "SERVERS_USERS", d-f))
					{
						ircsprintf(buf, "%u", sts->users);
						write(fdout, buf, strlen(buf));
					}
					else if (!strncmp(f, "SERVERS_LAG", d-f))
					{
						ircsprintf(buf, "%.3f", (double)sts->lag/1000);
						write(fdout, buf, strlen(buf));
					}
					else if (!strncmp(f, "SERVERS_VERSION", d-f))
					{
						char *v = sts->version;
						if (!v)
							v = "(no definida)";
						write(fdout, v, strlen(v));
					}
					else if (!strncmp(f, "SERVERS_UPTIME", d-f))
					{
						time_t t;
						Duracion d;
						t = GMTime() - sts->uptime;
						MideDuracion(t, &d);
						ircsprintf(buf, "%u días, %02u:%02u:%02u", d.sems*7+d.dias, d.horas, d.mins, d.segs);
						write(fdout, buf, strlen(buf));
					}
					else if (!strncmp(f, "SERVERS_FIN", d-f))
						break;
					e = d+1;
				}
				e = c;
			}
		}
		s = d+1;
	}		
	write(fdout, s, p+len-s);
#ifdef _WIN32
	UnmapViewOfFile(p);
	CloseHandle(mp);
	CloseHandle(fdin);
#else
	munmap(p, len);
	close(fdin);
#endif
	close(fdout);
}
int SSTemplates(Proc *proc)
{
	char *c, *d, *e, *f;
	time_t t = time(0);
	if (proc->time + statserv->refresco < t)
	{
		if (!templates[proc->proc])
		{
			proc->proc = 0;
			proc->time = t;
			return 1;
		}
		ParseaTemplate(templates[proc->proc]);
		proc->proc++;
	}
	return 0;
}
