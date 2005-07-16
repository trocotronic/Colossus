/*
 * $Id: sql.c,v 1.3 2005-07-16 15:25:30 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
extern const char *ErrorDl(void);
#endif

SQL sql = NULL;
void LiberaSQL()
{
	int i, j;
	if (!sql)
		return;
	irc_dlclose(sql->hmod);
	for (i = 0; (sql->tablas[i][0]); i++)
	{
		for (j = 1; (sql->tablas[i][j]); j++)
			ircfree(sql->tablas[i][j]);
		ircfree(sql->tablas[i][0]);
	}
	ircfree(sql);
	sql = NULL;
}
int CargaSQL()
{
	char *ar = NULL, *dll, tmppath[128];
#ifdef _WIN32
	char tmppdb[128], pdb[128];
#endif
	int (*Carga)();
	int tipo = 0;
	if (!strcasecmp(conf_db->tipo, "MySQL"))
	{
#ifdef _WIN32
		ar = SQL_DIR "mysql.dll";
#else
		ar = SQL_DIR "mysql.so";
		tipo = 1;
#endif
	}
	else if (!strcasecmp(conf_db->tipo, "PostGreSQL"))
	{
#ifdef _WIN32
		ar = SQL_DIR "postgresql.dll";
#else
		ar = SQL_DIR "postgresql.so";
#endif
		tipo = 2;
	}
	dll = strrchr(ar, '/')+1;
	ircsprintf(tmppath, "./tmp/%s", dll);
#ifdef _WIN32
	strcpy(pdb, ar);
	dll = strrchr(pdb, '.');
	strcpy(dll, ".pdb");
	dll = strrchr(pdb, '/')+1;
	ircsprintf(tmppdb, "./tmp/%s", dll);
#endif
	if (!copyfile(ar, tmppath))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar)", ar);
		return 1;
	}
#ifdef _WIN32
	if (!copyfile(pdb, tmppdb))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar pdb)", pdb);
		return 2;
	}
#endif
	if (sql)
		LiberaSQL();
	BMalloc(sql, struct _sql);
	if ((sql->hmod = irc_dlopen(tmppath, RTLD_LAZY)))
	{
		irc_dlsym(sql->hmod, "Carga", Carga);
		if (!Carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Carga)", ar);
			LiberaSQL();
			return 3;
		}
		sql->tipo = tipo;
		sql->Carga = Carga;
		if (Carga())
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Carga())", ar);
			LiberaSQL();
			return 3;
		}
	}
	else
	{
		Alerta(FADV, "Ha sido imposible cargar %s (dlopen): %s", ar, irc_dlerror());
		return 4;
	}
	Senyal(SIGN_SQL);
	return 0;
}
int SQLEsTabla(char *tabla)
{
	char buf[256];
	int i = 0;
	ircsprintf(buf, "%s%s", PREFIJO, tabla);
	while (sql->tablas[i][0])
	{
		if (!strcasecmp(sql->tablas[i][0], buf))
			return 1;
		i++;
	}
	return 0;
}
int SQLEsCampo(char *tabla, char *campo)
{
	char buf[256];
	int i = 0, j;
	ircsprintf(buf, "%s%s", PREFIJO, tabla);
	while (sql->tablas[i][0])
	{
		if (!strcasecmp(sql->tablas[i][0], buf))
		{
			for (j = 1; sql->tablas[i][j]; j++)
			{
				if (!strcasecmp(sql->tablas[i][j], campo))
					return 1;
			}
			return 0;
		}
		i++;
	}
	return 0;
}
SQLRes SQLQuery(const char *query, ...)
{
	va_list vl;
	char buf[BUFSIZE];
	va_start(vl, query);
	ircvsprintf(buf, query, vl);
	va_end(vl);
	return sql->Query(buf);
}
char *SQLEscapa(const char *item)
{
	return sql->Escapa(item);
}
void SQLCargaTablas()
{
	sql->CargaTablas();
}
void SQLFreeRes(SQLRes res)
{
	sql->FreeRes(res);
}
SQLRow SQLFetchRow(SQLRes res)
{
	return sql->FetchRow(res);
}
char *SQLCogeRegistro(char *tabla, char *registro, char *campo)
{
	SQLRes *res;
	SQLRow row;
	static char resultado[BUFSIZE];
	char *reg_corr, *cam_corr = NULL;
	reg_corr = SQLEscapa(registro);
	if (campo)
		cam_corr = SQLEscapa(campo);
	res = SQLQuery("SELECT %s from %s%s where LOWER(item)='%s'", cam_corr ? cam_corr : "*", PREFIJO, tabla, strtolower(reg_corr));
	Free(reg_corr);
	if (campo)
		Free(cam_corr);
	if (!res)
		return NULL;
	row = SQLFetchRow(res);
	if (!row)
	{
		SQLFreeRes(res);
		return NULL;
	}
	if (!campo)
	{
		SQLFreeRes(res);
		return (u_char *)!NULL;
	}
	if (BadPtr(row[0]))
	{
		SQLFreeRes(res);
		return NULL;
	}
	strncpy(resultado, row[0], sizeof(resultado));
	SQLFreeRes(res);
	return resultado;
}
void SQLInserta(char *tabla, char *registro, char *campo, char *valor, ...)
{
	char *reg_c, *cam_c, *val_c = NULL;
	char buf[BUFSIZE];
	va_list vl;
	if (!BadPtr(valor))
	{
		va_start(vl, valor);
		ircvsprintf(buf, valor, vl);
		va_end(vl);
	}
	reg_c = SQLEscapa(registro);
	cam_c = SQLEscapa(campo);
	if (!BadPtr(valor))
		val_c = SQLEscapa(buf);
	if (!SQLCogeRegistro(tabla, registro, NULL))
		SQLQuery("INSERT INTO %s%s (item) values ('%s')", PREFIJO, tabla, reg_c);
	SQLQuery("UPDATE %s%s SET %s='%s' where LOWER(item)='%s'", PREFIJO, tabla, cam_c, val_c ? val_c : "", strtolower(reg_c));
	Free(reg_c);
	Free(cam_c);
	ircfree(val_c);
}
void SQLBorra(char *tabla, char *registro)
{
	if (registro)
	{
		char *reg_c;
		reg_c = SQLEscapa(registro);
		if (SQLCogeRegistro(tabla, registro, NULL))
			SQLQuery("DELETE from %s%s where LOWER(item)='%s'", PREFIJO, tabla, strtolower(reg_c));
		Free(reg_c);
	}
	else
		SQLQuery("DELETE from %s%s", PREFIJO, tabla);
}
int SQLNumRows(SQLRes res)
{
	return sql->NumRows(res);
}
