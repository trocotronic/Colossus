/*
 * $Id: soporte.c,v 1.1 2005-02-20 16:26:45 Trocotronic Exp $ 
 */

#include "struct.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <utime.h>
#include <sys/resource.h>
#endif

const char NTL_tolower_tab[] = {
       /* x00-x07 */ '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
       /* x08-x0f */ '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
       /* x10-x17 */ '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
       /* x18-x1f */ '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
       /* ' '-x27 */    ' ',    '!',    '"',    '#',    '$',    '%',    '&', '\x27',
       /* '('-'/' */    '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
       /* '0'-'7' */    '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
       /* '8'-'?' */    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
       /* '@'-'G' */    '@',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
       /* 'H'-'O' */    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
       /* 'P'-'W' */    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
       /* 'X'-'_' */    'x',    'y',    'z',    '{',    '|',    '}',    '~',    '_',
       /* '`'-'g' */    '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
       /* 'h'-'o' */    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
       /* 'p'-'w' */    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
       /* 'x'-x7f */    'x',    'y',    'z',    '{',    '|',    '}',    '~', '\x7f'
};
const char NTL_toupper_tab[] = {
       /* x00-x07 */ '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
       /* x08-x0f */ '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
       /* x10-x17 */ '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
       /* x18-x1f */ '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
       /* ' '-x27 */    ' ',    '!',    '"',    '#',    '$',    '%',    '&', '\x27',
       /* '('-'/' */    '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
       /* '0'-'7' */    '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
       /* '8'-'?' */    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
       /* '@'-'G' */    '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
       /* 'H'-'O' */    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
       /* 'P'-'W' */    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
       /* 'X'-'_' */    'X',    'Y',    'Z',    '[', '\x5c',    ']',    '^',    '_',
       /* '`'-'g' */    '`',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
       /* 'h'-'o' */    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
       /* 'p'-'w' */    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
       /* 'x'-x7f */    'X',    'Y',    'Z',    '[', '\x5c',    ']',    '^', '\x7f'
};
#ifdef NEED_STRCASECMP
int strcasecmp(const char *a, const char *b)
{
	const char *ra = a;
	const char *rb = b;
	while (ToLower(*ra) == ToLower(*rb)) 
	{
		if (!*ra++)
			return 0;
		else
			++rb;
	}
	return (*ra - *rb);
}
#endif
#ifdef NEED_STRNCASECMP
int strncasecmp(const char *a, const char *b, int len)
{
	const char *ra = a;
	const char *rb = b;
	int l = 0;
	while (ToLower(*ra) == ToLower(*rb) && l++ < len) 
	{
		if (!*ra++)
			return 0;
		else
			++rb;
	}
	return (*ra - *rb);
}
#endif
void ircstrdup(char **dest, const char *orig)
{
	ircfree(*dest);
	if (!orig)
		*dest = NULL;
	else
		*dest = strdup(orig);
}
char *my_itoa(int i)
{
	static char buf[128];
#ifndef _WIN32	
	sprintf_irc(buf, "%d", i);
#else
	_itoa(i, buf, 10);
#endif
	return (buf);
}
#ifdef NEED_INET_NTOA
char *inet_ntoa(struct sockaddr_in *in)
{
	static char buf[16];
	u_char *s = (u_char *)in;
	int  a, b, c, d;
	a = (int)*s++;
	b = (int)*s++;
	c = (int)*s++;
	d = (int)*s++;
	sprintf_irc(buf, "%d.%d.%d.%d", a, b, c, d);
	return buf;
}
#endif
#ifdef NEED_STRERROR
char *strerror(int err_no)
{
	extern char *sys_errlist[];
	extern int sys_nerr;
	static char buff[40];
	char *errp;
	errp = (err_no > sys_nerr ? (char *)NULL : sys_errlist[err_no]);
	if (errp == (char *)NULL)
	{
		errp = buff;
		sprintf_irc(errp, "Error desconocido %d", err_no);

	}
	return errp;
}

#endif
/* funciones extraidas del unreal */
time_t getfilemodtime(char *filename)
{
#ifdef _WIN32
	FILETIME cTime;
	SYSTEMTIME sTime, lTime;
	ULARGE_INTEGER fullTime;
	HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	if (!GetFileTime(hFile, NULL, NULL, &cTime))
		return 0;
	CloseHandle(hFile);
	FileTimeToSystemTime(&cTime, &sTime);
	SystemTimeToTzSpecificLocalTime(NULL, &sTime, &lTime);
	SystemTimeToFileTime(&sTime, &cTime);
	fullTime.LowPart = cTime.dwLowDateTime;
	fullTime.HighPart = cTime.dwHighDateTime;
	fullTime.QuadPart -= 116444736000000000;
	fullTime.QuadPart /= 10000000;
	return fullTime.LowPart;
#else
	struct stat sb;
	if (stat(filename, &sb))
		return 0;
	return sb.st_mtime;
#endif
}
void setfilemodtime(char *filename, time_t mtime)
{
#ifdef _WIN32
	FILETIME mTime;
	LONGLONG llValue;
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	llValue = Int32x32To64(mtime, 10000000) + 116444736000000000;
	mTime.dwLowDateTime = (long)llValue;
	mTime.dwHighDateTime = (DWORD)llValue >> 32;
	SetFileTime(hFile, &mTime, &mTime, &mTime);
	CloseHandle(hFile);
#else
	struct utimbuf utb;
	utb.actime = utb.modtime = mtime;
	utime(filename, &utb);
#endif
}
int copyfile(char *src, char *dest)
{
	char buf[2048];
	time_t mtime = getfilemodtime(src);
#ifdef _WIN32
	int srcfd = open(src, _O_RDONLY|_O_BINARY);
#else
	int srcfd = open(src, O_RDONLY);
#endif
	int destfd;
	int len;
	if (srcfd < 0)
		return 0;
#ifdef _WIN32
	destfd = open(dest, _O_BINARY|_O_WRONLY|_O_CREAT, _S_IWRITE);
#else
	destfd  = open(dest, O_WRONLY|O_CREAT, DEFAULT_PERMISSIONS);
#endif
	if (destfd < 0)
	{
		fecho(FADV, "No se puede crear el archivo '%s'", dest);
		return 0;
	}

	while ((len = read(srcfd, buf, 1023)) > 0)
	{
		if (write(destfd, buf, len) != len)
		{
			fecho(FADV, "No se puede escribir en el archivo '%s'. Quizás por falta de espacio en su HD.", dest);
			close(srcfd);
			close(destfd);
			unlink(dest);
			return 0;
		}
	}
	close(srcfd);
	close(destfd);
	setfilemodtime(dest, mtime);
	return 1;	
}
char *str_replace(char *str, char orig, char rep)
{
	static char rem[BUFSIZE];
	char *remp;
	remp = &rem[0];
	strcpy(remp, str);
	while (*remp)
	{
		if (*remp == orig)
			*remp = rep;
		remp++;
	}
	return rem;
}
char *strtolower(char *str)
{
	static char tol[BUFSIZE];
	char *tolo;
	tolo = &tol[0];
	strcpy(tolo, str);
	while (*tolo)
	{
		*tolo = ToLower(*tolo);
		tolo++;
	}
	return tol;
}
char *strtoupper(char *str)
{
	static char tou[BUFSIZE];
	char *toup;
	toup = &tou[0];
	strcpy(toup, str);
	while (*toup)
	{
		*toup = ToUpper(*toup);
		toup++;
	}
	return tou;
}
