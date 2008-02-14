/*
 * $Id: msn.c,v 1.10 2008-02-14 16:24:51 Trocotronic Exp $ 
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
u_int trid = 0;
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
int MSNCQuit(Cliente *, char *);
MSNCn *msncns = NULL;
Hash MSNuTab[UMAX];
Hash MSNcTab[CHMAX];
MSNCl *msncls = NULL;
MSNSB *msnsbs = NULL;

struct MSNComs msncoms[] = {
	{ "RPT" , MSNProcesaRPT , 2 , 1 } ,
	{ "HELP" , MSNHelp , 2 , 0 } ,
	{ "JOIN" , MSNJoin , 2 , 0 } ,
	{ "PART" , MSNPart , 2 , 0 } ,
	{ 0x0 , 0x0 , 0x0 }
};
MSNCl *MSNBuscaCliente(char *cuenta)
{
	MSNCl *al = NULL;
	if (cuenta)
	{
		u_int hash;
		hash = HashCliente(cuenta);
		for (al = (MSNCl *)MSNuTab[hash].item; al; al = al->hsig)
		{
			if (!strcasecmp(al->cuenta, cuenta))
				return al;
		}
	}
	return NULL;
}
MSNCn *MSNBuscaCanal(char *canal)
{
	MSNCn *an = NULL;
	if (canal)
	{
		u_int hash;
		hash = HashCanal(canal);
		for (an = (MSNCn *)MSNcTab[hash].item; an; an = an->hsig)
		{
			if (!strcasecmp(an->cn->nombre, canal))
				return an;
		}
	}
	return NULL;
}
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
		if (!strcasecmp(cuenta, aux->mcl->cuenta))
			return aux;
	}
	return NULL;
}
MSNLCl *BuscaLCl(MSNSB *sb, char *canal)
{
	MSNCn *mcn;
	if ((mcn = MSNBuscaCanal(canal)))
	{
		MSNLCl *lcl;
		for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
		{
			if (lcl->mcl == sb->mcl)
				return lcl;
		}
	}
	return NULL;
}
MSNLCn *BuscaLCn(MSNSB *sb, char *canal)
{
	MSNCn *mcn;
	if ((mcn = MSNBuscaCanal(canal)))
	{
		MSNLCn *lcn;
		for (lcn = sb->mcl->lcn; lcn; lcn = lcn->sig)
		{
			if (lcn->mcn == mcn)
				return lcn;
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
	ircfree(DALoginServer);
	ircfree(DALoginPath);
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
		SockClose(sck);
		strtok(data, " ");
		param = strtok(NULL, " ");
		param = strstr(param, "//")+2;
		ircstrdup(DALoginServer, strtok(param, "/"));
		ircstrdup(DALoginPath, strtok(NULL, "/"));
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
				ircstrdup(DALoginServer, strtok(strtok(NULL, "="), "/"));
				ircstrdup(DALoginPath, strtok(NULL, "/"));
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
	ircfree(sb->SBToken);
	ircfree(sb->SBId);
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
		SockWrite(sck, "CAL %i %s", ++trid, sb->mcl->cuenta);
	else if (!strncmp(data, "JOI", 3))
	{
		strtok(data, " ");
		SendMSN(sb->mcl->cuenta, NULL);
	}
	/*else if (!strncmp(data, "BYE", 3))
	{
		char *c, *param;
		strtok(data, " ");
		param = strtok(NULL, " ");
		if ((c = strchr(param, '\r')))
			*c = '\0';
		else if ((c = strchr(param, '\n')))
			*c = '\0';
		//SockClose(sb->SBsck);
	}*/
	return 0;
}
SOCKFUNC(MSNSBClose)
{
	MSNSB *sb;
	if (!(sb = BuscaSB(sck)))
		return 1;
	LiberaItem(sb, msnsbs);
	return 0;
}
SOCKFUNC(MSNNSOpen)
{
	SockWrite(sck, MSN_VER);
#ifdef _WIN32
	SetDlgItemText(hwMain, BT_MSN, "Complemento MSN (ON)");
	CheckDlgButton(hwMain, BT_MSN, BST_CHECKED);
#endif
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
			SockClose(sck);
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
			SockClose(sck);
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
				SockClose(sck);
				return 1;
			}
			if (!SockOpen("nexus.passport.com", 80, MSNNexusOpen, MSNNexusRead, NULL, NULL))
			{
				Loguea(LOG_MSN, "No puede conectar al Nexus.");
				SockClose(sck);
				return 1;
			}
		}
		else if (strstr(data, "OK "))
		{
			trid = 4;
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
		MSNCl *mcl = BMalloc(MSNCl);
		char *cuenta, *nombre, *listas, *grupos;
		strtok(data, " ");
		cuenta = strtok(NULL, " ");
		nombre = strtok(NULL, " ");
		listas = strtok(NULL, " ");
		grupos = strtok(NULL, " ");
		mcl->cuenta = strdup(cuenta);
		mcl->nombre = strdup(nombre);
		mcl->alias = strdup(strtok(cuenta, "@"));
		if (!strcasecmp(cuenta, conf_msn->master))
			mcl->master = 1;
		AddItem(mcl, msncls);
		InsertaClienteEnHash((Cliente *)mcl, cuenta, MSNuTab);
		if (listas && atoi(listas) == 8)
		{
			if (!conf_msn->solomaster || mcl->master)
			{
				SockWrite(sck, "ADD %i AL %s %s", ++trid, cuenta, nombre);
				SockWrite(sck, "ADD %i FL %s %s", ++trid, cuenta, nombre);
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
		MSNCl *mcl;
		strtok(data, " ");
		id = strtok(NULL, " ");
		serv = strtok(NULL, " ");
		strtok(NULL, " ");
		tok = strtok(NULL, " ");
		cuenta = strtok(NULL, " ");
		serv = strtok(serv, ":");
		port = strtok(NULL, ":");
		if ((mcl = MSNBuscaCliente(cuenta)) && !(sb = BuscaSBCuenta(cuenta)))
		{
			sb = BMalloc(MSNSB);
			sb->mcl = mcl;
			AddItem(sb, msnsbs);
			ircstrdup(sb->SBId, id);
			ircstrdup(sb->SBToken, tok);
			sb->SBsck = SockOpenEx(serv, atoi(port), MSNSBOpen, MSNSBRead, NULL, MSNSBClose, 30, 0, OPT_NORECVQ);
		}
	}
	else if (!strncmp(data, "ILN", 3))
	{
		char *cuenta;
		MSNCl *mcl;
		strtok(data, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		cuenta = strtok(NULL, " ");
		if ((mcl = MSNBuscaCliente(cuenta)))
			mcl->estado = 1;
	}
	else if (!strncmp(data, "NLN", 3))
	{
		char *cuenta;
		MSNCl *mcl;
		strtok(data, " ");
		strtok(NULL, " ");
		cuenta = strtok(NULL, " ");
		if ((mcl = MSNBuscaCliente(cuenta)))
			mcl->estado = 1;
	}
	else if (!strncmp(data, "FLN", 3))
	{
		char *cuenta;
		MSNSB *sb;
		MSNCl *mcl;
		MSNLCn *lcn, *ccsig;
		MSNLCl *lcl;
		strtok(data, " ");
		cuenta = strtok(NULL, " ");
		if ((mcl = MSNBuscaCliente(cuenta)))
			mcl->estado = 0;
		for (lcn = mcl->lcn; lcn; lcn = ccsig)
		{
			ccsig = lcn->sig;
			for (lcl = lcn->mcn->lcl; lcl; lcl = lcl->sig)
			{
				if (lcl->mcl == mcl)
				{
					LiberaItem(lcl, lcn->mcn->lcl);
					Responde((Cliente *)lcn->mcn->cn, msncl, "%s@***** Cierra", mcl->alias);
					break;
				}
			}
			Free(lcn);
		}
		mcl->lcn = NULL;
		if ((sb = BuscaSBCuenta(cuenta)))
			LiberaItem(sb, msnsbs);
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
	{
		SockWrite(sck, "BLP %i BL", ++trid);
		SendMSN(conf_msn->master, "Soy el complemento MSN de %s. Para cualquier duda que tenga, utilice el comando HELP.", COLOSSUS_VERSION);
	}
	return 0;
}
SOCKFUNC(MSNNSClose)
{
	MSNSB *sb, *bsig;
	MSNCl *mcl, *lsig;
	MSNCn *mcn, *csig;
	MSNLCl *lcl, *llsig;
	MSNLCn *lcn, *ccsig;
	sckMSNNS = NULL;
#ifdef _WIN32
	SetDlgItemText(hwMain, BT_MSN, "Complemento MSN (OFF)");
	CheckDlgButton(hwMain, BT_MSN, BST_UNCHECKED);
#endif
	for (mcl = msncls; mcl; mcl = lsig)
	{
		lsig = mcl->sig;
		for (lcn = mcl->lcn; lcn; lcn = ccsig)
		{
			ccsig = lcn->sig;
			Free(lcn);
		}
		Free(mcl->cuenta);
		Free(mcl->nombre);
		Free(mcl);
	}
	for (mcn = msncns; mcn; mcn = csig)
	{
		csig = mcn->sig;
		for (lcl = mcn->lcl; lcl; lcl = llsig)
		{
			llsig = lcl->sig;
			Free(lcl);
		}
		Free(mcn);
	}
	for (sb = msnsbs; sb; sb = bsig)
	{
		bsig = sb->sig;
		Free(sb);
	}
	return 0;
}
int CargaMSN()
{
	int i;
	if (!conf_msn)
		return 1;
	if (sckMSNNS)
		SockClose(sckMSNNS);
	msnsbs = NULL;
	msncls = NULL;
	msncns = NULL;
	for (i = 0; i < UMAX; i++)
	{
		MSNuTab[i].item = NULL;
		MSNuTab[i].items = 0;
	}
	for (i = 0; i < CHMAX; i++)
	{
		MSNcTab[i].item = NULL;
		MSNcTab[i].items = 0;
	}
	if (!(sckMSNNS = SockOpen("messenger.hotmail.com", 1863, MSNNSOpen, MSNNSRead, NULL, MSNNSClose)))
		return 1;
	InsertaSenyal(SIGN_SYNCH, MSNSynch);
	InsertaSenyal(SIGN_SOCKCLOSE, MSNSockClose);
	InsertaSenyal(SIGN_CMSG, MSNCMsg);
	InsertaSenyal(SIGN_PART, MSNCPart);
	InsertaSenyal(SIGN_KICK, MSNCKick);
	InsertaSenyal(SIGN_JOIN, MSNCJoin);
	InsertaSenyal(SIGN_QUIT, MSNCQuit);
	return 0;
}
int DescargaMSN()
{
	if (sckMSNNS)
		SockClose(sckMSNNS);
	BorraSenyal(SIGN_SYNCH, MSNSynch);
	BorraSenyal(SIGN_SOCKCLOSE, MSNSockClose);
	BorraSenyal(SIGN_CMSG, MSNCMsg);
	BorraSenyal(SIGN_PART, MSNCPart);
	BorraSenyal(SIGN_KICK, MSNCKick);
	BorraSenyal(SIGN_JOIN, MSNCJoin);
	BorraSenyal(SIGN_QUIT, MSNCQuit);
	return 0;
}
int SendMSN(char *cuenta, char *msg, ...)
{
	MSNSB *sb;
	MSNCl *mcl;
	if (!(mcl = MSNBuscaCliente(cuenta)))
		return 1;
	if (!mcl->estado)
		return 2;
	if (!(sb = BuscaSBCuenta(cuenta)))
	{
		sb = BMalloc(MSNSB);
		sb->mcl = mcl;
		AddItem(sb, msnsbs);
	}
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
		CambiarCharset("ISO-8859-1", "UTF-8", sb->SBMsg, buf, BUFSIZE);
		SockWrite(sb->SBsck, "MSG %i U %i", ++trid, 113+strlen(buf));
		SockWrite(sb->SBsck, "MIME-Version: 1.0");
		SockWrite(sb->SBsck, "Content-Type: text/plain; charset=UTF-8");
		SockWrite(sb->SBsck, "X-MMS-IM-Format: FN=Arial; EF=B; CO=0; CS=0; PF=0");
		SockWrite(sb->SBsck, "");
		SockWriteEx(sb->SBsck, 0, buf);
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
		MSNLCl *lcl;
		if (!(lcl = BuscaLCl(sb, com)))
		{
			SendMSN(sb->mcl->cuenta, "No estás dentro de %s", com);
			return;
		}
		if (!(argv[0] = strtok(NULL, "")))
		{
			SendMSN(sb->mcl->cuenta, "Faltan parámetros");
			return;
		}
		Responde((Cliente *)lcl->mcn->cn, msncl, "%s@***** > %s", sb->mcl->alias, argv[0]);
		return;
	}
	for (mcom = msncoms; mcom->com; mcom++)
	{
		if (!strcasecmp(mcom->com, com))
			break;
	}
	if (!mcom->com)
	{
		SendMSN(sb->mcl->cuenta, "Comando desconocido.");
		return;
	}
	if (mcom->master && !sb->mcl->master)
	{
		SendMSN(sb->mcl->cuenta, "Acceso denegado.");
		return;
	}
	if (conf_msn->solomaster && !sb->mcl->master)
	{
		SendMSN(sb->mcl->cuenta, "Función deshabilitada. Acceso sólo para masters.");
		return;
	}
	if (mcom->params == 1)
		mcom->func(sb, NULL, 0);
	else if (mcom->params == 2)
	{
		if (!(argv[0] = strtok(NULL, "")))
		{
			SendMSN(sb->mcl->cuenta, "Faltan parámetros");
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
			SendMSN(sb->mcl->cuenta, "Faltan parámetros");
			return;
		}
		mcom->func(sb, argv, argc);
	}
}
MSNFUNC(MSNProcesaRPT)
{
	SendMSN(sb->mcl->cuenta, argv[0]);
	return 0;
}
MSNFUNC(MSNHelp)
{
	ircsprintf(buf, "Complemento MSN para %s.\r\n\r\n"
		"El complemento MSN es una pasarela entre el Messenger y el IRC. Los comandos permitidos son:", COLOSSUS_VERSION);
	SendMSN(sb->mcl->cuenta, buf);
	ircsprintf(buf, "HELP Muestra esta ayuda\r\n"
				"JOIN Entra en un canal y te permite ver todo lo que acontece\r\n"
				"PART Sale de un canal");
	SendMSN(sb->mcl->cuenta, buf);
	return 0;
}
MSNFUNC(MSNJoin)
{
	MSNCn *mcn;
	Canal *cn;
	MSNLCl *lcl;
	MSNLCn *lcn;
	if (!SockIrcd)
	{
		SendMSN(sb->mcl->cuenta, "El sistema no está conectado al IRC");
		return 1;
	}
	if (!(cn = BuscaCanal(argv[0])))
	{
		SendMSN(sb->mcl->cuenta, "Este canal no existe");
		return 1;
	}
	if (cn->privado)
	{
		SendMSN(sb->mcl->cuenta, "Este canal tiene la entrada restringida");
		return 1;
	}
	if (!(mcn = MSNBuscaCanal(argv[0])))
	{
		mcn = BMalloc(MSNCn);
		mcn->cn = cn;
		EntraBot(msncl, argv[0]);
		AddItem(mcn, msncns);
		InsertaCanalEnHash((Canal *)mcn, argv[0], MSNcTab);
	}
	if (!(lcl = BuscaLCl(sb, argv[0])))
	{
		lcl = BMalloc(MSNLCl);
		lcl->mcl = sb->mcl;
		lcl->mcn = mcn;
		AddItem(lcl, mcn->lcl);
	}
	if (!(lcn = BuscaLCn(sb, argv[0])))
	{
		lcn = BMalloc(MSNLCn);
		lcn->mcl = sb->mcl;
		lcn->mcn = mcn;
		AddItem(lcn, sb->mcl->lcn);
		SendMSN(sb->mcl->cuenta, "Ok");
		Responde((Cliente *)lcl->mcn->cn, msncl, "%s@***** Entra", sb->mcl->alias);
	}
	return 0;
}
MSNFUNC(MSNPart)
{
	MSNLCl *lcl;
	MSNLCn *lcn;
	if (!SockIrcd)
	{
		SendMSN(sb->mcl->cuenta, "El sistema no está conectado al IRC");
		return 1;
	}
	if (!(lcl = BuscaLCl(sb, argv[0])) || !(lcn = BuscaLCn(sb, argv[0])))
	{
		SendMSN(sb->mcl->cuenta, "No estás dentro de %s", argv[0]);
		return 1;
	}
	LiberaItem(lcn, sb->mcl->lcn);
	if (!lcl->mcn->lcl->sig)
	{
		SacaBot(msncl, argv[0], NULL);
		BorraCanalDeHash((Canal *)lcl->mcn, lcl->mcn->cn->nombre, MSNcTab);
		LiberaItem(lcl->mcn, msncns);
	}
	LiberaItem(lcl, lcl->mcn->lcl);
	SendMSN(sb->mcl->cuenta, "Ok");
	Responde((Cliente *)lcl->mcn->cn, msncl, "%s@***** Sale", sb->mcl->alias);
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
	MSNLCl *lcl;
	if ((mcn = MSNBuscaCanal(cn->nombre)))
	{
		for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
			SendMSN(lcl->mcl->cuenta, "[%s] %s > %s", cn->nombre, cl->nombre, msg);
	}
	return 0;
}
int MSNCPart(Cliente *cl, Canal *cn, char *msg)
{
	MSNCn *mcn;
	MSNLCl *lcl;
	if ((mcn = MSNBuscaCanal(cn->nombre)))
	{
		for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
			SendMSN(lcl->mcl->cuenta, "[%s] %s Sale > %s", cn->nombre, cl->nombre, msg ? msg : "");
	}
	return 0;
}
int MSNCKick(Cliente *cl, Canal *cn, char *msg)
{
	MSNCn *mcn;
	MSNLCl *lcl;
	if ((mcn = MSNBuscaCanal(cn->nombre)))
	{
		for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
			SendMSN(lcl->mcl->cuenta, "[%s] %s Pateado > %s", cn->nombre, cl->nombre, msg);
	}
	return 0;
}
int MSNCJoin(Cliente *cl, Canal *cn)
{
	MSNCn *mcn;
	MSNLCl *lcl;
	if ((mcn = MSNBuscaCanal(cn->nombre)))
	{
		for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
			SendMSN(lcl->mcl->cuenta, "[%s] %s Entra", cn->nombre, cl->nombre);
	}
	return 0;
}
int MSNCQuit(Cliente *cl, char *motivo)
{
	MSNCn *mcn;
	MSNLCl *lcl;
	LinkCanal *lk;
	for (lk = cl->canal; lk; lk = lk->sig)
	{
		if ((mcn = MSNBuscaCanal(lk->cn->nombre)))
		{
			for (lcl = mcn->lcl; lcl; lcl = lcl->sig)
				SendMSN(lcl->mcl->cuenta, "[%s] %s Cierra > %s", mcn->cn->nombre, cl->nombre, motivo ? motivo : "");
		}
	}
	return 0;
}
#endif
