typedef struct _ws NewsServ;
struct _ws
{
	Modulo *hmod;
};

#define WS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define WS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define WS_ERR_EMPT "\00304ERROR: %s"

typedef struct _noticia Noticia;
typedef struct _noticia
{
	Noticia *sig;
	char titular[BUFSIZE];
	char *t;
	char descripcion[BUFSIZE];
	char *d;
	u_int id;
	u_int servicio;
};
typedef struct _rss 
{
	u_int opt;
	u_int tipo;
	u_int *ids;
	u_int items;
	u_int *tmp;
	u_int q;
	XML_Parser pxml;
	FILE *fp;
	Noticia not;
	Sock *sck;
	u_int servicio;
}Rss;

#define WS_SQL "noticias_rss"
