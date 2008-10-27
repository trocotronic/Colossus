/*
 * $Id: udb.c,v 1.27 2008/02/16 23:19:43 Trocotronic Exp $
 */

#ifdef _WIN32
#include <io.h>
#else
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"
#include "modulos/operserv.h"
#include "modulos/ipserv.h"
#include "bdd.h"
#ifdef USA_ZLIB
#include "zip.h"
int zDeflate(int, FILE *, int);
int zInflate(FILE *, FILE *);
#endif


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

int SigEOS();
extern ChanServ *chanserv;
extern void CargaChanServ(Extension *);
extern void DescargaChanServ(Extension *);
extern NickServ *nickserv;
extern void CargaNickServ(Extension *);
extern void DescargaNickServ(Extension *);
extern OperServ *operserv;
extern void CargaOperServ(Extension *);
extern void DescargaOperServ(Extension *);
extern IpServ *ipserv;
extern void CargaIpServ(Extension *);
extern void DescargaIpServ(Extension *);

IRCFUNC(m_db);
IRCFUNC(m_dbq);
IRCFUNC(m_eos_U);
#define MSG_DB "DB"
#define TOK_DB "DB"
#define MSG_DBQ "DBQ"
#define TOK_DBQ "DBQ"
#define MSG_SVS3MODE "SVS3MODE"
#define TOK_SVS3MODE "vv"
#define MSG_EOS "EOS"
#define TOK_EOS "ES"
void UdbDaleCosas(Cliente *);
int UdbOptimiza();
int UdbBackup();
int SigPreNick(Cliente *, char *);
int SigPostNick_U(Cliente *, int);
int SigSynch();
int SigSockOpen();
int SigSockClose();
u_long UMODE_SUSPEND;
u_long UMODE_PRIVDEAF;
u_long UMODE_SHOWIP;
extern u_long UMODE_SERVICES;
extern void ProcesaModo(Cliente *, Canal *, char **, int);
extern void EntraCliente(Cliente *, char *);
extern int CierraUDB();
#ifdef _WIN32
long base64dec(char *);
char *base64enc(long);
#else
extern long base64dec(char *);
extern char *base64enc(long);
#endif
#define NOSERVDEOP 0x40
IRCFUNC(*sjoin);
IRCFUNC(m_sjoin_U);
int opts = 0;
Timer *timeropt = NULL, *timerbck = NULL;
int (*p_svsmode_r)(Cliente *, Cliente *, char *, ...);
#define UDB_AUTOOPT 0x1
#define UDB_AUTOBCK 0x2

