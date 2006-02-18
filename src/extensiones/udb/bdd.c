#include "struct.h"
#include "ircd.h"
#include "bdd.h"
#include "md5.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#define O_BINARY 0x0
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
#define DaUdb(x) do{ x = (Udb *)Malloc(sizeof(Udb)); bzero(x, sizeof(Udb)); }while(0)
char bloques[DBMAX];
time_t gmts[DBMAX];
int regs[DBMAX];
u_int BDD_TOTAL = 0;
int dataver = 0;
FILE *fcrc = NULL;

void SetDataVer(int);
int GetDataVer();
int ActualizaHash(Udb *);
Udb *IdAUdb(int);
void CargaBloques();
int TruncaBloque(Cliente *, Udb *, u_long);

Opts NivelesBDD[] = {
	{ 0x1 , "OPER" } ,
	{ 0x2 , "ADMIN" } ,
	{ 0x4 , "ROOT" } ,
	{ 0x0 , 0x0 }
};
Udb *AltaBloque(char letra, char *ruta, u_int *id)
{
	int ids = 0;
	Udb *reg;
	if (ultimo)
		ids = ultimo->id + 1;
	reg = (Udb *)Malloc(sizeof(Udb));
	reg->data_char = (char *)NULL;
	reg->data_long = 0L;
	reg->id = ids;
	reg->item = ruta;
	reg->mid = ultimo;
	reg->hsig = reg->up = reg->down = (Udb *)NULL;
	*id = ids;
	ultimo = reg;
	bloques[BDD_TOTAL++] = letra;
	return reg;
}
void VaciaHash(int id)
{
	int i;
	for (i = 0; i < 2048; i++)
		hash[id][i] = NULL;
}
void AltaHash()
{
	Udb *reg;
	int id;
	hash = (Udb ***)Malloc(sizeof(Udb **) * BDD_TOTAL);
	for (reg = ultimo; reg; reg = reg->mid)
	{
		id = reg->id;
		hash[id] = (Udb **)Malloc(sizeof(Udb *) * 2048);
		VaciaHash(id);
		gmts[id] = 0L;
	}
}
void printea(Udb *bloq, int escapes)
{
	int i;
	char tabs[32];
	tabs[0] = '\0';
	for (i = 0; i < escapes; i++)
		strcat(tabs, "\t");
	if (bloq->id)
		tabs[escapes] = bloq->id;
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
	strncpy(p2, DB_DIR "temporal", sizeof(p2));
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
		fclose(fp);
		fclose(tmp);
		unlink(p1);
		rename(p2, p1);
		unlink(p2);
		ActualizaHash(IdAUdb(i));
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
			if (*(c-2) == ':' && *(c-1) == *(N_OPE_TOK)) /* lo tenemos! */
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
	fclose(fp);
	fclose(tmp);
	unlink(DB_DIR "nicks.udb");
	rename(DB_DIR "temporal", DB_DIR "nicks.udb");
	unlink(DB_DIR "temporal");
	ActualizaHash(nicks);
	return 0;
}
void BddInit()
{
	FILE *fh;
	int ver;
#ifdef _WIN32
	mkdir(DB_DIR);
#else
	mkdir(DB_DIR, 0744);
#endif
	bzero(bloques, sizeof(bloques));
	bzero(regs, sizeof(regs));
	if (!sets)
		sets = AltaBloque('S', DB_DIR "set.udb", &BDD_SET);
	if (!nicks)
		nicks = AltaBloque('N', DB_DIR "nicks.udb", &BDD_NICKS);
	if (!chans)
		chans = AltaBloque('C', DB_DIR "canales.udb", &BDD_CHANS);
	if (!ips)
		ips = AltaBloque('I', DB_DIR "ips.udb", &BDD_IPS);
	AltaHash();
	if ((fh = fopen(DB_DIR "crcs", "ab")))
	{
		fclose(fh);
		fcrc = fopen(DB_DIR "crcs", "r+b");
	}
	switch ((ver = GetDataVer()))
	{
		case 0:
		case 1:
			ActualizaDataVer2();
		case 2:
			ActualizaDataVer3();
	}
	SetDataVer(3);
	//printea(ips, 0);
}
void CifraCadenaAHex(char *origen, char *destino, int len)
{
	int i;
	destino[0] = 0;
	for (i = 0; i < len; i++)
	{
		char tmp[2];
		ircsprintf(tmp, "%02x", origen[i]);
		strcat(destino, tmp);
	}
}
u_long ObtieneHash(Udb *bloq)
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
		hashl = Crc32(par, inode.st_size);
	}
	close(fp);
	Free(par);
	return hashl;
}
u_long LeeHash(int id)
{
	u_long hash = 0L;
	char lee[9];
	if (fseek(fcrc, 8 * id, SEEK_SET))
		return 0L;
	bzero(lee, 9);
	fread(lee, 1, 8, fcrc);
	if (!sscanf(lee, "%lX", &hash))
		return 0L;
	return hash;
}
time_t LeeGMT(int id)
{
	char lee[11];
	if (fseek(fcrc, BDD_TOTAL * 8 + 10 * id, SEEK_SET))
		return 0L;
	bzero(lee, 11);
	fread(lee, 1, 10, fcrc);
	return (time_t)atoul(lee);
}
int GetDataVer()
{
	if (!dataver)
	{
		char ver[3];
		if (fseek(fcrc, 72, SEEK_SET))
			return 0;
		bzero(ver, 3);
		fread(ver, 1, 2, fcrc);
		if (!sscanf(ver, "%X", &dataver))
			return 0;
		return dataver;
	}
	return dataver;
}
void SetDataVer(int v)
{
	char ver[3];
	bzero(ver, 3);
	if (fseek(fcrc, 72, SEEK_SET))
		return;
	ircsprintf(ver, "%X", v);
	fwrite(ver, 1, 2, fcrc);
}
int ActualizaGMT(Udb *bloque, time_t gm)
{
	char lee[11];
	time_t hora = gm ? gm : time(0);
	bzero(lee, 11);
	if (fseek(fcrc, BDD_TOTAL * 8 + 10 * bloque->id, SEEK_SET))
		return -1;
	ircsprintf(lee, "%lu", hora);
	fwrite(lee, 1, 10, fcrc);
	gmts[bloque->id] = hora;
	return 0;
}
int ActualizaHash(Udb *bloque)
{
	char lee[9];
	u_long lo;
	bzero(lee, 9);
	if (fseek(fcrc, 8 * bloque->id, SEEK_SET))
		return -1;
	lo = ObtieneHash(bloque);
	ircsprintf(lee, "%lX", lo);
	fwrite(lee, 1, 8, fcrc);
	return 0;
}
/* devuelve el puntero a todo el bloque a partir de su id o letra */
Udb *IdAUdb(int id)
{
	Udb *reg;
	for (reg = ultimo; reg; reg = reg->mid)
	{
		if (bloques[reg->id] == id || reg->id == id)
			return reg;
	}
	return NULL;
}
void InsertaRegistroEnHash(Udb *registro, int donde, char *clave)
{
	u_int hashv;
	hashv = HashCliente(clave) % 2048;
	registro->hsig = hash[donde][hashv];
	hash[donde][hashv] = registro;
}
int BorraRegistroDeHash(Udb *registro, int donde, char *clave)
{
	Udb *aux, *prev = NULL;
	u_int hashv;
	hashv = HashCliente(clave) % 2048;
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
Udb *BuscaUdbEnHash(char *clave, int donde, Udb *lugar)
{
	u_int hashv;
	Udb *aux;
	hashv = HashCliente(clave) % 2048;
	for (aux = hash[donde][hashv]; aux; aux = aux->hsig)
	{
		if ((*(clave+1) == '\0' && aux->id == *clave) || !strcasecmp(clave, aux->item))
			return aux;
	}
	return lugar;
}
Udb *BuscaRegistro(int tipo, char *clave)
{
	if (!clave)
		return NULL;
	return BuscaUdbEnHash(clave, tipo, NULL);
}
Udb *BuscaBloque(char *clave, Udb *bloque)
{
	Udb *aux;
	if (!clave)
		return NULL;
	for (aux = bloque->down; aux; aux = aux->mid)
	{
		if ((*(clave+1) == '\0' && aux->id == *clave) || !strcasecmp(clave, aux->item))
			return aux;
	}
	return NULL;
}
Udb *CreaRegistro(Udb *bloque)
{
	Udb *reg;
	DaUdb(reg);
	reg->up = bloque;
	reg->mid = bloque->down;
	bloque->down = reg;
	return reg;
}
Udb *DaFormato(char *form, Udb *reg)
{
	Udb *root = NULL;
	form[0] = '\0';
	if (!reg)
		return reg;
	if (reg->up)
		root = DaFormato(form, reg->up);
	else
		return reg;
	if (reg->id)
		chrcat(form, reg->id);
	else
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
int GuardaEnArchivo(Udb *reg, int tipo)
{
	char form[BUFSIZE];
	Udb *root;
	FILE *fp;
	form[0] = '\0';
	root = DaFormato(form, reg);
	strcat(form, "\n");
	if (!(fp = fopen(root->item, "ab")))
		return -1;
	fputs(form, fp);
	fclose(fp);
	ActualizaHash(IdAUdb(tipo));
	return 0;
}
/* guarda en un archivo un registro pero hacia abajo */
int GuardaEnArchivoInv(Udb *reg, int tipo)
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
void LiberaMemoriaUdb(int tipo, Udb *reg)
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
	if (reg->data_char)
		Free(reg->data_char);
	if (reg->item)
		Free(reg->item);
	Free(reg);
}
void BorraRegistro(int tipo, Udb *reg, int archivo)
{
	Udb *aux, *down, *prev = NULL, *up;
	if (!(up = reg->up)) /* estamos arriba de todo */
		return;
	aux = IdAUdb(tipo);
	BorraRegistroDeHash(reg, tipo, reg->item);
	if (reg->data_char)
		Free(reg->data_char);
	reg->data_char = NULL;
	reg->data_long = 0L;
	down = reg->down;
	reg->down = NULL;
	if (archivo)
		GuardaEnArchivo(reg, tipo);
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
	if (!reg->up->up)
		regs[tipo]--;
	LiberaMemoriaUdb(tipo, reg);
	if (!up->down)
		BorraRegistro(tipo, up, archivo);
}
Udb *InsertaRegistro(int tipo, Udb *bloque, char *item, char *data_char, u_long data_long, int archivo)
{
	Udb *reg, *root;
	if (!bloque || !item)
		return NULL;
	root = IdAUdb(tipo);
	if (!(reg = BuscaBloque(item, bloque)))
	{
		reg = CreaRegistro(bloque);
		if (!bloque->up)
			regs[tipo]++;
		if (*(item+1) == '\0')
		{
			reg->id = *item;
			reg->item = strdup("");
		}
		else
			reg->item = strdup(item);
		InsertaRegistroEnHash(reg, tipo, item);
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
		GuardaEnArchivo(reg, tipo);
	return reg;
}
int NivelOperBdd(char *oper)
{
	Udb *reg;
	if ((reg = BuscaRegistro(BDD_NICKS, oper)))
	{
		Udb *aux;
		if ((aux = BuscaBloque(N_OPE_TOK, reg)))
			return aux->data_long;
	}
	return 0;
}
char *CifraIpTEA_U(char *ipreal)
{
	static char cifrada[512], clave[13];
	char *p, *clavec;
	int ts = 0;
	Udb *bloq;
	unsigned int ourcrc, v[2], k[2], x[2];
	ourcrc = Crc32(ipreal, strlen(ipreal));
	if ((bloq = BuscaRegistro(BDD_SET, S_CLA_TOK)))
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
int ParseaLinea(int tipo, char *cur, int archivo)
{
	char *ds, *cop, *sp;
	Udb *bloq;
	int ins = 0;
	bloq = IdAUdb(tipo);
	cop = cur = strdup(cur);
	while ((ds = strchr(cur, ':')))
	{
		if (*(ds + 1) == ':')
		{
			*ds++ = '\0';
			if ((sp = strchr(cur, ' ')))
				*sp = '\0';
			bloq = InsertaRegistro(tipo, bloq, cur, sp, 0, archivo);
		}
		else /* ya no son :: */
			break;
		cur = ++ds;
	}
	if ((ds = strchr(cur, ' ')))
	{
		*ds++ = '\0';
		if (BadPtr(ds))
			goto borra;
		if (*ds == CHAR_NUM) /* son todo dígitos */
		{
			if (InsertaRegistro(tipo, bloq, cur, NULL, atoul(++ds), archivo))
				ins = 1;
		}
		else
		{
			if (InsertaRegistro(tipo, bloq, cur, ds, 0, archivo))
				ins = 1;
		}
	}
	else
	{
		borra:
		if ((bloq = BuscaBloque(cur, bloq)))
		{
			BorraRegistro(tipo, bloq, archivo);
			ins = -1;
		}
	}
	Free(cop);
	return ins;
}
void CargaBloque(int tipo)
{
	Udb *root;
	u_long lee, obtiene, trunca = 0L, bytes = 0L;
	char linea[BUFSIZE], *c;
	FILE *fp;
	root = IdAUdb(tipo);
	lee = LeeHash(tipo);
	obtiene = ObtieneHash(root);
	if (lee != obtiene)
	{
		Info("El bloque %c está corrupto (%lu != %lu)", bloques[root->id], lee, obtiene);
		if ((fp = fopen(root->item, "wb")))
		{
			fclose(fp);
			ActualizaHash(root);
		}
		if (linkado)
			EnviaAServidor(":%s DB %s RES %c 0", me.nombre, linkado->nombre, bloques[root->id]);
		return;
	}
	gmts[tipo] = LeeGMT(tipo);
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
			ParseaLinea(tipo, linea, 0);
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
	if (trunca)
		TruncaBloque(&me, root, trunca);
}
void DescargaBloque(int tipo)
{
	Udb *aux, *sig, *bloq;
	bloq = IdAUdb(tipo);
	for (aux = bloq->down; aux; aux = sig)
	{
		sig = aux->mid;
		BorraRegistro(tipo, aux, 0);
	}
	bloq->data_long = 0L;
	VaciaHash(tipo);
}
void CargaBloques()
{
	u_int i;
	for (i = 0; i < BDD_TOTAL; i++)
		DescargaBloque(i);
	for (i = 0; i < BDD_TOTAL; i++)
		CargaBloque(i);
}
int TruncaBloque(Cliente *cl, Udb *bloq, u_long bytes)
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
			EnviaAServidor(":%s DB %s ERR DRP %i %c fread", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
			return 1;
		}
		fclose(fp);
		if ((fp = fopen(bloq->item, "wb")))
		{
			if (fwrite(contenido, 1, bytes, fp) != bytes)
			{
				fclose(fp);
				EnviaAServidor(":%s DB %s ERR DRP %i %c fwrite", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
				return 1;
			}
			fclose(fp);
			ActualizaHash(bloq);
			DescargaBloque(bloq->id);
			CargaBloque(bloq->id);
		}
		else
		{
			EnviaAServidor(":%s DB %s ERR DRP %i %c fopen(wb)", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
			return 1;
		}
		if (contenido)
			Free(contenido);
	}
	else
	{
		EnviaAServidor(":%s DB %s ERR DRP %i %c fopen(rb)", me.nombre, cl->nombre, E_UDB_FATAL, bdd);
		return 1;
	}
	return 0;
}
int Optimiza(Udb *bloq)
{
	FILE *fp;
	if ((fp = fopen(bloq->item, "wb")))
		fclose(fp);
	GuardaEnArchivoInv(bloq->down, bloq->id);
	DescargaBloque(bloq->id);
	CargaBloque(bloq->id);
	return 0;
}
/*!
 * @desc: Propaga un registro UDB por la red.
 * @params: $item [in] Cadena con formato a propagar.
 	    $... [in] Argumentos variables según cadena con formato.
 * @cat: IRCd
 !*/
 
void PropagaRegistro(char *item, ...)
{
	va_list vl;
	char r, buf[1024];
	int ins;
	Udb *bloq;
	u_long pos;
	va_start(vl, item);
	ircvsprintf(buf, item, vl);
	va_end(vl);
	r = buf[0];
	bloq = IdAUdb(r);
	pos = bloq->data_long;
	ins = ParseaLinea(bloq->id, &buf[3], 1);
	if (strchr(buf, ' '))
	{
		if (ins == 1)
			EnviaAServidor(":%s DB * INS %lu %s", me.nombre, pos, buf);
	}
	else
	{
		if (ins == -1)
			EnviaAServidor(":%s DB * DEL %lu %s", me.nombre, pos, buf);
	}
}
