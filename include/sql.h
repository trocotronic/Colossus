/*
 * $Id: sql.h,v 1.7 2008/05/31 21:46:06 Trocotronic Exp $
 */

typedef char ** SQLRow;
typedef void * SQLRes;
#define MAX_TAB 128
typedef struct _sql {
	void *recurso;
	char servinfo[128];
	char clientinfo[128];
	struct sqltabla
	{
		char *tabla;
		int campos;
		char *create;
		unsigned cargada:1;
		char *campo[MAX_TAB];
	}tabla[MAX_TAB];
	int _errno;
	int tablas;
	const char *_errstr;
}*SQL;

extern MODVAR SQL sql;

extern int CargaSQL();
extern void LiberaSQL();

extern SQLRes SQLQuery(const char *, ...);
extern char *SQLEscapa(const char *);
extern int SQLEsTabla(char *);
extern void SQLCargaTablas();
extern SQLRow SQLFetchRow(SQLRes);
extern void SQLFreeRes(SQLRes);
extern char *SQLCogeRegistro(char *, char *, char *);
extern void SQLInserta(char *, char *, char *, char *, ...);
extern void SQLBorra(char *, char *);
extern int SQLNumRows(SQLRes);
extern int SQLEsCampo(char *, char *);
extern void SQLSeek(SQLRes, u_long);
extern int SQLVersionTabla(char *);
extern int SQLNuevaTabla(char *, char *, ...);
extern SQLRes SQLQueryVL(const char *, va_list);
extern int SQLDump(char *);
extern void SQLRefrescaTabla(char *);
extern int SQLImportar(char *);
#define SQL_BCK_DIR "database/mysql/backup"
