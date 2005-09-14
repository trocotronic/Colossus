/*
 * $Id: sql.h,v 1.2 2005-09-14 14:45:04 Trocotronic Exp $ 
 */

typedef char ** SQLRow;
typedef void * SQLRes;
#define MAX_TAB 128
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
	int (*Carga)();
	void (*Descarga)();
	void (*CargaTablas)();
	char servinfo[128];
	char clientinfo[128];
	char *tablas[MAX_TAB][MAX_TAB]; /* la posicion [x][0] siempre contendrá el nombre de la tabla */
}*SQL;

extern MODVAR SQL sql;

extern int CargaSQL(char *);
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
