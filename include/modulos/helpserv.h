/*
 * $Id: helpserv.h,v 1.1 2006-11-01 11:38:26 Trocotronic Exp $ 
 */

typedef struct _hs HelpServ;
struct _hs
{
	Modulo *hmod;
	char *canal;
	char *url_test;
	struct _castigo
	{
		int bantype;
		int minutos;
	}*castigo;
};
