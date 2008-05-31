/*
 * $Id: sql.c,v 1.16 2008-05-31 22:06:33 Trocotronic Exp $
 */

#include "struct.h"
#include <stdio.h>
#ifdef _WIN32
#include <mysql.h>
#include <winerror.h>
#else
#include <mysql/mysql.h>
#include <sys/stat.h>
#endif
#include "ircd.h"

SQL sql = NULL;
MYSQL *mysql = NULL;
void SetSQLErrno();
SQLRes MySQLQuery(const char *);
static char *server_args[] = {
	"this_program",       /* this string is not used */
	"--datadir=./database/mysql/data",
	"--basedir=./database/mysql",
	"--key_buffer_size=32M"
};
static char *server_groups[] = {
	"embedded",
	"server",
	"this_program_SERVER",
	(char *)NULL
};
void LiberaSQL()
{
	int i, j;
	if (!sql)
		return;
	for (i = 0; (sql->tablas[i][0]); i++)
	{
		for (j = 1; (sql->tablas[i][j]); j++)
			ircfree(sql->tablas[i][j]);
		ircfree(sql->tablas[i][0]);
	}
	mysql_library_end();
	ircfree(sql);
	sql = NULL;
}
int CargaSQL()
{
	char *c;
	my_bool rec = 1;
	FILE *fp;
	mkdir("database/mysql/data",0600);
	if (sql)
		LiberaSQL();
	sql = BMalloc(struct _sql);
	if (mysql_library_init(sizeof(server_args) / sizeof(char *), server_args, server_groups))
		return -1;
	if (!(mysql = mysql_init(NULL)))
		return -1;
	mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "libmysqld_client");
	mysql_options(mysql, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);
	if (!mysql_real_connect(mysql, NULL, NULL, NULL, NULL, 0, NULL, 0))
	{
		Alerta(FERR, "MySQL no puede conectar\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
		return -1;
	}
	if (mysql_select_db(mysql, "colossus"))
	{
#ifdef _WIN32
		if (MessageBox(NULL, "La base de datos no existe. ¿Quiere crearla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (Pregunta("La base de datos no existe. ¿Quiere crearla?") == 1)
#endif
		{
			SQLQuery("CREATE DATABASE IF NOT EXISTS colossus");
			if (sql->_errno)
			{
				Alerta(FERR, "Ha sido imposible crear la base de datos\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -3;
			}
			else
				Alerta(FOK, "La base de datos se ha creado con exito");
			/* Esto no debería ocurrir nunca */
			if (mysql_select_db(mysql, "colossus"))
			{
				Alerta(FERR, "Error fatal\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -4;
			}
		}
		else
		{
			Alerta(FERR, "Para utilizar los servicios es necesario una base de datos");
			return -5;
		}
	}
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &rec);
	sql->recurso = (void *)mysql;
	if ((fp = fopen("utils/prev-db[110].sql", "r")))
	{
#ifdef _WIN32
		if (MessageBox(NULL, "Se ha encontrado una copia del sistema antiguo. ¿Quiere volcar los datos al nuevo sistema (los datos actuales se borrarán)?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (Pregunta("Se ha encontrado una copia del sistema antiguo. ¿Quiere volcar los datos al nuevo sistema (los datos actuales se borraran)?") == 1)
#endif
		{
			char tmp[8092];
			int i;
			SQLCargaTablas();
			for (i = 0; sql->tablas[i][0]; i++)
				SQLQuery("DROP TABLE %s", sql->tablas[i][0]);
			while (fgets(tmp, sizeof(tmp), fp))
			{
				if (tmp[0] != '\n' && tmp[0] != '\r')
					SQLQuery(tmp);
			}
			fclose(fp);
			unlink("utils/prev-db[110].sql");
		}
		else
			fclose(fp);
	}
	ircsprintf(sql->servinfo, "MySQL %s", mysql_get_server_info(mysql));
	ircsprintf(sql->clientinfo, "MySQL %s", mysql_get_client_info());
	if ((c = strchr(sql->servinfo, '-')))
		*c = '\0';
	if ((c = strchr(sql->clientinfo, '-')))
		*c = '\0';
	SQLCargaTablas();
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
	int i = 0;
	if (sql)
	{
		while (sql->tablas[i][0])
		{
			if (!strcasecmp(sql->tablas[i][0], tabla))
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
	int i = 0, j;
	if (sql)
	{
		while (sql->tablas[i][0])
		{
			if (!strcasecmp(sql->tablas[i][0], tabla))
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
SQLRes MySQLQuery(const char *query)
{
	MYSQL_RES *resultado;
	if (!mysql)
	{
		CargaSQL();
		if (!mysql)
			return NULL;
	}
	if (mysql_ping(mysql))
		CargaSQL();
	if (mysql_query(mysql, query) < 0)
	{
		Info("SQL ha detectado un error.[Backup Buffer: %s][%i:%s]\n", query, mysql_errno(mysql), mysql_error(mysql));
		return NULL;
	}
	if ((resultado = mysql_store_result(mysql)))
	{
		if (!mysql_num_rows(resultado))
		{
			mysql_free_result(resultado);
			return NULL;
		}
		return (SQLRes)resultado;
	}
	else if (mysql_errno(mysql))
	{
		Info("SQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", query, mysql_errno(mysql), mysql_error(mysql));
		return NULL;
	}
	return NULL;
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
	char *buf;
	if (sql)
	{
		va_start(vl, query);
		buf = (char *)Malloc(sizeof(char) * (strlen(query)*4+1));
		ircvsprintf(buf, query, vl);
		va_end(vl);
#ifdef DEBUG
		Debug("SQL Query: %s", buf);
#endif
		res = MySQLQuery(buf);
		Free(buf);
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
		esc = (char *)Malloc(sizeof(char) * (strlen(item) * 2 + 1));
		mysql_real_escape_string(mysql, esc, item, strlen(item));
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
		MYSQL_RES *res, *cp;
		MYSQL_ROW row;
		MYSQL_FIELD *field;
		int i = 0, j;
		char *tabla;
		if ((res = mysql_list_tables(mysql, NULL)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				tabla = row[0];
				if (strncmp(PREFIJO, tabla, strlen(PREFIJO)))
					continue;
				ircstrdup(sql->tablas[i][0], &tabla[strlen(PREFIJO)]);
				ircsprintf(buf, "SELECT * FROM %s", tabla);
				if ((mysql_query(mysql, buf)) || (!(cp = mysql_store_result(mysql))))
				{
					Alerta(FADV, "SQL ha detectado un error durante la carga de tablas.\n[%i: %s]\n", mysql_errno(mysql), mysql_error(mysql));
					continue;
				}
				for (j = 1; (field = mysql_fetch_field(cp)); j++)
					ircstrdup(sql->tablas[i][j], field->name);
				sql->tablas[i][j] = NULL;
				mysql_free_result(cp);
				i++;
			}
			sql->tablas[i][0] = NULL;
			mysql_free_result(res);
		}
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
		mysql_free_result((MYSQL_RES *)res);
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
		row = (SQLRow)mysql_fetch_row((MYSQL_RES *)res);
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
		rows = (int)mysql_num_rows((MYSQL_RES *)res);
		SetSQLErrno();
	}
	return rows;
}
void SetSQLErrno()
{
	if (sql)
		sql->_errno = mysql_errno(mysql);
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
		mysql_data_seek(res, off);
		SetSQLErrno();
	}
}

/*!
 * @desc: Devuelve la versión de una tabla.
 * @params: $tabla [in] Nombre de la tabla.
 * @ver SQLInserta SQLCogeRegistro
 * @cat: SQL
 !*/
int SQLVersionTabla(char *tabla)
{
	char *reg;
	if (!(reg = SQLCogeRegistro(SQL_VERSIONES, tabla, "version")))
		return -1;
	else
		return atoi(reg);
}
