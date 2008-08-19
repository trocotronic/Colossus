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
	char *tablas[MAX_TAB][MAX_TAB]; /* la posicion [x][0] siempre contendrá el nombre de la tabla */
	int _errno;
	char *tablecreate[MAX_TAB]; /* rutinas para la creacion de tablas */
	int numtablas;
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
