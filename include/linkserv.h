#define LS_MAX_COMS 256

typedef struct _ls LinkServ;
struct _ls
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	char *residente;
	bCom *comando[LS_MAX_COMS]; /* comandos soportados */
	int comandos;
};

extern LinkServ *linkserv;

#define LS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define LS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define LS_ERR_EMPT "\00304ERROR: %s"
#define LS_ERR_FORB ERR_FORB

#define LS_MYSQL "links"

typedef struct _links Links;
struct _links
{
	Sock *sck;
	char *red;
}; /* lo hago como array dinámico */
