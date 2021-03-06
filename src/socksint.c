/*
 * $Id: socksint.c,v 1.14 2008/02/13 16:47:18 Trocotronic Exp $
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
//	ActivaModulos ,
//	ActualizaComponentes ,
	NULL
};

//Signatura sgn;
//Componente comps[MAX_COMP];
//int componentes = 0;

SOCKFUNC(MotdAbre)
{
	char *pk;
	if (conf_set->userid && (pk = SQLCogeRegistro(SQL_CONFIG, "PBKey", "valor")))
	{
		char tmp[4096], b64[2048];
		u_long cr;
		bzero(b64, sizeof(b64));
		ircsprintf(tmp, "%s-83726982-%s", cpid, conf_set->userid);
		cr = Crc32(tmp, strlen(tmp));
		b64_encode(pk, strlen(pk), b64, sizeof(b64));
		ircsprintf(tmp, "userid=%s&cr=%X&cpid=%s&pkey=%s", conf_set->userid, cr + 0xAB87E438, cpid, b64);
		SockWrite(sck, "POST /inicio.php HTTP/1.0");
		SockWrite(sck, "Content-Type: application/x-www-form-urlencoded");
		SockWrite(sck, "Content-Length: %u", strlen(tmp));
		SockWrite(sck, "Host: colossus.redyc.com");
		SockWrite(sck, "");
		SockWrite(sck, tmp);
	}
	else
	{
		SockWrite(sck, "GET /inicio.php HTTP/1.1");
		SockWrite(sck, "Accept: */*");
		SockWrite(sck, "Host: colossus.redyc.com");
		SockWrite(sck, "Connection: close");
		SockWrite(sck, "");
	}
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
				{
					Info("Existe una versi�n m�s nueva de Colossus. Desc�rguela de www.redyc.com");
#ifdef _WIN32
					if (MessageBox(NULL, "Existe una versi�n m�s nueva de Colossus. �Quiere ir a la p�gina de Redyc para descargarla?", "Nueva versi�n", MB_YESNO|MB_ICONQUESTION) == IDYES)
						ShellExecute(NULL, "open", "http://www.redyc.com", NULL, NULL, SW_SHOWNORMAL);
#endif
				}
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
		if (conf_server->autocon)
		{
#endif
		if (!SockIrcd)
			AbreSockIrcd();
#ifdef _WIN32
		}
#endif
		if (refrescando)
		{
			if (SockIrcd)
			{
				LlamaSenyal(SIGN_SYNCH, 0);
				LlamaSenyal(SIGN_EOS, 0);
			}
			refrescando = 0;
		}
	}
	else
		tasyncs[i++]();
	return 0;
}
/*
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
		ircsprintf(tmp, "ver=2&cpuid=%s", cpid);
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
			SQLInserta(SQL_CONFIG, "Signatura", "valor", data+1);
			strlcpy(sgn.sgn, data+1, sizeof(sgn.sgn));
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
						SQLBorra(SQL_CONFIG, "Signatura");
						bzero(sgn.sgn, sizeof(sgn.sgn));
					case 2:
						Info("El m�dulo %s no tiene permiso para usarse en su servidor (err:%i)", mds->md[i].hmod->info->nombre, err);
						break;
					case 3:
						Info("Clave para el m�dulo %s desconocida", mds->md[i].hmod->info->nombre);
						break;
					case 4:
						Info("Clave para el m�dulo %s incorrecta", mds->md[i].hmod->info->nombre);
						break;
					case 100:
					case 101:
						Info("Existe un error con la conexi�n. P�ngase en contacto con el autor (err:%i)", err);
						break;
					default:
						Info("Error desconocido para el m�dulo %s", mds->md[i].hmod->info->nombre);
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
	Modulo *mod, *tmp;
	MDS *mds = NULL;
	int inf = 1;
	if (sgn.mds)
		DetieneMDS();
	for (mod = modulos; mod; mod = tmp)
	{
		tmp = mod->sig;
		if (mod->activo)
		{
			if (!mod->serial)
			{
				Info("Introduzca la clave de acceso para el m�dulo %s", mod->info->nombre);
				DescargaModulo(mod);
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
	if (inf)
		SiguienteTAsync(0);
  	return 0;
}
void DetieneMDS()
{
	MDS *mds, *tmp;
	for (mds = sgn.mds; mds; mds = tmp)
	{
		tmp = mds->sig;
		SockClose(mds->sck);
		Free(mds);
	}
	sgn.mds = NULL;
}
void CargaSignatura()
{
	char *tmp;
	sgn.mds = NULL;
	bzero(sgn.sgn, sizeof(sgn.sgn));
	if ((tmp = SQLCogeRegistro(SQL_CONFIG, "Signatura", "valor")))
		strlcpy(sgn.sgn, tmp, sizeof(sgn.sgn)-1);
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
		char *c;
#ifdef _WIN32
		char *d;
#endif
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
	//if (conf_set->actualiza)
	if (0)
	{
		bzero(comps, sizeof(Componente) * MAX_COMP);
		if (!SockOpen("colossus.redyc.com", 80, ACOpen, ACRead, NULL, ACClose))
			SiguienteTAsync(0);
	}
	else
		SiguienteTAsync(0);
	return 0;
}
*/
