/*
 * $Id: main.c,v 1.94 2007-01-18 14:10:58 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <process.h>
#include <direct.h>
#else
#include <sys/resource.h> 
#include <sys/types.h>
#endif
#include <signal.h>
#include <pthread.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "socksint.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Sock *SockActual;
Senyal *senyals[MAXSIGS];
char tokbuf[BUFSIZE];
#ifdef USA_CONSOLA
char conbuf[128];
HANDLE hStdin;
#endif
char spath[PMAX];
extern void BorraTemporales();
extern void EscribePid();
extern int LeePid();
extern void CpuId();

/*!
 * @desc: Indica si se está refrescando el programa: 1 si lo está haciendo; 0, si no.
 * @cat: Programa
 !*/
int refrescando = 0;

#ifdef _WIN32
LPCSTR cmdLine;
#else
char **margv;
#endif

/*!
 * @desc: Hora timestamp en la que se inició el programa.
 * @cat: Programa
 !*/
time_t iniciado;

extern void LeeSocks();
extern void CierraSocks();
void CargaSignatura();
extern void DetieneMDS();
extern int CargaCache();

#ifdef _WIN32
void LoopPrincipal(void *);
#endif
SOCKFUNC(MotdAbre);
SOCKFUNC(MotdLee);
extern int SigPostNick(Cliente *, int);
extern int SigCDestroy(Canal *);
extern int SiguienteTAsync(int);

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
		strlcat(tabs, "\t", sizeof(tabs));
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
			strlcat(tabs, "\t", sizeof(tabs));
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
	DetieneMDS();
	DescargaExtensiones(protocolo);
	DescargaModulos();
	DescargaProtocolo();
	DescargaConfiguracion();
	if (ParseaConfiguracion(CPATH, &config, 1) < 0)
		CierraColossus(-1);
	DistribuyeConfiguracion(&config);
	if (!SockIrcd)
		DistribuyeMe(&me);
	Senyal(SIGN_SQL);
	SiguienteTAsync(1);
#ifdef POSIX_SIGNALS
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
	if (SockIrcd)
	{
		Senyal(SIGN_SYNCH);
		Senyal(SIGN_EOS);
	}
	refrescando = 0;
}
VOIDSIG Reinicia()
{
	CierraTodo();
#ifdef _WIN32
	CleanUp();
	WinExec(cmdLine, SW_SHOWDEFAULT);
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
#ifndef _WIN32
	struct sigaction act;
#endif
	char nofork = 0;
	iniciado = time(0);
#ifndef _WIN32
	if (getpgid(LeePid()) != -1)
		return 1;
#endif
	ListaSocks.abiertos = ListaSocks.tope = 0;
	for (i = 0; i < MAXSOCKS; i++)
		ListaSocks.socket[i] = NULL;
	memset(senyals, (int)NULL, sizeof(senyals));
	mkdir("tmp", 0700);
	getcwd(spath, sizeof(spath));
#ifdef FORCE_CORE
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
		printf("unlimit core size failed; errno = %d\n", errno);
#endif
	/* rutina del unreal */
	while (--argc > 0 && (*++argv)[0] == '-') 
	{
		char *p = argv[0] + 1;
		int flag = *p++;
		if (flag == '\0' || *p == '\0') 
		{
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
#ifndef _WIN32
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
	bzero(tklines, sizeof(tklines));
	/* las primeras señales deben ser del núcleo */
	InsertaSenyal(SIGN_SYNCH, EntraBots);
	InsertaSenyal(SIGN_EOS, EntraResidentes);
	InsertaSenyal(SIGN_POST_NICK, SigPostNick);
	InsertaSenyal(SIGN_CDESTROY, SigCDestroy);
	
	if (ParseaConfiguracion(CPATH, &config, 1) < 0)
		return 1;
	DistribuyeConfiguracion(&config);
	DistribuyeMe(&me);	
	EscribePid();
#ifndef _WIN32
	if (sql && sql->clientinfo)
		fprintf(stderr, "\t\t+Cliente SQL %s\n", sql->clientinfo);
	if (sql && sql->servinfo)
		fprintf(stderr, "\t\t+Servidor SQL %s\n", sql->servinfo);
	fprintf(stderr, "\n\t\tTrocotronic - http://www.redyc.com/\n");
	fprintf(stderr, "\t\t(c)2004-2007\n");
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
	//IniciaXML();
#ifdef _WIN32
	cmdLine = GetCommandLine();
#else
	margv = argv;
#endif
	CargaCache();
	Senyal(SIGN_STARTUP);
	Senyal(SIGN_SQL);
	pthread_mutex_init(&mutex, NULL);
#ifdef _WIN32
	signal(SIGSEGV, CleanUpSegv);
#else
  #ifdef POSIX_SIGNALS
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGHUP);
	sigaction(SIGHUP, &act, NULL);
	act.sa_handler = Reinicia;
	sigaddset(&act.sa_mask, SIGINT);
	sigaction(SIGINT, &act, NULL);
	act.sa_handler = CierraColossus;
	sigaddset(&act.sa_mask, SIGTERM);
	sigaction(SIGTERM, &act, NULL);
	act.sa_handler = AbreSockIrcd;
	sigaddset(&act.sa_mask, SIGPIPE);
	sigaction(SIGPIPE, &act, NULL);
  #elif BSD_RELIABLE_SIGNALS
	signal(SIGHUP, Refresca);
	signal(SIGTERM, CierraColossus);
	signal(SIGINT, Reinicia);
	signal(SIGPIPE, AbreSockIrcd);
  #endif
#endif
	CargaSignatura();
	CpuId();
	SockOpen("colossus.redyc.com", 80, MotdAbre, MotdLee, NULL, NULL);
	SiguienteTAsync(1);
#ifndef _WIN32
	if (!nofork && fork())
		exit(0);
#else
	EscuchaIrcd();
	return 0;
}
void LoopPrincipal(void *args)
{
#endif
	IniciaProcesos();
#ifndef _WIN32
	return 0;
#endif
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
