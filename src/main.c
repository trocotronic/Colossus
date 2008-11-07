/*
 * $Id: main.c,v 1.125 2008/06/01 22:56:45 Trocotronic Exp $
 */

#ifdef _WIN32
#include <process.h>
#include <direct.h>
#else
#include <sys/resource.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#endif
#include <signal.h>
#include <pthread.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "socksint.h"
#ifdef USA_ZLIB
#include "zip.h"
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Sock *SockActual;
Senyal *senyals[MAXSIGS];
char tokbuf[BUFSIZE];
/* gmt offset */
int t_off = 0;
#ifdef USA_CONSOLA
char conbuf[128];
HANDLE hStdin;
#endif
char spath[PMAX];
extern void BorraTemporales();
extern void EscribePid();
extern int LeePid();
extern void CpuId();
#ifndef _WIN32
int getpgid(int);
#endif
#ifdef POSIX_SIGNALS
void SetSignal(int, void (*)(int));
#endif
extern int ProcCache();
extern int IniciaIPLoc();
extern void SSLKeysInit();

/*!
 * @desc: Indica si se está refrescando el programa: 1 si lo está haciendo; 0, si no.
 * @cat: Programa
 !*/
int refrescando = 0;

int mainversion = COLOSSUS_VERINT;

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
//void CargaSignatura();
//extern void DetieneMDS();
extern int CargaCache();

#ifdef _WIN32
void LoopPrincipal(void *);
#endif
SOCKFUNC(MotdAbre);
SOCKFUNC(MotdLee);
extern int SigPostNick(Cliente *, int);
extern int SigCDestroy(Canal *);
extern int SiguienteTAsync(int);
extern char logo[];

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
	Info("Refrescando servicios...");
	refrescando = 1;
	DetieneProceso(ProcCache);
	//DetieneMDS();
	DescargaExtensiones(protocolo);
	DescargaModulos();
	DescargaProtocolo();
	DescargaConfiguracion();
	if (ParseaConfiguracion(CPATH, &config, 1) < 0)
		CierraColossus(-1);
	DistribuyeConfiguracion(&config);
	if (!SockIrcd)
		DistribuyeMe(&me);
	LlamaSenyal(SIGN_SQL, 0);
#ifdef POSIX_SIGNALS
	SetSignal(SIGHUP, Refresca);
#else
  #ifndef _WIN32
	(void)signal(SIGHUP, s_rehash);
  #endif
#endif
	IniciaProceso(ProcCache);
	SiguienteTAsync(1);
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
#ifdef _WIN32
void ColossusFin(int sig)
#else
VOIDSIG ColossusFin()
#endif
{
#ifdef _WIN32
	CleanUpSegv(sig);
#endif
	CierraColossus(0);
}
#ifdef POSIX_SIGNALS
void SetSignal(int s, void (*func)(int))
{
	struct sigaction s_act;
	bzero(&s_act, sizeof(s_act));
	s_act.sa_handler = func;
	sigemptyset(&s_act.sa_mask);
	sigaddset(&s_act.sa_mask, s);
	sigaction(s, &s_act, NULL);
}
#endif
/*!
 * @desc: Cierra el programa
 * @params: $excode [in] Indica el código de salida. Este número es el que se pasa en la llamada exit()
 * @cat: Programa
 !*/

