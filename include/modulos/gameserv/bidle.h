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
};
typedef enum Penas { PEN_NICK = 0 , PEN_PART , PEN_QUIT , PEN_LOGOUT , PEN_KICK , PEN_MSG , PEN_QUEST }Penas;
#define GS_BIDLE "bidle"

extern int ProcesaBidle(Cliente *, char *);
extern int BidleSockClose();
extern int BidleEOS();
extern Bidle *bidle;
