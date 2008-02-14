/*
 * $Id: soporte.c,v 1.21 2008-02-14 16:19:28 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include <pthread.h>
#include <iconv.h>
#ifdef _WIN32
#include <sys/timeb.h>
#include <io.h>
#else
#include <sys/time.h>
#include <utime.h>
#include <dlfcn.h>
#endif
#include <sys/stat.h>
extern pthread_mutex_t mutex;

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
       /* 'X'-'_' */    'x',    'y',    'z',    '[', '\x5c',    ']',    '^',    '_',
       /* '`'-'g' */    '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
       /* 'h'-'o' */    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
       /* 'p'-'w' */    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
       /* 'x'-x7f */    'x',    'y',    'z',    '{',    '|',    '}',    '~', '\x7f',
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       		/*192*/	'à',	'á',	'â',	'ã',	'ä',	  0,	  0,	'ç',
       		/*200*/	'è',	'é',	'ê',	'ë',	'ì',	'í',	'î',	'ï',
       			  0,	'ñ',	'ò',	'ó',	'ô',	'õ',	'ö',	  0,
       		/*216*/	  0,	'ù',	'ú',	'û',	'ü',	'ý',	  0,	  0,
       		/*224*/	'à',	'á',	'â',	'ã',	'ä',	  0,	  0,	'ç',
       		/*232*/	'è',	'é',	'ê',	'ë',	'ì',	'í',	'î',	'ï',
       		/*240*/	  0,	'ñ',	'ò',	'ó',	'ô',	'õ',	'ö',	  0,
       		/*248*/	  0,	'ù',	'ú',	'ü',	'û',	'ý',	  0,	  0
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
       /* 'x'-x7f */    'X',    'Y',    'Z',    '{',    '|',    '}',    '~', '\x7f',
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       			  0,      0,	  0,	  0,	  0,	  0,	  0,	  0,
       		/*192*/	'À',	'Á',	'Â',	'Ã',	'Ä',	  0,	  0,	'Ç',
       		/*200*/	'È',	'É',	'Ê',	'Ë',	'Ì',	'Í',	'Î',	'Ï',
       			  0,	'Ñ',	'Ò',	'Ó',	'Ô',	'Õ',	'Ö',	  0,
       		/*216*/	  0,	'Ù',	'Ú',	'Û',	'Ü',	'Ý',	  0,	  0,
       		/*224*/	'À',	'Á',	'Â',	'Ã',	'Ä',	  0,	  0,	'Ç',
       		/*232*/	'È',	'É',	'Ê',	'Ë',	'Ì',	'Í',	'Î',	'Ï',
       		/*240*/	  0,	'Ñ',	'Ò',	'Ó',	'Ô',	'Õ',	'Ö',	  0,
       		/*248*/	  0,	'Ù',	'Ú',	'Û',	'Ü',	'Ý',	  0,	  0
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
#ifdef NEED_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size)
{
	size_t len = strlen(src);
	size_t ret = len;
	if (size <= 0)
		return 0;
	if (len >= size)
		len = size - 1;
	memcpy(dst, src, len);
	dst[len] = 0;
	return ret;
}
#endif
#ifdef NEED_STRLCAT
size_t strlcat(char *dst, const char *src, size_t size)
{
	size_t len1 = strlen(dst);
	size_t len2 = strlen(src);
	size_t ret = len1 + len2;

	if (size <= len1)
		return size;
	if (len1 + len2 >= size)
		len2 = size - (len1 + 1);

	if (len2 > 0) {
		memcpy(dst + len1, src, len2);
		dst[len1 + len2] = 0;
	}
	
	return ret;
}
#endif
#ifdef NEED_STRISTR
const char *stristr(const char *c, const char *a)
{
	const char *r, *s;
	if (BadPtr(a))
		return c;
	for (; !BadPtr(c); c++)
	{
		if (ToUpper(*c) == ToUpper(*a))
		{
			for (r = c, s = a; !BadPtr(r) && !BadPtr(s); r++, s++)
			{
				if (ToUpper(*r) != ToUpper(*s))
					break;
			}
			if (BadPtr(s))
				return c;
		}
	}
	return NULL;
}
#endif
void strcopia(char **dest, const char *orig)
{
	ircfree(*dest);
	if (!orig)
		*dest = NULL;
	else
		*dest = strdup(orig);
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
	ircsprintf(buf, "%d.%d.%d.%d", a, b, c, d);
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
		ircsprintf(errp, "Error desconocido %d", err_no);

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
	mTime.dwLowDateTime = (DWORD)llValue;
	mTime.dwHighDateTime = (DWORD)(llValue >> 32);
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
	destfd = open(dest, _O_BINARY|_O_TRUNC|_O_WRONLY|_O_CREAT, _S_IWRITE);
#else
	destfd  = open(dest, O_WRONLY|O_TRUNC|O_CREAT, DEFAULT_PERMISSIONS);
#endif
	if (destfd < 0)
	{
		Alerta(FADV, "No se puede crear el archivo '%s'", dest);
		return 0;
	}

	while ((len = read(srcfd, buf, 1023)) > 0)
	{
		if (write(destfd, buf, len) != len)
		{
			Alerta(FADV, "No se puede escribir en el archivo '%s'. Quizás por falta de espacio en su HD.", dest);
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

/*!
 * @desc: Reemplaza un caracter por otro.
 * @params: $str [in/out] Cadena a examinar
 	    $orig [in] Caracter original.
 	    $rep [in] Caracter final.
 * @ret: Devuelve la cadena reemplazada.
 * @cat: Programa
 !*/
 
char *str_replace(char *str, char orig, char rep)
{
	static char rem[BUFSIZE];
	char *remp;
	remp = &rem[0];
	strlcpy(remp, str, sizeof(rem));
	while (*remp)
	{
		if (*remp == orig)
			*remp = rep;
		remp++;
	}
	return rem;
}
/*!
 * @desc: Convierte una cadena a mayúsculas.
 * @params: $str [in] Cadena a convertir.
 * @ret: Devuelve la cadena convertida a mayúsculas.
 * @cat: Programa
 * @ver: strtoupper
 !*/
char *strtolower(char *str)
{
	static char tol[BUFSIZE];
#ifdef _WIN32
	strlcpy(tol, str, sizeof(tol));
	return _strlwr(tol);
#else
	char *tolo;
	tolo = &tol[0];
	strlcpy(tolo, str, sizeof(tol));
	while (*tolo)
	{
		*tolo = ToLower(*tolo);
		tolo++;
	}
	return tol;
#endif
}
/*!
 * @desc: Convierte una cadena a mayúsculas.
 * @params: $str [in] Cadena a convertir.
 * @ret: Devuelve la cadena convertida a mayúsculas.
 * @cat: Programa
 * @ver: strtolower
 !*/
char *strtoupper(char *str)
{
	static char tou[BUFSIZE];
#ifdef _WIN32
	strlcpy(tou, str, sizeof(tou));
	return _strupr(tou);
#else
	char *toup;
	toup = &tou[0];
	strlcpy(toup, str, sizeof(tou));
	while (*toup)
	{
		*toup = ToUpper(*toup);
		toup++;
	}
	return tou;
#endif
}

/*!
 * @desc: Cuenta el número de apariciones de una cadena dentro de otra.
 * @params: $pajar [in] Cadena donde buscar.
 	    $aguja [in] Cadena que debe buscarse.
 * @ret: Devuelve el número de veces que aparece <i>aguja</i> dentro del <i>pajar</i>
 * @cat: Programa
 !*/
 
int StrCount(char *pajar, char *aguja)
{
	char *c = pajar;
	int tot = 0;
	while ((c = strstr(c, aguja)))
	{
		c++;
		tot++;
	}
	return tot;
}

static u_int seed = 1;
void rstart(u_int _seed) 
{
    seed = _seed;
}

u_int rrand(void) {
    seed = seed * 214013 + 2531011;
    return (seed >> 16) & 0x7fff;
}
/*!
 * @desc: Genera un número aleatorio entre márgenes
 * @params: $min [in] Margen inferior
 	    $max [in] Margen superior
 * @ret: Devuelve el número generado
 * @cat: Programa
 !*/
u_int Aleatorio(u_int min, u_int max)
{
	static int i = 0;
	u_int r;
	if (!i)
	{
		rstart(time(0));
		i = 1;
	}
	r = (rrand() % (max-min+1))+min;
	return r;
}
/*!
 * @desc: Genera una cadena aleatoria siguiendo un patrón
 * @params: $patron [in] Patrón a seguir.
 * Existen algunos comodines para sustituir.
 * - ? : Cifra aleatoria
 * - º : Letra mayúscula aleatoria
 * - ! : Letra minúscula aleatoria
 * - # : Letra aleatoria (min/may)
 * - * : Cifra o letra aleatoria
 * El resto de símbolos se copiarán sin sustituir
 * @ret: Devuelve la cadena generada
 * @cat: Programa
 !*/

char *AleatorioEx(char *patron)
{
	char *ptr;
	static char buf[BUFSIZE];
	int len;
	const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ", *lower = "abcdefghijklmnopqrstuvwxyz";
	len = strlen(patron);
	buf[0] = '\0';
	ptr = &buf[0];
	while (*patron)
	{
		switch(*patron)
		{
			case '?':
				*ptr++ = Aleatorio(0, 9) + 48;
				break;
			case 'º':
				*ptr++ = upper[Aleatorio(0, 25)];
				break;
			case '!':
				*ptr++ = lower[Aleatorio(0, 25)];
				break;
			case '#':
				*ptr++ = (Aleatorio(0, 1) == 1 ? upper[Aleatorio(0, 25)] : lower[Aleatorio(0, 25)]);
				break;
			case '*':
				*ptr++ = (Aleatorio(0, 1) == 1 ? Aleatorio(0, 9) + 48 : (Aleatorio(0, 1) == 1 ? upper[Aleatorio(0, 25)] : lower[Aleatorio(0, 25)]));
				break;
			default:
				*ptr++ = *patron;
		}
		patron++;
	}
	*ptr = '\0';
	return buf;
}

/*!
 * @desc: Devuelve el valor TimeStamp actual con milisegundos
 * @ret: Devuelve el valor TimeStamp actual con milisegundos.
 * @cat: Programa
 !*/

double microtime()
{
	double segs, milisegs;
#ifdef _WIN32
	struct _timeb atime;
	_ftime(&atime);
	segs = atime.time;
	milisegs = atime.millitm;
#else
	struct timeval atime;
	gettimeofday(&atime, NULL);
	segs = atime.tv_sec;
	milisegs = atime.tv_usec;
#endif
	while (milisegs > 1) 
		milisegs /= 10;
	return (segs + milisegs);
}

/*!
 * @desc: Convierte un array a una cadena, cuyos elementos se separan con espacios.
 * @params: $array [in] Array a concatenar.
  	    $total [in] Número total de elementos.
 	    $parte [in] Inicio de la concatenación.
 	    $hasta [in] Final de la concatenación. Si vale -1, se une hasta el final del array.
 * @ret: Devuelve la cadena concatenada.
 * @ex: char *prueba[5] = {
 		"Esto" ,
 		"quedará",
 		"unido",
 		"por",
 		"espacios"
 	};
 	printf("%s", Unifica(array, 5, 0, -1));
 	//Imprimirá "Esto quedará unido por espacios"
 * @cat: Programa
 !*/

char *Unifica(char *array[], int total, int parte, int hasta)
{
	static char imp[BUFSIZE];
	int i, len = sizeof(imp), j;
	imp[0] = '\0';
	for (i = parte; i < total; i++)
	{
		if (len > 0)
		{
			j = strlen(array[i]);
			strncat(imp, array[i], MIN(j, len));
			len -= MIN(j, len);
			if (i != total - 1)
				strlcat(imp, " ", sizeof(imp));
			if (i == hasta)
				break;
		}
		else
			break;
	}
	return imp;
}
char *gettok(char *str, int pos, char sep)
{
	int i;
	static char buftok[BUFSIZE];
	while (!BadPtr(str))
	{
		if (*str == sep || pos == 1)
		{
			if (pos > 1)
			{
				str++;
				pos--;
			}
			if (pos == 1)
			{
				while (*str == sep)
					str++;
				for (i = 0; !BadPtr(str); i++)
				{
					buftok[i] = *str++;
					if (*str == sep)
					{
						buftok[i+1] = '\0';
						return buftok;
					}
				}
				buftok[i] = '\0';
				return buftok;
			}
		}
		str++;
	}
	return NULL;
}
/*!
 * @desc: Convierte una fecha en TS a formato legible.
 * @params: $tim [in] Puntero a fecha en TS.
 * @ret: Devuelve esa fecha de forma legible.
 * @ex: time_t tm;
 tm = time(NULL);
 printf("%s", Fecha(&tm));
 * @cat: Programa
 !*/
 
char *Fecha(time_t *tim)
{
    static char *wday_name[] = {
        "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"
    };
    static char *mon_name[] = {
        "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
        "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
    };
    static char result[26];
    struct tm *timeptr;
    timeptr = localtime(tim);
    sprintf(result, "%.3s %.2d %.3s %d %.2d:%.2d:%.2d",
        wday_name[timeptr->tm_wday],
        timeptr->tm_mday,
        mon_name[timeptr->tm_mon],
        1900 + timeptr->tm_year,
        timeptr->tm_hour,
        timeptr->tm_min, timeptr->tm_sec);
    return result;
}

/*!
 * @desc: Añade un caracter a una cadena. Análogo a strcat.
 * @params: $dest [in/out] Cadena a concatenar. El usuario es responsable de garantizar que haya espacio suficiente.
 	    $car [in] Caracter a añadir.
 * @ret: Devuelve la cadena <i>des</i> con el nuevo caracter añadido. NULL, en caso de error.
 * @cat: Programa
 !*/
 
char *chrcat(char *dest, char car)
{
	char *c;
	if ((c = strchr(dest, '\0')))
	{
		*c = car;
		*(c + 1) = '\0';
		return dest;
	}
	return NULL;
}

/*!
 * @desc: Elimina todas las apariciones de un caracter en una cadena.
 * @params: $dest [in/out] Cadena a procesar.
 	    $rem [in] Caracter a eliminar.
 * @ret: Devuelve la cadena <i>dest</i> con el caracter eliminado.
 * @cat: Programa
 !*/
 
char *chrremove(char *dest, char rem)
{
	char *c, *r = dest;
	for (c = dest; !BadPtr(c); c++)
	{
		if (*c != rem)
			*r++ = *c;
	}
	return dest;
}


/*!
 * @desc: Repite un caracter varias veces.
 * @params: $car [in] Caracter a repetir.
 	    $veces [in] Número de veces a repetir.
 * @ret: Devuelve una cadena con el caracter repetido varias veces.
 * @cat: Programa
 !*/
 
char *Repite(char car, int veces)
{
	static char ret[128];
	int i = 0;
	if (veces < sizeof(ret)-1)
	{
		while (i < veces)
			ret[i++] = car;
	}
	ret[i] = 0;
	return ret;
}

/*!
 * @desc: Busca una opción en una lista de opciones a partir de su ítem.
 * @params: $item [in] Ítem a buscar.
 	    $lista [in] Lista donde buscar.
 * @ret: Devuelve el valor de esa opción.
 * @cat: Programa
 !*/
 
int BuscaOpt(char *item, Opts *lista)
{
	Opts *ofl;
	for (ofl = lista; ofl->item; ofl++)
	{
		if (!strcasecmp(item, ofl->item))
			return ofl->opt;
	}
	return 0;
}

/*!
 * @desc: Busca un ítem en una lista de opciones a partir de su opción.
 * @params: $opt [in] Opción a buscar.
 		$lista [in] Lista donde buscar.
 * @ret: Devuelve el ítem de esa opción. NULL si no lo encuentra.
 * @cat: Programa
 !*/
 
char *BuscaOptItem(int opt, Opts *lista)
{
	Opts *ofl;
	for (ofl = lista; ofl->item; ofl++)
	{
		if (ofl->opt == opt)
			return ofl->item;
	}
	return NULL;
}
Recurso CopiaDll(char *dll, char *archivo, char *tmppath)
{
	Recurso hmod;
	char *c;
#ifdef _WIN32
	char tmppdb[MAX_FNAME], pdb[MAX_FNAME];
#endif
	if (!(c = strrchr(dll, '/')))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta)", dll);
		return NULL;
	}
	strlcpy(archivo, ++c, MAX_FNAME);
	ircsprintf(tmppath, "./tmp/%s", archivo);
#ifdef _WIN32
	strlcpy(pdb, dll, sizeof(pdb));
	if (!(c = strrchr(pdb, '.')))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla pdb)", dll);
		return NULL;
	}
	strlcpy(c, ".pdb", sizeof(pdb) - (c-pdb));
	if (!(c = strrchr(pdb, '/')))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (falla ruta pdb)", pdb);
		return NULL;
	}
	c++;
	ircsprintf(tmppdb, "./tmp/%s", c);