int CierraColossus(int excode)
{
#ifndef _WIN32
	fprintf(stderr, "Cerrando Colossus... ");
#endif
	LlamaSenyal(SIGN_CLOSE, 0);
	CierraTodo();
	LiberaSQL();
#ifndef _WIN32
	fprintf(stderr, "OK\n");
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
	time_t t;
	iniciado = time(0);
#ifndef _WIN32
	if (getpgid(LeePid()) != -1)
		return 1;
#endif
	ListaSocks.abiertos = ListaSocks.tope = 0;
	for (i = 0; i < MAXSOCKS; i++)
		ListaSocks.socket[i] = NULL;
	bzero(senyals, sizeof(senyals));
	mkdir("tmp", 0700);
	getcwd(spath, sizeof(spath));
#ifdef FORCE_CORE
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
		printf("unlimit core size failed; errno = %d\n", errno);
#endif
	/* rutina del unreal */
	CpuId();
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
			case 'c':
				(void)printf("cpuid %s\n", cpid);
				exit(0);
			case 'v':
				(void)printf("%s (rv%i)\n", COLOSSUS_VERNUM, rev);
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
	fprintf(stderr, "%s", logo);
	fprintf(stderr, "\t\t" COLOSSUS_VERSION " (rv%i)\n", rev);
  #ifdef USA_ZLIB
	fprintf(stderr, "\t\t+ZLIB %s\n", zlibVersion());
  #endif
  #ifdef USA_SSL
	fprintf(stderr, "\t\t+%s\n", OPENSSL_VERSION_TEXT);
  #endif
  	fprintf(stderr, "\n");
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
	t = time(0);
	srand(t);
	t_off = -(t - mktime(gmtime(&t)));
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
#ifdef USA_SSL
	SSLInit();
	SSLKeysInit();
#endif
#ifndef _WIN32
	if (sql && sql->clientinfo)
		fprintf(stderr, "\t\t+Cliente SQL %s\n", sql->clientinfo);
	if (sql && sql->servinfo)
		fprintf(stderr, "\t\t+Servidor SQL %s\n", sql->servinfo);
	fprintf(stderr, "\n\t\tTrocotronic - http://www.redyc.com/\n");
	fprintf(stderr, "\t\t(c)2004-2008\n");
	fprintf(stderr, "\n");
	if (!nofork && fork())
		exit(0);
	EscribePid();
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
	IniciaIPLoc();
#ifdef _WIN32
	cmdLine = GetCommandLine();
#else
	margv = argv;
#endif
	if (!sql && CargaSQL())
		CierraColossus(-1);
	if (!SQLNuevaTabla(SQL_VERSIONES, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"version int default NULL, "
  		"KEY item (item) "
		");", PREFIJO, SQL_VERSIONES))
	{
		for (i = 0; i < sql->tablas; i++)
			SQLInserta(SQL_VERSIONES, sql->tabla[i].tabla, "version", "1");
	}
	SQLNuevaTabla(SQL_CONFIG, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"valor text default NULL, "
  		"KEY item (item) "
		");", PREFIJO, SQL_CONFIG);
	CargaCache();
	LlamaSenyal(SIGN_STARTUP, 0);
	LlamaSenyal(SIGN_SQL, 0);
	for (i = 0; i < sql->tablas; i++)
	{
		if (!SQLCogeRegistro(SQL_VERSIONES, sql->tabla[i].tabla, "version"))
			SQLInserta(SQL_VERSIONES, sql->tabla[i].tabla, "version", "1");
	}
	pthread_mutex_init(&mutex, NULL);
#ifdef _WIN32
	signal(SIGSEGV, ColossusFin);
	signal(SIGTERM, ColossusFin);
	signal(SIGINT, ColossusFin);
	signal(SIGABRT, ColossusFin);
	signal(SIGILL, ColossusFin);
#else
  #ifdef POSIX_SIGNALS
  	SetSignal(SIGHUP, Refresca);
  	SetSignal(SIGINT, Reinicia);
  	SetSignal(SIGTERM, ColossusFin);
  	SetSignal(SIGKILL, ColossusFin);
  	SetSignal(SIGQUIT, ColossusFin);
  	SetSignal(SIGPIPE, AbreSockIrcd);
  #elif BSD_RELIABLE_SIGNALS
	signal(SIGHUP, Refresca);
	signal(SIGTERM, ColossusFin);
	signal(SIGINT, Reinicia);
	signal(SIGPIPE, AbreSockIrcd);
  #endif
#endif
	//CargaSignatura();
	SockOpen("colossus.redyc.com", 80, MotdAbre, MotdLee, NULL, NULL);
	SiguienteTAsync(1);
#ifdef _WIN32
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
