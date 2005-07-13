/*
 * $Id: mysql.c,v 1.1 2005-07-13 14:06:37 Trocotronic Exp $ 
 */

#include "struct.h"
#ifdef _WIN32
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif
#include "ircd.h"

MYSQL *mysql = NULL;
SQLRes Query(const char *);
char *Escapa(const char *);
void FreeRes(SQLRes);
int NumRows(SQLRes);
SQLRow FetchRow(SQLRes);
void Descarga();
void CargaTablas();
int Carga()
{
	if (!(mysql = mysql_init(NULL)))
		return -1;
	if (!mysql_real_connect(mysql, conf_db->host, conf_db->login, conf_db->pass, NULL, conf_db->puerto, NULL, CLIENT_COMPRESS))
		return -1;
	if (mysql_get_server_version(mysql) < 40100)
	{
		Alerta(FERR, "Versión incorrecta de SQL. Use almenos la versión 4.1.0");
		return -2;
	}
	if (mysql_select_db(mysql, conf_db->bd))
	{
#ifdef _WIN32
		if (MessageBox(hwMain, "La base de datos no existe. ¿Quieres crearla?", "MySQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (Pregunta("La base de datos no existe. ¿Quieres crearla?") == 1)
#endif
		{
			char tmp[128];
			ircsprintf(tmp, "CREATE DATABASE IF NOT EXISTS %s", conf_db->bd);
			if (SQLQuery(tmp))
			{
				Alerta(FERR, "Ha sido imposible crear la base de datos\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -3;
			}
			else
				Alerta(FOK, "La base de datos se ha creado con exito");
			/* Esto no debería ocurrir nunca */
			if (mysql_select_db(mysql, conf_db->bd))
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
	sql->recurso = (void *)mysql;
	sql->Query = Query;
	sql->Descarga = Descarga;
	sql->CargaTablas = CargaTablas;
	sql->FetchRow = FetchRow;
	sql->FreeRes = FreeRes;
	sql->NumRows = NumRows;
	sql->Escapa = Escapa;
	ircsprintf(sql->servinfo, "MySQL %s", mysql_get_server_info(mysql));
	ircsprintf(sql->clientinfo, "MySQL %s", mysql_get_client_info());
	CargaTablas();
	return 0;
}
SQLRes Query(const char *query)
{
	MYSQL_RES *resultado;
	Debug(query);
	//pthread_mutex_lock(&mutex);
	if (!mysql)
		return NULL;
	if (mysql_query(mysql, query) < 0)
	{
		Alerta(FADV, "SQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", query, mysql_errno(mysql), mysql_error(mysql));
		//pthread_mutex_unlock(&mutex);
		return NULL;
	}
	if ((resultado = mysql_store_result(mysql)))
	{
		if (!mysql_num_rows(resultado))
		{
			mysql_free_result(resultado);
			//pthread_mutex_unlock(&mutex);
			return NULL;
		}
		//pthread_mutex_unlock(&mutex);
		return (SQLRes)resultado;
	}
	//pthread_mutex_unlock(&mutex);
	return NULL;
}
void FreeRes(SQLRes res)
{
	mysql_free_result((MYSQL_RES *)res);
}
int NumRows(SQLRes res)
{
	return (int)mysql_num_rows((MYSQL_RES *)res);
}
SQLRow FetchRow(SQLRes res)
{
	return (SQLRow)mysql_fetch_row((MYSQL_RES *)res);
}
void Descarga()
{
	mysql_close(mysql);
	mysql = NULL;
}
void CargaTablas()
{
	MYSQL_RES *res, *cp;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	int i = 0, j;
	char *tabla;
	//pthread_mutex_lock(&mutex);
	if ((res = mysql_list_tables(mysql, NULL)))
	{
		//pthread_mutex_unlock(&mutex);
		while ((row = mysql_fetch_row(res)))
		{
			tabla = row[0];
			if (strncmp(PREFIJO, tabla, strlen(PREFIJO)))
				continue;
			ircstrdup(sql->tablas[i][0], tabla);
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
	//else
		//pthread_mutex_unlock(&mutex);
}
char *Escapa(const char *item)
{
	char *tmp;
	tmp = (char *)Malloc(sizeof(char) * (strlen(item) * 2 + 1));
	//pthread_mutex_lock(&mutex);
	mysql_real_escape_string(mysql, tmp, item, strlen(item));
	//pthread_mutex_unlock(&mutex);
	return tmp;
}
char *SQLFetchArray(MYSQL_RES *res, const char *campo, MYSQL_ROW row)
{
	MYSQL_FIELD *field;
	int i;
	if (!res || !row || !campo)
		return NULL;
	//pthread_mutex_lock(&mutex);
	mysql_field_seek(res, 0);
	for (i = 0; (field = mysql_fetch_field(res)); i++)
	{
		if (!strcasecmp(field->name, campo))
			break;
	}
	//pthread_mutex_unlock(&mutex);
	if (!field)
		return NULL;
	return row[i]; 
}
