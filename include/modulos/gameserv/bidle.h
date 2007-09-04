typedef struct _bidle Bidle;
struct _bidle
{
	Cliente *cl;
	Canal *cn;
	char *nick;
	int base;
	double paso;
	double paso_pen;
	unsigned voz;
	unsigned nocodes;
	int eventos;
	int limit_pen;
	char *canal;
	char *topic;
	int maxx;
	int maxy;
	int maxtop;
	double paso_compra;
	double paso_vende;
	struct 
	{
		struct 
		{
			char *user;
			int x, y;
		}user[4];
		int users;
		time_t tiempo;
		int x[2], y[2];
		int tipo;
		int fase;
		time_t initime;
	}quest;
	struct BidlePos
	{
		SQLRow user;
		unsigned batallado:1;
	}**pos;
};
typedef enum Penas { PEN_NICK = 0 , PEN_PART , PEN_QUIT , PEN_LOGOUT , PEN_KICK , PEN_MSG , PEN_QUEST }Penas;
#define GS_BIDLE "bidle"

extern int ProcesaBidle(Cliente *, char *);
extern int BidleSockClose();
extern int BidleEOS();
extern Bidle *bidle;

#define BIDLE_ITEMS_POS 20
#define BIDLE_ITEMS 10

#define B_PEND 0x1
#define B_ISIN 0x2
#define B_FUND 0x4
#define B_RECV 0x8
