/*
 * $Id: parseconf.c,v 1.4 2004-09-17 19:09:46 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include "struct.h"
#include "modulos.h"

struct Conf_server *conf_server = NULL;
struct Conf_db *conf_db = NULL;
struct Conf_smtp *conf_smtp = NULL;
struct Conf_set *conf_set = NULL;
struct Conf_log *conf_log = NULL;

static int _test_server		(Conf *, int *);
static int _test_db 			(Conf *, int *);
static int _test_smtp		(Conf *, int *);
static int _test_set			(Conf *, int *);
static int _test_modulos		(Conf *, int *);
static int _test_log			(Conf *, int *);

static void _conf_server		(Conf *);
static void _conf_db 		(Conf *);
static void _conf_smtp		(Conf *);
static void _conf_set		(Conf *);
static void _conf_modulos		(Conf *);
static void _conf_log		(Conf *);

/* el ultimo parámetro es la obliguetoriedad:
   Si es OBL, significa que es obligatorio a partir de la version dada (incluida)
   Si es OPC, significa que es opcional a partir de la versión dada (incluida)
   */
static cComConf cComs[] = {
	{ "server" , _test_server, _conf_server , OBL , 1 },
	{ "db" , _test_db, _conf_db , OBL ,  1 },
#ifdef _WIN32
	{ "smtp" , _test_smtp, _conf_smtp , OPC , 5 }, /* es opcional a partir de la version 5 */
#else
	{ "smtp" , _test_smtp, _conf_smtp , OBL , 1 },
#endif
	{ "set" , _test_set, _conf_set , OBL , 1 },
	{ "modulo" , _test_modulos, _conf_modulos , OPC , 1 },
	{ "log" , _test_log , _conf_log , OPC , 1 } ,
	{0x0,0x0,0x0}
};
struct bArchivos
{
	char *serv;
	char *archivo;
	int (*test)(Conf *modulo, int *error);
	void (*set)(Conf *modulo, int id);
	int id;
};
;
void libera_conf(Conf *seccion)
{
	int i;
	if (!seccion)
		return;
	ircfree(seccion->item);
	ircfree(seccion->data);
	for (i = 0; i < seccion->secciones; i++)
		libera_conf(seccion->seccion[i]);
	if (!seccion->root && seccion->archivo) /* top */
		ircfree(seccion->archivo);
	if (seccion->root)
		ircfree(seccion);
}
#define pce(x) conferror("[%s:%i] " x, archivo, linea)
/* parseconf
 * 
 * Inicializa una config y la mete en rama.
 * 0 Error
 * 1 Ok
 */
