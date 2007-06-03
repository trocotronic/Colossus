typedef struct _kyrhos Kyrhos;
struct _kyrhos
{
	Cliente *cl;
	char *nick;
};
#define O_GUANTES 0
#define O_PALA 1
#define O_MIGAS 2
#define O_OBJS 3
typedef struct _kyrhosobj KyrhosObj;
struct _kyrhosobj
{
	char *nombre;
	u_int pos;
	u_int defensa;
	u_int ataque;
	u_int vida;
	unsigned vestir:1;
	u_int art;
};
#define E_LAGARTO 0
#define E_ENEMS 1
typedef struct _kyrhosenemigo KyrhosEnemigo;
struct _kyrhosenemigo
{
	char *nombre;
	u_int pos;
	u_int defensa;
	u_int ataque;
	int vida;
	double experiencia;
	u_int art;
};
typedef struct _kyrhosuser KyrhosUser;
typedef struct _kyrhoslucha KyrhosLucha;
typedef struct _kyrhospos KyrhosPos;
struct _kyrhosuser
{
	struct _kyrhosuser *sig;
	Cliente *cl;
	KyrhosPos *pos;
	u_int oro;
	u_int defensa;
	u_int ataque;
	double experiencia;
	u_int objetos;
	u_int posobjs[O_OBJS];
	u_int usando;
	int vida;
	KyrhosLucha *klu;
	u_int muertos;
};
struct _kyrhoslucha
{
	KyrhosUser *kus;
	KyrhosEnemigo *ken;
	Timer *tim;
	u_int esquiv;
};
struct _kyrhospos
{
	u_int xy;
	u_int N;
	u_int E;
	u_int S;
	u_int O;
	int (*func)(KyrhosUser *);
};
#define GS_KYRHOS "kyrhos"
#define MAX_POS 16
#define P_NADA 0
#define P_NORTE 1
#define P_ESTE 2
#define P_SUR 3
#define P_OESTE 4
#define P_ENTRAR 5
#define P_SALIR 6
#define P_SUBIR 7
#define P_BAJAR 8

#define POS13G 0x10000040
#define POS14G 0x20000040

#define POS13H 0x10000080
#define POS14H 0x20000080
#define POS15H 0x40000080
#define POS16H 0x80000080

#define POS13I 0x10000100

#define POS13J 0x10000200

extern MODVAR const char NTL_toupper_tab[];
extern MODVAR const char NTL_tolower_tab[];

#define P_EXP 12 /* escalones de experiencia */

extern int ProcesaKyrhos(Cliente *, char *);
extern int KyrhosSockClose();
