/*
 * $Id: statserv.h,v 1.4 2005-07-13 14:06:25 Trocotronic Exp $ 
 */

#define SS_MAX_COMS 256
#define DOMINUMS 258 /* no tocar */

struct stsno {
	u_int no;
	u_int max;
	u_int hoy;
	u_int semana;
	u_int mes;
	time_t max_time;
	time_t hoy_time;
	time_t semana_time;
	time_t mes_time;
};
typedef struct _ss StatServ;
struct _ss
{
	char *nick;
	char *ident;
	char *host;
	char *modos;
	char *realname;
	char *mascara;
	int opts;
	char *residente;
	bCom *comando[SS_MAX_COMS]; /* comandos soportados */
	int comandos;
	int laguea;
	int puerto;
	char *template;
	/* aqui empiezan los stat */
	struct stsno usuarios;
	struct stsno canales;
	struct stsno servidores;
	struct stsno operadores;
	// stat de servidores
	struct {
		Cliente *serv;
		u_int usuarios;
		u_int operadores;
		u_int max_users;
		u_int lag;
		char *version;
		u_int uptime;
	}servidor[256];
	// stat de clientes
	struct {
		char *cliente;
		char *patron;
		int usuarios;
	}clientes[256];
	// stat de dominios
	struct {
		char dominio[5]; /* como máximo son 4, .info */
		char *donde;
		u_int usuarios;
	}dominios[DOMINUMS]; /* +1 para los desconocidos */
	Cliente *cl;
	Modulo *mod;
};

extern StatServ *statserv;

#define SS_ERR_PARA "\00304ERROR: Faltan parámetros: %s "
#define SS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define SS_ERR_EMPT "\00304ERROR: %s"

#define SS_SQL "stats"

#define actualiza(x,y) SQLInserta(SS_SQL, x, "valor", "%lu", y)
#define coge(x) (u_int)atoi(SQLCogeRegistro(SS_SQL, x, "valor"))

#define STSUSERS 1
#define STSCHANS 2
#define STSSERVS 3
#define STSOPERS 4
