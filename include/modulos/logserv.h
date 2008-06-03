/*
 * $Id: logserv.h,v 1.1 2006/11/01 11:38:26 Trocotronic Exp $ 
 */

typedef struct _ls LogServ;
struct _ls
{
	Modulo *hmod;
};

#define LS_SQL "logs"

#define LS_ERR_PARA "\00304ERROR: Faltan parámetros: %s %s "
#define LS_ERR_SNTX "\00304ERROR: Sintaxis incorrecta: %s"
#define LS_ERR_EMPT "\00304ERROR: %s"

typedef struct _logchan LogChan;
struct _logchan
{
	struct _logchan *sig;
	int fd;
	Canal *cn;
	char envio[8];
};
#define DIR_LOGS "modulos/logserv/"