#endif
	if (!copyfile(dll, tmppath))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar)", dll);
		return NULL;
	}
#ifdef _WIN32
	if (!copyfile(pdb, tmppdb))
	{
		Alerta(FADV, "Ha sido imposible cargar %s (no puede copiar pdb)", pdb);
		return NULL;
	}
#endif
	if ((hmod = irc_dlopen(tmppath, RTLD_LAZY)))
		return hmod;
	Alerta(FADV, "Ha sido imposible cargar %s (dlopen): %s", dll, irc_dlerror());
	return NULL;
}
/*!
 * @desc: Calcula la duración a partir de los segundos dados en un formato legible. Rellena una estructura de tipo <i>Duracion</i> que contiene de forma legible la duración indicada.
 * @params: $segs [in] Segundos totales de la duración.
 		$d [out] Estructura rellenada que contiene los segundos desglosados en semanas, días, horas, minutos y segundos.
 * @cat: Programa
 !*/
int MideDuracionEx(u_int segs, Duracion *d)
{
	d->sems = segs/604800;
	segs %= 604800;
	d->dias = segs/86400;
	segs %= 86400;
	d->horas = segs/3600;
	segs %= 3600;
	d->mins = segs/60;
	segs %= 60;
	d->segs = segs;
	return 0;
}
/*!
 * @desc: Calcula la duración a partir de los segundos dados en un formato legible.
 * @params: $segs [in] Segundos totales de la duración.
 * @cat: Programa
 * @ret: Devuelve un puntero a char que contiene la duración de forma "%u días, %02u:%02u:%02u"
 !*/
