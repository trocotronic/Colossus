/*
 * $Id: sqlite.c,v 1.6 2007-01-18 14:38:00 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "struct.h"
#include <sqlite3.h>

sqlite3 *sqlite = NULL;
SQLRes Query(const char *);
char *Escapa(const char *);
void FreeRes(SQLRes);
int NumRows(SQLRes);
SQLRow FetchRow(SQLRes);
void Descarga();
void CargaTablas();
#ifdef _WIN32
#define DB_PATH "sql\\sqlite\\"
#else
#define DB_PATH "sql/sqlite/"
#endif
int Carga()
{
	mkdir(DB_PATH, 0600);
	ircsprintf(buf, "%s%s.cdt", DB_PATH, conf_db->bd);
	if (sqlite3_open(buf, &sqlite) != SQLITE_OK)
	{
		Alerta(FERR, "SQLite no puede conectar\n%s (%i)", sqlite3_errmsg(sqlite), sqlite3_errcode(sqlite));
		 sqlite3_close(sqlite);
		return -1;
	}
	sql->recurso = (void *)sqlite;
	sql->Query = Query;
	sql->Descarga = Descarga;
	sql->CargaTablas = CargaTablas;
	sql->FetchRow = FetchRow;
	sql->FreeRes = FreeRes;
	sql->NumRows = NumRows;
	sql->Escapa = Escapa;
	ircsprintf(sql->servinfo, "SQLite %s", sqlite3_libversion());
	ircsprintf(sql->clientinfo, "SQLite %s", sqlite3_libversion());
	CargaTablas();
	return 0;
}
SQLRes Query(const char *query)
{
	unsigned char *ztail;
	int err;
	sqlite3_stmt *pmt;	
	if (sqlite3_prepare(sqlite, query, -1, &pmt, &ztail) != SQLITE_OK && (err = sqlite3_errcode(sqlite)) != 1)
	{
		Alerta(FADV, "SQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", query, err, sqlite3_errmsg(sqlite));
		ircfree(ztail);
	}
	if (sqlite3_step(pmt) != SQLITE_ROW)
	{
		sqlite3_finalize(pmt);
		return NULL;
	}
	sqlite3_reset(pmt);
#ifdef DEBUG
	Debug("%s", query);
#endif
	return (SQLRes)pmt;
}
void FreeRes(SQLRes res)
{
	sqlite3_finalize((sqlite3_stmt *)res);
}
int NumRows(SQLRes res)
{
	u_int i;
	sqlite3_reset((sqlite3_stmt *)res);
	for (i = 0; sqlite3_step((sqlite3_stmt *)res) == SQLITE_ROW; i++);
	sqlite3_reset((sqlite3_stmt *)res);
	return i;
}
SQLRow FetchRow(SQLRes res)
{
	int e;
	e = sqlite3_step((sqlite3_stmt *)res);
	if (e == SQLITE_ROW)
	{
		static const unsigned char *result[256];
		int i, m;
		m = sqlite3_column_count((sqlite3_stmt *)res);
		for (i = 0; i < m; i++)
			result[i] = sqlite3_column_text((sqlite3_stmt *)res, i);
		result[i] = NULL;
		return (SQLRow)result;
	}
	return NULL;
}
void Descarga()
{

}
void CargaTablas()
{
	unsigned char *z, *tabla;
	sqlite3_stmt *pmt, *pmt2;
	int i = 0, j;
	if (sqlite3_prepare(sqlite, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name", -1, &pmt, &z) == SQLITE_OK)
	{
		while (sqlite3_step(pmt) == SQLITE_ROW)
		{
			int cols;
			tabla = (char *)sqlite3_column_text(pmt, 0);
			if (strncmp(PREFIJO, tabla, strlen(PREFIJO)))
				continue;
			ircstrdup(sql->tablas[i][0], tabla);
			ircsprintf(buf, "SELECT * FROM %s", tabla);
			if (sqlite3_prepare(sqlite, buf, -1, &pmt2, &z) == SQLITE_OK)
			{
				cols = sqlite3_column_count(pmt2);
				for (j = 0; j < cols; j++)
					ircstrdup(sql->tablas[i][j+1], sqlite3_column_name(pmt2,j));
				sql->tablas[i][j+1] = NULL;
			}
			sqlite3_finalize(pmt2);
			i++;
		}
	}
	sqlite3_finalize(pmt);
}
char *Escapa(const char *item)
{
	return strdup(item);
}
