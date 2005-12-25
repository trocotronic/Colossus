/*
 * $Id: main.c,v 1.64 2005-12-25 21:13:55 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#ifdef UDB
#include "bdd.h"
#endif
#ifdef USA_ZLIB
#include "zip.h"
#endif
#include <signal.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <utime.h>
#include <sys/resource.h>
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Sock *SockActual;
Senyal *senyals[MAXSIGS];
Timer *timers = NULL;
Proc *procs = NULL;
char tokbuf[BUFSIZE];
#ifdef USA_CONSOLA
char conbuf[128];
HANDLE hStdin;
#endif
#ifdef _WIN32
char spath[MAX_PATH];
#else
char spath[PATH_MAX];
#endif
/*!
 * @desc: Indica si se está refrescando el programa: 1 si lo está haciendo; 0, si no.
 * @cat: Programa
 !*/
int refrescando = 0;


MODVAR char **margv;
/*!
 * @desc: Hora timestamp en la que se inició el programa.
 * @cat: Programa
 !*/
time_t iniciado;

int CargaCache();
int ProcCache();
extern void LeeSocks();
extern void CierraSocks();

#ifdef USA_CONSOLA
void *consola_loop_principal(void *);
void parsea_comando(char *);
#endif
#ifdef _WIN32
void LoopPrincipal(void *);
#endif
#define DescargaProtocolo() do { if (protocolo) { LiberaMemoriaProtocolo(protocolo); protocolo = NULL;	} }while(0)
SOCKFUNC(MotdAbre);
SOCKFUNC(MotdLee);

#ifndef _WIN32
const char logo[] = {
	32 , 32 , 95 , 95 , 95 , 95 , 95 , 32 , 32 , 32 , 32 , 32 , 32 , 95 , 10 ,
	32 , 47 , 95 , 95 , 95 , 95 , 95 , 124 , 32 , 32 , 32 , 32 , 124 , 32 , 124 , 10 ,
	124 , 32 , 124 , 32 , 32 , 32 , 32 , 32 , 95 , 95 , 95 , 32 , 124 , 32 , 124 , 32 , 95 , 95 , 95 , 32 , 32 , 95 , 95 , 95 , 32 , 95 , 95 , 95 , 32 , 95 , 32 , 32 , 32 , 95 , 32 , 95 , 95 , 95 , 10 ,
	124 , 32 , 124 , 32 , 32 , 32 , 32 , 47 , 32 , 95 , 32 , 92 , 124 , 32 , 124 , 47 , 32 , 95 , 32 , 92 , 47 , 32 , 95 , 95 , 47 , 32 , 95 , 95 , 124 , 32 , 124 , 32 , 124 , 32 , 47 , 32 , 95 , 95 , 124 , 10 ,
	124 , 32 , 124 , 95 , 95 , 95 , 124 , 32 , 40 , 95 , 41 , 32 , 124 , 32 , 124 , 32 , 40 , 95 , 41 , 32 , 92 , 95 , 95 , 32 , 92 , 95 , 95 , 32 , 92 , 32 , 124 , 95 , 124 , 32 , 92 , 95 , 95 , 32 , 92 , 10 ,
	32 , 92 , 95 , 95 , 95 , 95 , 95 , 92 , 95 , 95 , 95 , 47 , 124 , 95 , 124 , 92 , 95 , 95 , 95 , 47 , 124 , 95 , 95 , 95 , 47 , 95 , 95 , 95 , 47 , 92 , 95 , 95 , 44 , 95 , 124 , 95 , 95 , 95 , 47 , 10 ,
	0 
};
#endif
char *ExMalloc(size_t size, char *file, long line)
{
	void *x;
	x = malloc(size);
#ifdef DEBUG
	Debug("Dando direccion %X", x);
#endif
	if (!x)
	{
		Alerta(FERR,  "[%s:%i] Te has quedado sin memoria", file, line);
		exit(-1);
	}
	return x;
}

/* 
 * print_r
 * Imprime una estructura de forma legible
 */
