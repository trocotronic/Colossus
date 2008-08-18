/*
 * $Id: smtp.c,v 1.28 2008/04/05 20:39:22 Trocotronic Exp $
 */

#include "struct.h"
#include "httpd.h"
#ifdef _WIN32
#else
#include <arpa/nameser.h>
#include <resolv.h>
#include <time.h>
#endif
#include <sys/stat.h>
#define INTSMTP 5

SmtpData *smtps = NULL;

SOCKFUNC(ProcesaSmtp);
SOCKFUNC(CierraSmtp);
void EmailArchivosVL(char *, char *, Opts *, int (*)(SmtpData *), char *, va_list *);

static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

int b64_encode(char const *src, size_t srclength, char *target, size_t targsize)
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
int b64_decode(char const *src, char *target, size_t targsize)
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
	strlcpy(tokbuf, data, sizeof(tokbuf));
	if (atoi(strtok(tokbuf, " ")) != numeric)
		return 1;
	return 0;
}
#ifndef _WIN32
struct mx
{
    int pref;
    char host[1024];
};

#ifndef HFIXEDSZ
# define HFIXEDSZ 12
#endif
#ifndef INT16SZ
# define INT16SZ sizeof(cit_int16_t)
#endif
#ifndef INT32SZ
# define INT32SZ sizeof(cit_int32_t)
#endif
int mxcomp(int p1, int p2)
{
	if (p1 > p2)
    		return(1);
	else if (p1 < p2)
		return(0);
	else
		return(rand() % 2);
}
void sort_mxrecs (struct mx *mxrecs, int nmx)
{
	int a, b;
	struct mx t1, t2;
	if (nmx < 2)
		return;
	for (a = nmx - 2; a >= 0; --a)
	{
		for (b = 0; b <= a; ++b)
		{
			if (mxcomp(mxrecs[b].pref,mxrecs[b+1].pref))
			{
				memcpy(&t1, &mxrecs[b], sizeof(struct mx));
				memcpy(&t2, &mxrecs[b+1], sizeof(struct mx));
				memcpy(&mxrecs[b], &t2, sizeof(struct mx));
				memcpy(&mxrecs[b+1], &t1, sizeof(struct mx));
			}
		}
	}
}
char *getmx(char *dest)
{
	union
	{
		u_char bytes[1024];
		HEADER header;
    	}ans;
	int ret;
	static char retb[BUFSIZE];
	unsigned char *startptr, *endptr, *ptr;
	char expanded_buf[1024];
	unsigned short pref, type;
	int n = 0;
	int qdcount;
	struct mx *mxrecs = NULL;
	int nmx = 0;
	ret = res_query(dest, C_IN, T_MX, (unsigned char *)ans.bytes, sizeof(ans));
	if (ret < 0)
	{
		mxrecs = malloc(sizeof(struct mx));
		mxrecs[0].pref = 0;
		strlcpy(mxrecs[0].host, dest, sizeof(mxrecs[0].host));
		nmx = 0;
	}
	else
	{
		if (ret > sizeof(ans))
			ret = sizeof(ans);
		startptr = &ans.bytes[0];
		endptr = &ans.bytes[ret];
		ptr = startptr + HFIXEDSZ;	/* skip header */
		for (qdcount = ntohs(ans.header.qdcount); qdcount--; ptr += ret + QFIXEDSZ)
		{
			if ((ret = dn_skipname(ptr, endptr)) < 0)
				return NULL;
		}
		while(1)
		{
			bzero(expanded_buf, sizeof(expanded_buf));
			ret = dn_expand (startptr, endptr, ptr, expanded_buf, sizeof(expanded_buf));
			if (ret < 0)
				break;
			ptr += ret;
			GETSHORT (type, ptr);
			ptr += INT16SZ + INT32SZ;
			GETSHORT (n, ptr);
			if (type != T_MX)
				ptr += n;
			else
			{
				GETSHORT(pref, ptr);
				ret = dn_expand(startptr, endptr, ptr, expanded_buf, sizeof(expanded_buf));
				ptr += ret;
				++nmx;
				if (mxrecs == NULL)
					mxrecs = malloc(sizeof(struct mx));
				else
					mxrecs = realloc (mxrecs, (sizeof(struct mx) * nmx));
				mxrecs[nmx - 1].pref = pref;
				strlcpy(mxrecs[nmx - 1].host, expanded_buf, sizeof(mxrecs[nmx - 1].host));
			}
		}
	}
	sort_mxrecs(mxrecs, nmx);
	strlcpy(retb, mxrecs[0].host, sizeof(ret));
	return retb;
}
#endif
#define SDK
char *Mx(char *dominio)
{
#ifdef _WIN32
	int len;
	char *res;
	static char ret[128];
#endif
	char *host = NULL;
	if (!dominio)
		return NULL;
	if ((host = CogeCache(CACHE_MX, dominio, 0)))
		return host;
#ifdef _WIN32
 	ircsprintf(buf, "-type=mx %s", dominio);
 	if (!EjecutaComandoSinc("nslookup", buf, &len, &res))
 	{
 		char *t;
 		int prev = 9999, pref;
 		for (t = strtok(res, "\r\n"); t; t = strtok(NULL, "\r\n"))
 		{
 			if (sscanf(t, "%*s\tMX preference = %i, mail exchanger = %s", &pref, buf))
 			{
 				if (pref < prev)
 				{
 					strlcpy(ret, buf, sizeof(ret));
 					host = ret;
 					prev = pref;
 				}
 			}
 		}
 		Free(res);
 		if (host)
 			InsertaCache(CACHE_MX, dominio, 86400, 0, host);
 		return host;
 	}
#else
	if ((host = getmx(dominio)))
		InsertaCache(CACHE_MX, dominio, 86400, 0, host);
	return host;
#endif
	return (conf_smtp ? conf_smtp->host : NULL);
}
SmtpData *EncolaSmtp(char *para, char *de, char *tema, char *cuerpo, Opts *fps, int (*closefunc)(SmtpData *))
{
	SmtpData *smtp;
	char *hostmx;
	if (conf_smtp)
		hostmx = conf_smtp->host;
	else
	{
		char *dominio;
		strlcpy(tokbuf, para, sizeof(tokbuf));
		strtok(tokbuf, "@");
		dominio = strtok(NULL, "@");
		hostmx = Mx(dominio);
	}
	if (!hostmx)
		return NULL;
	smtp = BMalloc(SmtpData);
	smtp->para = strdup(para);
	smtp->de = strdup(de);
	smtp->tema = strdup(tema);
	smtp->cuerpo = strdup(cuerpo);
	smtp->fps = fps;
	smtp->closefunc = closefunc;
	smtp->estado = 0;
#ifdef USA_SSL
	if (conf_smtp->ssl)
		smtp->sck = SockOpenEx(hostmx, conf_smtp->puerto, NULL, ProcesaSmtp, NULL, CierraSmtp, 30, 0, OPT_SSL);
	else
#endif
		smtp->sck = SockOpen(hostmx, conf_smtp->puerto, NULL, ProcesaSmtp, NULL, CierraSmtp);
	AddItem(smtp, smtps);
	return smtp;
}
SmtpData *BuscaSmtp(Sock *sck)
{
	SmtpData *smtp;
	for (smtp = smtps; smtp; smtp = smtp->sig)
	{
		if (smtp->sck == sck)
			return smtp;
	}
	return NULL;
}
int LiberaSmtp(SmtpData *smtp)
{
	if (!smtp)
		return 0;
	if (smtp->closefunc)
		smtp->closefunc(smtp);
	BorraItem(smtp, smtps);
	Free(smtp->de);
	Free(smtp->para);
	Free(smtp->tema);
	Free(smtp->cuerpo);
	if (smtp->fps)
	{
		Opts *ofl;
		for (ofl = smtp->fps; ofl && ofl->item; ofl++)
			Free(ofl->item);
		Free(smtp->fps);
	}
	Free(smtp);
	return 1;
}

