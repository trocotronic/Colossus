/*
 * $Id: smtp.c,v 1.10 2005-06-29 21:13:55 Trocotronic Exp $ 
 */

#include <time.h>
#include "struct.h"
#define MAXSMTP 128
#define INTSMTP 5

SmtpData *colasmtp[MAXSMTP];
int totalcola = 0;
int conta = 0;

SOCKFUNC(ProcesaSmtp);
SOCKFUNC(CierraSmtp);

static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

int b64_encode(unsigned char const *src, size_t srclength, char *target, size_t targsize)
{
	size_t datalength = 0;
	u_char input[3];
	u_char output[4];
	size_t i;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		target[datalength++] = Base64[output[2]];
		target[datalength++] = Base64[output[3]];
	}
    
	/* Now we worry about padding. */
	if (0 != srclength) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++)
			input[i] = *src++;
	
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		if (srclength == 1)
			target[datalength++] = Pad64;
		else
			target[datalength++] = Base64[output[2]];
		target[datalength++] = Pad64;
	}
	if (datalength >= targsize)
		return (-1);
	target[datalength] = '\0';	/* Returned value doesn't count \0. */
	return (datalength);
}
int b64_decode(char const *src, unsigned char *target, size_t targsize)
{
	int tarindex, state, ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (ch == 32)	/* Skip whitespace anywhere. */
			continue;

		if (ch == Pad64)
			break;

		pos = strchr(Base64, ch);
		if (pos == 0) 		/* A non-base64 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if ((size_t)tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - Base64) << 2;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 4;
				target[tarindex+1]  = ((pos - Base64) & 0x0f)
							<< 4 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 2;
				target[tarindex+1]  = ((pos - Base64) & 0x03)
							<< 6;
			}
			tarindex++;
			state = 3;
			break;
		case 3:
			if (target) {
				if ((size_t)tarindex >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base64);
			}
			tarindex++;
			state = 0;
			break;
		default:
			abort();
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (ch != 32)
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (ch != 32)
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target && target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}
	return (tarindex);
}
char *base64_encode(char *src, int len)
{
	static char buf2[512];
	b64_encode(src, len, buf2, sizeof(buf2));
	return buf2;
}
char *base64_decode(char *src)
{
	static char buf2[512];
	b64_decode(src, buf2, sizeof(buf2));
	return buf2;
}
int ParseSmtp(char *data, int numeric)
{
	strcpy(tokbuf, data);
	if (atoi(strtok(tokbuf, " ")) != numeric)
		return 1;
	return 0;
}
char *Mx(char *dominio)
{
#define SDK
#ifdef _WIN32
 #ifdef SDK
	HMODULE api;
	char *cache;
	if (!dominio)
		return NULL;
	if ((cache = CogeCache(CACHE_MX, dominio, 0)))
		return cache;
	if (VerInfo.dwMajorVersion == 5)
	{
		if ((api = LoadLibrary("mx.dll")))
		{
			char *(*mx)(char *);
			char *host;
			if ((mx = (char * (*)(char *))GetProcAddress(api, "MX")))
			{
				host = (mx)(dominio);
				if (host)
				{
					FreeLibrary(api);
					InsertaCache(CACHE_MX, dominio, 86400, 0, host);
					return host;
				}
			}
		}
	}
 #else
	HANDLE hStdin, hStdout;
	DWORD btr, btw;
	char orden[512];
	static char host[512];
	if (!dominio)
		return NULL;
	bzero(host, 512);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	ircsprintf(orden, "nslookup -type=mx %s", dominio);
	if (CreatePipe(&hStdout, &hStdin, NULL, 0))
	{
		int i;
		WriteFile(hStdin, orden, strlen(orden), &btw, NULL);
		for (;;)
		{
			if (!(i =ReadFile(hStdout, host, 512, &btr, NULL)) || !btr)
				break;
			Debug("%s %i %i", host, i, btr);
		}
		/*while ((fgets(buf, BUFSIZE, pp)))
		{
			char *mx;
			strtok(buf, "=");
			strtok(NULL, "=");
			if ((mx = strtok(NULL,"\n")))
			{
				_pclose(pp);
				strcpy(host, ++mx);
				MySQLInserta(MYSQL_CACHE, dominio, "valor", host);
				MySQLInserta(MYSQL_CACHE, dominio, "hora", "%lu", time(0));
				return host;
			}
		}
		_pclose(pp);*/
	}
 #endif
