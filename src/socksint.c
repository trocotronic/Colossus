/*
 * $Id: socksint.c,v 1.4 2007-02-02 17:43:02 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "socksint.h"

int ActivaModulos();
int ActualizaComponentes();
void DetieneMDS();

int (*tasyncs[])() = {
	ActivaModulos ,
	ActualizaComponentes ,
	NULL
};

Signatura sgn;
Componente comps[MAX_COMP];
int componentes = 0;

SOCKFUNC(MotdAbre)
{
	SockWrite(sck, "GET /inicio.php HTTP/1.1");
	SockWrite(sck, "Accept: */*");
	SockWrite(sck, "Host: colossus.redyc.com");
	SockWrite(sck, "Connection: close");
	SockWrite(sck, "");
	return 0;
}
SOCKFUNC(MotdLee)
{
	if (*data == '#')
	{
		int ver;
		char *tok;
		data++;
		strlcpy(tokbuf, data, sizeof(tokbuf));
		for (tok = strtok(tokbuf, "#"); tok; tok = strtok(NULL, "#"))
		{
			if (EsIp(tok))
				ircstrdup(me.ip, tok);
			else if ((ver = atoi(tok)))
			{
				if (COLOSSUS_VERINT < ver)
					Info("Existe una versión más nueva de Colossus. Descárguela de www.redyc.com");
			}
			else
				Info(tok);
		}
	}
	return 0;
}
int SiguienteTAsync(int fuerza)
{
	static int i = 0;
	if (fuerza)
		i = 0;
	if (!tasyncs[i])
	{
		i = 0;
#ifdef _WIN32
		ChkBtCon(SockIrcd ? 1 : 0, 0);
#else
		AbreSockIrcd();
#endif
	}
	else
		tasyncs[i++]();
	return 0;
}
MDS *BuscaMDS(Sock *sck)
{
	MDS *mds;
	for (mds = sgn.mds; mds; mds = mds->sig)
	{
		if (mds->sck == sck)
			return mds;
	}
	return NULL;
}
SOCKFUNC(ActivoAbre)
{
	MDS *mds;
	char tmp[SOCKBUF];
	int i;
	if ((mds = BuscaMDS(sck)))
	{
		ircsprintf(tmp, "ver=2&cpuid=%s", sgn.cpuid);
		if (!BadPtr(sgn.sgn))
		{
			strlcat(tmp, "&sgn=", sizeof(tmp));
			strlcat(tmp, sgn.sgn, sizeof(tmp));
		}
		for (i = 0; i < MAX_MDS && mds->md[i].hmod; i++)
		{
			ircsprintf(buf, "&serial[%i]=%s&modulo[%i]=%s", i, mds->md[i].hmod->serial, i, mds->md[i].hmod->info->nombre);
			strlcat(tmp, buf, sizeof(tmp));
		}
		SockWrite(sck, "POST /validar.php HTTP/1.0");
		SockWrite(sck, "Content-Type: application/x-www-form-urlencoded");
		SockWrite(sck, "Content-Length: %u", strlen(tmp));
		SockWrite(sck, "Host: colossus.redyc.com");
		SockWrite(sck, "");
		SockWrite(sck, tmp);
	}
	return 0;
}
SOCKFUNC(ActivoLee)
{
	MDS *mds;
	if ((mds = BuscaMDS(sck)))
	{
		if (*data == '$')
		{
			FILE *fp;
			if ((fp = fopen("colossus.sgn", "w")))
			{
				fwrite(data+1, 1, strlen(data)-1, fp);
				fclose(fp);
				strlcpy(sgn.sgn, data+1, sizeof(sgn.sgn));
			}
		}
		else if (*data == '#')
		{
			int i = 0;
			char *c, *d, tmp[BUFSIZE];
			strlcpy(tmp, data, sizeof(tmp));
			c = tmp;
			while (!BadPtr(c) && (d = strchr(c, ' ')))
			{
				int err;
				*d = '\0';
				err = atoi(c+1);
				switch(err)
				{
					case 0:
						mds->md[i].res = 0;
						mds->md[i].hmod->activo = 0;
						ReconectaBot(mds->md[i].hmod->nick);
						break;
					case 1:
						unlink("colossus.sgn");
						bzero(sgn.sgn, sizeof(sgn.sgn));
					case 2:
						Info("El módulo %s no tiene permiso para usarse en su servidor (err:%i)", mds->md[i].hmod->info->nombre, err);
						break;
					case 3:
						Info("Clave para el módulo %s desconocida", mds->md[i].hmod->info->nombre);
						break;
					case 4:
						Info("Clave para el módulo %s incorrecta", mds->md[i].hmod->info->nombre);
						break;
					case 100:
					case 101:
						Info("Existe un error con la conexión. Póngase en contacto con el autor (err:%i)", err);
						break;
					default:
						Info("Error desconocido para el módulo %s", mds->md[i].hmod->info->nombre);
				}
				c = d+1;
				i++;
			}
		}
	}
	return 0;
}
SOCKFUNC(ActivoCierra)
{
	MDS *mds;
	if (!data && (mds = BuscaMDS(sck)))
	{
		int i;
		for (i = 0; i < MAX_MDS && mds->md[i].hmod; i++)
		{
			if (mds->md[i].res)
				DescargaModulo(mds->md[i].hmod);
		}
		BorraItem(mds, sgn.mds);
		Free(mds);
	}
	if (!sgn.mds)
		SiguienteTAsync(0);
	return 0;
}
int ActivaModulos()
{
	Modulo *mod;
	MDS *mds = NULL;
	int inf = 1;
	if (sgn.mds)
		DetieneMDS();
	for (mod = modulos; mod; mod = mod->sig)
	{
		if (mod->activo)
		{
			if (!mod->serial)
			{
				Modulo *tmp;
				tmp = mod->sig;
				Info("Introduzca la clave de acceso para el módulo %s", mod->info->nombre);
				DescargaModulo(mod);
				mod = tmp;
			}
			else
			{
				int i;
				if (inf)
				{
					Info("Verificando servicios...");
#ifdef _WIN32
					ChkBtCon(SockIrcd ? 1 : 0, 1);
#endif
					inf = 0;
				}
				empieza:
				if (!mds)
				{
					mds = BMalloc(MDS);
					mds->sck = SockOpen("colossus.redyc.com", 80, ActivoAbre, ActivoLee, NULL, ActivoCierra);
					AddItem(mds, sgn.mds);
				}
				for (i = 0; i < MAX_MDS && mds->md[i].hmod; i++);
				if (i == MAX_MDS)
				{
					mds = NULL;
					goto empieza;
				}
				mds->md[i].hmod = mod;
				mds->md[i].res = 1;
			}
		}
	}
#ifndef _WIN32
	if (inf)
		SiguienteTAsync(0);
#endif
  	return 0;
}
void DetieneMDS()
{
	MDS *mds, *tmp;
	for (mds = sgn.mds; mds; mds = tmp)
	{
		tmp = mds->sig;
		SockClose(mds->sck, LOCAL);
		Free(mds);
	}
	sgn.mds = NULL;
}
void CargaSignatura()
{
	FILE *fp;
	sgn.mds = NULL;
	bzero(sgn.sgn, sizeof(sgn.sgn));
	if ((fp = fopen("colossus.sgn", "r")))
	{
		fgets(sgn.sgn, sizeof(sgn.sgn)-1, fp);
		fclose(fp);
	}
}
int BuscaComponente(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_COMP; i++)
	{
		if (comps[i].sck == sck)
			return i;
	}
	return -1;
}
SOCKFUNC(AbreComp)
{
	int i;
	char tmp[SOCKBUF];
	if ((i = BuscaComponente(sck)) < 0)
		return 1;
	ircsprintf(tmp, "descarga=%s", comps[i].ruta);
	SockWrite(sck, "POST /componentes.php HTTP/1.0");
	SockWrite(sck, "Host: colossus.redyc.com");
	SockWrite(sck, "Content-Type: application/x-www-form-urlencoded");
	SockWrite(sck, "Content-Length: %u", strlen(tmp));
	SockWrite(sck, "");
	SockWrite(sck, tmp);
	return 0;
}
SOCKFUNC(LeeComp)
{
	int i;
	char *c;
	if ((i = BuscaComponente(sck)) < 0)
		return 1;
	if (!comps[i].fd && (c = strstr(data, "\r\n\r\n")))
	{
		c += 4;
		ircsprintf(buf, "tmp/%s.tmp", strrchr(comps[i].ruta, '/')+1);
		comps[i].tmp = strdup(buf);
		comps[i].fd = open(buf, O_CREAT|O_BINARY|O_WRONLY, 0600);
		write(comps[i].fd, c, len - (c-data));
	}
	else
		write(comps[i].fd, data, len);
	return 0;
}
SOCKFUNC(CierraComp)
{
	int i;
	if ((i = BuscaComponente(sck)) < 0)
		return 1;
	close(comps[i].fd);
	unlink(comps[i].ruta);
	rename(comps[i].tmp, comps[i].ruta);
	unlink(comps[i].tmp);
	ircfree(comps[i].ruta);
	ircfree(comps[i].tmp);
	bzero(&comps[i], sizeof(Componente));
	if (!--componentes)
	{
		Info("Ok.");
		Refresca();
	}
	return 0;
}
int InsertaComponente(char *ruta)
{
	if (componentes == MAX_COMP)
		return 0;
	if ((comps[componentes].sck = SockOpenEx("colossus.redyc.com", 80, AbreComp, LeeComp, NULL, CierraComp, 30, 30, OPT_BIN | OPT_NORECVQ)))
	{
		comps[componentes].ruta = strdup(ruta);
		componentes++;
		Info("Actualizando %s...", strrchr(ruta, '/')+1);
	}
	return 1;
}
SOCKFUNC(ACOpen)
{
	componentes = 0;
#ifdef _WIN32
	SockWrite(sck, "GET /componentes.php?info=win32 HTTP/1.1");
#else
	SockWrite(sck, "GET /componentes.php?info=linux HTTP/1.1");
#endif
	SockWrite(sck, "Accept: */*");
	SockWrite(sck, "Host: colossus.redyc.com");
	SockWrite(sck, "Connection: close");
	SockWrite(sck, "");
	return 0;
}
SOCKFUNC(ACRead)
{
	if (*data == '#')
	{
		int fd;
		char *c, *d;
		struct stat sb;
		u_long t;
		c = strchr(data++, ' ');
		*c++ = '\0';
		if ((fd = open(data, O_RDONLY|O_BINARY)) < 0)
			return 1;
		if (fstat(fd, &sb) == -1)
		{
			close(fd);
			return 1;
		}
		t = atoul(c);
		c = (char *)Malloc(sizeof(char) * (sb.st_size + 1));
		read(fd, c, sb.st_size);
		c[sb.st_size] = '\0';
		if (t != Crc32(c, sb.st_size))
		{
			InsertaComponente(data);
#ifdef _WIN32
			d = strrchr(data, '.');
			strcpy(d, ".pdb");
			InsertaComponente(data);
#endif
		}
		Free(c);
		close(fd);
	}
	return 0;
}
SOCKFUNC(ACClose)
{
	if (!componentes)
	{
		Info("Ok.");
		SiguienteTAsync(0);
	}
	return 0;
}
int ActualizaComponentes()
{
	if (conf_set->actualiza)
	{
		bzero(comps, sizeof(Componente) * MAX_COMP);
		if (!SockOpen("colossus.redyc.com", 80, ACOpen, ACRead, NULL, ACClose))
			SiguienteTAsync(0);
	}
	else
		SiguienteTAsync(0);
	return 0;
}
