/*
 * $Id: bdd.c,v 1.13 2004-10-23 22:41:48 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include <fcntl.h>
#include "struct.h"
#include "ircd.h"
#include <sys/stat.h>
#include "md5.h"

/* The implementation here was originally done by Gary S. Brown.  I have
   borrowed the tables directly, and made some minor changes to the
   crc32-function (including changing the interface). //ylo */

  /* ============================================================= */
  /*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
  /*  code or tables extracted from it, as desired without restriction.     */
  /*                                                                        */
  /*  First, the polynomial itself and its table of feedback terms.  The    */
  /*  polynomial is                                                         */
  /*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
  /*                                                                        */
  /*  Note that we take it "backwards" and put the highest-order term in    */
  /*  the lowest-order bit.  The X^32 term is "implied"; the LSB is the     */
  /*  X^31 term, etc.  The X^0 term (usually shown as "+1") results in      */
  /*  the MSB being 1.                                                      */
  /*                                                                        */
  /*  Note that the usual hardware shift register implementation, which     */
  /*  is what we're using (we're merely optimizing it by doing eight-bit    */
  /*  chunks at a time) shifts bits into the lowest-order term.  In our     */
  /*  implementation, that means shifting towards the right.  Why do we     */
  /*  do it this way?  Because the calculated CRC must be transmitted in    */
  /*  order from highest-order term to lowest-order term.  UARTs transmit   */
  /*  characters in order from LSB to MSB.  By storing the CRC this way,    */
  /*  we hand it to the UART in the order low-byte to high-byte; the UART   */
  /*  sends each low-bit to hight-bit; and the result is transmission bit   */
  /*  by bit from highest- to lowest-order term without requiring any bit   */
  /*  shuffling on our part.  Reception works similarly.                    */
  /*                                                                        */
  /*  The feedback terms table consists of 256, 32-bit entries.  Notes:     */
  /*                                                                        */
  /*      The table can be generated at runtime if desired; code to do so   */
  /*      is shown later.  It might not be obvious, but the feedback        */
  /*      terms simply represent the results of eight shift/xor opera-      */
  /*      tions for all combinations of data and CRC register values.       */
  /*                                                                        */
  /*      The values must be right-shifted by eight bits by the "updcrc"    */
  /*      logic; the shift must be unsigned (bring in zeroes).  On some     */
  /*      hardware you could probably optimize the shift in assembler by    */
  /*      using byte-swap instructions.                                     */
  /*      polynomial $edb88320                                              */
  /*                                                                        */
  /*  --------------------------------------------------------------------  */

static u_long crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };

/* Return a 32-bit CRC of the contents of the buffer. */

u_long our_crc32(const u_char *s, u_int len)
{
  u_int i;
  u_long crc32val;
  crc32val = 0;
  for (i = 0;  i < len;  i ++)
    {
      crc32val =
        crc32_tab[(crc32val ^ s[i]) & 0xff] ^
          (crc32val >> 8);
    }
  return crc32val;
}
#define NUMNICKLOG 6
#define NUMNICKMAXCHAR 'z'	/* See convert2n[] */
#define NUMNICKBASE 64		/* (2 << NUMNICKLOG) */
#define NUMNICKMASK 63		/* (NUMNICKBASE-1) */

u_int base64toint(const char *s);
const char *inttobase64(char *buf, u_int v, u_int count);

/* El siguiente algoritmo de encriptación está cogido al pie de la letra del ircu del iRC-Hispano */
/*
 * TEA (cifrado)
 *
 * Cifra 64 bits de datos, usando clave de 64 bits (los 64 bits superiores son cero)
 * Se cifra v[0]^x[0], v[1]^x[1], para poder hacer CBC facilmente.
 *
 */