void print_r(Conf *conf, int escapes)
{
	int i;
	char tabs[32];
	tabs[0] = '\0';
	for (i = 0; i < escapes; i++)
		strcat(tabs, "\t");
	Error("%s%s%s%s%s%s", tabs, conf->item, conf->data ? " \"" : "", conf->data ? conf->data : "", conf->data ? "\"" : "", conf->secciones ? " {" : ";");
	if (conf->secciones)
		escapes++;
	for (i = 0; i < conf->secciones; i++)
		print_r(conf->seccion[i], escapes);
	if (conf->secciones)
	{
		escapes--;
		tabs[0] = '\0';
		for (i = 0; i < escapes; i++)
			strcat(tabs, "\t");
		Error("%s};", tabs);
	}
}
void IniciaProcesos()
{
	for(;;)
	{
#ifdef USA_CONSOLA
		if (!BadPtr(conbuf))
		{
			parsea_comando(conbuf);
			conbuf[0] = '\0';
		}
#endif
		LeeSocks();
		CompruebaCronos();
		ProcesosAuxiliares();
	}
	/* si llegamos aquí estamos saliendo del programa */
	CierraSocks();
#ifdef _WIN32
	ChkBtCon(0, 0);
#endif
}
void EscribePid()
{
#ifdef PID
	int  fd;
	char buff[20];
	if ((fd = open(PID, O_CREAT | O_WRONLY, 0600)) >= 0)
	{
		bzero(buff, sizeof(buff));
		sprintf(buff, "%5i\n", (int)getpid());
		if (write(fd, buff, strlen(buff)) == -1)
			Debug("No se puede escribir el archivo pid %s", PID);
		close(fd);
		return;
	}
#ifdef	DEBUG
	else
		Debug("No se puede abrir el archivo pid %s", PID);
#endif
#endif
}
int SigPostNick(Cliente *cl, char nuevo)
{
	Nivel *niv;
	if (IsId(cl) && (niv = BuscaNivel("ROOT")) && !strcasecmp(conf_set->root, cl->nombre))
		cl->nivel |= niv->nivel;
	return 0;
}
void BorraTemporales()
{
	Modulo *mod;
	for (mod = modulos; mod; mod = mod->sig)
	{
		//Alerta(FOK,"eliminando %s",mod->tmparchivo);
		unlink(mod->tmparchivo);
	}
}
void CierraTodo()
{
	CierraSocks();
	BorraTemporales();
#ifdef _WIN32
	DestroyWindow(hwMain);
#endif
}
VOIDSIG Refresca()
{
	Conf config;
#ifdef	POSIX_SIGNALS
	struct sigaction act;
#endif
	Info("Refrescando servicios...");
	refrescando = 1;
	DescargaModulos();
	DescargaProtocolo();
	DescargaConfiguracion();
	ParseaConfiguracion(CPATH, &config, 1);
	DistribuyeConfiguracion(&config);
	CargaModulos();
	Senyal(SIGN_SQL);
#ifdef UDB
	CargaBloques();
#endif
	if (SockIrcd)
	{
		Senyal(SIGN_SYNCH);
		Senyal(SIGN_EOS);
	}
	else
		DistribuyeMe(&me, &SockIrcd);
#ifdef	POSIX_SIGNALS
	act.sa_handler = Refresca;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
#else
  #ifndef _WIN32
	(void)signal(SIGHUP, s_rehash);
  #endif
#endif
	refrescando = 0;
}
VOIDSIG Reinicia()
{
	CierraTodo();
#ifdef _WIN32
	CleanUp();
	execv(margv[0], margv);
#else
	execv(SPATH, margv);
#endif
	exit(-1);
}
/*!
 * @desc: Cierra el programa
 * @params: $excode [in] Indica el código de salida. Este número es el que se pasa en la llamada exit()
 * @cat: Programa
 !*/

VOIDSIG CierraColossus(int excode)
{
	CierraTodo();
#ifdef _WIN32
	CleanUp();
#endif
	exit(excode);
}
#ifdef _WIN32
int IniciaPrograma(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	Conf config;
	int i;
#ifdef FORCE_CORE
	struct rlimit corelim;
#endif
	char nofork = 0;
	iniciado = time(0);
	ListaSocks.abiertos = ListaSocks.tope = 0;
	for (i = 0; i < MAXSOCKS; i++)
		ListaSocks.socket[i] = NULL;
	memset(senyals, (int)NULL, sizeof(senyals));
#ifdef _WIN32	
	mkdir("tmp");
	_getcwd(spath, sizeof(spath));
#else
	mkdir("tmp", 0744);
	getcwd(spath, sizeof(spath));
#endif	
	/*{
		char volname[MAX_PATH+1], filename[MAX_PATH+1];
		DWORD serial, maxcom, sysflags;
		GetVolumeInformation(NULL, volname, MAX_PATH+1, &serial, &maxcom, &sysflags, filename, MAX_PATH+1);
		Debug("%i %i %s %s", serial, maxcom, volname, filename);
		QueryDosDevice(NULL, volname, MAX_PATH+1);
		Debug("%s", volname);
	}*/
#ifdef FORCE_CORE
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
		printf("unlimit core size failed; errno = %d\n", errno);
#endif
	/* rutina del unreal */
	while (--argc > 0 && (*++argv)[0] == '-') 
	{
		char *p = argv[0] + 1;
		int  flag = *p++;
		if (flag == '\0' || *p == '\0') {
			if (argc > 1 && argv[1][0] != '-') 
			{
				p = *++argv;
				argc--;
			} 
			else
				p = "";
		}
		switch (flag) 
		{
		  case 'F':
			  nofork = 1;
			  break;
		  case 's':
			  (void)printf("sizeof(Cliente) == %li\n", (long)sizeof(Cliente));
			  (void)printf("sizeof(Canal) == %li\n", (long)sizeof(Cliente));
			  (void)printf("sizeof(Sock) == %li\n", (long)sizeof(Sock));
			  (void)printf("sizeof(Modulo) == %li\n", (long)sizeof(Modulo));
			  (void)printf("sizeof(SQL) == %li\n", (long)sizeof(SQL));
			  (void)printf("sizeof(Protocolo) == %li\n", (long)sizeof(Protocolo));
			  exit(0);
			  break;
#ifndef _WIN32
		  case 'v':
			  (void)printf("%s\n", COLOSSUS_VERNUM);
#else
		  case 'v':
			  MessageBox(NULL, COLOSSUS_VERSION, "Versión Colossus/Win32", MB_OK);
#endif
			  exit(0);
		}
	}
#if !defined(_WIN32) && defined(DEFAULT_PERMISSIONS)
	chmod(CPATH, DEFAULT_PERMISSIONS);
#endif
#ifndef _WIN32
  	for (i = 0; logo[i] != 0; i++)
		fprintf(stderr, "%c", logo[i]);
	fprintf(stderr, "\n\t\t" COLOSSUS_VERSION "\n");
  #ifdef UDB
	fprintf(stderr, "\t\t+UDB " UDB_VER "\n");
  #endif
  #ifdef USA_ZLIB
	fprintf(stderr, "\t\t+ZLIB %s\n", zlibVersion());
  #endif
  #ifdef USA_SSL
	fprintf(stderr, "\t\t+%s\n", OPENSSL_VERSION_TEXT);
  #endif
#endif
	for (i = 0; i < UMAX; i++)
	{
		uTab[i].item = NULL;
		uTab[i].items = 0;
	}
	for (i = 0; i < CHMAX; i++)
	{
		cTab[i].item = NULL;
		cTab[i].items = 0;
	}
	/* las primeras señales deben ser del núcleo */
	InsertaSenyal(SIGN_SYNCH, EntraBots);
	InsertaSenyal(SIGN_EOS, EntraResidentes);
	InsertaSenyal(SIGN_POST_NICK, SigPostNick);
	if (ParseaConfiguracion(CPATH, &config, 1) < 0)
		return 1;
	DistribuyeConfiguracion(&config);
	DistribuyeMe(&me, &SockIrcd);
	CargaModulos();
	Senyal(SIGN_SQL);
#ifndef _WIN32
	if (sql->clientinfo)
		fprintf(stderr, "\t\t+Cliente SQL %s\n", sql->clientinfo);
	if (sql->servinfo)
		fprintf(stderr, "\t\t+Servidor SQL %s\n", sql->servinfo);
	fprintf(stderr, "\n\t\tTrocotronic - http://www.rallados.net\n");
	fprintf(stderr, "\t\t(c)2004-2005\n");
	fprintf(stderr, "\n");
#endif
/*	if (EsArchivo("backup.sql"))
	{
#ifdef _WIN32		
		if (MessageBox(hwMain, "Se ha encontrado una copia de la base de datos. ¿Quieres restaurarla?", "SQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
		if (Pregunta("Se ha encontrado una copia de la base de datos. ¿Quieres restaurarla?") == 1)
#endif
		{
			if (!SQLRestaura())
			{
#ifdef _WIN32				
				if (MessageBox(hwMain, "Se ha restaurado con éxito. ¿Quieres borrar la copia?", "SQL", MB_YESNO|MB_ICONQUESTION) == IDYES)
#else
				if (Pregunta("Se ha restaurado con éxito. ¿Quieres borrar la copia?") == 1)
#endif
					remove("backup.sql");
			}
			else
			{
				Alerta(FSQL, "Ha sido imposible restaurar la copia.");
				CierraColossus(-1);
			}
		}
	}
	*/
	margv = argv;
	CargaCache();
#ifdef UDB
	BddInit();
#endif
	pthread_mutex_init(&mutex, NULL);
#ifdef _WIN32
	signal(SIGSEGV, CleanUpSegv);
#else
  #ifdef POSIX_SIGNALS
  	struct sigaction act;
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
	act.sa_handler = Reinicia;
	(void)sigaddset(&act.sa_mask, SIGINT);
	(void)sigaction(SIGINT, &act, NULL);
	act.sa_handler = CierraColossus;
	(void)sigaddset(&act.sa_mask, SIGTERM);
	(void)sigaction(SIGTERM, &act, NULL);
	act.sa_handler = AbreSockIrcd;
	(void)sigaddset(&act.sa_mask, SIGPIPE);
	(void)sigaction(SIGPIPE, &act, NULL);
  #elif BSD_RELIABLE_SIGNALS
	signal(SIGHUP, Refresca);
	signal(SIGTERM, CierraColossus);
	signal(SIGINT, Reinicia);
	signal(SIGPIPE, AbreSockIrcd);
  #endif
#endif
	SockOpen("www.rallados.net", 80, MotdAbre, MotdLee, NULL, NULL, ADD);
#ifndef _WIN32
	if (!nofork && fork())
		exit(0);
	EscribePid();
	AbreSockIrcd();
#else
	EscuchaIrcd();
	return 0;
}
void LoopPrincipal(void *args)
{
#endif
	intentos = 0;
	IniciaProcesos();
#ifndef _WIN32
	return 0;
#endif
}

