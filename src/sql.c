/*
 * $Id: sql.c,v 1.5 2005-09-16 13:48:06 Trocotronic Exp $ 
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
int CargaSQL(char *archivo)
{
	char *amod, tmppath[128];
#ifdef _WIN32
	char tmppdb[128], pdb[128];
#endif
	int (*Carga)();
	amod = strrchr(archivo, '/');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta)", archivo);
		return 1;
	}
	amod++;
	ircsprintf(tmppath, "./tmp/%s", amod);
#ifdef _WIN32
	strcpy(pdb, archivo);
	amod = strrchr(pdb, '.');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla pdb)", archivo);
		return 2;
	}
	strcpy(amod, ".pdb");
	amod = strrchr(pdb, '/');
	if (!amod)
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta pdb)", pdb);
		return 2;
	}
	amod++;
	ircsprintf(tmppdb, "./tmp/%s", amod);
#endif
	if (!copyfile(archivo, tmppath))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar)", archivo);
		return 2;
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
	ircsprintf(buf, "%s%s", PREFIJO, tabla);
	while (sql->tablas[i][0])
	{
		if (!strcasecmp(sql->tablas[i][0], buf))
			return 1;
		i++;
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

/*!
 * @desc: Manda un privado a la base de datos SQL.
 * @params: $query [in] Privado. Cadena con formato.
 	    $... [in] Argumentos variables seg�n cadena con formato.
 * @ret: Devuelve un recurso de Privado SQL; NULL si hay un error.
 * @cat: SQL
 !*/
 
SQLRes SQLQuery(const char *query, ...)
{
	va_list vl;
	char buf[BUFSIZE];
	va_start(vl, query);
	ircvsprintf(buf, query, vl);
	va_end(vl);
	return sql->Query(buf);
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
	return sql->Escapa(item);
}

/*!
 * @desc: Refresca las tablas.
 * Esta funci�n debe llamarse cada vez que se cree una nueva tabla.
 * @ver: SQLEsTabla
 * @cat: SQL
 !*/
 
void SQLCargaTablas()
{
	sql->CargaTablas();
}

/*!
 * @desc: Libera un privado.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
void SQLFreeRes(SQLRes res)
{
	sql->FreeRes(res);
}

/*!
 * @desc: Devuelve una fila correspondiente a la respuesta del privado.
 Cada vez que sea llamado devuelve una nueva fila. Devuelve NULL cuando no hay m�s filas a devolver.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
SQLRow SQLFetchRow(SQLRes res)
{
	return sql->FetchRow(res);
}

/*!
 * @desc: Coge un registro SQL insertado con SQLInserta.
 * @params: $tabla [in] Tabla a la que pertenece el registro.
 	    $registro [in] Registro o �ndice a buscar.
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

/*!
 * @desc: Inserta o actualiza un registro SQL.
 * @params: $tabla [in] Tabla del registro.
 	    $registro [in] Registro o �ndice.
 	    $campo [in] Campo a insertar o modificar.
 	    $valor [in] Cadena con formato que pertenece al campo.
 	    $... [in] Argumentos variables seg�n cadena con formato.
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
	Free(reg_c);
	Free(cam_c);
	ircfree(val_c);
}

/*!
 * @desc: Elimina un registro insertado con SQLInserta
 * @params: $tabla [in] Tabla donde est� el registro.
 	    $registro [in] Registro a eliminar. Si es NULL, se vac�a toda la tabla.
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
		Free(reg_c);
	}
	else
		SQLQuery("DELETE from %s%s", PREFIJO, tabla);
}

/*!
 * @desc: Devuelve el n�mero de filas de la respuesta de un privado SQL.
 * @params: $res [in] Recurso de privado SQL devuelto por SQLQuery.
 * @ret: Devuelve el n�mero de filas de la respuesta de un privado SQL.
 * @ver: SQLQuery
 * @cat: SQL
 !*/
 
int SQLNumRows(SQLRes res)
{
	return sql->NumRows(res);
}
