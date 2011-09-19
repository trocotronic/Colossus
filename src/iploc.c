#include "struct.h"
#include "ircd.h"
#define XML_STATIC
#define IPLOC
#include <expat.h>

int IPLocSignQuit(Cliente *, char *);
int IPLocSignPostNick(Cliente *, int);
SOCKFUNC(IPLocSockOpen);
SOCKFUNC(IPLocSockRead);
SOCKFUNC(IPLocSockClose);
void XMLCALL IPLocXMLFin(IPLocRec *, const XML_Char *);
void XMLCALL IPLocXMLData(IPLocRec *, const XML_Char *, int);

IPLocRec *irecs = NULL;
IPLocRecl *irecls = NULL;

IPLocRec *BuscaIPLocRec(Sock *sck)
{
	IPLocRec *irec;
	for (irec = irecs; irec; irec = irec->sig)
	{
		if (irec->sck == sck)
			return irec;
	}
	return NULL;
}
int IniciaIPLoc()
{
#ifndef IPLOC
	return 0;
#else
	InsertaSenyal(SIGN_QUIT, IPLocSignQuit);
	InsertaSenyal(SIGN_POST_NICK, IPLocSignPostNick);
	return 0;
#endif
}
int DetieneIPLoc()
{
	BorraSenyal(SIGN_QUIT, IPLocSignQuit);
	BorraSenyal(SIGN_POST_NICK, IPLocSignPostNick);
	return 0;
}
int IPLocSignQuit(Cliente *cl, char *quit)
{
	IPLocRecl *irec;
	for (irec = irecls; irec; irec = irec->sig)
	{
		if (irec->cl == cl)
		{
			LiberaItem(irec, irecls);
			return 1;
		}
	}
	return 0;
}
int IPLocParseaRespuesta(char *data, IPLocRec *irec)
{
	XML_Parser xml = XML_ParserCreate(NULL);
	//XML_SetStartElementHandler(xml, IPLocXMLInicio);
	XML_SetEndElementHandler(xml, IPLocXMLFin);
	XML_SetCharacterDataHandler(xml, IPLocXMLData);
	XML_SetUserData(xml, irec);
	if (!XML_Parse(xml, data, strlen(data), 1))
	{
		//if (irec->func)
		//	irec->func(XML_GetErrorCode(xml), irec->ip, NULL);
		//	Debug("error %s (%s) en %i %i", XML_ErrorString(XML_GetErrorCode(rs->pxml)), tokbuf, XML_GetCurrentLineNumber(rs->pxml), XML_GetCurrentColumnNumber(rs->pxml));
		//	Debug("%s", buf);
	}
	XML_ParserFree(xml);
	return 0;
}
void IPLocFree(IPLoc *loc)
{
	if (!loc)
		return;
	ircfree(loc->country);
	ircfree(loc->state);
	ircfree(loc->city);
	ircfree(loc->isp);
	ircfree(loc->countrycode);
	ircfree(loc->areacode);
	ircfree(loc->dmacode);
	ircfree(loc->postalcode);
	ircfree(loc->latitude);
	ircfree(loc->longitude);
	ircfree(loc);
}
int IPLocClCallBack(int code, char *ip, IPLoc *loc)
{
	if (code == 0)
	{
		IPLocRecl *irec;
		for (irec = irecls; irec; irec = irec->sig)
		{
			if (irec->ip == ip)
			{
				irec->cl->loc = loc;
				LiberaItem(irec, irecls);
				return 1;
			}
		}
		IPLocFree(loc);
	}
	return 0;
}
int IPLocSignPostNick(Cliente *cl, int modo)
{
	if (!modo) // nueva conexión
	{
		IPLocRecl *irec = BMalloc(IPLocRecl);
		irec->ip = cl->ip;
		irec->cl = cl;
		AddItem(irec, irecls);
		IPLocResolv(cl->ip, IPLocClCallBack);
	}
	return 0;
}
int IPLocResolv(char *ip, int (*func)(int, char *, IPLoc *))
{
	char *data;
	IPLocRec *irec = BMalloc(IPLocRec);
	irec->ip = ip;
	if ((data = CogeCache(CACHE_IPLOC, ip, 0)))
	{
		IPLocParseaRespuesta(data, irec);
		if (func)
			func(0, ip, irec->iloc);
		Free(irec);
	}
	else
	{
		irec->func = func;
		if ((irec->sck = SockOpen("www.redyc.com", 80, IPLocSockOpen, IPLocSockRead, NULL, IPLocSockClose)))
		{
			AddItem(irec, irecs);
			return 0;
		}
	}
	return 1;
}
void XMLCALL IPLocXMLFin(IPLocRec *irec, const XML_Char *nombre)
{
	if (!irec->iloc)
		irec->iloc = BMalloc(IPLoc);
	if (!strcmp(nombre, "city"))
		irec->iloc->city = strdup(irec->tmp);
	else if (!strcmp(nombre, "state"))
		irec->iloc->state = strdup(irec->tmp);
	else if (!strcmp(nombre, "country"))
		irec->iloc->country = strdup(irec->tmp);
	else if (!strcmp(nombre, "isp"))
		irec->iloc->isp = strdup(irec->tmp);
	else if (!strcmp(nombre, "countryCode"))
		irec->iloc->countrycode = strdup(irec->tmp);
	else if (!strcmp(nombre, "areaCode"))
		irec->iloc->areacode = strdup(irec->tmp);
	else if (!strcmp(nombre, "postalCode"))
		irec->iloc->postalcode = strdup(irec->tmp);
	else if (!strcmp(nombre, "dmaCode"))
		irec->iloc->dmacode = strdup(irec->tmp);
	else if (!strcmp(nombre, "latitude"))
		irec->iloc->latitude = strdup(irec->tmp);
	else if (!strcmp(nombre, "longitude"))
		irec->iloc->longitude = strdup(irec->tmp);
}
void XMLCALL IPLocXMLData(IPLocRec *irec, const XML_Char *s, int len)
{
	ircfree(irec->tmp);
	irec->tmp = (char *)Malloc(sizeof(char *) * (len+1));
	strncpy(irec->tmp, s, len);
	irec->tmp[len] = 0;
}
SOCKFUNC(IPLocSockOpen)
{
	IPLocRec *irec;
	if ((irec = BuscaIPLocRec(sck)))
	{
		SockWrite(sck, "GET /?_iploc=%s HTTP/1.1", irec->ip);
		SockWrite(sck, "Accept: */*");
		SockWrite(sck, "Host: www.redyc.com");
		SockWrite(sck, "Connection: close");
		SockWrite(sck, "");
	}
	return 0;
}
SOCKFUNC(IPLocSockRead)
{
	if (!strncmp(data, "<?xml", 5))
	{
		IPLocRec *irec;
		if ((irec = BuscaIPLocRec(sck)))
		{
			InsertaCache(CACHE_IPLOC, irec->ip, 86400, 0, data);
			IPLocParseaRespuesta(data, irec);
		}
	}
	return 0;
}
SOCKFUNC(IPLocSockClose)
{
	IPLocRec *irec;
	if ((irec = BuscaIPLocRec(sck)))
	{
		if (irec->func)
			irec->func(0, irec->ip, irec->iloc);
		ircfree(irec->tmp);
		LiberaItem(irec, irecs);
	}
	return 0;
}
