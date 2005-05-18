/*
 * $Id: mysql.c,v 1.10 2005-05-18 18:51:04 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
MYSQL *mysql = NULL;
struct mysql_t mysql_tablas;
int carga_mysql()
{
	if (!(mysql = mysql_init(NULL)))
		return 0;
	if (!mysql_real_connect(mysql, conf_db->host, conf_db->login, conf_db->pass, NULL, conf_db->puerto, NULL, CLIENT_COMPRESS))
		return 0;
	if (mysql_get_server_version(mysql) < 40100)
	{
		fecho(FERR, "Versi�n incorrecta de MySQL. Use almenos la versi�n 4.1.0");
		return -4;
	}
	if (mysql_select_db(mysql, conf_db->bd))
	{
#ifdef _WIN32
		if (MessageBox(hwMain, "La base de datos no existe. �Quieres crearla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (pregunta("La base de datos no existe. �Quieres crearla?") == 1)
#endif
		{
			if (_mysql_query("CREATE DATABASE IF NOT EXISTS %s", conf_db->bd))
			{
				fecho(FERR, "Ha sido imposible crear la base de datos\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -1;
			}
			else
				fecho(FOK, "La base de datos se ha creado con exito");
			/* Esto no deber�a ocurrir nunca */
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
	senyal(SIGN_MYSQL);
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
	pthread_mutex_lock(&mutex);
	if (mysql_query(mysql, buf) < 0)
	{
		fecho(FADV, "MySQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", buf, mysql_errno(mysql), mysql_error(mysql));
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	if ((resultado = mysql_store_result(mysql)))
	{
		if (!mysql_num_rows(resultado))
		{
			mysql_free_result(resultado);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		pthread_mutex_unlock(&mutex);
		return resultado;
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}
char *_mysql_get_registro(char *tabla, char *registro, char *campo)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	static char resultado[BUFSIZE];
	char *reg_corr, *cam_corr = NULL;
	reg_corr = _mysql_escapa(registro);
	if (campo)
	{
		cam_corr = (char *)Malloc(sizeof(char) * (strlen(campo) * 2 + 3));
		cam_corr[0] = '`';
		pthread_mutex_lock(&mutex);
		mysql_real_escape_string(mysql, &cam_corr[1], campo, strlen(campo));
		pthread_mutex_unlock(&mutex);
		strcat(cam_corr, "`");
	}
	res = _mysql_query("SELECT %s from %s%s where item='%s'", cam_corr ? cam_corr : "*", PREFIJO, tabla, reg_corr);
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
	if (BadPtr(row[0]))
		return NULL;
	strncpy(resultado, row[0], sizeof(resultado));
	return resultado;
}
char *_mysql_get_num(MYSQL *mysql, char *tabla, int registro, char *campo)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	char *cam_corr = NULL;
	if (campo)
		cam_corr = _mysql_escapa(campo);
	res = _mysql_query("SELECT %s from %s%s", cam_corr ? cam_corr : "*", PREFIJO, tabla);
	if (campo)
		Free(cam_corr);
	if (!res)
		return NULL;
	for (i = 0; (row = mysql_fetch_row(res)); i++)
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
	reg_c = _mysql_escapa(registro);
	cam_c = _mysql_escapa(campo);
	if (!BadPtr(valor))
		val_c = _mysql_escapa(buf);
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
		reg_c = _mysql_escapa(registro);
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
	char buf[4096], buf2[4096], *tabla, *field;
	if (!(fp = fopen("backup.sql", "wb")))
		return 1;
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (strncmp(mysql_tablas.tabla[i], PREFIJO, strlen(PREFIJO)))
			continue;
		bzero(buf, BUFSIZE);
		tabla = mysql_tablas.tabla[i];
		sprintf_irc(buf, "CREATE TABLE %s ( \n", tabla);
		res = _mysql_query("SHOW FIELDS FROM %s", tabla);
		while ((row = mysql_fetch_row(res)))
		{
			bzero(buf2, BUFSIZE);
			sprintf_irc(buf2, "\t`%s` %s%s%s%s%s%s%s, \n",
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
			sprintf_irc(buf2, "\tKEY `%s` (`%s`), \n",
				row[2],
				row[4]);
			strcat(buf, buf2);
		}
		mysql_free_result(res);
		buf[strlen(buf)-3] = '\0'; /* quitamos la ultima coma */
		strcat(buf, "\n) ");
		res = _mysql_query("SHOW TABLE STATUS FROM %s LIKE '%s'", conf_db->bd, tabla);
		row = mysql_fetch_row(res);
		sprintf_irc(buf2, "ENGINE=%s", _mysql_fetch_array(res, "Engine", row));
		/*if ((field = _mysql_fetch_array(res, "Collation", row)))
		{
			strcat(buf2, " DEFAULT CHARSET=");
			strcat(buf2, field);
		}*/
		if ((field = _mysql_fetch_array(res, "Comment", row)))
		{
			strcat(buf2, " COMMENT='");
			strcat(buf2, field);
			strcat(buf2, "'");
		}
		if ((field = _mysql_fetch_array(res, "Auto_increment", row)) && *field != '0')
		{
			strcat(buf2, " AUTO_INCREMENT=");
			strcat(buf2, field);
		}
		mysql_free_result(res);
		strcat(buf, buf2);
		fprintf(fp, "%s;\n", buf);
		if ((res = _mysql_query("SELECT * from %s", tabla)))
		{
			while ((row = mysql_fetch_row(res)))
			{
				pthread_mutex_lock(&mutex);
				if (!(resf = mysql_list_fields(mysql, tabla, NULL)))
				{
					pthread_mutex_unlock(&mutex);
					break;
				}
				pthread_mutex_unlock(&mutex);
				bzero(buf, BUFSIZE);
				sprintf_irc(buf, "INSERT INTO `%s` VALUES (", tabla);
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
	char *linea, buf[BUFSIZE], buf2[2048];
	int i;
	if (!(fp = fopen("backup.sql", "r")))
		return 1;
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strncmp(PREFIJO, mysql_tablas.tabla[i], strlen(PREFIJO)))
			_mysql_query("DROP table `%s`", mysql_tablas.tabla[i]);
		Free(mysql_tablas.tabla[i]);
	}
	mysql_tablas.tablas = 0;
	buf2[0] = '\0';
	while ((linea = fgets(buf, BUFSIZE, fp)))
	{
		linea[strlen(linea)-1] = '\0';
		strcat(buf2, linea);
		if (linea[strlen(linea)-1] == ';')
		{
			_mysql_query(buf2);
			buf2[0] = '\0';
		}
	}
	fclose(fp);
	_mysql_carga_tablas();
	senyal(SIGN_MYSQL);
	return 0;
}
void _mysql_carga_tablas()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int i;
	mysql_tablas.tablas = 0;
	pthread_mutex_lock(&mutex);
	if ((res = mysql_list_tables(mysql, NULL)))
	{
		pthread_mutex_unlock(&mutex);
		for (i = 0; (row = mysql_fetch_row(res)); i++)
			ircstrdup(mysql_tablas.tabla[i], row[0]);
		mysql_tablas.tablas = i;
		mysql_free_result(res);
	}
	else
		pthread_mutex_unlock(&mutex);
}
int _mysql_existe_tabla(char *tabla)
{
	char buf[256];
	int i;
	sprintf_irc(buf, "%s%s", PREFIJO, tabla);
	for (i = 0; i < mysql_tablas.tablas; i++)
	{
		if (!strcasecmp(mysql_tablas.tabla[i], buf))
			return 1;
	}
	return 0;
}
char *_mysql_escapa(char *item)
{
	char *tmp;
	tmp = (char *)Malloc(sizeof(char) * (strlen(item) * 2 + 1));
	pthread_mutex_lock(&mutex);
	mysql_real_escape_string(mysql, tmp, item, strlen(item));
	pthread_mutex_unlock(&mutex);
	return tmp;
}
char *_mysql_fetch_array(MYSQL_RES *res, const char *campo, MYSQL_ROW row)
{
	MYSQL_FIELD *field;
	int i;
	if (!res || !row || !campo)
		return NULL;
	pthread_mutex_lock(&mutex);
	mysql_field_seek(res, 0);
	for (i = 0; (field = mysql_fetch_field(res)); i++)
	{
		if (!strcasecmp(field->name, campo))
			break;
	}
	pthread_mutex_unlock(&mutex);
	if (!field)
		return NULL;
	return row[i]; 
}
