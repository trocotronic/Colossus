/*
 * $Id: msn.c,v 1.2 2008-02-11 00:05:43 Trocotronic Exp $ 
 */

#ifdef USA_SSL
#include "struct.h"
#include "ircd.h"
#include "msn.h"

Sock *sckMSNNS = NULL;
char *lc = NULL;
char *DALoginServer = NULL;
char *DALoginPath = NULL;
SOCKFUNC(MSNNSClose);
int trid = 0;
int masterin = 0;
int contactos = 0;
int lsts = 0;
void MSNProcesaMsg(MSNSB *, char *);
MSNFUNC(MSNProcesaRPT);
MSNFUNC(MSNHelp);
MSNFUNC(MSNJoin);
MSNFUNC(MSNPart);
Cliente *msncl = NULL;
int MSNSynch();
int MSNSockClose();
int MSNCMsg(Cliente *, Canal *, char *);
int MSNCPart(Cliente *, Canal *, char *);
int MSNCKick(Cliente *, Canal *, char *);
int MSNCJoin(Cliente *, Canal *);
MSNCn *msncns = NULL;

struct MSNComs msncoms[] = {
	{ "RPT" , MSNProcesaRPT , 2 , 1 } ,
	{ "HELP" , MSNHelp , 2 , 0 } ,
	{ "JOIN" , MSNJoin , 2 , 0 } ,
	{ "PART" , MSNPart , 2 , 0 } ,
	{ 0x0 , 0x0 , 0x0 }
};
MSNSB *msnsbs = NULL;

MSNSB *BuscaSB(Sock *sck)
{
	MSNSB *aux;
	for (aux = msnsbs; aux; aux = aux->sig)
	{
		if (sck == aux->SBsck)
			return aux;
	}
	return NULL;
}
MSNSB *BuscaSBCuenta(char *cuenta)
{
	MSNSB *aux;
	for (aux = msnsbs; aux; aux = aux->sig)
	{
		if (!strcasecmp(cuenta, aux->SBcuenta))
			return aux;
	}
	return NULL;
}
int BorraSB(MSNSB *sb, int fuerza)
{
	if (fuerza)
	{
		MSNCn *mcn;
		MSNLSB *lsb;
		for (mcn = sb->mcn; mcn; mcn = mcn->sig)
		{
			for (lsb = mcn->lsb; lsb; lsb = lsb->sig)
			{
				if (!strcasecmp(lsb->cuenta, sb->SBcuenta))
				{
					ircfree(lsb->cuenta);
					LiberaItem(lsb, mcn->lsb);
					break;
				}
			}
		}
	}
	BorraItem(sb, msnsbs);
	if (!EsCerr(sb->SBsck))
		SockClose(sb->SBsck, LOCAL);
	ircfree(sb->SBToken);
	ircfree(sb->SBId);
	ircfree(sb->SBcuenta);
	ircfree(sb);
	return 1;
}
MSNCn *BuscaMCn(char *nombre)
{
	MSNCn *mcn;
	for (mcn = msncns; mcn; mcn = mcn->sig)
	{
		if (!strcasecmp(mcn->mcn->nombre, nombre))
			return mcn;
	}
	return NULL;
}
MSNLSB *BuscaLSB(MSNSB *sb, char *canal)
{
	MSNCn *cn;
	if ((cn = BuscaMCn(canal)))
	{
		MSNLSB *lsb;
		for (lsb = cn->lsb; lsb; lsb = lsb->sig)
		{
			if (!strcasecmp(lsb->cuenta, sb->SBcuenta))
				return lsb;
		}
	}
	return NULL;
}

