typedef struct _iploc IPLoc;
typedef struct _iplocrec IPLocRec;
typedef struct _iplocrecl IPLocRecl;

struct _iploc
{
	char *city;
	char *state;
	char *country;
	char *isp;
	char *countrycode;
	char *areacode;
	char *postalcode;
	char *dmacode;
	char *latitude;
	char *longitude;
};
struct _iplocrec
{
	IPLocRec *sig;
	Sock *sck;
	IPLoc *iloc;
	int (*func)(int, char *, IPLoc *);
	char *ip;
	char *tmp;
};
struct _iplocrecl
{
	IPLocRecl *sig;
	struct _cliente *cl;
	char *ip;
};

extern int IPLocResolv(char *, int (*)(int, char *, IPLoc *));
extern void IPLocFree(IPLoc *);