#endif
	return (conf_smtp ? conf_smtp->host : NULL);
}
void EncolaSmtp(char *para, char *de, char *tema, char *cuerpo)
{
	SmtpData *smtp;
	if (totalcola == MAXSMTP)
		return;
	smtp = (SmtpData *)Malloc(sizeof(SmtpData));
	smtp->para = strdup(para);
	smtp->de = strdup(de);
	smtp->tema = strdup(tema);
	smtp->cuerpo = strdup(cuerpo);
	smtp->enviado = 0;
	smtp->intentos = 0;
	colasmtp[totalcola++] = smtp;
}
int DesencolaSmtp()
{
	char *dominio, *hostmx;
	if (!totalcola)
		return 0;
	conta = 0;
	strcpy(tokbuf, colasmtp[0]->para);
	strtok(tokbuf, "@");
	dominio = strtok(NULL, "@");
	hostmx = conf_smtp ? conf_smtp->host : Mx(dominio);
	if (!hostmx)
		return 1;
	if (!SockOpen(hostmx, 25, NULL, ProcesaSmtp, NULL, CierraSmtp, ADD))
		return 1;
	return 0;
}
void LiberaMemoriaMensaje()
{
	int i;
	Free(colasmtp[0]->de);
	Free(colasmtp[0]->para);
	Free(colasmtp[0]->tema);
	Free(colasmtp[0]->cuerpo);
	Free(colasmtp[0]);
	for (i = 0; i < totalcola; i++)
		colasmtp[i] = colasmtp[i+1];
	totalcola--;
	DesencolaSmtp();
}
void EnviaEmail(char *para, char *de, char *tema, char *cuerpo)
{
	EncolaSmtp(para, de, tema, cuerpo);
	if (totalcola == 1)
		DesencolaSmtp();
}
void Email(char *para, char *tema, char *cuerpo, ...)
{
	char buf[BUFSIZE];
	va_list vl;
	va_start(vl, cuerpo);
	ircvsprintf(buf, cuerpo, vl);
	va_end(vl);
	EnviaEmail(para, conf_set->admin, tema, buf);
}
SOCKFUNC(ProcesaSmtp)
{
	SmtpData *smtp;
	smtp = colasmtp[0];
	//printf("%% %s\n", data);
	if (!ParseSmtp(data, 220))
	{
		if (conf_smtp && conf_smtp->login)
			SockWrite(sck, OPT_CRLF, "EHLO Colossus");
		else
		{
			SockWrite(sck, OPT_CRLF, "HELO Colossus");
			conta = 3;
		}
	}
	if (!ParseSmtp(data, 250) && conta == 0)
	{
		SockWrite(sck, OPT_CRLF, "AUTH LOGIN");
		conta++;
	}
	else if (!ParseSmtp(data, 334) && conta == 1)
	{
		SockWrite(sck, OPT_CRLF, base64_encode(conf_smtp->login, strlen(conf_smtp->login)));
		conta++;
	}
	else if (!ParseSmtp(data, 334) && conta == 2)
	{
		SockWrite(sck, OPT_CRLF, base64_encode(conf_smtp->pass, strlen(conf_smtp->pass)));
		conta++;
	}
	else if (!ParseSmtp(data, 235))
	{
		SockWrite(sck, OPT_CRLF, "MAIL FROM: %s", smtp->de);
		conta++;
	}
	else if (!ParseSmtp(data, 250) && conta == 3)
	{
		SockWrite(sck, OPT_CRLF, "MAIL FROM: %s", smtp->de);
		conta++;
	}
	else if (!ParseSmtp(data, 250) && conta == 4)
	{
		SockWrite(sck, OPT_CRLF, "RCPT TO: %s", smtp->para);
		conta++;
	}
	else if (!ParseSmtp(data, 250) && conta == 5)
	{
		SockWrite(sck, OPT_CRLF, "DATA");
		conta++;
	}
	if (!ParseSmtp(data, 354))
	{
		SockWrite(sck, OPT_CRLF, "From: \"%s\" <%s>", conf_set->red, smtp->de);
		SockWrite(sck, OPT_CRLF, "To: <%s>", smtp->para);
		SockWrite(sck, OPT_CRLF, "Subject: %s", smtp->tema);
		SockWrite(sck, OPT_CRLF, "Date: %lu", time(0));
		SockWrite(sck, OPT_CRLF, "%s", smtp->cuerpo);
		SockWrite(sck, OPT_CRLF, ".");
		SockWrite(sck, OPT_CRLF, "QUIT");
		smtp->enviado = 1;
	}
	if (!ParseSmtp(data, 221))
		SockClose(sck, LOCAL);
	return 0;
}
SOCKFUNC(CierraSmtp)
{
	if (!colasmtp[0]->enviado && colasmtp)
	{
		if (totalcola == MAXSMTP)
			Alerta(FADV, "Es imposible reenviar el email %s.");
		else
		{
			if (++(colasmtp[0]->intentos) == INTSMTP)
				Alerta(FADV, "Ha sido imposible enviar el email %s.");
			else
			{
				int i;
				colasmtp[totalcola++] = colasmtp[0];
				for (i = 0; i < totalcola; i++)
					colasmtp[i] = colasmtp[i+1];
				totalcola--;
				Alerta(FADV, "No se puede enviar el email %s. Poniendolo al final de la cola", colasmtp[0]->para);
			}
		}
	}
	LiberaMemoriaMensaje();
	return 1;
}