/*!
 * @desc: Indica si un archivo existe o no
 * @params: $archivo [in] Ruta completa del archivo
 * @ret: Devuelve 1 si existe; 0, si no.
 * @cat: Programa
 !*/

int EsArchivo(char *archivo)
{
	FILE *fp;
	if (!(fp = fopen(archivo, "r")))
		return 0;
	fclose(fp);
	return 1;
}

/*!
 * @desc: Indica si se trata de una ip en notación por puntos (X.X.X.X)
 * @params: $ip [in] Ip a analizar
 * @ret: Devuelve 1 si se trata de una ip en notación por puntos; 0, si no.
 * @cat: Internet
 !*/

int EsIp(char *ip)
{
	char *oct;
	if (!ip)
		return 0;
	if ((oct = strrchr(ip, '.')))
	{
		if (*(++oct) < 65) /* es ip */
			return 1;
	}
	return 0;
}
typedef struct
{
	char **destino;
	char *ip;
}Host;
void Dominio(Host *aux)
{
	struct hostent *he;
	int addr;
	addr = inet_addr(aux->ip);
	if ((he = gethostbyaddr((char *)&addr, 4, AF_INET)))
	{
		*aux->destino = strdup(he->h_name);
		InsertaCache(CACHE_HOST, aux->ip, 86400, 0, he->h_name);
	}
	else
		ircfree(*aux->destino);
	Free(aux->ip);
	Free(aux);
}

/*!
 * @desc: Resuelve una ip asíncronamente
 * @params: $destino [out] Dirección del puntero que apunta a la zona de memoria donde se guardará el resultado
 	    $ip [in] Dirección IP en notación de puntos a resolver
 * @cat: Internet
 !*/

void ResuelveHost(char **destino, char *ip)
{
	char *cache;
	if (EsIp(ip))
	{
		if ((cache = CogeCache(CACHE_HOST, ip, 0)))
			*destino = strdup(cache);
		else
		{
			pthread_t id;
			Host *aux;
			BMalloc(aux, Host);
			aux->destino = destino;
			aux->ip = strdup(ip);
			pthread_create(&id, NULL, (void *)Dominio, (void *)aux);
		}
	}
	else
		*destino = strdup(ip);
}

/*!
 * @desc: Genera un número aleatorio entre márgenes
 * @params: $ini [in] Margen inferior
 	    $fin [in] Margen superior
 * @ret: Devuelve el número generado
 * @cat: Programa
 !*/

