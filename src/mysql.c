#include "struct.h"
#include "ircd.h"
struct mysql_t mysql_tablas;
int carga_mysql()
{
	if (!(mysql = mysql_init(NULL)))
		return 0;
	if (!mysql_real_connect(mysql, conf_db->host, conf_db->login, conf_db->pass, NULL, conf_db->puerto, NULL, CLIENT_COMPRESS))
		return 0;
	if (mysql_select_db(mysql, conf_db->bd))
	{
#ifdef _WIN32
		if (MessageBox(NULL, "La base de datos no existe. ¿Quieres crearla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (pregunta("La base de datos no existe. ¿Quieres crearla?") == 1)
#endif
		{
			if (_mysql_query("CREATE DATABASE IF NOT EXISTS %s", conf_db->bd))
			{
				fecho(FERR, "Ha sido imposible crear la base de datos\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -1;
			}
			else
				fecho(FOK, "La base de datos se ha creado con exito");
			/* Esto no debería ocurrir nunca */
			if (mysql_select_db(mysql, conf_db->bd))
			{
				fecho(FERR, "Error fatal\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -2;
			}
		}
		else
		{
			fecho(FERR, "Para utilizar los servicios es necesario una base de datos");
			return -3;
		}
	}
	_mysql_carga_tablas();
	signal(SIGN_MYSQL);
	return 1;
}
MYSQL_RES *_mysql_query(char *formato, ...)
{
	va_list vl;
	char buf[BUFSIZE];
	MYSQL_RES *resultado;
	va_start(vl, formato);
	vsprintf_irc(buf, formato, vl);
	va_end(vl);
	if (mysql_query(mysql, buf) < 0)
	{
		fecho(FADV, "MySQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", buf, mysql_errno(mysql), mysql_error(mysql));
		return (MYSQL_RES *)mysql_error(mysql);
	}
	if ((resultado = mysql_store_result(mysql)))
	{
		if (!mysql_num_rows(resultado))
		{
			mysql_free_result(resultado);
			return NULL;
		}
		return resultado;
	}
	return NULL;
}
char *_mysql_get_registro(char *tabla, char *registro, char *campo)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *reg_corr, *cam_corr = NULL;
	reg_corr = (char *)Malloc(sizeof(char) * strlen(registro) * 2 + 1);
	mysql_real_escape_string(mysql, reg_corr, registro, strlen(registro));
	if (campo)
	{
		cam_corr = (char *)Malloc(sizeof(char) * strlen(campo) * 2 + 1);
		mysql_real_escape_string(mysql, cam_corr, campo, strlen(campo));
	}
	res = _mysql_query("SELECT `%s` from %s%s where item='%s'", cam_corr ? cam_corr : "*", PREFIJO, tabla, reg_corr);
	Free(reg_corr);
	if (campo)
		Free(cam_corr);
	if (!res)
		return NULL;
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	if (!res)
		return NULL;
	if (!campo)
		return (u_char *)!NULL;
	return BadPtr(row[0]) ? NULL : row[0];
}
char *_mysql_get_num(MYSQL *mysql, char *tabla, int registro, char *campo)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	char *cam_corr = NULL;
	if (campo)
	{
		cam_corr = (char *)Malloc(sizeof(char) * strlen(campo) * 2 + 1);
		mysql_real_escape_string(mysql, cam_corr, campo, strlen(campo));
	}
	res = _mysql_query("SELECT %s from %s%s", cam_corr ? cam_corr : "*", PREFIJO, tabla);
	if (campo)
		Free(cam_corr);
	if (!res)
		return NULL;
	for (i = 0; row = mysql_fetch_row(res); i++)
	{
		mysql_free_result(res);
		if (i == registro)
			return row[0];
	}
	return NULL;
}
void _mysql_add(char *tabla, char *registro, char *campo, char *valor, ...)
{
	char *reg_c, *cam_c, *val_c = NULL;
	char buf[BUFSIZE];
	va_list vl;
	if (!BadPtr(valor))
	{
		va_start(vl, valor);
		vsprintf_irc(buf, valor, vl);
		va_end(vl);
	}
	reg_c = (char *)Malloc(sizeof(char) * strlen(registro) * 2 + 1);
	cam_c = (char *)Malloc(sizeof(char) * strlen(campo) * 2 + 1);
	if (!BadPtr(valor))
		val_c = (char *)Malloc(sizeof(char) * strlen(buf) * 2 + 1);
	mysql_real_escape_string(mysql, reg_c, registro, strlen(registro));
	mysql_real_escape_string(mysql, cam_c, campo, strlen(campo));
	if (!BadPtr(valor))
		mysql_real_escape_string(mysql, val_c, buf, strlen(buf));
	if (!_mysql_get_registro(tabla, registro, NULL))
		_mysql_query("INSERT INTO %s%s (item) values ('%s')", PREFIJO, tabla, reg_c);
	_mysql_query("UPDATE %s%s SET `%s`='%s' where item='%s'", PREFIJO, tabla, cam_c, val_c ? val_c : "", reg_c);
	Free(reg_c);
	Free(cam_c);
	ircfree(val_c);
}
void _mysql_del(char *tabla, char *registro)
{
	if (registro)
	{
		char *reg_c;
		reg_c = (char *)Malloc(sizeof(char) * strlen(registro) * 2 + 1);
		mysql_real_escape_string(mysql, reg_c, registro, strlen(registro));
		if (_mysql_get_registro(tabla, registro, NULL))
			_mysql_query("DELETE from %s%s where item='%s'", PREFIJO, tabla, reg_c);
		Free(reg_c);
	}
	else
		_mysql_query("DELETE from %s%s", PREFIJO, tabla);
}
int _mysql_backup()
{
	MYSQL_RES *res, *resf;
	MYSQL_ROW row;
	MYSQL_FIELD *fi;
	FILE *fp;
	int i, j;
	char buf[BUFSIZE], buf2[BUFSIZE], *tabla;
	if (!(fp = fopen("backup.sql", "wb")))
		return 1;
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		bzero(buf, BUFSIZE);
		tabla = mysql_tablas.tabla[i];
		sprintf_irc(buf, "CREATE TABLE %s ( ", tabla);
		res = _mysql_query("SHOW FIELDS FROM %s", tabla);
		while ((row = mysql_fetch_row(res)))
		{
			bzero(buf2, BUFSIZE);
			sprintf_irc(buf2, "`%s` %s%s%s%s%s%s%s, ",
				row[0], 
				row[1], 
				BadPtr(row[2]) ? " NOT NULL" : "",
				BadPtr(row[4]) ? "" : " default '",
				BadPtr(row[4]) ? "" : row[4],
				BadPtr(row[4]) ? "" : "'",
				BadPtr(row[5]) ? "" : " ",
				BadPtr(row[5]) ? "" : row[5]);
			strcat(buf, buf2);
		}
		mysql_free_result(res);
		/* todas las tablas son KEY x (x) */
		res = _mysql_query("SHOW KEYS FROM %s", tabla);
		while ((row = mysql_fetch_row(res)))
		{
			bzero(buf2, BUFSIZE);
			sprintf_irc(buf2, "KEY `%s` (`%s`),",
				row[2],
				row[4]);
			strcat(buf, buf2);
		}
		mysql_free_result(res);
		buf[strlen(buf)-1] = '\0'; /* quitamos la ultima coma */
		strcat(buf, ") ");
		res = _mysql_query("SHOW TABLE STATUS FROM %s LIKE '%s'", conf_db->bd, tabla);
		row = mysql_fetch_row(res);
		mysql_free_result(res);
		bzero(buf2, BUFSIZE);
		sprintf_irc(buf2, "TYPE=%s%s%s%s",
			row[1],
			BadPtr(row[14]) ? "" : " COMMENT='",
			BadPtr(row[14]) ? "" : row[14],
			BadPtr(row[14]) ? "" : "'");
		strcat(buf, buf2);
		fprintf(fp, "%s;\n", buf);
		if ((res = _mysql_query("SELECT * from %s", tabla)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				bzero(buf, BUFSIZE);
				sprintf_irc(buf, "INSERT INTO `%s` VALUES (", tabla);
				resf = mysql_list_fields(mysql, tabla, NULL);
				for (j = 0; (fi = mysql_fetch_field(resf)); j++)
				{
					bzero(buf2, BUFSIZE);
					if (fi->type == FIELD_TYPE_VAR_STRING || fi->type == FIELD_TYPE_STRING || fi->type == FIELD_TYPE_BLOB)
						sprintf_irc(buf2, "%s%s%s, ", 
						!row[j] ? "" : "'",
						!row[j] ? "NULL" : row[j],
						!row[j] ? "" : "'");
					else
						sprintf_irc(buf2, "%s, ", !row[j] ? "NULL" : row[j]);
					strcat(buf, buf2);
				}
				buf[strlen(buf)-2] = '\0'; /* quitamos la ultima coma y espacio */
				strcat(buf, ")");
				fprintf(fp, "%s;\n", buf);
				mysql_free_result(resf);
			}
			mysql_free_result(res);
		}
	}
	fclose(fp);
	return 0;
}
int _mysql_restaura()
{
	FILE *fp;
	char *linea, buf[BUFSIZE];
	int i;
	if (!(fp = fopen("backup.sql", "r")))
		return 1;
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		_mysql_query("DROP table `%s`", mysql_tablas.tabla[i]);
		Free(mysql_tablas.tabla[i]);
	}
	mysql_tablas.tablas = 0;
	while ((linea = fgets(buf, BUFSIZE, fp)))
	{
		linea[strlen(linea)-1] = '\0';
		_mysql_query(linea);
	}
	fclose(fp);
	_mysql_carga_tablas();
	signal(SIGN_MYSQL);
	return 0;
}
void _mysql_carga_tablas()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	mysql_tablas.tablas = 0;
	if ((res = mysql_list_tables(mysql, NULL)))
	{
		for (i = 0; (row = mysql_fetch_row(res)); i++)
			mysql_tablas.tabla[i] = strdup(row[0]);
		mysql_tablas.tablas = i;
		mysql_free_result(res);
	}
}
