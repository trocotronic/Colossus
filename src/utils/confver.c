/*
 * $Id: confver.c,v 1.2 2008/01/21 19:46:45 Trocotronic Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
	char buf[1024], *l, *c, d, ver[32];
	FILE *fp1, *fp2;
	int v1, v2, v3, rev, comps = 1;
	if (!(fp1 = fopen("cambios", "r")))
		return -1;
	while (fgets(buf, sizeof(buf)-1, fp1))
	{
		l = buf;
	}
	fclose(fp1);
	if (!(c = strchr(l, ' ')))
		return -1;
	*c = '\0';
	rev = atoi(l+6);
	if (!(fp1 = fopen("include/version.h", "r")))
		return -1;
	while (fgets(buf, sizeof(buf)-1, fp1))
	{
		if (!strncmp(buf, "#define COLOSSUS_VERNUM", 23))
		{
			if ((c = strchr(buf, '\r')))
				*c = '\0';
			else if ((c = strchr(buf, '\n')))
				*c = '\0';
			v1 = atoi(&buf[25]);
			v2 = atoi(strchr(buf, '.')+1);
			d = buf[strlen(buf)-2];
			if (d >= 97 && d <= 122)
				v3 = d-96;
			else
				v3 = 0;
		}
	}
	fclose(fp1);
	if ((fp1 = fopen("src/version.c", "r")))
	{
		while (fgets(buf, sizeof(buf)-1, fp1))
		{
			if (!strncmp(buf, "int compilacion", 15))
			{
				c = strrchr(buf, ' ')+1;
				comps = atoi(c)+1;
			}
		}
		fclose(fp1);
	}
	if (!(fp1 = fopen("src/version.c.tpl", "r")))
		return -1;
	if (!(fp2 = fopen("src/version.c", "w")))
		return -1;
	while (fgets(buf, sizeof(buf)-1, fp1))
	{
		if (!strncmp(buf, "char *bid\r\n", 9))
			fprintf(fp2, "char *bid = \"%i.%i.%i.%i\";\r\n", v1, v2, v3, rev);
		else if (!strncmp(buf, "int compilacion", 15))
			fprintf(fp2, "int compilacion = %i;\r\n", comps);
		else if (!strncmp(buf, "int rev", 7))
			fprintf(fp2, "int rev = %i;\r\n", rev);
		else
			fprintf(fp2, "%s", buf);
	}
	fclose(fp1);
	fclose(fp2);
#ifdef _WIN32
	if (!(fp1 = fopen("src/win32/colossus.rc.tpl", "r")))
		return -1;
	if (!(fp2 = fopen("src/win32/colossus.rc", "w")))
		return -1;
	while (fgets(buf, sizeof(buf)-1, fp1))
	{
		if (!strncmp(buf, " FILEVERSION", 12))
			fprintf(fp2, " FILEVERSION %i,%i,%i,%i\r\n", v1, v2, v3, rev);
		else if (!strncmp(buf, " PRODUCTVERSION", 15))
			fprintf(fp2, " PRODUCTVERSION %i,%i,%i,%i\r\n", v1, v2, v3, rev);
		else if (strstr(buf, "FileVersion"))
		{
			if (v3)
				fprintf(fp2, "            VALUE \"FileVersion\", \"%i.%i%c\\0\"\r\n", v1, v2, v3+96);
			else
				fprintf(fp2, "            VALUE \"FileVersion\", \"%i.%i\\0\"\r\n", v1, v2);
		}
		else if (strstr(buf, "ProductVersion"))
		{
			if (v3)
				fprintf(fp2, "            VALUE \"ProductVersion\", \"%i.%i%c\\0\"\r\n", v1, v2, v3+96);
			else
				fprintf(fp2, "            VALUE \"ProductVersion\", \"%i.%i\\0\"\r\n", v1, v2);
		}
		else
			fprintf(fp2, "%s", buf);
	}
	fclose(fp1);
	fclose(fp2);
#endif
	return 0;
}