int parseconf(char *archivo, Conf *rama, char avisa)
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
			fecho(FERR, "No existe el archivo %s", archivo);
		return -1;
	}
	if (fstat(fp, &inode) == -1)
	{
		if (avisa)
			fecho(FERR, "No se puede hacer fstat");
		return -2;
	}
	if (!inode.st_size)
	{
		if (avisa)
			fecho(FERR, "El archivo esta vacio");
		close(fp);
		return -3;
	}
	f = par = (char *)Malloc(inode.st_size + 1);
	par[inode.st_size] = '\0';
	if (read(fp, par, inode.st_size) != inode.st_size)
	{
		if (avisa)
			fecho(FERR, "Error fatal de lectura");
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
				da_Malloc(actual, Conf);
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
				da_Malloc(actual, Conf);
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
						if (*par == '*' && *(par + 1) == '/')
							break;
					}
					par++;
				}
				else if (*par == '/')
					for (par++; *par != '\n'; par++);
				break;
			case '#':
				while (*par != '\n')
					par++;
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
						conferror("[%s:%i] Ítem inválido.", archivo, linea);
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
					da_Malloc(actual, Conf);
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
void distribuye_conf(Conf *config)
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
		if (com->func != 0x0)
		{
			if (!com->testfunc(config->seccion[i], &errores))
				com->func(config->seccion[i]);
		}
		else
			conferror("[%s:%i] Sección extra: '%s'.", config->seccion[i]->archivo, config->seccion[i]->linea, config->seccion[i]->item);
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
			if (!busca_entrada(config, com->nombre))
			{
				fecho(FERR, "No se encuentra la seccion %s", com->nombre);
				errores++;
			}		
		}
		com++;
	}
	if (errores)
	{
		fecho(FERR, "Hay %i error%s en la configuracion. No se puede cargar", errores, errores > 1 ? "es" : "");
		cierra_colossus(-1);
	}
	libera_conf(config);
}
Conf *busca_entrada(Conf *config, char *meta)
{
	int i;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcasecmp(config->seccion[i]->item, meta))
			return config->seccion[i];
	}
	return NULL;
}
int _test_server(Conf *config, int *errores)
{
	short error_parcial = 0;
	int puerto;
	Conf *eval, *aux;
	struct hostent *addr;
	if (!(eval = busca_entrada(config, "addr")))
	{
		conferror("[%s:%s] No se encuentra la directriz addr.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz addr esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		if (!(addr = gethostbyname(eval->data)))
		{
			conferror("[%s:%s::%s] No se puede resolver el host.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "puerto")))
	{
		conferror("[%s:%s] No se encuentra la directriz puerto.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz puerto esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 65535)
			{
				conferror("[%s:%s::%s] El puerto debe estar entre 0 y 65535.", config->archivo, config->item, eval->item);
				error_parcial++;
			}
		}
	}
	if ((eval = busca_entrada(config, "puerto_escucha")))
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz puerto_escucha esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 65535)
			{
				conferror("[%s:%s::%s] El puerto_escucha debe estar entre 0 y 65535.", config->archivo, config->item, eval->item);
				error_parcial++;
			}
			aux = busca_entrada(config, "puerto");
			if (puerto == atoi(aux->data))
			{
				conferror("[%s:%s::%s] El puerto_escucha no puede ser el mismo que puerto.", config->archivo, config->item, eval->item);
				error_parcial++;
			}
		}
	}
	if (!(eval = busca_entrada(config, "password")))
	{
		conferror("[%s:%s] No se encuentra la directriz password.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!(aux = busca_entrada(eval, "local")))
		{
			conferror("[%s:%s::%s] No se encuentra la directriz local.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz local esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
		if (!(aux = busca_entrada(eval, "remoto")))
		{
			conferror("[%s:%s::%s] No se encuentra la directriz remoto.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz remoto esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
	}
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz host esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "info")))
	{
		conferror("[%s:%s] No se encuentra la directriz info.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz puerto esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "numeric")))
	{
		conferror("[%s:%s] No se encuentra la directriz numeric.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz numeric esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			puerto = atoi(eval->data);
			if (puerto < 0 || puerto > 255)
			{
				conferror("[%s:%s::%s] La directriz numeric debe estar entre 0 y 255.", config->archivo, config->item, eval->item);
				error_parcial++;
			}
		}
	}
#ifdef ZIP_LINKS
	if ((eval = busca_entrada(config, "compresion")))
	{
		puerto = atoi(eval->data);
		if (puerto < 1 || puerto > 9)
		{
			conferror("[%s:%s::%s] La directriz compresion debe estar entre 1 y 9.", config->archivo, config->item, eval->item);
				error_parcial++;
		}
	}
#endif
	*errores += error_parcial;
	return error_parcial;
}
void _conf_server(Conf *config)
{
	int i, p;
	if (!conf_server)
		da_Malloc(conf_server, struct Conf_server);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "addr"))
			ircstrdup(&conf_server->addr, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "puerto"))
			conf_server->puerto = atoi(config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "password"))
		{
			if (!conf_server->password)
				da_Malloc(conf_server->password, struct Conf_server_password);
			conf_server->password->local = conf_server->password->remoto = NULL;
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "local"));
					ircstrdup(&conf_server->password->local, config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "remoto"));
					ircstrdup(&conf_server->password->remoto, config->seccion[i]->seccion[p]->data);
			}
		}
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&conf_server->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "info"))
			ircstrdup(&conf_server->info, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "numeric"))
			conf_server->numeric = atoi(config->seccion[i]->data);
#ifdef ZIP_LINKS
		if (!strcmp(config->seccion[i]->item, "compresion"))
			conf_server->compresion = atoi(config->seccion[i]->data);
