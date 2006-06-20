/*
 * $Id: tvserv.c,v 1.17 2006-06-20 13:19:41 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/tvserv.h"

TvServ *tvserv = NULL;

SOCKFUNC(TSAbreDataSock);
SOCKFUNC(TSCierraDataSock);
SOCKFUNC(TSLeeTv);
SOCKFUNC(TSLeeHoroscopo);
SOCKFUNC(TSLeeTiempo);
SOCKFUNC(TSLeeCine);
SOCKFUNC(TSLeePeli);
SOCKFUNC(TSLeeLiga);
BOTFUNC(TSTv);
BOTFUNCHELP(TSHTv);
BOTFUNC(TSHelp);
BOTFUNC(TSHoroscopo);
BOTFUNCHELP(TSHHoroscopo);
BOTFUNC(TSTiempo);
BOTFUNCHELP(TSHTiempo);
BOTFUNC(TSCine);
BOTFUNCHELP(TSHCine);
BOTFUNC(TSPelis);
BOTFUNCHELP(TSHPelis);
BOTFUNC(TSLiga);
BOTFUNCHELP(TSHLiga);

DataSock *cola[MAX_COLA];
int hora = 6;

static bCom tvserv_coms[] = {
	{ "help" , TSHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "tv" , TSTv , N1 , "Programación de las televisiones de España." , TSHTv } ,
	{ "horoscopo" , TSHoroscopo , N1 , "Horóscopo del día." , TSHHoroscopo } ,
	{ "tiempo" , TSTiempo , N1, "Previsión meteorológica." , TSHTiempo } ,
	{ "cine" , TSCine , N1 , "Cartelera de cines españoles." , TSHCine } ,
	{ "pelis" , TSPelis , N1 , "Información de las películas que se proyectan en los cines españoles." , TSHPelis } ,
	{ "liga" , TSLiga , N1 , "Información sobre la liga de fútbol profesional." , TSHLiga } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void TSSet(Conf *, Modulo *);
int TSTest(Conf *, int *);
int ActualizaTvServ();
char *TSFecha(time_t);
void TSActualizaHoroscopo();
void TSActualizaProg();
int TSSigSQL();
int TSSigQuit(Cliente *, char *);

ModInfo MOD_INFO(TvServ) = {
	"TvServ" ,
	0.3 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

Cadena cadenas[] = {
	{ "TVE1" , "TVE1" , 'n' } ,
	{ "TVE2" , "TVE2" , 'n' } ,
	{ "T5" , "Tele+5" , 'n' } ,
	{ "Cuatro" , "Cuatro" , 'n' } ,
	{ "A3" , "Antena+3" , 'n' } ,
	{ "LaSexta" , "La+Sexta" , 'n' } ,
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
Opts provincias[] = {
	{ 1 , "A Coruña" } ,
	{ 5 , "Alava" } ,
	{ 2 , "Albacete" } ,
	{ 3 , "Alicante" } ,
	{ 4 , "Almeria" } ,
	{ 6 , "Asturias" } ,
	{ 7 , "Avila" } ,
	{ 8 , "Badajoz" } ,
	{ 10 , "Barcelona" } ,
	{ 12 , "Burgos" } ,
	{ 13 , "Caceres" } ,
	{ 15 , "Cadiz" } ,
	{ 14 , "Cantabria" } ,
	{ 16 , "Castellon" } ,
	{ 17 , "Ceuta" } ,
	{ 18 , "Ciudad Real" } ,
	{ 19 , "Cordoba" } ,
	{ 20 , "Cuenca" } ,
	{ 21 , "Girona" } ,
	{ 22 , "Granada" } ,
	{ 23 , "Guadalajara" } ,
	{ 24 , "Guipuzcoa" } ,
	{ 25 , "Huelva" } ,
	{ 26 , "Huesca" } ,
	{ 9 , "Islas Baleares" } ,
	{ 27 , "Jaen" } ,
	{ 31 , "La Rioja" } ,
	{ 30 , "Las Palmas" } ,
	{ 28 , "Leon" },
	{ 29 , "Lleida" } ,
	{ 32 , "Lugo" } ,
	{ 33 , "Madrid" } ,
	{ 34 , "Malaga" } ,
	{ 35 , "Melilla" } ,
	{ 36 , "Murcia" } ,
	{ 37 , "Navarra" } ,
	{ 38 , "Ourense" } ,
	{ 39 , "Palencia" } ,
	{ 40 , "Pontevedra" } ,
	{ 41 , "Salamanca" } ,
	{ 46 , "Santa Cruz de Tenerife" } ,
	{ 42 , "Segovia" } ,
	{ 43 , "Sevilla" } ,
	{ 44 , "Soria" } ,
	{ 45 , "Tarragona" } ,
	{ 46 , "Teruel" } ,
	{ 47 , "Toledo" } ,
	{ 49 , "Valencia" } ,
	{ 50 , "Valladolid" } ,
	{ 11 , "Vizcaya" } ,
	{ 51 , "Zamora" } ,
	{ 52 , "Zaragoza" } ,
	{ 0x0 , 0x0 }
};
char *prov1[] = { "A Coruña","CEE","Ferrol","Naron","Santa Eugenia de Ribeira","Santiago",NULL };
char *prov2[] = { "Albacete","Almansa","Villarrobledo",NULL };
char *prov3[] = { "Alicante","Alcoy","Benidorm","Denia","Elche","Orihuela","Petrer","San Juan","Torrevieja","Villajoyosa","Xabia",NULL };
char *prov4[] = { "Almeria","Aguadulce","El Ejido","Roquetas de Mar",NULL };
char *prov5[] = { "Vitoria",NULL };
char *prov6[] = { "Oviedo","Aviles","Gijon","Pola de Siero",NULL };
char *prov7[] = { "Avila",NULL };
char *prov8[] = { "Badajoz","Almendralejo","Don Benito","Merida","Villanueva de la Serena",NULL };
char *prov9[] = { "Palma de Mallorca","Alaior","Cala Ratjada","Ciutadella","Eivissa","Inca","Manacor","Mao",NULL };
char *prov10[] = { "Barcelona","Abrera","Arenys de Mar","Badalona","Barbera del Valles","Berga","Calella","Cerdanyola del Valles","Cornella","El Prat de Llobregat","Gava","Granollers","Igualada","L'Hospitalet","Manresa","Mataro","Mollet","Pineda de Mar","Sabadell","Sant Cugat","Sant Feliu de Llobregat","Sitges","Terrassa","Vic","Viladecans","Vilafranca del Penedes","Vilanova i la Geltru",NULL };
char *prov11[] = { "Bilbao","Barakaldo","Basauri","Guetxo","Leioa","Ondarroa","Santurce",NULL };
char *prov12[] = { "Burgos",NULL };
char *prov13[] = { "Caceres","Coria","Plasencia",NULL };
char *prov14[] = { "Santander","Camargo","El Astillero","Los Corrales de Buelna","Noja",NULL };
char *prov15[] = { "Cadiz","Algeciras","Arcos de la Frontera","Chiclana","Jerez de la Frontera","La Linea","Los Barrios","Puerto de Santa Maria","Rota","San Fernando","Sanlucar de Barrameda",NULL };
char *prov16[] = { "Castellon","Benicarlo","Benicasim","Grao de Castellon","Segorbe","Vila-Real","Vinaros",NULL };
char *prov17[] = { "Ceuta",NULL };
char *prov18[] = { "Ciudad Real","Alcazar de San Juan","Puertollano","Valdepeñas",NULL };
char *prov19[] = { "Cordoba","Puente Genil",NULL };
char *prov20[] = { "Cuenca",NULL };
char *prov21[] = { "Girona","Blanes","Figueres","Olot","Palamos","Platja d'Aro","Ripoll","Roses",NULL };
char *prov22[] = { "Granada","Baza","Motril","Pulianas",NULL };
char *prov23[] = { "Guadalajara",NULL };
char *prov24[] = { "San Sebastian","Azkoitia","Beasain","Eibar","Irun","Oñati","Tolosa","Usurbil","Zarauz","Zumaia",NULL };
char *prov25[] = { "Huelva","Isla Cristina","Lepe",NULL };
char *prov26[] = { "Huesca","Barbastro","Binefar","Fraga","Graus","Jaca","Mequinenza","Monzon","Sabiñanigo","Tamarite de Litera",NULL };
char *prov27[] = { "Jaen","Andujar","Linares","Ubeda",NULL };
char *prov28[] = { "Leon","Cistierna","Ponferrada","Santa Maria",NULL };
char *prov29[] = { "Lleida","Agramunt","Alfarras","Balaguer","Bellpuig","Cervera","La Pobla de Segur","La Seu d'Urgell","Linyola","Mollerussa","Penelles","Pont de suert","Ponts","Solsona","Sort","Tarrega","Tremp","Vielha",NULL };
char *prov30[] = { "Las Palmas de Gran Canaria","Fuerteventura","Lanzarote","Santa Lucia",NULL };
char *prov31[] = { "Logroño","Calahorra","Viana",NULL };
char *prov32[] = { "Lugo","Foz","Monforte de Lemos",NULL };
char *prov33[] = { "Madrid","Alcala de Henares","Alcobendas","Alcorcon","Aranjuez","Arroyomolinos","Boadilla del Monte","Collado Villalba","Coslada","Fuenlabrada","Getafe","Las Rozas","Leganes","Majadahonda","Mejorada del Campo","Mostoles","Parla","Pozuelo de Alarcon","Rivas-Vaciamadrid","San Fernando de Henares","San Lorenzo de El Escorial","San Martin de Valdeiglesias","San Sebastian de los Reyes","Torrejon de Ardoz","Torrelodones","Tres Cantos","Valdemoro",NULL };
char *prov34[] = { "Malaga","Antequera","Benalmadena","Cala del Moral","Coin","Estepona","Fuengirola","Marbella","Mijas","Ronda","Velez",NULL };
char *prov35[] = { "Melilla",NULL };
char *prov36[] = { "Murcia","Aguilas","Alhama","Cartagena","Lorca","Los Alcazares","San Javier","Totana",NULL };
char *prov37[] = { "Pamplona","Estella","Huarte","Tudela",NULL };
char *prov38[] = { "Ourense",NULL };
char *prov39[] = { "Palencia","Aguilar de Campoo",NULL };
char *prov40[] = { "Pontevedra","Cangas","Lalin","Ponteareas","Teis","Vigo","Villagarcia de Arosa",NULL };
char *prov41[] = { "Salamanca","Bejar","Ciudad Rodrigo","Peñaranda de bracamonte",NULL };
char *prov42[] = { "Segovia",NULL };
char *prov43[] = { "Sevilla","Alcala de Guadaira","Bormujos","Camas","Dos Hermanas","Ecija","Tomares","Utrera",NULL };
char *prov44[] = { "Soria",NULL };
char *prov45[] = { "Tarragona","Amposta","Comarruga","Cubelles","El Vendrell","Montblanc","Reus","Roquetes",NULL };
char *prov46[] = { "Santa Cruz de Tenerife","Candelaria","Frontera (Isla de Hierro)","Icod de los Vinos","La Laguna","Playa de las Americas","Puerto de la Cruz","Villa de la Orotava",NULL };
char *prov47[] = { "Teruel","Alcorisa",NULL };
char *prov48[] = { "Toledo","Talavera de la Reina","Torrijos","Villacañas",NULL };
char *prov49[] = { "Valencia","Aldaya","Alfafar","Alzira","Canet d'Enberenger","Cullera","El Saler","Gandia","L'Eliana","Oliva","Onteniente","Paterna","Sagunt","Sedavi","Serra","Xativa","Xeraco","Xirivella",NULL };
char *prov50[] = { "Valladolid","Medina del Campo","Medina de Rio Seco","Peñafiel","Zaratan",NULL };
char *prov51[] = { "Zamora",NULL };
char *prov52[] = { "Zaragoza","Calatayud",NULL };
char **pueblos[] = { 
	prov1 , prov2 , prov3 , prov4 , prov5 , prov6 , prov7 , prov8 , prov9 , prov10 ,
	prov11 , prov12 , prov13 , prov14 , prov15 , prov16 , prov17 , prov18 , prov19 , prov20 ,
	prov21 , prov22 , prov23 , prov24 , prov25 , prov26 , prov27 , prov28 , prov29 , prov30 ,
	prov31 , prov32 , prov33 , prov34 , prov35 , prov36 , prov37 , prov38 , prov39 , prov40 ,
	prov41 , prov42 , prov43 , prov44 , prov45 , prov46 , prov47 , prov48 , prov49 , prov50 ,
	prov51 , prov52
};

char *prid1[] = { "6","2223","45","2118","1294","52",NULL };
char *prid2[] = { "123","1133","293",NULL };
char *prid3[] = { "43","201","55","140","143","202","291","56","142","203","57",NULL };
char *prid4[] = { "118","174","279","2185",NULL };
char *prid5[] = { "51",NULL };
char *prid6[] = { "115","185","116","1129",NULL };
char *prid7[] = { "59",NULL };
char *prid8[] = { "122","1231","278","156","1262",NULL };
char *prid9[] = { "36","182","1260","181","267","178","272","180",NULL };
char *prid10[] = { "1","1112","67","68","69","233","213","71","73","1145","75","77","72","78","79","65","81","83","86","87","88","1140","90","98","287","1166","1302",NULL };
char *prid11[] = { "4","108","107","110","109","1161","1293",NULL };
char *prid12[] = { "62",NULL };
char *prid13[] = { "158","255","151",NULL };
char *prid14[] = { "124","166","1111","2190","2191",NULL };
char *prid15[] = { "163","149","1135","150","147","155","2175","154","1283","152","2235",NULL };
char *prid16[] = { "42","53","1315","2205","170","1131","54",NULL };
char *prid17[] = { "198",NULL };
char *prid18[] = { "145","218","1212","2231",NULL };
char *prid19[] = { "148","289",NULL };
char *prid20[] = { "112",NULL };
char *prid21[] = { "76","211","93","94","100","95","234","285",NULL };
char *prid22[] = { "117","1089","217","2228",NULL };
char *prid23[] = { "111",NULL };
char *prid24[] = { "2176","1169","1170","1254","281","1171","1172","264","1256","1173",NULL };
char *prid25[] = { "153","212","1090",NULL };
char *prid26[] = { "34","241","237","240","239","242","235","238","243","236",NULL };
char *prid27[] = { "119","244","176","177",NULL };
char *prid28[] = { "60","1124","1136","295",NULL };
char *prid29[] = { "63","2194","230","221","2193","222","226","1188","223","219","1290","276","1139","224","227","220","225","228",NULL };
char *prid30[] = { "188","2180","192","283",NULL };
char *prid31[] = { "113","268","2233",NULL };
char *prid32[] = { "49","253","256",NULL };
char *prid33[] = { "2","8","9","10","105","2178","12","14","16","17","18","19","20","21","22","23","24","26","27","1137","33","28","2229","29","280","31","273",NULL };
char *prid34[] = { "125","245","1692","1312","2226","159","1088","161","246","247","209",NULL };
char *prid35[] = { "164",NULL };
char *prid36[] = { "120","1128","284","191","197","2173","2224","275",NULL };
char *prid37[] = { "132","196","2234","282",NULL };
char *prid38[] = { "50",NULL };
char *prid39[] = { "126","1211",NULL };
char *prid40[] = { "48","286","2222","252","2208","46","254",NULL };
char *prid41[] = { "61","2241","1085","1083",NULL };
char *prid42[] = { "128",NULL };
char *prid43[] = { "7","210","1317","250","1091","160","248","2174",NULL };
char *prid44[] = { "146",NULL };
char *prid45[] = { "64","1093","1247","1291","232","231","84","96",NULL };
char *prid46[] = { "190","1115","277","1134","274","2128","189","1318",NULL };
char *prid47[] = { "35","249",NULL };
char *prid48[] = { "165","297","1316","1147",NULL };
char *prid49[] = { "5","2018","2237","1168","251","2179","206","38","39","1096","1151","1108","41","134","2225","168","2186","135",NULL };
char *prid50[] = { "58","1127","1087","1084","2009",NULL };
char *prid51[] = { "129",NULL };
char *prid52[] = { "3","1289",NULL };
char **prids[] = {
	prid1 , prid2 , prid3 , prid4 , prid5 , prid6 , prid7 , prid8 , prid9 , prid10 ,
	prid11 , prid12 , prid13 , prid14 , prid15 , prid16 , prid17 , prid18 , prid19 , prid20 ,
	prid21 , prid22 , prid23 , prid24 , prid25 , prid26 , prid27 , prid28 , prid29 , prid30 ,
	prid31 , prid32 , prid33 , prid34 , prid35 , prid36 , prid37 , prid38 , prid39 , prid40 ,
	prid41 , prid42 , prid43 , prid44 , prid45 , prid46 , prid47 , prid48 , prid49 , prid50 ,
	prid51 , prid52
};

DataSock *BuscaCola(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
			return cola[i];
	}
	return NULL;
}
DataSock *InsertaCola(char *query, int numero, Cliente *cl)
{
	int i;
	for (i = 0; i < MAX_COLA && cola[i]; i++);
	if (i < MAX_COLA)
	{
		DataSock *dts;
		BMalloc(dts, DataSock);
		if (query)
			ircstrdup(dts->query, query);
		dts->numero = numero;
		dts->cl = cl;
		cola[i] = dts;
		return dts;
	}
	return NULL;
}
int BorraCola(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
		{
			ircfree(cola[i]->query);
			ircfree(cola[i]);
			return 1;
		}
	}
	return 0;
}
int TSBuscaCadena(char *nombre)
{
	int i;
	for (i = 0; cadenas[i].nombre; i++)
	{
		if (!strcasecmp(nombre, cadenas[i].nombre))
			return i;
	}
	return -1;
}
int TSBuscaHoroscopo(char *horo)
{
	int i;
	for (i = 0; horoscopos[i]; i++)
	{
		if (!strcasecmp(horoscopos[i], horo))
			return i+1;
	}
	return 0;
}
int MOD_CARGA(TvServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	mod->activo = 1;
	if (mod->config)
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
	else
		TSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(TvServ)()
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i])
			SockClose(cola[i]->sck, LOCAL);
	}
	BorraSenyal(SIGN_SQL, TSSigSQL);
	BorraSenyal(SIGN_QUIT, TSSigQuit);
	BotUnset(tvserv);
	return 0;
}
int TSTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], tvserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void TSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!tvserv)
		BMalloc(tvserv, TvServ);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, tvserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, tvserv_coms);
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
		ProcesaComsMod(NULL, mod, tvserv_coms);
	InsertaSenyal(SIGN_SQL, TSSigSQL);
	InsertaSenyal(SIGN_QUIT, TSSigQuit);
	bzero(cola, sizeof(DataSock) * MAX_COLA);
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
			if (!strncmp("&nbsp;", c, 6))
			{
				*dest++ = ' ';
				c += 6;
			}
			else if (!strncmp("acute;", c+2, 6))
			{
				switch (*(c+1))
				{
					case 'a':
					case 'A':
						*dest++ = *(c+1)+128;
						break;
					case 'u':
					case 'U':
						*dest++ = *(c+1)+133;
						break;
					default:
						*dest++ = *(c+1)+132;
				}
				c += 8;
			}
			else if (!strncmp("&quot;", c, 6))
			{
				*dest++ = '"';
				c += 6;
			}
			else if (!strncmp("tilde;", c+2, 6))
			{
				*dest++ = *(c+1)+131;
				c += 8;
			}
			else if (*(c+1) == '#')
			{
				char tmp[5];
				int i;
				c += 2;
				for (i = 0; i < 4 && *c != ';'; i++)
					tmp[i] = *c++;
				tmp[i] = '\0';
				*dest++ = atoi(tmp);
				c++;
			}
			else if (!strncmp("uml;", c+2, 4))
			{
				switch (*(c+1))
				{
					case 'a':
					case 'A':
						*dest++ = *(c+1)+131;
						break;
					case 'e':
					case 'E':
					case 'i':
					case 'I':
						*dest++ = *(c+1)+134;
						break;
					case 'o':
					case 'O':
					case 'u':
					case 'U':
						*dest++ = *(c+1)+135;
						break;
				}
				c += 6;
			}
			else
				*dest++ = *c++;
		}
		else if (*c == '\t')
			c++;
		else if (*c == '\'')
		{
			*dest++ = '\\';
			*dest++ = '\'';
			c++;
		}
		else if (*c == '%')
		{
			*dest++ = '%';
			*dest++ = *c++;
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
	ts -= hora * 3600;
    	timeptr = localtime(&ts); 
    	ircsprintf(TSFecha, "%.2d/%.2d/%.2d", timeptr->tm_mday, timeptr->tm_mon + 1, timeptr->tm_year - 100);
    	//ircsprintf(TSFecha, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d", timeptr->tm_mday, timeptr->tm_mon + 1, timeptr->tm_year - 100, timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
    	return TSFecha;
}
BOTFUNC(TSTv)
{
	int cdn;
	SQLRes res;
	char *tsf = TSFecha(time(NULL));
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, fc->com, "cadena");
		return 1;
	}
	if ((cdn = TSBuscaCadena(param[1])) < 0)
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Esta cadena no existe");
		return 1;
	}
	if (!(res = SQLQuery("SELECT programacion from %s%s where fecha='%s' AND LOWER(item)='%s'", PREFIJO, TS_TV, tsf, strtolower(cadenas[cdn].nombre))))
	{
		DataSock *dts;
		SQLQuery("TRUNCATE TABLE %s%s", PREFIJO, TS_TV);
		ircsprintf(buf, "/teletexto.asp?programacion=%s&tv=%c", cadenas[cdn].texto, cadenas[cdn].tipo);
		if ((dts = InsertaCola(buf, cdn, cl)))
			dts->sck = SockOpen("www.teletexto.com", 80, TSAbreDataSock, TSLeeTv, NULL, TSCierraDataSock);
		else
		{
			Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está ocupado. Pruebe dentro de unos segundos");
			return 1;
		}
	}
	else
	{
		SQLRow row;
		Responde(cl, CLI(tvserv), "\00312%s\003 (%s)", param[1], tsf);
		while ((row = SQLFetchRow(res)))
			Responde(cl, CLI(tvserv), row[0]);
	}
	return 0;
}
BOTFUNC(TSHoroscopo)
{
	SQLRes res;
	SQLRow row;
	char *tsf = TSFecha(time(NULL));
	int h;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, fc->com, "signo");
		return 1;
	}
	if (!(h = TSBuscaHoroscopo(param[1])))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Este horóscopo no existe");
		return 1;
	}
	if (!(res = SQLQuery("SELECT prediccion from %s%s where fecha='%s' AND LOWER(item)='%s'", PREFIJO, TS_HO, tsf, strtolower(param[1]))))
	{
		DataSock *dts;
		SQLQuery("TRUNCATE TABLE %s%s", PREFIJO, TS_HO);
		ircsprintf(buf, "/teletexto.asp?horoscopo=%i", h);
		if ((dts = InsertaCola(buf, h, cl)))
			dts->sck = SockOpen("www.teletexto.com", 80, TSAbreDataSock, TSLeeHoroscopo, NULL, TSCierraDataSock);
		else
		{
			Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está ocupado. Pruebe dentro de unos segundos");
			return 1;
		}
	}
	else
	{
		Responde(cl, CLI(tvserv), "\00312%s\003 (%s)", param[1], tsf);
		while ((row = SQLFetchRow(res)))
			Responde(cl, CLI(tvserv), row[0]);
	}
	return 0;
}
BOTFUNC(TSTiempo)
{
	char pueblo[256];
	DataSock *dts;
	int n;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, fc->com, "[nº] pueblo");
		return 1;
	}
	if (CogeCache(CACHE_TIEMPO, strchr(cl->mask, '!') + 1, tvserv->hmod->id))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Debes esperar almenos 30 segundos antes de volver a usar este servicio");
		return 1;
	}
	/*if (*param[1] == '\'')
	{
		param[1]++;
		for (i = 1; i < params; i++)
		{
			if ((c = strchr(param[i], '\'')))
			{
				*c = '\0';
				strncpy(pueblo, Unifica(param, params, 1, i), sizeof(pueblo));
				break;
			}
		}
		if (i == params)
		{
			Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Has olvidado cerrar comillas");
			return 1;
		}
	}
	else
		strncpy(pueblo, param[1], sizeof(pueblo));
	
	i++;
	if (*param[i] == '\'')
	{
		int j;
		param[i]++;
		for (j = i; j < params; j++)
		{
			if ((c = strchr(param[j], '\'')))
			{
				*c = '\0';
				strncpy(pueblo, Unifica(param, params, i, j), sizeof(pueblo));
				break;
			}
		}
		if (j == params)
		{
			Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Has olvidado cerrar comillas");
			return 1;
		}
	}
	else
		strncpy(pueblo, param[i], sizeof(pueblo));
	if (!(p = TSBuscaProvincia(provincia)))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Esta provincia no existe");
		return 1;
	}
	*/
	if (!IsDigit(*param[1]))
	{
		strncpy(pueblo, str_replace(Unifica(param, params, 1, -1), ' ', '+'), sizeof(pueblo));
		n = 0;
	}
	else
	{
		strncpy(pueblo, str_replace(Unifica(param, params, 2, -1), ' ', '+'), sizeof(pueblo));
		n = atoi(param[1]);
	}
	strncpy(pueblo, str_replace(pueblo, 'ñ', 'n'), sizeof(pueblo));
	//strncpy(provincia, str_replace(p, ' ', '+'), sizeof(provincia));
	//strncpy(provincia, str_replace(p, 'ñ', 'n'), sizeof(provincia));
	ircsprintf(buf, "/buscar.php?criterio=%s", pueblo);
	if ((dts = InsertaCola(buf, n, cl)))
		dts->sck = SockOpen("tiempo.meteored.com", 80, TSAbreDataSock, TSLeeTiempo, NULL, TSCierraDataSock);
	else
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está ocupado. Pruebe dentro de unos segundos");
		return 1;
	}
	return 0;
}
BOTFUNC(TSCine)
{
	char pueblo[256], *p;
	int i, j, b = 0, n;
	DataSock *dts;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, fc->com, "[nº] pueblo");
		return 1;
	}
	if (CogeCache(CACHE_TIEMPO, strchr(cl->mask, '!') + 1, tvserv->hmod->id))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Debes esperar almenos 30 segundos antes de volver a usar este servicio");
		return 1;
	}
	if (!IsDigit(*param[1]))
	{
		p = Unifica(param, params, 1, -1);
		n = 0;
	}
	else
	{
		p = Unifica(param, params, 2, -1);
		n = atoi(param[1]);
	}
	for (i = 0; i < 52 && !b; i++)
	{
		for (j = 0; pueblos[i][j]; j++)
		{
			if (!strcasecmp(pueblos[i][j], p))
			{
				b = 1;
				break;
			}
		}
	}
	if (!b)
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Este pueblo no dispone de un cine");
		return 1;
	}
	strncpy(pueblo, str_replace(p, ' ', '+'), sizeof(pueblo));
	strncpy(pueblo, str_replace(pueblo, 'ñ', 'n'), sizeof(pueblo));
	ircsprintf(buf, "/cartelera/?provincia=%i&buscar=&municipio=%s", i, prids[i-1][j]);
	if ((dts = InsertaCola(buf, n, cl)))
		dts->sck = SockOpen("www.elmundo.es", 80, TSAbreDataSock, TSLeeCine, NULL, TSCierraDataSock);
	else
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está ocupado. Pruebe dentro de unos segundos");
		return 1;
	}
	return 0;
}
BOTFUNC(TSPelis)
{
	DataSock *dts;
	int n = 0;
	if (CogeCache(CACHE_TIEMPO, strchr(cl->mask, '!') + 1, tvserv->hmod->id))
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Debes esperar almenos 30 segundos antes de volver a usar este servicio");
		return 1;
	}
	if (params > 1)
	{
		if (!IsDigit(*param[1]))
		{
			ircsprintf(buf, "/metropoli/cine.html?%s", Unifica(param, params, 1, -1));
			strncpy(buf, str_replace(buf, ' ', '+'), sizeof(buf));
		}
		else
		{
			strcpy(buf, "/metropoli/cine.html");
			n = atoi(param[1]);
		}
	}
	else
	{
		strcpy(buf, "/metropoli/cine.html");
		n = -1;
	}
	if ((dts = InsertaCola(buf, n, cl)))
		dts->sck = SockOpen("www.elmundo.es", 80, TSAbreDataSock, TSLeePeli, NULL, TSCierraDataSock);
	else
	{
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "El servicio está ocupado. Pruebe dentro de unos segundos");
		return 1;
	}
	return 0;
}
BOTFUNC(TSLiga)
{
	SQLRes res;
	SQLRow row;
	int i;
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), TS_ERR_PARA, fc->com, "{1a|2a}");
		return 1;
	}
	if (strcasecmp(param[1], "1a") && strcasecmp(param[1], "2a"))
	{
		Responde(cl, CLI(tvserv), TS_ERR_SNTX, "Sólo se aceptan 1a y 2a división");
		return 1;
	}
	if (!(res = SQLQuery("SELECT * from %s%s where division='%c' AND fecha='%s'", PREFIJO, TS_LI, *param[1], TSFecha(time(0)))))
	{
		DataSock *dts;
		SQLQuery("TRUNCATE TABLE %s%s", PREFIJO, TS_LI);
		ircsprintf(buf, "/edicion/marca/futbol/%ca_division/es/index.html", *param[1]);
		if ((dts = InsertaCola(buf, 1, cl)))
			dts->sck = SockOpen("www.marca.com", 80, TSAbreDataSock, TSLeeLiga, NULL, TSCierraDataSock);
	}
	else
	{
		Responde(cl, CLI(tvserv), "Clasificación de la Liga \00312%ca\003 división", *param[1]);
		for (i = 1; (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(tvserv), "\00312%i\003 %s - %s", i, row[0], row[2]);
	}
	return 0;
}
BOTFUNCHELP(TSHTv)
{
	int i;
	Responde(cl, CLI(tvserv), "Muestra la programación de las distintas cadenas de la televisión de España.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Cadenas disponibles:");
	for (i = 0; cadenas[i].nombre; i++)
		Responde(cl, CLI(tvserv), cadenas[i].nombre);
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312TV cadena");
	return 0;
}
BOTFUNCHELP(TSHHoroscopo)
{
	int i;
	Responde(cl, CLI(tvserv), "Muestra la predicción del día del signo zodiacal que se indique.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Zodiacos disponibles:");
	for (i = 0; horoscopos[i]; i++)
		Responde(cl, CLI(tvserv), horoscopos[i]);
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312HOROSCOPO signo");
	return 0;
}
BOTFUNCHELP(TSHTiempo)
{
	Responde(cl, CLI(tvserv), "Consulta la previsión meteorológica para un pueblo/ciudad del mundo.");
	Responde(cl, CLI(tvserv), "Este comando realiza una búsqueda y muestra el resultado.");
	Responde(cl, CLI(tvserv), "En caso de encontrar varias coincidencias con el pueblo/ciudad proporcionado, se mostrará una lista con varios resultados precedidos por un número.");
	Responde(cl, CLI(tvserv), "Para consultar el tiempo de un elemento de esta lista, tendrá que especificar el número que le precede.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312TIEMPO [nº] pueblo/ciudad");
	return 0;
}
BOTFUNCHELP(TSHCine)
{
	Responde(cl, CLI(tvserv), "Realiza una búsqueda de la cartelera de un pueblo/ciudad.");
	Responde(cl, CLI(tvserv), "Si el pueblo dispone de un cine o más, se mostrará una lista con todos los cines precedidos por un número.");
	Responde(cl, CLI(tvserv), "Este número es el que tendrá que especificar nuevamente para consultar la cartelera, seguido del pueblo al que pertenece.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312CINE [nº] pueblo/ciudad");
	return 0;
}
BOTFUNCHELP(TSHPelis)
{
	Responde(cl, CLI(tvserv), "Muestra información sobre una película que se esté proyectando en los cines españoles.");
	Responde(cl, CLI(tvserv), "Si no se especifica ningún parámetro, se listarán todas las películas reconocidas precedidas por un número.");
	Responde(cl, CLI(tvserv), "También es posible efectuar esta consulta a partir del número mencionado.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312PELIS [película|nº]");
	return 0;
}
BOTFUNCHELP(TSHLiga)
{
	Responde(cl, CLI(tvserv), "Muestra la clasificación de la Liga española de fútbol, de primera y segunda división.");
	Responde(cl, CLI(tvserv), " ");
	Responde(cl, CLI(tvserv), "Sintaxis: \00312LIGA {1a|2a}");
	return 0;
}
BOTFUNC(TSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(tvserv), "\00312%s\003 es un servicio de ocio para los usuarios que ofrece distintos servicios.", tvserv->hmod->nick);
		Responde(cl, CLI(tvserv), "Todos los servicios se actualizan a diario.");
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Servicios prestados:");
		ListaDescrips(tvserv->hmod, cl);
		Responde(cl, CLI(tvserv), " ");
		Responde(cl, CLI(tvserv), "Para más información, \00312/msg %s %s comando", tvserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], tvserv->hmod, param, params))
		Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
