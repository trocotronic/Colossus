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
#define port_msg(x) (protocolo->especiales + x)->msg
#define port_tok(x) (protocolo->especiales + x)->tok
#define port_func(x) (protocolo->especiales + x)->func
#define port_existe(x) port_func(x)
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

#define port_umodo(x) (protocolo->umodos + x)->mode
#define port_cmodo(x) (protocolo->cmodos + x)->mode
#define port_umodo_f(x) (protocolo->umodos + x)->flag
#define port_cmodo_f(x) (protocolo->cmodos + x)->flag
#define TRIO(x) port_func(P_TRIO)(x)

//estas macros corresponden a la posición dentro de la tabla
// *** NO CAMBIAR EL ORDEN ***
#define UMODE_REGNICK 	port_umodo(0)
#define UMODE_NETADMIN	port_umodo(1)
#define UMODE_OPER	port_umodo(2)
#define UMODE_HELPOP	port_umodo(3)
#define UMODE_HIDE	port_umodo(4)
#ifdef UDB
#define UMODE_SUSPEND	port_umodo(5)
#endif

#define MODE_RGSTR	port_cmodo(0)
#define MODE_RGSTRONLY 	port_cmodo(1)
#define MODE_OPERONLY   port_cmodo(2)
#define MODE_ADMONLY   	port_cmodo(3)
#define MODE_HALF	port_cmodo(4)
#define MODE_ADM	port_cmodo(5)
#define MODE_OWNER	port_cmodo(6)

#define UMODEF_REGNICK 	port_umodo_f(0)
#define UMODEF_NETADMIN	port_umodo_f(1)
#define UMODEF_OPER	port_umodo_f(2)
#define UMODEF_HELPOP	port_umodo_f(3)
#define UMODEF_HIDE	port_umodo_f(4)
#ifdef UDB
#define UMODEF_SUSPEND	port_umodo_f(5)
#endif

#define MODEF_RGSTR	port_cmodo_f(0)
#define MODEF_RGSTRONLY port_cmodo_f(1)
#define MODEF_OPERONLY  port_cmodo_f(2)
#define MODEF_ADMONLY   port_cmodo_f(3)
#define MODEF_HALF	port_cmodo_f(4)
#define MODEF_ADM	port_cmodo_f(5)
#define MODEF_OWNER	port_cmodo_f(6)
