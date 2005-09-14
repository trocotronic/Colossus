/*
 * $Id: parseconf.c,v 1.17 2005-09-14 14:45:05 Trocotronic Exp $ 
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
#include "protocolos.h"
#include "md5.h"

struct Conf_server *conf_server = NULL;
struct Conf_db *conf_db = NULL;
struct Conf_smtp *conf_smtp = NULL;
struct Conf_set *conf_set = NULL;
struct Conf_log *conf_log = NULL;
#ifdef USA_SSL
struct Conf_ssl *conf_ssl = NULL;
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

static void ConfServer	(Conf *);
static void ConfDb 		(Conf *);
static void ConfSmtp		(Conf *);
static void ConfSet		(Conf *);
static void ConfLog		(Conf *);
#ifdef USA_SSL
static void ConfSSL		(Conf *);
#endif
static void ConfModulos	(Conf *);

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
	{ "smtp" , TestSmtp, ConfSmtp , OBL , 1 } ,
#endif
	{ "set" , TestSet, ConfSet , OBL , 1 } ,
	{ "modulo" , TestModulos, ConfModulos , OPC , 1 } ,
	{ "log" , TestLog , ConfLog , OPC , 1 } ,
#ifdef USA_SSL
	{ "ssl" , TestSSL , ConfSSL , OPC , 1 } ,
#endif
	{ "protocolo" , TestProtocolo , NULL , OBL , 1 } ,
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
	conf_set->red = strdup(tmp);
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
	int fp, linea;
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
#ifdef _WIN32	
	if ((fp = open(archivo, O_RDONLY|O_BINARY)) == -1)
#else
	if ((fp = open(archivo, O_RDONLY)) == -1)
#endif	
	{
		if (avisa)
			Alerta(FERR, "No existe el archivo %s", archivo);
		return -1;
	}
	if (fstat(fp, &inode) == -1)
	{
		if (avisa)
			Alerta(FERR, "No se puede hacer fstat");
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
						break;
					}
					else if (*par == '\n')
					{
						pce("Final de línea inesperado.");
						linea++;
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
					break;
				}
				BMalloc(actual, Conf);
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
							break;
						}
						par++;
					}
				}
				BMalloc(actual, Conf);
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
					break;
				}
				while (*par != ' ' && *par != ';')
				{
					if (*par == '{' || *par == '\n' || *par == '\r')
					{
						Error("[%s:%i] Ítem inválido.", archivo, linea);
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
					BMalloc(actual, Conf);
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
	return 0;
}
/* 
 * distribuye_conf
 * Distribuye por bloques la configuración parseada
 */
void DistribuyeConfiguracion(Conf *config)
{
	int i, errores = 0;
	cComConf *com;
	for (i = 0; i < config->secciones; i++)
	{
		com = &cComs[0];
		while (com->nombre != 0x0)
		{
			if (!strcasecmp(config->seccion[i]->item, com->nombre))
				break;
			com++;
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
	if ((eval = BuscaEntrada(config, "puerto_escucha")))
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz puerto_escucha esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 65535)
			{
				Error("[%s:%s::%s::%i] El puerto_escucha debe estar entre 0 y 65535.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
			}
			aux = BuscaEntrada(config, "puerto");
			if (puerto == atoi(aux->data))
			{
				Error("[%s:%s::%s::%i] El puerto_escucha no puede ser el mismo que puerto.", config->archivo, config->item, eval->item, eval->linea);
				error_parcial++;
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
		BMalloc(conf_server, struct Conf_server);
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
		else if (!strcmp(config->seccion[i]->item, "puerto_escucha"))
			conf_server->escucha = atoi(config->seccion[i]->data);
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
	if (!(eval = BuscaEntrada(config, "tipo")))
	{
		Error("[%s:%s] No se encuentra la directriz tipo.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			Error("[%s:%s::%s::%i] La directriz tipo esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
		else
		{
			if (strcasecmp(eval->data, "MySQL") && strcasecmp(eval->data, "PostGreSQL"))
			{
				Error("[%s:%s::%s::%i] Tipo de base de datos incorrecto.", config->archivo, config->item, eval->item, eval->linea);
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
		BMalloc(conf_db, struct Conf_db);
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
		BMalloc(conf_smtp, struct Conf_smtp);
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
		BMalloc(conf_set, struct Conf_set);
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
			strncpy(conf_set->clave_cifrado, config->seccion[i]->data, sizeof(conf_set->clave_cifrado));
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
		}
		ircfree(mod->mascara);
		mod->mascara = (char *)Malloc(sizeof(char) * (strlen(mod->nick) + 1 + strlen(mod->ident) + 1 + strlen(mod->host) + 1));
		ircsprintf(mod->mascara, "%s!%s@%s", mod->nick, mod->ident, mod->host);
	}
}
int TestLog(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	int i;
	Opts *ofl = NULL;
	if ((eval = BuscaEntrada(config, "tamaño")))
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
			for (ofl = Opts_Log; ofl && ofl->item; ofl++)
			{
				if (!strcmp(aux->item, ofl->item))
					break;
			}
			if (!ofl || !ofl->item)
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
	Opts *ofl = NULL;
	if (!conf_log)
		BMalloc(conf_log, struct Conf_log);
	ircstrdup(conf_log->archivo, config->data);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "tamaño"))
			conf_log->size = atoi(config->seccion[i]->data) * 1024;
		else if (!strcmp(config->seccion[i]->item, "opciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				for (ofl = Opts_Log; ofl && ofl->item; ofl++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, ofl->item))
					{
						conf_log->opts |= ofl->opt;
						break;
					}
				}
			}
		}
	}
}
#ifdef USA_SSL
int TestSSL(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	int i;
	Opts *ofl = NULL;
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
			for (ofl = Opts_SSL; ofl && ofl->item; ofl++)
			{
				if (!strcmp(aux->item, ofl->item))
					break;
			}
			if (!ofl || !ofl->item)
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
	Opts *ofl = NULL;
	if (!conf_ssl)
		BMalloc(conf_ssl, struct Conf_ssl);
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
			{
				for (ofl = Opts_Log; ofl && ofl->item; ofl++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, ofl->item))
					{
						conf_ssl->opts |= ofl->opt;
						break;
					}
				}
			}
		}
		else if (!strcmp(config->seccion[i]->item, "cifrados"))
			ircstrdup(conf_ssl->cifrados, config->seccion[i]->data);
	}
	SSLInit();
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
#ifndef _WIN32
void Error(char *formato, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, formato);
	ircvsprintf(buf, formato, vl);
	va_end(vl);
	strcat(buf, "\r\n");
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
	strcat(buf, "\r\n");
	fprintf(stderr, buf);
	return -1;
}
#endif
