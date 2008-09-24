/*
 * $Id: bdd.c,v 1.21 2008/02/13 18:45:24 Trocotronic Exp $
 */

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define fsync _commit
#define ftruncate _chsize
#else
#include <sys/io.h>
#include <time.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "bdd.h"
#include "md5.h"

Udb *UDB_NICKS = NULL, *UDB_CANALES = NULL, *UDB_IPS = NULL, *UDB_SET = NULL, *UDB_LINKS = NULL, *UDB_LINES = NULL;
Udb ***hash;
UDBloq *ultimo = NULL;
UDBloq *N = NULL, *C= NULL, *S = NULL, *I = NULL, *L = NULL, *K = NULL;
#define DaUdb(x) do{ x = (Udb *)Malloc(sizeof(Udb)); bzero(x, sizeof(Udb)); }while(0)
u_int BDD_TOTAL = 0;
#define MAX_HASH 2048

int SetDataVer(u_int);
int CogeDataVer();
Udb *BorraRegistro(u_int, Udb *, int);
int ActualizaDataVer2(), ActualizaDataVer3(), ActualizaDataVer4();

Opts NivelesBDD[] = {
	{ 0x1 , "OPER" } ,
	{ 0x2 , "ADMIN" } ,
	{ 0x4 , "ROOT" } ,
	{ 0x0 , 0x0 }
};
UDBloq *AltaBloque(char letra, char *ruta, Udb **dest)
{
	u_int id = 0;
	UDBloq *reg;
	if (ultimo)
		id = ultimo->id + 1;
	reg = (UDBloq *)Malloc(sizeof(UDBloq));
	reg->arbol = (Udb *)Malloc(sizeof(Udb));
	reg->arbol->id = id;
	reg->arbol->up = NULL;
	reg->arbol->down = NULL;
	reg->arbol->mid = NULL;
	reg->arbol->hsig = NULL;
	reg->arbol->item = NULL;
	reg->arbol->data_char = NULL;
	reg->arbol->data_long = 0L;
	reg->crc32 = 0L;
	reg->id = id;
	reg->lof = 0L;
	reg->letra = letra;
	reg->path = ruta;
	reg->regs = 0;
	reg->gmt = 0L;
	reg->res = NULL;
	reg->ver = 0;
	*dest = reg->arbol;
	reg->sig = ultimo;
	ultimo = reg;
	BDD_TOTAL++;
	return reg;
}
void VaciaHash(int id)
{
	int i;
	for (i = 0; i < MAX_HASH; i++)
		hash[id][i] = NULL;
}
void AltaHash()
{
	UDBloq *reg;
	u_int id;
	hash = (Udb ***)Malloc(sizeof(Udb **) * BDD_TOTAL);
	for (reg = ultimo; reg; reg = reg->sig)
	{
		id = reg->id;
		hash[id] = (Udb **)Malloc(sizeof(Udb *) * MAX_HASH);
		VaciaHash(id);
	}
}
void printea(Udb *bloq, int escapes)
{
	int i;
	char tabs[32];
	tabs[0] = '\0';
	for (i = 0; i < escapes; i++)
		strlcat(tabs, "\t", sizeof(tabs));
//	if (bloq->id)
//		tabs[escapes] = bloq->id;
	if (bloq->data_char)
		Debug("%s%s \"%s\"%s", tabs, bloq->item,  bloq->data_char, bloq->down ? " {" : ";");
	else if (bloq->data_long)
		Debug("%s%s *%lu%s", tabs, bloq->item,  bloq->data_long, bloq->down ? " {" : ";");
	else
		Debug("%s%s %s", tabs, bloq->item, bloq->down ? " {" : ";");
	if (bloq->down)
	{
		printea(bloq->down, ++escapes);
		escapes--;
		Debug("%s};", tabs);
	}
	if (bloq->mid)
		printea(bloq->mid, escapes);
}
int IniciaUDB()
{
	int dataver;
	mkdir(DB_DIR, 0744);
	mkdir(DB_DIR_BCK, 0744);
	if (!S)
		S = AltaBloque('S', DB_DIR "set.udb", &UDB_SET);
	if (!N)
		N = AltaBloque('N', DB_DIR "nicks.udb", &UDB_NICKS);
	if (!C)
		C = AltaBloque('C', DB_DIR "canales.udb", &UDB_CANALES);
	if (!I)
		I = AltaBloque('I', DB_DIR "ips.udb", &UDB_IPS);
	if (!L)
		L = AltaBloque('L', DB_DIR "links.udb", &UDB_LINKS);
	if (!K)
		K = AltaBloque('K', DB_DIR "lines.udb", &UDB_LINES);
	AltaHash();
	switch ((dataver = CogeDataVer()))
	{
		case 0:
		case 1:
			ActualizaDataVer2();
		case 2:
			ActualizaDataVer3();
		case 3:
			ActualizaDataVer4();
	}
	SetDataVer(5);
	return 1;
}
u_long ObtieneHash(UDBloq *bloq)
{
	char *par;
	struct stat inode;
	u_long crc32 = 0L;
	size_t t;
	if (fstat(bloq->fd, &inode) < 0)
		return 0L;
	t = inode.st_size - INI_DATA;
	if (t <= 0)
		return 0L;
	par = Malloc(t);
	lseek(bloq->fd, INI_DATA, SEEK_SET);
	if (read(bloq->fd, par, t) == t)
		crc32 = Crc32(par, t);
	Free(par);
	return crc32;
}
int ActualizaHash(UDBloq *bloq)
{
	struct stat inode;
	bloq->crc32 = ObtieneHash(bloq);
	if (lseek(bloq->fd, INI_HASH, SEEK_SET) < 0)
		return 0;
	if (write(bloq->fd, &bloq->crc32, sizeof(bloq->crc32)) < 0)
		return 0;
	fsync(bloq->fd);
	if (fstat(bloq->fd, &inode) < 0)
		return 0;
	bloq->lof = inode.st_size - INI_DATA;
	return 1;
}
int CogeDataVer()
{
	FILE *fcrc;
	int dataver;
	if ((fcrc = fopen(DB_DIR "crcs", "r+b")))
	{
		char ver[3];
		if (fseek(fcrc, 72, SEEK_SET))
		{
			fclose(fcrc);
			return 0;
		}
		bzero(ver, 3);
		fread(ver, 1, 2, fcrc);
		fclose(fcrc);
		unlink(DB_DIR "crcs");
		if (!sscanf(ver, "%X", &dataver))
			return 0;
		return dataver;
	}
	return -1;
}
int SetDataVer(u_int v)
{
	UDBloq *bloq;
	for (bloq = ultimo; bloq; bloq = bloq->sig)
	{
		lseek(bloq->fd, INI_VER, SEEK_SET);
		if (write(bloq->fd, &v, sizeof(v)) < 0)
			return 0;
		fsync(bloq->fd);
	}
	return 1;
}
int ActualizaGMT(UDBloq *bloq, time_t gm)
{
	time_t hora = gm ? gm : time(0);
	lseek(bloq->fd, INI_GMT, SEEK_SET);
	if (write(bloq->fd, &hora, sizeof(hora)) < 0)
		return 0;
	fsync(bloq->fd);
	bloq->gmt = hora;
	return 0;
}
/* devuelve el puntero a todo el bloque a partir de su id o letra */
UDBloq *CogeDeId(u_int id)
{
	UDBloq *reg;
	for (reg = ultimo; reg; reg = reg->sig)
	{
		if (reg->letra == id || reg->id == id)
			return reg;
	}
	return NULL;
}
void InsertaRegistroEnHash(Udb *registro, u_int donde, char *clave)
{
	u_int hashv;
	hashv = HashCliente(clave) % MAX_HASH;
	registro->hsig = hash[donde][hashv];
	hash[donde][hashv] = registro;
}
int BorraRegistroDeHash(Udb *registro, u_int donde, char *clave)
{
	Udb *aux, *prev = NULL;
	u_int hashv;
	hashv = HashCliente(clave) % MAX_HASH;
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
Udb *BuscaBloque(char *clave, Udb *sup)
{
	u_int hashv;
	Udb *aux;
	if (!clave)
		return NULL;
	hashv = HashCliente(clave) % MAX_HASH;
	for (aux = hash[sup->id][hashv]; aux; aux = aux->hsig)
	{
		//if ((*(clave+1) == '\0' && !complow(aux->id, *clave)) || !compara(clave, aux->item))
		if (aux->up == sup && !strcasecmp(clave, aux->item))
			return aux;
	}
	return NULL;
}
Udb *CreaRegistro(Udb *bloque)
{
	Udb *reg;
	reg = (Udb *)Malloc(sizeof(Udb));
	reg->hsig = (Udb *)NULL;
	reg->data_char = reg->item = (char *)NULL;
	reg->data_long = 0L;
	reg->id = 0;
	reg->down = NULL;
	reg->up = bloque;
	reg->b64 = 0;
	reg->mid = bloque->down;
	bloque->down = reg;
	return reg;
}
Udb *DaFormato(char *form, Udb *reg, size_t t)
{
	Udb *root = NULL;
	form[0] = '\0';
	if (reg->up)
		root = DaFormato(form, reg->up, t);
	else
		return reg;
	if (!BadPtr(reg->item))
	{
		if (reg->b64)
		{
			char tmp[BUFSIZE];
			b64_encode(reg->item, strlen(reg->item), &tmp[1], BUFSIZE-1);
			tmp[0] = CHAR_B64;
			strlcat(form, tmp, t);
		}
		else
			strlcat(form, reg->item, t);
	}
	if (reg->down)
		strlcat(form, "::", t);
	else
	{
		if (reg->data_char)
		{
			strlcat(form, " ", t);
			if (*reg->data_char == CHAR_NUM)
				strlcat(form, "\\", t);
			strlcat(form, reg->data_char, t);
		}
		else if (reg->data_long)
		{
			char tmp[32];
			sprintf(tmp, " %c%lu", CHAR_NUM, reg->data_long);
			strlcat(form, tmp, t);
		}
	}
	return root ? root : reg;
}
int GuardaEnArchivo(Udb *reg, u_int tipo)
{
	char form[BUFSIZE];
	UDBloq *bloq;
	if (!(bloq = CogeDeId(tipo)))
		return 0;
	form[0] = '\0';
	DaFormato(form, reg, sizeof(form));
	strlcat(form, "\n", sizeof(form));
	lseek(bloq->fd, 0, SEEK_END);
	if (write(bloq->fd, form, sizeof(char) * strlen(form)) < 0)
		return 0;
	return 1;
}
int GuardaEnArchivoInv(Udb *reg, u_int tipo)
{
	if (!reg)
		return 0;
	if (reg->mid)
		GuardaEnArchivoInv(reg->mid, tipo);
	if (reg->down)
		GuardaEnArchivoInv(reg->down, tipo);
	else
		GuardaEnArchivo(reg, tipo);
	return 0;
}
int FijaCabecera(UDBloq *bloq)
{
	int fd;
	char path[BUFSIZE];
	size_t len = 0;
	u_long hash = 0L, gmt = 0L;
	u_int ver = 1;
	ircsprintf(path, "%s.tmp", bloq->path);
	if ((fd = open(path, O_CREAT | O_BINARY | O_RDWR, 0600)) < 0)
		return 1;
	if (write(fd, &bloq->id, sizeof(bloq->id)) < 0)
		return 2;
	if (write(fd, &ver, sizeof(ver)) < 0)
		return 3;
	if (write(fd, &hash, sizeof(hash)) < 0)
		return 4;
	if (write(fd, &gmt, sizeof(gmt)) < 0)
		return 5;
	lseek(bloq->fd, 0, SEEK_SET);
	while ((len = read(bloq->fd, buf, sizeof(buf))) > 0)
		write(fd, buf, len);
	fsync(bloq->fd);
	close(bloq->fd);
	close(fd);
	if (unlink(bloq->path) < 0)
		return 6;
	if (rename(path, bloq->path) < 0)
		return 7;
	if ((bloq->fd = open(bloq->path, O_CREAT | O_BINARY | O_RDWR, 0600)) < 0)
		return 8;
	ActualizaHash(bloq);
	return 0;
}
int CargaCabecera(UDBloq *bloq)
{
	u_int id = 0xFFFFFFFF;
	struct stat inode;
	int e;
	if ((bloq->fd = open(bloq->path, O_CREAT | O_BINARY | O_RDWR, 0600)) < 0)
		return -1;
	if (read(bloq->fd, &id, sizeof(id)) < 0)
		return -2;
	if (id != bloq->id && (e = FijaCabecera(bloq)))
		return e;
	lseek(bloq->fd, INI_VER, SEEK_SET);
	if (read(bloq->fd, &bloq->ver, sizeof(bloq->ver)) < 0)
		return -3;
	if (read(bloq->fd, &bloq->crc32, sizeof(bloq->crc32)) < 0)
		return -4;
	if (read(bloq->fd, &bloq->gmt, sizeof(bloq->gmt)) < 0)
		return -5;
	if (fstat(bloq->fd, &inode) < 0)
		return -6;
	bloq->lof = inode.st_size - INI_DATA;
	return 0;
}
int CargaBloque(u_int tipo)
{
	UDBloq *bloq;
	u_long obtiene, bytes = 0L;
	char linea[BUFSIZE], c;
	int i = 0, e;
	if (!(bloq = CogeDeId(tipo)))
	{
		Info("Ha sido imposible cargar el bloque %i", tipo);
		return 0;
	}
	if ((e = CargaCabecera(bloq)))
	{
		Info("Ha sido imposible cargar el bloque %i (%i:%i)", tipo, e, ERRNO);
		return 0;
	}
	obtiene = ObtieneHash(bloq);
	if (bloq->crc32 != obtiene)
	{
		Info("El bloque %c está corrupto (%lu != %lu)", bloq->letra, bloq->crc32, obtiene);
		ftruncate(bloq->fd, INI_DATA);
		ActualizaHash(bloq);
		if (linkado)
			EnviaAServidor(":%s DB %s RES %c 0", me.nombre, linkado->nombre, bloq->letra);
		return 0;
	}
	lseek(bloq->fd, INI_DATA, SEEK_SET);
	while (read(bloq->fd, &c, sizeof(c)) > 0)
	{
		if (c == '\r' || c == '\n')
		{
			linea[i] = '\0';
			bytes += strlen(linea) + 1;
			ParseaLinea(bloq, linea, 0);
			i = 0;
		}
		else
			linea[i++] = c;
	}
	if (i)
		TruncaBloque(bloq, bytes);
	return 1;
}
void DescargaBloque(u_int tipo)
{
	Udb *aux, *sig;
	UDBloq *root;
	if (!(root = CogeDeId(tipo)))
		return;
	for (aux = root->arbol->down; aux; aux = sig)
	{
		sig = aux->mid;
		BorraRegistro(tipo, aux, 0);
	}
	root->lof = 0L;
	root->regs = 0;
	VaciaHash(tipo);
	close(root->fd);
}
int CargaBloques()
{
	u_int i;
	for (i = 0; i < BDD_TOTAL; i++)
		DescargaBloque(i);
	for (i = 0; i < BDD_TOTAL; i++)
		CargaBloque(i);
	return 0;
}
int TruncaBloque(UDBloq *bloq, u_long bytes)
{
	if (ftruncate(bloq->fd, INI_DATA + bytes) < 0)
		return 0;
	ActualizaHash(bloq);
	DescargaBloque(bloq->id);
	CargaBloque(bloq->id);
	return 1;
}
int OptimizaBloque(UDBloq *bloq)
{
	if (ftruncate(bloq->fd, INI_DATA) < 0)
		return 0;
	GuardaEnArchivoInv(bloq->arbol->down, bloq->id);
	ActualizaHash(bloq);
	return 1;
}
void LiberaMemoriaUdb(u_int tipo, Udb *reg)
{
	if (reg->down)
	{
		BorraRegistroDeHash(reg->down, tipo, reg->down->item);
		LiberaMemoriaUdb(tipo, reg->down);
	}
	if (reg->mid)
	{
		BorraRegistroDeHash(reg->mid, tipo, reg->mid->item);
		LiberaMemoriaUdb(tipo, reg->mid);
	}
	//BorraRegistroDeHash(reg, tipo, reg->item);
	if (reg->data_char)
		Free(reg->data_char);
	if (reg->item)
		Free(reg->item);
	Free(reg);
}
Udb *BorraRegistro(u_int tipo, Udb *reg, int archivo)
{
	Udb *aux, *down, *prev = NULL, *up;
	UDBloq *bloq;
	if (!(up = reg->up)) /* estamos arriba de todo */
		return NULL;
	BorraRegistroDeHash(reg, tipo, reg->item);
	if (reg->data_char)
		Free(reg->data_char);
	reg->data_char = NULL;
	reg->data_long = 0L;
	down = reg->down;
	reg->down = NULL;
	bloq = CogeDeId(tipo);
	if (archivo)
	{
		GuardaEnArchivo(reg, tipo);
		ActualizaHash(bloq);
	}
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
	if (!reg->up->up && bloq)
		bloq->regs--;
	LiberaMemoriaUdb(tipo, reg);
	if (!up->down && up->up)
		up = BorraRegistro(tipo, up, archivo);
	return up;
}
Udb *InsertaRegistro(u_int tipo, Udb *bloque, char *item, char *data_char, u_long data_long, int archivo)
{
	Udb *reg;
	UDBloq *bloq;
	if (!bloque || !item)
		return NULL;
	bloq = CogeDeId(tipo);
	if (!(reg = BuscaBloque(item, bloque)))
	{
		reg = CreaRegistro(bloque);
		if (!bloque->up && bloq)
			bloq->regs++;
		reg->id = bloque->id;
		/*if (*(item+1) == '\0')
		{
			reg->id = *item;
			reg->item = strdup("");
		}
		else*/
			reg->item = strdup(item);
		InsertaRegistroEnHash(reg, tipo, item);
	}
	else
	{
		if ((!BadPtr(data_char) && !BadPtr(reg->data_char) && !strcmp(reg->data_char, data_char)) || (data_long == reg->data_long && data_long))
			return NULL;
	}
	if (reg->data_char)
		Free(reg->data_char);
	reg->data_char = NULL;
	if (data_char)
		reg->data_char = strdup(data_char);
	reg->data_long = data_long;
	if (archivo && (data_char || data_long))
	{
		GuardaEnArchivo(reg, tipo);
		ActualizaHash(bloq);
	}
	return reg;
}
int ParseaLinea(UDBloq *root, char *cur, int archivo)
{
	char *ds, *cop, *sp = NULL;
	Udb *bloq;
	cop = cur = strdup(cur);
	bloq = root->arbol;
	sp = strchr(cur, ' ');
	while ((ds = strchr(cur, ':')))
	{
		if (sp && sp < ds)
			break;
		if (*(ds + 1) == ':')
		{
			*ds++ = '\0';
			if (*cur)
			{
				if (*cur == CHAR_B64)
				{
					char tmp[BUFSIZE];
					b64_decode(cur+1, tmp, BUFSIZE);
					bloq = InsertaRegistro(bloq->id, bloq, tmp, NULL, 0, archivo);
					bloq->b64 = 1;
				}
				else
					bloq = InsertaRegistro(bloq->id, bloq, cur, NULL, 0, archivo);
			}
		}
		else /* ya no son :: */
			break;
		cur = ++ds;
	}
	if (sp)
	{
		*sp++ = '\0';
		if (BadPtr(sp))
			goto borra;
		if (*sp == '\\' && *(sp+1) == CHAR_NUM)
			bloq = InsertaRegistro(bloq->id, bloq, cur, sp+1, 0, archivo);
		else if (*sp == CHAR_NUM)
			bloq = InsertaRegistro(bloq->id, bloq, cur, NULL, atoul(++sp), archivo);
		else
			bloq = InsertaRegistro(bloq->id, bloq, cur, sp, 0, archivo);
	}
	else
	{
		borra:
		if (*cur == CHAR_B64)
		{
			char tmp[BUFSIZE];
			b64_decode(cur+1, tmp, BUFSIZE);
			if ((bloq = BuscaBloque(tmp, bloq)))
				bloq = BorraRegistro(bloq->id, bloq, archivo);
		}
		else
		{
			if ((bloq = BuscaBloque(cur, bloq)))
				bloq = BorraRegistro(bloq->id, bloq, archivo);
		}
	}
	Free(cop);
	if (bloq)
		return 0;
	return 1;
}
u_int LevelOperUdb(char *oper)
{
	Udb *reg;
	if ((reg = BuscaBloque(oper, UDB_NICKS)))
	{
		Udb *aux;
		if ((aux = BuscaBloque(N_OPE, reg)))
			return (u_int)aux->data_long;
	}
	return 0;
}
char *CifraIpTEA_U(char *ipreal)
{
	static char cifrada[512];
	char *p, *clavec = NULL, clave[13];
	int ts = 0;
	Udb *bloq;
	unsigned int ourcrc, v[2], k[2], x[2];
	bzero(clave, sizeof(clave));
	bzero(cifrada, sizeof(cifrada));
	ourcrc = Crc32(ipreal, strlen(ipreal));
	if ((bloq = BuscaBloque(S_CLA, UDB_SET)) && !BadPtr(bloq->data_char))
		clavec = bloq->data_char;
	else
		clavec = conf_set->clave_cifrado;
	strlcpy(clave, clavec, sizeof(clave));
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
				strlcpy(p, ipreal, sizeof(cifrada));
				break;
			}
		}
	}
	return cifrada;
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
		ParseaLinea(tipo, cur, 0);
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

