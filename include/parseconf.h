/*
 * $Id: parseconf.h,v 1.3 2004-09-24 22:41:11 Trocotronic Exp $ 
 */

#define MAXSECS 128
#define AUTOBOP 0x1
#define REKILL 0x2
#define RESP_PRIVMSG 0x4
#define RESP_NOTICE 0x8
#define NOSERVDEOP 0x10
#define REJOIN 0x20
typedef struct _conf Conf;
typedef struct _conf_com cComConf;

extern int parseconf(char *, Conf *, char);
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
	struct Conf_set_autojoin
	{
		char *usuarios;
		char *operadores;
	}*autojoin;
	struct Conf_set_modos
	{
		char *usuarios;
		char *canales;
	}*modos;
	/*struct Conf_set_blocks
	{
		char *bline;
	}*blocks;*/
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
#define PREFIJO conf_db->prefijo
#define OPC 1
#define OBL 2