char *MideDuracion(u_int segs)
{
	Duracion d;
	static char tmp[64];
	MideDuracionEx(segs, &d);
	ircsprintf(tmp, "%u días, %02u:%02u:%02u", d.sems*7+d.dias, d.horas, d.mins, d.segs);
	return tmp;
}
/*
 * @desc: Convierte una cadena con un conjunto de caracteres a otro conjunto de caracteres. Por ejemplo, permite pasar de UTF-8 a ISO-8859-1
 * @params: $from [in] Charset de la cadena de origen. Más información en http://www.gnu.org/software/libiconv/documentation/libiconv/iconv_open.3.html
 		$to [in] Charset al que se quiere convertir.
 		$src [in] Cadena que se desea transformar.
 		$dst [out] Buffer de destino al que se copiará la nueva cadena. Debe ser lo suficientemente grande para albergar la nueva cadena.
 		$siz [in] Tamaño máximo del buffer de destino.
 * @cat: Programa
 * @ret: Devuelve el número de bytes transformados o -1 si ha ocurrido un error.
!*/
int CambiarCharset(char *from, char *to, char *src, char *dst, size_t siz)
{
	iconv_t icv;
	size_t t1, t2;
	char *d = dst;
	if ((icv = iconv_open(to, from)) == (iconv_t)-1)
		return -1;
	t1 = strlen(src);
	t2 = siz;
	if (iconv(icv, &src, &t1, &dst, &t2) == -1)
		siz = -1;
	else
		d[(siz = (siz-t2))] = '\0';
	iconv_close(icv);
	return siz;
}