void tea(u_int v[], u_int k[], u_int x[])
{
  u_int y = v[0] ^ x[0], z = v[1] ^ x[1], sum = 0, delta = 0x9E3779B9;
  u_int a = k[0], b = k[1], n = 32;
  u_int c = 0, d = 0;

  while (n-- > 0)
  {
    sum += delta;
    y += ((z << 4) + a) ^ ((z + sum) ^ ((z >> 5) + b));
    z += ((y << 4) + c) ^ ((y + sum) ^ ((y >> 5) + d));
  }

  x[0] = y;
  x[1] = z;
}
void untea(u_int v[], u_int k[], u_int x[])
{
  u_int y = v[0] ^ x[0], z = v[1] ^ x[1], sum = 0xC6EF3720, delta = 0x9E3779B9;
  u_int a = k[0], b = k[1], n = 32;
  u_int c = 0, d = 0;

  while (n-- > 0)
  {
    z -= ((y << 4) + c) ^ ((y + sum) ^ ((y >> 5) + d));
    y -= ((z << 4) + a) ^ ((z + sum) ^ ((z >> 5) + b));
    sum -= delta;
  }

  x[0] = y;
  x[1] = z;
}
   
/*
 * convert2y[] converts a numeric to the corresponding character.
 * The following characters are currently known to be forbidden:
 *
 * '\0' : Because we use '\0' as end of line.
 *
 * ' '  : Because parse_*() uses this as parameter seperator.
 * ':'  : Because parse_server() uses this to detect if a prefix is a
 *        numeric or a name.
 * '+'  : Because m_nick() uses this to determine if parv[6] is a
 *        umode or not.
 * '&', '#', '+', '$', '@' and '%' :
 *        Because m_message() matches these characters to detect special cases.
 */
static const char convert2y[] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','[',']'
};

static const u_int convert2n[] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  52,53,54,55,56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 
   0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0,
   0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0,

   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

u_int base64toint(const char *s)
{
  u_int i = convert2n[(u_char)*s++];
  while (*s)
  {
    i <<= NUMNICKLOG;
    i += convert2n[(u_char)*s++];
  }
  return i;
}

const char *inttobase64(char *buf, u_int v, u_int count)
{
  buf[count] = '\0';
  while (count > 0)
  {
    buf[--count] = convert2y[(v & NUMNICKMASK)];
    v >>= NUMNICKLOG;
  }
  return buf;
}
char *cifranick(char *nickname, char *pass)
{
    unsigned int v[2], k[2], x[2];
    int cont = (conf_set->nicklen + 8) / 8;
    char *tmpnick;
    char tmppass[12 + 1];
    unsigned int *p; /* int == 32 bits */

    char *nick;    /* Nick normalizado */
    static char clave[12 + 1];                /* Clave encriptada */
    int i = 0;
    if (nickname == NULL || pass == NULL)
    	return "\0";
    tmpnick = (char *)Malloc(sizeof(char) * ((8 * (conf_set->nicklen + 8) / 8) + 1));
    nick = (char *)Malloc(sizeof(char) * (conf_set->nicklen + 1));
    strcpy(nick, nickname);

     /* Normalizar nick */
    while (nick[i] != 0)
    {
       nick[i] = ToLower(nick[i]);
       i++;
    }  

    memset(tmpnick, 0, (8 * (conf_set->nicklen + 8) / 8) + 1);
    strncpy(tmpnick, nick , (8 * (conf_set->nicklen + 8) / 8) + 1 - 1);

    memset(tmppass, 0, sizeof(tmppass));
    strncpy(tmppass, pass, sizeof(tmppass) - 1);

    /* relleno   ->   123456789012 */
    strncat(tmppass, "AAAAAAAAAAAA", sizeof(tmppass) - strlen(tmppass) -1);

    x[0] = x[1] = 0;
    
    k[1] = base64toint(tmppass + 6);
    tmppass[6] = '\0';
    k[0] = base64toint(tmppass);
    p = (unsigned int *)tmpnick;
    while(cont--)
    {
      v[0] = (u_int)ntohl(*p++);      /* 32 bits */
      v[1] = (u_int)ntohl(*p++);      /* 32 bits */
      tea(v, k, x);
    }

    inttobase64(clave, x[0], 6);
    inttobase64(clave + 6, x[1], 6);
    
    Free(nick);
    Free(tmpnick);

    return clave;
}

