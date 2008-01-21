/*
 * $Id: msn.c,v 1.1 2008-01-21 19:51:13 Trocotronic Exp $ 
 */

#ifdef USA_SSL
#include "struct.h"

#define MSN_VER "VER 1 MSNP8 CVR0"

Sock *sckMSNNS = NULL;
Sock *sckMSNSB = NULL;
char *lc = NULL;
char *DALoginServer = NULL;
char *DALoginPath = NULL;
char *SBId = NULL;
char *SBToken = NULL;
char SBMsg[BUFSIZE];
SOCKFUNC(MSNNSClose);
int trid = 0;
int masterin = 0;
int contactos = 0;
int lsts = 0;
void MSNProcesaMsg(char *);
struct MSNComs
{
	char *com;
	int (*func)(char **, int);
	int params;
};
int MSNProcesaRPT(char **, int);
struct MSNComs msncoms[] = {
	{ "RPT" , MSNProcesaRPT , 2 } ,
	{ 0x0 , 0x0 , 0x0 }
};

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
	if (SBId)
		SockWrite(sck, "ANS %i %s %s %s", ++trid, conf_msn->cuenta, SBToken, SBId);
	else
		SockWrite(sck, "USR %i %s %s", ++trid, conf_msn->cuenta, SBToken);
	return 0;
}
SOCKFUNC(MSNSBRead)
{
	char *c;
	Debug("[Parseando: %s]", data);
	if (!strncmp(data, "MSG", 3))
	{
		char *msg, *acc;
		if ((msg = strstr(data, "\r\n\r\n")))
		{
			*msg = '\0';
			msg += 4;
		}
		if (!strncasecmp(strtok(data, "\r\n")+4, conf_msn->master, strlen(conf_msn->master)))
		{
			strtok(NULL, "\r\n");
			if (!strncmp(strtok(NULL, "\r\n"), "Content-Type: text/plain", 24))
				MSNProcesaMsg(msg);
		}
	}
	else if (!strncmp(data, "USR", 3))
		SockWrite(sck, "CAL %i %s", ++trid, conf_msn->master);
	else if (!strncmp(data, "JOI", 3))
	{
		strtok(data, " ");
		if (!strcasecmp(strtok(NULL, " "), conf_msn->master))
			SendMSN(NULL);
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
		if (!strcasecmp(param, conf_msn->master))
			SockClose(sckMSNSB, LOCAL);
	}
	return 0;
}
SOCKFUNC(MSNSBClose)
{
	sckMSNSB = NULL;
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
			char *serv, *port;
			serv = strtok(NULL, " ");
			strtok(NULL, " ");
			SBToken = strtok(NULL, " ");
			SBId = NULL;
			serv = strtok(serv, ":");
			port = strtok(NULL, ":");
			sckMSNSB = SockOpenEx(serv, atoi(port), MSNSBOpen, MSNSBRead, NULL, MSNSBClose, 30, 0, OPT_NORECVQ);
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
			if (!strcasecmp(param, conf_msn->master))
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
			if (!strcasecmp(param, conf_msn->master))
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
			if (!strcasecmp(param, conf_msn->master))
			{
				SockWrite(sck, "REM %i FL %s", ++trid, param);
				SockWrite(sck, "REM %i AL %s", ++trid, param);
			}
		}
	}
	else if (!strncmp(data, "RNG", 3))
	{
		char *serv, *port;
		strtok(data, " ");
		SBId = strtok(NULL, " ");
		serv = strtok(NULL, " ");
		strtok(NULL, " ");
		SBToken = strtok(NULL, " ");
		serv = strtok(serv, ":");
		port = strtok(NULL, ":");
		if (sckMSNSB)
			SockClose(sckMSNSB, LOCAL);
		sckMSNSB = SockOpenEx(serv, atoi(port), MSNSBOpen, MSNSBRead, NULL, MSNSBClose, 30, 0, OPT_NORECVQ);
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
		strtok(data, " ");
		if (!strcasecmp(strtok(NULL, " "), conf_msn->master))
			masterin = 0;
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
		SendMSN("Cliente conecta");
	return 0;
}
SOCKFUNC(MSNNSClose)
{
	sckMSNNS = NULL;
	return 0;
}
int CargaMSN()
{
	if (!conf_msn)
		return 1;
	if (sckMSNNS)
		SockClose(sckMSNNS, LOCAL);
	if (sckMSNSB)
		SockClose(sckMSNSB, LOCAL);
	if (!(sckMSNNS = SockOpen("messenger.hotmail.com", 1863, MSNNSOpen, MSNNSRead, NULL, MSNNSClose)))
		return 1;
	return 0;
}
int DescargaMSN()
{
	if (sckMSNNS)
		SockClose(sckMSNNS, LOCAL);
	if (sckMSNSB)
		SockClose(sckMSNSB, LOCAL);
	return 0;
}
int SendMSN(char *msg, ...)
{
	if (!masterin)
		return 1;
	if (msg)
	{
		va_list vl;
		va_start(vl, msg);
		ircvsprintf(SBMsg, msg, vl);
		va_end(vl);
	}
	if (!sckMSNSB)
		SockWrite(sckMSNNS, "XFR %i SB", ++trid);
	else
	{
		SockWrite(sckMSNSB, "MSG %i U %i", ++trid, 113+strlen(SBMsg));
		SockWrite(sckMSNSB, "MIME-Version: 1.0");
		SockWrite(sckMSNSB, "Content-Type: text/plain; charset=UTF-8");
		SockWrite(sckMSNSB, "X-MMS-IM-Format: FN=Arial; EF=B; CO=0; CS=0; PF=0");
		SockWrite(sckMSNSB, "");
		SockWriteEx(sckMSNSB, 0, SBMsg);
	}
	return 0;
}
void MSNProcesaMsg(char *msg)
{
	char *argv[16], *com;
	int argc = 0;
	struct MSNComs *mcom;
	com = strtok(msg, " ");
	for (mcom = msncoms; mcom->com; mcom++)
	{
		if (!strcasecmp(mcom->com, com))
			break;
	}
	if (!mcom->com)
	{
		SendMSN("Comando desconocido.");
		return;
	}
	if (mcom->params == 1)
		mcom->func(NULL, 0);
	else if (mcom->params == 2)
	{
		argv[0] = strtok(NULL, "");
		mcom->func(argv, 1);
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
		mcom->func(argv, argc);
	}
}
int MSNProcesaRPT(char **argv, int argc)
{
	SendMSN(argv[0]);
	return 0;
}
#endif