int Aleatorio(int ini, int fin)
{
	int r, i;
#ifdef _WIN32
	struct _timeb aburst;
	_ftime(&aburst);
#else
	struct timeval aburst;
	gettimeofday(&aburst, NULL);
#endif
	i = (int)&aburst;
	srand(i ^ (time(NULL) + rand()));
	r = rand() % (fin + 1);
#ifdef _WIN32
	srand(i ^ (aburst.millitm + rand()));
#else
	srand(i ^ (aburst.tv_usec + rand()));
#endif
	if (r < ini)
		return fin - r;
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
 * @desc: Inserta una señal
 * Durante el transcurso del programa se generan varias señales. Cada vez que salta una señal se ejecutan las funciones que estén asociadas a ella.
 * @params: $senyal [in] Tipo de señal a seguir. Estas señales pueden pasar un número indefinido de parámetros, según sea el tipo de señal
 * 	    Sólo se aceptan las siguientes señales (entre paréntesis se detallan los parámetros que aceptan):
 		- SIGN_SQL (): Ha terminado la carga del motor SQL
 		- SIGN_UMODE (Cliente *cl, char *umodos): Se ha producido un cambio en los modos de usuario del cliente <i>cl</i>. La cadena <i>umodos</i> contiene los cambios.
 		- SIGN_QUIT (Cliente *cl, char *mensaje: El cliente <i>cl</i> ha sido desconectado con el mensaje <i>mensaje</i>.
 		- SIGN_EOS (): Se ha terminado la unión entre servidores.
 		- SIGN_MODE (Cliente *cl, Canal *cn, char *modos): El cliente <i>cl</i> ha efectuado el cambio de modos <i>modos</i> del canal <i>cn</i>.
 		- SIGN_JOIN (Cliente *cl, Canal *cn): El cliente <i>cl</i> se une al canal <i>cn</i>.
 		- SIGN_SYNCH (): Se inicia la sincronización con el servidor.
 		- SIGN_KICK (Cliente *cl, Cliente *al, Canal *cn, char *motivo): El cliente <i>cl</i> expulsa al cliente <i>al</i> del canal <i>cn</i> con el motivo <i>motivo</i>.
 		- SIGN_TOPIC (Cliente *cl, Canal *cn, char *topic): El cliente <i>cl</i> pone el topic <i>topic</i> en el canal <i>cn</i>.
 		- SIGN_PRE_NICK (Cliente *cl, char *nuevo): El cliente <i>cl</i> va a cambiarse el nick a <i>nuevo</i>.
 		- SIGN_POST_NICK (Cliente *cl, int modo): El cliente <i>cl</i> ha efectuado una operación de nick. Si es una nueva conexión, <i>modo</i> vale 0.
 		- SIGN_AWAY (Cliente *cl, char *mensaje): El cliente <i>cl</i> se pone away con el mensaje <i>mensaje</i>. Si mensaje apunta a NULL, el cliente regresa de away.
 		- SIGN_PART (Cliente *cl, Canal *cn, char *mensaje): El cliente <i>cl</i> abandona el canal <i>cn</i> con el mensaje <i>mensaje</i>. Si no hay mensaje, apunta a NULL.
 *	    $func [in] Función a ejecutar. Esta función debe estar definida según sea el tipo de señal que controla.
 		Recibirá los parámetros que se han descrito arriba. Por ejemplo, si es una función para una señal SIGN_UMODE, recibirá 2 parámetros.
 * @ex: 	int Umodos(Cliente *, char *);
 	InsertaSenyal(SIGN_UMODE, Umodos);
 	...
 	int Umodos(Cliente *cl, char *umodos)
 	{
 		...
 		printf("El usuario ha cambiado sus modos");
 		return 0;
 	}
 * @ver: BorraSenyal
 * @cat: Señales
 !*/

void InsertaSenyal(short senyal, int (*func)())
{
	Senyal *sign, *aux;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
			return;
	}
	BMalloc(sign, Senyal);
	sign->senyal = senyal;
	sign->func = func;
	if (!senyals[senyal])
		senyals[senyal] = sign;
	else
	{
		for (aux = senyals[senyal]; aux; aux = aux->sig)
		{
			if (!aux->sig)
			{
				aux->sig = sign;
				break;
			}
		}
	}
}

/*!
 * @desc: Elimina una señal
 * @params: $senyal [in] Tipo de señal. Ver InsertaSenyal para más información.
 * 	    $func [in] Función que se desea eliminar.
 * @ret: Devuelve 1 en caso de que sea eliminado; 0, si no.
 * @ver: InsertaSenyal
 * @cat: Señales
 !*/

int BorraSenyal(short senyal, int (*func)())
{
	Senyal *aux, *prev = NULL;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				senyals[senyal] = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
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
 * @desc: Inicia un cronométro.
 * Estos cronómetros permiten ejecutar funciones varias veces en un intervalo de tiempo.
 * @params: $nombre [in] Cadena que indica el nombre del crono.
 	    $sck [in] Socket relacionado. Si es un cronómetro independiente, dejarlo en NULL.
 	    Esto permite tener varios cronómetros con el mismo nombre siempre que tengan conexiones distintas.
 	    $veces [in] Entero que indica el número de veces que se ejecutará la función. Si es 0, la función se ejecuta indefinidamente.
 	    $cada [in] Entero que indica los segundos entre veces que se ejecuta la función.
 	    $func [in] Puntero a la función a ejecutar. Debe ser de tipo int.
 	    $args [in] Puntero genérico que apunta a un tipo de datos que se le pasaran a la función cada vez que sea ejecutada.
 	    $sizearg [in] Tamaño del tipo de datos que se pasa a la función.
 * @ver: ApagaCrono
 * @cat: Cronometros
 !*/

void IniciaCrono(char *nombre, Sock *sck, int veces, int cada, int (*func)(), void *args, size_t sizearg)
{
	Timer *aux;
	BMalloc(aux, Timer);
	aux->nombre = strdup(nombre);
	aux->sck = sck;
	aux->cuando = microtime() + cada;
	aux->veces = veces;
	aux->cada = cada;
	aux->func = func;
	if (args)
	{
		aux->args = Malloc(sizearg);
		memcpy(aux->args, args, sizearg);
	}
	AddItem(aux, timers);
}

/*!
 * @desc: Detiene un cronómetro
 * @params: $nombre [in] Nombre del cronómetro.
 	    $sck [in] Conexión a la que pertenece.
 * @ret: Devuelve 1 si se detiene; 0, si no.
 * @ver: IniciaCrono
 * @cat: Cronometros
 !*/

int ApagaCrono(char *nombre, Sock *sck)
{
	Timer *aux, *prev = NULL;
	for (aux = timers; aux; aux = aux->sig)
	{
		if (!strcasecmp(nombre, aux->nombre) && aux->sck == sck)
		{
			Free(aux->nombre);
			Free(aux->args);
			if (prev)
				prev->sig = aux->sig;
			else
				timers = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void CompruebaCronos()
{
	Timer *aux, *sig;
	double ms;
	if (refrescando)
		return;
	ms = microtime();
	for (aux = timers; aux; aux = sig)
	{
		sig = aux->sig;
		if (aux->cuando < ms)
		{
			if (aux->args)
				aux->func(aux->args);
			else
				aux->func();
			aux->lleva++;
			aux->cuando = ms + aux->cada;
			if (aux->lleva == aux->veces)
				ApagaCrono(aux->nombre, aux->sck);
		}
	}
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
	int i;
	imp[0] = '\0';
	for (i = parte; i < total; i++)
	{
		strcat(imp, array[i]);
		if (i != total - 1)
			strcat(imp, " ");
		if (i == hasta)
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
void ProcesosAuxiliares()
{
	Proc *aux;
	if (refrescando)
		return;
	for (aux = procs; aux; aux = aux->sig)
		aux->func(aux);
}

/*!
 * @desc: Inicia un proceso.
 	Durante la ejecución del programa, puede ser necesario procesar una cantidad de datos suficiente como para no detener el programa.
 	Estas funciones son un principio de hilos o "threads", pero en su forma más primitiva.
 	Por eso tiene un contador que permite partir o dividir el proceso de datos en partes.
 * @params: $func [in] Función a ejecutar.
	Es importante que la propia función especifique próximo tiempo TS para volver a ser ejecutada.
	Cuando se llame a esta función se le pasará como primer parámetro el proceso que la llama.
 * @ex: ProcFunc(MiProc)
 {
 	if (proc->proc == 50) //Se ha ejecutado 50 veces
 	{
 		proc->proc = 0;
 		proc->time = time(NULL) + 3600; //No se volverá a ejecutar hasta pasada 1 hora
 	}
 	else
 		proc->proc++;
 	return 0;
}
	...
	IniciaProceso(MiProc);
 * @ver: DetieneProceso
 * @cat: Procesos
 !*/

void IniciaProceso(int (*func)())
{
	Proc *proc;
	for (proc = procs; proc; proc = proc->sig)
	{
		if (proc->func == func)
			return;
	}
	proc = (Proc *)Malloc(sizeof(Proc));
	proc->func = func;
	proc->proc = 0;
	proc->time = (time_t) 0;
	AddItem(proc, procs);
}

/*!
 * @desc: Detiene un proceso.
 * @params: $func [in] Función a detener.
 * @ret: Devuelve 1 si se detiene con éxito; 0, si no.
 * @cat: Procesos
 !*/

int DetieneProceso(int (*func)())
{
	Proc *proc, *prev = NULL;
	for (proc = procs; proc; proc = proc->sig)
	{
		if (proc->func == func)
		{
			if (prev)
				prev->sig = proc->sig;
			else
				procs = proc->sig;
			Free(proc);
			return 1;
		}
		prev = proc;
	}
	return 0;
}
/*
#define MAX 32
typedef struct
{
	double valors[MAX][MAX];
	int ordre;
}mat;
double det(mat *);
void print_mat(mat *A)
{
	int i, j;
	for (i = 0; i < A->ordre; i++)
	{
		for (j = 0; j < A->ordre; j++)
			printf("%f ", A->valors[i][j]);
		printf("\n");
	}
	printf("------\n");
}
mat menor(mat A, int fila, int col)
{
	int i, j;
	for (i = fila; i < A.ordre; i++)
	{
		for (j = 0; j < A.ordre; j++)
			A.valors[i][j] = A.valors[i+1][j];
	}
	for (j = col; j < A.ordre; j++)
	{
		for (i = 0; i < A.ordre; i++)
			A.valors[i][j] = A.valors[i][j+1];
	}
	A.ordre--;
	print_mat(A);
	return A;
}
double adjunt(mat *A, int fila, int col)
{
	return ((fila + col) % 2 ? -1 : 1)*A->valors[fila][col]*det(menor(*A, fila, col));
}
double det(mat *A)
{
	int i;
	double determ = 0;
	if (A->ordre == 2)
		return A->valors[0][0] * A->valors[1][1] - A->valors[0][1] * A->valors[1][0];
	for (i = 0; i < A->ordre; i++)
		determ += adjunt(A, i, 0);
	return determ;
}
int mainn(int argc, char *argv[])
{
	mat A;
	int i, j, ordre;
	ordre = atoi(argv[1]);
	srand(time(NULL));
	for (i = 0; i < ordre; i++)
	{
		for (j = 0; j < ordre; j++)
		{
			A.valors[i][j] = rand() % 3;
			//printf("%f ",A.valors[i][j]);
		}
	}
	//printf("\n");
	A.ordre = ordre;
	print_mat(&A);
	printf("\nDeterminant: %f", det(&A));
	return 1;
}
*/

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
 * @desc: Genera una ventana de información.
 * @params: $err [in] Tipo de información.
 		- FERR: Se trata de un error. Mostrará una aspa roja.
 		- FSQL: Se trata de una advertencia en el motor SQL.
 		- FADV: Se trata de una advertencia. Mostrará una admiración amarilla.
 		- FOK: Se trata de un información. Mostrará una "i" azúl.
 	    $error [in] Cadena con formato a mostrar.
 	    $... [in] Argumentos variables según la cadena de formato.
 * @cat: Programa
 !*/
 
void Alerta(char err, char *error, ...)
{
	char buf[BUFSIZE];
#ifdef _WIN32
	int opts = MB_OK;
#endif
	va_list vl;
	va_start(vl, error);
	ircvsprintf(buf, error, vl);
	va_end(vl);
#ifdef _WIN32
	switch (err)
	{
		case FERR:
			opts |= MB_ICONERROR;
			break;
		case FSQL:
		case FADV:
			opts |= MB_ICONWARNING;
			break;
		case FOK:
			opts |= MB_ICONINFORMATION;
			break;
	}
	MessageBox(hwMain, buf, "Colossus", opts);
#else
	switch (err)
	{
		case FERR:
			fprintf(stderr, "[ERROR: %s]\n", buf);
			break;
		case FSQL:
			fprintf(stderr, "[SQL: %s]\n", buf);
			break;
		case FADV:
			fprintf(stderr, "[Advertencia: %s]\n", buf);
			break;
		case FOK:
			fprintf(stderr, "[Ok: %s]\n", buf);
			break;
	}
#endif
}
int CargaCache()
{
	if (!SQLEsTabla(SQL_CACHE))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"valor varchar(255) default NULL, "
  			"hora int4 default '0', "
  			"owner int4 default '0', "
  			"tipo varchar(255) default NULL "
			");", PREFIJO, SQL_CACHE))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SQL_CACHE);
	}
	else
	{
		if (!SQLEsCampo(SQL_CACHE, "owner"))
			SQLQuery("ALTER TABLE %s%s ADD owner int4 NOT NULL default '0';", PREFIJO, SQL_CACHE);
		if (!SQLEsCampo(SQL_CACHE, "tipo"))
			SQLQuery("ALTER TABLE %s%s ADD tipo VARCHAR(255) default NULL;", PREFIJO, SQL_CACHE);
	}
	SQLCargaTablas();
	IniciaProceso(ProcCache);
	return 1;
}

/*!
 * @desc: Coge un valor de la Caché interna.
 * @params: $tipo [in] Tipo de datos.
 	    $item [in] Ítem a solicitar.
 	    $id [in] Dueño o propietario de esta Caché.
 * @ret: Devuelve el valor del ítem solicitado correspondiente a su dueño.
 * @ver: InsertaCache BorraCache
 * @cat: Cache
 !*/

char *CogeCache(char *tipo, char *item, int id)
{
	SQLRes *res;
	SQLRow row;
	char *tipo_c, *item_c;
	static char row0[BUFSIZE];
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	res = SQLQuery("SELECT valor,hora FROM %s%s WHERE item='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, item_c, tipo_c, id);
	Free(item_c);
	Free(tipo_c);
	if (res)
	{
		time_t hora;;
		row = SQLFetchRow(res);
		hora = (time_t)atoul(row[1]);
		if (hora && hora < time(0))
		{
			SQLFreeRes(res);
			BorraCache(tipo, item, id);
			return NULL;
		}
		else
		{
			if (BadPtr(row[0]))
			{
				SQLFreeRes(res);
				return NULL;
			}
			strncpy(row0, row[0], sizeof(row0));
			SQLFreeRes(res);
			return row0;
		}
	}
	return NULL;
}

/*!
 * @desc: Inserta un dato en la Caché interna.
 * @params: $tipo [in] Tipo de datos a insertar, a definir por el usuario.
 	    $item [in] Ítem a insertar.
 	    $off [in] Tiempo de validez, en segundos. Pasado estos segundos, esta entrada se borrará automáticamente.
 	    $id [in] Propietario o dueño de la entrada.
 	    $valor [in] Cadena con formato que corresponde al valor de ese ítem. Puede ser NULL si no tiene valor.
 	    $... [in] Argumentos variables según la cadena de formato.
 * @ver: CogeCache BorraCache
 * @cat: Cache
 !*/
 
void InsertaCache(char *tipo, char *item, int off, int id, char *valor, ...)
{
	char *tipo_c, *item_c, *valor_c = NULL, buf[BUFSIZE];
	va_list vl;
	buf[0] = '\0';
	if (!BadPtr(valor))
	{
		va_start(vl, valor);
		ircvsprintf(buf, valor, vl);
		va_end(vl);
		valor_c = SQLEscapa(buf);
	}
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	if (CogeCache(tipo, item, id))
		SQLQuery("UPDATE %s%s SET valor='%s', hora=%lu WHERE item='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, valor_c ? valor_c : item_c, off ? time(0) + off : 0, item_c, tipo_c, id);
	else
		SQLQuery("INSERT INTO %s%s (item,valor,hora,tipo,owner) values ('%s','%s',%lu,'%s',%i)", PREFIJO, SQL_CACHE, item_c, valor_c ? valor_c : item_c, off ? time(0) + off : 0, tipo_c, id);
	Free(item_c);
	Free(tipo_c);
	ircfree(valor_c);
}
int ProcCache(Proc *proc)
{
	u_long ts = time(0);
	if ((u_long)proc->time + 1800 < ts) /* lo hacemos cada 30 mins */
	{
		SQLQuery("DELETE from %s%s where hora < %lu AND hora !=0", PREFIJO, SQL_CACHE, ts);
		proc->proc = 0;
		proc->time = ts;
		return 1;
	}
	return 0;
}

/*!
 * @desc: Elimina una entrada de la Caché interna.
 * @params: $tipo [in] Tipo de datos.
 	    $item [in] Ítem a eliminar.
 	    $id [in] Dueño o propietario de la entrada.
 * @ver: CogeCache InsertaCache
 * @cat: Cache
 !*/
 
void BorraCache(char *tipo, char *item, int id)
{
	char *tipo_c, *item_c;
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	SQLQuery("DELETE FROM %s%s WHERE LOWER(item)='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, strtolower(item_c), tipo_c, id);
	Free(item_c);
	Free(tipo_c);
}
#ifdef USA_CONSOLA
#define MyErrorExit(x) { Alerta(FERR, x); exit(-1); }
void *consola_loop_principal(void *args)
{
	HANDLE hStdin;
	DWORD fdwMode, fdwSaveOldMode, cNumRead; 
	char irInBuf[128], cop[128]; 
	cop[0] = '\0';
	hStdin = GetStdHandle(STD_INPUT_HANDLE); 
	if (hStdin == INVALID_HANDLE_VALUE) 
		MyErrorExit("GetStdHandle");
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) 
		MyErrorExit("GetConsoleMode");
	fdwMode = ENABLE_PROCESSED_INPUT; 
	if (!SetConsoleMode(hStdin, fdwMode)) 
		MyErrorExit("SetConsoleMode");
	while (1)
	{
		char c;
		bzero(irInBuf, 128);
		if (!ReadConsole(hStdin, (LPVOID)irInBuf, 128, &cNumRead, NULL))
            			MyErrorExit("ReadConsoleInput");
            	textcolor(LIGHTGREEN);
		printf("%c", *irInBuf);
		textcolor(LIGHTGRAY);
            	if (*irInBuf == '\r')
            	{
            		printf("\n");
            		strcpy(conbuf, (char *) cop);
            		cop[0] = '\0';
            	}
            	else
            		strcat(cop, irInBuf);
        }
}
void parsea_comando(char *comando)
{
	if (!comando)
		return;
	if (!strcasecmp(comando, "REHASH"))
	{
		Alerta(FOK, "Refrescando servicios");
		Refresca();
	}
	/* else if (!strcasecmp(comando, "RESTART"))
	{
		Alerta(FOK, "Reiniciando servicios");
		Reinicia();
	} */
	else if (!strcasecmp(comando, "VERSION"))
	{
#ifdef UDB
		printf("\n\tv%s+UDB2.1\n\tServicios para IRC.\n\n\tTrocotronic - 2004-2005\n\thttp://www.rallados.net\n\n", COLOSSUS_VERSION);
#else
		printf("\n\tv%s\n\tServicios para IRC.\n\n\tTrocotronic - 2004-2005\n\thttp://www.rallados.net\n\n", COLOSSUS_VERSION);
#endif
	}
	else if (!strcasecmp(comando, "QUIT"))
	{
		Alerta(FOK, "Saliendo de la aplicacion");
		Sleep(1000);
		exit(0);
	}
	else if (!strcasecmp(comando, "AYUDA"))
	{
		Alerta(FOK, "Comandos disponibles:");
		Alerta(FOK, "\t-REHASH Refresca la configuracion y recarga los modulos");
		Alerta(FOK, "\t-VERSION Muestra la version actual");
		Alerta(FOK, "\t-QUIT Cierra el programa");
	}
	else
		Alerta(FERR, "Comando desconocido. Para mas informacion escriba AYUDA");
}
#endif
#define DEBUG
void Debug(char *formato, ...)
{
#ifndef DEBUG
	return;
#else
	char debugbuf[8000];
	va_list vl;
#ifdef _WIN32
	static HANDLE *conh = NULL;
	LPDWORD len = 0;
#endif
	va_start(vl, formato);
	ircvsprintf(debugbuf, formato, vl);
	va_end(vl);
	strncat(debugbuf, "\r\n", sizeof(debugbuf));
#ifdef _WIN32
	if (!conh)
	{
		if (AllocConsole())
		{
			conh = (HANDLE *)Malloc(sizeof(HANDLE));
			*conh = GetStdHandle(STD_OUTPUT_HANDLE);
		}
	}
	WriteFile(*conh, debugbuf, strlen(debugbuf), len, NULL);
#else
	fprintf(stderr, debugbuf);
#endif
#endif
}

/*!
 * @desc: Guarda en el archivo .log los datos especificados, según se haya especificado en la configuración del programa.
 * @params: $opt [in] Tipo de información.
 		- LOG_CONN: Relacionado con conexiones.
 		- LOG_SERVER: Relacionado con servidores.
 		- LOG_ERROR: Relacionado con errores.
 	    $formato [in] Cadena con formato a loguear.
 	    $... [in] Argumentos variables según cadena de formato.
 * @cat: Programa
 !*/
 
void Loguea(int opt, char *formato, ...)
{
	char buf[256], auxbuf[512];
	int fp;
	va_list vl;
	time_t tm;
	struct stat inode;
	if (!conf_log || !(conf_log->opts & opt))
		return;
	tm = time(0);
	va_start(vl, formato);
	ircvsprintf(buf, formato, vl);
	va_end(vl);
	ircsprintf(auxbuf, "(%s) > %s\n", Fecha(&tm), buf);
	if (stat(conf_log->archivo, &inode) == -1)
		return;
	if (conf_log->size && inode.st_size > conf_log->size)
		fp = open(conf_log->archivo, O_CREAT|O_WRONLY|O_TRUNC, S_IREAD|S_IWRITE);
	else
		fp = open(conf_log->archivo, O_CREAT|O_APPEND|O_WRONLY, S_IREAD|S_IWRITE);
	if (fp == -1)
		return;
	write(fp, auxbuf, strlen(auxbuf));
	close(fp);
}
#ifndef _WIN32
int Pregunta(char *preg)
{
	fprintf(stderr, "%s (s/n)\n", preg);
	if (getc(stdin) == 's')
		return 1;
	return 0;
}
#endif

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
u_long Crc32(const char *s, u_int len)
{
	u_int i;
	u_long crc32val = 0L;
	for (i = 0;  i < len;  i ++)
		crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
	return crc32val;
}
#define NUMNICKLOG 6
#define NUMNICKMAXCHAR 'z'
#define NUMNICKBASE 64
#define NUMNICKMASK 63
u_int base64toint(const char *s);
const char *inttobase64(char *buf, u_int v, u_int count);

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
char *CifraNick(char *nickname, char *pass)
{
	u_int v[2], k[2], x[2], *p;
	int cont = (conf_set->nicklen + 8) / 8, i = 0;
	char *tmpnick, tmppass[12 + 1], *nick;
	static char clave[12 + 1];
	if (nickname == NULL || pass == NULL)
		return "\0";
	tmpnick = (char *)Malloc(sizeof(char) * ((8 * (conf_set->nicklen + 8) / 8) + 1));
	nick = (char *)Malloc(sizeof(char) * (conf_set->nicklen + 1));
	strcpy(nick, nickname);
	while (nick[i] != 0)
	{
		nick[i] = ToLower(nick[i]);
		i++;
	}  
	memset(tmpnick, 0, (8 * (conf_set->nicklen + 8) / 8) + 1);
	strncpy(tmpnick, nick , (8 * (conf_set->nicklen + 8) / 8) + 1 - 1);
	memset(tmppass, 0, sizeof(tmppass));
	strncpy(tmppass, pass, sizeof(tmppass) - 1);
	strncat(tmppass, "AAAAAAAAAAAA", sizeof(tmppass) - strlen(tmppass) -1);
	x[0] = x[1] = 0;
	k[1] = base64toint(tmppass + 6);
	tmppass[6] = '\0';
	k[0] = base64toint(tmppass);
	p = (u_int *)tmpnick;
	while(cont--)
	{
		v[0] = (u_int)ntohl(*p++);
		v[1] = (u_int)ntohl(*p++);
		tea(v, k, x);
	}
	inttobase64(clave, x[0], 6);
	inttobase64(clave + 6, x[1], 6);
	Free(nick);
	Free(tmpnick);
	return clave;
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
void add_item(Item *item, Item **lista)
{
	item->sig = *lista;
	*lista = item;
}
Item *del_item(Item *item, Item **lista, char borra)
{
	Item *aux, *prev = NULL;
	for (aux = *lista; aux; aux = aux->sig)
	{
		if (aux == item)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				*lista = aux->sig;
			if (borra)
				Free(item);
			return (prev ? prev->sig : *lista);
		}
		prev = aux;
	}
	return NULL;
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
 * @desc: Busca una opción en una lista de opciones.
 * @params: $item [in] Opción a buscar.
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

int CreaClave(char *clave)
{
	int ret = 0;
	if (!(ret = atoi(clave)))
	{
		char *c;
		for (c = clave; !BadPtr(c); c++)
			ret ^= (*c * 255);
	}
	return ret;
}
int CreaSalto(char *clave)
{
	int ret = 0;
	char *c;
	for (c = clave; !BadPtr(c); c++)
		ret += *c;
	return ret;
}
u_int BytesNeededToDecode(u_int cb, u_int nEqualSigns)
{
	if(cb % 4 || !cb || nEqualSigns > 2)
		return 0;
	
	return (cb >> 2) * 3 - nEqualSigns;
}
u_int CountEqualSigns(char *lpInput)
{
	int len, i;
	u_int uEqualSigns = 0;

	if(BadPtr(lpInput))
		return 0xFFFFFFFF;

	len = strlen(lpInput);
	
	for(i = len; i > 0; i--) {
		if(lpInput[i - 1] == '=')
			uEqualSigns++;
		else
			break;
		
		if(uEqualSigns > 2)
			return 0xFFFFFFFF;
	}
	
	return uEqualSigns;
}
char *Encripta(char *cadena, char *clave)
{
	int semilla, saltos, i, m;
	static char buf[BUFSIZE];
	if (!clave || !cadena)
		return NULL;
	semilla = CreaClave(clave);
	saltos = CreaSalto(clave);
	srand(semilla);
	while (saltos--) 
		rand();
	m = strlen(cadena);
	for (i = 0; i < m; i++)
		cadena[i] ^= rand()*255;
	b64_encode(cadena, m, buf, sizeof(buf));
	return buf;
}
char *Desencripta(char *cadena, char *clave)
{
	static char buf[BUFSIZE];
	int semilla, saltos, i, m;
	m = BytesNeededToDecode(strlen(cadena), CountEqualSigns(cadena));
	b64_decode(cadena, buf, sizeof(buf));
	semilla = CreaClave(clave);
	saltos = CreaSalto(clave);
	srand(semilla);
	while (saltos--) 
		rand();
	for (i = 0; i < m; i++)
		buf[i] ^= rand()*255;
	buf[i] = 0;
	return buf;
}
char *Long2Char(u_long num)
{
	static char tmp[32];
	int i = 0;
	do
	{
		tmp[i++] = (char)num % 100;
	}while ((num /= 100));
	tmp[i] = 0;
	return tmp;
}

/* rutinas MOTD */
SOCKFUNC(MotdAbre)
{
	SockWrite(sck, OPT_CRLF, "GET /colossus/motd.txt HTTP/1.1");
	SockWrite(sck, OPT_CRLF, "Accept: */*");
	SockWrite(sck, OPT_CRLF, "Host: www.rallados.net");
	SockWrite(sck, OPT_CRLF, "");
	return 0;
}
SOCKFUNC(MotdLee)
{
	if (*data == '#')
	{
		int ver;
		if ((ver = atoi(data+1)) && ver < COLOSSUS_VERINT)
			Info("Existe una versión más nueva de Colossus. Descárguela de www.rallados.net");
		else
			Info(data+1);
	}
	return 0;
}
