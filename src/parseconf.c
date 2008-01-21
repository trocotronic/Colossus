/*
 * $Id: parseconf.c,v 1.34 2008-01-21 19:46:46 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#else
#include <netdb.h>
#include <time.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"

struct Conf_server *conf_server = NULL;
struct Conf_db *conf_db = NULL;
struct Conf_smtp *conf_smtp = NULL;
struct Conf_set *conf_set = NULL;
struct Conf_log *conf_log = NULL;
#ifdef USA_SSL
struct Conf_ssl *conf_ssl = NULL;
#endif
struct Conf_httpd *conf_httpd = NULL;
#ifdef USA_SSL
struct Conf_msn *conf_msn = NULL;
#endif

static int TestServer		(Conf *, int *);
static int TestDb 			(Conf *, int *);
static int TestSmtp		(Conf *, int *);
static int TestSet		(Conf *, int *);
static int TestModulos	(Conf *, int *);
static int TestLog		(Conf *, int *);
#ifdef USA_SSL
static int TestSSL		(Conf *, int *);
#endif
static int TestProtocolo	(Conf *, int *);
static int TestHttpd (Conf *, int *);
#ifdef USA_SSL
static int TestMSN	(Conf *, int *);
#endif

static void ConfServer	(Conf *);
static void ConfDb 		(Conf *);
static void ConfSmtp		(Conf *);
static void ConfSet		(Conf *);
static void ConfLog		(Conf *);
#ifdef USA_SSL
static void ConfSSL		(Conf *);
#endif
static void ConfModulos	(Conf *);
static void ConfHttpd	(Conf *);
#ifdef USA_SSL
static void ConfMSN	(Conf *);
#endif

extern int IniciaHTTPD();
extern int DetieneHTTPD();
extern int CargaMSN();
extern int DescargaMSN();

/* el ultimo parámetro es la obliguetoriedad:
   Si es OBL, significa que es obligatorio a partir de la version dada (incluida)
   Si es OPC, significa que es opcional a partir de la versión dada (incluida)
   */
static cComConf cComs[] = {
	{ "server" , TestServer, ConfServer , OBL , 1 } ,
	{ "db" , TestDb, ConfDb , OBL ,  1 } ,
#ifdef _WIN32
	{ "smtp" , TestSmtp, ConfSmtp , OPC , 5 } , /* es opcional a partir de la version 5 */
#else
	{ "smtp" , TestSmtp, ConfSmtp , OPC , 1 } ,
#endif
	{ "set" , TestSet, ConfSet , OBL , 1 } ,
	{ "modulo" , TestModulos, ConfModulos , OPC , 1 } ,
	{ "log" , TestLog , ConfLog , OPC , 1 } ,
#ifdef USA_SSL
	{ "ssl" , TestSSL , ConfSSL , OPC , 1 } ,
#endif
	{ "protocolo" , TestProtocolo , NULL , OBL , 1 } ,
	{ "httpd" , TestHttpd , ConfHttpd , OPC , 1 } ,
#ifdef USA_SSL
	{ "msn" , TestMSN , ConfMSN , OPC , 1 } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};
struct bArchivos
{
	char *serv;
	char *archivo;
	int (*test)(Conf *modulo, int *error);
	void (*set)(Conf *modulo, int id);
	int id;
};
#ifdef USA_SSL
static Opts Opts_SSL[] = {
	{ SSLFLAG_FAILIFNOCERT , "certificado-cliente" } ,
	{ SSLFLAG_DONOTACCEPTSELFSIGNED , "solo-oficiales" } ,
	{ SSLFLAG_VERIFYCERT , "verificar-certificado" } ,
	{ 0x0 , 0x0 }
};
#endif
static Opts Opts_Log[] = {
	{ LOG_ERROR , "errores" } ,
	{ LOG_SERVER , "servidores" } ,
	{ LOG_CONN , "conexiones" } ,
#ifdef USA_SSL
	{ LOG_MSN , "msn" } ,
#endif
	{ 0x0 , 0x0 }
};

/*!
 * @desc: Libera una estructura de configuración.
 * @params: $seccion [in] Sección a liberar.
 * @ver: ParseaConfiguracion
 * @cat: Configuracion
 !*/
 
