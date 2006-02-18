#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"
#include "modulos/operserv.h"
#include "modulos/ipserv.h"
#include "bdd.h"

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
	3.22,
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
	BddInit();
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
	InsertaSenyal(SIGN_SQL, (int(*)())CargaBloques);
	InsertaSenyal(SIGN_EOS, SigEOS);
	InsertaSenyal(SIGN_PRE_NICK, SigPreNick);
	InsertaSenyal(SIGN_POST_NICK, SigPostNick_U);
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
		if ((reg = BuscaRegistro(BDD_NICKS, cl->nombre)) && (bloq = BuscaBloque(N_MOD_TOK, reg)))
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
	if (match(parv[1], me.nombre))
		return 0;
	if (!strcasecmp(parv[2], "INF"))
	{
		Udb *bloq;
		time_t gm;
		bloq = IdAUdb(*parv[3]);
		gm = atoul(parv[5]);
		if (strcmp(parv[4], bloq->data_char))
		{
			if (gm > gmts[bloq->id])
			{
				TruncaBloque(cl, bloq, 0);
				EnviaAServidor(":%s DB %s RES %c 0", me.nombre, parv[0], *parv[3]);
				ActualizaGMT(bloq, gm);
				if (++bloqs == BDD_TOTAL)
					EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
			}
			else if (gm == gmts[bloq->id])
				EnviaAServidor(":%s DB %s RES %c %lu", me.nombre, parv[0], *parv[3], bloq->data_long);
			/* si es menor, el otro nodo vaciará su db y nos mandará un RES, será cuando empecemos el resumen. abremos terminado nuestro burst */
		}
		else
		{
			if (++bloqs == BDD_TOTAL)
				EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
		}
	}
	else if (!strcasecmp(parv[2], "RES"))
	{
		u_long bytes;
		Udb *bloq;
		bloq = IdAUdb(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes < bloq->data_long) /* tiene menos, se los mandamos */
		{
			FILE *fp;
			if ((fp = fopen(bloq->item, "rb")))
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
				EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
			}
		}
		else if (!bytes && !bloq->data_long)
			EnviaAServidor(":%s DB %s FDR %c 0", me.nombre, cl->nombre, *parv[3]);
		if (++bloqs == BDD_TOTAL)
				EnviaAServidor(":%s %s", me.nombre, TOK_EOS);
	}
	else if (!strcasecmp(parv[2], "INS"))
	{
		char buf[1024], tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr(bloques, tipo))
		{
			EnviaAServidor(":%s DB %s ERR INS %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = IdAUdb(tipo);
		if (bytes != bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR INS %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		ircsprintf(buf, "%s %s", r, Unifica(parv, parc, 5, -1));
		ParseaLinea(bloq->id, buf, 1);
	}
	else if (!strcasecmp(parv[2], "DEL"))
	{
		char tipo, *r = parv[4];
		u_long bytes;
		Udb *bloq;
		tipo = *r;
		if (!strchr(bloques, tipo))
		{
			EnviaAServidor(":%s DB %s ERR DEL %i %c", me.nombre, cl->nombre, E_UDB_NODB, tipo);
			return 1;
		}
		bytes = atoul(parv[3]);
		bloq = IdAUdb(tipo);
		if (bytes != bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR DEL %i %c %lu", me.nombre, cl->nombre, E_UDB_LEN, tipo, bloq->data_long);
			return 1;
		}
		r += 3;
		ParseaLinea(bloq->id, r, 1);
	}
	else if (!strcasecmp(parv[2], "DRP"))
	{
		Udb *bloq;
		u_long bytes;
		if (!strchr(bloques, *parv[3]))
		{
			EnviaAServidor(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		bloq = IdAUdb(*parv[3]);
		bytes = atoul(parv[4]);
		if (bytes > bloq->data_long)
		{
			EnviaAServidor(":%s DB %s ERR DRP %i %c", me.nombre, cl->nombre, E_UDB_LEN, *parv[3]);
			return 1;
		}
		TruncaBloque(cl, bloq, bytes);
	}
	else if (!strcmp(parv[2], "ERR")) /* tratamiento de errores */
	{
		int error = 0;
		error = atoi(parv[4]);
		switch (error)
		{
			case E_UDB_LEN:
			{
				Udb *bloq;
				u_long bytes;
				char *cb;
				bloq = IdAUdb(*parv[5]);
				cb = strchr(parv[5], ' ') + 1; /* esto siempre debe cumplirse, si no a cascar */
				bytes = atoul(cb);
				TruncaBloque(cl, bloq, bytes);
				break;
			}
		}
		/* una vez hemos terminado, retornamos puesto que estos comandos sólo van dirigidos a un nodo */
		return 1;
	}
	/* DB * OPT <bdd> <NULL>*/
	else if (!strcmp(parv[2], "OPT"))
	{
		Udb *bloq;
		if (!strchr(bloques, *parv[3]))
		{
			EnviaAServidor(":%s DB %s ERR OPT %i %c", me.nombre, cl->nombre, E_UDB_NODB, *parv[3]);
			return 1;
		}
		bloq = IdAUdb(*parv[3]);
		Optimiza(bloq);
	}
	return 0;
}
IRCFUNC(m_dbq)
{
	char *cur, *pos, *ds;
	Udb *bloq;
	pos = cur = strdup(parv[2]);
	if (!(bloq = IdAUdb(*pos)))
	{
		EnviaAServidor(":%s 339 %s :La base de datos %c no existe.", me.nombre, cl->nombre, *pos);
		return 0;
	}
	if (parc == 3)
		parv[1] = parv[2];
	if (*(++cur) != '\0')
	{
		if (*cur++ != ':' || *cur++ != ':' || *cur == '\0')
		{
			EnviaAServidor(":%s 339 %s :Formato de bloque incorrecto.", me.nombre, cl->nombre);
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
						EnviaAServidor(":%s 339 %s :DBQ %s::%c %lu", me.nombre, cl->nombre, parv[1], aux->id, aux->data_long);
					else if (aux->data_char)
						EnviaAServidor(":%s 339 %s :DBQ %s::%c %s", me.nombre, cl->nombre, parv[1], aux->id, aux->data_char);
					else
						EnviaAServidor(":%s 339 %s :DBQ %s::%c (no tiene datos)", me.nombre, cl->nombre, parv[1], aux->id);
				}
			}
		}
	}
	else
	{
		int id = bloq->id;
		EnviaAServidor(":%s 339 %s :%i %i %lu %s %lu %lX", me.nombre, cl->nombre, id, regs[id], bloq->data_long, bloq->data_char, LeeGMT(id), LeeHash(id));
	} 
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
			al = BuscaCliente(p, NULL);
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
	Udb *aux;
	for (aux = ultimo; aux; aux = aux->mid)
		EnviaAServidor(":%s DB %s INF %c %s %lu", me.nombre, linkado->nombre, bloques[aux->id], aux->data_char, gmts[aux->id]);
	return 0;
}
void UdbDaleCosas(Cliente *cl)
{
	Udb *reg, *bloq;
	if (!(reg = BuscaRegistro(BDD_NICKS, cl->nombre)))
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
	Udb *aux;
	if (!SockIrcd)
		return 1;
	if (!proc || proc->time + 1800 < hora)
	{
		for (i = 0; i < BDD_TOTAL; i++)
		{
			aux = IdAUdb(i);
			if (gmts[i] && gmts[i] + 86400 < hora)
			{
				EnviaAServidor(":%s DB * OPT %c %lu", me.nombre, bloques[i], hora);
				Optimiza(aux);
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
