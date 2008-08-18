/*
 * $Id: parseconf.h,v 1.23 2008/05/31 21:46:06 Trocotronic Exp $
 */

#define MAXSECS 128
#define AUTOBOP 0x1
#define REKILL 0x2
#define RESP_PRIVMSG 0x4
#define RESP_NOTICE 0x8
#define REJOIN 0x10
#define NO_OVERRIDE 0x20

/*!
 * @desc: Estructura recursiva que conforma un archivo de configuración tras su parseo.
 * @params: $item Nombre de la directriz o bloque.
 	    $data Valor de la directriz o bloque.
 	    $seccion Subbloques de este bloque (bloques hijos).
 	    $root Bloque padre.
 	    $secciones Número de subbloques que tiene.
 	    $archivo Ruta al archivo de configuración.
 	    $linea Línea de la directriz o bloque.
 * @cat: Configuracion
 * @ver: ParseaConfiguracion
 !*/

typedef struct _conf
{
	char *item;
	char *data;
	struct _conf *seccion[MAXSECS];
	struct _conf *root;
	int secciones;
	char *archivo;
	int linea;
}Conf;
struct Conf_server
{
	char *addr;
	int puerto;
	char *host;
	char *info;
	struct Conf_server_password
	{
		char *local;
		char *remoto;
	}password;
	int numeric;
#ifdef USA_ZLIB
	int compresion;
#endif
#ifdef USA_SSL
	unsigned usa_ssl:1;
#endif
	int escucha;
	char *bind_ip;
};
struct Conf_db
{
	char *host;
	char *login;
	char *pass;
	char *bd;
	char *prefijo;
	int puerto;
};
struct Conf_smtp
{
	char *host;
	char *login;
	char *pass;
	int puerto;
#ifdef USA_SSL
	unsigned ssl:1;
#endif
};
struct Conf_set
{
	int opts;
	char *root;
	char clave_cifrado[33];
	char *admin;
	struct Conf_set_reconectar
	{
		int intentos;
		int intervalo;
	}reconectar;
	char *debug;
	char *red;
	unsigned actualiza:1;
};
struct Conf_log
{
	int opts;
	int size;
	char *archivo;
};
#ifdef USA_SSL
struct Conf_ssl
{
	int opts;
	int usa_egd;
	char *egd_path;
	char *x_server_cert_pem;
	char *x_server_key_pem;
	char *trusted_ca_file;
	char *cifrados;
};
#endif
struct Conf_httpd
{
	int puerto;
	char *url;
	int max_age;
	char *php;
};
struct Conf_msn
{
	char *cuenta;
	char *pass;
	char *master;
	unsigned solomaster:1;
	char *servidor;
};

#define LOG_ERROR 0x1
#define LOG_SERVER 0x2
#define LOG_CONN 0x4
#define LOG_MSN 0x8

typedef struct _conf_com
{
	char *nombre;
	int (*testfunc)(Conf *, int *);
	void (*func)(Conf *);
	short opt;
	DWORD min_os;
}cComConf;

extern int ParseaConfiguracion(char *, Conf *, char);
extern void LiberaMemoriaConfiguracion(Conf *);
extern void DistribuyeConfiguracion(Conf *);
extern Conf *BuscaEntrada(Conf *, char *);
extern void DescargaConfiguracion();

extern MODVAR struct Conf_server *conf_server;
extern MODVAR struct Conf_db *conf_db;
extern MODVAR struct Conf_smtp *conf_smtp;
extern MODVAR struct Conf_set *conf_set;
extern MODVAR struct Conf_log *conf_log;
#ifdef USA_SSL
extern MODVAR struct Conf_ssl *conf_ssl;
#endif
extern MODVAR struct Conf_httpd *conf_httpd;
extern MODVAR struct Conf_msn *conf_msn;
#define PREFIJO (conf_db ? conf_db->prefijo : "")
#define OPC 1
#define OBL 2
#ifdef USA_SSL
#define SSL_SERVER_CERT_PEM (conf_ssl && conf_ssl->x_server_cert_pem ? conf_ssl->x_server_cert_pem : "server.cert.pem")
#define SSL_SERVER_KEY_PEM (conf_ssl && conf_ssl->x_server_key_pem ? conf_ssl->x_server_key_pem : "server.key.pem")
#endif
