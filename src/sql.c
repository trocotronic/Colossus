/*
 * $Id: sql.c,v 1.13 2007-08-20 01:46:24 Trocotronic Exp $ 
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
void SetSQLErrno();

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
int CargaSQL(char *sqlf)
{
	char archivo[MAX_FNAME], tmppath[PMAX];
	int (*Carga)();
	if (sql)
		LiberaSQL();
	sql = BMalloc(struct _sql);
	if ((sql->hmod = CopiaDll(sqlf, archivo, tmppath)))
	{
		irc_dlsym(sql->hmod, "Carga", Carga);
		if (!Carga)
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Carga)", archivo);
			LiberaSQL();
			return 3;
		}
		sql->Carga = Carga;
		if (Carga())
		{
			Alerta(FADV, "Ha sido imposible cargar %s (Carga())", archivo);
			LiberaSQL();
			return 3;
		}
	}
	else
	{
		Alerta(FADV, "Ha sido imposible cargar %s (dlopen): %s", archivo, irc_dlerror());
		return 4;
	}
	SetSQLErrno();
	return 0;
}

/*!
 * @desc: Mira si una tabla existe o no.
 * @params: $tabla [in] Nombre de la tabla.
 * @ret: Devuelve 1 si existe; 0, si no.
 * @cat: SQL
 !*/
 
int SQLEsTabla(char *tabla)
{
	char buf[256];
	int i = 0;
	if (sql)
	{
		ircsprintf(buf, "%s%s", PREFIJO, tabla);
		while (sql->tablas[i][0])
		{
			if (!strcasecmp(sql->tablas[i][0], buf))
				return 1;
			i++;
		}
	}
	return 0;
}

/*!
 * @desc: Mira si un campo existe en una tabla o no.
 * @params: $tabla [in] Tabla a examinar.
 	    $campo [in] Campo a mirar.
 * @ret: Devuelve 1 si el campo existe en esa tabla; 0, si no.
 * @cat: SQL
 !*/
 
int SQLEsCampo(char *tabla, char *campo)
{
	char buf[256];
	int i = 0, j;
	if (sql)
	{
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
	}
	return 0;
}

/*!
 * @desc: Manda un privado a la base de datos SQL.
 * @params: $query [in] Privado. Cadena con formato.
 	    $... [in] Argumentos variables según cadena con formato.
 * @ret: Devuelve un recurso de Privado SQL; NULL si hay un error.
 * @cat: SQL
 !*/
 
SQLRes SQLQuery(const char *query, ...)
{
	va_list vl;
	SQLRes res = NULL;
	char buf[BUFSIZE];
	if (sql)
	{
		va_start(vl, query);
		ircvsprintf(buf, query, vl);
		va_end(vl);
#ifdef DEBUG
		Debug("SQL Query: %s", buf);
#endif
		res = sql->Query(buf);
		SetSQLErrno();
	}
	return res;
}

/*!
 * @desc: Escapa o prepara un elemento para ser enviado en un privado SQL.
 * @params: $item [in] Cadena a preparar.
 * @ret: Devuelve una nueva cadena preparada para ser enviada. Una vez enviada, debe liberarse con Free.
 * @ver: Free
 * @cat: SQL
 !*/
 
char *SQLEscapa(const char *item)
{
	char *esc = NULL;
	if (sql)
	{
		esc = sql->Escapa(item);
		SetSQLErrno();
	}
	return esc;
}

/*!
 * @desc: Refresca las tablas.
 * Esta función debe llamarse cada vez que se cree una nueva tabla.
 * @ver: SQLEsTabla
 * @cat: SQL
 !*/
 
void SQLCargaTablas()
{
	if (sql)
	{
		sql->CargaTablas();
		SetSQLErrno();
	}
}

/*!
 * @desc: Libera un privado.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
void SQLFreeRes(SQLRes res)
{
	if (sql)
	{
		sql->FreeRes(res);
		SetSQLErrno();
	}
}

/*!
 * @desc: Devuelve una fila correspondiente a la respuesta del privado.
 Cada vez que sea llamado devuelve una nueva fila. Devuelve NULL cuando no hay más filas a devolver.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
SQLRow SQLFetchRow(SQLRes res)
{
	SQLRow row = NULL;
	if (sql)
	{
		row = sql->FetchRow(res);
		SetSQLErrno();
	}
	return row;
}

/*!
 * @desc: Coge un registro SQL insertado con SQLInserta.
 * @params: $tabla [in] Tabla a la que pertenece el registro.
 	    $registro [in] Registro o índice a buscar.
 	    $campo [in] Campo a devolver. Si es NULL, consulta si existe ese registro o no.
 * @ret: Devuelve el valor del campo; NULL si no existe.
 * @ver: SQLInserta SQLBorra
 * @cat: SQL
 !*/
 
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
	SetSQLErrno();
	ircfree(reg_corr);
	if (campo)
		ircfree(cam_corr);
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
		return (char *)!NULL;
	}
	if (BadPtr(row[0]))
	{
		SQLFreeRes(res);
		return NULL;
	}
	strlcpy(resultado, row[0], sizeof(resultado));
	SQLFreeRes(res);
	return resultado;
}

/*!
 * @desc: Inserta o actualiza un registro SQL.
 * @params: $tabla [in] Tabla del registro.
 	    $registro [in] Registro o índice.
 	    $campo [in] Campo a insertar o modificar.
 	    $valor [in] Cadena con formato que pertenece al campo.
 	    $... [in] Argumentos variables según cadena con formato.
 * @ver: SQLBorra SQLCogeRegistro
 * @cat: SQL
 !*/
 
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
	ircfree(reg_c);
	ircfree(cam_c);
	ircfree(val_c);
	SetSQLErrno();
}

/*!
 * @desc: Elimina un registro insertado con SQLInserta
 * @params: $tabla [in] Tabla donde está el registro.
 	    $registro [in] Registro a eliminar. Si es NULL, se vacía toda la tabla.
 * @ver: SQLInserta SQLCogeRegistro
 * @cat: SQL
 !*/
 
void SQLBorra(char *tabla, char *registro)
{
	if (registro)
	{
		char *reg_c;
		reg_c = SQLEscapa(registro);
		if (SQLCogeRegistro(tabla, registro, NULL))
			SQLQuery("DELETE from %s%s where LOWER(item)='%s'", PREFIJO, tabla, strtolower(reg_c));
		ircfree(reg_c);
	}
	else
		SQLQuery("DELETE from %s%s", PREFIJO, tabla);
	SetSQLErrno();
}

/*!
 * @desc: Devuelve el número de filas de la respuesta de un privado SQL.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ret: Devuelve el número de filas de la respuesta de un privado SQL.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
int SQLNumRows(SQLRes res)
{
	int rows = 0;
	if (sql)
	{
		rows = sql->NumRows(res);
		SetSQLErrno();
	}
	return rows;
}
void SetSQLErrno()
{
	if (sql && sql->GetErrno)
		sql->_errno = sql->GetErrno();
}
/*!
 * @desc: Sitúa el puntero interno de un recurso SQLRes a la fila indicada.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 		$off [in] Número de fila a situar el puntero. Usar 0 para situarlo al principio. Este offset va desde 0 hasta SQLNumRows-1.
 * @ver: SQLQuery SQLFetchRow SQLNumRows
 * @cat: SQL
 !*/
void SQLSeek(SQLRes res, u_long off)
{
	if (sql)
	{
		sql->Seek(res, off);
		SetSQLErrno();
	}
}
