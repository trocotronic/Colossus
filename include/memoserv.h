#define MS_MAX_COMS 256
typedef struct _ms MemoServ;
struct _ms
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	int def;
	int cada;
	char *residente;
	bCom *comando[MS_MAX_COMS]; /* comandos soportados */
	int comandos;
};

#define MS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define MS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define MS_ERR_NOTR "\00304ERROR: Este canal no está registrado."
#define MS_ERR_EMPT "\00304ERROR: %s"

#define MS_MYSQL "mensajes"
#define MS_SET "mopts"

extern MemoServ *memoserv;

DLLFUNC extern int memoserv_send(char *, char *, char *);

#define MS_OPT_LOG 0x1 /* +l */
#define MS_OPT_AWY 0x2 /* +w */
#define MS_OPT_NEW 0x4 /* +n */
#define MS_OPT_CNO 0x1
#define MS_OPT_ALL (MS_OPT_LOG | MS_OPT_NEW)
