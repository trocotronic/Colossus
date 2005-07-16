/*
 * $Id: protocolos.h,v 1.5 2005-07-16 15:25:27 Trocotronic Exp $ 
 */

typedef struct _com
{
	char *msg;
	char *tok;
	void *(*func)();
}Com;
typedef struct _proto 
{
#ifdef _WIN32
	HMODULE hprot;
#else
	void *hprot;
#endif
	char *archivo;
	char *tmparchivo;
	ModInfo *info;
	int (*carga)();
	int (*descarga)();
	//IRCFUNC(*sincroniza);
	void (*inicia)();
	SOCKFUNC(*parsea);
	Com *especiales;
	mTab *umodos;
	mTab *cmodos;
}Protocolo;
	
extern MODVAR Protocolo *protocolo;
extern int CargaProtocolo(Conf *);
extern void LiberaMemoriaProtocolo(Protocolo *);

//Macros de compatibilidad
#define MSG_NULL NULL
#define TOK_NULL NULL
#define p_null NULL
#define COM_NULL { MSG_NULL , TOK_NULL , p_null }
#define ProtMsg(x) (protocolo->especiales + x)->msg
#define ProtTok(x) (protocolo->especiales + x)->tok
#define ProtFunc(x) (protocolo->especiales + x)->func
#define P_TRIO 0
#define P_MODO_USUARIO_LOCAL 1
#define P_MODO_USUARIO_REMOTO 2
#define P_MODO_CANAL 3
#define P_CAMBIO_USUARIO_LOCAL 4
#define P_CAMBIO_USUARIO_REMOTO 5
#define P_JOIN_USUARIO_LOCAL 6
#define P_JOIN_USUARIO_REMOTO 7
#define P_PART_USUARIO_LOCAL 8
#define P_PART_USUARIO_REMOTO 9
#define P_QUIT_USUARIO_LOCAL 10
#define P_QUIT_USUARIO_REMOTO 11
#define P_NUEVO_USUARIO 12
#define P_PRIVADO 13
#define P_CAMBIO_HOST_LOCAL 14
#define P_CAMBIO_HOST_REMOTO 15
#define P_FORB_NICK 16
#define P_LAG 17
#define P_WHOIS_ESPECIAL 18
#define P_GLINE 19
#define P_KICK 20
#define P_TOPIC 21
#define P_NOTICE 22
#define P_INVITE 23
#define P_MSG_VL 24

#define ProtUmodo(x) (protocolo->umodos + x)->mode
#define ProtCmodo(x) (protocolo->cmodos + x)->mode
#define ProtUmodo_f(x) (protocolo->umodos + x)->flag
#define ProtCmodo_f(x) (protocolo->cmodos + x)->flag
#define TRIO(x) ProtFunc(P_TRIO)(x)

//estas macros corresponden a la posición dentro de la tabla
// *** NO CAMBIAR EL ORDEN ***
#define UMODE_REGNICK 	ProtUmodo(0)
#define UMODE_NETADMIN	ProtUmodo(1)
#define UMODE_OPER	ProtUmodo(2)
#define UMODE_HELPOP	ProtUmodo(3)
#define UMODE_HIDE	ProtUmodo(4)
#ifdef UDB
#define UMODE_SUSPEND	ProtUmodo(5)
#endif

#define MODE_RGSTR	ProtCmodo(0)
#define MODE_RGSTRONLY 	ProtCmodo(1)
#define MODE_OPERONLY   ProtCmodo(2)
#define MODE_ADMONLY   	ProtCmodo(3)
#define MODE_HALF	ProtCmodo(4)
#define MODE_ADM	ProtCmodo(5)
#define MODE_OWNER	ProtCmodo(6)

#define UMODEF_REGNICK 	ProtUmodo_f(0)
#define UMODEF_NETADMIN	ProtUmodo_f(1)
#define UMODEF_OPER	ProtUmodo_f(2)
#define UMODEF_HELPOP	ProtUmodo_f(3)
#define UMODEF_HIDE	ProtUmodo_f(4)
#ifdef UDB
#define UMODEF_SUSPEND	ProtUmodo_f(5)
#endif

#define MODEF_RGSTR	ProtCmodo_f(0)
#define MODEF_RGSTRONLY ProtCmodo_f(1)
#define MODEF_OPERONLY  ProtCmodo_f(2)
#define MODEF_ADMONLY   ProtCmodo_f(3)
#define MODEF_HALF	ProtCmodo_f(4)
#define MODEF_ADM	ProtCmodo_f(5)
#define MODEF_OWNER	ProtCmodo_f(6)

#ifdef ENLACE_DINAMICO
 #define PROT_INFO(name) Prot_Info
 #define PROT_CARGA(name) Prot_Carga
 #define PROT_DESCARGA(name) Prot_Descarga
 #define PROT_PARSEA(name) Prot_Parsea
 #define PROT_INICIA(name) Prot_Inicia
 #define PROT_UMODOS(name) Prot_Umodos
 #define PROT_CMODOS(name) Prot_Cmodos
 #define PROT_COMANDOS(name) Prot_Comandos
#else
 #define PROT_INFO(name) name##_Info
 #define PROT_CARGA(name) name##_Carga
 #define PROT_DESCARGA(name) name##_Descarga
 #define PROT_PARSEA(name) name##_Parsea
 #define PROT_INICIA(name) name##_Inicia
 #define PROT_UMODOS(name) name##_Umodos
 #define PROT_CMODOS(name) name##_Cmodos
 #define PROT_COMANDOS(name) name##_Comandos
#endif
