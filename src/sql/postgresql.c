/*
 * $Id: postgresql.c,v 1.4 2005-07-16 15:25:35 Trocotronic Exp $ 
 */

#include "struct.h"
#include <libpq-fe.h>
#include "ircd.h"

PGconn *postgres = NULL;
SQLRes Query(const char *);
char *Escapa(const char *);
void FreeRes(SQLRes);
int NumRows(SQLRes);
SQLRow FetchRow(SQLRes);
void Descarga();
void CargaTablas();
int fila = 0;
int Carga()
{
	char port[6];
	u_long version;
	ircsprintf(port, "%i", conf_db->puerto ? conf_db->puerto : 5432);
#ifdef _WIN32
	reintenta:
#endif
	if (!(postgres = PQsetdbLogin(conf_db->host, port, NULL, NULL, conf_db->bd, conf_db->login, conf_db->pass)) || PQstatus(postgres) != CONNECTION_OK)
	{
		char *msg = PQerrorMessage(postgres);
#ifdef _WIN32
		if (!strncmp("FATAL:  database \"", msg, 17))
		{
			char *user;
			user = strdup(PreguntaCampo("¿Usuario?", "Introduce el nombre del superusuario", NULL));
			if (!(postgres = PQsetdbLogin(conf_db->host, port, NULL, NULL, "template1", user, PreguntaCampo("¿Pass?", "Introduce la contraseña del superusuario", NULL))) || PQstatus(postgres) != CONNECTION_OK)
			{
				Free(user);
				Alerta(FADV, "Ha sido imposible crear la base de datos PostGreSQL.\n[%s]\n", PQerrorMessage(postgres));
				return -6;
			}
			Free(user);
			ircsprintf(buf, "CREATE DATABASE %s WITH ENCODING='LATIN1' OWNER=%s", conf_db->bd, conf_db->login);
			PQexec(postgres, buf);
			PQfinish(postgres);
			goto reintenta;
		}
		else
#endif
			Alerta(FADV, "Ha sido imposible conectar PostGreSQL.\n[%s]\n", msg);
		return -1;
	}
	if ((version = PQserverVersion(postgres)) < 8000)
	{
		Alerta(FERR, "Versión incorrecta de SQL (%lu). Use almenos la versión 8.0", version);
		return -4;
	}
	//for (i = 0; i < max; i++)
	//{
	//	if (!strcasecmp(
/*	if (mysql_select_db(mysql, conf_db->bd))
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
				return -1;
			}
			else
				Alerta(FOK, "La base de datos se ha creado con exito");
	
			if (mysql_select_db(mysql, conf_db->bd))
			{
				Alerta(FERR, "Error fatal\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
				return -2;
			}
		}
		else
		{
			Alerta(FERR, "Para utilizar los servicios es necesario una base de datos");
			return -3;
		}
	}*/
	sql->recurso = (void *)postgres;
	sql->Query = Query;
	sql->Descarga = Descarga;
	sql->CargaTablas = CargaTablas;
	sql->FetchRow = FetchRow;
	sql->FreeRes = FreeRes;
	sql->NumRows = NumRows;
	sql->Escapa = Escapa;
	PQsetClientEncoding(postgres, "LATIN1");
	ircsprintf(sql->servinfo, "PostGreSQL %s", PQparameterStatus(postgres, "server_version"));
	CargaTablas();
	return 0;
}
SQLRes Query(const char *query)
{
	PGresult *resultado;
#ifndef DEBUG
	Debug(query);
#endif
	if (!postgres)
		return NULL;
	if (!(resultado = PQexec(postgres, query)))
	{
		Alerta(FADV, "SQL ha detectado un error.\n[Backup Buffer: %s]\n[%s]\n", query, PQerrorMessage(postgres));
		return NULL;
	}
	switch(PQresultStatus(resultado))
	{
		case PGRES_TUPLES_OK:
			if (PQntuples(resultado) > 0)
			{
				fila = 0; /* preparamos para la primera fila */
				return (SQLRes)resultado;
			}
			break;
		case PGRES_BAD_RESPONSE:
			Info("Sintaxis SQL incorrecta.\n[%s]", query);
			break;
		case PGRES_FATAL_ERROR:
			Alerta(FADV, "SQL ha detectado un error.\n[Backup Buffer: %s]\n[%s]\n", query, PQerrorMessage(postgres));
			break;
	}
	PQclear(resultado);
	return NULL;
}
void FreeRes(SQLRes res)
{
	PQclear((PGresult *)res);
}
int NumRows(SQLRes res)
{
	return (int)PQntuples((PGresult *)res);
}
SQLRow FetchRow(SQLRes res)
{
	static char *row[512]; /* espero que haya suficiente */
	int i, max = PQnfields((PGresult *)res), filas = PQntuples((PGresult *)res);
	if (filas == fila)
		return NULL;
	for (i = 0; i < max; i++)
		row[i] = PQgetvalue((PGresult *)res, fila, i);
	row[i] = NULL;
	fila++;
	return (SQLRow)row;
}
void Descarga()
{
	//PQexec(postgres, "END");
	PQfinish(postgres);
	postgres = NULL;
}
void CargaTablas()
{
	PGresult *resultado, *cp;
	int i, max, j, campos;
	char *tabla;
	resultado = PQexec(postgres, "select relname from pg_stat_user_tables order by relname");
	max = PQntuples(resultado);
	for (i = 0; i < max; i++)
	{
		tabla = PQgetvalue(resultado, i, 0);
		if (strncmp(PREFIJO, tabla, strlen(PREFIJO)))
			continue;
		ircstrdup(sql->tablas[i][0], tabla);
		ircsprintf(buf, "SELECT * FROM %s", tabla);
		cp = PQexec(postgres, buf);
		campos = PQnfields(cp);
		for (j = 0; j < campos; j++)
			ircstrdup(sql->tablas[i][j+1], PQfname(cp, j));
		sql->tablas[i][j+1] = NULL;
		PQclear(cp);
	}
	sql->tablas[i][0] = NULL;
	PQclear(resultado);
}
char *Escapa(const char *item)
{
	char *tmp;
	tmp = (char *)Malloc(sizeof(char) * (strlen(item) * 2 + 1));
	PQescapeString(tmp, item, strlen(item));
	return tmp;
}