#endif
		if (!strcmp(config->seccion[i]->item, "puerto_escucha"))
			conf_server->escucha = atoi(config->seccion[i]->data);
	}
	if (!conf_server->escucha)
		conf_server->escucha = conf_server->puerto;
}
int _test_db(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	struct hostent *db;
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz host esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		if (!(db = gethostbyname(eval->data)))
		{
			conferror("[%s:%s::%s] No se puede resolver el host.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "login")))
	{
		conferror("[%s:%s] No se encuentra la directriz login.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz login esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "pass")))
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz pass esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "nombre")))
	{
		conferror("[%s:%s] No se encuentra la directriz nombre.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz nombre esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "prefijo")))
	{
		conferror("[%s:%s] No se encuentra la directriz prefijo.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz prefijo esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "puerto")))
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz puerto esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			int puerto;
			puerto = atoi(eval->data);
			if (puerto < 1 || puerto > 65535)
			{
				conferror("[%s:%s::%s] El puerto debe estar entre 1 y 65535.", config->archivo, config->item, eval->item);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void _conf_db(Conf *config)
{
	int i;
	if (!conf_db)
		da_Malloc(conf_db, struct Conf_db);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&conf_db->host, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "login"))
			ircstrdup(&conf_db->login, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "pass"))
			ircstrdup(&conf_db->pass, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "nombre"))
			ircstrdup(&conf_db->bd, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "prefijo"))
			ircstrdup(&PREFIJO, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item ,"puerto"))
			conf_db->puerto = atoi(config->seccion[i]->data);
	}
}
int _test_smtp(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	struct hostent *smtp;
	if (!(eval = busca_entrada(config, "host")))
	{
		conferror("[%s:%s] No se encuentra la directriz host.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz host esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		if (!(smtp = gethostbyname(eval->data)))
		{
			conferror("[%s:%s::%s] No se puede resolver el host.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "login")))
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz login esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "pass")))
	{
		if (!(eval = busca_entrada(config, "login")))
		{
			conferror("[%s:%s::%s] Para usar la directriz pass se requiere la directriz login.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz pass esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void _conf_smtp(Conf *config)
{
	int i;
	if (!conf_smtp)
		da_Malloc(conf_smtp, struct Conf_smtp);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "host"))
			ircstrdup(&(conf_smtp->host), config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "login"))
			ircstrdup(&(conf_smtp->login), config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "pass"))
			ircstrdup(&(conf_smtp->pass), config->seccion[i]->data);
	}
}
int _test_set(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval, *aux;
	if (!(eval = busca_entrada(config, "respuesta")))
	{
		conferror("[%s:%s] No se encuentra la directriz respuesta.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz respuesta esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		if (strcasecmp(eval->data, "PRIVMSG") && strcasecmp(eval->data, "NOTICE"))
		{
			conferror("[%s:%s::%s] La respuesta solo puede tomar los valores PRIVMSG o NOTICE.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "root")))
	{
		conferror("[%s:%s] No se encuentra la directriz root.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!eval->data)
		{
			conferror("[%s:%s::%s] La directriz root esta vacia.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
	}
	if ((eval = busca_entrada(config, "autojoin")))
	{
		if ((aux = busca_entrada(eval, "usuarios")))
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz usuarios esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
			if (*(aux->data) != '#')
			{
				conferror("[%s:%s::%s::%s] Los canales deben empezar por #.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
		if ((aux = busca_entrada(eval, "opers")))
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz opers esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
			if (*(aux->data) != '#')
			{
				conferror("[%s:%s::%s::%s] Los canales deben empezar por #.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
	}
	if ((eval = busca_entrada(config, "modos")))
	{
		if ((aux = busca_entrada(eval, "usuarios")))
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz usuarios esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
		if ((aux = busca_entrada(eval, "canales")))
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz canales esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
			else
			{
				char forb[] = "ohvbeqar";
				int i;
				for (i = 0; forb[i] != '\0'; i++)
				{
					if (strchr(aux->data, forb[i]))
					{
						conferror("[%s:%s::%s::%s] Los modos +ohvbeqar no se permiten.", config->archivo, config->item, eval->item, aux->item);
						error_parcial++;
					}
				}
			}
		}
	}
	/*if (!(eval = busca_entrada(config, "blocks")))
	{
		conferror("[%s:%s] No se encuentra la directriz blocks.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!(aux = busca_entrada(eval, "bline")))
		{
			conferror("[%s:%s::%s] No se encuentra la directriz bline.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!aux->data)
			{
				conferror("[%s:%s::%s::%s] La directriz bline esta vacia.", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
	}*/
	if (!(eval = busca_entrada(config, "cloak")))
	{
		conferror("[%s:%s] No se encuentra la directriz cloak.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (eval->secciones != 3)
		{
			conferror("[%s:%s::%s] Solo se necesitan 3 claves, no %i.", config->archivo, config->item, eval->item, eval->secciones);
			error_parcial++;
		}
	}
	if (!(eval = busca_entrada(config, "admin")))
	{
		conferror("[%s:%s] No se encuentra la directriz admin.", config->archivo, config->item);
		error_parcial++;
	}
	if (!(eval = busca_entrada(config, "reconectar")))
	{
		conferror("[%s:%s] No se encuentra la directriz reconectar.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int rec;
		if (!(aux = busca_entrada(eval, "intentos")))
		{
			conferror("[%s:%s::%s] No se encuentra la directriz intentos.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			
			if (!(rec = atoi(aux->data)) || rec < 1 || rec >999)
			{
				conferror("[%s:%s::%s::%s] Intentos de reconexion invalido (1-999).", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
		if (!(aux = busca_entrada(eval, "intervalo")))
		{
			conferror("[%s:%s::%s] No se encuentra la directriz intervalo.", config->archivo, config->item, eval->item);
			error_parcial++;
		}
		else
		{
			if (!(rec = atoi(aux->data)) || rec < 1 || rec > 300)
			{
				conferror("[%s:%s::%s::%s] Intervalo de reconexion invalido (1-300).", config->archivo, config->item, eval->item, aux->item);
				error_parcial++;
			}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void _conf_set(Conf *config)
{
	int i, p;
	if (!conf_set)
	{
		da_Malloc(conf_set, struct Conf_set);
		conf_set->opts = RESP_PRIVMSG;
		conf_set->nicklen = 30;
	}
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "autobop"))
			conf_set->opts |= AUTOBOP;
		if (!strcmp(config->seccion[i]->item, "rekill"))
			conf_set->opts |= REKILL;
		if (!strcmp(config->seccion[i]->item, "no_server_deop"))
			conf_set->opts |= NOSERVDEOP;
		if (!strcmp(config->seccion[i]->item, "rejoin"))
			conf_set->opts |= REJOIN;
		if (!strcmp(config->seccion[i]->item, "respuesta"))
		{
			if (!strcasecmp(config->seccion[i]->data, "PRIVMSG"))
				conf_set->opts |= RESP_PRIVMSG;
			if (!strcasecmp(config->seccion[i]->data, "NOTICE"))
			conf_set->opts |= RESP_NOTICE;
		}
		if (!strcmp(config->seccion[i]->item, "root"))
			ircstrdup(&conf_set->root, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "autojoin"))
		{
			if (!conf_set->autojoin)
				da_Malloc(conf_set->autojoin, struct Conf_set_autojoin);
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "usuarios"))
					ircstrdup(&conf_set->autojoin->usuarios, config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "opers"))
					ircstrdup(&conf_set->autojoin->operadores, config->seccion[i]->seccion[p]->data);
			}
		}
		if (!strcmp(config->seccion[i]->item, "modos"))
		{
			if (!conf_set->modos)
				da_Malloc(conf_set->modos, struct Conf_set_modos);
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "usuarios"))
					ircstrdup(&conf_set->modos->usuarios, config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "canales"))
					ircstrdup(&conf_set->modos->canales, config->seccion[i]->seccion[p]->data);
			}
		}
		/*if (!strcmp(config->seccion[i]->item, "blocks"))
		{
			if (!conf_set->blocks)
				da_Malloc(conf_set->blocks, struct Conf_set_blocks);
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "bline"))
					ircstrdup(&conf_set->blocks->bline, config->seccion[i]->seccion[p]->data);
			}
		}*/
		if (!strcmp(config->seccion[i]->item, "cloak"))
		{
			char tmp[512], result[16];
			MD5_CTX hash;
			if (!conf_set->cloak)
				da_Malloc(conf_set->cloak, struct Conf_set_cloak);
			ircstrdup(&conf_set->cloak->clave1, config->seccion[i]->seccion[0]->item);
			ircstrdup(&conf_set->cloak->clave2, config->seccion[i]->seccion[1]->item);
			ircstrdup(&conf_set->cloak->clave3, config->seccion[i]->seccion[2]->item);
			sprintf_irc(tmp, "%s:%s:%s", conf_set->cloak->clave1, conf_set->cloak->clave2, conf_set->cloak->clave3);
			MD5Init(&hash);
			MD5Update(&hash, tmp, strlen(tmp));
			MD5Final(result, &hash);
			sprintf_irc(conf_set->cloak->crc, "MD5:%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
				(u_int)(result[0] & 0xf), (u_int)(result[0] >> 4),
				(u_int)(result[1] & 0xf), (u_int)(result[1] >> 4),
				(u_int)(result[2] & 0xf), (u_int)(result[2] >> 4),
				(u_int)(result[3] & 0xf), (u_int)(result[3] >> 4),
				(u_int)(result[4] & 0xf), (u_int)(result[4] >> 4),
				(u_int)(result[5] & 0xf), (u_int)(result[5] >> 4),
				(u_int)(result[6] & 0xf), (u_int)(result[6] >> 4),
				(u_int)(result[7] & 0xf), (u_int)(result[7] >> 4),
				(u_int)(result[8] & 0xf), (u_int)(result[8] >> 4),
				(u_int)(result[9] & 0xf), (u_int)(result[9] >> 4),
				(u_int)(result[10] & 0xf), (u_int)(result[10] >> 4),
				(u_int)(result[11] & 0xf), (u_int)(result[11] >> 4),
				(u_int)(result[12] & 0xf), (u_int)(result[12] >> 4),
				(u_int)(result[13] & 0xf), (u_int)(result[13] >> 4),
				(u_int)(result[14] & 0xf), (u_int)(result[14] >> 4),
				(u_int)(result[15] & 0xf), (u_int)(result[15] >> 4));
		}
		if (!strcmp(config->seccion[i]->item, "admin"))
			ircstrdup(&conf_set->admin, config->seccion[i]->data);
		if (!strcmp(config->seccion[i]->item, "reconectar"))
		{
			if (!conf_set->reconectar)
				da_Malloc(conf_set->reconectar, struct Conf_set_reconectar);
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "intentos"))
					conf_set->reconectar->intentos = atoi(config->seccion[i]->seccion[p]->data);
				if (!strcmp(config->seccion[i]->seccion[p]->item, "intervalo"))
					conf_set->reconectar->intervalo = atoi(config->seccion[i]->seccion[p]->data);
			}
		}
		if (!strcmp(config->seccion[i]->item, "debug"))
			ircstrdup(&conf_set->debug, config->seccion[i]->data);
	}
}
int _test_modulos(Conf *config, int *errores)
{
	return 0;
}
void _conf_modulos(Conf *config)
{
	if (crea_modulo(config->data))
		conferror("[%s:%s] No se puede cargar %s.", config->archivo, config->item, config->data);
}
int _test_log(Conf *config, int *errores)
{
	short error_parcial = 0;
	Conf *eval;
	if ((eval = busca_entrada(config, "tamaño")))
	{
		if (atoi(eval->data) < 0)
		{
			conferror("[%s:%s] El tamaño máximo del archivo log debe ser mayor que 0 bytes.", config->archivo, config->item);
			error_parcial++;
		}
	}
	if (BadPtr(config->data))
	{
		conferror("[%s:%s] Falta nombre de archivo.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (!is_file(config->data))
			fclose(fopen(config->data, "w"));
	}
	if (!(eval = busca_entrada(config, "opciones")))
	{
		conferror("[%s:%s] No se encuentra la directriz opciones.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		int i;
		for (i = 0; i < eval->secciones; i++)
		{
			if (strcmp(eval->seccion[i]->item, "conexiones") &&
				strcmp(eval->seccion[i]->item, "errores") &&
				strcmp(eval->seccion[i]->item, "servidores"))
				{
					conferror("[%s:%s::%s] No es una opción válida %s.", config->archivo, config->item, eval->item, eval->seccion[i]->item);
					error_parcial++;
				}
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void _conf_log(Conf *config)
{
	int i, p;
	if (!conf_log)
		da_Malloc(conf_log, struct Conf_log);
	ircstrdup(&conf_log->archivo, config->data);
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "tamaño"))
			conf_log->size = atoi(config->seccion[i]->data) * 1024;
		if (!strcmp(config->seccion[i]->item, "opciones"))
		{
			for (p = 0; p < config->seccion[i]->secciones; p++)
			{
				if (!strcmp(config->seccion[i]->seccion[p]->item, "conexiones"))
					conf_log->opts |= LOG_CONN;
				if (!strcmp(config->seccion[i]->seccion[p]->item, "errores"))
					conf_log->opts |= LOG_ERROR;
				if (!strcmp(config->seccion[i]->seccion[p]->item, "servidores"))
					conf_log->opts |= LOG_SERVER;
			}
		}
	}
}
#ifndef _WIN32
void conferror(char *formato, ...)
{
	char buf[BUFSIZE], *texto = NULL;
	va_list vl;
	va_start(vl, formato);
	vsprintf_irc(buf, formato, vl);
	va_end(vl);
	strcat(buf, "\r\n");
	fprintf(stderr, buf);	
	Free(texto);
}
#endif