SOCKFUNC(TSAbreDataSock)
{
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	SockWrite(sck, "GET %s HTTP/1.1", dts->query);
	SockWrite(sck, "Accept: */*");
	SockWrite(sck, "Host: %s", sck->host);
	SockWrite(sck, "Connection: close");
	SockWrite(sck, "");
	return 0;
}
SOCKFUNC(TSCierraDataSock)
{
	BorraCola(sck);
	return 0;
}
SOCKFUNC(TSLeeTv)
{
	static char imp = 0, txt[BUFSIZE], *tsf;
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	if (!imp && !strncmp("<td width=12", data, 12))
	{
		imp = 1;
		txt[0] = '\0';
	}
	else if (imp && !strcmp(data, "\t"))
	{
		SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_TV, cadenas[dts->numero].nombre, tsf, txt[0] == ' ' ? &txt[1] : &txt[0]);
		Responde(dts->cl, CLI(tvserv), txt[0] == ' ' ? &txt[1] : &txt[0]);
		imp = 0;
	}
	else if (strstr(data, "</html>"))
		SockClose(sck, LOCAL);
	else if (!strncmp(data, "HTTP/", 5))
	{
		tsf = TSFecha(time(0));
		Responde(dts->cl, CLI(tvserv), "\00312%s\003 (%s)", cadenas[dts->numero].nombre, tsf);
	}
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
SOCKFUNC(TSLeeHoroscopo)
{
	static char txt[BUFSIZE], tmp[BUFSIZE], *tsf;
	static int salud = 0, dinero = 0, amor = 0, cat = 0;
	DataSock *dts;
	char *c;
	if (!(dts = BuscaCola(sck)))
		return 1;
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
			SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_HO, horoscopos[dts->numero - 1], tsf, txt);
			Responde(dts->cl, CLI(tvserv), txt);
			cat = 0;
		}
	}
	else if (strstr(data, "dinero.gif"))
		dinero = StrCount(data, "dinero.gif");
	else if (strstr(data, "salud.gif"))
		salud = StrCount(data, "salud.gif");
	else if (strstr(data, "amor.gif"))
		amor = StrCount(data, "amor.gif");
	else if (!strcmp(data, "</td></tr><tr><td>"))
	{
		ircsprintf(txt, "Salud: \00312%s\003 - Dinero: \00312", Repite('*', salud));
		strcat(txt, Repite('*', dinero));
		strcat(txt, "\003 - Amor: \00312");
		strcat(txt, Repite('*', amor));
		SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s')", PREFIJO, TS_HO, horoscopos[dts->numero - 1], tsf, txt);
		salud = dinero = amor = 0;
		SockClose(sck, LOCAL);
		Responde(dts->cl, CLI(tvserv), txt);
	}
	else if (!strncmp(data, "HTTP/", 5))
	{
		tsf = TSFecha(time(0));
		Responde(dts->cl, CLI(tvserv), "\00312%s\003 (%s)", horoscopos[dts->numero - 1], tsf);
	}
	return 0;
}
SOCKFUNC(TSLeeTiempo)
{
	char tmp[SOCKBUF], *c, *d;
	DataSock *dts;
	Cliente *cl;
	static int dia = 0, sig = 0;
	int numero;
	if (!(dts = BuscaCola(sck)))
		return 1;
	cl = dts->cl;
	numero = dts->numero;
	if (!strncmp("<font size=1", data, 12))
	{
		int regs;
		TSQuitaTags(data, tmp);
		sscanf(tmp, "Número de registros encontrados: %i", &regs);
		c = strstr(data, "f'>");
		c += 4;
		if (regs == 1)
		{
			if ((c = strchr(c, '\'')))
			{
				c++;
				d = strchr(c, '\'');
				*d = '\0';
				if ((d = strrchr(c, '.')))
					*d = '\0';
				ircsprintf(buf, "/%s-%i.html", c, ++dia);
				strncpy(buf, str_replace(buf, ' ', '+'), sizeof(buf));
				SockClose(sck, LOCAL);	
				if ((dts = InsertaCola(buf, 0, cl)))
					dts->sck = SockOpen("tiempo.meteored.com", 80, TSAbreDataSock, TSLeeTiempo, NULL, TSCierraDataSock);
			}
		}
		else if (regs > 1)
		{
			if (!dts->numero)
			{
				int i;
				Responde(cl, CLI(tvserv), "Se han encontrado varias coincidencias. Use /msg %s TIEMPO <nº> '%s'", CLI(tvserv)->nombre, strchr(dts->query, '=')+1);
				for (c = data, i = 1; (c = strstr(c, ".html")); i++)
				{
					c += 7;
					if ((d = strstr(c, "</a>")))
					{
						*d = '\0';
						Responde(cl, CLI(tvserv), "\00312%i\003.- %s", i, c);
					}
					c = d+1;
				}
				SockClose(sck, LOCAL);	
			}
			else if (dts->numero > regs)
			{
				Responde(cl, CLI(tvserv), TS_ERR_EMPT, "Este número no existe. Vuelva a ejecutar la búsqueda");
				return 1;
			}
			else
			{
				int i;
				for (c = data, i = 1; (c = strstr(c, "<a href='")); i++)
				{
					if (i == dts->numero)
					{
						c += 9;
						d = strchr(c, '\'');
						*d = '\0';
						if ((d = strrchr(c, '.')))
						{
							*d = '\0';
							ircsprintf(buf, "/%s-%i.html", c, ++dia);
							strncpy(buf, str_replace(buf, ' ', '+'), sizeof(buf));	
							SockClose(sck, LOCAL);	
							if ((dts = InsertaCola(buf, 0, cl)))
								dts->sck = SockOpen("tiempo.meteored.com", 80, TSAbreDataSock, TSLeeTiempo, NULL, TSCierraDataSock);
						}
						break;
					}
					else
						c = strstr(c, "</a>");
				}
			}
		}
		else
		{
			Responde(cl, CLI(tvserv), TS_ERR_EMPT, "No se han encontrado coincidencias");
			SockClose(sck, LOCAL);
			return 1;
		}
	}
	else if (!strncmp("<!-- M", data, 6))
		sig = 1;
	else if (sig)
	{
		char *tok, *c;
		for (tok = strstr(data, "<tr>"); tok; tok = strstr(tok, "<tr>"))
		{
			c = strstr(tok, "</tr>");
			*c = '\0';
			TSQuitaTags(tok, tmp);
			Responde(cl, CLI(tvserv), tmp);
			tok = c+1;
			if (!strncmp(tok+12, "<font size", 10))
			{
				tok += 12;
				c = strstr(tok, "</font");
				*c = '\0';
				TSQuitaTags(tok, tmp);
				Responde(cl, CLI(tvserv), tmp);
				tok = c+1;
			}	
		}
		dia = sig = 0;
		if (!IsOper(cl))
			InsertaCache(CACHE_TIEMPO, strchr(cl->mask, '!') + 1, 30, tvserv->hmod->id, "%lu", time(0));
		SockClose(sck, LOCAL);
	}
	return 0;
}
SOCKFUNC(TSLeeCine)
{
	static int m = 0, n = 0, i = 1;
	char tmp[SOCKBUF], *c, *d;
	Cliente *cl;
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	cl = dts->cl;
	//Debug("%s", data);
	if (m)
	{
		if (*(data+1) == '/')
		{
			SockClose(sck, LOCAL);
			m = 0;
			i = 1;
		}
		else
		{
			if (!dts->numero)
			{
				TSQuitaTags(data, tmp);
				Responde(cl, CLI(tvserv), "\00312%i\003.- %s", i++, tmp);
			}
			else if (i++ == dts->numero)
			{
				c = strchr(data, '"')+1;
				d = strchr(c, '"');
				*d = '\0';
				SockClose(sck, LOCAL);
				if ((dts = InsertaCola(c, 0, cl)))
					dts->sck = SockOpen("www.elmundo.es", 80, TSAbreDataSock, TSLeeCine, NULL, TSCierraDataSock);
				m = 0;
			}
		}
	}
	else if (n)
	{
		if (strstr(data, "VOLVER"))
		{
			m = n = 0;
			i = 1;
			SockClose(sck, LOCAL);
			if (!IsOper(cl))
				InsertaCache(CACHE_TIEMPO, strchr(cl->mask, '!') + 1, 30, tvserv->hmod->id, "%lu", time(0));
		}
		else
		{
			if (!strncmp("</ta", data, 4))
				Responde(cl, CLI(tvserv), " ");
			else
			{
				TSQuitaTags(data, tmp);
				if (!strncmp("<a", data, 2))
					Responde(cl, CLI(tvserv), "%c%s", 31, tmp);
				else
				{
					for (i = 0; tmp[i] == ' ' && tmp[i] != '\0'; i++);
					if (tmp[i] != '\0')
						Responde(cl, CLI(tvserv), &tmp[i]);
				}
			}
		}
	}
	else if (!strncmp("<td align=\"l", data, 12))
	{
		n = 1;
		TSQuitaTags(data, tmp);
		if (tmp[0] != '\0')
			Responde(cl, CLI(tvserv), tmp);
	}
	else if (strstr(data, "Seleccione un cine"))
	{
		if (!dts->numero)
		{
			int prov, i, j, k = 1;
			char t[8];
			sscanf(dts->query, "/cartelera/?provincia=%i&buscar=&municipio=%s", &prov, t);
			for (i = 0; i < 52 && k; i++)
			{
				for (j = 0; prids[i][j]; j++)
				{
					if (!strcasecmp(t, prids[i][j]))
					{
						Responde(cl, CLI(tvserv), "Se han encontrado varios cines en esta localidad. Use /msg %s CINE <nº> '%s'", CLI(tvserv)->nombre, pueblos[i][j]);
						k = 0;
						break;
					}
				}
			}
		}
		m = 1;
	}
	return 0;
}
SOCKFUNC(TSLeePeli)
{
	static int m = 0, n = 0, i = 1;
	char tmp[SOCKBUF], *c, *d;
	static char snd[BUFSIZE];
	Cliente *cl;
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	cl = dts->cl;
	if (m)
	{
		if (!strncmp("</s", data, 3))
		{
			i = 1;
			m = 0;
			SockClose(sck, LOCAL);
			if (dts->numero != -1)
				Responde(cl, CLI(tvserv), TS_ERR_EMPT, "No se ha encontrado esta película");
		}
		else
		{
			if (dts->numero == -1)
			{
				TSQuitaTags(data, tmp);
				if (tmp[0] != '\0')
					Responde(cl, CLI(tvserv), "\00312%i\003.- %s", i++, tmp);
			}
			else if (!dts->numero)
			{
				TSQuitaTags(data, tmp);
				c = strchr(dts->query, '?')+1;
				if (!strcasecmp(tmp, c))
				{
					reloc:
					c = strchr(data, '"')+1;
					d = strchr(c, '?');
					*d = '\0';
					SockClose(sck, LOCAL);
					if ((dts = InsertaCola(c, 0, cl)))
						dts->sck = SockOpen("www.elmundo.es", 80, TSAbreDataSock, TSLeePeli, NULL, TSCierraDataSock);
					m = 0;
					i = 1;
				}
			}
			else if (dts->numero == i++)
				goto reloc;
		}
	}
	else if (n)
	{
		if (!strncmp("</tr>", data, 5))
		{
			if (snd[0] != '\0')
			{
				if (snd[0] == ' ' && snd[1] != '\0')
					Responde(cl, CLI(tvserv), &snd[1]);
				else
					Responde(cl, CLI(tvserv), snd);
				snd[0] = '\0';
			}
		}
		else if (!strncmp(data, "</ta", 4))
				strcat(snd, " ");
		else if (!strncmp("<td height=\"6", data, 13))
		{
			n = 0;
			snd[0] = '\0';
			SockClose(sck, LOCAL);
		}
		else
		{
			TSQuitaTags(data, tmp);
			if (tmp[0] != '\0')
			{
				if (tmp[0] != ' ')
					strcat(snd, " ");
				strcat(snd, tmp);
			}
		}
	}
	else if (!strncmp("<option s", data, 9))
	{
		m = 1;
		ircstrdup(dts->query, str_replace(dts->query, '+', ' '));
	}
	else if (strstr(data, "&nbsp;CARTELERA"))
	{
		snd[0] = '\0';
		n = 1;
	}
	return 0;
}
SOCKFUNC(TSLeeLiga)
{
	static int m = 0, j = 1;
	int i;
	static char tmp2[BUFSIZE], *c, *d, *e, *tsf;
	char tmp[SOCKBUF];
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	if (m)
	{
		if (!strncmp(data, " <tr", 4))
			tmp2[0] = '\0';
		else if (!strncmp(data, " </tr", 5))
		{
			c = strchr(tmp2, '\t');
			c++;
			d = strchr(c, '\t');
			*d++ = '\0';
			e = strchr(d, '\t');
			*e = '\0';
			SQLQuery("INSERT INTO %s%s values ('%s', '%s', '%s', %i)", PREFIJO, TS_LI, c, tsf, d, dts->numero);
			Responde(dts->cl, CLI(tvserv), "\00312%i\003 %s - %s", j++, c, d);
		}
		else if (!strncmp(data, "<!-- ^", 6))
		{
			m = 0;
			j = 1;
			SockClose(sck, LOCAL);
		}
		else
		{
			TSQuitaTags(data, tmp);
			if (tmp[0] != '\0')
			{
				for (i = 0; tmp[i] == ' '; i++);
				strcat(tmp2, &tmp[i]);
				strcat(tmp2, "\t");
			}
		}
	}	
	else if (!strncmp(data, "<!-- v", 6))
	{
		tmp2[0] = '\0';
		m = 1;
	}
	else if (!strncmp(data, "HTTP/", 5))
	{
		tsf = TSFecha(time(0));
		Responde(dts->cl, CLI(tvserv), "Clasificación de la Liga \00312%ia\003 división", dts->numero);
	}
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
	if (!SQLEsTabla(TS_LI))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"fecha varchar(255) default NULL, "
  			"puntos int2 default '0', "
  			"division int2 default '0' "
			");", PREFIJO, TS_LI))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, TS_LI);
	}
	SQLCargaTablas();
	return 1;
}
int TSSigQuit(Cliente *cl, char *mensaje)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->cl == cl)
		{
			SockClose(cola[i]->sck, LOCAL);
			break;
		}
	}
	return 0;
}