void LiberaMemoriaConfiguracion(Conf *seccion)
{
	int i;
	if (!seccion)
		return;
	ircfree(seccion->item);
	ircfree(seccion->data);
	for (i = 0; i < seccion->secciones; i++)
		LiberaMemoriaConfiguracion(seccion->seccion[i]);
	if (!seccion->root && seccion->archivo) /* top */
		ircfree(seccion->archivo);
	if (seccion->root)
		ircfree(seccion);
}
void LiberaMemoriaServer()
{
	if (!conf_server)
		return;
	ircfree(conf_server->addr);
	ircfree(conf_server->host);
	ircfree(conf_server->info);
	ircfree(conf_server->password.local);
	ircfree(conf_server->password.remoto);
	ircfree(conf_server->bind_ip);
	bzero(conf_server, sizeof(struct Conf_server));
}
void LiberaMemoriaDb()
{
	if (!conf_db)
		return;
	LiberaSQL();
	ircfree(conf_db->host);
	ircfree(conf_db->login);
	ircfree(conf_db->pass);
	ircfree(conf_db->bd);
	ircfree(conf_db->prefijo);
	bzero(conf_db, sizeof(struct Conf_db));
}
void LiberaMemoriaSmtp()
{
	if (!conf_smtp)
		return;
	ircfree(conf_smtp->host);
	ircfree(conf_smtp->login);
	ircfree(conf_smtp->pass);
	bzero(conf_smtp, sizeof(struct Conf_smtp));
}
void LiberaMemoriaSet()
{
	char *tmp = NULL;
	if (!conf_set)
		return;
	ircfree(conf_set->root);
	ircfree(conf_set->admin);
	if (conf_set->red)
		tmp = strdup(conf_set->red);
	ircfree(conf_set->red);
	ircfree(conf_set->debug);
	bzero(conf_set->clave_cifrado, sizeof(conf_set->clave_cifrado));
	bzero(conf_set, sizeof(struct Conf_set));
	if (tmp)
		conf_set->red = strdup(tmp);
	DescargaNiveles();
	if (tmp)
		Free(tmp);
}
void LiberaMemoriaLog()
{
	if (!conf_log)
		return;
	ircfree(conf_log->archivo);
	bzero(conf_log, sizeof(struct Conf_log));
}
#ifdef USA_SSL
void LiberaMemoriaSSL()
{
	if (!conf_ssl)
		return;
	ircfree(conf_ssl->egd_path);
	ircfree(conf_ssl->x_server_cert_pem);
	ircfree(conf_ssl->x_server_key_pem);
	ircfree(conf_ssl->trusted_ca_file);
	ircfree(conf_ssl->cifrados);
	bzero(conf_ssl, sizeof(struct Conf_ssl));
}
#endif
void LiberaMemoriaHttpd()
{
	if (!conf_httpd)
		return;
	DetieneHTTPD();
	ircfree(conf_httpd->url);
	ircfree(conf_httpd->php);
	bzero(conf_httpd, sizeof(struct Conf_httpd));
}
#ifdef USA_SSL
void LiberaMemoriaMSN()
{
	if (!conf_msn)
		return;
	DescargaMSN();
	ircfree(conf_msn->cuenta);
	ircfree(conf_msn->pass);
	ircfree(conf_msn->master);
	bzero(conf_msn, sizeof(struct Conf_msn));
}
#endif
void DescargaConfiguracion()
{
	LiberaMemoriaServer();
	LiberaMemoriaDb();
	LiberaMemoriaSmtp();
	LiberaMemoriaSet();
	LiberaMemoriaLog();
#ifdef USA_SSL
	LiberaMemoriaSSL();
#endif
	LiberaMemoriaHttpd();
#ifdef USA_SSL
	LiberaMemoriaMSN();
#endif
}
#define pce(x) Error("[%s:%i] " x, archivo, linea)
/* parseconf
 * 
 * Inicializa una config y la mete en rama.
 * 0 Error
 * 1 Ok
 */
/*!
 * @desc: Parsea un archivo de configuración
 * @params: $archivo [in] Ruta del archivo a parsear.
 	    $rama [out] Rama o raíz.
 	    $avisa [in] Si vale 1, se mostrarán mensajes de error; si no, 0.
 * @ret: Devuelve 0 si no ocurre ningún error.
 * @ver: LiberaMemoriaConfiguracion
 * @cat: Configuracion
 !*/
 
