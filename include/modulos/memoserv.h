/*
 * $Id: memoserv.h,v 1.2 2004-12-31 12:27:55 Trocotronic Exp $ 
 */

typedef struct _ms MemoServ;
struct _ms
{
	int opts;
	int def;
	int cada;
	Modulo *hmod;
};

#define MS_ERR_PARA "\00304ERROR: Faltan par�metros: %s "
#define MS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define MS_ERR_NOTR "\00304ERROR: Este canal no est� registrado."
#define MS_ERR_EMPT "\00304ERROR: %s"

#define MS_MYSQL "mensajes"
#define MS_SET "mopts"

extern MemoServ memoserv;

DLLFUNC extern int memoserv_send(char *, char *, char *);

#define MS_OPT_LOG 0x1 /* +l */
#define MS_OPT_AWY 0x2 /* +w */
#define MS_OPT_NEW 0x4 /* +n */
#define MS_OPT_CNO 0x1
#define MS_OPT_ALL (MS_OPT_LOG | MS_OPT_NEW)