/*!
 * @desc: Propaga un registro UDB por la red. Debe estar cargada la extensión UDB.
 * @params: $item [in] Cadena con formato a propagar.
 	    $... [in] Argumentos variables según cadena con formato.
 * @cat: IRCd
 !*/

void PropagaRegistro(char *item, ...)
{
	va_list vl;
	char r, buf[1024];
	UDBloq *bloq;
	u_long pos;
	va_start(vl, item);
	ircvsprintf(buf, item, vl);
	va_end(vl);
	r = buf[0];
	if (!(bloq = CogeDeId(r)))
		return;
	pos = bloq->lof;
	if (!ParseaLinea(bloq, &buf[3], 1))
	{
		if (strchr(buf, ' '))
			EnviaAServidor(":%s DB * INS %lu %s", me.nombre, pos, buf);
		else
			EnviaAServidor(":%s DB * DEL %lu %s", me.nombre, pos, buf);
	}
}
int ActualizaDataVer2()
{
	char *archivos[] = {
		"set.udb" ,
		"nicks.udb" ,
		"canales.udb" ,
		"ips.udb" ,
		NULL
	};
	char *tokens[][32] = {
		{
			"clave_cifrado" , "L" ,
			"sufijo" , "J" ,
			"NickServ" , "N" ,
			"ChanServ" , "C" ,
			"IpServ" , "I" ,
			"clones" , "S" ,
			"quit_ips" , "T" ,
			"quit_clones" , "Q" ,
			NULL
		} ,
		{
			"pass" , "P" ,
			"forbid" , "B" ,
			"vhost" , "V" ,
			"suspendido" , "S" ,
			"oper" , "O" ,
			"desafio" , "D" ,
			"modos" , "M" ,
			"snomasks" , "K" ,
			"swhois" , "W" ,
			NULL
		} ,
		{
			"fundador" , "F" ,
			"modos" , "M" ,
			"topic" , "T" ,
			"accesos" , "A" ,
			"forbid" , "B" ,
			"suspendido" , "S" ,
			NULL
		} ,
		{
			"clones" , "S" ,
			"nolines" , "E" ,
			NULL
		}
	};
	int vec[][8] = {
		{ 1 } ,
		{ 0 , 1 } ,
		{ 0 , 1 , 0 } ,
		{ 0 , 1 }
	};
	FILE *fp, *tmp;
	int i, j, k;
	char *c, *d, buf[8192], f, p1[PMAX], p2[PMAX];
	strlcpy(p2, DB_DIR "temporal", sizeof(p2));
	for (i = 0; archivos[i]; i++)
	{
		ircsprintf(p1, "%s%s", DB_DIR, archivos[i]);
		if (!(fp = fopen(p1, "rb")))
			return 1;
		if (!(tmp = fopen(p2, "wb")))
			return 1;
		while (fgets(buf, sizeof(buf), fp))
		{
			c = buf;
			f = 1;
			k = 0;
			while (*c != '\r' && *c != '\n')
			{
				if ((d = strchr(c, ':')) && *(d+1) == ':')
				{
					*d = '\0';
					f = 0;
					if (vec[i][k])
					{
						for (j = 0; tokens[i][j]; j += 2)
						{
							if (!strcmp(tokens[i][j], c))
							{
								fwrite(tokens[i][j+1], 1, strlen(tokens[i][j+1]), tmp);
								f = 1;
								break;
							}
						}
					}
					if (!f)
						fwrite(c, 1, strlen(c), tmp);
					fwrite("::", 1, 2, tmp);
					c = d+2;
					f = 1;
					k++;
				}
				else if (f && (d = strchr(c, ' ')))
				{
					*d = '\0';
					f = 0;
					if (vec[i][k])
					{
						for (j = 0; tokens[i][j]; j += 2)
						{
							if (!strcmp(tokens[i][j], c))
							{
								fwrite(tokens[i][j+1], 1, strlen(tokens[i][j+1]), tmp);
								f = 1;
								break;
							}
						}
					}
					if (!f)
						fwrite(c, 1, strlen(c), tmp);
					f = 0;
					fwrite(" ", 1, 1, tmp);
					c = d+1;
					k++;
				}
				else
				{
					fwrite(c, 1, strlen(c), tmp);
					break;
				}
			}
		}
		fflush(tmp);
		fclose(fp);
		fclose(tmp);
		unlink(p1);
		rename(p2, p1);
		unlink(p2);
		//ActualizaHash(CogeDeId(i));
	}
	return 0;
}
int ActualizaDataVer3()
{
	FILE *fp, *tmp;
	char *c, buf[8192];
	if (!(fp = fopen(DB_DIR "nicks.udb", "rb")))
		return 1;
	if (!(tmp = fopen(DB_DIR "temporal", "wb")))
		return 1;
	while (fgets(buf, sizeof(buf), fp))
	{
		if ((c = strchr(buf, ' ')))
		{
			if (*(c-2) == ':' && *(c-1) == *(N_OPE)) /* lo tenemos! */
			{
				int val;
				sscanf(c, " %*c%i\n", &val);
				if (val & 0x1)
					val &= ~0x1;
				if (val & 0x2)
				{
					val &= ~0x2;
					val |= 0x1;
				}
				if (val & 0x4)
					val &= ~0x4;
				if (val & 0x8)
				{
					val &= ~0x8;
					val |= 0x2;
				}
				if (val & 0x10)
				{
					val &= ~0x10;
					val |= 0x4;
				}
				*c = '\0';
				ircsprintf(buf, "%s %c%i\n", buf, CHAR_NUM, val);
			}
			fwrite(buf, 1, strlen(buf), tmp);
		}
	}
	fflush(tmp);
	fclose(fp);
	fclose(tmp);
	unlink(DB_DIR "nicks.udb");
	rename(DB_DIR "temporal", DB_DIR "nicks.udb");
	unlink(DB_DIR "temporal");
	//ActualizaHash(N);
	return 0;
}
int ActualizaDataVer4()
{
	//ActualizaHash(N);
	//ActualizaHash(C);
	//ActualizaHash(I);
	//ActualizaHash(S);
	return 0;
}