int ParseaConfiguracion(char *archivo, Conf *rama, char avisa)
{
	int fp, linea, error = 0;
	char *pitem, *pdata, *par, *ini, *f;
	struct stat inode;
	Conf *actual, *root;
	pitem = pdata = NULL;
	root = rama;
	root->item = strdup("MainRoot");
	root->secciones = 0;
	root->data = NULL;
	root->archivo = strdup(archivo);
	root->root = NULL;
	if ((fp = open(archivo, O_RDONLY|O_BINARY)) == -1)
	{
		if (avisa)
			Alerta(FERR, "No existe el archivo %s", archivo);
		return -1;
	}
	if (fstat(fp, &inode) == -1)
	{
		if (avisa)
			Alerta(FERR, "No se puede hacer fstat");
		close(fp);
		return -2;
	}
	if (!inode.st_size)
	{
		if (avisa)
			Alerta(FERR, "El archivo esta vacio");
		close(fp);
		return -3;
	}
	f = par = (char *)Malloc(inode.st_size + 1);
	par[inode.st_size] = '\0';
	if (read(fp, par, inode.st_size) != inode.st_size)
	{
		if (avisa)
			Alerta(FERR, "Error fatal de lectura");
		return -4;
	}
	close(fp);
	ini = par;
	linea = 1;
	while (!BadPtr(par))
	{
		switch(*par)
		{
			case '"':
				if (!pitem)
				{
					pce("Datos sin ítem.");
					error++;
					break;
				}
				ini = ++par;
				if (*par == '"')
					break;
				while (*par != '"')
				{
					if (*par == '\\')
						par++;
					else if (BadPtr(par))
					{
						pce("Final de archivo inesperado.");
						error++;
						break;
					}
					else if (*par == '\n')
					{
						pce("Final de línea inesperado.");
						linea++;
						error++;
						break;
					}
					par++;
				}
				*par = '\0';
				pdata = ini;
				break;
			case ';':
				if (!pitem && !pdata)
					break;
				else if (!pitem)
				{
					pce("Campo sin ítem.");
					error++;
					break;
				}
				actual = BMalloc(Conf);
				actual->item = strdup(pitem);
				if (pdata)
					actual->data = strdup(pdata);
				actual->linea = linea;
				actual->root = root;
				actual->archivo = root->archivo;
				root->seccion[root->secciones++] = actual;
				pitem = pdata = NULL;
				break;
			case '}':
				if (!root->root)
				{
					pce("Hay llaves cerradas '}' de más.");
					error++;
					break;
				}
				root->root->seccion[root->root->secciones++] = root;
				root = root->root;
				break;
			case '{':
				if (!pitem)
				{
					int llaves = 1;
					pce("Bloque sin ítem. Omitiendo bloque.");
					while (llaves)
					{
						if (*par == '{')
							llaves++;
						else if (*par == '}')
							llaves--;
						else if (BadPtr(par))
						{
							pce("Final de archivo inesperado.");
							error++;
							break;
						}
						par++;
					}
				}
				actual = BMalloc(Conf);
				actual->item = strdup(pitem);
				if (pdata)
					actual->data = strdup(pdata);
				actual->linea = linea;
				actual->root = root;
				actual->archivo = root->archivo;
				root = actual;
				pitem = pdata = NULL;
				break;
			case '\n':
				linea++;
			case '\r':
			case '\t':
			case ' ':
				break;
			case '/':
				par++;
				if (*par == '*')
				{
					for (par++; !BadPtr(par); par++)
					{
						if (*par == '\n')
							linea++;
						if (*par == '*' && *(par + 1) == '/')
							break;
					}
					par++;
				}
				else if (*par == '/')
				{
					for (par++; *par != '\n'; par++);
					linea++;
				}
				break;
			case '#':
				while (*par != '\n')
					par++;
				linea++;
				break;
			default:
				ini = par;
				if (pdata)
				{
					pce("Datos extra, no se ha cerrado ';'.");
					while (*par++ != ';');
					pdata = pitem = NULL;
					error++;
					break;
				}
				while (*par != ' ' && *par != ';')
				{
					if (*par == '{' || *par == '\n' || *par == '\r')
					{
						Error("[%s:%i] Ítem inválido.", archivo, linea);
						error++;
						break;
					}
					par++;
				}
				if (*par == ';')
				{
					*par = '\0';
					if (pitem)
						pdata = ini;
					else
						pitem = ini;
					actual = BMalloc(Conf);
					actual->item = strdup(pitem);
					if (pdata)
						actual->data = strdup(pdata);
					actual->linea = linea;
					actual->root = root;
					actual->archivo = root->archivo;
					root->seccion[root->secciones++] = actual;
					pitem = pdata = NULL;
				}
				else
				{
					*par = '\0';
					if (pitem)
						pdata = ini;
					else
						pitem = ini;
				}
		}
		par++;
	}
	Free(f);
	if (avisa && (pdata || pitem))
	{
		pce("Final de archivo inesperado.");
		return -5;
	}
	return error;
}
void Printea(Conf *conf, int escapes)
{
	int i;
	char tabs[32];
	Conf *c;
	tabs[0] = '\0';
	for (i = 0; i < escapes; i++)
		strlcat(tabs, "\t", sizeof(tabs));
//	if (bloq->id)
//		tabs[escapes] = bloq->id;
	for (i = 0; i < conf->secciones; i++)
	{
		c = conf->seccion[i];
		if (c->data)
			Debug("%s%s \"%s\"%s", tabs, c->item, c->data, c->secciones ? " {" : ";");
		else
			Debug("%s%s%s", tabs, c->item, c->secciones ? " {" : ";");
		if (c->secciones)
		{
			Printea(c, ++escapes);
			escapes--;
			Debug("%s};", tabs);
		}
	}
}	
/* 
 * distribuye_conf
 * Distribuye por bloques la configuración parseada
 */
