/*
 * $Id: version.c.tpl,v 1.2 2007/11/10 18:43:16 Trocotronic Exp $ 
 */

#ifndef __TIMESTAMP__
#define __TIMESTAMP__ __DATE__ " " __TIME__
#endif

#ifndef _WIN32
const char logo[] = {
	"  ____        _                              \n"
	" / ___| ___  | |  ___   ___  ___  _   _  ___ \n"
	"| |    / _ \\ | | / _ \\ / __|/ __|| | | |/ __|\n"
	"| |___| (_) || || (_) |\\__ \\\\__ \\| |_| |\\__ \\\n"
	" \\____|\\___/ |_| \\___/ |___/|___/ \\__,_||___/\n"
	"                                             \n"
};
#endif

char *creditos[] = {
	"Este programa ha salido tras horas y horas de dedicación y entusiasmo." ,
	"Quiero agradecer a toda la gente que me ha ayudado y que ha colaborado, aportando su semilla, a que este programa vea la luz." ,
	"A todos los usuarios que lo usan que contribuyen con sugerencias, informando de fallos y mejorándolo poco a poco." ,
	" " ,
	"Puedes descargar este programa de forma gratuita en \x1F\00312http://www.redyc.com" ,
	0
};
char *creado = __TIMESTAMP__;
char *bid = NULL;
int compilacion = 0;
int rev = 0;
