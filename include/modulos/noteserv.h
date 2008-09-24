/*
 * $Id: noteserv.h,v 1.1 2006/11/01 11:38:26 Trocotronic Exp $ 
 */

typedef struct _es NoteServ;
struct _es
{
	Modulo *hmod;
};

#define ES_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define ES_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define ES_ERR_EMPT "\00304ERROR: %s"

#define ES_SQL "agenda"
