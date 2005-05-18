/*
 * $Id: bdd.c,v 1.25 2005-05-18 18:51:03 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include "struct.h"
#include "ircd.h"
#include <sys/stat.h>
#include "md5.h"

#ifdef UDB
#include "bdd.h"
#ifndef _WIN32
#define O_BINARY 0x0
//#include <sys/mman.h>
#endif
Udb *nicks = NULL;
Udb *chans = NULL;
Udb *ips = NULL;
Udb *sets = NULL;
u_int BDD_NICKS;
u_int BDD_CHANS;
u_int BDD_IPS;
u_int BDD_SET;
Udb ***hash;
Udb *ultimo = NULL;
#define da_Udb(x) do{ x = (Udb *)Malloc(sizeof(Udb)); bzero(x, sizeof(Udb)); }while(0)
#define atoul(x) strtoul(x, NULL, 10)
char bloques[128];
time_t gmts[128];
u_int BDD_TOTAL = 0;
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
	int id;
	hash = (Udb ***)Malloc(sizeof(Udb **) * BDD_TOTAL);
	for (reg = ultimo; reg; reg = reg->mid)
	{
		id = ID(reg);
		hash[id] = (Udb **)Malloc(sizeof(Udb *) * 2048);
		bzero(hash[id], sizeof(Udb *) * 2048);
		gmts[id] = 0L;
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
		ircstrdup(bloq->data_char, MDString(par));
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
time_t lee_gmt(int id)
{
	FILE *fp;
	char lee[11];
	if (!(fp = fopen(DB_DIR "crcs", "r")))
		return 0L;
	fseek(fp, BDD_TOTAL * 8 + 10 * id, SEEK_SET);
	bzero(lee, 11);
	fread(lee, 1, 10, fp);
	fclose(fp);
	return atoul(lee);
}
DLLFUNC int actualiza_gmt(Udb *bloque, time_t gm)
{
	char lee[11];
	FILE *fh;
	time_t hora = gm ? gm : time(0);
	bzero(lee, 11);
	if (!(fh = fopen(DB_DIR "crcs", "r+")))
		return -1;
	fseek(fh, BDD_TOTAL * 8 + 10 * ID(bloque), SEEK_SET);
	sprintf_irc(lee, "%ul", hora);
	fwrite(lee, 1, 10, fh);
	fclose(fh);
	gmts[ID(bloque)] = hora;
	return 0;
}
DLLFUNC int actualiza_hash(Udb *bloque)
{
	char lee[9];
	FILE *fh;
	u_long lo;
	bzero(lee, 9);
	if (!(fh = fopen(DB_DIR "crcs", "r+")))
		return -1;
	lo = obtiene_hash(bloque);
	fseek(fh, 8 * ID(bloque), SEEK_SET);
	sprintf_irc(lee, "%X", lo);
	fwrite(lee, 1, 8, fh);
	fclose(fh);
	return 0;
}
/* devuelve el puntero a todo el bloque a partir de su id o letra */
DLLFUNC Udb *coge_de_id(int id)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if ((LETRA(reg) == id) || (ID(reg) == id))
			return reg;
	}
	return NULL;
}
/* devuelve su id a partir de una letra */
DLLFUNC u_int coge_de_char(char tipo)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if (LETRA(reg) == tipo)
			return ID(reg);
	}
	return tipo;
}
u_char coge_de_tipo(int tipo)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if (ID(reg) == tipo)
			return LETRA(reg);
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
	if (!reg)
		return reg;
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
/* guarda en un archivo un registro pero hacia abajo */
int guarda_en_archivo_ex(Udb *reg, int tipo)
{
	if (!reg)
		return 0;
	if (reg->mid)
		guarda_en_archivo_ex(reg->mid, tipo);
	if (reg->down)
		guarda_en_archivo_ex(reg->down, tipo);
	else
		guarda_en_archivo(reg, tipo);
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
	Udb *reg, *root;
	if (!bloque || !item)
		return NULL;
	tipo = coge_de_char(tipo);
	root = coge_de_id(tipo);
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
		clavec = conf_set->clave_cifrado;
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
DLLFUNC int parsea_linea(int tipo, char *cur, int archivo)
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
DLLFUNC void carga_bloque(int tipo)
{
	Udb *root;
	u_long lee, obtiene, trunca = 0L, bytes = 0L;
	char bloque, linea[BUFSIZE], *c;
	FILE *fp;
	bloque = coge_de_tipo(tipo);
	root = coge_de_id(tipo);
	if ((lee = lee_hash(tipo)) != (obtiene = obtiene_hash(root)))
	{
		Info("El bloque %c está corrupto (%lu != %lu)", bloque, lee, obtiene);
		if ((fp = fopen(root->item, "wb")))
		{
			fclose(fp);
			actualiza_hash(root);
		}
		if (linkado)
			sendto_serv(":%s DB %s RES %c 0", me.nombre, linkado->nombre, bloque);
		return;
	}
	gmts[tipo] = lee_gmt(tipo);
		if ((fp = fopen(root->item, "rb")))
	{
		while (fgets(linea, sizeof(linea), fp))
		{
			if ((c = strchr(linea, '\r')))
				*c = '\0';
			if ((c = strchr(linea, '\n')))
				*c ='\0';
			else
			{
				trunca = bytes;
				break;
			}
			bytes += strlen(linea) + 1;
			parsea_linea(tipo, linea, 0);
			bzero(linea, sizeof(linea));
		}
		fclose(fp);
	}
/*
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
		else 
		{
			trunca = cur - p;
			break;
		}
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
*/
	if (trunca)
		trunca_bloque(&me, root, trunca);
}
DLLFUNC void descarga_bloque(int tipo)
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
	u_int i;
	for (i = 0; i < BDD_TOTAL; i++)
		descarga_bloque(i);
	for (i = 0; i < BDD_TOTAL; i++)
		carga_bloque(i);
}
int trunca_bloque(Cliente *cl, Udb *bloq, u_long bytes)
{
	FILE *fp;
	char *contenido = NULL, bdd;
	bdd = bloq->id & 0xFF;
	if ((fp = fopen(bloq->item, "rb")))
	{
		contenido = Malloc(bytes);
		if (fread(contenido, 1, bytes, fp) != bytes)
		{
			fclose(fp);
			sendto_serv(":%s DB %s ERR DRP %i %c fread", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
			return 1;
		}
		fclose(fp);
		if ((fp = fopen(bloq->item, "wb")))
		{
			int id;
			id = coge_de_char(bdd);
			if (fwrite(contenido, 1, bytes, fp) != bytes)
			{
				fclose(fp);
				sendto_serv(":%s DB %s ERR DRP %i %c fwrite", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
				return 1;
			}
			fclose(fp);
			actualiza_hash(bloq);
			descarga_bloque(id);
			carga_bloque(id);
		}
		else
		{
			sendto_serv(":%s DB %s ERR DRP %i %c fopen(wb)", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
			return 1;
		}
		if (contenido)
			Free(contenido);
	}
	else
	{
		sendto_serv(":%s DB %s ERR DRP %i %c fopen(rb)", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
		return 1;
	}
	return 0;
}
DLLFUNC int optimiza(Udb *bloq)
{
	FILE *fp;
	int id = ID(bloq);
	if ((fp = fopen(bloq->item, "wb")))
		fclose(fp);
	guarda_en_archivo_ex(bloq->down, id);
	descarga_bloque(id);
	carga_bloque(id);
	return 0;
}
#endif