/*!
 * @desc: Envía un email.
 * @params: $para [in] Email de destino.
 	    $tema [in] Tema del mensaje.
 	    $cuerpo [in] Cuerpo del mensaje. Es una cadena con formato.
 	    $... [in] Argumentos variables según cadena con formato.
 * @cat: Programa
 !*/

void Email(char *para, char *tema, char *cuerpo, ...)
{
	va_list vl;
	va_start(vl, cuerpo);
	EmailArchivosVL(para, tema, NULL, NULL, cuerpo, &vl);
	va_end(vl);
}
/*!
 * @desc: Envía archivos vía email.
 * @params: $para [in] Email de destino.
 		$tema [in] Tema del mensaje.
 		$fps [in] Matriz de <i>Opts</i> correspondiente a los archivos a adjuntar. Debe crearse por <i>Malloc()</i>.
 		El campo <i>item</i> corresponde al nombre del archivo. Debe ser una copia por <i>strdup()</i>.
 		El campo <i>opt</i> corresponde a un descriptor devuelto por <i>open()</i>.
 		El último elemento debe ser NULL para indicar el fin de la matriz.
 		Al final de la función, se ejecutará <i>Free()</i> sobre el campo <i>item</i> y sobre el puntero <i>fps</i>.
 		$closefunc [in] Función que se llama una vez se ha enviado el correo correctamente. Se pasa como puntero <i>SmtpData</i> referente al correo.
 		Esta función deberá cerrar los descriptores usados por <i>open()</i> en los campos <i>opt</i>, liberar los campos <i>item</i> y liberar el puntero <i>fps</i>.
 		$cuerpo [in] Cuerpo del mensaje. Es una cadena con formato.
 		$... [in] Argumentos variables según cadena con formato.
 * @cat: Programa
 !*/