ModInfo MOD_INFO(UDB) = {
	"UDB" ,
	3.4,
	"Trocotronic" ,
	"trocotronic@telefonica.net"
};
int p_svsmode_U(Cliente *cl, Cliente *bl, char *modos, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, modos);
	ircvsprintf(buf, modos, vl);
	va_end(vl);
	ProcesaModosCliente(cl, modos);
	EnviaAServidor(":%s %s %s %s %lu", bl->nombre, TOK_SVS3MODE, cl->nombre, buf, time(0));
	LlamaSenyal(SIGN_UMODE, 2, cl, modos);
	return 0;
}
int MOD_CARGA(UDB)(Extension *ext, Protocolo *prot)
{
	Modulo *mod;
	int i;
	Comando *com;
	opts = 0;
	for (mod = modulos; mod; mod = mod->sig)
	{
		if (!strcmp(mod->info->nombre, "ChanServ"))
			chanserv = (ChanServ *)mod->conf;
		else if (!strcmp(mod->info->nombre, "NickServ"))
			nickserv = (NickServ *)mod->conf;
		else if (!strcmp(mod->info->nombre, "OperServ"))
			operserv = (OperServ *)mod->conf;
		else if (!strcmp(mod->info->nombre, "IpServ"))
			ipserv = (IpServ *)mod->conf;
	}
	if (ext->config)
	{
		int j;
		for (j = 0; j < ext->config->secciones; j++)
		{
			if (!strcmp(ext->config->seccion[j]->item, "autooptimizar"))
				opts |= UDB_AUTOOPT;
			else if (!strcmp(ext->config->seccion[j]->item, "autobackup"))
				opts |= UDB_AUTOBCK;
		}
	}
	IniciaUDB();
	p_svsmode_r = (int(*)(Cliente *, Cliente *, char *, ...))protocolo->comandos[P_MODO_USUARIO_REMOTO];
	protocolo->comandos[P_MODO_USUARIO_REMOTO] = (int(*)())p_svsmode_U;
	InsertaComando(MSG_DB, TOK_DB, m_db, INI, 5);
	InsertaComando(MSG_DBQ, TOK_DBQ, m_dbq, INI, 2);
	InsertaComando(MSG_EOS, TOK_EOS, m_eos_U, INI, MAXPARA);
	if ((com = BuscaComando("SJOIN")))
	{
		sjoin = com->funcion[0];
		BorraComando("SJOIN", sjoin);
	}
	InsertaComando("SJOIN", "~", m_sjoin_U, INI, MAXPARA);
	InsertaSenyal(SIGN_SQL, CargaBloques);
	InsertaSenyal(SIGN_EOS, SigEOS);
	InsertaSenyal(SIGN_PRE_NICK, SigPreNick);
	InsertaSenyalEx(SIGN_POST_NICK, SigPostNick_U, INI);
	InsertaSenyal(SIGN_SYNCH, SigSynch);
	InsertaSenyal(SIGN_SOCKOPEN, SigSockOpen);
	InsertaSenyal(SIGN_SOCKCLOSE, SigSockClose);
	InsertaSenyal(SIGN_CLOSE, CierraUDB);
	InsertaModoProtocolo('S', &UMODE_SUSPEND, protocolo->umodos);
	InsertaModoProtocolo('D', &UMODE_PRIVDEAF, protocolo->umodos);
	InsertaModoProtocolo('X', &UMODE_SHOWIP, protocolo->umodos);
	for (i = 0; i < MAXMOD && protocolo->umodos[i].flag && protocolo->umodos[i].mode; i++)
	{
		if (protocolo->umodos[i].flag == 'S')
			protocolo->umodos[i].flag = 'k';
	}
	if (chanserv)
		CargaChanServ(ext);
	if (nickserv)
		CargaNickServ(ext);
	if (operserv)
		CargaOperServ(ext);
	if (ipserv)
		CargaIpServ(ext);
	return 0;
}
int MOD_DESCARGA(UDB)(Extension *ext, Protocolo *prot)
{
	if (chanserv)
		DescargaChanServ(ext);
	if (nickserv)
		DescargaNickServ(ext);
	if (operserv)
		DescargaOperServ(ext);
	if (ipserv)
		DescargaIpServ(ext);
	if (opts & UDB_AUTOOPT && timeropt)
	{
		ApagaCrono(timeropt);
		timeropt = NULL;
	}
	if (opts & UDB_AUTOBCK && timerbck)
	{
		ApagaCrono(timerbck);
		timerbck = NULL;
	}
	BorraComando(MSG_DB, m_db);
	BorraComando(MSG_DBQ, m_dbq);
	BorraComando(MSG_EOS, m_eos_U);
	BorraComando("SJOIN", m_sjoin_U);
	InsertaComando("SJOIN", "~", sjoin, INI, MAXPARA);
	BorraSenyal(SIGN_SQL, CargaBloques);
	BorraSenyal(SIGN_EOS, SigEOS);
	BorraSenyal(SIGN_PRE_NICK, SigPreNick);
	BorraSenyal(SIGN_POST_NICK, SigPostNick_U);
	BorraSenyal(SIGN_SYNCH, SigSynch);
	BorraSenyal(SIGN_SOCKOPEN, SigSockOpen);
	BorraSenyal(SIGN_SOCKCLOSE, SigSockClose);
	BorraSenyal(SIGN_CLOSE, CierraUDB);
	protocolo->comandos[P_MODO_USUARIO_REMOTO] = (int(*)())p_svsmode_r;
	opts = 0;
	return 0;
}
int SigEOS()
{
	PropagaRegistro("S::D md5");
	return 0;
}
int SigPreNick(Cliente *cl, char *nuevo)
{
	if (strcasecmp(nuevo, cl->nombre))
	{
		Udb *bloq, *reg;
		ProcesaModosCliente(cl, "-S");
		if ((reg = BuscaBloque(cl->nombre, UDB_NICKS)) && (bloq = BuscaBloque(N_MOD, reg)))
		{
			char tmp[32];
			ircsprintf(tmp, "-%s", bloq->data_char);
			ProcesaModosCliente(cl, tmp);
		}
	}
	return 0;
}
int SigPostNick_U(Cliente *cl, int nuevo)
{
	UdbDaleCosas(cl);
	return 0;
}
int SigSockOpen()
{
	EnviaAServidor("PROTOCTL UDB" UDB_VER, me.nombre);
	return 0;
}
IRCFUNC(m_db)
{
	static int bloqs = 0;
	if (parc < 5)
	{
		EnviaAServidor(":%s DB %s ERR 0 %i 0", me.nombre, cl->nombre, E_UDB_PARAMS);
		return 1;
	}
	if (!strcasecmp(parv[2], "INF"))
	{
		if (!match(parv[1], me.nombre))
		{
			UDBloq *bloq;
			time_t gm;
			u_long crc32;
			if (!(bloq = CogeDeId(*parv[3])))
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, bloq->letra);
				return 1;
			}
			gm = atoul(parv[5]);
			sscanf(parv[4], "%lX", &crc32);
			if (crc32 != bloq->crc32)
			{
				if (gm > bloq->gmt)
				{
					TruncaBloque(bloq, 0);
					EnviaAServidor(":%s DB %s RES %c 0", me.nombre, parv[0], *parv[3]);
					bloq->res = cl; /* esperamos su res */
					ActualizaGMT(bloq, gm);
					if (++bloqs == BDD_TOTAL)
						EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
				}
				else if (gm == bloq->gmt)
				{
					EnviaAServidor(":%s DB %s RES %c %lu", me.nombre, parv[0], *parv[3], bloq->lof);
					bloq->res = cl;
				}
				/* si es menor, el otro nodo vaciará su db y nos mandará un RES, será cuando empecemos el resumen. abremos terminado nuestro burst */
			}
			else
			{
				if (++bloqs == BDD_TOTAL)
					EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
			}
		}
	}
	else if (!strcasecmp(parv[2], "RES"))
	{
		if (!match(parv[1], me.nombre))
		{
			u_long bytes;
			UDBloq *bloq;
			if (!(bloq = CogeDeId(*parv[3])))
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
				return 1;
			}
			if (bloq->res && bloq->res != cl)
			{
				EnviaAServidor(":%s DB %s ERR RES %i %c", me.nombre, cl->nombre, E_UDB_RPROG, *parv[3]);
				return 1;
			}
			bytes = atoul(parv[4]);
			if (bytes <= bloq->lof) /* tiene menos o los mismos, se los mandamos */
			{
				char c, linea[BUFSIZE];
				int i = 0;
				lseek(bloq->fd, INI_DATA + bytes, SEEK_SET);
				while (read(bloq->fd, &c, sizeof(c)) > 0)
				{
					if (c == '\r' || c == '\n')
					{
						linea[i] = '\0';
						if (strchr(linea, ' '))
							EnviaAServidor(":%s DB * INS %lu %c::%s", me.nombre, bytes, *parv[3], linea);
						else
							EnviaAServidor(":%s DB * DEL %lu %c::%s", me.nombre, bytes, *parv[3], linea);
						bytes += strlen(linea) + 1;
						i = 0;
					}
					else
						linea[i++] = c;
				}
				EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
				bloq->res = NULL; /* OJO! ya hemos terminado, si no se queda esperando */
			}
			//else
			//	bloq->res = cl; /* esperamos su res */
		//	else if (!bytes && !bloq->data_long)
		//		EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
			if (++bloqs == BDD_TOTAL)
					EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
		}
	}
	else if (!strcasecmp(parv[2], "INS"))
	{
		if (!match(parv[1], me.nombre))
		{
			char buf[1024], *r = parv[4];
			u_long bytes;
			UDBloq *bloq;
			if (parc < 6)
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_PARAMS, *r);
				return 1;
			}
			if (!(bloq = CogeDeId(*r)))
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, *r);
				return 1;
			}
			bytes = atoul(parv[3]);
			if (bloq->res)
			{
				if (bloq->res != cl)
				{
					EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_RPROG, *r);
					return 1;
				}
			}
			if (bytes != bloq->lof)
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, *r, bloq->lof);
				/* a partir de este punto el servidor que recibe esta instrucción debe truncar su bloque con el valor recibido
				   y propagar el truncado por la red, obviamente en el otro sentido.
				   Si el nodo que lo ha enviado es un LEAF propagará un DRP pero que no llegará a ningún sitio porque no tiene servidores
				   en el otro sentido. Si es un HUB, tendrá vía libre para propagar el DRP.
				   A efectos, sería lo mismo que el nodo receptor mandara un DRP, pero así se ahorra una línea ;) */
				return 1;
			}
			r += 3;
			ircsprintf(buf, "%s %s", r, parv[5]);
			if (ParseaLinea(bloq, buf, 1))
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_REP, *parv[4]);
				return 1;
			}
		}
	}
	else if (!strcasecmp(parv[2], "DEL"))
	{
		if (!match(parv[1], me.nombre))
		{
			char *r = parv[4];
			u_long bytes;
			UDBloq *bloq;
			if (!(bloq = CogeDeId(*r)))
			{
				EnviaAServidor(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_NODB, *r);
				return 1;
			}
			bytes = atoul(parv[3]);
			if (bloq->res)
			{
				if (bloq->res != cl)
				{
					EnviaAServidor(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_RPROG, *r);
					return 1;
				}
			}
			else
			if (bytes != bloq->lof)
			{
				EnviaAServidor(":%s DB %s ERR DEL %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, *r, bloq->lof);
				return 1;
			}
			r += 3;
			if (ParseaLinea(bloq, r, 1))
			{
				EnviaAServidor(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_REP, *parv[4]);
				return 1;
			}
		}
	}
	else if (!strcasecmp(parv[2], "DRP"))
	{
		if (!match(parv[1], me.nombre))
		{
			u_long bytes;
			UDBloq *bloq;
			if (!(bloq = CogeDeId(*parv[3])))
			{
				EnviaAServidor(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
				return 1;
			}
			if (bloq->res && bloq->res != cl)
			{
				EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_RPROG, *parv[3]);
				return 1;
			}
			bytes = atoul(parv[4]);
			if (bytes > bloq->lof)
			{
				EnviaAServidor(":%s DB %s ERR DRP %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, *parv[3], bloq->lof);
				return 1;
			}
			TruncaBloque(bloq, bytes);
		}
	}
/*	else if (!strcmp(parv[2], "ERR"))
	{
		int error = 0;
		if (!match(parv[1], me.nombre))
		{
			error = atoi(parv[4]);
			switch (error)
			{
				case E_UDB_LEN:
				{
					UDBloq *bloq;
					u_long bytes;
					char *cb;
					bloq = CogeDeId(*parv[5]);
					cb = strchr(parv[5], ' ') + 1;
					bytes = atoul(cb);
					TruncaBloque(cl, bloq, bytes);
					break;
				}
			}
		}
	}*/
	/* DB * OPT <bdd> <NULL>*/
	else if (!strcmp(parv[2], "OPT"))
	{
		UDBloq *bloq;
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR OPT %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		OptimizaBloque(bloq);
		ActualizaGMT(bloq, atoul(parv[4]));
	}
	else if (!strcmp(parv[2], "FDR"))
	{
		if (!match(parv[1], me.nombre))
		{
			UDBloq *bloq;
			if (!(bloq = CogeDeId(*parv[3])))
			{
				EnviaAServidor(":%s DB %s ERR FDR %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
				return 1;
			}
			if (bloq->res && bloq->res != cl)
			{
				EnviaAServidor(":%s DB %s ERR FDR %i %c", me.nombre, cl->nombre, E_UDB_NORES, *parv[3]);
				return 1;
			}
			bloq->res = NULL;
		}
	}
	else if (!strcmp(parv[2], "BCK"))
	{
		UDBloq *bloq;
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR BCK %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		switch (CopiaSeguridad(bloq, parv[4]))
		{
			case -1:
			case -2:
				EnviaAServidor(":%s DB %s ERR BCK %i %c", me.nombre, cl->nombre, E_UDB_NOOPEN, *parv[3]);
				break;
			case -3:
				EnviaAServidor(":%s DB %s ERR BCK %i %c zDeflate", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
				break;
		}
	}
	else if (!strcmp(parv[2], "RST"))
	{
		UDBloq *bloq;
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR RST %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		ActualizaGMT(bloq, atoul(parv[5]));
		switch (RestauraSeguridad(bloq, parv[4]))
		{
			case -1:
			case -2:
				TruncaBloque(bloq, 0);
				EnviaAServidor(":%s DB %s RES %c 0", me.nombre, parv[0], *parv[3]);
				bloq->res = cl;
				break;
			case -3:
				EnviaAServidor(":%s DB %s ERR RST %i %c zInflate", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
				break;
		}
	}
	else if (!strcmp(parv[2], "FHO"))
	{
		UDBloq *bloq;
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR FHO %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		ActualizaGMT(bloq, atoul(parv[4]));
	}
	return 0;
}
IRCFUNC(m_dbq)
{
	char *cur, *pos, *ds;
	Udb *bloq;
	UDBloq *root;
	if (match(parv[1], me.nombre))
		return 0;
	pos = cur = strdup(parv[2]);
	if (!(root = CogeDeId(*pos)))
	{
		EnviaAServidor(":%s 339 %s :La base de datos %c no existe.", me.nombre, cl->nombre, *pos);
		Free(pos);
		return 1;
	}
	bloq = root->arbol;
	if (parc == 3)
		parv[1] = parv[2];
	if (*(++cur) != '\0')
	{
		if (*cur++ != ':' || *cur++ != ':' || *cur == '\0')
		{
			EnviaAServidor(":%s 339 %s :Formato de bloque incorrecto.", me.nombre, cl->nombre);
			Free(pos);
			return 1;
		}

		while ((ds = strchr(cur, ':')))
		{
			if (*(ds + 1) == ':')
			{
				*ds++ = '\0';
				if (!(bloq = BuscaBloque(cur, bloq)))
					goto nobloq;
			}
			else
				break;
			cur = ++ds;
		}
		if (!(bloq = BuscaBloque(cur, bloq)))
		{
			nobloq:
			EnviaAServidor(":%s 339 %s :No se encuentra el bloque %s.", me.nombre, cl->nombre, cur);
		}
		else
		{
			if (bloq->data_long)
				EnviaAServidor(":%s 339 %s :DBQ %s %lu", me.nombre, cl->nombre, parv[1], bloq->data_long);
			else if (bloq->data_char)
				EnviaAServidor(":%s 339 %s :DBQ %s %s", me.nombre, cl->nombre, parv[1], bloq->data_char);
			else
			{
				Udb *aux;
				for (aux = bloq->down; aux; aux = aux->mid)
				{
					if (aux->data_long)
						EnviaAServidor(":%s 339 %s :DBQ %s::%s %lu", me.nombre, cl->nombre, parv[1], aux->item, aux->data_long);
					else if (aux->data_char)
						EnviaAServidor(":%s 339 %s :DBQ %s::%s %s", me.nombre, cl->nombre, parv[1], aux->item, aux->data_char);
					else
						EnviaAServidor(":%s 339 %s :DBQ %s::%s (no tiene datos)", me.nombre, cl->nombre, parv[1], aux->item);
				}
			}
		}
	}
	else
		EnviaAServidor(":%s 339 %s :%i %i %lu %lu %lX %s", me.nombre, cl->nombre, root->id, root->regs, root->lof, root->gmt, root->crc32, root->res ? "*" : "");
	Free(pos);
	return 0;
}
IRCFUNC(m_eos_U)
{
	Udb *reg;
	if (cl == linkado)
	{
		UdbOptimiza();
		if (!(reg = BuscaBloque(me.nombre, UDB_LINKS)))
			PropagaRegistro("L::%s::O *%lu", me.nombre, L_OPT_PROP);
	}
	return 0;
}
IRCFUNC(m_sjoin_U)
{
	Cliente *al = NULL;
	Canal *cn = NULL;
	char *q, *p, tmp[BUFSIZE], mod[8];
	time_t creacion;
	cn = CreaCanal(parv[2]);
	creacion = base64dec(parv[1]);
	strlcpy(tmp, parv[parc-1], sizeof(tmp));
	for (p = tmp; (q = strchr(p, ' ')); p = q)
	{
		al = NULL;
		q = strchr(p, ' ');
		if (q)
			*q++ = '\0';
		mod[0] = '\0';
		while (*p)
		{
			if (*p == '*')
				strlcat(mod, "q", sizeof(mod));
			else if (*p == '~')
				strlcat(mod, "a", sizeof(mod));
			else if (*p == '@')
				strlcat(mod, "o", sizeof(mod));
			else if (*p == '%')
				strlcat(mod, "h", sizeof(mod));
			else if (*p == '+')
				strlcat(mod, "v", sizeof(mod));
			else if (*p == '&')
			{
				p++;
				strlcat(mod, "b", sizeof(mod));
				break;
			}
			else if (*p == '"')
			{
				p++;
				strlcat(mod, "e", sizeof(mod));
				break;
			}
			else if (*p == '\'')
			{
				p++;
				break;
			}
			else
				break;
			p++;
		}
		if (mod[0] != 'b' && mod[0] != 'e') /* es un usuario */
		{
			if ((al = BuscaCliente(p)))
				EntraCliente(al, parv[2]);
		}
		if (mod[0])
		{
			char *c, *arr[7];
			int i = 0;
			arr[i++] = mod;
			for (c = &mod[0]; *c; c++)
				arr[i++] = p;
			arr[i] = NULL;
			ProcesaModo(cl, cn, arr, i);
			if (al)
				LlamaSenyal(SIGN_MODE, 4, al, cn, arr, i);
		}
	}
	if (creacion < cn->creacion) /* debemos quitar todo lo nuestro */
	{
		MallaParam *mpm;
		MallaMascara *mmk;
		Mascara *mk, *tmk;
		cn->modos = 0L;
		for (mpm = cn->mallapm; mpm; mpm = mpm->sig)
			ircfree(mpm->param);
		for (mmk = cn->mallamk; mmk; mmk = mmk->sig)
		{
			for (mk = mmk->malla; mk; mk = tmk)
			{
				tmk = mk->sig;
				Free(mk->mascara);
				Free(mk);
			}
			mmk->malla = NULL;
		}
		if (cl != linkado)
		{
			MallaCliente *mcl;
			LinkCliente *lk, *tlk;
			if (conf_set->opts & NOSERVDEOP)
			{
				int i = 0;
				char *modos = (char *)Malloc(sizeof(char) * (protocolo->modos + 1));
				tmp[0] = '\0';
				for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
				{
					for (lk = mcl->malla; lk; lk = lk->sig)
					{
						if (i == protocolo->modos)
						{
							modos[i] = '\0';
							ProtFunc(P_MODO_CANAL)(&me, cn, "+%s %s", modos, tmp);
							i = 0;
							tmp[0] = '\0';
						}
						modos[i++] = mcl->flag;
						strlcat(tmp, lk->cl->nombre, sizeof(tmp));
						strlcat(tmp, " ", sizeof(tmp));
					}
				}
				if (i > 0)
				{
					modos[i] = '\0';
					ProtFunc(P_MODO_CANAL)(&me, cn, "+%s %s", modos, tmp);
				}
				Free(modos);
			}
			else
			{
				for (mcl = cn->mallacl; mcl; mcl = mcl->sig)
				{
					for (lk = mcl->malla; lk; lk = tlk)
					{
						tlk = lk->sig;
						Free(lk);
					}
					mcl->malla = NULL;
				}
			}
		}
		cn->creacion = creacion;
	}
	if (parc > 4 && creacion <= cn->creacion) /* hay modos */
	{
		ProcesaModo(cl, cn, parv + 3, parc - 4);
		LlamaSenyal(SIGN_MODE, 4, cl, cn, parv + 3, parc - 4);
	}
	return 0;
}
int SigSynch()
{
	UDBloq *aux;
	if ((opts & UDB_AUTOOPT) && !timeropt)
		timeropt = IniciaCrono(0, 86400, UdbOptimiza, NULL);
	if ((opts & UDB_AUTOBCK) && !timerbck)
		timerbck = IniciaCrono(0, 86400, UdbBackup, NULL);
	for (aux = ultimo; aux; aux = aux->sig)
		EnviaAServidor(":%s DB %s INF %c %lX %lu", me.nombre, linkado->nombre, aux->letra, aux->crc32, aux->gmt);
	return 0;
}
int SigSockClose()
{
	if (opts & UDB_AUTOOPT)
	{
		ApagaCrono(timeropt);
		timeropt = NULL;
	}
	if (opts & UDB_AUTOBCK)
	{
		ApagaCrono(timerbck);
		timerbck = NULL;
	}
	return 0;
}
void UdbDaleCosas(Cliente *cl)
{
	Udb *reg, *bloq;
	if (!(reg = BuscaBloque(cl->nombre, UDB_NICKS)))
		return;
	if (!BuscaBloque(N_SUS, reg))
	{
		ProcesaModosCliente(cl, "+r");
		if ((bloq = BuscaBloque(N_MOD, reg)))
			ProcesaModosCliente(cl, bloq->data_char);
		if ((bloq = BuscaBloque(N_OPE, reg)))
		{
			u_long nivel = bloq->data_long;
			if (nivel & BuscaOpt("OPER", NivelesBDD))
				ProcesaModosCliente(cl, "+h");
			if (nivel & BuscaOpt("ADMIN", NivelesBDD) || nivel & BuscaOpt("ROOT", NivelesBDD))
				ProcesaModosCliente(cl, "+oN");
		}
	}
	else
		ProcesaModosCliente(cl, "+S");
}
int UdbOptimiza()
{
	u_int i;
	UDBloq *aux;
	time_t hora = time(0);
	if (!SockIrcd)
		return 1;
	for (i = 0; i < BDD_TOTAL; i++)
	{
		if ((aux = CogeDeId(i)) && (aux->gmt + 86400) < hora)
		{
			EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, aux->letra, hora);
			OptimizaBloque(aux);
			ActualizaGMT(aux, hora);
		}
	}
	return 0;
}
int UdbBackup()
{
	u_int i;
	UDBloq *aux;
	time_t hora = time(0);
	if (!SockIrcd)
		return 1;
	for (i = 0; i < BDD_TOTAL; i++)
	{
		if ((aux = CogeDeId(i)))
		{
			strftime(buf, sizeof(buf), "%d%m%y-%H%M", gmtime(&hora));
			EnviaAServidor(":%s DB * BCK %c %s", me.nombre, aux->letra, buf);
			CopiaSeguridad(aux,	buf);
		}
	}
	return 0;
}
#ifdef USA_ZLIB
/* rutinas de www.zlib.net */
#define CHUNK 16384
int zDeflate(int source, FILE *dest, int level)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
		return ret;
	lseek(source, 0, SEEK_SET);
	do
	{
		ret = read(source, in, CHUNK);
		if (ret < 0)
		{
			deflateEnd(&strm);
			return Z_ERRNO;
		}
		flush = (ret == 0 ? Z_FINISH : Z_NO_FLUSH);
		strm.avail_in = ret;
		strm.next_in = in;
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);
	deflateEnd(&strm);
	return Z_OK;
}
int zInflate(FILE *source, FILE *dest)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;
	do
	{
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source))
		{
			inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&strm);
					return ret;
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);
	inflateEnd(&strm);
	return (ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR);
}
#endif
int CopiaSeguridad(UDBloq *bloq, char *nombre)
{
	FILE *fp2;
	char tmp[BUFSIZE];
	ircsprintf(tmp, DB_DIR_BCK "%c%s.bck.udb", bloq->letra, nombre);
	if ((fp2 = fopen(tmp, "wb")))
	{
#ifdef USA_ZLIB
		if (zDeflate(bloq->fd, fp2, Z_DEFAULT_COMPRESSION) != Z_OK)
		{
			fclose(fp2);
			return -3;
		}
#else
		size_t leidos;
		while ((leidos = read(bloq->fd, tmp, BUFSIZE)))
			fwrite(tmp, 1, leidos, fp2);
#endif
		fclose(fp2);
	}
	else
		return -1;
	return 0;
}
int RestauraSeguridad(UDBloq *bloq, char *nombre)
{
	FILE *fp1, *fp2;
	char tmp[BUFSIZE];
	ircsprintf(tmp, DB_DIR_BCK "%c%s.bck.udb", bloq->letra, nombre);
	if ((fp1 = fopen(tmp, "rb")))
	{
		close(bloq->fd);
		if ((fp2 = fopen(bloq->path, "wb")))
		{
#ifdef USA_ZLIB
			if (zInflate(fp1, fp2) != Z_OK)
			{
				fclose(fp2);
				fclose(fp1);
				return -3;
			}
#else
			size_t leidos;
			while ((leidos = fread(tmp, 1, BUFSIZE, fp1)))
				fwrite(tmp, 1, leidos, fp2);
#endif
			fclose(fp2);
		}
		else
		{
			fclose(fp1);
			return -2;
		}
		fclose(fp1);
		DescargaBloque(bloq->id);
		CargaBloque(bloq->id);
	}
	else
		return -1;
	return 0;
}
#ifdef _WIN32
/* ':' and '#' and '&' and '+' and '@' must never be in this table. */
/* these tables must NEVER CHANGE! >) */
char int6_to_base64_map[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
	    'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	    'U', 'V',
	'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	    'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	    '{', '}'
};

char base64_to_int6_map[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
	-1, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, -1, 63, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static char *int_to_base64(long val)
{
	/* 32/6 == max 6 bytes for representation,
	 * +1 for the null, +1 for byte boundaries
	 */
	static char base64buf[8];
	long i = 7;

	base64buf[i] = '\0';

	/* Temporary debugging code.. remove before 2038 ;p.
	 * This might happen in case of 64bit longs (opteron/ia64),
	 * if the value is then too large it can easily lead to
	 * a buffer underflow and thus to a crash. -- Syzop
	 */
	if (val > 2147483647L)
	{
		abort();
	}

	do
	{
		base64buf[--i] = int6_to_base64_map[val & 63];
	}
	while (val >>= 6);

	return base64buf + i;
}

static long base64_to_int(char *b64)
{
	int v = base64_to_int6_map[(u_char)*b64++];

	if (!b64)
		return 0;

	while (*b64)
	{
		v <<= 6;
		v += base64_to_int6_map[(u_char)*b64++];
	}

	return v;
}
char *base64enc(long i)
{
	if (i < 0)
		return ("0");
	return int_to_base64(i);
}
long base64dec(char *b64)
{
	if (b64)
		return base64_to_int(b64);
	else
		return 0;
}
#endif
