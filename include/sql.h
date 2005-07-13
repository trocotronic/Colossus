/*
 * $Id: sql.h,v 1.1 2005-07-13 14:06:23 Trocotronic Exp $ 
 */

typedef char ** SQLRow;
typedef void * SQLRes;
#define MAX_TAB 128
#define SQL_DIR "sql/"
typedef struct _sql {
	void *recurso;
#ifdef _WIN32
	HMODULE hmod;
#else
	void *hmod;
#endif
	SQLRes (*Query)(const char *);
	char **(*FetchRow)(SQLRes);
	void (*FreeRes)(SQLRes);
	int (*NumRows)(SQLRes);
	char *(*Escapa)(const char *);
	int tipo; /* tipo sistema SQL (mysql 1, postgres 2, etc) */
	int (*Carga)();
	void (*Descarga)();
	void (*CargaTablas)();
	char servinfo[128];
	char clientinfo[128];
	char *tablas[MAX_TAB][MAX_TAB]; /* la posicion [x][0] siempre contendrá el nombre de la tabla */
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
