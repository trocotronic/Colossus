/*
 * $Id: mysql.c,v 1.7 2006-10-31 23:49:12 Trocotronic Exp $ 
 */

#include "struct.h"
#ifdef _WIN32
#include <mysql.h>
#include <winerror.h>
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
#ifdef _WIN32
struct errentry 
{
	unsigned long oscode;           /* OS return value */
	int errnocode;  /* System V error code */
};
static struct errentry errtable[] = {
	{  ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
	{  ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
	{  ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
	{  ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
	{  ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
	{  ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
	{  ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
	{  ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
	{  ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
	{  ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
	{  ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
	{  ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
	{  ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
	{  ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
	{  ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
	{  ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
	{  ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
	{  ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
	{  ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
	{  ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
	{  ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
	{  ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
	{  ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
	{  ERROR_FAIL_I24,               EACCES    },  /* 83 */
	{  ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
	{  ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
	{  ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
	{  ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
	{  ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
	{  ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
	{  ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
	{  ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
	{  ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
	{  ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
	{  ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
	{  ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
	{  ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
	{  ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
	{  ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
	{  ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
	{  ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
	{  ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
	{  ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
	{  ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
	{  ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }    /* 1816 */
};
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN
#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define ERRTABLESIZE (sizeof(errtable)/sizeof(errtable[0]))
void __cdecl _dosmaperr (unsigned long oserrno)
{
	int i;
	_doserrno = oserrno;        /* set _doserrno */
	for (i = 0; i < ERRTABLESIZE; ++i) 
	{
		if (oserrno == errtable[i].oscode) 
		{
			errno = errtable[i].errnocode;
			return;
		}
	}
	if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
		errno = EACCES;
	else if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
		errno = ENOEXEC;
	else
		errno = EINVAL;
}
#endif

int Carga()
{
	char *c;
	//if (mysql_server_init(sizeof(server_args) / sizeof(char *), server_args, server_groups))
	//	return -1;
	if (!(mysql = mysql_init(NULL)))
		return -1;
	if (!mysql_real_connect(mysql, conf_db->host, conf_db->login, conf_db->pass, NULL, conf_db->puerto, NULL, CLIENT_COMPRESS))
	{
		Alerta(FERR, "MySQL no puede conectar\n%s (%i)", mysql_error(mysql), mysql_errno(mysql));
		return -1;
	}
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
	if ((c = strchr(sql->servinfo, '-')))
		*c = '\0';
	if ((c = strchr(sql->clientinfo, '-')))
		*c = '\0';
	CargaTablas();
	return 0;
}
SQLRes Query(const char *query)
{
	MYSQL_RES *resultado;
#ifdef DEBUG
	Debug(query);
#endif
	if (!mysql)
		return NULL;
	pthread_mutex_lock(&mutex);
	if (mysql_query(mysql, query) < 0)
	{
		Alerta(FADV, "SQL ha detectado un error.\n[Backup Buffer: %s]\n[%i: %s]\n", query, mysql_errno(mysql), mysql_error(mysql));
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
		return (SQLRes)resultado;
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}
void FreeRes(SQLRes res)
{
	pthread_mutex_lock(&mutex);
	mysql_free_result((MYSQL_RES *)res);
	pthread_mutex_unlock(&mutex);
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
	pthread_mutex_lock(&mutex);
	mysql_close(mysql);
	mysql = NULL;
	pthread_mutex_unlock(&mutex);
}
void CargaTablas()
{
	MYSQL_RES *res, *cp;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	int i = 0, j;
	char *tabla;
	pthread_mutex_lock(&mutex);
	if ((res = mysql_list_tables(mysql, NULL)))
	{
		pthread_mutex_unlock(&mutex);
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
	else
		pthread_mutex_unlock(&mutex);
}
char *Escapa(const char *item)
{
	char *tmp;
	tmp = (char *)Malloc(sizeof(char) * (strlen(item) * 2 + 1));
	pthread_mutex_lock(&mutex);
	mysql_real_escape_string(mysql, tmp, item, strlen(item));
	pthread_mutex_unlock(&mutex);
	return tmp;
}
