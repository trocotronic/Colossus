/*
 * $Id: statserv.c,v 1.21 2007-05-27 19:14:37 Trocotronic Exp $ 
 */

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
BOTFUNC(SSReplyVer);

static bCom statserv_coms[] = {
	{ "\1VERSION" , SSReplyVer , N0 , "Respuesta reply" , NULL } ,
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

void SSSet(Conf *, Modulo *);
int SSTest(Conf *, int *);
int SSLaguea(Cliente *);
int SSSigSQL();
HDIRFUNC(LeeHDir);
int SSCmdPostNick(Cliente *, int);
int SSCmdQuit(Cliente *, char *);
int SSCmdUmode(Cliente *, char *);
int SSCmdJoin(Cliente *, Canal *);
int SSCmdDestroyChan(Canal *);
int SSCmdServer(Cliente *, int);

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
		if (!(hdtest = CreaHDir("/html", LeeHDir)))
			errores++;
	}
	return errores;
}
int MOD_DESCARGA(StatServ)()
{
	if (conf_httpd)
		BorraHDir(hdtest);
	BorraSenyal(SIGN_SQL, SSSigSQL);
	BorraSenyal(SIGN_POST_NICK, SSCmdPostNick);
	BorraSenyal(SIGN_QUIT, SSCmdQuit);
	BorraSenyal(SIGN_UMODE, SSCmdUmode);
	BorraSenyal(SIGN_JOIN, SSCmdJoin);
	BorraSenyal(SIGN_CDESTROY, SSCmdDestroyChan);
	BorraSenyal(SIGN_SERVER, SSCmdServer);
	BotUnset(statserv);
	return 0;
}
int SSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, statserv_coms, 1);
	if (!(eval = BuscaEntrada(config, "canal_soporte")))
	{
		Error("[%s:%s] No se encuentra la directriz canal_soporte.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (BadPtr(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz canal_soporte esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void SSSet(Conf *config, Modulo *mod)
{
	int i, p;
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
		}
	}
	else
		ProcesaComsMod(NULL, mod, statserv_coms);
	InsertaSenyal(SIGN_SQL, SSSigSQL);
	InsertaSenyal(SIGN_POST_NICK, SSCmdPostNick);
	InsertaSenyal(SIGN_QUIT, SSCmdQuit);
	InsertaSenyal(SIGN_UMODE, SSCmdUmode);
	InsertaSenyal(SIGN_JOIN, SSCmdJoin);
	InsertaSenyal(SIGN_CDESTROY, SSCmdDestroyChan);
	InsertaSenyal(SIGN_SERVER, SSCmdServer);
	BotSet(statserv);
}
HDIRFUNC(LeeHDir)
{
	return -1;
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
				lcl->sts--;
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
		
#define CogeSInt(x, y) do { ircsprintf(max, "%s_%s", b, x); y = CogeInt(max); }while(0)
#define ActualizaSInt(x, y) do { ircsprintf(max, "%s_%s", b, x); ActualizaInt(max, y); }while(0)
void ActualizaStats(char *b)
{
	u_int u, umax;
	time_t t = GMTime(), tcur;
	struct tm *l = localtime(&t), *lcur;
	char max[16];
	if (*b == 'u')
		u = ++stats.users;
	else if (*b == 'c')
		u = ++stats.chans;
	else if (*b == 's')
		u = ++stats.servs;
	else if (*b == 'o')
		u = ++stats.opers;
	CogeSInt("max", umax);
	if (umax < u)
	{
		ActualizaSInt("max", u);
		ActualizaSInt("curmax", t);
	}
	CogeSInt("hoy", umax);
	CogeSInt("curhoy", tcur);
	lcur = localtime(&tcur);
	if (umax < u || !(GetYear(lcur) == GetYear(l) && GetMon(lcur) == GetMon(l) && GetDay(lcur) == GetDay(l)))
	{
		ActualizaSInt("hoy", u);
		ActualizaSInt("curhoy", t);
	}
	CogeSInt("sem", umax);
	CogeSInt("cursem", tcur);
	lcur = localtime(&tcur);
	if (umax < u || !(GetYear(lcur) == GetYear(l) && GetMon(lcur) == GetMon(l) && GetWeek(lcur) == GetWeek(l)))
	{
		ActualizaSInt("sem", u);
		ActualizaSInt("cursem", t);
	}
	CogeSInt("mes", umax);
	CogeSInt("curmes", tcur);
	lcur = localtime(&tcur);
	if (umax < u || !(GetYear(lcur) == GetYear(l) && GetMon(lcur) == GetMon(l)))
	{
		ActualizaSInt("mes", u);
		ActualizaSInt("curmes", t);
	}
}
BOTFUNC(SSReplyVer)
{
	ListCl *lcl;
	StsChar *sts;
	int i;
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
	return 0;
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
			c++;
			sts = InsertaSts(c, BuscaTld(c), &(stats.ststld));
			sts->users++;
			lcl = InsertaCl(cl, sts, &cltld);
			if (IsOper(cl))
			{
				InsertaCl(cl, NULL, &(stats.stsopers));
				ActualizaStats("opers");
			}	
		}
		ActualizaStats("users");
		ProtFunc(P_MSG_VL)(cl, CLI(statserv), 1, "\1VERSION\1", NULL);
	}
	return 0;
}
int SSCmdQuit(Cliente *cl, char *motivo)
{
	ListCl *stcl, *prev = NULL;
	stats.users--;
	BorraCl(cl, &cltld);
	BorraCl(cl, &clver);
	if (IsOper(cl) && BorraCl(cl, &(stats.stsopers)))
		stats.opers--;
	return 0;
}
int SSCmdUmode(Cliente *cl, char *umodes)
{
	if (strchr(umodes, 'o'))
	{
		if (IsOper(cl))
		{
			InsertaCl(cl, NULL, &(stats.stsopers));
			ActualizaStats("opers");
		}
		else if (BorraCl(cl, &(stats.stsopers)))
			stats.opers--;
	}
	return 0;
}
int SSCmdJoin(Cliente *cl, Canal *cn)
{
	if (cn->miembros == 1)
		stats.chans++;
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
	EnviaAServidor(":%s VERSION :%s", n, cl->nombre);
	EnviaAServidor(":%s LUSERS :%s", n, cl->nombre);
	EnviaAServidor(":%s STATS u %s", n, cl->nombre);
	if (ProtFunc(P_LAG))
	{
		ProtFunc(P_LAG)(cl, CLI(statserv));
		sts->crono = IniciaCrono(0, statserv->laguea, SSLaguea, cl);
	}
	ActualizaStats("servers");
	return 0;
}
int SSLaguea(Cliente *cl)
{
	ProtFunc(P_LAG)(cl, CLI(statserv));
	return 0;
}
int SSSigSQL()
{
	if (!SQLEsTabla(SS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"item varchar(255) default NULL, "
  			"valor varchar(255) NOT NULL default '0', "
			"KEY item (item) "
			");", PREFIJO, SS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SS_SQL);
		else
		{
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_max");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_hoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_sem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_mes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_curmax");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_curhoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_cursem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "users_curmes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_max");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_hoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_sem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_mes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_curmax");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_curhoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_cursem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "canales_curmes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_max");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_hoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_sem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_mes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_curmax");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_curhoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_cursem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "servers_curmes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_max");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_hoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_sem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_mes");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_curxmax");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_curoy");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_cursem");
			SQLQuery("INSERT into %s%s (item) values ('%s')", PREFIJO, SS_SQL, "opers_curmes");
		}
	}
	SQLCargaTablas();
	return 1;
}