void DistribuyeConfiguracion(Conf *config)
{
	int i, errores = 0, cont = 0;
	cComConf *com;
	Conf *prot = NULL;
	for (i = 0; i < config->secciones; i++)
	{
		com = &cComs[0];
		while (com->nombre != 0x0)
		{
			
			if (!strcasecmp(config->seccion[i]->item, "protocolo"))
			{
				prot = config->seccion[i];
				cont = 1;
				break;
			}
			else if (!strcasecmp(config->seccion[i]->item, com->nombre))
				break;
			com++;
		}
		if (cont)
		{
			cont = 0;
			continue;
		}
		if (com->testfunc != 0x0)
		{
			if (!com->testfunc(config->seccion[i], &errores) && com->func)
				com->func(config->seccion[i]);
		}
		else
			Error("[%s:%i] Sección extra: '%s'.", config->seccion[i]->archivo, config->seccion[i]->linea, config->seccion[i]->item);
	}
	com = &cComs[0];
	while (com->nombre != 0x0)
	{
#ifdef _WIN32		
		if ((com->opt == OBL && VerInfo.dwMajorVersion >= com->min_os) || (com->opt == OPC && VerInfo.dwMajorVersion < com->min_os))
#else
		if (com->opt == OBL)
#endif			
		{
			if (!BuscaEntrada(config, com->nombre))
			{
				Alerta(FERR, "No se encuentra la seccion %s", com->nombre);
				errores++;
			}		
		}
		com++;
	}
	if (prot)
		TestProtocolo(prot, &errores);
	if (!sql)
		errores++;
	if (errores)
	{
		Alerta(FERR, "Hay %i error%s en la configuracion. No se puede cargar", errores, errores > 1 ? "es" : "");
		CierraColossus(-1);
	}
	LiberaMemoriaConfiguracion(config);
}
Conf *BuscaEntrada(Conf *config, char *meta)
{
	int i;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcasecmp(config->seccion[i]->item, meta))
			return config->seccion[i];
	}
	return NULL;
}
int TestServer(Conf *config, int *errores)
{
	short error_parcial = 0;
	int puerto;
	Conf *eval, *aux;
	if (!(eval = BuscaEntrada(config, "addr")))
	{
		Error("[%s:%s] No se encuentra la directriz addr.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz addr esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (!EsIp(eval->data) && !gethostbyname(eval->data))
		{
			Error("[%s:%s::%s::%i] No se puede resolver el host %s.", config->archivo, config->item, eval->item, eval->linea, eval->data);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "puerto")))
	{
		Error("[%s:%s] No se encuentra la directriz puerto.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz puerto esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 65535)
			{
				Error("[%s:%s::%s::%i] El puerto debe estar entre 0 y 65535.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
			}
		}
	}
	if ((eval = BuscaEntrada(config, "escucha")))
	{
		if ((aux = BuscaEntrada(eval, "puerto")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz puerto esta vacia.", config->archivo, config->item, eval->item, aux->item, eval->linea);
				error_parcial++;
			}
			else
			{
				puerto = atoi(aux->data);
				if (puerto < 1 || puerto > 65535)
				{
					Error("[%s:%s::%s::%s::%i] El puerto debe estar entre 1 y 65535.", config->archivo, config->item, eval->item, aux->item, eval->linea);
					error_parcial++;
				}
			}
		}
		if ((aux = BuscaEntrada(eval, "enlace")))
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz enlace esta vacia.", config->archivo, config->item, eval->item, aux->item, eval->linea);
				error_parcial++;
			}
			else
			{
				if (!EsIp(aux->data))
				{
					Error("[%s:%s::%s::%s::%i] La dirección de enlace debe ser una ip válida.", config->archivo, config->item, eval->item, aux->item, eval->linea);
					error_parcial++;
				}
			}
		}
	}
	if (!(eval = BuscaEntrada(config, "password")))
	{
		Error("[%s:%s] No se encuentra la directriz password.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!(aux = BuscaEntrada(eval, "local")))
		{
			Error("[%s:%s::%s] No se encuentra la directriz local.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz local esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if (!(aux = BuscaEntrada(eval, "remoto")))
		{
			Error("[%s:%s::%s] No se encuentra la directriz remoto.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!aux->data)
			{
				Error("[%s:%s::%s::%s::%i] La directriz remoto esta vacia.", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
	}
	if (!(eval = BuscaEntrada(config, "host")))
	{
		Error("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz host esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "info")))
	{
		Error("[%s:%s] No se encuentra la directriz info.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz puerto esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "numeric")))
	{
		Error("[%s:%s] No se encuentra la directriz numeric.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz numeric esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 255)
			{
				Error("[%s:%s::%s::%i] La directriz numeric debe estar entre 0 y 255.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
			}
		}
	}
#ifdef USA_ZLIB
	if ((eval = BuscaEntrada(config, "compresion")))
	{
		puerto = atoi(eval->data);
		if (puerto < 1 || puerto > 9)
		{
			Error("[%s:%s::%s::%i] La directriz compresion debe estar entre 1 y 9.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
		}
	}
#endif
	*errores += error_parcial;
	return error_parcial;
}
void ConfServer(Conf *config)
{
	int i, p;
	if (!conf_server)
		conf_server = BMalloc(struct Conf_server);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "addr"))
			ircstrdup(conf_server->addr, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "puerto"))
			conf_server->puerto = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "password"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "local"))
					ircstrdup(conf_server->password.local, config->seccion[i]->seccion[p]->data);
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "remoto"))
					ircstrdup(conf_server->password.remoto, config->seccion[i]->seccion[p]->data);
			}
		}
		else if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(conf_server->host, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "info"))
			ircstrdup(conf_server->info, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "numeric"))
			conf_server->numeric = atoi(config->seccion[i]->data);
#ifdef USA_ZLIB
		else if (!strcmp(config->seccion[i]->item, "compresion"))
			conf_server->compresion = atoi(config->seccion[i]->data);
#endif
		else if (!strcmp(config->seccion[i]->item, "escucha"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "puerto"))
					conf_server->escucha = atoi(config->seccion[i]->seccion[p]->data);
				else if (!strcmp(config->seccion[i]->seccion[p]->item, "enlace"))
					ircstrdup(conf_server->bind_ip, config->seccion[i]->seccion[p]->data);
			}
		}