/* ***************************************************** */

#ifdef UDB
#include "bdd.h"
#ifndef _WIN32
#define O_BINARY 0x0
#endif
Udb *nicks = NULL;
Udb *chans = NULL;
Udb *ips = NULL;
Udb *sets = NULL;
DLLFUNC u_int BDD_NICKS, BDD_CHANS, BDD_IPS, BDD_SET;
Udb ***hash;
Udb *ultimo = NULL;
#define da_Udb(x) do{ x = (Udb *)Malloc(sizeof(Udb)); bzero(x, sizeof(Udb)); }while(0)
#define atoul(x) strtoul(x, NULL, 10)
char bloques[128];
int BDD_TOTAL = 0;
void alta_bloque(char letra, char *ruta, Udb **reg, u_int *id)
{
	static u_int ids = 0;
	da_Udb(*reg);
	(*reg)->id |= (u_int)letra;
	(*reg)->id |= (ids << 8);
	(*reg)->item = ruta;
	(*reg)->mid = ultimo;
	*id = ids;
	ultimo = *reg;
	bloques[BDD_TOTAL++] = letra;
	ids++;
}
void alta_hash()
{
	Udb *reg;
	hash = (Udb ***)Malloc(sizeof(Udb **) * BDD_TOTAL);
	for (reg = ultimo; reg; reg = reg->mid)
	{
		hash[(reg->id >> 8)] = (Udb **)Malloc(sizeof(Udb *) * 2048);
		bzero(hash[(reg->id >> 8)], sizeof(Udb *) * 2048);
	}
}
void bdd_init()
{
	FILE *fh;
#ifdef _WIN32
	mkdir(DB_DIR);
#else
	mkdir(DB_DIR, 0744);
#endif
	bzero(bloques, sizeof(bloques));
	if (!nicks)
		alta_bloque('N', DB_DIR "nicks.udb", &nicks, &BDD_NICKS);
	if (!chans)
		alta_bloque('C', DB_DIR "canales.udb", &chans, &BDD_CHANS);
	if (!ips)
		alta_bloque('I', DB_DIR "ips.udb", &ips, &BDD_IPS);
	if (!sets)
		alta_bloque('S', DB_DIR "set.udb", &sets, &BDD_SET);
	alta_hash();
	if ((fh = fopen(DB_DIR "crcs", "a")))
		fclose(fh);
	carga_bloques();
}
void cifrado_str(char *origen, char *destino, int len)
{
	int i;
	destino[0] = 0;
	for (i = 0; i < len; i++)
	{
		char tmp[2];
		sprintf_irc(tmp, "%02x", origen[i]);
		strcat(destino, tmp);
	}
}
u_long obtiene_hash(Udb *bloq)
{
	int fp;
	char *par;
	u_long hashl = 0L;
	struct stat inode;
	if ((fp = open(bloq->item, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE)) == -1)
		return 0;
	if (fstat(fp, &inode) == -1)
	{
		close(fp);
		return 0;
	}
	par = (char *)malloc(inode.st_size + 1);
	par[inode.st_size] = '\0';
	if (read(fp, par, inode.st_size) == inode.st_size)
	{
		ircstrdup(&bloq->data_char, MDString(par));
		bloq->data_long = inode.st_size;
		hashl = our_crc32(par, inode.st_size);
	}
	close(fp);
	Free(par);
	return hashl;
}
u_long lee_hash(int id)
{
	FILE *fp;
	u_int hash = 0;
	char lee[9];
	if (!(fp = fopen(DB_DIR "crcs", "r")))
		return 0L;
	fseek(fp, 8 * id, SEEK_SET);
	bzero(lee, 9);
	fread(lee, 1, 8, fp);
	fclose(fp);
	if (!sscanf(lee, "%X", &hash))
		return 0L;
	return (u_long)hash;
}
int actualiza_hash(Udb *bloque)
{
	char lee[9];
	FILE *fh;
	u_long lo;
	bzero(lee, 9);
	if (!(fh = fopen(DB_DIR "crcs", "r+")))
		return -1;
	lo = obtiene_hash(bloque);
	fseek(fh, 8 * (bloque->id >> 8), SEEK_SET);
	sprintf_irc(lee, "%X", lo);
	fwrite(lee, 1, 8, fh);
	fclose(fh);
	return 0;
}
/* devuelve el puntero a todo el bloque a partir de su id o letra */
Udb *coge_de_id(int id)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if (((reg->id & 0xFF) == id) || ((reg->id >> 8) == id))
			return reg;
	}
	return NULL;
}
/* devuelve su id a partir de una letra */
int coge_de_char(char tipo)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if ((reg->id & 0xFF) == tipo)
			return (reg->id >> 8);
	}
	return tipo;
}
char coge_de_tipo(int tipo)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if ((reg->id >> 8) == tipo)
			return (reg->id & 0xFF);
	}
	return tipo;
}
void inserta_registro_en_hash(Udb *registro, int donde, char *clave)
{
	u_int hashv;
	hashv = hash_cliente(clave) % 2048;
	registro->hsig = hash[donde][hashv];
	hash[donde][hashv] = registro;
}
int borra_registro_de_hash(Udb *registro, int donde, char *clave)
{
	Udb *aux, *prev = NULL;
	u_int hashv;
	hashv = hash_cliente(clave) % 2048;
	for (aux = hash[donde][hashv]; aux; aux = aux->hsig)
	{
		if (aux == registro)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				hash[donde][hashv] = aux->hsig;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
Udb *busca_udb_en_hash(char *clave, int donde, Udb *lugar)
{
	u_int hashv;
	Udb *aux;
	hashv = hash_cliente(clave) % 2048;
	for (aux = hash[donde][hashv]; aux; aux = aux->hsig)
	{
		if (!strcasecmp(clave, aux->item))
			return aux;
	}
	return lugar;
}
DLLFUNC Udb *busca_registro(int tipo, char *clave)
{
	if (!clave)
		return NULL;
	tipo = coge_de_char(tipo);
	return busca_udb_en_hash(clave, tipo, NULL);
}
DLLFUNC Udb *busca_bloque(char *clave, Udb *bloque)
{
	Udb *aux;
	if (!clave)
		return NULL;
	for (aux = bloque->down; aux; aux = aux->mid)
	{
		if (!strcasecmp(clave, aux->item))
			return aux;
	}
	return NULL;
}
Udb *crea_registro(Udb *bloque)
{
	Udb *reg;
	da_Udb(reg);
	reg->up = bloque;
	reg->mid = bloque->down;
	bloque->down = reg;
	return reg;
}
Udb *da_formato(char *form, Udb *reg)
{
	Udb *root = NULL;
	form[0] = '\0';
	if (reg->up)
		root = da_formato(form, reg->up);
	else
		return reg;
	strcat(form, reg->item);
	if (reg->down)
		strcat(form, "::");
	else
	{
		if (reg->data_char)
		{
			strcat(form, " ");
			strcat(form, reg->data_char);
		}
		else if (reg->data_long)
		{
			char tmp[32];
			sprintf(tmp, "%c%lu", CHAR_NUM, reg->data_long);
			strcat(form, " ");
			strcat(form, tmp);
		}
	}
	return root ? root : reg;
}
int guarda_en_archivo(Udb *reg, int tipo)
{
	char form[BUFSIZE];
	Udb *root;
	FILE *fp;
	form[0] = '\0';
	root = da_formato(form, reg);
	strcat(form, "\n");
	if (!(fp = fopen(root->item, "ab")))
		return -1;
	fputs(form, fp);
	fclose(fp);
	actualiza_hash(coge_de_id(tipo));
	return 0;
}
void libera_memoria_udb(int tipo, Udb *reg)
{
	if (reg->down)
	{
		borra_registro_de_hash(reg->down, tipo, reg->down->item);
		libera_memoria_udb(tipo, reg->down);
	}
	if (reg->mid)
	{
		borra_registro_de_hash(reg->mid, tipo, reg->mid->item);
		libera_memoria_udb(tipo, reg->mid);
	}
	if (reg->data_char)
		Free(reg->data_char);
	if (reg->item)
		Free(reg->item);
	Free(reg);
}
void borra_registro(int tipo, Udb *reg, int archivo)
{
	Udb *aux, *down, *prev = NULL, *up;
	if (!(up = reg->up)) /* estamos arriba de todo */
		return;
	tipo = coge_de_char(tipo);
	aux = coge_de_id(tipo);
	borra_registro_de_hash(reg, tipo, reg->item);
	if (reg->data_char)
		Free(reg->data_char);
	reg->data_char = NULL;
	reg->data_long = 0L;
	down = reg->down;
	
	reg->down = NULL;
	if (archivo)
		guarda_en_archivo(reg, tipo);
	if (!reg->sig)
		aux->sig = reg->prev;
	else
		reg->sig->prev = reg->prev;
	if (!reg->prev)
		aux->prev = reg->sig;
	else
		reg->prev->sig = reg->sig;
	for (aux = up->down; aux; aux = aux->mid)
	{
		if (aux == reg)
		{
			if (prev)
				prev->mid = aux->mid;
			else
				up->down = aux->mid;
			break;
		}
		prev = aux;
	}
	reg->mid = NULL;
	reg->down = down;
	libera_memoria_udb(tipo, reg);
	if (!up->down)
		borra_registro(tipo, up, archivo);
}
Udb *inserta_registro(int tipo, Udb *bloque, char *item, char *data_char, u_long data_long, int archivo)
{
	Udb *reg;
	if (!bloque || !item)
		return NULL;
	tipo = coge_de_char(tipo);
	if (!(reg = busca_bloque(item, bloque)))
	{
		reg = crea_registro(bloque);
		reg->item = strdup(item);
		inserta_registro_en_hash(reg, tipo, item);
	}
	else
	{
		if (data_char && reg->data_char && !strcmp(reg->data_char, data_char))
			return NULL;
	}
	if (reg->data_char)
		Free(reg->data_char);
	reg->data_char = NULL;
	if (data_char)
		reg->data_char = strdup(data_char);
	reg->data_long = data_long;
	if (archivo && (data_char || data_long))
		guarda_en_archivo(reg, tipo);
	return reg;
}
DLLFUNC int level_oper_bdd(char *oper)
{
	Udb *reg;
	if ((reg = busca_registro(BDD_NICKS, oper)))
	{
		Udb *aux;
		if ((aux = busca_bloque("oper", reg)))
			return aux->data_long;
	}
	return 0;
}
char *cifra_ip(char *ipreal)
{
	static char cifrada[512], clave[13];
	char *p, *clavec;
	int ts = 0;
	Udb *bloq;
	unsigned int ourcrc, v[2], k[2], x[2];
	ourcrc = our_crc32(ipreal, strlen(ipreal));
	if ((bloq = busca_registro(BDD_SET, "clave_cifrado")))
		clavec = bloq->data_char;
	else
		clavec = conf_set->cloak->crc;
	strncpy(clave, clavec, 12);
	clave[12] = '\0';
	p = cifrada;
	while (1)
  	{
		x[0] = x[1] = 0;
		k[0] = base64toint(clave);
		k[1] = base64toint(clave + 6);
    		v[0] = (k[0] & 0xffff0000) + ts;
    		v[1] = ourcrc;
    		tea(v, k, x);
    		inttobase64(p, x[0], 6);
    		p[6] = '.';
    		inttobase64(p + 7, x[1], 6);
		if (strchr(p, '[') == NULL && strchr(p, ']') == NULL)
			break;
    		else
		{
			if (++ts == 65536)
			{
				strcpy(p, ipreal);
				break;
			}
		}
	}
	return cifrada;
}	
int parsea_linea(int tipo, char *cur, int archivo)
{
	char *ds, *cop, *sp;
	Udb *bloq;
	int ins = 0;
	bloq = coge_de_id(tipo);
	cop = cur = strdup(cur);
	while ((ds = strchr(cur, ':')))
	{
		if (*(ds + 1) == ':')
		{
			*ds++ = '\0';
			if ((sp = strchr(cur, ' ')))
				*sp = '\0';
			bloq = inserta_registro(tipo, bloq, cur, sp, 0, archivo);
		}
		else /* ya no son :: */
			break;
		cur = ++ds;
	}
	if ((ds = strchr(cur, ' ')))
	{
		*ds++ = '\0';
		if (*ds == CHAR_NUM) /* son todo dígitos */
		{
			if (inserta_registro(tipo, bloq, cur, NULL, atoul(++ds), archivo))
				ins = 1;
		}
		else
		{
			if (inserta_registro(tipo, bloq, cur, ds, 0, archivo))
				ins = 1;
		}
	}
	else
	{
		if ((bloq = busca_bloque(cur, bloq)))
		{
			borra_registro(tipo, bloq, archivo);
			ins = -1;
		}
	}
	Free(cop);
	return ins;
}
void carga_bloque(int tipo)
{
	char *cur, *ds, *p;
	Udb *root;
	u_long lee, obtiene;
	char bloque = 0;
	int len, fd;
#ifdef _WIN32
	HANDLE mapa, archivo;
#else
	struct stat sb;
#endif
	bloque = coge_de_tipo(tipo);
	root = coge_de_id(tipo);
	if ((lee = lee_hash(root->id >> 8)) != (obtiene = obtiene_hash(root)))
	{
		fecho(FADV, "El bloque %c está corrupto (%lu != %lu)", bloque, lee, obtiene);
		if ((fd = open(root->item, O_WRONLY|O_TRUNC)))
		{
			close(fd);
			actualiza_hash(root);
		}
		if (linkado)
			sendto_serv(":%s DB %s RES %c 0", me.nombre, linkado->nombre, bloque);
		return;
	}
#ifdef _WIN32
	if ((archivo = CreateFile(root->item, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
#else
	if ((fd = open(root->item, O_RDONLY)) == -1)
#endif
		return;
#ifdef _WIN32
	len = GetFileSize(archivo, NULL);
	if (!(mapa = CreateFileMapping(archivo, NULL, PAGE_READWRITE, 0, len, NULL)))
		return;
	p = cur = MapViewOfFile(mapa, FILE_MAP_COPY, 0, 0, 0);
	if (!p)
		return;
#else
	if (fstat(fd, &sb) == -1)
		return;
	len = sb.st_size;
	p = cur = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
#endif
	while (cur < (p + len))
	{
		if ((ds = strchr(cur, '\r')))
			*ds = '\0';
		if ((ds = strchr(cur, '\n')))
			*ds = '\0';
		parsea_linea(tipo, cur, 0);
		cur = ++ds;
	}
#ifdef _WIN32
	UnmapViewOfFile(p);
	CloseHandle(mapa);
	CloseHandle(archivo);
#else
	munmap(p, len);
	close(fd);
#endif
}
void descarga_bloque(int tipo)
{
	Udb *aux, *sig, *bloq;
	bloq = coge_de_id(tipo);
	for (aux = bloq->down; aux; aux = sig)
	{
		sig = aux->mid;
		borra_registro(tipo, aux, 0);
	}
	bloq->data_long = 0L;
}
void carga_bloques()
{
	int i;
	for (i = 0; i < BDD_TOTAL; i++)
		descarga_bloque(i);
	for (i = 0; i < BDD_TOTAL; i++)
		carga_bloque(i);
}
#endif