void EmailArchivos(char *para, char *tema, Opts *fps, int (*closefunc)(SmtpData *), char *cuerpo, ...)
{
	va_list vl;
	va_start(vl, cuerpo);
	EmailArchivosVL(para, tema, fps, closefunc, cuerpo, &vl);
	va_end(vl);
}
void EmailArchivosVL(char *para, char *tema, Opts *fps, int (*closefunc)(SmtpData *), char *cuerpo, va_list *vl)
{
	char buf[BUFSIZE];
	if (vl)
		ircvsprintf(buf, cuerpo, *vl);
	else
		strlcpy(buf, cuerpo, sizeof(buf));
	EncolaSmtp(para, conf_set->admin, tema, buf, fps, closefunc);
}
SOCKFUNC(ProcesaSmtp)
{
	SmtpData *smtp;
	if (!(smtp = BuscaSmtp(sck)))
		return 1;
	//printf("%% %s\n", data);
	if (!ParseSmtp(data, 220))
	{
		if (conf_smtp && conf_smtp->login)
			SockWrite(sck, "EHLO Colossus");
		else
		{
			SockWrite(sck, "HELO Colossus");
			smtp->estado = 3;
		}
	}
	if (!ParseSmtp(data, 250) && smtp->estado == 0)
	{
		SockWrite(sck, "AUTH LOGIN");
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 334) && smtp->estado == 1)
	{
		SockWrite(sck, base64_encode(conf_smtp->login, strlen(conf_smtp->login)));
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 334) && smtp->estado == 2)
	{
		SockWrite(sck, base64_encode(conf_smtp->pass, strlen(conf_smtp->pass)));
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 235))
	{
		SockWrite(sck, "MAIL FROM: <%s>", smtp->de);
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 250) && smtp->estado == 3)
	{
		SockWrite(sck, "MAIL FROM: <%s>", smtp->de);
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 250) && smtp->estado == 4)
	{
		SockWrite(sck, "RCPT TO: <%s>", smtp->para);
		smtp->estado++;
	}
	else if (!ParseSmtp(data, 250) && smtp->estado == 5)
	{
		SockWrite(sck, "DATA");
		smtp->estado++;
	}
	if (!ParseSmtp(data, 354))
	{
		char timebuf[128], *tipo;
		Opts *ofl;
		time_t ahora = time(0);
#ifdef _WIN32
		int hora;
#endif
		SockWrite(sck, "From: \"%s\" <%s>", conf_set->red, smtp->de);
		SockWrite(sck, "To: \"%s\" <%s>", smtp->para, smtp->para);
		SockWrite(sck, "Subject: %s", smtp->tema);
#ifdef _WIN32
		hora = abs(_timezone)+_daylight*3600*(_timezone < 0 ? 1 : -1);
		strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S", localtime(&ahora));
		ircsprintf(buf, " %s%02i%02i",_timezone < 0 ? "+" : "-",hora/3600, (hora%3600)/60);
		strlcat(timebuf, buf, sizeof(timebuf));
#else
		strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S %z", localtime(&ahora));
#endif
		SockWrite(sck, "Date: %s", timebuf);
		SockWrite(sck, "MIME-Version: 1.0");
		SockWrite(sck, "Content-Type: multipart/mixed;");
		SockWrite(sck, "\tboundary=\"-=_NextPart\"");
		SockWrite(sck, "X-Priority: 3");
		SockWrite(sck, "X-MSMail-Priority: Normal");
		SockWrite(sck, "X-Mailer: " COLOSSUS_VERSION);
		SockWrite(sck, "X-MimeOLE: Produced By " COLOSSUS_VERSION);
		SockWrite(sck, "");
		SockWrite(sck, "This is a multi-part message in MIME format.");
		SockWrite(sck, "");
		SockWrite(sck, "---=_NextPart");
		SockWrite(sck, "Content-Type: text/plain;");
		SockWrite(sck, "\tformat=flowed;");
		SockWrite(sck, "\tcharset=\"iso-8859-1\";");
		SockWrite(sck, "\treply-type=original");
		SockWrite(sck, "Content-Transfer-Encoding: 7bit");
		SockWrite(sck, "");
		SockWrite(sck, "%s", smtp->cuerpo);
		for (ofl = smtp->fps; ofl && ofl->item; ofl++)
		{
			struct stat inode;
			char *t;
			int b64 = 1;
			if (ofl->opt == -1)
				continue;
			SockWrite(sck, "");
			SockWrite(sck, "---=_NextPart");
			if (!(tipo = BuscaTipo(strrchr(ofl->item, '.'))))
				tipo = "text/plain";
			SockWrite(sck, "Content-Type: %s;", tipo);
			SockWrite(sck, "\tname=\"%s\";", ofl->item);
			if (!strncmp(tipo, "text", 4))
			{
				SockWrite(sck, "\tformat=flowed;");
				SockWrite(sck, "\treply-type=original");
				SockWrite(sck, "Content-Transfer-Encoding: quoted-printable");
				b64 = 0;
			}
			else
				SockWrite(sck, "Content-Transfer-Encoding: base64");
			SockWrite(sck, "Content-Disposition: attachment;");
			SockWrite(sck, "\tfilename=\"%s\"", ofl->item);
			SockWrite(sck, "");
			lseek(ofl->opt, 0, SEEK_SET);
			if (fstat(ofl->opt, &inode) != -1)
			{
				u_long r;
				t = (char *)Malloc(inode.st_size+1);
				t[inode.st_size] = '\0';
				if ((r = read(ofl->opt, t, inode.st_size)) == inode.st_size)
				{
					if (b64)
					{
						size_t len;
						char *dst;
						len = inode.st_size * 4 / 3 + 4;
						len += len / 72 + 1;
						dst = (char *)Malloc(sizeof(char) * len);
						if (b64_encode(t, inode.st_size, dst, sizeof(char) * len) > 0)
							SockWriteBin(sck, sizeof(char) * len, dst);
						Free(dst);
					}
					else
						SockWriteBin(sck, inode.st_size, t);
				}
				Free(t);
				//Debug("%u %u", r, inode.st_size);
			}
			//close(ofl->opt);
		}
		SockWrite(sck, "");
		SockWrite(sck, "---=_NextPart");
		SockWrite(sck, "");
		SockWrite(sck, ".");
		SockWrite(sck, "QUIT");
		smtp->enviado = 1;
	}
	if (!ParseSmtp(data, 221))
		SockClose(sck);
	return 0;
}
SOCKFUNC(CierraSmtp)
{
	SmtpData *smtp;
	if (!(smtp = BuscaSmtp(sck)))
		return 1;
	if (!smtp->enviado && ++(smtp->intentos) < INTSMTP)
	{
#ifdef USA_SSL
		if (conf_smtp->ssl)
			smtp->sck = SockOpenEx(sck->host, conf_smtp->puerto, NULL, ProcesaSmtp, NULL, CierraSmtp, 30, 0, OPT_SSL);
		else
#endif
			smtp->sck = SockOpen(sck->host, conf_smtp->puerto, NULL, ProcesaSmtp, NULL, CierraSmtp);
	}
	else
		LiberaSmtp(smtp);
	return 0;
}