#ifdef USA_SSL
		else if (!strcmp(config->seccion[i]->item, "ssl"))
			conf_server->usa_ssl = 1;
#endif
	}
	if (!conf_server->escucha)
		conf_server->escucha = conf_server->puerto;
}
int TestDb(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (BadPtr(config->data))
	{
		Error("[%s:%s::%i] Falta nombre de archivo.", config->archivo, config->item, config->linea);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "host")))
	{
		Error("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz host esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (!EsIp(eval->data) && !gethostbyname(eval->data))
		{
			Error("[%s:%s::%s::%i] No se puede resolver el host %s.", config->archivo, config->item, eval->item, eval->linea, eval->data);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "login")))
	{
		Error("[%s:%s] No se encuentra la directriz login.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz login esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "pass")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz pass esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "nombre")))
	{
		Error("[%s:%s] No se encuentra la directriz nombre.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz nombre esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "prefijo")))
	{
		Error("[%s:%s] No se encuentra la directriz prefijo.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz prefijo esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "puerto")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz puerto esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			int puerto;
			puerto = atoi(eval->data);
			if (puerto < 1 || puerto > 65535)
			{
				Error("[%s:%s::%s::%i] El puerto debe estar entre 1 y 65535.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfDb(Conf *config)
{
	int i;
	if (!conf_db)
		conf_db = BMalloc(struct Conf_db);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(conf_db->host, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "login"))
			ircstrdup(conf_db->login, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "pass"))
			ircstrdup(conf_db->pass, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "nombre"))
			ircstrdup(conf_db->bd, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "prefijo"))
			ircstrdup(PREFIJO, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item ,"puerto"))
			conf_db->puerto = atoi(config->seccion[i]->data);
	}
	if (CargaSQL(config->data))
		Error("[%s:%s] Ha sido imposible cargar el motor SQL %s.", config->archivo, config->item, config->data);
}
int TestSmtp(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "host")))
	{
		Error("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz host esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (!EsIp(eval->data) && !gethostbyname(eval->data))
		{
			Error("[%s:%s::%s::%i] No se puede resolver el host %s.", config->archivo, config->item, eval->item, eval->linea, eval->data);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "login")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz login esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "pass")))
	{
		if (!(eval = BuscaEntrada(config, "login")))
		{
			Error("[%s:%s::%s::%i] Para usar la directriz pass se requiere la directriz login.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz pass esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfSmtp(Conf *config)
{
	int i;
	if (!conf_smtp)
		conf_smtp = BMalloc(struct Conf_smtp);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup((conf_smtp->host), config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "login"))
			ircstrdup((conf_smtp->login), config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "pass"))
			ircstrdup((conf_smtp->pass), config->seccion[i]->data);
	}
}
int TestSet(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	if (!(eval = BuscaEntrada(config, "respuesta")))
	{
		Error("[%s:%s] No se encuentra la directriz respuesta.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz respuesta esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (strcasecmp(eval->data, "PRIVMSG") && strcasecmp(eval->data, "NOTICE"))
		{
			Error("[%s:%s::%s::%i] La respuesta solo puede tomar los valores PRIVMSG o NOTICE.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "root")))
	{
		Error("[%s:%s] No se encuentra la directriz root.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz root esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "admin")))
	{
		Error("[%s:%s] No se encuentra la directriz admin.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "reconectar")))
	{
		Error("[%s:%s] No se encuentra la directriz reconectar.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int rec;
		if (!(aux = BuscaEntrada(eval, "intentos")))
		{
			Error("[%s:%s::%s] No se encuentra la directriz intentos.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			
			if (!(rec = atoi(aux->data)) || rec < 1 || rec >999)
			{
				Error("[%s:%s::%s::%s::%i] Intentos de reconexion invalido (1-999).", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
		if (!(aux = BuscaEntrada(eval, "intervalo")))
		{
			Error("[%s:%s::%s] No se encuentra la directriz intervalo.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!(rec = atoi(aux->data)) || rec < 1 || rec > 300)
			{
				Error("[%s:%s::%s::%s::%i] Intervalo de reconexion invalido (1-300).", config->archivo, config->item, eval->item, aux->item, aux->linea);
				error_parcial++;
			}
		}
	}
	if (!(eval = BuscaEntrada(config, "clave_cifrado")))
	{
		Error("[%s:%s] No se encuentra la directriz clave_cifrado", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (strlen(eval->data) > 32)
		{
			Error("[%s:%s::%s::%i] La clave de cifrado es demasiado larga. Sólo puede tener 32 caracteres.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfSet(Conf *config)
{
	int i, p;
	if (!conf_set)
		conf_set = BMalloc(struct Conf_set);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "autobop"))
			conf_set->opts |= AUTOBOP;
		else if (!strcmp(config->seccion[i]->item, "rekill"))
			conf_set->opts |= REKILL;
		else if (!strcmp(config->seccion[i]->item, "rejoin"))
			conf_set->opts |= REJOIN;
		else if (!strcmp(config->seccion[i]->item, "respuesta"))
		{
			if (!strcasecmp(config->seccion[i]->data, "PRIVMSG"))
				conf_set->opts |= RESP_PRIVMSG;
			else if (!strcasecmp(config->seccion[i]->data, "NOTICE"))
				conf_set->opts |= RESP_NOTICE;
		}
		else if (!strcmp(config->seccion[i]->item, "root"))
			ircstrdup(conf_set->root, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "admin"))
			ircstrdup(conf_set->admin, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "reconectar"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "intentos"))
					conf_set->reconectar.intentos = atoi(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "intervalo"))
					conf_set->reconectar.intervalo = atoi(config->seccion[i]->seccion[p]->data);
			}
		}
		else if (!strcmp(config->seccion[i]->item, "debug"))
			ircstrdup(conf_set->debug, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "clave_cifrado"))
			strlcpy(conf_set->clave_cifrado, config->seccion[i]->data, sizeof(conf_set->clave_cifrado));
		else if (!strcmp(config->seccion[i]->item, "niveles"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
				InsertaNivel(config->seccion[i]->seccion[p]->item, config->seccion[i]->seccion[p]->data);
		}
		else if (!strcmp(config->seccion[i]->item, "auto_actualiza"))
			conf_set->actualiza = 1;
	}
}
int TestModulos(Conf *config, int *errores)
{
	short int error_parcial = 0;
	Conf *eval;
	if (BadPtr(config->data))
	{
		Error("[%s:%s::%i] Falta nombre de archivo.", config->archivo, config->item, config->linea);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "nick")))
	{
		Error("[%s:%s] No se encuentra la directriz nick.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "ident")))
	{
		Error("[%s:%s] No se encuentra la directriz ident.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "host")))
	{
		Error("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = BuscaEntrada(config, "realname")))
	{
		Error("[%s:%s] No se encuentra la directriz realname.", config->archivo, config->item);
		error_parcial++;
	}
	return error_parcial;
}
void ConfModulos(Conf *config)
{
	int i;
	Modulo *mod;
	if (!(mod = CreaModulo(config->data)))
		Error("[%s:%s] Ha sido imposible cargar el módulo %s.", config->archivo, config->item, config->data);
	else
	{
		int val;
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "nick"))
				ircstrdup(mod->nick, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "ident"))
				ircstrdup(mod->ident, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "host"))
				ircstrdup(mod->host, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "realname"))
				ircstrdup(mod->realname, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "modos"))
				ircstrdup(mod->modos, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "residente"))
				ircstrdup(mod->residente, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "config"))
			{
				char *path, *k;
				path = strdup(config->data);
				if (!(k = strrchr(path, '/')))
					k = strrchr(path, '\\');
				if (k)
					*k = '\0';
				ircfree(mod->config);
				mod->config = (char *)Malloc(sizeof(char) * (strlen(config->seccion[i]->data) + (path ? strlen(path) : 0) + 2));
				ircsprintf(mod->config, "%s/%s", path, config->seccion[i]->data);
				Free(path);
			}
			else if (!strcmp(config->seccion[i]->item, "serial"))
				ircstrdup(mod->serial, config->seccion[i]->data);
		}
		ircfree(mod->mascara);
		mod->mascara = (char *)Malloc(sizeof(char) * (strlen(mod->nick) + 1 + strlen(mod->ident) + 1 + strlen(mod->host) + 1));
		ircsprintf(mod->mascara, "%s!%s@%s", mod->nick, mod->ident, mod->host);
		val = (*mod->carga)(mod);
		if (val)
		{
			mod->descarga = NULL; /* no ha llegado a cargarse, por tanto no hace falta descargarlo remotamente */
			Alerta(FADV, "No se ha podido cargar el modulo %s", mod->archivo);
			DescargaModulo(mod);
		}
	}
}
int TestLog(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	int i;
	if ((eval = BuscaEntrada(config, "tamaño")) || (eval = BuscaEntrada(config, "tamano")))
	{
		if (atoi(eval->data) < 0)
		{
			Error("[%s:%s::%s::%i] El tamaño máximo del archivo log debe ser mayor que 0 bytes.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (BadPtr(config->data))
	{
		Error("[%s:%s::%i] Falta nombre de archivo.", config->archivo, config->item, config->linea);
		error_parcial++;
	}
	else
	{
		if (!EsArchivo(config->data))
			fclose(fopen(config->data, "w"));
	}
	if (!(eval = BuscaEntrada(config, "opciones")))
	{
		Error("[%s:%s] No se encuentra la directriz opciones.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		for (i = 0; i < eval->secciones; i++)
		{
			aux = eval->seccion[i];
			if (!BuscaOpt(aux->item, Opts_Log))
			{
				Error("[%s:%s::%s::%i] La opción %s es desconocida.", config->archivo, config->item, eval->item, aux->linea, aux->item);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfLog(Conf *config)
{
	int i, p;
	if (!conf_log)
		conf_log = BMalloc(struct Conf_log);
	ircstrdup(conf_log->archivo, config->data);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "tamaño") || !strcmp(config->seccion[i]->item, "tamano"))
			conf_log->size = atoi(config->seccion[i]->data) * 1024;
		else if (!strcmp(config->seccion[i]->item, "opciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
				conf_log->opts |= BuscaOpt(config->seccion[i]->seccion[p]->item, Opts_Log);
		}
	}
}
#ifdef USA_SSL
int TestSSL(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	int i;
	if ((eval = BuscaEntrada(config, "certificado")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz certificado esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "clave")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz clave esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "certificados-seguros")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz certificado esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "opciones")))
	{
		for (i = 0; i < eval->secciones; i++)
		{
			aux = eval->seccion[i];
			if (!BuscaOpt(aux->item, Opts_SSL))
			{
				Error("[%s:%s::%s::%i] La opción %s es desconocida.", config->archivo, config->item, eval->item, aux->linea, aux->item);
				error_parcial++;
			}
		}
	}
	if ((eval = BuscaEntrada(config, "cifrados")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz cifrados esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfSSL(Conf *config)
{
	int i, p;
	if (!conf_ssl)
		conf_ssl = BMalloc(struct Conf_ssl);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "egd"))
		{
			conf_ssl->usa_egd = 1;
			if (config->seccion[i]->data)
				ircstrdup(conf_ssl->egd_path, config->seccion[i]->data);
		}
		else if (!strcmp(config->seccion[i]->item, "certificado"))	
			ircstrdup(conf_ssl->x_server_cert_pem, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "clave"))
			ircstrdup(conf_ssl->x_server_key_pem, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "certificados-seguros"))
			ircstrdup(conf_ssl->trusted_ca_file, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "opciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
				conf_ssl->opts |= BuscaOpt(config->seccion[i]->seccion[p]->item, Opts_SSL);
		}
		else if (!strcmp(config->seccion[i]->item, "cifrados"))
			ircstrdup(conf_ssl->cifrados, config->seccion[i]->data);
	}
}
#endif
int TestProtocolo(Conf *config, int *errores)
{
	short error_parcial = 0;
	if (BadPtr(config->data))
	{
		Error("[%s:%s::%i] Falta nombre de archivo.", config->archivo, config->item, config->linea);
		error_parcial++;
	}
	if (CargaProtocolo(config))
	{
		Error("[%s:%s::%i] Ha sido imposible cargar el protocolo %s.", config->archivo, config->item, config->linea, config->data);
		error_parcial++;
	}
	*errores += error_parcial;
	return error_parcial;
}
int TestHttpd(Conf *config, int *errores)
{
	short error_parcial = 0;
	int puerto;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "url")))
	{
		Error("[%s:%s] No se encuentra la directriz url.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz url esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if ((eval = BuscaEntrada(config, "puerto")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz puerto esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 65535)
			{
				Error("[%s:%s::%s::%i] El puerto debe estar entre 0 y 65535.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
			}
		}
	}
	if (!(eval = BuscaEntrada(config, "ruta_php")))
	{
		Error("[%s:%s] No se encuentra la directriz ruta_php.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz ruta_php esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		if (!EsArchivo(eval->data))
		{
			Error("[%s:%s::%s::%i] No se encuentra el archivo %s.", config->archivo, config->item, eval->item, eval->linea, eval->data);
			error_parcial++;
		}			
	}		
	*errores += error_parcial;
	return error_parcial;
}
void ConfHttpd(Conf *config)
{
	int i;
	if (!conf_httpd)
		conf_httpd = BMalloc(struct Conf_httpd);
	conf_httpd->puerto = 80;
	conf_httpd->max_age = -1;
	ircfree(conf_httpd->url);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "url"))
			ircstrdup(conf_httpd->url, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "puerto"))
			conf_httpd->puerto = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "edad_maxima"))
			conf_httpd->max_age = atoi(config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "ruta_php"))
			ircstrdup(conf_httpd->php, config->seccion[i]->data);
	}
	IniciaHTTPD();
}
#ifdef USA_SSL
int TestMSN(Conf *config, int *errores)
{
	short error_parcial = 0;
	int puerto;
	Conf *eval;
	if (!(eval = BuscaEntrada(config, "cuenta")))
	{
		Error("[%s:%s] No se encuentra la directriz cuenta.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz cuenta esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "pass")))
	{
		Error("[%s:%s] No se encuentra la directriz pass.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz pass esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "master")))
	{
		Error("[%s:%s] No se encuentra la directriz master.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz master esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void ConfMSN(Conf *config)
{
	int i;
	if (!conf_msn)
		conf_msn = BMalloc(struct Conf_msn);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "cuenta"))
			ircstrdup(conf_msn->cuenta, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "pass"))
			ircstrdup(conf_msn->pass, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "master"))
			ircstrdup(conf_msn->master, config->seccion[i]->data);
	}
	CargaMSN();
}
#endif
#ifndef _WIN32
void Error(char *formato, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, formato);
	ircvsprintf(buf, formato, vl);
	va_end(vl);
	strlcat(buf, "\r\n", sizeof(buf));
	fprintf(stderr, buf);	
}
int Info(char *formato, ...)
{
	char buf[BUFSIZE], txt[BUFSIZE];
	struct tm *timeptr;
	time_t ts;
	va_list vl;
	ts = time(0);
	timeptr = localtime(&ts);
	va_start(vl, formato);
	ircsprintf(txt, "(%.2i:%.2i:%.2i) %s\r\n", timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, formato);
	ircvsprintf(buf, txt, vl);
	va_end(vl);
	strlcat(buf, "\r\n", sizeof(buf));
	fprintf(stderr, buf);
	return -1;
}
#endif
