/*
 * $Id: parseconf.h,v 1.7 2005-02-18 22:12:13 Trocotronic Exp $ 
 */

#define MAXSECS 128
#define AUTOBOP 0x1
#define REKILL 0x2
#define RESP_PRIVMSG 0x4
#define RESP_NOTICE 0x8
#define REJOIN 0x10
#define NO_OVERRIDE 0x20
typedef struct _conf Conf;
typedef struct _conf_com cComConf;

extern int parseconf(char *, Conf *, char);
extern void libera_conf(Conf *);
extern void distribuye_conf(Conf *);
extern Conf *busca_entrada(Conf *, char *);

struct _conf
{
	char *item;
	char *data;
	struct _conf *seccion[MAXSECS];
	struct _conf *root;
	int secciones;
	char *archivo;
	int linea;
};
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
	}*password;
	int numeric;
#ifdef USA_ZLIB
	int compresion;
#endif
#ifdef USA_SSL
	char *cifrados;
#endif
	int escucha;
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
};
struct Conf_set
{
	int opts;
	char *root;
	int nicklen;
	struct Conf_set_cloak
	{
		char *clave1;
		char *clave2;
		char *clave3;
		u_char crc[64];
	}*cloak;
	char *admin;
	char *red;
	struct Conf_set_reconectar
	{
		int intentos;
		int intervalo;
	}*reconectar;
	char *debug;
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
	char usa_egd;
	char *egd_path;
	char *x_server_cert_pem;
	char *x_server_key_pem;
	char *trusted_ca_file;
};
#endif
typedef struct _opts Opts;
struct _opts
{
	int opt;
	char *item;
};
#define LOG_ERROR 0x1
#define LOG_SERVER 0x2
#define LOG_CONN 0x4

struct _conf_com
{
	char *nombre;
	int (*testfunc)(Conf *, int *);
	void (*func)(Conf *);
	short opt;
	DWORD min_os;
};
extern MODVAR struct Conf_server *conf_server;
extern MODVAR struct Conf_db *conf_db;
extern MODVAR struct Conf_smtp *conf_smtp;
extern MODVAR struct Conf_set *conf_set;
extern MODVAR struct Conf_log *conf_log;
#ifdef USA_SSL
extern MODVAR struct Conf_ssl *conf_ssl;
#endif
#define PREFIJO conf_db->prefijo
#define OPC 1
#define OBL 2
#ifdef USA_SSL
#define SSL_SERVER_CERT_PEM (conf_ssl->x_server_cert_pem ? conf_ssl->x_server_cert_pem : "server.cert.pem")
#define SSL_SERVER_KEY_PEM (conf_ssl->x_server_key_pem ? conf_ssl->x_server_key_pem : "server.key.pem")
#endif
