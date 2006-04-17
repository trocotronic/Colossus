typedef struct _pbs ProbServ;
struct _pbs
{
	Modulo *hmod;
};

#define PBS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define PBS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define PBS_ERR_EMPT "\00304ERROR: %s"

#define PBS_SQL "prueba"

extern ProbServ *probserv;
