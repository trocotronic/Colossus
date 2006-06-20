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
int zDeflate(FILE *, FILE *, int);
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
int UdbCompruebaOpts(Proc *);
int SigPreNick(Cliente *, char *);
int SigPostNick_U(Cliente *, int);
int SigSynch();
int SigSockOpen();
u_long UMODE_SUSPEND;
u_long UMODE_PRIVDEAF;
u_long UMODE_SHOWIP;
extern u_long UMODE_SERVICES;
extern void ProcesaModo(Cliente *, Canal *, char **, int);
extern void EntraCliente(Cliente *, char *);
IRCFUNC(*sjoin);
IRCFUNC(m_sjoin_U);
int opts = 0;
#define UDB_AUTOOPT 0x1

ModInfo MOD_INFO(UDB) = {
	"UDB" ,
	3.3,
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
	Senyal2(SIGN_UMODE, cl, modos);
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
		}
	}
	IniciaUDB();
	if (opts & UDB_AUTOOPT)
		IniciaProceso(UdbCompruebaOpts);
	protocolo->comandos[P_MODO_USUARIO_REMOTO] = p_svsmode_U;
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
	if (opts & UDB_AUTOOPT)
		DetieneProceso(UdbCompruebaOpts);
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
		if ((reg = BuscaBloque(cl->nombre, UDB_NICKS)) && (bloq = BuscaBloque(N_MOD_TOK, reg)))
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
	if (nuevo == 1)
		UdbDaleCosas(cl);
	return 0;
}
int SigSockOpen()
{
	EnviaAServidor("PROTOCTL UDB" UDB_VER "=%s,SD", me.nombre);
	return 0;
}
IRCFUNC(m_db)
{
	static int bloqs = 0;
	if (parc < 5)
	{
		EnviaAServidor(":%s DB %s ERR 0 %i", me.nombre, cl->nombre, E_UDB_PARAMS);
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
					TruncaBloque(cl, bloq, 0);
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
				/* si es menor, el otro nodo vaciar� su db y nos mandar� un RES, ser� cuando empecemos el resumen. abremos terminado nuestro burst */
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
				FILE *fp;
				if ((fp = fopen(bloq->path, "rb")))
				{
					char linea[512], *d;
					fseek(fp, bytes, SEEK_SET);
					while (!feof(fp))
					{
						bzero(linea, 512);
						if (!fgets(linea, 512, fp))
							break;
						if ((d = strchr(linea, '\r')))
							*d = '\0';
						if ((d = strchr(linea, '\n')))
							*d = '\0';
						if (strchr(linea, ' '))
							EnviaAServidor(":%s DB * INS %lu %c::%s", me.nombre, bytes, *parv[3], linea);
						else
							EnviaAServidor(":%s DB * DEL %lu %c::%s", me.nombre, bytes, *parv[3], linea);
						bytes = ftell(fp);
					}
					fclose(fp);
				}
				else
					EnviaAServidor(":%s DB %s ERR RES %i %c", me.nombre, cl->nombre, E_UDB_NOOPEN, *parv[3]);
				EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
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
				EnviaAServidor(":%s DB %s ERR INS %i", me.nombre, cl->nombre, E_UDB_PARAMS);
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
				/* a partir de este punto el servidor que recibe esta instrucci�n debe truncar su bloque con el valor recibido
				   y propagar el truncado por la red, obviamente en el otro sentido. 
				   Si el nodo que lo ha enviado es un LEAF propagar� un DRP pero que no llegar� a ning�n sitio porque no tiene servidores
				   en el otro sentido. Si es un HUB, tendr� v�a libre para propagar el DRP. 
				   A efectos, ser�a lo mismo que el nodo receptor mandara un DRP, pero as� se ahorra una l�nea ;) */
				return 1;
			}
			r += 3;
			ircsprintf(buf, "%s %s", r, parv[5]);
			if (ParseaLinea(bloq->id, buf, 1))
			{
				EnviaAServidor(":%s DB %s ERR INS %i", me.nombre, cl->nombre, E_UDB_REP);
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
			if (ParseaLinea(bloq->id, r, 1))
			{
				EnviaAServidor(":%s DB %s ERR DEL %i", me.nombre, cl->nombre, E_UDB_REP);
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
			TruncaBloque(cl, bloq, bytes);
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
		FILE *fp1, *fp2;
		char tmp[BUFSIZE];
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR BCK %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		ircsprintf(tmp, DB_DIR_BCK "%c%s.bck.udb", *parv[3], parv[4]);
		if ((fp1 = fopen(bloq->path, "rb")) && (fp2 = fopen(tmp, "wb")))
		{
#ifdef USA_ZLIB
			if (zDeflate(fp1, fp2, Z_DEFAULT_COMPRESSION) != Z_OK)
				EnviaAServidor(":%s DB %s ERR BCK %i %c zDeflate", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
#else
			size_t leidos;
			while ((leidos = fread(tmp, 1, BUFSIZE, fp1)))
				fwrite(tmp, 1, leidos, fp2);
#endif
			fclose(fp1);
			fclose(fp2);
		}
		else
			EnviaAServidor(":%s DB %s ERR BCK %i %c", me.nombre, cl->nombre, E_UDB_NOOPEN, *parv[3]);
	}
	else if (!strcmp(parv[2], "RST"))
	{
		UDBloq *bloq;
		FILE *fp1, *fp2;
		char tmp[BUFSIZE];
		if (!(bloq = CogeDeId(*parv[3])))
		{
			EnviaAServidor(":%s DB %s ERR RST %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		ActualizaGMT(bloq, atoul(parv[5]));
		ircsprintf(tmp, DB_DIR_BCK "%c%s.bck.udb", *parv[3], parv[4]);
		if ((fp1 = fopen(tmp, "rb")))
		{
			if ((fp2 = fopen(bloq->path, "wb")))
			{
#ifdef USA_ZLIB
				if (zInflate(fp1, fp2) != Z_OK)
					EnviaAServidor(":%s DB %s ERR RST %i %c zInflate", me.nombre, cl->nombre, E_UDB_FATAL, *parv[3]);
#else
				size_t leidos;
				while ((leidos = fread(tmp, 1, BUFSIZE, fp1)))
					fwrite(tmp, 1, leidos, fp2);
#endif
				fclose(fp2);
			}
			fclose(fp1);
			ActualizaHash(bloq);
			DescargaBloque(bloq->id);
			CargaBloque(bloq->id);
		}
		else
		{
			TruncaBloque(cl, bloq, 0);
			EnviaAServidor(":%s DB %s RES %c 0", me.nombre, parv[0], *parv[3]);
			bloq->res = cl;
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
		EnviaAServidor(":%s 339 %s :%i %i %lu %lu %lX", me.nombre, cl->nombre, root->id, root->regs, root->lof, root->gmt, root->crc32);
	Free(pos);
	return 0;
}
IRCFUNC(m_eos_U)
{
	if (cl == linkado)
		UdbCompruebaOpts(NULL);
	return 0;
}
IRCFUNC(m_sjoin_U)
{
	Cliente *al = NULL;
	Canal *cn = NULL;
	char *q, *p, tmp[BUFSIZE], mod[6];
	cn = InfoCanal(parv[2], !0);
	strcpy(tmp, parv[parc-1]);
	for (p = tmp; (q = strchr(p, ' ')); p = q)
	{
		q = strchr(p, ' ');
		al = NULL;
		if (q)
			*q++ = '\0';
		mod[0] = '\0';
		while (*p)
		{
			if (*p == '.')
				strcat(mod, "q");
			else if (*p == '$')
				strcat(mod, "a");
			else if (*p == '@')
				strcat(mod, "o");
			else if (*p == '%')
				strcat(mod, "h");
			else if (*p == '+')
				strcat(mod, "v");
			else if (*p == '&')
			{
				p++;
				strcat(mod, "b");
				break;
			}
			else if (*p == '"')
			{
				p++;
				strcat(mod, "e");
				break;
			}
			else
				break;
			p++;
		}
		if (mod[0] != 'b' && mod[0] != 'e') /* es un usuario */
		{
			al = BuscaCliente(p);
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
				Senyal4(SIGN_MODE, al, cn, arr, i);
		}
	}
	if (parc > 4) /* hay modos */
	{
		ProcesaModo(cl, cn, parv + 3, parc - 4);
		Senyal4(SIGN_MODE, cl, cn, parv + 3, parc - 4);
	}
	return 0;
}
int SigSynch()
{
	UDBloq *aux;
	for (aux = ultimo; aux; aux = aux->sig)
		EnviaAServidor(":%s DB %s INF %c %lX %lu", me.nombre, linkado->nombre, aux->letra, aux->crc32, aux->gmt);
	return 0;
}
void UdbDaleCosas(Cliente *cl)
{
	Udb *reg, *bloq;
	if (!(reg = BuscaBloque(cl->nombre, UDB_NICKS)))
		return;
	if (!BuscaBloque(N_SUS_TOK, reg))
	{
		ProcesaModosCliente(cl, "+r");
		if ((bloq = BuscaBloque(N_MOD_TOK, reg)))
			ProcesaModosCliente(cl, bloq->data_char);
		if ((bloq = BuscaBloque(N_OPE_TOK, reg)))
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
int UdbCompruebaOpts(Proc *proc)
{
	time_t hora = time(0);
	u_int i;
	UDBloq *aux;
	if (!SockIrcd)
		return 1;
	if (!proc || proc->time + 1800 < hora)
	{
		for (i = 0; i < BDD_TOTAL; i++)
		{
			aux = CogeDeId(i);
			if (aux->gmt && aux->gmt + 86400 < hora)
			{
				EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, aux->letra, hora);
				OptimizaBloque(aux);
				ActualizaGMT(aux, hora);
			}
		}
		if (proc)
		{
			proc->proc = 0;
			proc->time = hora;
		}
	}
	return 0;
}
#ifdef USA_ZLIB
/* rutinas de www.zlib.net */
#define CHUNK 16384
int zDeflate(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}
int zInflate(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
#endif