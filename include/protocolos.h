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
extern int carga_protocolo(Conf *);
extern void libera_protocolo(Protocolo *);