SOCKFUNC(MSNLoginOpen)
{
	char *ucuenta, *upass;
	ucuenta = URLEncode(conf_msn->cuenta);
	upass = URLEncode(conf_msn->pass);
	SockWrite(sck, "GET /%s HTTP/1.1", DALoginPath);
	SockWrite(sck, "Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%%3A%%2F%%2Fmessenger%%2Emsn%%2Ecom,sign-in=%s,pwd=%s,%s", ucuenta, upass, lc);
	SockWrite(sck, "Host: %s", DALoginServer);
	SockWrite(sck, "");
	Free(ucuenta);
	Free(upass);
	return 0;
}
SOCKFUNC(MSNLoginRead)
{
	if (!strcmp(data, "HTTP/1.1 401 Unauthorized"))
	{
		Loguea(LOG_MSN, "Cuenta o password incorrectos. Autentificación fallida.");
		return 1;
	}
	else if (!strncmp(data, "Authentication-Info:", 20))
	{
		char *param;
		strtok(data, " ");
		strtok(NULL, " ");
		param = strtok(NULL, " ");
		for (param = strtok(param, ","); param; param = strtok(NULL, ","))
		{
			if (!strncmp(param, "from-PP=", 8))
			{
				strtok(param, "'");
				param = strtok(NULL, "'");
				SockWrite(sckMSNNS, "USR 4 TWN S %s", param);
			}
		}
	}
	else if (!strncmp(data, "Location:", 9))
	{
		char *param;
		SockClose(sck, LOCAL);
		strtok(data, " ");
		param = strtok(NULL, " ");
		param = strstr(param, "//")+2;
		DALoginServer = strtok(param, "/");
		DALoginPath = strtok(NULL, "/");
		if (!SockOpenEx(DALoginServer, 443, MSNLoginOpen, MSNLoginRead, NULL, NULL, 30, 0, OPT_SSL))
		{
			Loguea(LOG_MSN, "No puede conectar al LoginServer.");
		 	return 1;
		}
	}
	return 0;
}
SOCKFUNC(MSNNexusOpen)
{
	SockWrite(sck, "GET /rdr/pprdr.asp HTTP/1.0");
	SockWrite(sck, "");
	return 0;
}
SOCKFUNC(MSNNexusRead)
{
	if (!strncmp(data, "PassportURLs:", 13))
	{
		char *param;
		strtok(data, " ");
		param = strtok(NULL, " ");
		for (param = strtok(param, ","); param; param = strtok(NULL, ","))
		{
			if (!strncmp(param, "DALogin=", 7))
			{
				strtok(param, "=");
				DALoginServer = strtok(NULL, "=");
				DALoginServer = strtok(DALoginServer, "/");
				DALoginPath = strtok(NULL, "/");
				if (!SockOpenEx(DALoginServer, 443, MSNLoginOpen, MSNLoginRead, NULL, NULL, 30, 0, OPT_SSL))
				{
					Loguea(LOG_MSN, "No puede conectar al LoginServer.");
				 	return 1;
				}
			}
		}
	}
	return 0;
}
SOCKFUNC(MSNSBOpen)
{
	MSNSB *sb;
	if (!(sb = BuscaSB(sck)))
		return 1;
	if (sb->SBId)
		SockWrite(sck, "ANS %i %s %s %s", ++trid, conf_msn->cuenta, sb->SBToken, sb->SBId);
	else
		SockWrite(sck, "USR %i %s %s", ++trid, conf_msn->cuenta, sb->SBToken);
	return 0;
}
SOCKFUNC(MSNSBRead)
{
	char *c;
	MSNSB *sb;
	Debug("[Parseando: %s]", data);
	if (!(sb = BuscaSB(sck)))
		return 1;
	if (!strncmp(data, "MSG", 3))
	{
		char *msg, *acc, *cuenta;
		if ((msg = strstr(data, "\r\n\r\n")))
		{
			*msg = '\0';
			msg += 4;
		}
		cuenta = strtok(data, "\r\n")+4;
		strtok(NULL, "\r\n");
		if (!strncmp(strtok(NULL, "\r\n"), "Content-Type: text/plain", 24))
			MSNProcesaMsg(sb, msg);
	}
	else if (!strncmp(data, "USR", 3))
		SockWrite(sck, "CAL %i %s", ++trid, sb->SBcuenta);
	else if (!strncmp(data, "JOI", 3))
	{
		strtok(data, " ");
		SendMSN(sb->SBcuenta, NULL);
	}
	else if (!strncmp(data, "BYE", 3))
	{
		char *c, *param;
		strtok(data, " ");
		param = strtok(NULL, " ");
		if ((c = strchr(param, '\r')))
			*c = '\0';
		else if ((c = strchr(param, '\n')))
			*c = '\0';
		SockClose(sb->SBsck, REMOTO);
	}
	return 0;
}
SOCKFUNC(MSNSBClose)
{
	MSNSB *sb;
	if (!(sb = BuscaSB(sck)))
		return 1;
	BorraSB(sb, data ? 1 : 0);
	return 0;
}
SOCKFUNC(MSNNSOpen)
{
	SockWrite(sck, MSN_VER);
	return 0;
}
SOCKFUNC(MSNNSRead)
{
	if (!strncmp(data, "VER", 3))
	{
		if (!strcmp(MSN_VER, data))
			SockWrite(sck, "CVR 2 0x0409 win 4.10 i386 MSNMSGR 7.0.0816 MSMSGS %s", conf_msn->cuenta);
		else
		{
			Loguea(LOG_MSN, "No se puede verificar la versión");
			SockClose(sck, LOCAL);
		}
	}
	else if (!strncmp(data, "CVR", 3))
		SockWrite(sck, "USR 3 TWN I %s", conf_msn->cuenta);
	else if (!strncmp(data, "XFR", 3))
	{
		char *com;
		strtok(data, " ");
		strtok(NULL, " ");
		com = strtok(NULL, " ");
		if (!strcmp(com, "SB"))
		{
			char *serv, *port, *tok;
			MSNSB *sb;
			Sock *sck;
			serv = strtok(NULL, " ");
			strtok(NULL, " ");
			tok = strtok(NULL, " ");
			serv = strtok(serv, ":");
			port = strtok(NULL, ":");
			for (sb = msnsbs; sb; sb = sb->sig)
			{
				if (!sb->SBsck)
					break;
			}
			if (!sb)
				return 1;
			ircstrdup(sb->SBToken, tok);
			if ((sck = SockOpenEx(serv, atoi(port), MSNSBOpen, MSNSBRead, NULL, MSNSBClose, 30, 0, OPT_NORECVQ)))
				sb->SBsck = sck;
		}
		else if (!strcmp(com, "NS"))
		{
			char *serv, *port;
			SockClose(sck, LOCAL);
			if ((serv = strtok(NULL, " ")))
			{
				serv = strtok(serv, ":");
				port = strtok(NULL, ":");
				if (!(sckMSNNS = SockOpen(serv, atoi(port), MSNNSOpen, MSNNSRead, NULL, MSNNSClose)))
					Loguea(LOG_MSN, "No puede conectar al servidor NS.");
			}
		}
	}
	else if (!strncmp(data, "USR", 3))
	{
		if (strstr(data, "TWN "))
		{
			if (!(lc = strstr(data, "lc=")))
			{
				Loguea(LOG_MSN, "No encuentra lc.");
				SockClose(sck, LOCAL);
				return 1;
			}
			if (!SockOpen("nexus.passport.com", 80, MSNNexusOpen, MSNNexusRead, NULL, NULL))
			{
				Loguea(LOG_MSN, "No puede conectar al Nexus.");
				SockClose(sck, LOCAL);
				return 1;
			}
		}
		else if (strstr(data, "OK "))
		{
			trid = 4;
			masterin = 0;
			SockWrite(sck, "SYN %i 0", ++trid);
		}
	}
	else if (!strncmp(data, "CHL", 3))
	{
		char *param, tmp[64];
		strtok(data, " ");
		strtok(NULL, " ");
		param = strtok(NULL, " ");
		ircsprintf(tmp, "%s%s", param, "Q1P7W2E4J9R8U3S5");
		SockWrite(sck, "QRY %i msmsgs@msnmsgr.com 32", ++trid);
		SockWriteEx(sck, 0, "%s", MDString(tmp, strlen(tmp)));
	}
	else if (!strncmp(data, "LST", 3))
	{
		if (atoi(&data[strlen(data)-1]) == 8)
		{
			char *param;
			strtok(data, " ");
			param = strtok(NULL, " ");
			if (!conf_msn->solomaster || !strcasecmp(param, conf_msn->master))
			{
				char *name = strtok(NULL, " ");
				SockWrite(sck, "ADD %i AL %s %s", ++trid, param, name);
				SockWrite(sck, "ADD %i FL %s %s", ++trid, param, name);
			}
		}
		if (++lsts == contactos)
			SockWrite(sck, "CHG %i NLN 0", ++trid);
	}
	else if (!strncmp(data, "ADD", 3))
	{
		char *param;
		strtok(data, " ");
		if (atoi(strtok(NULL, " ")) == 0)
		{
			strtok(NULL, " ");
			strtok(NULL, " ");
			param = strtok(NULL, " ");
			if (!conf_msn->solomaster || !strcasecmp(param, conf_msn->master))
			{
				char *name = strtok(NULL, " ");
				SockWrite(sck, "ADD %i AL %s %s", ++trid, param, name);
				SockWrite(sck, "ADD %i FL %s %s", ++trid, param, name);
			}
		}
	}
	else if (!strncmp(data, "REM", 3))
	{
		char *param;
		strtok(data, " ");
		if (atoi(strtok(NULL, " ")) == 0)
		{
			strtok(NULL, " ");
			strtok(NULL, " ");
			param = strtok(NULL, " ");
			if (!conf_msn->solomaster || !strcasecmp(param, conf_msn->master))
			{
				SockWrite(sck, "REM %i FL %s", ++trid, param);
				SockWrite(sck, "REM %i AL %s", ++trid, param);
			}
		}
	}
	else if (!strncmp(data, "RNG", 3))
	{
		char *serv, *port, *cuenta, *id, *tok;
		MSNSB *sb;
		strtok(data, " ");
		id = strtok(NULL, " ");
		serv = strtok(NULL, " ");
		strtok(NULL, " ");
		tok = strtok(NULL, " ");
		cuenta = strtok(NULL, " ");
		serv = strtok(serv, ":");
		port = strtok(NULL, ":");
		if (!(sb = BuscaSBCuenta(cuenta)))
		{
			sb = BMalloc(MSNSB);
			sb->SBcuenta = strdup(cuenta);
			AddItem(sb, msnsbs);
		}
		else if (sb->SBsck)
			SockClose(sb->SBsck, LOCAL);
		ircstrdup(sb->SBId, id);
		ircstrdup(sb->SBToken, tok);
		sb->SBsck = SockOpenEx(serv, atoi(port), MSNSBOpen, MSNSBRead, NULL, MSNSBClose, 30, 0, OPT_NORECVQ);
	}
	else if (!strncmp(data, "ILN", 3))
	{
		strtok(data, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		if (!strcasecmp(strtok(NULL, " "), conf_msn->master))
			masterin = 1;
	}
	else if (!strncmp(data, "NLN", 3))
	{
		strtok(data, " ");
		strtok(NULL, " ");
		if (!strcasecmp(strtok(NULL, " "), conf_msn->master))
			masterin = 1;
	}
	else if (!strncmp(data, "FLN", 3))
	{
		char *cuenta;
		MSNSB *sb;
		strtok(data, " ");
		cuenta = strtok(NULL, " ");
		if (!strcasecmp(cuenta, conf_msn->master))
			masterin = 0;
		if ((sb = BuscaSBCuenta(cuenta)))
			BorraSB(sb, 1);
	}
	else if (!strncmp(data, "CHG", 3))
	{
		char *coded = URLEncode(COLOSSUS_VERSION);
		SockWrite(sck, "REA %i %s %s", ++trid, conf_msn->cuenta, coded);
		Free(coded);
	}
	else if (!strncmp(data, "REA", 3))
	{
		strtok(data, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		if (!strcasecmp(strtok(NULL, " "), conf_msn->cuenta))
			SockWrite(sck, "PNG");
	}
	else if (!strncmp(data, "SYN", 3))
	{
		strtok(data, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		contactos = atoi(strtok(NULL, " "));
		lsts = 0;
	}
	else if (!strncmp(data, "QNG", 3))
		SendMSN(conf_msn->master, "Soy el complemento MSN de %s. Para cualquier duda que tenga, utilice el comando HELP.", COLOSSUS_VERSION);
	return 0;
}
SOCKFUNC(MSNNSClose)
{
	sckMSNNS = NULL;
	return 0;
}
int CargaMSN()
{
	MSNSB *sb, *sig;
	if (!conf_msn)
		return 1;
	if (sckMSNNS)
		SockClose(sckMSNNS, LOCAL);
	for (sb = msnsbs; sb; sb = sig)
	{
		sig = sb->sig;
		BorraSB(sb, 1);
	}
	msnsbs = NULL;
	if (!(sckMSNNS = SockOpen("messenger.hotmail.com", 1863, MSNNSOpen, MSNNSRead, NULL, MSNNSClose)))
		return 1;
	InsertaSenyal(SIGN_SYNCH, MSNSynch);
	InsertaSenyal(SIGN_SOCKCLOSE, MSNSockClose);
	InsertaSenyal(SIGN_CMSG, MSNCMsg);
	InsertaSenyal(SIGN_PART, MSNCPart);
	InsertaSenyal(SIGN_KICK, MSNCKick);
	InsertaSenyal(SIGN_JOIN, MSNCJoin);
	return 0;
}
int DescargaMSN()
{
	MSNSB *sb, *sig;
	if (sckMSNNS)
		SockClose(sckMSNNS, LOCAL);
	for (sb = msnsbs; sb; sb = sig)
	{
		sig = sb->sig;
		BorraSB(sb, 1);
	}
	msnsbs = NULL;
	BorraSenyal(SIGN_SYNCH, MSNSynch);
	BorraSenyal(SIGN_SOCKCLOSE, MSNSockClose);
	BorraSenyal(SIGN_CMSG, MSNCMsg);
	BorraSenyal(SIGN_PART, MSNCPart);
	BorraSenyal(SIGN_KICK, MSNCKick);
	BorraSenyal(SIGN_JOIN, MSNCJoin);
	return 0;
}
int SendMSN(char *cuenta, char *msg, ...)
{
	MSNSB *sb;
	if (!(sb = BuscaSBCuenta(cuenta)))
	{
		sb = BMalloc(MSNSB);
		sb->SBcuenta = strdup(cuenta);
		if (!strcasecmp(cuenta, conf_msn->master))
			sb->master = 1;
		AddItem(sb, msnsbs);
	}
	if (sb->master && !masterin)
		return 1;
	if (msg)
	{
		va_list vl;
		va_start(vl, msg);
		ircvsprintf(sb->SBMsg, msg, vl);
		va_end(vl);
	}
	if (!sb->SBsck)
		SockWrite(sckMSNNS, "XFR %i SB", ++trid);
	else
	{
		SockWrite(sb->SBsck, "MSG %i U %i", ++trid, 113+strlen(sb->SBMsg));
		SockWrite(sb->SBsck, "MIME-Version: 1.0");
		SockWrite(sb->SBsck, "Content-Type: text/plain; charset=UTF-8");
		SockWrite(sb->SBsck, "X-MMS-IM-Format: FN=Arial; EF=B; CO=0; CS=0; PF=0");
		SockWrite(sb->SBsck, "");
		SockWriteEx(sb->SBsck, 0, sb->SBMsg);
	}
	return 0;
}
void MSNProcesaMsg(MSNSB *sb, char *msg)
{
	char *argv[16], *com;
	int argc = 0;
	struct MSNComs *mcom;
	com = strtok(msg, " ");
	if (*com == '#')
	{
		MSNLSB *lsb;
		if (!(lsb = BuscaLSB(sb, com)))
		{
			SendMSN(sb->SBcuenta, "No estás en ese canal");
			return;
		}
		if (!(argv[0] = strtok(NULL, "")))
		{
			SendMSN(sb->SBcuenta, "Faltan parámetros");
			return;
		}
		Responde((Cliente *)lsb->mcn->mcn, msncl, "%s@***** > %s", strtok(sb->SBcuenta, "@"), argv[0]);
		return;
	}
	for (mcom = msncoms; mcom->com; mcom++)
	{
		if (!strcasecmp(mcom->com, com))
			break;
	}
	if (!mcom->com)
	{
		SendMSN(sb->SBcuenta, "Comando desconocido.");
		return;
	}
	if (mcom->master && !sb->master)
	{
		SendMSN(sb->SBcuenta, "Acceso denegado.");
		return;
	}
	if (conf_msn->solomaster && !sb->master)
	{
		SendMSN(sb->SBcuenta, "Función deshabilitada. Acceso sólo para masters.");
		return;
	}
	if (mcom->params == 1)
		mcom->func(sb, NULL, 0);
	else if (mcom->params == 2)
	{
		if (!(argv[0] = strtok(NULL, "")))
		{
			SendMSN(sb->SBcuenta, "Faltan parámetros");
			return;
		}
		mcom->func(sb, argv, 1);
	}
	else
	{
		int params = MIN(15, mcom->params-2);
		for (argv[argc] = strtok(NULL, " "); argv[argc]; argv[argc] = strtok(NULL, " "))
		{
			if (++argc == params)
			{
				argv[argc] = strtok(NULL, "");
				break;
			}
		}
		if (argc+1 < params)
		{
			SendMSN(sb->SBcuenta, "Faltan parámetros");
			return;
		}
		mcom->func(sb, argv, argc);
	}
}
MSNFUNC(MSNProcesaRPT)
{
	SendMSN(sb->SBcuenta, argv[0]);
	return 0;
}
MSNFUNC(MSNHelp)
{
	ircsprintf(buf, "Complemento MSN para %s.\r\n\r\n"
		"El complemento MSN es una pasarela entre el Messenger y el IRC. Los comandos permitidos son:", COLOSSUS_VERSION);
	SendMSN(sb->SBcuenta, buf);
	ircsprintf(buf, "HELP Muestra esta ayuda\r\n"
				"JOIN Entra en un canal y te permite ver todo lo que acontece\r\n"
				"PART Sale de un canal");
	SendMSN(sb->SBcuenta, buf);
	return 0;
}
MSNFUNC(MSNJoin)
{
	MSNCn *mcn;
	Canal *icn;
	MSNLSB *lsb;
	if (!SockIrcd)
	{
		SendMSN(sb->SBcuenta, "El sistema no está conectado al IRC");
		return 1;
	}
	if (!(icn = BuscaCanal(argv[0])))
	{
		SendMSN(sb->SBcuenta, "Este canal no existe");
		return 1;
	}
	if (!(mcn = BuscaMCn(argv[0])))
	{
		mcn = BMalloc(MSNCn);
		mcn->mcn = icn;
		EntraBot(msncl, argv[0]);
		AddItem(mcn, msncns);
	}
	if (!(lsb = BuscaLSB(sb, argv[0])))
	{
		lsb = BMalloc(MSNLSB);
		ircstrdup(lsb->cuenta, sb->SBcuenta);
		lsb->mcn = mcn;
		AddItem(lsb, mcn->lsb);
	}
	AddItem(mcn, sb->mcn);
	return 0;
}
MSNFUNC(MSNPart)
{
	MSNCn *mcn;
	MSNLSB *lsb;
	if (!SockIrcd)
	{
		SendMSN(sb->SBcuenta, "El sistema no está conectado al IRC");
		return 1;
	}
	if (!(lsb = BuscaLSB(sb, argv[0])))
	{
		SendMSN(sb->SBcuenta, "No estás dentro");
		return 1;
	}
	mcn = lsb->mcn;
	BorraItem(mcn, sb->mcn);
	LiberaItem(lsb, mcn->lsb);
	if (!mcn->lsb)
	{
		SacaBot(msncl, argv[0], NULL);
		LiberaItem(mcn, msncns);
	}
	return 0;
}
int MSNSynch()
{
	if (conf_msn->nick && (msncl = CreaBot(conf_msn->nick, "colossus", me.nombre, "kBq", "Complemento MSN")))
	{
	}
	return 0;
}	
int MSNSockClose()
{
	msncl = NULL;
	return 0;
}
int MSNCMsg(Cliente *cl, Canal *cn, char *msg)
{
	MSNCn *mcn;
	MSNLSB *lsb;
	if ((mcn = BuscaMCn(cn->nombre)))
	{
		for (lsb = mcn->lsb; lsb; lsb = lsb->sig)
			SendMSN(lsb->cuenta, "[%s] %s > %s", cn->nombre, cl->nombre, msg);
	}
	return 0;
}
int MSNCPart(Cliente *cl, Canal *cn, char *msg)
{
	MSNCn *mcn;
	MSNLSB *lsb;
	if ((mcn = BuscaMCn(cn->nombre)))
	{
		for (lsb = mcn->lsb; lsb; lsb = lsb->sig)
			SendMSN(lsb->cuenta, "[%s] %s Sale > %s", cn->nombre, cl->nombre, msg ? msg : "");
	}
	return 0;
}
int MSNCKick(Cliente *cl, Canal *cn, char *msg)
{
	MSNCn *mcn;
	MSNLSB *lsb;
	if ((mcn = BuscaMCn(cn->nombre)))
	{
		for (lsb = mcn->lsb; lsb; lsb = lsb->sig)
			SendMSN(lsb->cuenta, "[%s] %s Pateado > %s", cn->nombre, cl->nombre, msg);
	}
	return 0;
}
int MSNCJoin(Cliente *cl, Canal *cn)
{
	MSNCn *mcn;
	MSNLSB *lsb;
	if ((mcn = BuscaMCn(cn->nombre)))
	{
		for (lsb = mcn->lsb; lsb; lsb = lsb->sig)
			SendMSN(lsb->cuenta, "[%s] %s Entra", cn->nombre, cl->nombre);
	}
	return 0;
}
#endif
