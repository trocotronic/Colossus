#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define unlink _unlink
#define getcwd _getcwd
#define PMAX MAX_PATH
#else
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#define PMAX PATH_MAX
#endif

void Error(char *e)
{
	printf(e);
	getchar();
	exit(-1);
}
char *Dato(char *c)
{
	char *d;
	if ((d = strchr(c, '\r')))
		*d = 0;
	if ((d = strchr(c, '\n')))
		*d = 0;
	while (*c == ' ')
		c++;
	if (*c == '"')
		c++;
	if (c[strlen(c)-1] == ';')
		c[strlen(c)-1] = '\0';
	if (c[strlen(c)-1] == '"')
		c[strlen(c)-1] = '\0';
	return c;
}
int copyfile(char *src, char *dst)
{
#ifdef _WIN32
	CopyFile(src, dst, 0);
#else
	char tmp[PMAX*2+16];
	sprintf(tmp, "cp %s %s", src, dst);
	system(tmp);
#endif
	return 0;
}
int main()
{
	FILE *fw, *fp;
	int mysql = -1;
	char tmp[512], tmp2[512];
	char *host = NULL;
	char *pass = NULL;
	char *user = NULL;
	char *db = NULL;
	char *pref = NULL;
	char *puerto = strdup("3306");
	char *c, *d, p, *f;
	int plen = 0;
	char *udbfs[] = { "set", "nicks", "canales", "ips", "links", "lines" , NULL };
	char spath[PMAX+1];
#ifdef _WIN32
	WIN32_FIND_DATA wFind;
	HANDLE dir;
#else
	DIR *dir;
	struct dirent *dir_entry;
#endif
	printf("==========================================================================\n");
	printf("======= Herramienta para actualizar al nuevo sistema Colossus 1.11 =======\n");
	printf("==========================================================================\n\n");
	printf("Bienvenido a la utilidad para actualizar su base de datos al nuevo sistema.\n\n");
	printf("***** FASE 1: Actualizacion del database de UDB *****\n\n");
#ifdef _WIN32
	getcwd(spath, sizeof(spath));
	if ((c = strstr(spath, "\\utils")))
#else
	if ((c = strstr(spath, "/utils")))
#endif
		*c = 0;
#ifdef _WIN32
	sprintf(tmp, "%s\\database\\*", spath);
	if ((dir = FindFirstFile(tmp, &wFind)) != INVALID_HANDLE_VALUE)
#else
	sprintf(tmp, "./database");
	if ((dir = opendir(tmp)))
#endif
	{
#ifdef _WIN32
		FindClose(dir);
#else
		closedir(dir);
#endif
		mkdir("../database/udb",0744);
		for (plen = 0; udbfs[plen]; plen++)
		{
			printf("Moviendo %s...\n", udbfs[plen]);
			sprintf(tmp, "../database/%s.udb", udbfs[plen]);
			sprintf(tmp2, "../database/udb/%s.udb", udbfs[plen]);
			copyfile(tmp, tmp2);
			unlink(tmp);
		}
#ifdef _WIN32
		sprintf(tmp, "%s\\database\\backup\\*", spath);
		if ((dir = FindFirstFile(tmp, &wFind)) != INVALID_HANDLE_VALUE)
#else
		sprintf(tmp, "./database/backup");
		if ((dir = opendir(tmp)))
#endif
		{
			mkdir("../database/udb/backup", 0744);
#ifdef _WIN32
			do
			{
				f = wFind.cFileName;
#else
			while ((dir_entry = readdir(dir)) != NULL)
			{
				f = dir_entry->d_name;
#endif
				if (strcmp(f, ".") && strcmp(f, ".."))
				{
					printf("Moviendo %s...\n", f);
					sprintf(tmp, "../database/backup/%s", f);
					sprintf(tmp2, "../database/udb/backup/%s", f);
					copyfile(tmp, tmp2);
					unlink(tmp);
				}
#ifdef _WIN32
			}while (FindNextFile(dir, &wFind));
			FindClose(dir);
			RemoveDirectory("../database/backup");
#else
			}
			closedir(dir);
			system("rm -rf ../database/backup");
#endif
		}
	}
	printf("\n***** FASE 1: COMPLETADA *****\n\n");
	printf("***** FASE 2: Actualizacion del motor MySQL *****\n\n");
	printf("A continuacion se procedera a utilizar los datos utilizados en colossus.conf para actualizarlos.\n\n");
	printf("Pulse una tecla para continuar...\n");
	getchar();
	printf("Detectando archivo colossus.conf... ");
	if (!(fp = fopen("../colossus.conf","r")))
	{
		printf("FALLA\n");
		Error("No se encuentra el archivo colossus.conf\nAbortando...");
	}
	else
		printf("OK\n");
	printf("Escaneando configuracion...\n");
	printf("Buscando bloque db { }...");
	fw = fopen("colossus.tmp", "w");
	while (fgets(tmp, sizeof(tmp), fp))
	{
		c = tmp;
		if (mysql == 1)
		{
			if ((d = strstr(c, "host")))
			{
				host = strdup(Dato(d+4));
				printf("Parametro host encontrado...\n");
			}
			else if ((d = strstr(c, "puerto")))
			{
				puerto = strdup(Dato(d+6));
				printf("Parametro puerto encontrado...\n");
			}
			else if ((d = strstr(c, "login")))
			{
				user = strdup(Dato(d+5));
				printf("Parametro login encontrado...\n");
			}
			else if ((d = strstr(c, "pass")))
			{
				pass = strdup(Dato(d+4));
				printf("Parametro pass encontrado...\n");
			}
			else if ((d = strstr(c, "nombre")))
			{
				db = strdup(Dato(d+6));
				printf("Parametro nombre encontrado...\n");
			}
			else if ((d = strstr(c, "prefijo")))
			{
				pref = strdup(Dato(d+7));
				printf("Parametro prefijo encontrado...\n");
			}
			else if (*c == '}')
			{
				mysql--;
				continue;
			}
		}
		else if (strstr(tmp, "mysql.dll"))
		{
			printf("OK\n");
			mysql = 1;
		}
		if (mysql <= 0)
			fwrite(tmp, 1, strlen(tmp), fw);
	}
	fclose(fp);
	fclose(fw);
	if (mysql == -1)
		Error("FALLA\nNo se encuentra bloque db { }");
	plen = strlen(pref);
	printf("Escaneado finalizado\n\nSe utilizaran los siguientes datos:\n\n");
	printf("host = %s\n", host);
	printf("puerto = %s\n", puerto);
	printf("user = %s\n", user);
	printf("pass = %s\n", pass);
	printf("db_name = %s\n", db);
	printf("prefijo = %s\n\n", pref);
	printf("Son correctos? [S/n]\n");
	fgets(tmp, sizeof(tmp), stdin);
	if (tmp[0] == 'S' || tmp[0] == 's' || tmp[0] == '\0')
	{
		char buf[8092];
		char params[128];
		buf[0] = '\0';
		//mysqldump -h %server% --user=%user% --password=%pass% --port=%port% %cdb%
		sprintf(params, "--host=%s --user=%s --password=%s --port=%s", host, user, pass, puerto);
		sprintf(tmp, "mysql %s --execute=\"SHOW TABLES\" %s > out.txt", params, db);
		system(tmp);
		if ((fp = fopen("out.txt", "r")))
		{
			while (fgets(tmp, sizeof(tmp), fp))
			{
				if (!strncmp(tmp, pref, plen))
				{
					strcat(buf, Dato(tmp));
					strcat(buf, " ");
				}
			}
			fclose(fp);
			unlink("out.txt");
			printf("Se actualizaran las siguientes tablas:\n");
			printf("%s\n\n", buf);
		}
		printf("Ejecutando migracion. Espere... ");
		sprintf(tmp, "mysqldump %s --skip-opt --compact --result-file=tmp.sql %s %s", params, db, buf);
		system(tmp);
		printf("OK\n\n");
		printf("Empaquetando estructura... ");
		if ((fw = fopen("tmp2.sql", "w")))
		{
			if ((fp = fopen("tmp.sql", "r")))
			{
				while (fgets(buf, sizeof(buf), fp))
				{
					plen = strlen(buf);
					if (buf[plen-(buf[plen-2] == '\r' ? 3 : 2)] != ';')
					{
						if (buf[plen-1] == '\n')
							buf[plen-1] = 0;
						if (buf[plen-1] == '\r')
							buf[plen-1] = 0;
					}
					fwrite(buf, 1, strlen(buf), fw);
				}
				fclose(fp);
				unlink("tmp.sql");
				printf("OK\n\n");
			}
			else
				Error("FALLA\nNo se puede empaquetar. Abortando...");
			fclose(fw);
		}
		else
			Error("FALLA\nNo se puede empaquetar. Abortando...");
		if ((fw = fopen("prev-db[110].sql", "w")))
		{
			if ((fp = fopen("tmp2.sql", "r")))
			{
				plen = strlen(pref);
				while (fgets(buf, sizeof(buf), fp))
				{
					sprintf(tmp, "`%s", pref);
					if ((c = strstr(buf, tmp)))
					{
						d = c+plen+1;
						*c = 0;
						fwrite(buf, 1, strlen(buf), fw);
						fwrite("`", 1, 1, fw);
						fwrite(d, 1, strlen(d), fw);
					}
					else
						fwrite(buf, 1, strlen(buf), fw);
				}
				fclose(fp);
				unlink("tmp2.sql");
				printf("OK\n\n");
			}
			else
				Error("FALLA\nNo se puede empaquetar. Abortando...");
			fclose(fw);
		}
		else
			Error("FALLA\nNo se puede empaquetar. Abortando...");
		printf("Actualizacion completada con exito. Cuando inicie Colossus detectara que hay una actualizacion disponible y le pedira su autorizacion "
			"para que se lleve a cabo. Digale que Si.\n\n");
		printf("***** FASE 2: COMPLETADA *****\n\n");
		printf("***** FASE 3: Modificacion del archivo colossus.conf *****\n\n");
		copyfile("colossus.tmp", "../colossus.conf");
		unlink("colossus.tmp");
		printf("***** FASE 3: COMPLETADA *****\n\n");
		printf("Pulse cualquier tecla para salir");
	}
	else
		printf("Abortando...");
	getchar();
	return 0;
}
