/*
 * $Id: sql.c,v 1.21 2008/06/01 22:25:37 Trocotronic Exp $
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
#include <time.h>

SQL sql = NULL;
MYSQL *mysql = NULL;
void SetSQLErrno();
SQLRes MySQLQuery(const char *);
char *server_args[] = {
	"colossus",       /* this string is not used */
	"--defaults-file=./database/mysql/my.ini"
};
char *server_groups[] = {
	"embedded",
	"server",
	"colossus",
	(char *)NULL
};
void LiberaSQL()
{
	int i, j;
	if (!sql)
		return;
	for (i = 0; i < sql->tablas; i++)
	{
		for (j = 0; j < sql->tabla[i].campos; j++)
			ircfree(sql->tabla[i].campo[j]);
		ircfree(sql->tabla[i].tabla);
		ircfree(sql->tabla[i].create);
	}
	if (conf_db)
		mysql_close(mysql);
	//mysql_library_end(); //si lo descomento, casca al refrescar (wtf?)
	ircfree(sql);
}
int CargaSQL()
{
	char *c, *host = NULL, *user = NULL, *pass = NULL, *bdd = "colossus";
	my_bool rec = 1;
	int puerto = 0;
	FILE *fp;
	if (!conf_db)
		mkdir("database/mysql/data",0700);
	if (sql)
		LiberaSQL();
	sql = BMalloc(struct _sql);
	if (mysql_library_init(sizeof(server_args) / sizeof(char *), server_args, server_groups))
	{
		Error("Ha sido imposible cargar el motor MySQL [library_init]");
		return -1;
	}
	if (!(mysql = mysql_init(NULL)))
	{
		Error("Ha sido imposible cargar el motor MySQL [init]");
		return -1;
	}
	if (!conf_db)
	{
		mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "libmysqld_client");
		mysql_options(mysql, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);
	}
	else
	{
		host = conf_db->host;
		user = conf_db->login;
		pass = conf_db->pass;
		bdd = conf_db->bd;
		puerto = conf_db->puerto;
		mysql_options(mysql, MYSQL_OPT_USE_REMOTE_CONNECTION, NULL);
	}
	if (!mysql_real_connect(mysql, host, user, pass, NULL, puerto, NULL, 0))
	{
		Alerta(FERR, "MySQL no puede conectar\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
		return -1;
	}
	if (conf_db && mysql_get_server_version(mysql) < 40100)
	{
		Alerta(FERR, "Versión incorrecta de SQL. Use almenos la versión 4.1.0");
		return -2;
	}
	if (mysql_select_db(mysql, bdd))
	{
		SQLQuery("CREATE DATABASE IF NOT EXISTS colossus CHARACTER SET utf8");
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
#if MYSQL_VERSION_ID >= 50013
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &rec);
#endif
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
			for (i = 0; i < sql->tablas; i++)
				SQLQuery("DROP TABLE %s", sql->tabla[i].tabla);
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
 * @ret: Devuelve su índice si existe; -1, si no.
 * @cat: SQL
 !*/

int SQLEsTabla(char *tabla)
{
	int i;
	if (sql)
	{
		for (i = 0; i < sql->tablas; i++)
		{
			if (!strcasecmp(sql->tabla[i].tabla, tabla))
				return i;
		}
	}
	return -1;
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
	int i, j;
	if (sql)
	{
		if ((i = SQLEsTabla(tabla)) != -1)
		{
			for (j = 0; j < sql->tabla[i].campos; j++)
			{
				if (!strcasecmp(sql->tabla[i].campo[j], campo))
					return 1;
			}
			return 0;
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
	SQLRes res = NULL;
	if (sql)
	{
		va_list vl;
		va_start(vl, query);
		res = SQLQueryVL(query, vl);
		va_end(vl);
	}
	return res;
}

SQLRes SQLQueryVL(const char *query, va_list vl)
{
	SQLRes res = NULL;
	char buf[8192]; //presupongo que un query no superará los 8KB (son muchos KB)
	ircvsprintf(buf, query, vl);
#ifdef DEBUG
	Debug("SQL Query: %s", buf);
#endif
	res = MySQLQuery(buf);
	SetSQLErrno();
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
		MYSQL_RES *res;
		MYSQL_ROW row;
		char *tabla;
		if ((res = mysql_list_tables(mysql, NULL)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				tabla = row[0];
				if (strncmp(PREFIJO, tabla, strlen(PREFIJO)))
					continue;
				if (SQLEsTabla(&tabla[strlen(PREFIJO)]) == -1)
				{
					ircstrdup(sql->tabla[sql->tablas++].tabla, &tabla[strlen(PREFIJO)]);
					SQLRefrescaTabla(&tabla[strlen(PREFIJO)]);
				}
			}
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
	{
		sql->_errno = mysql_errno(mysql);
		sql->_errstr = mysql_error(mysql);
	}
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
 * @ver: SQLInserta SQLCogeRegistro
 * @cat: SQL
 * @ret: Devuelve la versión de la tabla.
 !*/
int SQLVersionTabla(char *tabla)
{
	char *reg;
	if (!(reg = SQLCogeRegistro(SQL_VERSIONES, tabla, "version")))
		return -1;
	else
		return atoi(reg);
}

/*!
 * @desc: Añade una nueva tabla.
 * @params: $query [in] Sintaxis del comando CREATE TABLE. Cadena con formato.
 		$... [in] Argumentos variables según cadena con formato.
 * @ver: SQLQuery
 * @cat: SQL
 * @ret: Devuelve 0 si no ha habido ningún fallo o distinto a 0 si ha ocurrido un fallo.
 !*/
int SQLNuevaTabla(char *tabla, char *str, ...)
{
	int ret = -1, i;
	if (sql->tablas == MAX_TAB)
		return -1;
	if (sql)
	{
		char buf[8192];
		va_list vl;
		va_start(vl, str);
		sql->_errno = 0;
		if ((i = SQLEsTabla(tabla)) == -1)
		{
			SQLRes res = NULL;
			res = SQLQueryVL(str, vl);
			if ((ret = sql->_errno))
			{
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, tabla);
				return sql->_errno;
			}
			SQLInserta(SQL_VERSIONES, tabla, "version", "1");
			i = sql->tablas++;
			ircstrdup(sql->tabla[i].tabla, tabla);
		}
		else
			ret = -2;
		sql->tabla[i].cargada = 1;
		ircvsprintf(buf, str, vl);
		va_end(vl);
		sql->tabla[i].create = strdup(buf);
		SQLRefrescaTabla(tabla);
	}
	return ret;
}

void SQLRefrescaTabla(char *tabla)
{
	MYSQL_RES *cp;
	MYSQL_FIELD *field;
	int i, j, c = 0;
	retry:
	ircsprintf(buf, "SELECT * FROM %s%s", PREFIJO, tabla);
	if (mysql_query(mysql, buf) || !(cp = mysql_store_result(mysql)))
	{
		if (mysql_errno(mysql) == 1035 && !c)
		{
			c = 1;
			ircsprintf(buf, "REPAIR TABLE %s", tabla);
			if (!mysql_query(mysql, buf))
				goto retry;
			else
			{
				Alerta(FADV, "SQL ha intentado reparar la tabla %s sin éxito.\n[%i: %s]\n", tabla, mysql_errno(mysql), mysql_error(mysql));
				return;
			}
		}
		else
		{
			Alerta(FADV, "SQL ha detectado un error durante la carga de la tabla %s.\n[%i: %s]\n", tabla, mysql_errno(mysql), mysql_error(mysql));
			return;
		}
	}
	if ((i = SQLEsTabla(tabla)) != -1)
	{
		for (j = 0; (field = mysql_fetch_field(cp)); j++)
			ircstrdup(sql->tabla[i].campo[j], field->name);
		sql->tabla[i].campos = j;
	}
	mysql_free_result(cp);
}

/*!
 * @desc: Crea una copia de seguridad de toda la base de datos. Sólo se hará la copia de las tablas creadas a partir de SQLNuevaTabla.
 * @params: $tag [in] Tag o etiqueta con el que se nombrará el archivo al que se volcarán los datos. Si existe, se eliminará. Si es NULL, se usará la fecha actual.
 * @ver: SQLNuevaTabla SQLImportar
 * @cat: SQL
 * @ret: Devuelve 0 si todo ha ido bien; distinto de 0 si ha ocurrido un error.
 !*/
int SQLDump(char *tag)
{
	FILE *fp;
	int i, j;
	SQLRes res;
	SQLRow row;
	mkdir(SQL_BCK_DIR, 700);
	if (!tag)
	{
		struct tm *tt;
		time_t t = time(0);
		tt = localtime(&t);
		sprintf(buf, SQL_BCK_DIR "/backup-%04lu%02lu%02lu%02lu%02lu%02lu.sql", (u_long)(tt->tm_year+1900), (u_long)tt->tm_mon, (u_long)tt->tm_mday, (u_long)tt->tm_hour, (u_long)tt->tm_min, (u_long)tt->tm_sec);
	}
	else
		sprintf(buf, SQL_BCK_DIR "/backup-%s.sql", tag);
	if (!(fp = fopen(buf, "w")))
		return 1;
	for (i = 0; i < sql->tablas; i++)
	{
		if (sql->tabla[i].cargada)
		{
			fputs(sql->tabla[i].create, fp);
			fputs("\n", fp);
		}
	}
	for (i = 0; i < sql->tablas; i++)
	{
		if (sql->tabla[i].cargada && (res = SQLQuery("SELECT * FROM %s%s", PREFIJO, sql->tabla[i].tabla)))
		{
			while ((row = SQLFetchRow(res)))
			{
				fprintf(fp, "INSERT INTO `%s%s` VALUES (", PREFIJO, sql->tabla[i].tabla);
				for (j = 0; j < sql->tabla[i].campos; j++)
				{
					if (!row[j])
						fputs("NULL", fp);
					else
					{
						ircsprintf(tokbuf, "%li", atol(row[j]));
						if (!strcmp(tokbuf, row[j])) /* es numero */
							fputs(tokbuf, fp);
						else
						{
							char *q = SQLEscapa(row[j]);
							fprintf(fp, "'%s'", q);
							Free(q);
						}
					}
					if (j < sql->tabla[i].campos-1)
						fputs(",", fp);
				}
				fputs(");\n", fp);
			}
			SQLFreeRes(res);
		}
	}
	fclose(fp);
	return 0;
}
/*!
 * @desc: Restaura una copia de seguridad a partir de una etiqueta.
 * @params: $tag [in] Tag o etiqueta del archivo a importar.
 * @ver: SQLNuevaTabla SQLDump
 * @cat: SQL
 * @ret: Devuelve 0 si todo ha ido bien; 1 si no puede abrir el archivo.
 !*/
int SQLImportar(char *tag)
{
	FILE *fp;
	int i;
	sprintf(buf, SQL_BCK_DIR "/backup-%s.sql", tag);
	if (!(fp = fopen(buf, "r")))
		return 1;
	for (i = 0; i < sql->tablas; i++)
		SQLQuery("DROP TABLE %s", sql->tabla[i].tabla);
	while (fgets(buf, sizeof(buf), fp))
		SQLQuery(buf);
	fclose(fp);
	return 0;
}
