/*
 * $Id: helpserv.h,v 1.2 2007/05/27 19:14:36 Trocotronic Exp $ 
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
